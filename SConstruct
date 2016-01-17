import glob
import os
import sys

from scons_generate import GenerateHeader, GenerateVersions

'''
DMR_VERSION_MAJOR = '0'
DMR_VERSION_MINOR = '1'
DMR_VERSION_PATCH = '0'
DMR_VERSION_TAG   = 'beta'
if os.path.isdir('.git'):
    GIT_VERSION = subprocess.check_output([
        'git', 'describe', '--long',
    ]).strip().split('-')
    DMR_VERSION_TAG   = 'git'
    DMR_VERSION_PATCH = '-'.join(GIT_VERSION[-2:])

"""
DMR_VERSION_MAJOR=DMR_VERSION_MAJOR,
DMR_VERSION_MINOR=DMR_VERSION_MINOR,
DMR_VERSION_PATCH=DMR_VERSION_PATCH,
DMR_VERSION_TAG=DMR_VERSION_TAG,
"""

'''

env = Environment(
    BUILDDIR='#build',
    SUPPORTDIR='#support',
    CPPDEFINES=[
        '_GNU_SOURCE',
        '_REENTRANT',
    ],
    CPPPATH=[
        os.path.join('#include'),
    ],
    CCFLAGS=[
        '-Wall',
        '-Wextra',
        '-ffast-math',
    ],
)

GenerateVersions(env, 'versions')

AddOption(
    '--enable-all',
    action='store_true',
    default=False,
    help='Enable all optional features',
)
AddOption(
    '--enable-proto-mbe',
    action='store_true',
    default=False,
    help='Enable the mbe proto',
)
AddOption(
    '--with-debug',
    dest='with_debug',
    action='store_true',
    default=False,
    help='Compile with debug symbols',
)
AddOption(
    '--with-dmalloc',
    dest='with_dmalloc',
    action='store_true',
    default=False,
    help='Compile with dmalloc',
)
AddOption(
    '--with-tests',
    dest='with_tests',
    action='store_true',
    default=False,
    help='Compile with unit tests',
)

env.Append(
    ENABLE_ALL=int(GetOption('enable_all')),
    ENABLE_PROTO_MBE=int(GetOption('enable_all') or GetOption('enable_proto_mbe')),
    WITH_DEBUG=int(GetOption('with_debug')),
    WITH_DMALLOC=int(GetOption('with_dmalloc')),
)

required_libs_with_header = (
    (1, 'talloc', 'talloc.h', 'c'),
)

if GetOption('with_debug'):
    env.Append(
        CCFLAGS=[
            '-g',
        ],
        LINKFLAGS=[
            '-g',
        ],
    )

else:
    env.Append(
        CCFLAGS=[
            '-O2',
        ],
    )

if GetOption('with_dmalloc'):
    env.Append(
        CPPDEFINES=[
            'WITH_DMALLOC',
        ],
    )
    required_libs_with_header += ((0, 'dmalloc', 'dmalloc.h', 'c'),)

if sys.platform == 'darwin':
    env.ParseConfig('pkg-config --cflags --libs talloc')
    # To support Homebrew, http://brew.sh/
    env.Append(
        CPPPATH=[
            '/usr/local/include',
        ],
        LIBPATH=[
            '/usr/local/lib',
        ],
    )

if sys.platform == 'linux2':
    env.ParseConfig('pkg-config --cflags --libs talloc')

if sys.platform == 'win32':
    env.Tool('mingw')
    env.Append(
        CPPDEFINES=[
            '_CRT_NONSTDC_NO_DEPRECATE',
        ],
        LINKFLAGS=[
            '-static-libgcc',
        ],
    )

Export('env')

if GetOption('enable_all') or GetOption('enable_proto_mbe'):
    mbelib = env.SConscript(
        os.path.join('src', 'mbelib', 'SConscript'),
        variant_dir='build/mbelib',
        duplicate=0,
    )
    env.Install('dist', mbelib)

if sys.platform in ('darwin', 'linux2'):
    required_libs_with_header += (
        (1, 'pthread', 'pthread.h', 'c'),
    )

if not env.GetOption('clean'):
    conf = Configure(env)
    for required, lib, header, compiler in required_libs_with_header:
        result = {}
        if conf.CheckLibWithHeader(lib, header, compiler):
            result['HAVE_' + lib.upper()] = 1
        elif required:
            print(lib + ' is required')
            Exit(1)
        else:
            result['HAVE_' + lib.upper()] = 0

        conf.env.Append(**result)
    env = conf.Finish()

for path in (
        'include/dmr/config.h',
        'include/dmr/version.h',
        'src/cmd/noisebridge/version.h',
    ):
    env.AlwaysBuild(env.Command(path, path + '.in', GenerateHeader))


dmr = env.SConscript(
    os.path.join('src', 'dmr', 'SConscript'),
    variant_dir='build/libdmr',
    duplicate=0,
)
env.Install('dist', dmr)

if GetOption('enable_all') or GetOption('enable_proto_mbe'):
    env.Depends(dmr, mbelib)

dmrdump = env.SConscript(
    os.path.join('src', 'cmd', 'dmrdump', 'SConscript'),
    variant_dir='build/cmd/dmrdump',
    duplicate=0,
)
env.Install('dist', dmrdump)
env.Depends(dmrdump, dmr)

'''
mmdvmplay = env.SConscript(
    os.path.join('src', 'cmd', 'mmdvmplay', 'SConscript'),
    variant_dir='build/cmd/mmdvmplay',
    duplicate=0,
)
env.Install('dist', mmdvmplay)
env.Depends(mmdvmplay, dmr)

mmdvmtest = env.SConscript(
    os.path.join('src', 'cmd', 'mmdvmtest', 'SConscript'),
    variant_dir='build/cmd/mmdvmtest',
    duplicate=0,
)
env.Install('dist', mmdvmtest)
env.Depends(mmdvmtest, dmr)
'''

if GetOption('with_tests'):
    tests = env.SConscript(
        os.path.join('test', 'SConscript'),
        variant_dir='build/test',
        duplicate=0,
    )
    env.Depends(tests, dmr)

if sys.platform in ('darwin', 'linux2'):
    noisebridge = env.SConscript(
        os.path.join('src', 'cmd', 'noisebridge', 'SConscript'),
        variant_dir='build/cmd/noisebridge',
        duplicate=0,
    )
    env.Install('dist', noisebridge)
    env.Depends(noisebridge, dmr)

    '''
    noisebridge_modules = env.SConscript(
        os.path.join('src', 'cmd', 'noisebridge', 'module', 'SConscript'),
        variant_dir='build/cmd/noisebridge/module',
        duplicate=0,
    )
    env.Install(os.path.join('dist', 'module'), noisebridge_modules)
    env.Depends(noisebridge_modules, dmr)
    '''

if sys.platform == 'win32':
    for dst, src in (
            ('dist/x64', os.path.join('wpcap', 'Lib', 'x64', '*.lib')),
            ('dist',     os.path.join('wpcap', 'Lib', '*.lib')),
        ):
        env.Install(dst, Glob(os.path.join('#support', 'windows', src)))
