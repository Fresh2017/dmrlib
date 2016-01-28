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
import pickle
import shlex
import subprocess
import sys
from functools import wraps
from jinja2 import Environment, FileSystemLoader


platform = sys.platform
if platform == 'linux2':
    platform = 'linux'


option_defaults = (
    ('config-cache',  'Configure cache',       'path',     'config.cache'),
    ('cross-compile', 'Cross compiler',        'prefix',   ''),
    ('cross-execute', 'Cross executer',        'command',  ''),
    ('platform',      'Target platform',       'name',     platform),
    ('prefix',        'Install prefix',        'path',     '/usr/local'),
    ('builddir',      'Build directory',       'path',     '{pwd}/build'),
    ('with-debug',    'Enable debug features', 'bool',     False),
    ('with-mbelib',   'Enable mbelib support', 'bool',     False),
)

os.environ.setdefault('CROSS_COMPILE', '')
os.environ.setdefault('CC', os.environ.get('CROSS_COMPILE') + 'gcc')

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
)
optional_libraries = (
    ('bsd',       'bsd/bsd.h'),
    ('portaudio', 'portaudio.h'),
    ('proc',      'libproc.h'),
)

optional_defines = (
    ('SO_REUSEADDR', ('sys/socket.h',), []),
    ('SO_REUSEPORT', ('sys/socket.h',), []),
)

optional_binaries = (
    ('pkg-config', ('--version',)),
)

optional_compiles = (
    # Compiler specific
    ('mingw',           os.path.join('test', 'have_mingw.c'), []),
    ('inline',          os.path.join('test', 'have_inline.c'), []),
    ('restrict',        os.path.join('test', 'have_restrict.c'), []),
    ('binary literals', os.path.join('test', 'have_binary_literals.c'), []),
    # Libc
    ('libc ipv6',       os.path.join('test', 'have_libc_ipv6.c'), []),
    ('libc scope_id',   os.path.join('test', 'have_libc_scope_id.c'), []),
    # Functions
    ('if_indextoname',  os.path.join('test', 'have_if_indextoname.c'), []),
    ('getline',         os.path.join('test', 'have_getline.c'), []),
    ('strtok_r',        os.path.join('test', 'have_strtok_r.c'), []),
    # Types
    ('socklen_t',       os.path.join('test', 'have_socklen_t.c'), []),
    # Multiplexing
    ('epoll',           os.path.join('test', 'have_epoll.c'), []),
    ('/dev/epoll',      os.path.join('test', 'have_dev_epoll.c'), []),
    ('kqueue',          os.path.join('test', 'have_kqueue.c'), []),
    ('poll',            os.path.join('test', 'have_poll.c'), []),
    ('select',          os.path.join('test', 'have_select.c'), []),
)
required_compiles = ()

generated_files = (
    os.path.join('include', 'dmr', 'config.h'),
    os.path.join('include', 'dmr', 'version.h'),
    os.path.join('src', 'cmd', 'noisebridge', 'version.h'),
    os.path.join('src', 'common', 'config.h'),
)


CONFIG_CACHE = {}
LOG_FD = None


def env_append(key, value):
    if key in os.environ:
        os.environ[key] += ' ' + value
    else:
        os.environ[key] = value


def env_name(name):
    for char in '/-. ':
        name = name.replace(char, '_')
    return name.strip('_').upper()


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


def cache(key):
    def decorate(func):
        @wraps(func)
        def _decorated(name, *args, **kwargs):
            global CONFIG_CACHE
            try:
                if CONFIG_CACHE[(key, name)]:
                    echo('checking {0} {1}... yes (cached)\n'.format(
                        key, name))
                    return True
            except KeyError:
                pass

            result = func(name, *args, **kwargs)
            if result is True:
                CONFIG_CACHE[(key, name)] = True
            return result
        return _decorated
    return decorate


def cache_load(args):
    global CONFIG_CACHE

    if args.config_cache == '':
        log('config cache: disabled')
        return True

    log('config cache: loading from {0}'.format(args.config_cache))

    if os.path.exists(args.config_cache):
        with open(args.config_cache, 'rb') as fp:
            CONFIG_CACHE.update(pickle.load(fp))

    def _cache_save():
        global CONFIG_CACHE
        with open(args.config_cache, 'wb') as fp:
            pickle.dump(CONFIG_CACHE, fp)

    atexit.register(_cache_save)
    return True


def have(name, result):
    os.environ['HAVE_' + env_name(name)] = str(int(result))


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


def _unlink(filename):
    try:
        os.unlink(filename)
    except:
        pass


def check_platform(args):
    global required_headers
    global required_libraries

    echo('checking platform... {}... '.format(sys.platform))
    if args.platform in ('darwin', 'linux'):
        required_libraries += (
            ('pthread', 'pthread.h'),
            ('pcap',    'pcap.h'),
        )
        os.environ['LIBPCAP_NAME'] = 'pcap'

    if args.platform == 'linux':
        os.environ['BINEXT'] = ''
        os.environ['ARLIBPRE'] = 'lib'
        os.environ['ARLIBEXT'] = '.a'
        os.environ['SHLIBPRE'] = 'lib'
        os.environ['SHLIBEXT'] = '.so'
        echo('ok\n')

    elif args.platform == 'darwin':
        os.environ['BINEXT'] = ''
        os.environ['ARLIBPRE'] = 'lib'
        os.environ['ARLIBEXT'] = '.a'
        os.environ['SHLIBPRE'] = 'lib'
        os.environ['SHLIBEXT'] = '.dylib'
        echo('ok\n')
        have_pkg_manager = False

        if not have_pkg_manager:
            for brew in ('/usr/local', '/opt/brew', '/opt/homebrew'):
                if os.access(os.path.join(brew, 'bin/brew'), os.X_OK):
                    echo('enabling Homebrew (http://brew.sh/) support in {0}...\n'.format(brew))
                    env_append('CFLAGS', '-I{0}/include'.format(brew))
                    env_append('LDFLAGS', '-L{0}/lib'.format(brew))
                    have_pkg_manager = True
                    break

        if not have_pkg_manager:
            for base in ('/opt/mports', '/opt/macports', '/opt/local'):
                if os.path.isdir(base):
                    echo('enabling MacPorts (https://www.macports.org) support in {0}...\n'.format(base))
                    env_append('CFLAGS', '-I{0}/include'.format(base))
                    env_append('LDFLAGS', '-L{0}/lib'.format(base))
                    have_pkg_manager = True
                    break

        if not have_pkg_manager:
            echo('no package manager was detected, it is highly recommended to get either:\n')
            echo(' - Homebrew, see http://brew.sh/\n')
            echo(' - MacPorts, see https://www.macports.org/\n')

    elif args.platform in ('win32', 'win64'):
        os.environ['BINEXT'] = '.exe'
        os.environ['ARLIBPRE'] = 'lib'
        os.environ['ARLIBEXT'] = '.a'
        os.environ['SHLIBPRE'] = ''
        os.environ['SHLIBEXT'] = '.dll'
        env_append('CFLAGS', '-I' + os.path.join('support', 'windows', 'wpcap', 'Include'))
        env_append('LDFLAGS', '-L' + os.path.join('support', 'windows', 'wpcap', 'Lib'))
        os.environ['LIBPCAP_NAME'] = 'wpcap'

        required_headers += ('windows.h',)
        required_libraries += (
            ('ws2_32', 'winsock2.h'),
            ('wpcap',  ('pcap.h', 'Packet32.h')),
        )

    else:
        echo('not supported\n'.format(sys.platform))
        return False

    return True


@cache('binary')
def check_binary(binary, args=(), optional=False):
    echo('checking binary {0}... '.format(binary))
    name = env_name(binary)
    if _test_call((binary,) + args):
        return True
    return False


def check_compiler(binary):
    echo('checking compiler {0}... '.format(binary))
    with tempfile.NamedTemporaryFile(suffix='check-compiler.c') as temp:
        temp.write('int main() { return 0; }\n\n')
        temp.flush()
        temp.seek(0)
        log('test program ({0}):\n{1}\n'.format(binary, temp.read()))
        if _test_call([
                binary,
                '-o',
                temp.name + '.o',
                temp.name,
            ]):
            os.environ['CC'] = binary
            return True


@cache('for')
def check_compile(description, filename, libs=[]):
    echo('checking for {0}... '.format(description))
    log('test program from: {0}\n'.format(filename))
    fd, temp = tempfile.mkstemp(suffix='check-compile.o')
    os.close(fd)
    try:
        args = [
            os.environ['CC'],
            '-o',
            temp,
            filename
        ] + shlex.split(os.environ.get('CFLAGS', ''))
        for lib in libs:
            args.append('-l{0}'.format(lib))

        name = env_name(description)
        os.environ['HAVE_{0}'.format(name)] = '0'
        code = _test_call_code(args)
        if code == 0:
            # Compilation success, now run
            code = _test_call_code([temp])
            if code == 0:
                echo('yes\n')
                return True

        echo('no\n')
        log('failed with exit code {0}'.format(code))
        return False
    finally:
        _unlink(temp)


@cache('define')
def check_define(define, headers=[], libs=[]):
    echo('checking define {0}... '.format(define))
    with tempfile.NamedTemporaryFile(suffix='check-define.c') as temp:
        temp.write('#include <stdlib.h>\n')
        for header in headers:
            temp.write('#include <{0}>\n'.format(header))
        temp.write('int main(int argc, char **argv) {\n')
        temp.write('#if defined({0})\n'.format(define))
        temp.write('    return 0;\n')
        temp.write('#else\n')
        temp.write('    return 1;\n')
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

        try:
            code = _test_call_code(args)
            if code == 0:
                # Compilation success, now run
                code = _test_call_code([temp.name + '.o'])
                if code == 0:
                    echo('yes\n')
                    return True

            echo('no\n')
            log('failed with exit code {0}'.format(code))
            return False
        finally:
            _unlink(temp.name + '.o')


@cache('header')
def check_header(header, optional=False):
    echo('checking header {0}... '.format(header))
    with tempfile.NamedTemporaryFile(suffix='check-header.c') as temp:
        temp.write('#include <{0}>\n'.format(header))
        temp.write('int main() { return 0; }\n\n')
        temp.seek(0)
        log('test program:\n{0}\n'.format(temp.read()))
        args = [
            os.environ['CC'],
            '-o',
            temp.name + '.o',
            temp.name,
        ] + shlex.split(os.environ.get('CFLAGS', ''))

        name = header.upper().replace('.', '_').replace('/', '_')
        try:
            if _test_call(args):
                return True

            return optional
        finally:
            _unlink(temp.name + '.o')


@cache('library')
def check_library(name, headers):
    echo('checking library {0}... '.format(name))
    with tempfile.NamedTemporaryFile(suffix='check-library.c') as temp:
        headers = headers or []
        if not isinstance(headers, (list, tuple)):
            headers = (headers,)
        for header in headers:
            temp.write('#include <{0}>\n'.format(header))
        temp.write('int main() { return 0; }\n\n')
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
        try:
            return _test_call(args)
        finally:
            _unlink(temp.name + '.o')


def check_pkg_config(package):
    echo('checking package {0}... '.format(package))
    name = env_name(package)
    return _test_call(['pkg-config', '--exists', package])


def check_versions(name):
    echo('checking versions file {0}... '.format(name))
    tag = ''
    patch_git = None
    if os.path.isdir('.git'):
        try:
            GIT_VERSION = subprocess.check_output([
                'git', 'describe', '--long', '--tags',
            ]).strip().split('-')

            tag = 'git'
            patch_git = '-'.join(GIT_VERSION[-2:])

        except subprocess.CalledProcessError:
            tag = 'git'
            pass

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
    if not check_platform(args):
        return False

    if not check_compiler(os.environ.get('CC')):
        return False

    for binary, args in optional_binaries:
        have(binary, 0)
        if check_binary(binary, args):
            have(binary, 1)

    for header in required_headers:
        if not check_header(header):
            return False
        have(header, 1)

    for header in optional_headers:
        have(header, check_header(header))

    for define, headers, libraries in optional_defines:
        headers = headers or []
        libraries = libraries or []
        have(define, check_define(define, headers, libraries))

    for name, header in required_libraries:
        if not check_library(name, header):
            return False
        have('lib' + name, True)

    for name, header in optional_libraries:
        have('lib' + name, check_library(name, header))

    for name, code, libs in optional_compiles:
        have(name, check_compile(name, code, libs))

    for name, code, libs in required_compiles:
        if not check_compile(name, code, libs):
            return False
        have(name, True)

    lua_version = None
    os.environ['LUA_USE_PKG_CONFIG'] = '0'
    if os.environ.get('HAVE_PKG_CONFIG', '') == '1':
        for version in ('lua5.3', 'lua5.2', 'lua'):
            if check_pkg_config(version):
                lua_version = version
                os.environ['LUA_USE_PKG_CONFIG'] = '1'
                have(version, 1)
                break

    if lua_version is None:
        for version in ('lua5.3', 'lua5.2', 'lua'):
            if check_library(version, ('lua.h',)):
                lua_version = version
                have(version, 1)

    if lua_version is None:
        echo('no suitable lua version could be found\n')
        return False

    os.environ['LUA_VERSION'] = lua_version

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
                help=desc + ' (default: {0})'.format(default) if default else desc,
            )

    args = parser.parse_args()

    if args.cross_compile:
        os.environ['CROSS_COMPILE'] = args.cross_compile + '-'

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

    if not cache_load(args):
        echo('configure cache load failed, see config.log for more details\n')

    if not configure(args):
        echo('configure failed, see config.log for more details\n')
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(run())
