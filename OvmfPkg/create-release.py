#!/usr/bin/python
#
# Copyright (c) 2010 - 2013, Intel Corporation. All rights reserved.<BR>
#
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

import os
import re
import StringIO
import subprocess
import sys
import zipfile

is_unix = not sys.platform.startswith('win')

if not is_unix:
    print "This script currently only supports unix-like systems"
    sys.exit(-1)

if os.path.exists('OvmfPkgX64.dsc'):
    os.chdir('..')

if not os.path.exists(os.path.join('OvmfPkg', 'OvmfPkgX64.dsc')):
    print "OvmfPkg/OvmfPkgX64.dsc doesn't exist"
    sys.exit(-1)

def run_and_capture_output(args, checkExitCode = True):
    p = subprocess.Popen(args=args, stdout=subprocess.PIPE)
    stdout = p.stdout.read()
    ret_code = p.wait()
    if checkExitCode:
        assert ret_code == 0
    return stdout

gcc_version = run_and_capture_output(args=('gcc', '--version'))
gcc_re = re.compile(r'\s*\S+\s+\([^\)]+?\)\s+(\d+(?:\.\d+)*)(?:\s+.*)?')
mo = gcc_re.match(gcc_version)
if not mo:
    print "Unable to find GCC version"
    sys.exit(-1)
gcc_version = map(lambda n: int(n), mo.group(1).split('.'))

if 'TOOLCHAIN' in os.environ:
    TOOLCHAIN = os.environ['TOOLCHAIN']
else:
    assert(gcc_version[0] == 4)
    minor = max(4, min(7, gcc_version[1]))
    TOOLCHAIN = 'GCC4' + str(minor)

def git_based_version():
    dir = os.getcwd()
    if not os.path.exists('.git'):
        os.chdir('OvmfPkg')
    stdout = run_and_capture_output(args=('git', 'log',
                                          '-n', '1',
                                          '--abbrev-commit'))
    regex = re.compile(r'^\s*git-svn-id:\s+\S+@(\d+)\s+[0-9a-f\-]+$',
                       re.MULTILINE)
    mo = regex.search(stdout)
    if mo:
        version = 'r' + mo.group(1)
    else:
        version = stdout.split(None, 3)[1]
    os.chdir(dir)
    return version

def svn_info():
    dir = os.getcwd()
    os.chdir('OvmfPkg')
    stdout = run_and_capture_output(args=('svn', 'info'))
    os.chdir(dir)
    return stdout

def svn_based_version():
        buf = svn_info()
        revision_re = re.compile('^Revision\:\s*([\da-f]+)$', re.MULTILINE)
        mo = revision_re.search(buf)
        assert(mo is not None)
        return 'r' + mo.group(1)

def get_revision():
    if os.path.exists(os.path.join('OvmfPkg', '.svn')):
        return svn_based_version()
    else:
        return git_based_version()

revision = get_revision()

newline_re = re.compile(r'(\n|\r\n|\r(?!\n))', re.MULTILINE)
def to_dos_text(str):
    return newline_re.sub('\r\n', str)

def gen_build_info():
    distro = run_and_capture_output(args=('lsb_release', '-sd')).strip()

    machine = run_and_capture_output(args=('uname', '-m')).strip()

    gcc_version_str = '.'.join(map(lambda v: str(v), gcc_version))

    ld_version = run_and_capture_output(args=('ld', '--version'))
    ld_version = ld_version.split('\n')[0].split()[-1]

    iasl_version = run_and_capture_output(args=('iasl'), checkExitCode=False)
    iasl_version = filter(lambda s: s.find(' version ') >= 0, iasl_version.split('\n'))[0]
    iasl_version = iasl_version.split(' version ')[1].strip()

    sb = StringIO.StringIO()
    print >> sb, 'edk2:    ', revision
    print >> sb, 'compiler: GCC', gcc_version_str, '(' + TOOLCHAIN + ')'
    print >> sb, 'binutils:', ld_version
    print >> sb, 'iasl:    ', iasl_version
    print >> sb, 'system:  ', distro, machine.replace('_', '-')
    return to_dos_text(sb.getvalue())

def read_file(filename):
    f = open(filename)
    d = f.read()
    f.close()
    return d

LICENSE = to_dos_text(
'''This OVMF binary release is built from source code licensed under
the BSD open source license.  The BSD license is documented at
http://opensource.org/licenses/bsd-license.php, and a copy is
shown below.

One sub-component of the OVMF project is a FAT filesystem driver.  The FAT
filesystem driver code is also BSD licensed, but the code license contains
one additional term.  This license can be found at
https://github.com/tianocore/tianocore.github.io/wiki/Edk2-fat-driver
and a copy is shown below (following the normal BSD license).

=== BSD license: START ===

''')

LICENSE += read_file(os.path.join('MdePkg', 'License.txt'))

LICENSE += to_dos_text(
'''
=== BSD license: END ===

=== FAT filesystem driver license: START ===

''')

LICENSE += read_file(os.path.join('FatBinPkg', 'License.txt'))

LICENSE += to_dos_text(
'''
=== FAT filesystem driver license: END ===
''')

def build(arch):
    args = (
        'OvmfPkg/build.sh',
        '-t', TOOLCHAIN,
        '-a', arch,
        '-b', 'RELEASE'
        )
    logname = 'build-%s.log' % arch
    build_log = open(logname, 'w')
    print 'Building OVMF for', arch, '(%s)' % logname, '...',
    sys.stdout.flush()
    p = subprocess.Popen(args=args, stdout=build_log, stderr=build_log)
    ret_code = p.wait()
    if ret_code == 0:
        print '[done]'
    else:
        print '[error 0x%x]' % ret_code
    return ret_code

def create_zip(arch):
    global build_info
    filename = 'OVMF-%s-%s.zip' % (arch, revision)
    print 'Creating', filename, '...',
    sys.stdout.flush()
    if os.path.exists(filename):
        os.remove(filename)
    zipf = zipfile.ZipFile(filename, 'w', zipfile.ZIP_DEFLATED)

    zipf.writestr('BUILD_INFO', build_info)
    zipf.writestr('LICENSE', LICENSE)
    zipf.write(os.path.join('OvmfPkg', 'README'), 'README')
    FV_DIR = os.path.join(
        'Build',
        'Ovmf' + arch.title(),
        'RELEASE_' + TOOLCHAIN,
        'FV'
        )
    zipf.write(os.path.join(FV_DIR, 'OVMF.fd'), 'OVMF.fd')
    zipf.close()
    print '[done]'

build_info = gen_build_info()
build('IA32')
build('X64')
create_zip('IA32')
create_zip('X64')


