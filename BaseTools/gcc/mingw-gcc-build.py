#!/usr/bin/env python

## @file
#
# Automation of instructions from:
#   http://mingw-w64.svn.sourceforge.net/viewvc/mingw-w64/trunk/mingw-w64-doc/
#     howto-build/mingw-w64-howto-build.txt?revision=216&view=markup
#
# Copyright (c) 2008 - 2010, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.    The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#


from optparse import OptionParser
import os
import shutil
import subprocess
import sys
import tarfile
import urllib
import urlparse
try:
    from hashlib import md5
except Exception:
    from md5 import md5

if sys.version_info < (2, 5):
    #
    # This script (and edk2 BaseTools) require Python 2.5 or newer
    #
    print 'Python version 2.5 or later is required.'
    sys.exit(-1)

#
# Version and Copyright
#
VersionNumber = "0.01"
__version__ = "%prog Version " + VersionNumber
__copyright__ = "Copyright (c) 2008 - 2010, Intel Corporation.  All rights reserved."

class Config:
    """class Config

    Stores the configuration options for the rest of the script.

    Handles the command line options, and allows the code within
    the script to easily interact with the 'config' requested by
    the user.
    """

    def __init__(self):
        self.base_dir = os.getcwd()
        (self.options, self.args) = self.CheckOptions()
        self.__init_dirs__()

    def CheckOptions(self):
        Parser = \
            OptionParser(
                description=__copyright__,
                version=__version__,
                prog="mingw-gcc-build",
                usage="%prog [options] [target]"
                )
        Parser.add_option(
            "--arch",
            action = "store", type = "string",
            default = '',
            dest = "arch",
            help = "Processor architecture to build gcc for."
            )
        Parser.add_option(
            "--src-dir",
            action = "store", type = "string", dest = "src_dir",
            default = os.path.join(self.base_dir, 'src'),
            help = "Directory to download/extract binutils/gcc sources"
            )
        Parser.add_option(
            "--build-dir",
            action = "store", type = "string", dest = "build_dir",
            default = os.path.join(self.base_dir, 'build'),
            help = "Directory to download/extract binutils/gcc sources"
            )
        Parser.add_option(
            "--prefix",
            action = "store", type = "string", dest = "prefix",
            default = os.path.join(self.base_dir, 'install'),
            help = "Prefix to install binutils/gcc into"
            )
        Parser.add_option(
            "--skip-binutils",
            action = "store_true", dest = "skip_binutils",
            default = False,
            help = "Will skip building binutils"
            )
        Parser.add_option(
            "--skip-gcc",
            action = "store_true", dest = "skip_gcc",
            default = False,
            help = "Will skip building GCC"
            )
        Parser.add_option(
            "--symlinks",
            action = "store", type = "string", dest = "symlinks",
            default = os.path.join(self.base_dir, 'symlinks'),
            help = "Directory to create binutils/gcc symbolic links into."
            )
        Parser.add_option(
            "-v", "--verbose",
            action="store_true",
            type=None, help="Print verbose messages"
            )

        (Opt, Args) = Parser.parse_args()

        self.arch = Opt.arch.lower()
        allowedArchs = ('ia32', 'x64', 'ipf')
        if self.arch not in allowedArchs:
            Parser.error(
                'Please use --arch to specify one of: %s' %
                    ', '.join(allowedArchs)
                )
        self.target_arch = {'ia32': 'i686', 'x64': 'x86_64', 'ipf': 'ia64'}[self.arch]
        self.target_sys = {'ia32': 'pc', 'x64': 'pc', 'ipf': 'pc'}[self.arch]
        self.target_bin = {'ia32': 'mingw32', 'x64': 'mingw32', 'ipf': 'elf'}[self.arch]
        self.target_combo = '-'.join((self.target_arch, self.target_sys, self.target_bin))

        return (Opt, Args)

    def __init_dirs__(self):
        self.src_dir = os.path.realpath(os.path.expanduser(self.options.src_dir))
        self.build_dir = os.path.realpath(os.path.expanduser(self.options.build_dir))
        self.prefix = os.path.realpath(os.path.expanduser(self.options.prefix))
        self.symlinks = os.path.realpath(os.path.expanduser(self.options.symlinks))

    def IsConfigOk(self):

        building = []
        if not self.options.skip_binutils:
            building.append('binutils')
        if not self.options.skip_gcc:
            building.append('gcc')
        if len(building) == 0:
            print "Nothing will be built!"
            print
            print "Please try using --help and then change the configuration."
            return False

        print "Current directory:"
        print "   ", self.base_dir
        print "Sources download/extraction:", self.Relative(self.src_dir)
        print "Build directory            :", self.Relative(self.build_dir)
        print "Prefix (install) directory :", self.Relative(self.prefix)
        print "Create symlinks directory  :", self.Relative(self.symlinks)
        print "Building                   :", ', '.join(building)
        print
        answer = raw_input("Is this configuration ok? (default = no): ")
        if (answer.lower() not in ('y', 'yes')):
            print
            print "Please try using --help and then change the configuration."
            return False

        if self.arch.lower() == 'ipf':
            print
            print 'Please note that the IPF compiler built by this script has'
            print 'not yet been validated!'
            print
            answer = raw_input("Are you sure you want to build it? (default = no): ")
            if (answer.lower() not in ('y', 'yes')):
                print
                print "Please try using --help and then change the configuration."
                return False

        print
        return True

    def Relative(self, path):
        if path.startswith(self.base_dir):
            return '.' + path[len(self.base_dir):]
        return path

    def MakeDirs(self):
        for path in (self.src_dir, self.build_dir,self.prefix, self.symlinks):
            if not os.path.exists(path):
                os.makedirs(path)

class SourceFiles:
    """class SourceFiles

    Handles the downloading of source files used by the script.
    """

    def __init__(self, config):
        self.config = config
        self.source_files = self.source_files[config.arch]

        if config.options.skip_binutils:
            del self.source_files['binutils']

        if config.options.skip_gcc:
            del self.source_files['gcc']
            del self.source_files['mingw_hdr']

    source_files_common = {
        'binutils': {
            'url': 'http://www.kernel.org/pub/linux/devel/binutils/' + \
                   'binutils-$version.tar.bz2',
            'version': '2.20.51.0.5',
            'md5': '6d2de7cdf7a8389e70b124e3d73b4d37',
            },
        }

    source_files_x64 = {
        'gcc': {
            'url': 'http://ftpmirror.gnu.org/gcc/' + \
                   'gcc-$version/gcc-$version.tar.bz2',
            'version': '4.3.0',
            'md5': '197ed8468b38db1d3481c3111691d85b',
            },
        }

    source_files_ia32 = {
        'gcc': source_files_x64['gcc'],
        }

    source_files_ipf = source_files_x64.copy()
    source_files_ipf['gcc']['configure-params'] = (
        '--with-gnu-as', '--with-gnu-ld', '--with-newlib',
        '--verbose', '--disable-libssp', '--disable-nls',
        '--enable-languages=c,c++'
        )

    source_files = {
        'ia32': [source_files_common, source_files_ia32],
        'x64': [source_files_common, source_files_x64],
        'ipf': [source_files_common, source_files_ipf],
        }

    for arch in source_files:
        mergedSourceFiles = {}
        for source_files_dict in source_files[arch]:
            mergedSourceFiles.update(source_files_dict)
        for downloadItem in mergedSourceFiles:
            fdata = mergedSourceFiles[downloadItem]
            fdata['filename'] = fdata['url'].split('/')[-1]
            if 'extract-dir' not in fdata:
                for ext in ('.tar.gz', '.tar.bz2', '.zip'):
                    if fdata['filename'].endswith(ext):
                        fdata['extract-dir'] = fdata['filename'][:-len(ext)]
                        break
            replaceables = ('extract-dir', 'filename', 'url')
            for replaceItem in fdata:
                if replaceItem in replaceables: continue
                if type(fdata[replaceItem]) != str: continue
                for replaceable in replaceables:
                    if type(fdata[replaceable]) != str: continue
                    if replaceable in fdata:
                        fdata[replaceable] = \
                            fdata[replaceable].replace(
                                '$' + replaceItem,
                                fdata[replaceItem]
                                )
        source_files[arch] = mergedSourceFiles
    #print 'source_files:', source_files

    def GetAll(self):

        def progress(received, blockSize, fileSize):
            if fileSize < 0: return
            wDots = (100 * received * blockSize) / fileSize / 10
            if wDots > self.dots:
                for i in range(wDots - self.dots):
                    print '.',
                    sys.stdout.flush()
                    self.dots += 1

        maxRetries = 1
        for (fname, fdata) in self.source_files.items():
            for retries in range(maxRetries):
                try:
                    self.dots = 0
                    local_file = os.path.join(self.config.src_dir, fdata['filename'])
                    url = fdata['url']
                    print 'Downloading %s:' % fname, url
                    if retries > 0:
                        print '(retry)',
                    sys.stdout.flush()

                    completed = False
                    if os.path.exists(local_file):
                        md5_pass = self.checkHash(fdata)
                        if md5_pass:
                            print '[md5 match]',
                        else:
                            print '[md5 mismatch]',
                        sys.stdout.flush()
                        completed = md5_pass

                    if not completed:
                        urllib.urlretrieve(url, local_file, progress)

                    #
                    # BUGBUG: Suggest proxy to user if download fails.
                    #
                    # export http_proxy=http://proxyservername.mycompany.com:911
                    # export ftp_proxy=http://proxyservername.mycompany.com:911

                    if not completed and os.path.exists(local_file):
                        md5_pass = self.checkHash(fdata)
                        if md5_pass:
                            print '[md5 match]',
                        else:
                            print '[md5 mismatch]',
                        sys.stdout.flush()
                        completed = md5_pass

                    if completed:
                        print '[done]'
                        break
                    else:
                        print '[failed]'
                        print '  Tried to retrieve', url
                        print '  to', local_file
                        print 'Possible fixes:'
                        print '* If you are behind a web-proxy, try setting the',
                        print 'http_proxy environment variable'
                        print '* You can try to download this file separately',
                        print 'and rerun this script'
                        raise Exception()
                
                except KeyboardInterrupt:
                    print '[KeyboardInterrupt]'
                    return False

                except Exception, e:
                    print e

            if not completed: return False

        return True

    def checkHash(self, fdata):
        local_file = os.path.join(self.config.src_dir, fdata['filename'])
        expect_md5 = fdata['md5']
        data = open(local_file).read()
        md5sum = md5()
        md5sum.update(data)
        return md5sum.hexdigest().lower() == expect_md5.lower()

    def GetModules(self):
        return self.source_files.keys()

    def GetFilenameOf(self, module):
        return self.source_files[module]['filename']

    def GetMd5Of(self, module):
        return self.source_files[module]['md5']

    def GetExtractDirOf(self, module):
        return self.source_files[module]['extract-dir']

    def GetAdditionalParameters(self, module, step):
        key = step + '-params'
        if key in self.source_files[module]:
            return self.source_files[module][key]
        else:
            return tuple()

class Extracter:
    """class Extracter

    Handles the extraction of the source files from their downloaded
    archive files.
    """

    def __init__(self, source_files, config):
        self.source_files = source_files
        self.config = config

    def Extract(self, module):
        src = self.config.src_dir
        extractDst = os.path.join(src, self.config.arch)
        local_file = os.path.join(src, self.source_files.GetFilenameOf(module))
        moduleMd5 = self.source_files.GetMd5Of(module)
        extracted = os.path.join(extractDst, os.path.split(local_file)[1] + '.extracted')
        if not os.path.exists(extractDst):
            os.mkdir(extractDst)

        extractedMd5 = None
        if os.path.exists(extracted):
            extractedMd5 = open(extracted).read()

        if extractedMd5 != moduleMd5:
            print 'Extracting %s:' % self.config.Relative(local_file)
            tar = tarfile.open(local_file)
            tar.extractall(extractDst)
            open(extracted, 'w').write(moduleMd5)
        else:
            pass
            #print 'Previously extracted', self.config.Relative(local_file)

    def ExtractAll(self):
        for module in self.source_files.GetModules():
            self.Extract(module)

class Builder:
    """class Builder

    Builds and installs the GCC tool suite.
    """

    def __init__(self, source_files, config):
        self.source_files = source_files
        self.config = config

    def Build(self):
        if not self.config.options.skip_binutils:
            self.BuildModule('binutils')
        if not self.config.options.skip_gcc:
            self.BuildModule('gcc')
            self.MakeSymLinks()

    def IsBuildStepComplete(self, step):
        return \
            os.path.exists(
                os.path.join(
                    self.config.build_dir, self.config.arch, step + '.completed'
                    )
                )

    def MarkBuildStepComplete(self, step):
        open(
            os.path.join(
                self.config.build_dir, self.config.arch, step + '.completed'
                ),
            "w"
            ).close()


    def BuildModule(self, module):
        base_dir = os.getcwd()
        build_dir = os.path.join(self.config.build_dir, self.config.arch, module)
        module_dir = self.source_files.GetExtractDirOf(module)
        module_dir = os.path.realpath(os.path.join('src', self.config.arch, module_dir))
        configure = os.path.join(module_dir, 'configure')
        prefix = self.config.prefix
        if not os.path.exists(build_dir):
            os.makedirs(build_dir)
        os.chdir(build_dir)

        cmd = (
            configure,
            '--target=%s' % self.config.target_combo,
            '--prefix=' + prefix,
            '--with-sysroot=' + prefix,
            '--disable-werror',
            )
        if os.path.exists('/opt/local/include/gmp.h'):
            cmd += ('--with-gmp=/opt/local',)
        if module == 'gcc': cmd += ('--oldincludedir=/opt/local/include',)
        cmd += self.source_files.GetAdditionalParameters(module, 'configure')
        self.RunCommand(cmd, module, 'config', skipable=True)

        cmd = ('make',)
        if module == 'gcc':
            cmd += ('all-gcc',)
        self.RunCommand(cmd, module, 'build')

        cmd = ('make',)
        if module == 'gcc':
            cmd += ('install-gcc',)
        else:
            cmd += ('install',)
        self.RunCommand(cmd, module, 'install')

        os.chdir(base_dir)

        print '%s module is now built and installed' % module

    def RunCommand(self, cmd, module, stage, skipable=False):
        if skipable:
            if self.IsBuildStepComplete('%s.%s' % (module, stage)):
                return

        popen = lambda cmd: \
            subprocess.Popen(
                cmd,
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT
                )

        print '%s [%s] ...' % (module, stage),
        sys.stdout.flush()
        p = popen(cmd)
        output = p.stdout.read()
        p.wait()
        if p.returncode != 0:
            print '[failed!]'
            logFile = os.path.join(self.config.build_dir, 'log.txt')
            f = open(logFile, "w")
            f.write(output)
            f.close()
            raise Exception, 'Failed to %s %s\n' % (stage, module) + \
                'See output log at %s' % self.config.Relative(logFile)
        else:
            print '[done]'

        if skipable:
            self.MarkBuildStepComplete('%s.%s' % (module, stage))

    def MakeSymLinks(self):
        links_dir = os.path.join(self.config.symlinks, self.config.arch)
        if not os.path.exists(links_dir):
            os.makedirs(links_dir)
        startPrinted = False
        for link in ('ar', 'ld', 'gcc'):
            src = os.path.join(
                self.config.prefix, 'bin', self.config.target_combo + '-' + link
                )
            linkdst = os.path.join(links_dir, link)
            if not os.path.lexists(linkdst):
                if not startPrinted:
                    print 'Making symlinks in %s:' % self.config.Relative(links_dir),
                    startPrinted = True
                print link,
                os.symlink(src, linkdst)

        if startPrinted:
            print '[done]'

class App:
    """class App

    The main body of the application.
    """

    def __init__(self):
        config = Config()

        if not config.IsConfigOk():
            return

        config.MakeDirs()

        sources = SourceFiles(config)
        result = sources.GetAll()
        if result:
            print 'All files have been downloaded & verified'
        else:
            print 'An error occured while downloading a file'
            return

        Extracter(sources, config).ExtractAll()

        Builder(sources, config).Build()

App()

