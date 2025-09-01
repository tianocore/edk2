#!/usr/bin/python3
# SPDX-License-Identifier: BSD-2-Clause-Patent
import os
import sys
import json
import shutil
import pprint
import argparse
import subprocess

def openssl_configure(openssldir, target, ec = True):
    """ Run openssl Configure script. """
    cmdline = [
        'perl',
        'Configure',
        '--config=../UefiAsm.conf',
        '--api=1.1.1',
        '--with-rand-seed=none',
        target,
        'no-afalgeng',
        'no-aria',
        'no-async',
        'no-autoerrinit',
        'no-autoload-config',
        'no-bf',
        'no-blake2',
        'no-camellia',
        'no-capieng',
        'no-cast',
        'no-chacha',
        'no-cmac',
        'no-cmp',
        'no-cms',
        'no-ct',
        'no-deprecated',
        'no-des',
        'no-dgram',
        'no-dsa',
        'no-dso',
        'no-dtls',
        'no-dtls1-method',
        'no-dtls1_2-method',
        'no-dynamic-engine',
        'no-ec2m',
        'no-engine',
        'no-err',
        'no-filenames',
        'no-gost',
        'no-hw',
        'no-idea',
        'no-ktls',
        'no-makedepend',
        'no-module',
        'no-md4',
        'no-mdc2',
        'no-ml-dsa',
        'no-ml-kem',
        'no-multiblock',
        'no-nextprotoneg',
        'no-pic',
        'no-psk',
        'no-ocb',
        'no-ocsp',
        'no-padlockeng',
        'no-poly1305',
        'no-posix-io',
        'no-quic',
        'no-rc2',
        'no-rc4',
        'no-rc5',
        'no-rfc3779',
        'no-rmd160',
        'no-scrypt',
        'no-seed',
        'no-shared',
        'no-siphash',
        'no-siv',
        'no-slh-dsa',
        'no-sm2',
        'no-sm4',
        'no-sock',
        'no-srp',
        'no-srtp',
        'no-ssl',
        'no-ssl3-method',
        'no-ssl-trace',
        'no-static-engine',
        'no-stdio',
        'no-threads',
        'no-tls1_3',
        'no-ts',
        'no-ui-console',
        'no-whirlpool',
        'disable-legacy',
    ]
    if not ec:
        cmdline += [ 'no-ec', ]
    print('')
    print(f'# -*-  configure openssl for {target} (ec={ec})  -*-')
    rc = subprocess.run(cmdline, cwd = openssldir,
                        stdout = subprocess.PIPE,
                        stderr = subprocess.PIPE)
    if rc.returncode:
        print(rc.stdout)
        print(rc.stderr)
        sys.exit(rc.returncode)

def openssl_run_make(openssldir, target):
    """
    Run make utility to generate files or cleanup.
    Target can be either a string or a list of strings.
    """
    cmdline = [ 'make', '--silent' ]
    if isinstance(target, list):
        cmdline += target
    else:
        cmdline += [ target, ]
    rc = subprocess.run(cmdline, cwd = openssldir)
    rc.check_returncode()

def get_configdata(openssldir):
    """
    Slurp openssl config data as JSON,
    using a little perl helper script.
    """
    cmdline = [
        'perl',
        'perl2json.pl',
        openssldir,
    ]
    rc = subprocess.run(cmdline, stdout = subprocess.PIPE)
    rc.check_returncode()
    return json.loads(rc.stdout)

def is_asm(filename):
    """ Check whenevr the passed file is an assembler file """
    if filename.endswith('.s') or filename.endswith('.S'):
        return True
    return False

def copy_generated_file(src, dst):
    src_file = []
    with open(src, 'r') as fsrc:
        src_file = fsrc.readlines()
    with open(dst, 'w') as fdst:
        for lines in range(len(src_file)):
            s = src_file[lines]
            s = s.rstrip() + "\r\n"
            fdst.write(s.expandtabs())

def generate_files(openssldir, opensslgendir, asm, filelist):
    """
    Generate files, using make, and copy over the results to the
    directory tree for generated openssl files.  Creates
    subdirectories as needed.
    """
    openssl_run_make(openssldir, filelist)
    for filename in filelist:
        src = os.path.join(openssldir, filename)
        if is_asm(filename):
            """ rename MSFT asm files to .nasm """
            if 'IA32-MSFT' in asm:
                filename = filename.replace('.S', '.nasm')
            elif 'X64-MSFT' in asm:
                filename = filename.replace('.s', '.nasm')
            dst = os.path.join(opensslgendir, asm, filename)
        else:
            dst = os.path.join(opensslgendir, filename)
        os.makedirs(os.path.dirname(dst), exist_ok = True)
        copy_generated_file(src, dst)

def generate_include_files(openssldir, opensslgendir, asm, cfg):
    """ Generate openssl include files """
    print('# generate include files')
    filelist = cfg['unified_info']['generate'].keys()
    filelist = list(filter(lambda f: 'include' in f, filelist))
    generate_files(openssldir, opensslgendir, asm, filelist)

def generate_library_files(openssldir, opensslgendir, asm, cfg, obj):
    """
    Generate openssl source files for a given library.  Handles
    mostly assembler files, but a few C sources are generated too.
    """
    filelist = get_source_list(cfg, obj, True)
    if filelist:
        print(f'# generate source files for {obj}')
        generate_files(openssldir, opensslgendir, asm, filelist)

def generate_all_files(openssldir, opensslgendir, asm, cfg):
    """ Generate all files needed. """
    generate_include_files(openssldir, opensslgendir, asm, cfg)
    generate_library_files(openssldir, opensslgendir, asm, cfg, 'libcrypto')
    generate_library_files(openssldir, opensslgendir, asm, cfg, 'providers/libcommon.a')
    generate_library_files(openssldir, opensslgendir, asm, cfg, 'libssl')

def get_source_list(cfg, obj, gen):
    """
    Gets the list of source files needed to create a specific object.
     * If 'gen' is True the function returns the list of generated
       files.
     * If 'gen' is False the function returns the list of files not
       generated (which are used from the submodule directly).
    Note: Will call itself recursively to resolve nested dependencies.
    """
    sources = cfg['unified_info']['sources']
    generate = cfg['unified_info']['generate']
    srclist = []
    if sources.get(obj):
        for item in sources.get(obj):
            srclist += get_source_list(cfg, item, gen)
    else:
        is_generated = generate.get(obj) is not None
        if is_generated == gen:
            srclist += [ obj, ]
    return srclist

def asm_filter_fn(filename):
    """
    Filter asm source and define lists.  Drops files we don't want include.
    """
    exclude = [
        '/bn/',
        'OPENSSL_BN_ASM',
        'OPENSSL_IA32_SSE2',
        '/ec/',
        'ECP_NISTZ256_ASM',
        'X25519_ASM',
    ]
    for item in exclude:
        if item in filename:
            return False
    return True

def get_sources(cfg, obj, asm):
    """
    Get the list of all sources files.  Will fetch both generated
    and not generated file lists and update the paths accordingly, so
    the openssl submodule or the sub-tree for generated files is
    referenced as needed.
    """
    srclist = get_source_list(cfg, obj, False)
    genlist = get_source_list(cfg, obj, True)
    srclist = list(map(lambda x: f'$(OPENSSL_PATH)/{x}', srclist))
    c_list = list(map(lambda x: f'$(OPENSSL_GEN_PATH)/{x}',
                      filter(lambda x: not is_asm(x), genlist)))
    asm_list = list(map(lambda x: f'$(OPENSSL_GEN_PATH)/{asm}/{x}',
                        filter(is_asm, genlist)))
    asm_list = list(filter(asm_filter_fn, asm_list))
    return srclist + c_list + asm_list

def sources_filter_fn(filename):
    """
    Filter source lists.  Drops files we don't want include or
    need replace with our own uefi-specific version.
    """
    exclude = [
        'randfile.c',
        '/store/',
        '/storemgmt/',
        '/encode_decode/encode',
        '/pkcs12/',
        'statem_srvr.c',
        'extensions_srvr.c',
        'defltprov.c',
        'baseprov.c',
        'provider_predefined.c',
        'ecp_nistz256.c',
        'x86_64-gcc.c',
        'armcap.c',
    ]
    for item in exclude:
        if item in filename:
            return False
    return True

def libcrypto_sources(cfg, asm = None):
    """ Get source file list for libcrypto """
    files = get_sources(cfg, 'libcrypto', asm)
    files += get_sources(cfg, 'providers/libcommon.a', asm)
    files = list(filter(sources_filter_fn, files))
    return files

def libssl_sources(cfg, asm = None):
    """ Get source file list for libssl """
    files = get_sources(cfg, 'libssl', asm)
    files = list(filter(sources_filter_fn, files))
    return files

def update_inf(filename, sources, arch = None, defines = []):
    """
    Update inf file, replace source file list and build flags.
    """
    head = ''
    tail = ''
    state = 0

    if arch:
        section = f'Sources.{arch}'
        flags = f'OPENSSL_FLAGS_{arch}'
    else:
        section = None
        flags = f'OPENSSL_FLAGS_NOASM'
        state = 1

    # read and parse file
    with open(filename, 'r') as f:
        while True:
            line = f.readline()
            if line == '':
                break
            if state in [0, 1]:
                if flags in line:
                    (keep, replace) = line.split('=')
                    args = map(lambda x: f'-D{x}', defines)
                    head += keep + '= ' + ' '.join(args)
                    head = head.rstrip() + '\r\n'
                else:
                    head += line.rstrip() + '\r\n'
            if state == 0 and section in line:
                state = 1
            if state == 1 and 'Autogenerated files list starts here' in line:
                state = 2
            if state == 2 and 'Autogenerated files list ends here' in line:
                state = 3
            if state == 3:
                tail += line.rstrip() + '\r\n'

    # write updated file
    with open(filename, 'w') as f:
        f.write(head)
        for src in sources:
            f.write(f'  {src}\r\n')
        f.write(tail)

def update_MSFT_asm_format(asm, filelist):
    """ rename MSFT asm files to .nasm """
    if 'IA32-MSFT' in asm:
        for file_index in range(len(filelist)):
            filelist[file_index] = filelist[file_index].replace('.S', '.nasm')
    elif 'X64-MSFT' in asm:
        for file_index in range(len(filelist)):
            filelist[file_index] = filelist[file_index].replace('.s', '.nasm')

def main():
    # prepare
    os.chdir(os.path.dirname(os.path.abspath(__file__)))
    openssldir = os.path.join(os.getcwd(), 'openssl')
    opensslgendir = os.path.join(os.getcwd(), 'OpensslGen')

    # asm accel configs (see UefiAsm.conf)
    for ec in [True, False]:
        if ec:
            inf = 'OpensslLibFullAccel.inf'
            hdr = 'configuration-ec.h'
        else:
            inf = 'OpensslLibAccel.inf'
            hdr = 'configuration-noec.h'
        sources = {}
        defines = {}
        for asm in [ 'UEFI-IA32-MSFT', 'UEFI-IA32-GCC',
                     'UEFI-X64-MSFT', 'UEFI-X64-GCC',
                     'UEFI-AARCH64-GCC']:
            (uefi, arch, cc) = asm.split('-')
            archcc = f'{arch}-{cc}'

            openssl_configure(openssldir, asm, ec = ec);
            cfg = get_configdata(openssldir)
            generate_all_files(openssldir, opensslgendir, archcc, cfg)
            shutil.move(os.path.join(opensslgendir, 'include', 'openssl', 'configuration.h'),
                        os.path.join(opensslgendir, 'include', 'openssl', hdr))
            openssl_run_make(openssldir, 'distclean')

            srclist = libcrypto_sources(cfg, archcc) + libssl_sources(cfg, archcc)
            if arch in ['AARCH64']:
                sources[archcc] = list(map(lambda x: f'{x} | {cc}', filter(is_asm, srclist)))
            else:
                featureflagexp = 'gEfiCryptoPkgTokenSpaceGuid.PcdOpensslLibAssemblySourceStyleNasm'
                if cc == 'GCC':
                    featureflagexp = '!' + featureflagexp
                sources[archcc] = list(map(lambda x: f'{x} ||||{featureflagexp}', filter(is_asm, srclist)))
            update_MSFT_asm_format(archcc, sources[archcc])
            sources[arch] = list(filter(lambda x: not is_asm(x), srclist))
            defines[arch] = cfg['unified_info']['defines']['libcrypto']
            defines[arch] = list(filter(asm_filter_fn, defines[arch]))

        ia32accel = sources['IA32'] + sources['IA32-MSFT'] + sources['IA32-GCC']
        x64accel = sources['X64'] + sources['X64-MSFT'] + sources['X64-GCC']
        update_inf(inf, ia32accel, 'IA32', defines['IA32'])
        update_inf(inf, x64accel, 'X64', defines['X64'])
        aarch64accel = sources['AARCH64'] + sources['AARCH64-GCC']
        update_inf(inf, aarch64accel, 'AARCH64', defines['AARCH64'])

    # noaccel - ec enabled
    openssl_configure(openssldir, 'UEFI', ec = True);
    cfg = get_configdata(openssldir)
    generate_all_files(openssldir, opensslgendir, None, cfg)
    openssl_run_make(openssldir, 'distclean')

    defines = []
    if 'libcrypto' in cfg['unified_info']['defines']:
        defines = cfg['unified_info']['defines']['libcrypto']

    update_inf('OpensslLibFull.inf',
               libcrypto_sources(cfg) + libssl_sources(cfg),
               defines)

    # noaccel - ec disabled
    openssl_configure(openssldir, 'UEFI', ec = False);
    cfg = get_configdata(openssldir)
    generate_all_files(openssldir, opensslgendir, None, cfg)
    openssl_run_make(openssldir, 'distclean')

    update_inf('OpensslLibCrypto.inf',
               libcrypto_sources(cfg),
               None, defines)
    update_inf('OpensslLib.inf',
               libcrypto_sources(cfg) + libssl_sources(cfg),
               None, defines)

    # wrap header file
    confighdr = os.path.join(opensslgendir, 'include', 'openssl', 'configuration.h')
    with open(confighdr, 'w') as f:
        f.write('#ifdef EDK2_OPENSSL_NOEC\r\n'
                '# include "configuration-noec.h"\r\n'
                '#else\r\n'
                '# include "configuration-ec.h"\r\n'
                '#endif\r\n')

if __name__ == '__main__':
    sys.exit(main())
