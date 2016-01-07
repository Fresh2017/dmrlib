version = '0.0.1'

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
    '-ffast-math',
]
env['LIBPATH'] = [
    'build',
]
env.VariantDir('build', 'src')

binenv = env.Clone()
binenv['LIBPATH'] = [
    'build',
]
binenv.VariantDir('build/tool', 'tool')

if not env.GetOption('clean'):
    conf = Configure(env)
    for header in (
        'getopt.h',
        'inttypes.h',
        'net/ethernet.h',
        'netinet/ip.h',
        'netinet/udp.h',
        'stdbool.h',
        'stddef.h',
        'stdio.h',
        'stdlib.h',
        'string.h',
        ):
        if not conf.CheckHeader(header):
            print('{0} is missing'.format(header))
            Exit(1)

    if not conf.CheckLibWithHeader('pcap', 'pcap.h', 'c'):
        print('libpcap is missing')
        Exit(1)

env.ParseConfig('pkg-config --cflags --libs openssl')
binenv.ParseConfig('pkg-config --cflags --libs openssl')

env['LIBS'] = []
binenv['LIBS'] = [
    'dmrfec',
    'dmr',
    'pcap',
    'ssl',
]

# Build targets
bits = env.SharedObject('build/shared/bits.c')
libdmrfec = env.SharedLibrary('build/dmrfec', [
        Glob('build/dmrfec/*.c'),
        bits,
    ],
    SHLIBVERSION=version)
libdmr = env.SharedLibrary('build/dmr', [
        Glob('build/dmr/*.c'),
        Glob('build/dmr/buffer/*.c'),
        Glob('build/dmr/packet/*.c'),
        Glob('build/dmr/payload/*.c'),
        Glob('build/dmr/proto/*.c'),
        bits,
    ],
    LIBS=[
        'dmrfec',
        'crypto',
        'ssl',
    ],
    SHLIBVERSION=version)
Depends(libdmr, libdmrfec)

dmrdump = binenv.Program('build/dmrdump', 'build/tool/dmrdump.c')
Depends(dmrdump, libdmr)

dmrmmdvm = binenv.Program('build/dmrmmdvm', 'build/tool/dmrmmdvm.c')
Depends(dmrmmdvm, libdmr)

noisebridge = binenv.Program('build/noisebridge', 'build/tool/noisebridge.c')
Depends(noisebridge, libdmr)
