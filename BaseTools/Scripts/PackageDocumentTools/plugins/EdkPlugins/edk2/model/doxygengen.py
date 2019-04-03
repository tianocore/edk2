## @file
#
# This file produce action class to generate doxygen document for edk2 codebase.
# The action classes are shared by GUI and command line tools.
#
# Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent

"""This file produce action class to generate doxygen document for edk2 codebase.
   The action classes are shared by GUI and command line tools.
"""
from plugins.EdkPlugins.basemodel import doxygen
import os
try:
    import wx
    gInGui = True
except:
    gInGui = False
import re
from plugins.EdkPlugins.edk2.model import inf
from plugins.EdkPlugins.edk2.model import dec
from plugins.EdkPlugins.basemodel.message import *

_ignore_dir = ['.svn', '_svn', 'cvs']
_inf_key_description_mapping_table = {
  'INF_VERSION':'Version of INF file specification',
  #'BASE_NAME':'Module Name',
  'FILE_GUID':'Module Guid',
  'MODULE_TYPE': 'Module Type',
  'VERSION_STRING': 'Module Version',
  'LIBRARY_CLASS': 'Produced Library Class',
  'EFI_SPECIFICATION_VERSION': 'UEFI Specification Version',
  'PI_SPECIFICATION_VERSION': 'PI Specification Version',
  'ENTRY_POINT': 'Module Entry Point Function',
  'CONSTRUCTOR': 'Library Constructor Function'
}

_dec_key_description_mapping_table = {
  'DEC_SPECIFICATION': 'Version of DEC file specification',
  'PACKAGE_GUID': 'Package Guid'
}
class DoxygenAction:
    """This is base class for all doxygen action.
    """

    def __init__(self, doxPath, chmPath, outputPath, projname, mode='html', log=None, verbose=False):
        """Constructor function.
        @param  doxPath         the obosolution path of doxygen execute file.
        @param  outputPath      the obosolution output path.
        @param  log             log function for output message
        """
        self._doxPath       = doxPath
        self._chmPath       = chmPath
        self._outputPath    = outputPath
        self._projname      = projname
        self._configFile    = None          # doxygen config file is used by doxygen exe file
        self._indexPageFile = None          # doxygen page file for index page.
        self._log           = log
        self._mode          = mode
        self._verbose       = verbose
        self._doxygenCallback = None
        self._chmCallback     = None

    def Log(self, message, level='info'):
        if self._log is not None:
            self._log(message, level)

    def IsVerbose(self):
        return self._verbose

    def Generate(self):
        """Generate interface called by outer directly"""
        self.Log(">>>>>> Start generate doxygen document for %s... Zzz....\n" % self._projname)

        # create doxygen config file at first
        self._configFile = doxygen.DoxygenConfigFile()
        self._configFile.SetOutputDir(self._outputPath)

        self._configFile.SetWarningFilePath(os.path.join(self._outputPath, 'warning.txt'))
        if self._mode.lower() == 'html':
            self._configFile.SetHtmlMode()
        else:
            self._configFile.SetChmMode()

        self.Log("    >>>>>> Initialize doxygen config file...Zzz...\n")
        self.InitializeConfigFile()

        self.Log("    >>>>>> Generate doxygen index page file...Zzz...\n")
        indexPagePath = self.GenerateIndexPage()
        if indexPagePath is None:
            self.Log("Fail to generate index page!\n", 'error')
            return False
        else:
            self.Log("Success to create doxygen index page file %s \n" % indexPagePath)

        # Add index page doxygen file to file list.
        self._configFile.AddFile(indexPagePath)

        # save config file to output path
        configFilePath = os.path.join(self._outputPath, self._projname + '.doxygen_config')
        self._configFile.Generate(configFilePath)
        self.Log("    <<<<<< Success Save doxygen config file to %s...\n" % configFilePath)

        # launch doxygen tool to generate document
        if self._doxygenCallback is not None:
            self.Log("    >>>>>> Start doxygen process...Zzz...\n")
            if not self._doxygenCallback(self._doxPath, configFilePath):
                return False
        else:
            self.Log("Fail to create doxygen process!", 'error')
            return False

        return True

    def InitializeConfigFile(self):
        """Initialize config setting for doxygen project. It will be invoked after config file
           object is created. Inherited class should implement it.
        """

    def GenerateIndexPage(self):
        """Generate doxygen index page. Inherited class should implement it."""
        return None

    def RegisterCallbackDoxygenProcess(self, callback):
        self._doxygenCallback = callback

    def RegisterCallbackCHMProcess(self, callback):
        self._chmCallback = callback

class PlatformDocumentAction(DoxygenAction):
    """Generate platform doxygen document, will be implement at future."""

class PackageDocumentAction(DoxygenAction):
    """Generate package reference document"""

    def __init__(self, doxPath, chmPath, outputPath, pObj, mode='html', log=None, arch=None, tooltag=None,
                  onlyInclude=False, verbose=False):
        DoxygenAction.__init__(self, doxPath, chmPath, outputPath, pObj.GetName(), mode, log, verbose)
        self._pObj   = pObj
        self._arch   = arch
        self._tooltag = tooltag
        self._onlyIncludeDocument = onlyInclude

    def InitializeConfigFile(self):
        if self._arch == 'IA32':
            self._configFile.AddPreDefined('MDE_CPU_IA32')
        elif self._arch == 'X64':
            self._configFile.AddPreDefined('MDE_CPU_X64')
        elif self._arch == 'IPF':
            self._configFile.AddPreDefined('MDE_CPU_IPF')
        elif self._arch == 'EBC':
            self._configFile.AddPreDefined('MDE_CPU_EBC')
        else:
            self._arch = None
            self._configFile.AddPreDefined('MDE_CPU_IA32')
            self._configFile.AddPreDefined('MDE_CPU_X64')
            self._configFile.AddPreDefined('MDE_CPU_IPF')
            self._configFile.AddPreDefined('MDE_CPU_EBC')
            self._configFile.AddPreDefined('MDE_CPU_ARM')

        namestr = self._pObj.GetName()
        if self._arch is not None:
            namestr += '[%s]' % self._arch
        if self._tooltag is not None:
            namestr += '[%s]' % self._tooltag
        self._configFile.SetProjectName(namestr)
        self._configFile.SetStripPath(self._pObj.GetWorkspace())
        self._configFile.SetProjectVersion(self._pObj.GetFileObj().GetVersion())
        self._configFile.AddPattern('*.decdoxygen')

        if self._tooltag.lower() == 'msft':
            self._configFile.AddPreDefined('_MSC_EXTENSIONS')
        elif self._tooltag.lower() == 'gnu':
            self._configFile.AddPreDefined('__GNUC__')
        elif self._tooltag.lower() == 'intel':
            self._configFile.AddPreDefined('__INTEL_COMPILER')
        else:
            self._tooltag = None
            self._configFile.AddPreDefined('_MSC_EXTENSIONS')
            self._configFile.AddPreDefined('__GNUC__')
            self._configFile.AddPreDefined('__INTEL_COMPILER')

        self._configFile.AddPreDefined('ASM_PFX= ')
        self._configFile.AddPreDefined('OPTIONAL= ')

    def GenerateIndexPage(self):
        """Generate doxygen index page. Inherited class should implement it."""
        fObj   = self._pObj.GetFileObj()
        pdObj  = doxygen.DoxygenFile('%s Package Document' % self._pObj.GetName(),
                                     '%s.decdoxygen' % self._pObj.GetFilename())
        self._configFile.AddFile(pdObj.GetFilename())
        pdObj.AddDescription(fObj.GetFileHeader())

        defSection = fObj.GetSectionByName('defines')[0]
        baseSection = doxygen.Section('PackageBasicInformation', 'Package Basic Information')
        descr = '<TABLE>'
        for obj in defSection.GetObjects():
            if obj.GetKey() in _dec_key_description_mapping_table.keys():
                descr += '<TR>'
                descr += '<TD><B>%s</B></TD>' % _dec_key_description_mapping_table[obj.GetKey()]
                descr += '<TD>%s</TD>' % obj.GetValue()
                descr += '</TR>'
        descr += '</TABLE><br>'
        baseSection.AddDescription(descr)
        pdObj.AddSection(baseSection)

        knownIssueSection = doxygen.Section('Known_Issue_section', 'Known Issue')
        knownIssueSection.AddDescription('<ul>')
        knownIssueSection.AddDescription('<li> OPTIONAL macro for function parameter can not be dealed with doxygen, so it disapear in this document! </li>')
        knownIssueSection.AddDescription('</ul>')
        pdObj.AddSection(knownIssueSection)

        self.AddAllIncludeFiles(self._pObj, self._configFile)
        pages = self.GenerateIncludesSubPage(self._pObj, self._configFile)
        if len(pages) != 0:
            pdObj.AddPages(pages)
        pages = self.GenerateLibraryClassesSubPage(self._pObj, self._configFile)
        if len(pages) != 0:
            pdObj.AddPages(pages)
        pages = self.GeneratePcdSubPages(self._pObj, self._configFile)
        if len(pages) != 0:
            pdObj.AddPages(pages)
        pages = self.GenerateGuidSubPages(self._pObj, self._configFile)
        if len(pages) != 0:
            pdObj.AddPages(pages)
        pages = self.GeneratePpiSubPages(self._pObj, self._configFile)
        if len(pages) != 0:
            pdObj.AddPages(pages)
        pages = self.GenerateProtocolSubPages(self._pObj, self._configFile)
        if len(pages) != 0:
            pdObj.AddPages(pages)
        if not self._onlyIncludeDocument:
            pdObj.AddPages(self.GenerateModulePages(self._pObj, self._configFile))

        pdObj.Save()
        return pdObj.GetFilename()

    def GenerateIncludesSubPage(self, pObj, configFile):
        # by default add following path as include path to config file
        pkpath = pObj.GetFileObj().GetPackageRootPath()
        configFile.AddIncludePath(os.path.join(pkpath, 'Include'))
        configFile.AddIncludePath(os.path.join(pkpath, 'Include', 'Library'))
        configFile.AddIncludePath(os.path.join(pkpath, 'Include', 'Protocol'))
        configFile.AddIncludePath(os.path.join(pkpath, 'Include', 'Ppi'))
        configFile.AddIncludePath(os.path.join(pkpath, 'Include', 'Guid'))
        configFile.AddIncludePath(os.path.join(pkpath, 'Include', 'IndustryStandard'))

        rootArray = []
        pageRoot = doxygen.Page("Public Includes", "%s_public_includes" % pObj.GetName())
        objs = pObj.GetFileObj().GetSectionObjectsByName('includes')
        if len(objs) == 0: return []

        for obj in objs:
            # Add path to include path
            path = os.path.join(pObj.GetFileObj().GetPackageRootPath(), obj.GetPath())
            configFile.AddIncludePath(path)

            # only list common folder's include file
            if obj.GetArch().lower() != 'common':
                continue

            bNeedAddIncludePage = False
            topPage = doxygen.Page(self._ConvertPathToDoxygen(path, pObj), 'public_include_top')

            topPage.AddDescription('<ul>\n')
            for file in os.listdir(path):
                if file.lower() in _ignore_dir: continue
                fullpath = os.path.join(path, file)
                if os.path.isfile(fullpath):
                    self.ProcessSourceFileForInclude(fullpath, pObj, configFile)
                    topPage.AddDescription('<li> \link %s\endlink </li>\n' % self._ConvertPathToDoxygen(fullpath, pObj))
                else:
                    if file.lower() in ['library', 'protocol', 'guid', 'ppi', 'ia32', 'x64', 'ipf', 'ebc', 'arm', 'pi', 'uefi', 'aarch64']:
                        continue
                    bNeedAddSubPage = False
                    subpage = doxygen.Page(self._ConvertPathToDoxygen(fullpath, pObj), 'public_include_%s' % file)
                    subpage.AddDescription('<ul>\n')
                    for subfile in os.listdir(fullpath):
                        if subfile.lower() in _ignore_dir: continue
                        bNeedAddSubPage = True
                        subfullpath = os.path.join(fullpath, subfile)
                        self.ProcessSourceFileForInclude(subfullpath, pObj, configFile)
                        subpage.AddDescription('<li> \link %s \endlink </li>\n' % self._ConvertPathToDoxygen(subfullpath, pObj))
                    subpage.AddDescription('</ul>\n')
                    if bNeedAddSubPage:
                        bNeedAddIncludePage = True
                        pageRoot.AddPage(subpage)
            topPage.AddDescription('</ul>\n')
            if bNeedAddIncludePage:
                pageRoot.AddPage(topPage)

        if pageRoot.GetSubpageCount() != 0:
            return [pageRoot]
        else:
            return []

    def GenerateLibraryClassesSubPage(self, pObj, configFile):
        """
        Generate sub page for library class for package.
        One DEC file maybe contains many library class sections
        for different architecture.

        @param  fObj DEC file object.
        """
        rootArray = []
        pageRoot = doxygen.Page("Library Class", "%s_libraryclass" % pObj.GetName())
        objs = pObj.GetFileObj().GetSectionObjectsByName('libraryclass', self._arch)
        if len(objs) == 0: return []

        if self._arch is not None:
            for obj in objs:
                classPage = doxygen.Page(obj.GetClassName(),
                                         "lc_%s" % obj.GetClassName())
                comments = obj.GetComment()
                if len(comments) != 0:
                    classPage.AddDescription('<br>\n'.join(comments) + '<br>\n')
                pageRoot.AddPage(classPage)
                path = os.path.join(pObj.GetFileObj().GetPackageRootPath(), obj.GetHeaderFile())
                path = path[len(pObj.GetWorkspace()) + 1:]
                if len(comments) == 0:
                    classPage.AddDescription('\copydoc %s<p>' % obj.GetHeaderFile())
                section = doxygen.Section('ref', 'Refer to Header File')
                section.AddDescription('\link %s\n' % obj.GetHeaderFile())
                section.AddDescription(' \endlink<p>\n')
                classPage.AddSection(section)
                fullPath = os.path.join(pObj.GetFileObj().GetPackageRootPath(), obj.GetHeaderFile())
                self.ProcessSourceFileForInclude(fullPath, pObj, configFile)
        else:
            archPageDict = {}
            for obj in objs:
                if obj.GetArch() not in archPageDict.keys():
                    archPageDict[obj.GetArch()] = doxygen.Page(obj.GetArch(),
                                                               'lc_%s' % obj.GetArch())
                    pageRoot.AddPage(archPageDict[obj.GetArch()])
                subArchRoot = archPageDict[obj.GetArch()]
                classPage = doxygen.Page(obj.GetClassName(),
                                         "lc_%s" % obj.GetClassName())
                comments = obj.GetComment()
                if len(comments) != 0:
                    classPage.AddDescription('<br>\n'.join(comments) + '<br>\n')
                subArchRoot.AddPage(classPage)
                path = os.path.join(pObj.GetFileObj().GetPackageRootPath(), obj.GetHeaderFile())
                path = path[len(pObj.GetWorkspace()) + 1:]
                if len(comments) == 0:
                    classPage.AddDescription('\copydoc %s<p>' % obj.GetHeaderFile())
                section = doxygen.Section('ref', 'Refer to Header File')
                section.AddDescription('\link %s\n' % obj.GetHeaderFile())
                section.AddDescription(' \endlink<p>\n')
                classPage.AddSection(section)
                fullPath = os.path.join(pObj.GetFileObj().GetPackageRootPath(), obj.GetHeaderFile())

                self.ProcessSourceFileForInclude(fullPath, pObj, configFile)
        rootArray.append(pageRoot)
        return rootArray

    def ProcessSourceFileForInclude(self, path, pObj, configFile, infObj=None):
        """
        @param path        the analysising file full path
        @param pObj        package object
        @param configFile  doxygen config file.
        """
        if gInGui:
            wx.Yield()
        if not os.path.exists(path):
            ErrorMsg('Source file path %s does not exist!' % path)
            return

        if configFile.FileExists(path):
            return

        try:
            with open(path, 'r') as f:
                lines = f.readlines()
        except UnicodeDecodeError:
            return
        except IOError:
            ErrorMsg('Fail to open file %s' % path)
            return

        configFile.AddFile(path)

        no = 0
        for no in range(len(lines)):
            if len(lines[no].strip()) == 0:
                continue
            if lines[no].strip()[:2] in ['##', '//', '/*', '*/']:
                continue
            index = lines[no].lower().find('include')
            #mo = IncludePattern.finditer(lines[no].lower())
            mo = re.match(r"^#\s*include\s+[<\"]([\\/\w.]+)[>\"]$", lines[no].strip().lower())
            if not mo:
                continue
            mo = re.match(r"^[#\w\s]+[<\"]([\\/\w.]+)[>\"]$", lines[no].strip())
            filePath = mo.groups()[0]

            if filePath is None or len(filePath) == 0:
                continue

            # find header file in module's path firstly.
            fullPath = None

            if os.path.exists(os.path.join(os.path.dirname(path), filePath)):
                # Find the file in current directory
                fullPath = os.path.join(os.path.dirname(path), filePath).replace('\\', '/')
            else:
                # find in depedent package's include path
                incObjs = pObj.GetFileObj().GetSectionObjectsByName('includes')
                for incObj in incObjs:
                    incPath = os.path.join(pObj.GetFileObj().GetPackageRootPath(), incObj.GetPath()).strip()
                    incPath = os.path.realpath(os.path.join(incPath, filePath))
                    if os.path.exists(incPath):
                        fullPath = incPath
                        break
                if infObj is not None:
                    pkgInfObjs = infObj.GetSectionObjectsByName('packages')
                    for obj in  pkgInfObjs:
                        decObj = dec.DECFile(os.path.join(pObj.GetWorkspace(), obj.GetPath()))
                        if not decObj:
                            ErrorMsg ('Fail to create pacakge object for %s' % obj.GetPackageName())
                            continue
                        if not decObj.Parse():
                            ErrorMsg ('Fail to load package object for %s' % obj.GetPackageName())
                            continue
                        incObjs = decObj.GetSectionObjectsByName('includes')
                        for incObj in incObjs:
                            incPath = os.path.join(decObj.GetPackageRootPath(), incObj.GetPath()).replace('\\', '/')
                            if os.path.exists(os.path.join(incPath, filePath)):
                                fullPath = os.path.join(os.path.join(incPath, filePath))
                                break
                        if fullPath is not None:
                            break

            if fullPath is None and self.IsVerbose():
                self.Log('Can not resolve header file %s for file %s in package %s\n' % (filePath, path, pObj.GetFileObj().GetFilename()), 'error')
                return
            else:
                fullPath = fullPath.replace('\\', '/')
                if self.IsVerbose():
                    self.Log('Preprocessing: Add include file %s for file %s\n' % (fullPath, path))
                #LogMsg ('Preprocessing: Add include file %s for file %s' % (fullPath, path))
                self.ProcessSourceFileForInclude(fullPath, pObj, configFile, infObj)

    def AddAllIncludeFiles(self, pObj, configFile):
        objs = pObj.GetFileObj().GetSectionObjectsByName('includes')
        for obj in objs:
            incPath = os.path.join(pObj.GetFileObj().GetPackageRootPath(), obj.GetPath())
            for root, dirs, files in os.walk(incPath):
                for dir in dirs:
                    if dir.lower() in _ignore_dir:
                        dirs.remove(dir)
                for file in files:
                    path = os.path.normpath(os.path.join(root, file))
                    configFile.AddFile(path.replace('/', '\\'))

    def GeneratePcdSubPages(self, pObj, configFile):
        """
        Generate sub pages for package's PCD definition.
        @param pObj         package object
        @param configFile   config file object
        """
        rootArray = []
        objs = pObj.GetFileObj().GetSectionObjectsByName('pcd')
        if len(objs) == 0:
            return []

        pcdRootPage = doxygen.Page('PCD', 'pcd_root_page')
        typeRootPageDict = {}
        typeArchRootPageDict = {}
        for obj in objs:
            if obj.GetPcdType() not in typeRootPageDict.keys():
                typeRootPageDict[obj.GetPcdType()] = doxygen.Page(obj.GetPcdType(), 'pcd_%s_root_page' % obj.GetPcdType())
                pcdRootPage.AddPage(typeRootPageDict[obj.GetPcdType()])
            typeRoot = typeRootPageDict[obj.GetPcdType()]
            if self._arch is not None:
                pcdPage = doxygen.Page('%s' % obj.GetPcdName(),
                                        'pcd_%s_%s_%s' % (obj.GetPcdType(), obj.GetArch(), obj.GetPcdName().split('.')[1]))
                pcdPage.AddDescription('<br>\n'.join(obj.GetComment()) + '<br>\n')
                section = doxygen.Section('PCDinformation', 'PCD Information')
                desc  = '<TABLE>'
                desc += '<TR>'
                desc += '<TD><CAPTION>Name</CAPTION></TD>'
                desc += '<TD><CAPTION>Token Space</CAPTION></TD>'
                desc += '<TD><CAPTION>Token number</CAPTION></TD>'
                desc += '<TD><CAPTION>Data Type</CAPTION></TD>'
                desc += '<TD><CAPTION>Default Value</CAPTION></TD>'
                desc += '</TR>'
                desc += '<TR>'
                desc += '<TD><CAPTION>%s</CAPTION></TD>' % obj.GetPcdName().split('.')[1]
                desc += '<TD><CAPTION>%s</CAPTION></TD>' % obj.GetPcdName().split('.')[0]
                desc += '<TD><CAPTION>%s</CAPTION></TD>' % obj.GetPcdToken()
                desc += '<TD><CAPTION>%s</CAPTION></TD>' % obj.GetPcdDataType()
                desc += '<TD><CAPTION>%s</CAPTION></TD>' % obj.GetPcdValue()
                desc += '</TR>'
                desc += '</TABLE>'
                section.AddDescription(desc)
                pcdPage.AddSection(section)
                typeRoot.AddPage(pcdPage)
            else:
                keystr = obj.GetPcdType() + obj.GetArch()
                if keystr not in typeArchRootPageDict.keys():
                    typeArchRootPage = doxygen.Page(obj.GetArch(), 'pcd_%s_%s_root_page' % (obj.GetPcdType(), obj.GetArch()))
                    typeArchRootPageDict[keystr] = typeArchRootPage
                    typeRoot.AddPage(typeArchRootPage)
                typeArchRoot = typeArchRootPageDict[keystr]
                pcdPage = doxygen.Page('%s' % obj.GetPcdName(),
                                        'pcd_%s_%s_%s' % (obj.GetPcdType(), obj.GetArch(), obj.GetPcdName().split('.')[1]))
                pcdPage.AddDescription('<br>\n'.join(obj.GetComment()) + '<br>\n')
                section = doxygen.Section('PCDinformation', 'PCD Information')
                desc  = '<TABLE>'
                desc += '<TR>'
                desc += '<TD><CAPTION>Name</CAPTION></TD>'
                desc += '<TD><CAPTION>Token Space</CAPTION></TD>'
                desc += '<TD><CAPTION>Token number</CAPTION></TD>'
                desc += '<TD><CAPTION>Data Type</CAPTION></TD>'
                desc += '<TD><CAPTION>Default Value</CAPTION></TD>'
                desc += '</TR>'
                desc += '<TR>'
                desc += '<TD><CAPTION>%s</CAPTION></TD>' % obj.GetPcdName().split('.')[1]
                desc += '<TD><CAPTION>%s</CAPTION></TD>' % obj.GetPcdName().split('.')[0]
                desc += '<TD><CAPTION>%s</CAPTION></TD>' % obj.GetPcdToken()
                desc += '<TD><CAPTION>%s</CAPTION></TD>' % obj.GetPcdDataType()
                desc += '<TD><CAPTION>%s</CAPTION></TD>' % obj.GetPcdValue()
                desc += '</TR>'
                desc += '</TABLE>'
                section.AddDescription(desc)
                pcdPage.AddSection(section)
                typeArchRoot.AddPage(pcdPage)
        return [pcdRootPage]

    def _GenerateGuidSubPage(self, pObj, obj, configFile):
        guidPage = doxygen.Page('%s' % obj.GetName(),
                                'guid_%s_%s' % (obj.GetArch(), obj.GetName()))
        comments = obj.GetComment()
        if len(comments) != 0:
            guidPage.AddDescription('<br>'.join(obj.GetComment()) + '<br>')
        section = doxygen.Section('BasicGuidInfo', 'GUID Information')
        desc  = '<TABLE>'
        desc += '<TR>'
        desc += '<TD><CAPTION>GUID\'s Guid Name</CAPTION></TD><TD><CAPTION>GUID\'s Guid</CAPTION></TD>'
        desc += '</TR>'
        desc += '<TR>'
        desc += '<TD>%s</TD>' % obj.GetName()
        desc += '<TD>%s</TD>' % obj.GetGuid()
        desc += '</TR>'
        desc += '</TABLE>'
        section.AddDescription(desc)
        guidPage.AddSection(section)
        refFile = self.FindHeaderFileForGuid(pObj, obj.GetName(), configFile)
        if refFile:
            relPath = refFile[len(pObj.GetWorkspace()) + 1:]
            if len(comments) == 0:
                guidPage.AddDescription(' \\copydoc %s <br>' % relPath)

            section = doxygen.Section('ref', 'Refer to Header File')
            section.AddDescription('\link %s\n' % relPath)
            section.AddDescription('\endlink\n')
            self.ProcessSourceFileForInclude(refFile, pObj, configFile)
            guidPage.AddSection(section)
        return guidPage

    def GenerateGuidSubPages(self, pObj, configFile):
        """
        Generate sub pages for package's GUID definition.
        @param  pObj            package object
        @param  configFilf      doxygen config file object
        """
        pageRoot = doxygen.Page('GUID', 'guid_root_page')
        objs = pObj.GetFileObj().GetSectionObjectsByName('guids', self._arch)
        if len(objs) == 0: return []
        if self._arch is not None:
            for obj in objs:
                pageRoot.AddPage(self._GenerateGuidSubPage(pObj, obj, configFile))
        else:
            guidArchRootPageDict = {}
            for obj in objs:
                if obj.GetArch() not in guidArchRootPageDict.keys():
                    guidArchRoot = doxygen.Page(obj.GetArch(), 'guid_arch_root_%s' % obj.GetArch())
                    pageRoot.AddPage(guidArchRoot)
                    guidArchRootPageDict[obj.GetArch()] = guidArchRoot
                guidArchRoot = guidArchRootPageDict[obj.GetArch()]
                guidArchRoot.AddPage(self._GenerateGuidSubPage(pObj, obj, configFile))
        return [pageRoot]

    def _GeneratePpiSubPage(self, pObj, obj, configFile):
        guidPage = doxygen.Page(obj.GetName(), 'ppi_page_%s' % obj.GetName())
        comments = obj.GetComment()
        if len(comments) != 0:
            guidPage.AddDescription('<br>'.join(obj.GetComment()) + '<br>')
        section = doxygen.Section('BasicPpiInfo', 'PPI Information')
        desc  = '<TABLE>'
        desc += '<TR>'
        desc += '<TD><CAPTION>PPI\'s Guid Name</CAPTION></TD><TD><CAPTION>PPI\'s Guid</CAPTION></TD>'
        desc += '</TR>'
        desc += '<TR>'
        desc += '<TD>%s</TD>' % obj.GetName()
        desc += '<TD>%s</TD>' % obj.GetGuid()
        desc += '</TR>'
        desc += '</TABLE>'
        section.AddDescription(desc)
        guidPage.AddSection(section)
        refFile = self.FindHeaderFileForGuid(pObj, obj.GetName(), configFile)
        if refFile:
            relPath = refFile[len(pObj.GetWorkspace()) + 1:]
            if len(comments) == 0:
                guidPage.AddDescription(' \\copydoc %s <br>' % relPath)
            section = doxygen.Section('ref', 'Refer to Header File')
            section.AddDescription('\link %s\n' % relPath)
            section.AddDescription('\endlink\n')
            self.ProcessSourceFileForInclude(refFile, pObj, configFile)
            guidPage.AddSection(section)

        return guidPage

    def GeneratePpiSubPages(self, pObj, configFile):
        """
        Generate sub pages for package's GUID definition.
        @param  pObj            package object
        @param  configFilf      doxygen config file object
        """
        pageRoot = doxygen.Page('PPI', 'ppi_root_page')
        objs = pObj.GetFileObj().GetSectionObjectsByName('ppis', self._arch)
        if len(objs) == 0: return []
        if self._arch is not None:
            for obj in objs:
                pageRoot.AddPage(self._GeneratePpiSubPage(pObj, obj, configFile))
        else:
            guidArchRootPageDict = {}
            for obj in objs:
                if obj.GetArch() not in guidArchRootPageDict.keys():
                    guidArchRoot = doxygen.Page(obj.GetArch(), 'ppi_arch_root_%s' % obj.GetArch())
                    pageRoot.AddPage(guidArchRoot)
                    guidArchRootPageDict[obj.GetArch()] = guidArchRoot
                guidArchRoot = guidArchRootPageDict[obj.GetArch()]
                guidArchRoot.AddPage(self._GeneratePpiSubPage(pObj, obj, configFile))
        return [pageRoot]

    def _GenerateProtocolSubPage(self, pObj, obj, configFile):
        guidPage = doxygen.Page(obj.GetName(), 'protocol_page_%s' % obj.GetName())
        comments = obj.GetComment()
        if len(comments) != 0:
            guidPage.AddDescription('<br>'.join(obj.GetComment()) + '<br>')
        section = doxygen.Section('BasicProtocolInfo', 'PROTOCOL Information')
        desc  = '<TABLE>'
        desc += '<TR>'
        desc += '<TD><CAPTION>PROTOCOL\'s Guid Name</CAPTION></TD><TD><CAPTION>PROTOCOL\'s Guid</CAPTION></TD>'
        desc += '</TR>'
        desc += '<TR>'
        desc += '<TD>%s</TD>' % obj.GetName()
        desc += '<TD>%s</TD>' % obj.GetGuid()
        desc += '</TR>'
        desc += '</TABLE>'
        section.AddDescription(desc)
        guidPage.AddSection(section)

        refFile = self.FindHeaderFileForGuid(pObj, obj.GetName(), configFile)
        if refFile:
            relPath = refFile[len(pObj.GetWorkspace()) + 1:]
            if len(comments) == 0:
                guidPage.AddDescription(' \\copydoc %s <br>' % relPath)
            section = doxygen.Section('ref', 'Refer to Header File')
            section.AddDescription('\link %s\n' % relPath)
            section.AddDescription('\endlink\n')
            self.ProcessSourceFileForInclude(refFile, pObj, configFile)
            guidPage.AddSection(section)

        return guidPage

    def GenerateProtocolSubPages(self, pObj, configFile):
        """
        Generate sub pages for package's GUID definition.
        @param  pObj            package object
        @param  configFilf      doxygen config file object
        """
        pageRoot = doxygen.Page('PROTOCOL', 'protocol_root_page')
        objs = pObj.GetFileObj().GetSectionObjectsByName('protocols', self._arch)
        if len(objs) == 0: return []
        if self._arch is not None:
            for obj in objs:
                pageRoot.AddPage(self._GenerateProtocolSubPage(pObj, obj, configFile))
        else:
            guidArchRootPageDict = {}
            for obj in objs:
                if obj.GetArch() not in guidArchRootPageDict.keys():
                    guidArchRoot = doxygen.Page(obj.GetArch(), 'protocol_arch_root_%s' % obj.GetArch())
                    pageRoot.AddPage(guidArchRoot)
                    guidArchRootPageDict[obj.GetArch()] = guidArchRoot
                guidArchRoot = guidArchRootPageDict[obj.GetArch()]
                guidArchRoot.AddPage(self._GenerateProtocolSubPage(pObj, obj, configFile))
        return [pageRoot]

    def FindHeaderFileForGuid(self, pObj, name, configFile):
        """
        For declaration header file for GUID/PPI/Protocol.

        @param pObj         package object
        @param name         guid/ppi/protocol's name
        @param configFile   config file object

        @return full path of header file and None if not found.
        """
        startPath  = pObj.GetFileObj().GetPackageRootPath()
        incPath    = os.path.join(startPath, 'Include').replace('\\', '/')
        # if <PackagePath>/include exist, then search header under it.
        if os.path.exists(incPath):
            startPath = incPath

        for root, dirs, files in os.walk(startPath):
            for dir in dirs:
                if dir.lower() in _ignore_dir:
                    dirs.remove(dir)
            for file in files:
                fPath = os.path.join(root, file)
                if not IsCHeaderFile(fPath):
                    continue
                try:
                    f = open(fPath, 'r')
                    lines = f.readlines()
                    f.close()
                except IOError:
                    self.Log('Fail to open file %s\n' % fPath)
                    continue
                for line in lines:
                    if line.find(name) != -1 and \
                       line.find('extern') != -1:
                        return fPath.replace('\\', '/')
        return None

    def GetPackageModuleList(self, pObj):
        """
        Get all module's INF path under package's root path
        @param     pObj  package object
        @return    arrary of INF full path
        """
        mArray = []
        packPath = pObj.GetFileObj().GetPackageRootPath()
        if not os.path.exists:
            return None
        for root, dirs, files in os.walk(packPath):
            for dir in dirs:
                if dir.lower() in _ignore_dir:
                    dirs.remove(dir)
            for file in files:
                if CheckPathPostfix(file, 'inf'):
                    fPath = os.path.join(root, file).replace('\\', '/')
                    mArray.append(fPath)
        return mArray

    def GenerateModulePages(self, pObj, configFile):
        """
        Generate sub pages for package's module which is under the package
        root directory.

        @param  pObj            package object
        @param  configFilf      doxygen config file object
        """
        infList = self.GetPackageModuleList(pObj)
        rootPages = []
        libObjs = []
        modObjs = []
        for infpath in infList:
            infObj = inf.INFFile(infpath)
            #infObj = INFFileObject.INFFile (pObj.GetWorkspacePath(),
            #                                inf)
            if not infObj:
                self.Log('Fail create INF object for %s' % inf)
                continue
            if not infObj.Parse():
                self.Log('Fail to load INF file %s' % inf)
                continue
            if infObj.GetProduceLibraryClass() is not None:
                libObjs.append(infObj)
            else:
                modObjs.append(infObj)

        if len(libObjs) != 0:
            libRootPage = doxygen.Page('Libraries', 'lib_root_page')
            rootPages.append(libRootPage)
            for libInf in libObjs:
                libRootPage.AddPage(self.GenerateModulePage(pObj, libInf, configFile, True))

        if len(modObjs) != 0:
            modRootPage = doxygen.Page('Modules', 'module_root_page')
            rootPages.append(modRootPage)
            for modInf in modObjs:
                modRootPage.AddPage(self.GenerateModulePage(pObj, modInf, configFile, False))

        return rootPages

    def GenerateModulePage(self, pObj, infObj, configFile, isLib):
        """
        Generate page for a module/library.
        @param infObj     INF file object for module/library
        @param configFile doxygen config file object
        @param isLib      Whether this module is libary

        @param module doxygen page object
        """
        workspace = pObj.GetWorkspace()
        refDecObjs = []
        for obj in  infObj.GetSectionObjectsByName('packages'):
            decObj = dec.DECFile(os.path.join(workspace, obj.GetPath()))
            if not decObj:
                ErrorMsg ('Fail to create pacakge object for %s' % obj.GetPackageName())
                continue
            if not decObj.Parse():
                ErrorMsg ('Fail to load package object for %s' % obj.GetPackageName())
                continue
            refDecObjs.append(decObj)

        modPage = doxygen.Page('%s' % infObj.GetBaseName(),
                               'module_%s' % infObj.GetBaseName())
        modPage.AddDescription(infObj.GetFileHeader())

        basicInfSection = doxygen.Section('BasicModuleInformation', 'Basic Module Information')
        desc = "<TABLE>"
        for obj in infObj.GetSectionObjectsByName('defines'):
            key = obj.GetKey()
            value = obj.GetValue()
            if key not in _inf_key_description_mapping_table.keys(): continue
            if key == 'LIBRARY_CLASS' and value.find('|') != -1:
                clsname, types = value.split('|')
                desc += '<TR>'
                desc += '<TD><B>%s</B></TD>' % _inf_key_description_mapping_table[key]
                desc += '<TD>%s</TD>' % clsname
                desc += '</TR>'

                desc += '<TR>'
                desc += '<TD><B>Supported Module Types</B></TD>'
                desc += '<TD>%s</TD>' % types
                desc += '</TR>'
            else:
                desc += '<TR>'
                desc += '<TD><B>%s</B></TD>' % _inf_key_description_mapping_table[key]
                if key == 'EFI_SPECIFICATION_VERSION' and value == '0x00020000':
                    value = '2.0'
                desc += '<TD>%s</TD>' % value
                desc += '</TR>'
        desc += '</TABLE>'
        basicInfSection.AddDescription(desc)
        modPage.AddSection(basicInfSection)

        # Add protocol section
        data  = []
        for obj in infObj.GetSectionObjectsByName('pcd', self._arch):
            data.append(obj.GetPcdName().strip())
        if len(data) != 0:
            s = doxygen.Section('Pcds', 'Pcds')
            desc = "<TABLE>"
            desc += '<TR><TD><B>PCD Name</B></TD><TD><B>TokenSpace</B></TD><TD><B>Package</B></TD></TR>'
            for item in data:
                desc += '<TR>'
                desc += '<TD>%s</TD>' % item.split('.')[1]
                desc += '<TD>%s</TD>' % item.split('.')[0]
                pkgbasename = self.SearchPcdPackage(item, workspace, refDecObjs)
                desc += '<TD>%s</TD>' % pkgbasename
                desc += '</TR>'
            desc += "</TABLE>"
            s.AddDescription(desc)
            modPage.AddSection(s)

        # Add protocol section
        #sects = infObj.GetSectionByString('protocol')
        data  = []
        #for sect in sects:
        for obj in infObj.GetSectionObjectsByName('protocol', self._arch):
            data.append(obj.GetName().strip())
        if len(data) != 0:
            s = doxygen.Section('Protocols', 'Protocols')
            desc = "<TABLE>"
            desc += '<TR><TD><B>Name</B></TD><TD><B>Package</B></TD></TR>'
            for item in data:
                desc += '<TR>'
                desc += '<TD>%s</TD>' % item
                pkgbasename = self.SearchProtocolPackage(item, workspace, refDecObjs)
                desc += '<TD>%s</TD>' % pkgbasename
                desc += '</TR>'
            desc += "</TABLE>"
            s.AddDescription(desc)
            modPage.AddSection(s)

        # Add ppi section
        #sects = infObj.GetSectionByString('ppi')
        data  = []
        #for sect in sects:
        for obj in infObj.GetSectionObjectsByName('ppi', self._arch):
            data.append(obj.GetName().strip())
        if len(data) != 0:
            s = doxygen.Section('Ppis', 'Ppis')
            desc = "<TABLE>"
            desc += '<TR><TD><B>Name</B></TD><TD><B>Package</B></TD></TR>'
            for item in data:
                desc += '<TR>'
                desc += '<TD>%s</TD>' % item
                pkgbasename = self.SearchPpiPackage(item, workspace, refDecObjs)
                desc += '<TD>%s</TD>' % pkgbasename
                desc += '</TR>'
            desc += "</TABLE>"
            s.AddDescription(desc)
            modPage.AddSection(s)

        # Add guid section
        #sects = infObj.GetSectionByString('guid')
        data  = []
        #for sect in sects:
        for obj in infObj.GetSectionObjectsByName('guid', self._arch):
            data.append(obj.GetName().strip())
        if len(data) != 0:
            s = doxygen.Section('Guids', 'Guids')
            desc = "<TABLE>"
            desc += '<TR><TD><B>Name</B></TD><TD><B>Package</B></TD></TR>'
            for item in data:
                desc += '<TR>'
                desc += '<TD>%s</TD>' % item
                pkgbasename = self.SearchGuidPackage(item, workspace, refDecObjs)
                desc += '<TD>%s</TD>' % pkgbasename
                desc += '</TR>'
            desc += "</TABLE>"
            s.AddDescription(desc)
            modPage.AddSection(s)

        section = doxygen.Section('LibraryClasses', 'Library Classes')
        desc = "<TABLE>"
        desc += '<TR><TD><B>Name</B></TD><TD><B>Type</B></TD><TD><B>Package</B></TD><TD><B>Header File</B></TD></TR>'
        if isLib:
            desc += '<TR>'
            desc += '<TD>%s</TD>' % infObj.GetProduceLibraryClass()
            desc += '<TD>Produce</TD>'
            try:
                pkgname, hPath = self.SearchLibraryClassHeaderFile(infObj.GetProduceLibraryClass(),
                                                              workspace,
                                                              refDecObjs)
            except:
                self.Log ('fail to get package header file for lib class %s' % infObj.GetProduceLibraryClass())
                pkgname = 'NULL'
                hPath   = 'NULL'
            desc += '<TD>%s</TD>' % pkgname
            if hPath != "NULL":
                desc += '<TD>\link %s \endlink</TD>' % hPath
            else:
                desc += '<TD>%s</TD>' % hPath
            desc += '</TR>'
        for lcObj in infObj.GetSectionObjectsByName('libraryclasses', self._arch):
            desc += '<TR>'
            desc += '<TD>%s</TD>' % lcObj.GetClass()
            retarr = self.SearchLibraryClassHeaderFile(lcObj.GetClass(),
                                                       workspace,
                                                       refDecObjs)
            if retarr is not None:
                pkgname, hPath = retarr
            else:
                self.Log('Fail find the library class %s definition from module %s dependent package!' % (lcObj.GetClass(), infObj.GetFilename()), 'error')
                pkgname = 'NULL'
                hPath   = 'NULL'
            desc += '<TD>Consume</TD>'
            desc += '<TD>%s</TD>' % pkgname
            desc += '<TD>\link %s \endlink</TD>' % hPath
            desc += '</TR>'
        desc += "</TABLE>"
        section.AddDescription(desc)
        modPage.AddSection(section)

        section = doxygen.Section('SourceFiles', 'Source Files')
        section.AddDescription('<ul>\n')
        for obj in infObj.GetSourceObjects(self._arch, self._tooltag):
            sPath = infObj.GetModuleRootPath()
            sPath = os.path.join(sPath, obj.GetSourcePath()).replace('\\', '/').strip()
            if sPath.lower().endswith('.uni') or sPath.lower().endswith('.s') or sPath.lower().endswith('.asm') or sPath.lower().endswith('.nasm'):
                newPath = self.TranslateUniFile(sPath)
                configFile.AddFile(newPath)
                newPath = newPath[len(pObj.GetWorkspace()) + 1:]
                section.AddDescription('<li> \link %s \endlink </li>' %  newPath)
            else:
                self.ProcessSourceFileForInclude(sPath, pObj, configFile, infObj)
                sPath = sPath[len(pObj.GetWorkspace()) + 1:]
                section.AddDescription('<li>\link %s \endlink </li>' % sPath)
        section.AddDescription('</ul>\n')
        modPage.AddSection(section)

        #sects = infObj.GetSectionByString('depex')
        data  = []
        #for sect in sects:
        for obj in infObj.GetSectionObjectsByName('depex'):
            data.append(str(obj))
        if len(data) != 0:
            s = doxygen.Section('DependentSection', 'Module Dependencies')
            s.AddDescription('<br>'.join(data))
            modPage.AddSection(s)

        return modPage

    def TranslateUniFile(self, path):
        newpath = path + '.dox'
        #import core.textfile as textfile
        #file = textfile.TextFile(path)

        try:
            file = open(path, 'r')
        except (IOError, OSError) as msg:
            return None

        t = file.read()
        file.close()

        output = '/** @file \n'
        #output = '<html><body>'
        arr = t.split('\r\n')
        for line in arr:
            if line.find('@file') != -1:
                continue
            if line.find('*/') != -1:
                continue
            line = line.strip()
            if line.strip().startswith('/'):
                arr = line.split(' ')
                if len(arr) > 1:
                    line = ' '.join(arr[1:])
                else:
                    continue
            output += '%s<br>\n' % line
        output += '**/'

        if os.path.exists(newpath):
            os.remove(newpath)

        file = open(newpath, "w")
        file.write(output)
        file.close()
        return newpath

    def SearchPcdPackage(self, pcdname, workspace, decObjs):
        for decObj in  decObjs:
            for pcd in decObj.GetSectionObjectsByName('pcd'):
                if pcdname == pcd.GetPcdName():
                    return decObj.GetBaseName()
        return None

    def SearchProtocolPackage(self, protname, workspace, decObjs):
        for decObj in  decObjs:
            for proto in decObj.GetSectionObjectsByName('protocol'):
                if protname == proto.GetName():
                    return decObj.GetBaseName()
        return None

    def SearchPpiPackage(self, ppiname, workspace, decObjs):
        for decObj in  decObjs:
            for ppi in decObj.GetSectionObjectsByName('ppi'):
                if ppiname == ppi.GetName():
                    return decObj.GetBaseName()
        return None

    def SearchGuidPackage(self, guidname, workspace, decObjs):
        for decObj in  decObjs:
            for guid in decObj.GetSectionObjectsByName('guid'):
                if guidname == guid.GetName():
                    return decObj.GetBaseName()
        return None

    def SearchLibraryClassHeaderFile(self, className, workspace, decObjs):
        for decObj in  decObjs:
            for cls in decObj.GetSectionObjectsByName('libraryclasses'):
                if cls.GetClassName().strip() == className:
                    path = cls.GetHeaderFile().strip()
                    path = os.path.join(decObj.GetPackageRootPath(), path)
                    path = path[len(workspace) + 1:]
                    return decObj.GetBaseName(), path.replace('\\', '/')

        return None

    def _ConvertPathToDoxygen(self, path, pObj):
        pRootPath = pObj.GetWorkspace()
        path = path[len(pRootPath) + 1:]
        return path.replace('\\', '/')

def IsCHeaderFile(path):
    return CheckPathPostfix(path, 'h')

def CheckPathPostfix(path, str):
    index = path.rfind('.')
    if index == -1:
        return False
    if path[index + 1:].lower() == str.lower():
        return True
    return False
