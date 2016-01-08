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

if sys.platform == 'win32':
    env.Tool('mingw')

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
env.Install('lib', dmrfec)

dmr = env.SConscript(
    os.path.join('src', 'dmr', 'SConscript'),
    variant_dir='build/libdmr',
    duplicate=0,
)
env.Install('lib', dmr)
env.Depends(dmr, dmrfec)

dmrdump = env.SConscript(
    os.path.join('src', 'cmd', 'dmrdump', 'SConscript'),
    variant_dir='build/cmd/dmrdump',
    duplicate=0,
)
env.Install('bin', dmrdump)
env.Depends(dmrdump, dmr)

mmdvmtest = env.SConscript(
    os.path.join('src', 'cmd', 'mmdvmtest', 'SConscript'),
    variant_dir='build/cmd/mmdvmtest',
    duplicate=0,
)
env.Install('bin', mmdvmtest)
env.Depends(mmdvmtest, dmr)

noisebridge = env.SConscript(
    os.path.join('src', 'cmd', 'noisebridge', 'SConscript'),
    variant_dir='build/cmd/noisebridge',
    duplicate=0,
)
env.Install('bin', noisebridge)
env.Depends(noisebridge, dmr)
