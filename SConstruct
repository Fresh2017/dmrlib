env = Environment()
env['CC'] = 'clang'
env['CCFLAGS'] = [
    '-g',
    '-Iinclude',
    '-std=c99',
    '-Wall',
    '-Wextra',
    '-Wno-gnu-binary-literal',
    '-pedantic',
    '-D_GNU_SOURCE',
]
env['LIBPATH'] = [
    'build',
]
env.VariantDir('build', 'src')
conf = Configure(env)

# Build targets
bits = env.SharedObject('build/shared/bits.c')
libdmrfec = env.SharedLibrary('build/dmrfec', [
    Glob('build/dmrfec/*.c'),
    bits,
])
libdmr = env.SharedLibrary('build/dmr', [
    Glob('build/dmr/*.c'),
    Glob('build/dmr/packet/*.c'),
    Glob('build/dmr/payload/*.c'),
    Glob('build/dmr/proto/*.c'),
    bits,
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
