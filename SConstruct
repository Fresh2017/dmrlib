env = Environment()
env['CC'] = 'clang'
env['CCFLAGS'] = [
    '-Iinclude',
    '-std=c99',
    '-Wall',
    '-Wextra',
    '-D_GNU_SOURCE',
]
env['LIBPATH'] = [
    'build',
]
env.VariantDir('build', 'src')
conf = Configure(env)

# Build targets
libdmrfec = env.SharedLibrary('build/dmrfec', [
    Glob('build/dmrfec/*.c'),
    'build/dmr/bits.c'
])
libdmr = env.SharedLibrary('build/dmr', [
    Glob('build/dmr/*.c'),
    Glob('build/dmr/packet/*.c'),
    Glob('build/dmr/payload/*.c'),
    Glob('build/dmr/proto/*.c'),
], LIBS=['dmrfec'])
Depends(libdmr, libdmrfec)

binenv = env.Clone()
binenv['LIBS'] = [
    'dmrfec',
    'dmr',
    'pcap',
]
binenv['LIBPATH'] = [
    'build',
]
binenv.VariantDir('build/tool', 'tool')
dmrdump = binenv.Program('build/dmrdump', 'build/tool/dmrdump.c')
Depends(dmrdump, libdmr)
