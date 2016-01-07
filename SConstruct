import os
import sys

version = '0.0.1'

def which(binary, fatal=False):
    #sys.stdout.write('Looking for binary ' + binary + ' ... ')
    sys.stdout.write('Looking for {0} ... '.format(binary))
    for dirname in os.environ["PATH"].split(os.pathsep):
        filename = os.path.join(dirname, binary)
        if os.access(filename, os.X_OK):
            print(filename)
            return filename

    print('no')
    if fatal:
        Exit(1)

env = Environment()
env['CCFLAGS'] = [
    '-g',
    '-Iinclude',
    #'-std=c99',
    '-Wall',
    '-Wextra',
    #'-Wno-gnu-binary-literal',
    #'-pedantic',
    # https://msdn.microsoft.com/en-us/library/ms235384.aspx
    '-D_GNU_SOURCE',
    '-ffast-math',
]
env['LIBPATH'] = [
    'build',
]

required_headers = (
    'inttypes.h',
    'stdbool.h',
    'stddef.h',
    'stdio.h',
    'stdlib.h',
    'string.h',
)
compiled_libraries = {
    'libdmr':    'dmr',
    'libdmrfec': 'dmrfec',
}
linked_libraries = {
    'libdmr': [
        'dmrfec',
    ],
    'dmrdump': [
        'dmr',
        'dmrfec',
    ],
}
required_libraries = ()
required_pkgconfig = ()

if sys.platform in ('linux2', 'darwin'):
    env['CC'] = which('clang') or which('gcc')
    required_headers += (
        'getopt.h',
        'net/ethernet.h',
        'netinet/ip.h',
        'netinet/udp.h',
    )
    required_libraries += (
        ('pcap', 'pcap.h', 'c'),
    )
    required_pkgconfig += (
        'openssl',
    )
    linked_libraries['dmrdump'].extend([
        'pcap',
    ])

if sys.platform == 'darwin':
    env.Append(
        CCFLAGS=[
            '-Isupport/darwin/pcap/include',
        ],
        LINKFLAGS=[
            '-Lsupport/darwin/pcap/lib',
        ],
    )

if sys.platform == 'win32':
    Tool('mingw')(env)
    #os.environ['PATH'] += ';C:\\MinGW\\bin\\'
    #env['CC'] = which('gcc.exe')
    compiled_libraries = {
        name: value + '.dll'
        for name, value in compiled_libraries.items()
    }
    env.Append(
        CCFLAGS=[
            '-D_CRT_NONSTDC_NO_DEPRECATE',
            '-Isupport\\windows\\wpcap\\Include',
        ],
        LINKFLAGS=[
            '-static-libgcc',
        ],
        LIBPATH=[
            'support\\windows\\wpcap\\Lib',
        ],
    )
    linked_libraries['libdmr'].extend([
        'ws2_32',  # winsock2
    ])
    linked_libraries['dmrdump'].extend([
        'ws2_32',  # winsock2
        'Packet',
        'wpcap',
    ])

#env['CC'] = 'clang'
env.VariantDir('build', 'src')

binenv = env.Clone()
binenv.VariantDir('build/tool', 'tool')

if not env.GetOption('clean'):
    conf = Configure(env)
    for header in required_headers:
        if not conf.CheckHeader(header):
            print('{0} is missing'.format(header))
            Exit(1)

    for library, header, compiler in required_libraries:
        if not conf.CheckLibWithHeader(library, header, compiler):
            print('lib{0} is missing'.format(library))
            Exit(1)

for pkg in required_pkgconfig:
    env.ParseConfig('pkg-config --cflags --libs {0}'.format(pkg))
    binenv.ParseConfig('pkg-config --cflags --libs {0}'.format(pkg))

# Build targets
bits = env.SharedObject('build/shared/bits.c')
libdmrfec = env.SharedLibrary(
    'build/{libdmrfec}'.format(**compiled_libraries),
    [
        Glob('build/dmrfec/*.c'),
        bits,
    ],
)
libdmr = env.SharedLibrary(
    'build/{libdmr}'.format(**compiled_libraries),
    [
        Glob('build/sha256.c'),
        Glob('build/dmr/*.c'),
        Glob('build/dmr/buffer/*.c'),
        Glob('build/dmr/packet/*.c'),
        Glob('build/dmr/payload/*.c'),
        Glob('build/dmr/proto/*.c'),
        bits,
    ],
    LIBS=linked_libraries['libdmr'],
)
Depends(libdmr, libdmrfec)

dmrdump = binenv.Program(
    'build/dmrdump', [
        'build/tool/dmrdump.c',
    ],
    LIBS=linked_libraries['dmrdump'],
)
Depends(dmrdump, libdmr)

dmrmmdvm = binenv.Program('build/dmrmmdvm', 'build/tool/dmrmmdvm.c')
Depends(dmrmmdvm, libdmr)

noisebridge = binenv.Program('build/noisebridge', 'build/tool/noisebridge.c')
Depends(noisebridge, libdmr)
