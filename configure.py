#!/usr/bin/env python
#
# Quick and dirty configure
#

from __future__ import print_function

import atexit
import argparse
import datetime
import tempfile
import os
import shlex
import subprocess
import sys
from jinja2 import Environment, FileSystemLoader


option_defaults = (
    ('prefix',       'Install prefix',        'path', '/usr/local'),
    ('builddir',     'Build directory',       'path', '{pwd}/build'),
    ('with-debug',   'Enable debug features', 'bool', False),
    ('with-mbelib',  'Enable mbelib support', 'bool', False),
)

env_defaults = {
    'CC':     'gcc',
}

required_headers = (
    'ctype.h',
    'errno.h',
    'inttypes.h',
    'signal.h',
    'stdarg.h',
    'stdbool.h',
    'stddef.h',
    'stdio.h',
    'stdlib.h',
    'string.h',
    'time.h',
    'sys/param.h',
)
optional_headers = (
    'libgen.h',
    'arpa/inet.h',
    'net/ethernet.h',
    'netinet/ip.h',
    'netinet/udp.h',
)

required_libraries = (
    ('m',      'math.h'),
    ('talloc', 'talloc.h'),
    ('pcap',   'pcap.h'),
)
optional_libraries = (
    ('proc',   'libproc.h'),
)

optional_defines = (
    ('SO_REUSEADDR', ('sys/socket.h',), []),
    ('SO_REUSEPORT', ('sys/socket.h',), []),
)

if sys.platform in ('darwin', 'linux', 'linux2'):
    required_libraries += (('pthread', 'pthread.h'),)

elif sys.platform == 'win32':
    required_headers += ('windows.h',)
    required_libraries += (
        ('ws2_32', 'winsock2.h'),
    )

generated_files = (
    os.path.join('include', 'dmr', 'config.h'),
    os.path.join('include', 'dmr', 'version.h'),
    os.path.join('src', 'cmd', 'noisebridge', 'version.h'),
)


LOG_FD = None


def env_append(key, value):
    if key in os.environ:
        os.environ[key] += ' ' + value
    else:
        os.environ[key] = value


def log(*args):
    global LOG_FD
    LOG_FD.write('{0} {1}\n'.format(
        datetime.datetime.now(),
        ' '.join(args),
    ))
    LOG_FD.flush()


def echo(*args):
    sys.stdout.write(' '.join(args))
    sys.stdout.flush()


def _test_call_code(args):
    global LOG_FD
    log('running: {0}'.format(' '.join(args)))
    code = subprocess.call(args, stdout=LOG_FD, stderr=LOG_FD)
    log('exit code: {0}'.format(code))
    return code


def _test_call(args):
    code = _test_call_code(args)
    if code > 0:
        log('failed with exit code {0}'.format(code))
        echo('no\n')
        return False
    else:
        log('ok')
        echo('yes\n')
        return True


def check_platform():
    global required_libraries

    echo('checking platform... {}... '.format(sys.platform))
    if sys.platform in ('linux', 'linux2'):
        required_libraries += (('bsd', None),)
        echo('ok\n')

    elif sys.platform == 'darwin':
        echo('ok\n')
        echo('enabling Homebrew (http://brew.sh/) support in /usr/local...\n')
        env_append('CFLAGS', '-I/usr/local/include')
        env_append('LDFLAGS', '-L/usr/local/lib')

    else:
        echo('not supported\n'.format(sys.platform))
        return False

    return True


def check_compiler(bin):
    echo('checking compiler {0}... '.format(bin))
    with tempfile.NamedTemporaryFile(suffix='check-compiler.c') as temp:
        temp.write('int main(int argc, char **argv) { return 0; }')
        temp.flush()
        temp.seek(0)
        log('test program:\n{0}\n'.format(temp.read()))
        if _test_call([
                bin,
                '-o',
                temp.name + '.o',
                temp.name,
            ]):
            os.environ['CC'] = bin
            return True


def check_define(define, headers=[], libs=[]):
    echo('checking define {0}... '.format(define))
    with tempfile.NamedTemporaryFile(suffix='check-define.c') as temp:
        temp.write('#include <stdlib.h>\n')
        for header in headers:
            temp.write('#include <{0}>\n'.format(header))
        temp.write('int main(int argc, char **argv) {\n')
        temp.write('#if defined({0})\n'.format(define))
        temp.write('    exit(5);\n')
        temp.write('#else\n')
        temp.write('    exit(6);\n')
        temp.write('#endif\n')
        temp.write('}\n')
        temp.seek(0)
        log('test program:\n{0}\n'.format(temp.read()))
        args = [
            os.environ['CC'],
            '-o',
            temp.name + '.o',
            temp.name,
        ] + shlex.split(os.environ.get('CFLAGS', ''))
        for lib in libs:
            args.append('-l{0}'.format(lib))

        code = _test_call_code(args)
        if code == 0:
            # Compilation success, now run
            code = _test_call_code([temp.name + '.o'])
            if code == 5:
                echo('yes\n')
                os.environ['HAVE_{0}'.format(define)] = '1'
                return True

            elif code == 6:
                echo('no\n')
                os.environ['HAVE_{0}'.format(define)] = '0'
                return True

        echo('error\n')
        log('failed with exit code {0}'.format(code))
        return False


def check_header(header, optional=False):
    echo('checking header {0}... '.format(header))
    with tempfile.NamedTemporaryFile(suffix='check-header.c') as temp:
        temp.write('#include <{0}>\n'.format(header))
        temp.write('int main(int argc, char **argv) { return 0; }')
        temp.seek(0)
        log('test program:\n{0}\n'.format(temp.read()))
        args = [
            os.environ['CC'],
            '-o',
            temp.name + '.o',
            temp.name,
        ] + shlex.split(os.environ.get('CFLAGS', ''))

        name = header.upper().replace('.', '_').replace('/', '_')
        if _test_call(args):
            os.environ['HAVE_{0}'.format(name)] = '1'
            return True

        os.environ['HAVE_{0}'.format(name)] = '0'
        return optional


def check_library(name, headers):
    echo('checking library {0}... '.format(name))
    with tempfile.NamedTemporaryFile(suffix='check-library.c') as temp:
        headers = headers or []
        if not isinstance(headers, (list, tuple)):
            headers = (headers,)
        for header in headers:
            temp.write('#include <{0}>\n'.format(header))
        temp.write('int main(int argc, char **argv) { return 0; }')
        temp.seek(0)
        log('test program:\n{0}\n'.format(temp.read()))
        args = [
            os.environ['CC'],
            '-l{0}'.format(name),
            '-o',
            temp.name + '.o',
            temp.name,
        ]
        args.extend(shlex.split(os.environ.get('CFLAGS', '')))
        args.extend(shlex.split(os.environ.get('LDFLAGS', '')))
        if _test_call(args):
            os.environ['HAVE_LIB' + name.upper()] = '1'
            return True
        os.environ['HAVE_LIB' + name.upper()] = '0'
        return True


def check_versions(name):
    echo('checking versions file {0}... '.format(name))
    tag = ''
    patch_git = None
    if os.path.isdir('.git'):
        GIT_VERSION = subprocess.check_output([
            'git', 'describe', '--long',
        ]).strip().split('-')

        tag = 'git'
        patch_git = '-'.join(GIT_VERSION[-2:])

    with open(name) as fp:
        for line in fp.readlines():
            name, version = line.strip().split('=')
            major, minor, patch = version.strip().split('.')
            update = {
                    name.upper() + '_VERSION_MAJOR': major,
                    name.upper() + '_VERSION_MINOR': minor,
                    name.upper() + '_VERSION_PATCH': patch_git or patch,
                    name.upper() + '_VERSION_TAG': tag,
                    name.upper() + '_VERSION': '.'.join([
                            major, minor, patch_git or patch,
                    ]),
            }
            os.environ.update(**update)
            echo('{0}... '.format(name))

    echo('done\n')
    return True


def write_generated(args, source, target):
    echo('generating {0}... '.format(target))
    env = Environment(loader=FileSystemLoader('.'), trim_blocks=True)
    output = env.get_template(source).render(args=args, env=os.environ)
    with open(target, 'wb') as fp:
        fp.write(output)
    echo('done\n')
    return True


def write_makefile(args):
    return write_generated(args, 'Makefile.in', 'Makefile')


def configure(args):
    # Run checks
    if not check_platform():
        return False

    if not check_compiler(os.environ.get('CC', env_defaults['CC'])):
        return False

    for header in required_headers:
        if not check_header(header):
            return False
    for header in optional_headers:
        check_header(header, optional=True)

    for define, headers, libraries in optional_defines:
        headers = headers or []
        libraries = libraries or []
        if not check_define(define, headers, libraries):
            return False

    for name, header in required_libraries:
        if not check_library(name, header):
            return False

    for name, header in optional_libraries:
        os.environ['have_{0}'.format(name)] = str(int(check_library(name, header)))

    if not check_versions('versions'):
        return False

    for target in generated_files:
        if not write_generated(args, target + '.in', target):
            return False

    if not write_makefile(args):
        return False

    # We've made it!
    return True



def run():
    global LOG_FD

    parser = argparse.ArgumentParser()
    mapper = {
        'pwd': os.getcwd(),
    }
    for key, desc, kind, default in option_defaults:
        if isinstance(default, str):
            default = default.format(**mapper)
        if kind == 'bool':
            parser.add_argument(
                '--{0}'.format(key),
                default=default,
                action='store_false' if default else 'store_true',
                help=desc + ' (default: {0})'.format(['no', 'yes'][int(default)]),
            )
        else:
            parser.add_argument(
                '--{0}'.format(key),
                default=default,
                metavar=kind,
                help=desc + ' (default: {0})'.format(default),
            )

    args = parser.parse_args()

    LOG_FD = open('config.log', 'wb')
    LOG_FD.write('{0} starting {1} {2}\n'.format(
        datetime.datetime.now(),
        sys.argv[0],
        ' '.join(sys.argv[1:]),
    ))

    def _close_log():
        global LOG_FD
        LOG_FD.close()

    atexit.register(_close_log)

    if not configure(args):
        echo('configure failed, see config.log for more details')
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(run())
