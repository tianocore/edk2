#!/usr/bin/python
#
# Copyright (c) 2010 - 2012, Intel Corporation. All rights reserved.<BR>
#
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

release_type = 'alpha'

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

if 'TOOLCHAIN' in os.environ:
    TOOLCHAIN = os.environ['TOOLCHAIN']
else:
    TOOLCHAIN = 'GCC44'

def run_and_capture_output(args, checkExitCode = True):
    p = subprocess.Popen(args=args, stdout=subprocess.PIPE)
    stdout = p.stdout.read()
    ret_code = p.wait()
    if checkExitCode:
        assert ret_code == 0
    return stdout

def git_svn_info():
    dir = os.getcwd()
    os.chdir('OvmfPkg')
    stdout = run_and_capture_output(args=('git', 'svn', 'info'))
    os.chdir(dir)
    return stdout

def svn_info():
    dir = os.getcwd()
    os.chdir('OvmfPkg')
    stdout = run_and_capture_output(args=('svn', 'info'))
    os.chdir(dir)
    return stdout

def get_svn_info_output():
    if os.path.exists(os.path.join('OvmfPkg', '.svn')):
        return svn_info()
    else:
        return git_svn_info()

def get_revision():
    buf = get_svn_info_output()
    revision_re = re.compile('^Revision\:\s*(\d+)$', re.MULTILINE)
    mo = revision_re.search(buf)
    if mo is not None:
        return int(mo.group(1))

revision = get_revision()

newline_re = re.compile(r'(\n|\r\n|\r(?!\n))', re.MULTILINE)
def to_dos_text(str):
    return newline_re.sub('\r\n', str)

def gen_build_info():
    distro = run_and_capture_output(args=('lsb_release', '-sd')).strip()

    machine = run_and_capture_output(args=('uname', '-m')).strip()

    gcc_version = run_and_capture_output(args=('gcc', '--version'))
    gcc_version = gcc_version.split('\n')[0].split()[-1]

    ld_version = run_and_capture_output(args=('ld', '--version'))
    ld_version = ld_version.split('\n')[0].split()[-1]

    iasl_version = run_and_capture_output(args=('iasl'), checkExitCode=False)
    iasl_version = filter(lambda s: s.find(' version ') >= 0, iasl_version.split('\n'))[0]
    iasl_version = iasl_version.split(' version ')[1].strip()

    sb = StringIO.StringIO()
    print >> sb, 'edk2:    ', 'r%d' % revision
    print >> sb, 'compiler: GCC', gcc_version
    print >> sb, 'binutils:', ld_version
    print >> sb, 'iasl:    ', iasl_version
    print >> sb, 'system:  ', distro, machine.replace('_', '-')
    return to_dos_text(sb.getvalue())

LICENSE = to_dos_text(
'''This OVMF binary release is built from source code licensed under
the BSD open source license.  The BSD license is documented at
http://opensource.org/licenses/bsd-license.php, and a copy is
shown below.

One sub-component of the OVMF project is a FAT filesystem driver.  The FAT
filesystem driver code is also BSD licensed, but the code license contains
one additional term.  This license can be found at
http://sourceforge.net/apps/mediawiki/tianocore/index.php?title=Edk2-fat-driver,
and a copy is shown below (following the normal BSD license).

=== BSD license: START ===

Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in
  the documentation and/or other materials provided with the
  distribution.
* Neither the name of the Intel Corporation nor the names of its
  contributors may be used to endorse or promote products derived
  from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

=== BSD license: END ===

=== FAT filesystem driver license: START ===

Copyright (c) 2004, Intel Corporation. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in
  the documentation and/or other materials provided with the
  distribution.
* Neither the name of Intel nor the names of its
  contributors may be used to endorse or promote products derived
  from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

Additional terms:
In addition to the forgoing, redistribution and use of the code is
conditioned upon the FAT 32 File System Driver and all derivative
works thereof being used for and designed only to read and/or write
to a file system that is directly managed by an Extensible Firmware
Interface (EFI) implementation or by an emulator of an EFI
implementation.

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
    filename = 'OVMF-%s-r%d-%s.zip' % (arch, revision, release_type)
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


