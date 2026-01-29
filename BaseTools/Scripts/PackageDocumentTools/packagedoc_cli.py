## @file
# This module provide command line entry for generating package document!
#
# Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

from __future__ import print_function
import os, sys, logging, traceback, subprocess
from optparse import OptionParser

from plugins.EdkPlugins.edk2.model import baseobject
from plugins.EdkPlugins.edk2.model import doxygengen

gArchMarcoDict = {'ALL'      : 'MDE_CPU_IA32 MDE_CPU_X64 MDE_CPU_EBC MDE_CPU_IPF _MSC_EXTENSIONS __GNUC__ __INTEL_COMPILER',
                  'IA32_MSFT': 'MDE_CPU_IA32 _MSC_EXTENSIONS',
                  'IA32_GNU' : 'MDE_CPU_IA32 __GNUC__',
                  'X64_MSFT' : 'MDE_CPU_X64 _MSC_EXTENSIONS  ASM_PFX= OPTIONAL= ',
                  'X64_GNU'  : 'MDE_CPU_X64 __GNUC__  ASM_PFX= OPTIONAL= ',
                  'IPF_MSFT' : 'MDE_CPU_IPF _MSC_EXTENSIONS  ASM_PFX= OPTIONAL= ',
                  'IPF_GNU'  : 'MDE_CPU_IPF __GNUC__  ASM_PFX= OPTIONAL= ',
                  'EBC_INTEL': 'MDE_CPU_EBC __INTEL_COMPILER  ASM_PFX= OPTIONAL= '}

def parseCmdArgs():
    parser = OptionParser(version="Package Document Generation Tools - Version 0.1")
    parser.add_option('-w', '--workspace', action='store', type='string', dest='WorkspacePath',
                      help='Specify workspace absolute path. For example: c:\\tianocore')
    parser.add_option('-p', '--decfile', action='store', dest='PackagePath',
                      help='Specify the absolute path for package DEC file. For example: c:\\tianocore\\MdePkg\\MdePkg.dec')
    parser.add_option('-x', '--doxygen', action='store', dest='DoxygenPath',
                      help='Specify the absolute path of doxygen tools installation. For example: C:\\Program Files\\doxygen\bin\doxygen.exe')
    parser.add_option('-o', '--output', action='store', dest='OutputPath',
                      help='Specify the document output path. For example: c:\\docoutput')
    parser.add_option('-a', '--arch', action='store', dest='Arch', choices=list(gArchMarcoDict.keys()),
                      help='Specify the architecture used in preprocess package\'s source. For example: -a IA32_MSFT')
    parser.add_option('-m', '--mode', action='store', dest='DocumentMode', choices=['CHM', 'HTML'],
                      help='Specify the document mode from : CHM or HTML')
    parser.add_option('-i', '--includeonly', action='store_true', dest='IncludeOnly',
                      help='Only generate document for package\'s public interfaces produced by include folder. ')
    parser.add_option('-c', '--htmlworkshop', dest='HtmlWorkshopPath',
                      help='Specify the absolute path for Microsoft HTML Workshop\'s hhc.exe file. For example: C:\\Program Files\\HTML Help Workshop\\hhc.exe')
    (options, args) = parser.parse_args()

    # validate the options
    errors = []
    if options.WorkspacePath is None:
        errors.append('- Please specify workspace path via option -w!')
    elif not os.path.exists(options.WorkspacePath):
        errors.append("- Invalid workspace path %s! The workspace path should be exist in absolute path!" % options.WorkspacePath)

    if options.PackagePath is None:
        errors.append('- Please specify package DEC file path via option -p!')
    elif not os.path.exists(options.PackagePath):
        errors.append("- Invalid package's DEC file path %s! The DEC path should be exist in absolute path!" % options.PackagePath)

    default = "C:\\Program Files\\doxygen\\bin\\doxygen.exe"
    if options.DoxygenPath is None:
        if os.path.exists(default):
            print("Warning: Assume doxygen tool is installed at %s. If not, please specify via -x" % default)
            options.DoxygenPath = default
        else:
            errors.append('- Please specify the path of doxygen tool installation via option -x! or install it in default path %s' % default)
    elif not os.path.exists(options.DoxygenPath):
        errors.append("- Invalid doxygen tool path %s! The doxygen tool path should be exist in absolute path!" % options.DoxygenPath)

    if options.OutputPath is not None:
        if not os.path.exists(options.OutputPath):
            # create output
            try:
                os.makedirs(options.OutputPath)
            except:
                errors.append('- Fail to create the output directory %s' % options.OutputPath)
    else:
        if options.PackagePath is not None and os.path.exists(options.PackagePath):
            dirpath = os.path.dirname(options.PackagePath)
            default = os.path.join (dirpath, "Document")
            print('Warning: Assume document output at %s. If not, please specify via option -o' % default)
            options.OutputPath = default
            if not os.path.exists(default):
                try:
                    os.makedirs(default)
                except:
                    errors.append('- Fail to create default output directory %s! Please specify document output diretory via option -o' % default)
        else:
            errors.append('- Please specify document output path via option -o!')

    if options.Arch is None:
        options.Arch = 'ALL'
        print("Warning: Assume arch is \"ALL\". If not, specify via -a")

    if options.DocumentMode is None:
        options.DocumentMode = "HTML"
        print("Warning: Assume document mode is \"HTML\". If not, specify via -m")

    if options.IncludeOnly is None:
        options.IncludeOnly = False
        print("Warning: Assume generate package document for all package\'s source including publich interfaces and implementation libraries and modules.")

    if options.DocumentMode.lower() == 'chm':
        default = "C:\\Program Files\\HTML Help Workshop\\hhc.exe"
        if options.HtmlWorkshopPath is None:
            if os.path.exists(default):
                print('Warning: Assume the installation path of Microsoft HTML Workshop is %s. If not, specify via option -c.' % default)
                options.HtmlWorkshopPath = default
            else:
                errors.append('- Please specify the installation path of Microsoft HTML Workshop via option -c!')
        elif not os.path.exists(options.HtmlWorkshopPath):
            errors.append('- The installation path of Microsoft HTML Workshop %s does not exists. ' % options.HtmlWorkshopPath)

    if len(errors) != 0:
        print('\n')
        parser.error('Fail to start due to following reasons: \n%s' %'\n'.join(errors))
    return (options.WorkspacePath, options.PackagePath, options.DoxygenPath, options.OutputPath,
            options.Arch, options.DocumentMode, options.IncludeOnly, options.HtmlWorkshopPath)

def createPackageObject(wsPath, pkgPath):
    try:
        pkgObj = baseobject.Package(None, wsPath)
        pkgObj.Load(pkgPath)
    except:
        logging.getLogger().error ('Fail to create package object!')
        return None

    return pkgObj

def callbackLogMessage(msg, level):
    print(msg.strip())

def callbackCreateDoxygenProcess(doxPath, configPath):
    if sys.platform == 'win32':
        cmd = '"%s" %s' % (doxPath, configPath)
    else:
        cmd = '%s %s' % (doxPath, configPath)
    print(cmd)
    subprocess.call(cmd, shell=True)


def DocumentFixup(outPath, arch):
    # find BASE_LIBRARY_JUMP_BUFFER structure reference page

    print('\n    >>> Start fixup document \n')

    for root, dirs, files in os.walk(outPath):
        for dir in dirs:
            if dir.lower() in ['.svn', '_svn', 'cvs']:
                dirs.remove(dir)
        for file in files:
            if not file.lower().endswith('.html'): continue
            fullpath = os.path.join(outPath, root, file)
            try:
                f = open(fullpath, 'r')
                text = f.read()
                f.close()
            except:
                logging.getLogger().error('\nFail to open file %s\n' % fullpath)
                continue
            if arch.lower() == 'all':
                if text.find('BASE_LIBRARY_JUMP_BUFFER Struct Reference') != -1:
                    FixPageBASE_LIBRARY_JUMP_BUFFER(fullpath, text)
                if text.find('MdePkg/Include/Library/BaseLib.h File Reference') != -1:
                    FixPageBaseLib(fullpath, text)
                if text.find('IA32_IDT_GATE_DESCRIPTOR Union Reference') != -1:
                    FixPageIA32_IDT_GATE_DESCRIPTOR(fullpath, text)
            if text.find('MdePkg/Include/Library/UefiDriverEntryPoint.h File Reference') != -1:
                FixPageUefiDriverEntryPoint(fullpath, text)
            if text.find('MdePkg/Include/Library/UefiApplicationEntryPoint.h File Reference') != -1:
                FixPageUefiApplicationEntryPoint(fullpath, text)

    print('    >>> Finish all document fixing up! \n')

def FixPageBaseLib(path, text):
    print('    >>> Fixup BaseLib file page at file %s \n' % path)
    lines = text.split('\n')
    lastBaseJumpIndex = -1
    lastIdtGateDescriptor = -1
    for index in range(len(lines) - 1, -1, -1):
        line = lines[index]
        if line.strip() == '<td class="memname">#define BASE_LIBRARY_JUMP_BUFFER_ALIGNMENT&nbsp;&nbsp;&nbsp;4          </td>':
            lines[index] = '<td class="memname">#define BASE_LIBRARY_JUMP_BUFFER_ALIGNMENT&nbsp;&nbsp;&nbsp;4&nbsp;[IA32]    </td>'
        if line.strip() == '<td class="memname">#define BASE_LIBRARY_JUMP_BUFFER_ALIGNMENT&nbsp;&nbsp;&nbsp;0x10          </td>':
            lines[index] = '<td class="memname">#define BASE_LIBRARY_JUMP_BUFFER_ALIGNMENT&nbsp;&nbsp;&nbsp;0x10&nbsp;[IPF]   </td>'
        if line.strip() == '<td class="memname">#define BASE_LIBRARY_JUMP_BUFFER_ALIGNMENT&nbsp;&nbsp;&nbsp;8          </td>':
            lines[index] = '<td class="memname">#define BASE_LIBRARY_JUMP_BUFFER_ALIGNMENT&nbsp;&nbsp;&nbsp;9&nbsp;[EBC, x64]   </td>'
        if line.find('BASE_LIBRARY_JUMP_BUFFER_ALIGNMENT</a>&nbsp;&nbsp;&nbsp;4') != -1:
            lines[index] = lines[index].replace('BASE_LIBRARY_JUMP_BUFFER_ALIGNMENT</a>&nbsp;&nbsp;&nbsp;4',
                                 'BASE_LIBRARY_JUMP_BUFFER_ALIGNMENT</a>&nbsp;&nbsp;&nbsp;4&nbsp;[IA32]')
        if line.find('BASE_LIBRARY_JUMP_BUFFER_ALIGNMENT</a>&nbsp;&nbsp;&nbsp;0x10') != -1:
            lines[index] = lines[index].replace('BASE_LIBRARY_JUMP_BUFFER_ALIGNMENT</a>&nbsp;&nbsp;&nbsp;0x10',
                                 'BASE_LIBRARY_JUMP_BUFFER_ALIGNMENT</a>&nbsp;&nbsp;&nbsp;0x10&nbsp;[IPF]')
        if line.find('BASE_LIBRARY_JUMP_BUFFER_ALIGNMENT</a>&nbsp;&nbsp;&nbsp;8') != -1:
            lines[index] = lines[index].replace('BASE_LIBRARY_JUMP_BUFFER_ALIGNMENT</a>&nbsp;&nbsp;&nbsp;8',
                                 'BASE_LIBRARY_JUMP_BUFFER_ALIGNMENT</a>&nbsp;&nbsp;&nbsp;8&nbsp;[x64, EBC]')
        if line.find('>BASE_LIBRARY_JUMP_BUFFER</a>') != -1:
            if lastBaseJumpIndex != -1:
                del lines[lastBaseJumpIndex]
            lastBaseJumpIndex = index
        if line.find('>IA32_IDT_GATE_DESCRIPTOR</a></td>') != -1:
            if lastIdtGateDescriptor != -1:
                del lines[lastIdtGateDescriptor]
            lastIdtGateDescriptor = index
    try:
        f = open(path, 'w')
        f.write('\n'.join(lines))
        f.close()
    except:
        logging.getLogger().error("     <<< Fail to fixup file %s\n" % path)
        return
    print("    <<< Finish to fixup file %s\n" % path)

def FixPageIA32_IDT_GATE_DESCRIPTOR(path, text):
    print('    >>> Fixup structure reference IA32_IDT_GATE_DESCRIPTOR at file %s \n' % path)
    lines = text.split('\n')
    for index in range(len(lines) - 1, -1, -1):
        line = lines[index].strip()
        if line.find('struct {</td>') != -1 and lines[index - 2].find('>Uint64</a></td>') != -1:
            lines.insert(index, '<tr><td colspan="2"><br><h2>Data Fields For X64</h2></td></tr>')
        if line.find('struct {</td>') != -1 and lines[index - 1].find('Data Fields') != -1:
            lines.insert(index, '<tr><td colspan="2"><br><h2>Data Fields For IA32</h2></td></tr>')
    try:
        f = open(path, 'w')
        f.write('\n'.join(lines))
        f.close()
    except:
        logging.getLogger().error("     <<< Fail to fixup file %s\n" % path)
        return
    print("    <<< Finish to fixup file %s\n" % path)

def FixPageBASE_LIBRARY_JUMP_BUFFER(path, text):
    print('    >>> Fixup structure reference BASE_LIBRARY_JUMP_BUFFER at file %s \n' % path)
    lines = text.split('\n')
    bInDetail = True
    bNeedRemove = False
    for index in range(len(lines) - 1, -1, -1):
        line = lines[index]
        if line.find('Detailed Description') != -1:
            bInDetail = False
        if line.startswith('EBC context buffer used by') and lines[index - 1].startswith('x64 context buffer'):
            lines[index] = "IA32/IPF/X64/" + line
            bNeedRemove  = True
        if line.startswith("x64 context buffer") or line.startswith('IPF context buffer used by') or \
           line.startswith('IA32 context buffer used by'):
            if bNeedRemove:
                lines.remove(line)
        if line.find('>R0</a>') != -1 and not bInDetail:
            if lines[index - 1] != '<tr><td colspan="2"><br><h2>Data Fields For EBC</h2></td></tr>':
                lines.insert(index, '<tr><td colspan="2"><br><h2>Data Fields For EBC</h2></td></tr>')
        if line.find('>Rbx</a>') != -1 and not bInDetail:
            if lines[index - 1] != '<tr><td colspan="2"><br><h2>Data Fields For X64</h2></td></tr>':
                lines.insert(index, '<tr><td colspan="2"><br><h2>Data Fields For X64</h2></td></tr>')
        if line.find('>F2</a>') != -1 and not bInDetail:
            if lines[index - 1] != '<tr><td colspan="2"><br><h2>Data Fields For IPF</h2></td></tr>':
                lines.insert(index, '<tr><td colspan="2"><br><h2>Data Fields For IPF</h2></td></tr>')
        if line.find('>Ebx</a>') != -1 and not bInDetail:
            if lines[index - 1] != '<tr><td colspan="2"><br><h2>Data Fields For IA32</h2></td></tr>':
                lines.insert(index, '<tr><td colspan="2"><br><h2>Data Fields For IA32</h2></td></tr>')
    try:
        f = open(path, 'w')
        f.write('\n'.join(lines))
        f.close()
    except:
        logging.getLogger().error("     <<< Fail to fixup file %s" % path)
        return
    print("    <<< Finish to fixup file %s\n" % path)

def FixPageUefiDriverEntryPoint(path, text):
    print('    >>> Fixup file reference MdePkg/Include/Library/UefiDriverEntryPoint.h at file %s \n' % path)
    lines = text.split('\n')
    bInModuleEntry = False
    bInEfiMain     = False
    ModuleEntryDlCount  = 0
    ModuleEntryDelStart = 0
    ModuleEntryDelEnd   = 0
    EfiMainDlCount      = 0
    EfiMainDelStart     = 0
    EfiMainDelEnd       = 0

    for index in range(len(lines)):
        line = lines[index].strip()
        if line.find('EFI_STATUS</a> EFIAPI _ModuleEntryPoint           </td>') != -1:
            bInModuleEntry = True
        if line.find('EFI_STATUS</a> EFIAPI EfiMain           </td>') != -1:
            bInEfiMain = True
        if line.startswith('<p>References <a'):
            if bInModuleEntry:
                ModuleEntryDelEnd = index - 1
                bInModuleEntry = False
            elif bInEfiMain:
                EfiMainDelEnd = index - 1
                bInEfiMain = False
        if bInModuleEntry:
            if line.startswith('</dl>'):
                ModuleEntryDlCount = ModuleEntryDlCount + 1
            if ModuleEntryDlCount == 1:
                ModuleEntryDelStart = index + 1
        if bInEfiMain:
            if line.startswith('</dl>'):
                EfiMainDlCount = EfiMainDlCount + 1
            if EfiMainDlCount == 1:
                EfiMainDelStart = index + 1

    if EfiMainDelEnd > EfiMainDelStart:
        for index in range(EfiMainDelEnd, EfiMainDelStart, -1):
            del lines[index]
    if ModuleEntryDelEnd > ModuleEntryDelStart:
        for index in range(ModuleEntryDelEnd, ModuleEntryDelStart, -1):
            del lines[index]

    try:
        f = open(path, 'w')
        f.write('\n'.join(lines))
        f.close()
    except:
        logging.getLogger().error("     <<< Fail to fixup file %s" % path)
        return
    print("    <<< Finish to fixup file %s\n" % path)


def FixPageUefiApplicationEntryPoint(path, text):
    print('    >>> Fixup file reference MdePkg/Include/Library/UefiApplicationEntryPoint.h at file %s \n' % path)
    lines = text.split('\n')
    bInModuleEntry = False
    bInEfiMain     = False
    ModuleEntryDlCount  = 0
    ModuleEntryDelStart = 0
    ModuleEntryDelEnd   = 0
    EfiMainDlCount      = 0
    EfiMainDelStart     = 0
    EfiMainDelEnd       = 0

    for index in range(len(lines)):
        line = lines[index].strip()
        if line.find('EFI_STATUS</a> EFIAPI _ModuleEntryPoint           </td>') != -1:
            bInModuleEntry = True
        if line.find('EFI_STATUS</a> EFIAPI EfiMain           </td>') != -1:
            bInEfiMain = True
        if line.startswith('<p>References <a'):
            if bInModuleEntry:
                ModuleEntryDelEnd = index - 1
                bInModuleEntry = False
            elif bInEfiMain:
                EfiMainDelEnd = index - 1
                bInEfiMain = False
        if bInModuleEntry:
            if line.startswith('</dl>'):
                ModuleEntryDlCount = ModuleEntryDlCount + 1
            if ModuleEntryDlCount == 1:
                ModuleEntryDelStart = index + 1
        if bInEfiMain:
            if line.startswith('</dl>'):
                EfiMainDlCount = EfiMainDlCount + 1
            if EfiMainDlCount == 1:
                EfiMainDelStart = index + 1

    if EfiMainDelEnd > EfiMainDelStart:
        for index in range(EfiMainDelEnd, EfiMainDelStart, -1):
            del lines[index]
    if ModuleEntryDelEnd > ModuleEntryDelStart:
        for index in range(ModuleEntryDelEnd, ModuleEntryDelStart, -1):
            del lines[index]

    try:
        f = open(path, 'w')
        f.write('\n'.join(lines))
        f.close()
    except:
        logging.getLogger().error("     <<< Fail to fixup file %s" % path)
        return
    print("    <<< Finish to fixup file %s\n" % path)

if __name__ == '__main__':
    wspath, pkgpath, doxpath, outpath, archtag, docmode, isinc, hwpath = parseCmdArgs()

    # configure logging system
    logfilepath = os.path.join(outpath, 'log.txt')
    logging.basicConfig(format='%(levelname)-8s %(message)s', level=logging.DEBUG)

    # create package model object firstly
    pkgObj = createPackageObject(wspath, pkgpath)
    if pkgObj is None:
        sys.exit(-1)

    # create doxygen action model
    arch    = None
    tooltag = None
    if archtag.lower() != 'all':
        arch = archtag.split('_')[0]
        tooltag = archtag.split('_')[1]
    else:
        arch    = 'all'
        tooltag = 'all'

    # preprocess package and call doxygen
    try:
        action = doxygengen.PackageDocumentAction(doxpath,
                                                  hwpath,
                                                  outpath,
                                                  pkgObj,
                                                  docmode,
                                                  callbackLogMessage,
                                                  arch,
                                                  tooltag,
                                                  isinc,
                                                  True)
        action.RegisterCallbackDoxygenProcess(callbackCreateDoxygenProcess)
        action.Generate()
    except:
        message = traceback.format_exception(*sys.exc_info())
        logging.getLogger().error('Fail to create doxygen action! \n%s' % ''.join(message))
        sys.exit(-1)

    DocumentFixup(outpath, arch)

    # generate CHM is necessary
    if docmode.lower() == 'chm':
        indexpath = os.path.join(outpath, 'html', 'index.hhp')
        if sys.platform == 'win32':
            cmd = '"%s" %s' % (hwpath, indexpath)
        else:
            cmd = '%s %s' % (hwpath, indexpath)
        subprocess.call(cmd)
        print('\nFinish to generate package document! Please open %s for review' % os.path.join(outpath, 'html', 'index.chm'))
    else:
        print('\nFinish to generate package document! Please open %s for review' % os.path.join(outpath, 'html', 'index.html'))
