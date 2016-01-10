import os
import sys

env = Environment(
    BUILDDIR='#build',
    SUPPORTDIR='#support',
    CPPDEFINES=[
        '_GNU_SOURCE',
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

AddOption(
    '--with-dmalloc',
    dest='with_dmalloc',
    action='store_true',
    default=False,
    help='Compile with dmalloc',
)

if GetOption('with_dmalloc'):
    env.Append(
        CPPDEFINES=[
            'WITH_DMALLOC',
        ],
    )

if sys.platform == 'darwin':
    # To support Homebrew, http://brew.sh/
    env.Append(
        CPPPATH=[
            '/usr/local/include',
        ],
        LIBPATH=[
            '/usr/local/lib',
        ],
    )

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

shared = env.SConscript(
    os.path.join('src', 'shared', 'SConscript'),
    variant_dir='build/shared',
    duplicate=0,
)

dmrfec = env.SConscript(
    os.path.join('src', 'dmrfec', 'SConscript'),
    variant_dir='build/libdmrfec',
    duplicate=0,
)
env.Install('dist', dmrfec)

mbelib = env.SConscript(
    os.path.join('src', 'mbelib', 'SConscript'),
    variant_dir='build/mbelib',
    duplicate=0,
)
env.Install('dist', mbelib)

dmr = env.SConscript(
    os.path.join('src', 'dmr', 'SConscript'),
    variant_dir='build/libdmr',
    duplicate=0,
)
env.Install('dist', dmr)
env.Depends(dmr, dmrfec)
env.Depends(dmr, mbelib)

dmrdump = env.SConscript(
    os.path.join('src', 'cmd', 'dmrdump', 'SConscript'),
    variant_dir='build/cmd/dmrdump',
    duplicate=0,
)
env.Install('dist', dmrdump)
env.Depends(dmrdump, dmr)

mmdvmtest = env.SConscript(
    os.path.join('src', 'cmd', 'mmdvmtest', 'SConscript'),
    variant_dir='build/cmd/mmdvmtest',
    duplicate=0,
)
env.Install('dist', mmdvmtest)
env.Depends(mmdvmtest, dmr)

if sys.platform in ('darwin', 'linux2'):
    noisebridge = env.SConscript(
        os.path.join('src', 'cmd', 'noisebridge', 'SConscript'),
        variant_dir='build/cmd/noisebridge',
        duplicate=0,
    )
    env.Install('dist', noisebridge)
    env.Depends(noisebridge, dmr)

if sys.platform == 'win32':
    for dst, src in (
            ('dist/x64', os.path.join('wpcap', 'Lib', 'x64', '*.lib')),
            ('dist',     os.path.join('wpcap', 'Lib', '*.lib')),
        ):
        env.Install(dst, Glob(os.path.join('#support', 'windows', src)))
