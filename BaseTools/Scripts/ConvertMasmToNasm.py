# @file ConvertMasmToNasm.py
# This script assists with conversion of MASM assembly syntax to NASM
#
#  Copyright (c) 2007 - 2016, Intel Corporation. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

from __future__ import print_function

#
# Import Modules
#
import argparse
import io
import os.path
import re
import subprocess
import sys


class UnsupportedConversion(Exception):
    pass


class NoSourceFile(Exception):
    pass


class UnsupportedArch(Exception):
    unsupported = ('aarch64', 'arm', 'ebc', 'ipf')


class CommonUtils:

    # Version and Copyright
    VersionNumber = "0.01"
    __version__ = "%prog Version " + VersionNumber
    __copyright__ = "Copyright (c) 2007 - 2014, Intel Corporation. All rights reserved."
    __usage__ = "%prog [options] source.asm [destination.nasm]"

    def __init__(self, clone=None):
        if clone is None:
            self.args = self.ProcessCommandLine()
        else:
            self.args = clone.args

        self.unsupportedSyntaxSeen = False
        self.src = self.args.source
        self.keep = self.args.keep
        assert(os.path.exists(self.src))
        self.dirmode = os.path.isdir(self.src)
        srcExt = os.path.splitext(self.src)[1]
        assert (self.dirmode or srcExt != '.nasm')
        self.infmode = not self.dirmode and srcExt == '.inf'
        self.diff = self.args.diff
        self.git = self.args.git
        self.force = self.args.force

        if clone is None:
            self.rootdir = os.getcwd()
            self.DetectGit()
        else:
            self.rootdir = clone.rootdir
            self.gitdir = clone.gitdir
            self.gitemail = clone.gitemail

    def ProcessCommandLine(self):
        parser = argparse.ArgumentParser(description=self.__copyright__)
        parser.add_argument('--version', action='version',
                            version='%(prog)s ' + self.VersionNumber)
        parser.add_argument("-q", "--quiet", action="store_true",
                            help="Disable all messages except FATAL ERRORS.")
        parser.add_argument("--git", action="store_true",
                            help="Use git to create commits for each file converted")
        parser.add_argument("--keep", action="append", choices=('asm', 's'),
                            default=[],
                            help="Don't remove files with this extension")
        parser.add_argument("--diff", action="store_true",
                            help="Show diff of conversion")
        parser.add_argument("-f", "--force", action="store_true",
                            help="Force conversion even if unsupported")
        parser.add_argument('source', help='MASM input file')
        parser.add_argument('dest', nargs='?',
                            help='NASM output file (default=input.nasm; - for stdout)')

        return parser.parse_args()

    def RootRelative(self, path):
        result = path
        if result.startswith(self.rootdir):
            result = result[len(self.rootdir):]
            while len(result) > 0 and result[0] in '/\\':
                result = result[1:]
        return result

    def MatchAndSetMo(self, regexp, string):
        self.mo = regexp.match(string)
        return self.mo is not None

    def SearchAndSetMo(self, regexp, string):
        self.mo = regexp.search(string)
        return self.mo is not None

    def ReplacePreserveSpacing(self, string, find, replace):
        if len(find) >= len(replace):
            padded = replace + (' ' * (len(find) - len(replace)))
            return string.replace(find, padded)
        elif find.find(replace) >= 0:
            return string.replace(find, replace)
        else:
            lenDiff = len(replace) - len(find)
            result = string
            for i in range(lenDiff, -1, -1):
                padded = find + (' ' * i)
                result = result.replace(padded, replace)
            return result

    def DetectGit(self):
        lastpath = os.path.realpath(self.src)
        self.gitdir = None
        while True:
            path = os.path.split(lastpath)[0]
            if path == lastpath:
                self.gitemail = None
                return
            candidate = os.path.join(path, '.git')
            if os.path.isdir(candidate):
                self.gitdir = candidate
                self.gitemail = self.FormatGitEmailAddress()
                return
            lastpath = path

    def FormatGitEmailAddress(self):
        if not self.git or not self.gitdir:
            return ''

        cmd = ('git', 'config', 'user.name')
        name = self.RunAndCaptureOutput(cmd).strip()
        cmd = ('git', 'config', 'user.email')
        email = self.RunAndCaptureOutput(cmd).strip()
        if name.find(',') >= 0:
            name = '"' + name + '"'
        return name + ' <' + email + '>'

    def RunAndCaptureOutput(self, cmd, checkExitCode=True, pipeIn=None):
        if pipeIn:
            subpStdin = subprocess.PIPE
        else:
            subpStdin = None
        p = subprocess.Popen(args=cmd, stdout=subprocess.PIPE, stdin=subpStdin)
        (stdout, stderr) = p.communicate(pipeIn)
        if checkExitCode:
            if p.returncode != 0:
                print('command:', ' '.join(cmd))
                print('stdout:', stdout)
                print('stderr:', stderr)
                print('return:', p.returncode)
            assert p.returncode == 0
        return stdout.decode('utf-8', 'ignore')

    def FileUpdated(self, path):
        if not self.git or not self.gitdir:
            return

        cmd = ('git', 'add', path)
        self.RunAndCaptureOutput(cmd)

    def FileAdded(self, path):
        self.FileUpdated(path)

    def RemoveFile(self, path):
        if not self.git or not self.gitdir:
            return

        if self.ShouldKeepFile(path):
            return

        cmd = ('git', 'rm', path)
        self.RunAndCaptureOutput(cmd)

    def ShouldKeepFile(self, path):
        ext = os.path.splitext(path)[1].lower()
        if ext.startswith('.'):
            ext = ext[1:]
        return ext in self.keep

    def FileConversionFinished(self, pkg, module, src, dst):
        if not self.git or not self.gitdir:
            return

        if not self.args.quiet:
            print('Committing: Conversion of', dst)

        prefix = ' '.join(filter(lambda a: a, [pkg, module]))
        message = ''
        if self.unsupportedSyntaxSeen:
            message += 'ERROR! '
        message += '%s: Convert %s to NASM\n' % (prefix, src)
        message += '\n'
        message += 'The %s script was used to convert\n' % sys.argv[0]
        message += '%s to %s\n' % (src, dst)
        message += '\n'
        message += 'Contributed-under: TianoCore Contribution Agreement 1.0\n'
        assert(self.gitemail is not None)
        message += 'Signed-off-by: %s\n' % self.gitemail
        message = message.encode('utf-8', 'ignore')

        cmd = ('git', 'commit', '-F', '-')
        self.RunAndCaptureOutput(cmd, pipeIn=message)


class ConvertAsmFile(CommonUtils):

    def __init__(self, src, dst, clone):
        CommonUtils.__init__(self, clone)
        self.ConvertAsmFile(src, dst)
        self.FileAdded(dst)
        self.RemoveFile(src)

    def ConvertAsmFile(self, inputFile, outputFile=None):
        self.globals = set()
        self.unsupportedSyntaxSeen = False
        self.inputFilename = inputFile
        if not outputFile:
            outputFile = os.path.splitext(inputFile)[0] + '.nasm'
        self.outputFilename = outputFile

        fullSrc = os.path.realpath(inputFile)
        srcParentDir = os.path.basename(os.path.split(fullSrc)[0])
        maybeArch = srcParentDir.lower()
        if maybeArch in UnsupportedArch.unsupported:
            raise UnsupportedArch
        self.ia32 = maybeArch == 'ia32'
        self.x64 = maybeArch == 'x64'

        self.inputFileBase = os.path.basename(self.inputFilename)
        self.outputFileBase = os.path.basename(self.outputFilename)
        self.output = io.BytesIO()
        if not self.args.quiet:
            dirpath, src = os.path.split(self.inputFilename)
            dirpath = self.RootRelative(dirpath)
            dst = os.path.basename(self.outputFilename)
            print('Converting:', dirpath, src, '->', dst)
        lines = io.open(self.inputFilename).readlines()
        self.Convert(lines)
        if self.outputFilename == '-' and not self.diff:
            output_data = self.output.getvalue()
            if sys.version_info >= (3, 0):
                output_data = output_data.decode('utf-8', 'ignore')
            sys.stdout.write(output_data)
            self.output.close()
        else:
            f = io.open(self.outputFilename, 'wb')
            f.write(self.output.getvalue())
            f.close()
            self.output.close()

    endOfLineRe = re.compile(r'''
                                 \s* ( ; .* )? \n $
                             ''',
                             re.VERBOSE | re.MULTILINE
                             )
    begOfLineRe = re.compile(r'''
                                 \s*
                             ''',
                             re.VERBOSE
                             )

    def Convert(self, lines):
        self.proc = None
        self.anonLabelCount = -1
        output = self.output
        self.oldAsmEmptyLineCount = 0
        self.newAsmEmptyLineCount = 0
        for line in lines:
            mo = self.begOfLineRe.search(line)
            assert mo is not None
            self.indent = mo.group()
            lineWithoutBeginning = line[len(self.indent):]
            mo = self.endOfLineRe.search(lineWithoutBeginning)
            if mo is None:
                endOfLine = ''
            else:
                endOfLine = mo.group()
            oldAsm = line[len(self.indent):len(line) - len(endOfLine)]
            self.originalLine = line.rstrip()
            if line.strip() == '':
                self.oldAsmEmptyLineCount += 1
            self.TranslateAsm(oldAsm, endOfLine)
            if line.strip() != '':
                self.oldAsmEmptyLineCount = 0

    procDeclRe = re.compile(r'''
                                (?: ASM_PFX \s* [(] \s* )?
                                ([\w@][\w@0-9]*) \s*
                                [)]? \s+
                                PROC
                                (?: \s+ NEAR | FAR )?
                                (?: \s+ C )?
                                (?: \s+ (PUBLIC | PRIVATE) )?
                                (?: \s+ USES ( (?: \s+ \w[\w0-9]* )+ ) )?
                                \s* $
                            ''',
                            re.VERBOSE | re.IGNORECASE
                            )

    procEndRe = re.compile(r'''
                               ([\w@][\w@0-9]*) \s+
                               ENDP
                               \s* $
                           ''',
                           re.VERBOSE | re.IGNORECASE
                           )

    varAndTypeSubRe = r' (?: [\w@][\w@0-9]* ) (?: \s* : \s* \w+ )? '
    publicRe = re.compile(r'''
                              PUBLIC \s+
                              ( %s (?: \s* , \s* %s )* )
                              \s* $
                          ''' % (varAndTypeSubRe, varAndTypeSubRe),
                          re.VERBOSE | re.IGNORECASE
                          )

    varAndTypeSubRe = re.compile(varAndTypeSubRe, re.VERBOSE | re.IGNORECASE)

    macroDeclRe = re.compile(r'''
                                 ([\w@][\w@0-9]*) \s+
                                 MACRO
                                 \s* $
                             ''',
                             re.VERBOSE | re.IGNORECASE
                             )

    sectionDeclRe = re.compile(r'''
                                   ([\w@][\w@0-9]*) \s+
                                   ( SECTION | ENDS )
                                   \s* $
                               ''',
                               re.VERBOSE | re.IGNORECASE
                               )

    externRe = re.compile(r'''
                              EXTE?RN \s+ (?: C \s+ )?
                              ([\w@][\w@0-9]*) \s* : \s* (\w+)
                              \s* $
                           ''',
                          re.VERBOSE | re.IGNORECASE
                          )

    externdefRe = re.compile(r'''
                                 EXTERNDEF \s+ (?: C \s+ )?
                                 ([\w@][\w@0-9]*) \s* : \s* (\w+)
                                 \s* $
                             ''',
                             re.VERBOSE | re.IGNORECASE
                             )

    protoRe = re.compile(r'''
                             ([\w@][\w@0-9]*) \s+
                             PROTO
                             (?: \s+ .* )?
                             \s* $
                         ''',
                         re.VERBOSE | re.IGNORECASE
                         )

    defineDataRe = re.compile(r'''
                                  ([\w@][\w@0-9]*) \s+
                                  ( db | dw | dd | dq ) \s+
                                  ( .*? )
                                  \s* $
                              ''',
                              re.VERBOSE | re.IGNORECASE
                              )

    equRe = re.compile(r'''
                           ([\w@][\w@0-9]*) \s+ EQU \s+ (\S.*?)
                           \s* $
                       ''',
                       re.VERBOSE | re.IGNORECASE
                       )

    ignoreRe = re.compile(r'''
                              \. (?: const |
                                     mmx |
                                     model |
                                     xmm |
                                     x?list |
                                     [3-6]86p?
                                 ) |
                              page
                              (?: \s+ .* )?
                              \s* $
                          ''',
                          re.VERBOSE | re.IGNORECASE
                          )

    whitespaceRe = re.compile(r'\s+', re.MULTILINE)

    def TranslateAsm(self, oldAsm, endOfLine):
        assert(oldAsm.strip() == oldAsm)

        endOfLine = endOfLine.replace(self.inputFileBase, self.outputFileBase)

        oldOp = oldAsm.split()
        if len(oldOp) >= 1:
            oldOp = oldOp[0]
        else:
            oldOp = ''

        if oldAsm == '':
            newAsm = oldAsm
            self.EmitAsmWithComment(oldAsm, newAsm, endOfLine)
        elif oldOp in ('#include', ):
            newAsm = oldAsm
            self.EmitLine(oldAsm + endOfLine)
        elif oldOp.lower() in ('end', 'title', 'text'):
            newAsm = ''
            self.EmitAsmWithComment(oldAsm, newAsm, endOfLine)
        elif oldAsm.lower() == '@@:':
            self.anonLabelCount += 1
            self.EmitLine(self.anonLabel(self.anonLabelCount) + ':')
        elif self.MatchAndSetMo(self.ignoreRe, oldAsm):
            newAsm = ''
            self.EmitAsmWithComment(oldAsm, newAsm, endOfLine)
        elif oldAsm.lower() == 'ret':
            for i in range(len(self.uses) - 1, -1, -1):
                register = self.uses[i]
                self.EmitNewContent('pop     ' + register)
            newAsm = 'ret'
            self.EmitAsmWithComment(oldAsm, newAsm, endOfLine)
            self.uses = tuple()
        elif oldOp.lower() == 'lea':
            newAsm = self.ConvertLea(oldAsm)
            self.EmitAsmWithComment(oldAsm, newAsm, endOfLine)
        elif oldAsm.lower() == 'end':
            newAsm = ''
            self.EmitAsmWithComment(oldAsm, newAsm, endOfLine)
            self.uses = tuple()
        elif self.MatchAndSetMo(self.equRe, oldAsm):
            equ = self.mo.group(1)
            newAsm = '%%define %s %s' % (equ, self.mo.group(2))
            self.EmitAsmWithComment(oldAsm, newAsm, endOfLine)
        elif self.MatchAndSetMo(self.externRe, oldAsm) or \
                self.MatchAndSetMo(self.protoRe, oldAsm):
            extern = self.mo.group(1)
            self.NewGlobal(extern)
            newAsm = 'extern ' + extern
            self.EmitAsmWithComment(oldAsm, newAsm, endOfLine)
        elif self.MatchAndSetMo(self.externdefRe, oldAsm):
            newAsm = ''
            self.EmitAsmWithComment(oldAsm, newAsm, endOfLine)
        elif self.MatchAndSetMo(self.macroDeclRe, oldAsm):
            newAsm = '%%macro %s 0' % self.mo.group(1)
            self.EmitAsmWithComment(oldAsm, newAsm, endOfLine)
        elif oldOp.lower() == 'endm':
            newAsm = r'%endmacro'
            self.EmitAsmWithComment(oldAsm, newAsm, endOfLine)
        elif self.MatchAndSetMo(self.sectionDeclRe, oldAsm):
            name = self.mo.group(1)
            ty = self.mo.group(2)
            if ty.lower() == 'section':
                newAsm = '.' + name
            else:
                newAsm = ''
            self.EmitAsmWithComment(oldAsm, newAsm, endOfLine)
        elif self.MatchAndSetMo(self.procDeclRe, oldAsm):
            proc = self.proc = self.mo.group(1)
            visibility = self.mo.group(2)
            if visibility is None:
                visibility = ''
            else:
                visibility = visibility.lower()
            if visibility != 'private':
                self.NewGlobal(self.proc)
                proc = 'ASM_PFX(' + proc + ')'
                self.EmitNewContent('global ' + proc)
            newAsm = proc + ':'
            self.EmitAsmWithComment(oldAsm, newAsm, endOfLine)
            uses = self.mo.group(3)
            if uses is not None:
                uses = tuple(filter(None, uses.split()))
            else:
                uses = tuple()
            self.uses = uses
            for register in self.uses:
                self.EmitNewContent('    push    ' + register)
        elif self.MatchAndSetMo(self.procEndRe, oldAsm):
            newAsm = ''
            self.EmitAsmWithComment(oldAsm, newAsm, endOfLine)
        elif self.MatchAndSetMo(self.publicRe, oldAsm):
            publics = re.findall(self.varAndTypeSubRe, self.mo.group(1))
            publics = tuple(map(lambda p: p.split(':')[0].strip(), publics))
            for i in range(len(publics) - 1):
                name = publics[i]
                self.EmitNewContent('global ASM_PFX(%s)' % publics[i])
                self.NewGlobal(name)
            name = publics[-1]
            self.NewGlobal(name)
            newAsm = 'global ASM_PFX(%s)' % name
            self.EmitAsmWithComment(oldAsm, newAsm, endOfLine)
        elif self.MatchAndSetMo(self.defineDataRe, oldAsm):
            name = self.mo.group(1)
            ty = self.mo.group(2)
            value = self.mo.group(3)
            if value == '?':
                value = 0
            newAsm = '%s: %s %s' % (name, ty, value)
            newAsm = self.CommonConversions(newAsm)
            self.EmitAsmWithComment(oldAsm, newAsm, endOfLine)
        else:
            newAsm = self.CommonConversions(oldAsm)
            self.EmitAsmWithComment(oldAsm, newAsm, endOfLine)

    def NewGlobal(self, name):
        regex = re.compile(r'(?<![_\w\d])(?<!ASM_PFX\()(' + re.escape(name) +
                           r')(?![_\w\d])')
        self.globals.add(regex)

    def ConvertAnonymousLabels(self, oldAsm):
        newAsm = oldAsm
        anonLabel = self.anonLabel(self.anonLabelCount)
        newAsm = newAsm.replace('@b', anonLabel)
        newAsm = newAsm.replace('@B', anonLabel)
        anonLabel = self.anonLabel(self.anonLabelCount + 1)
        newAsm = newAsm.replace('@f', anonLabel)
        newAsm = newAsm.replace('@F', anonLabel)
        return newAsm

    def anonLabel(self, count):
        return '.%d' % count

    def EmitString(self, string):
        self.output.write(string.encode('utf-8', 'ignore'))

    def EmitLineWithDiff(self, old, new):
        newLine = (self.indent + new).rstrip()
        if self.diff:
            if old is None:
                print('+%s' % newLine)
            elif newLine != old:
                print('-%s' % old)
                print('+%s' % newLine)
            else:
                print('', newLine)
        if newLine != '':
            self.newAsmEmptyLineCount = 0
        self.EmitString(newLine + '\r\n')

    def EmitLine(self, string):
        self.EmitLineWithDiff(self.originalLine, string)

    def EmitNewContent(self, string):
        self.EmitLineWithDiff(None, string)

    def EmitAsmReplaceOp(self, oldAsm, oldOp, newOp, endOfLine):
        newAsm = oldAsm.replace(oldOp, newOp, 1)
        self.EmitAsmWithComment(oldAsm, newAsm, endOfLine)

    hexNumRe = re.compile(r'0*((?=[\da-f])\d*(?<=\d)[\da-f]*)h', re.IGNORECASE)

    def EmitAsmWithComment(self, oldAsm, newAsm, endOfLine):
        for glblRe in self.globals:
            newAsm = glblRe.sub(r'ASM_PFX(\1)', newAsm)

        newAsm = self.hexNumRe.sub(r'0x\1', newAsm)

        newLine = newAsm + endOfLine
        emitNewLine = ((newLine.strip() != '') or
                       ((oldAsm + endOfLine).strip() == ''))
        if emitNewLine and newLine.strip() == '':
            self.newAsmEmptyLineCount += 1
            if self.newAsmEmptyLineCount > 1:
                emitNewLine = False
        if emitNewLine:
            self.EmitLine(newLine.rstrip())
        elif self.diff:
            print('-%s' % self.originalLine)

    leaRe = re.compile(r'''
                           (lea \s+) ([\w@][\w@0-9]*) \s* , \s* (\S (?:.*\S)?)
                           \s* $
                       ''',
                       re.VERBOSE | re.IGNORECASE
                       )

    def ConvertLea(self, oldAsm):
        newAsm = oldAsm
        if self.MatchAndSetMo(self.leaRe, oldAsm):
            lea = self.mo.group(1)
            dst = self.mo.group(2)
            src = self.mo.group(3)
            if src.find('[') < 0:
                src = '[' + src + ']'
            newAsm = lea + dst + ', ' + src
        newAsm = self.CommonConversions(newAsm)
        return newAsm

    ptrRe = re.compile(r'''
                           (?<! \S )
                           ([dfq]?word|byte) \s+ (?: ptr ) (\s*)
                           (?= [[\s] )
                       ''',
                       re.VERBOSE | re.IGNORECASE
                       )

    def ConvertPtr(self, oldAsm):
        newAsm = oldAsm
        while self.SearchAndSetMo(self.ptrRe, newAsm):
            ty = self.mo.group(1)
            if ty.lower() == 'fword':
                ty = ''
            else:
                ty += self.mo.group(2)
            newAsm = newAsm[:self.mo.start(0)] + ty + newAsm[self.mo.end(0):]
        return newAsm

    labelByteRe = re.compile(r'''
                                 (?: \s+ label \s+ (?: [dfq]?word | byte ) )
                                 (?! \S )
                             ''',
                             re.VERBOSE | re.IGNORECASE
                             )

    def ConvertLabelByte(self, oldAsm):
        newAsm = oldAsm
        if self.SearchAndSetMo(self.labelByteRe, newAsm):
            newAsm = newAsm[:self.mo.start(0)] + ':' + newAsm[self.mo.end(0):]
        return newAsm

    unaryBitwiseOpRe = re.compile(r'''
                                      ( NOT )
                                      (?= \s+ \S )
                                  ''',
                                  re.VERBOSE | re.IGNORECASE
                                  )
    binaryBitwiseOpRe = re.compile(r'''
                                       ( \S \s+ )
                                       ( AND | OR | SHL | SHR )
                                       (?= \s+ \S )
                                   ''',
                                   re.VERBOSE | re.IGNORECASE
                                   )
    bitwiseOpReplacements = {
        'not': '~',
        'and': '&',
        'shl': '<<',
        'shr': '>>',
        'or': '|',
    }

    def ConvertBitwiseOp(self, oldAsm):
        newAsm = oldAsm
        while self.SearchAndSetMo(self.binaryBitwiseOpRe, newAsm):
            prefix = self.mo.group(1)
            op = self.bitwiseOpReplacements[self.mo.group(2).lower()]
            newAsm = newAsm[:self.mo.start(0)] + prefix + op + \
                newAsm[self.mo.end(0):]
        while self.SearchAndSetMo(self.unaryBitwiseOpRe, newAsm):
            op = self.bitwiseOpReplacements[self.mo.group(1).lower()]
            newAsm = newAsm[:self.mo.start(0)] + op + newAsm[self.mo.end(0):]
        return newAsm

    sectionRe = re.compile(r'''
                               \. ( code |
                                    data
                                  )
                               (?: \s+ .* )?
                               \s* $
                           ''',
                           re.VERBOSE | re.IGNORECASE
                           )

    segmentRe = re.compile(r'''
                               ( code |
                                 data )
                               (?: \s+ SEGMENT )
                               (?: \s+ .* )?
                               \s* $
                           ''',
                           re.VERBOSE | re.IGNORECASE
                           )

    def ConvertSection(self, oldAsm):
        newAsm = oldAsm
        if self.MatchAndSetMo(self.sectionRe, newAsm) or \
           self.MatchAndSetMo(self.segmentRe, newAsm):
            name = self.mo.group(1).lower()
            if name == 'code':
                if self.x64:
                    self.EmitLine('DEFAULT REL')
                name = 'text'
            newAsm = 'SECTION .' + name
        return newAsm

    fwordRe = re.compile(r'''
                             (?<! \S )
                             fword
                             (?! \S )
                         ''',
                         re.VERBOSE | re.IGNORECASE
                         )

    def FwordUnsupportedCheck(self, oldAsm):
        newAsm = oldAsm
        if self.SearchAndSetMo(self.fwordRe, newAsm):
            newAsm = self.Unsupported(newAsm, 'fword used')
        return newAsm

    __common_conversion_routines__ = (
        ConvertAnonymousLabels,
        ConvertPtr,
        FwordUnsupportedCheck,
        ConvertBitwiseOp,
        ConvertLabelByte,
        ConvertSection,
    )

    def CommonConversions(self, oldAsm):
        newAsm = oldAsm
        for conv in self.__common_conversion_routines__:
            newAsm = conv(self, newAsm)
        return newAsm

    def Unsupported(self, asm, message=None):
        if not self.force:
            raise UnsupportedConversion

        self.unsupportedSyntaxSeen = True
        newAsm = '%error conversion unsupported'
        if message:
            newAsm += '; ' + message
        newAsm += ': ' + asm
        return newAsm


class ConvertInfFile(CommonUtils):

    def __init__(self, inf, clone):
        CommonUtils.__init__(self, clone)
        self.inf = inf
        self.ScanInfAsmFiles()
        if self.infmode:
            self.ConvertInfAsmFiles()

    infSrcRe = re.compile(r'''
                              \s*
                              ( [\w@][\w@0-9/]* \.(asm|s) )
                              \s* (?: \| [^#]* )?
                              \s* (?: \# .* )?
                              $
                          ''',
                          re.VERBOSE | re.IGNORECASE
                          )

    def GetInfAsmFileMapping(self):
        srcToDst = {'order': []}
        for line in self.lines:
            line = line.rstrip()
            if self.MatchAndSetMo(self.infSrcRe, line):
                src = self.mo.group(1)
                srcExt = self.mo.group(2)
                dst = os.path.splitext(src)[0] + '.nasm'
                fullDst = os.path.join(self.dir, dst)
                if src not in srcToDst and not os.path.exists(fullDst):
                    srcToDst[src] = dst
                    srcToDst['order'].append(src)
        return srcToDst

    def ScanInfAsmFiles(self):
        src = self.inf
        assert os.path.isfile(src)
        f = io.open(src, 'rt')
        self.lines = f.readlines()
        f.close()

        path = os.path.realpath(self.inf)
        (self.dir, inf) = os.path.split(path)
        parent = os.path.normpath(self.dir)
        (lastpath, self.moduleName) = os.path.split(parent)
        self.packageName = None
        while True:
            lastpath = os.path.normpath(lastpath)
            (parent, basename) = os.path.split(lastpath)
            if parent == lastpath:
                break
            if basename.endswith('Pkg'):
                self.packageName = basename
                break
            lastpath = parent

        self.srcToDst = self.GetInfAsmFileMapping()

        self.dstToSrc = {'order': []}
        for src in self.srcToDst['order']:
            srcExt = os.path.splitext(src)[1]
            dst = self.srcToDst[src]
            if dst not in self.dstToSrc:
                self.dstToSrc[dst] = [src]
                self.dstToSrc['order'].append(dst)
            else:
                self.dstToSrc[dst].append(src)

    def __len__(self):
        return len(self.dstToSrc['order'])

    def __iter__(self):
        return iter(self.dstToSrc['order'])

    def ConvertInfAsmFiles(self):
        notConverted = []
        unsupportedArchCount = 0
        for dst in self:
            didSomething = False
            try:
                self.UpdateInfAsmFile(dst)
                didSomething = True
            except UnsupportedConversion:
                if not self.args.quiet:
                    print('MASM=>NASM conversion unsupported for', dst)
                notConverted.append(dst)
            except NoSourceFile:
                if not self.args.quiet:
                    print('Source file missing for', reldst)
                notConverted.append(dst)
            except UnsupportedArch:
                unsupportedArchCount += 1
            else:
                if didSomething:
                    self.ConversionFinished(dst)
        if len(notConverted) > 0 and not self.args.quiet:
            for dst in notConverted:
                reldst = self.RootRelative(dst)
                print('Unabled to convert', reldst)
        if unsupportedArchCount > 0 and not self.args.quiet:
            print('Skipped', unsupportedArchCount, 'files based on architecture')

    def UpdateInfAsmFile(self, dst, IgnoreMissingAsm=False):
        infPath = os.path.split(os.path.realpath(self.inf))[0]
        asmSrc = os.path.splitext(dst)[0] + '.asm'
        fullSrc = os.path.join(infPath, asmSrc)
        fullDst = os.path.join(infPath, dst)
        srcParentDir = os.path.basename(os.path.split(fullSrc)[0])
        if srcParentDir.lower() in UnsupportedArch.unsupported:
            raise UnsupportedArch
        elif not os.path.exists(fullSrc):
            if not IgnoreMissingAsm:
                raise NoSourceFile
        else:  # not os.path.exists(fullDst):
            conv = ConvertAsmFile(fullSrc, fullDst, self)
            self.unsupportedSyntaxSeen = conv.unsupportedSyntaxSeen

        fileChanged = False
        recentSources = list()
        i = 0
        while i < len(self.lines):
            line = self.lines[i].rstrip()
            updatedLine = line
            lineChanged = False
            preserveOldSource = False
            for src in self.dstToSrc[dst]:
                assert self.srcToDst[src] == dst
                updatedLine = self.ReplacePreserveSpacing(
                    updatedLine, src, dst)
                lineChanged = updatedLine != line
                if lineChanged:
                    preserveOldSource = self.ShouldKeepFile(src)
                    break

            if lineChanged:
                if preserveOldSource:
                    if updatedLine.strip() not in recentSources:
                        self.lines.insert(i, updatedLine + '\n')
                        recentSources.append(updatedLine.strip())
                        i += 1
                        if self.diff:
                            print('+%s' % updatedLine)
                    if self.diff:
                        print('', line)
                else:
                    if self.diff:
                        print('-%s' % line)
                    if updatedLine.strip() in recentSources:
                        self.lines[i] = None
                    else:
                        self.lines[i] = updatedLine + '\n'
                        recentSources.append(updatedLine.strip())
                        if self.diff:
                            print('+%s' % updatedLine)
            else:
                if len(recentSources) > 0:
                    recentSources = list()
                if self.diff:
                    print('', line)

            fileChanged |= lineChanged
            i += 1

        if fileChanged:
            self.lines = list(filter(lambda l: l is not None, self.lines))

        for src in self.dstToSrc[dst]:
            if not src.endswith('.asm'):
                fullSrc = os.path.join(infPath, src)
                if os.path.exists(fullSrc):
                    self.RemoveFile(fullSrc)

        if fileChanged:
            f = io.open(self.inf, 'w', newline='\r\n')
            f.writelines(self.lines)
            f.close()
            self.FileUpdated(self.inf)

    def ConversionFinished(self, dst):
        asmSrc = os.path.splitext(dst)[0] + '.asm'
        self.FileConversionFinished(
            self.packageName, self.moduleName, asmSrc, dst)


class ConvertInfFiles(CommonUtils):

    def __init__(self, infs, clone):
        CommonUtils.__init__(self, clone)
        infs = map(lambda i: ConvertInfFile(i, self), infs)
        infs = filter(lambda i: len(i) > 0, infs)
        dstToInfs = {'order': []}
        for inf in infs:
            for dst in inf:
                fulldst = os.path.realpath(os.path.join(inf.dir, dst))
                pair = (inf, dst)
                if fulldst in dstToInfs:
                    dstToInfs[fulldst].append(pair)
                else:
                    dstToInfs['order'].append(fulldst)
                    dstToInfs[fulldst] = [pair]

        notConverted = []
        unsupportedArchCount = 0
        for dst in dstToInfs['order']:
            didSomething = False
            try:
                for inf, reldst in dstToInfs[dst]:
                    inf.UpdateInfAsmFile(reldst, IgnoreMissingAsm=didSomething)
                    didSomething = True
            except UnsupportedConversion:
                if not self.args.quiet:
                    print('MASM=>NASM conversion unsupported for', reldst)
                notConverted.append(dst)
            except NoSourceFile:
                if not self.args.quiet:
                    print('Source file missing for', reldst)
                notConverted.append(dst)
            except UnsupportedArch:
                unsupportedArchCount += 1
            else:
                if didSomething:
                    inf.ConversionFinished(reldst)
        if len(notConverted) > 0 and not self.args.quiet:
            for dst in notConverted:
                reldst = self.RootRelative(dst)
                print('Unabled to convert', reldst)
        if unsupportedArchCount > 0 and not self.args.quiet:
            print('Skipped', unsupportedArchCount, 'files based on architecture')


class ConvertDirectories(CommonUtils):

    def __init__(self, paths, clone):
        CommonUtils.__init__(self, clone)
        self.paths = paths
        self.ConvertInfAndAsmFiles()

    def ConvertInfAndAsmFiles(self):
        infs = list()
        for path in self.paths:
            assert(os.path.exists(path))
        for path in self.paths:
            for root, dirs, files in os.walk(path):
                for d in ('.svn', '.git'):
                    if d in dirs:
                        dirs.remove(d)
                for f in files:
                    if f.lower().endswith('.inf'):
                        inf = os.path.realpath(os.path.join(root, f))
                        infs.append(inf)

        ConvertInfFiles(infs, self)


class ConvertAsmApp(CommonUtils):

    def __init__(self):
        CommonUtils.__init__(self)

        src = self.args.source
        dst = self.args.dest
        if self.infmode:
            ConvertInfFiles((src,), self)
        elif self.dirmode:
            ConvertDirectories((src,), self)
        elif not self.dirmode:
            ConvertAsmFile(src, dst, self)

ConvertAsmApp()
