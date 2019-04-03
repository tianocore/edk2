## @file
#
# Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

from __future__ import print_function
from __future__ import absolute_import
import os

from .message import *

class BaseDoxygeItem:
    def __init__(self, name, tag=''):
        self.mName = name
        self.mTag  = tag
        self.mDescription = ''
        self.mText = []

    def AddDescription(self, desc):
        self.mDescription = '%s%s' % (self.mDescription, desc)

    def __str__(self):
        return '\n'.join(self.mText)

    def Generate(self):
        """This interface need to be override"""

class Section(BaseDoxygeItem):
    def Generate(self):
        """This interface need to be override"""
        if len(self.mTag) != 0:
            self.mText.append(' \section %s %s' % (self.mName, self.mTag))
        else:
            self.mText.append(' \section %s' % self.mName)

        self.mText.append(self.mDescription)
        return self.mText

class Page(BaseDoxygeItem):
    def __init__(self, name, tag=None, isSort=True):
        BaseDoxygeItem.__init__(self, name, tag)
        self.mSubPages     = []
        self.mIsMainPage   = False
        self.mSections     = []
        self.mIsSort       = isSort

    def GetSubpageCount(self):
        return len(self.mSubPages)

    def AddPage(self, subpage):
        self.mSubPages.append(subpage)
        return subpage

    def AddPages(self, pageArray):
        if pageArray is None:
            return
        for page in pageArray:
            self.AddPage(page)

    def AddSection(self, section):
        self.mSections.append(section)
        self.mSections.sort(key=lambda x: x.mName.lower())

    def Generate(self):
        if self.mIsMainPage:
            self.mText.append('/** \mainpage %s' % self.mName)
            self.mIsSort = False
        else:
            self.mText.append('/** \page %s %s' % (self.mTag, self.mName))

        if len(self.mDescription) != 0:
            self.mText.append(self.mDescription)
        endIndex = len(self.mText)

        self.mSections.sort(key=lambda x: x.mName.lower())
        for sect in self.mSections:
            self.mText += sect.Generate()

        endIndex = len(self.mText)

        if len(self.mSubPages) != 0:
            self.mText.insert(endIndex, "<p> \section content_index INDEX")
            endIndex = len(self.mText)
            self.mText.insert(endIndex, '<ul>')
            endIndex += 1
            if self.mIsSort:
                self.mSubPages.sort(key=lambda x: x.mName.lower())
            for page in self.mSubPages:
                self.mText.insert(endIndex, '<li>\subpage %s \"%s\" </li>' % (page.mTag, page.mName))
                endIndex += 1
                self.mText += page.Generate()
            self.mText.insert(endIndex, '</ul>')
            endIndex += 1
        self.mText.insert(endIndex, ' **/')
        return self.mText

class DoxygenFile(Page):
    def __init__(self, name, file):
        Page.__init__(self, name)
        self.mFilename  = file
        self.mIsMainPage = True

    def GetFilename(self):
        return self.mFilename.replace('/', '\\')

    def Save(self):
        str = self.Generate()
        try:
            f = open(self.mFilename, 'w')
            f.write('\n'.join(str))
            f.close()
        except IOError as e:
            ErrorMsg ('Fail to write file %s' % self.mFilename)
            return False

        return True

doxygenConfigTemplate = """
DOXYFILE_ENCODING      = UTF-8
PROJECT_NAME           = %(ProjectName)s
PROJECT_NUMBER         = %(ProjectVersion)s
OUTPUT_DIRECTORY       = %(OutputDir)s
CREATE_SUBDIRS         = YES
OUTPUT_LANGUAGE        = English
BRIEF_MEMBER_DESC      = YES
REPEAT_BRIEF           = YES
ABBREVIATE_BRIEF       = "The $name class           " \\
                         "The $name widget           " \\
                         "The $name file           " \\
                         is \\
                         provides \\
                         specifies \\
                         contains \\
                         represents \\
                         a \\
                         an \\
                         the
ALWAYS_DETAILED_SEC    = NO
INLINE_INHERITED_MEMB  = NO
FULL_PATH_NAMES        = YES
STRIP_FROM_PATH        = %(StripPath)s
STRIP_FROM_INC_PATH    =
SHORT_NAMES            = YES
JAVADOC_AUTOBRIEF      = NO
QT_AUTOBRIEF           = NO
MULTILINE_CPP_IS_BRIEF = NO
DETAILS_AT_TOP         = YES
INHERIT_DOCS           = YES
SEPARATE_MEMBER_PAGES  = NO
TAB_SIZE               = 1
ALIASES                =
OPTIMIZE_OUTPUT_FOR_C  = YES
OPTIMIZE_OUTPUT_JAVA   = NO
BUILTIN_STL_SUPPORT    = NO
CPP_CLI_SUPPORT        = NO
SIP_SUPPORT            = NO
DISTRIBUTE_GROUP_DOC   = YES
SUBGROUPING            = YES
TYPEDEF_HIDES_STRUCT   = NO

EXTRACT_ALL            = YES
EXTRACT_PRIVATE        = NO
EXTRACT_STATIC         = NO
EXTRACT_LOCAL_CLASSES  = NO
EXTRACT_LOCAL_METHODS  = NO
EXTRACT_ANON_NSPACES   = NO
HIDE_UNDOC_MEMBERS     = NO
HIDE_UNDOC_CLASSES     = NO
HIDE_FRIEND_COMPOUNDS  = NO
HIDE_IN_BODY_DOCS      = NO
INTERNAL_DOCS          = NO
CASE_SENSE_NAMES       = NO
HIDE_SCOPE_NAMES       = NO
SHOW_INCLUDE_FILES     = NO
INLINE_INFO            = YES
SORT_MEMBER_DOCS       = YES
SORT_BRIEF_DOCS        = NO
SORT_BY_SCOPE_NAME     = YES
GENERATE_TODOLIST      = YES
GENERATE_TESTLIST      = YES
GENERATE_BUGLIST       = YES
GENERATE_DEPRECATEDLIST= YES
ENABLED_SECTIONS       =
MAX_INITIALIZER_LINES  = 30
SHOW_USED_FILES        = NO
SHOW_DIRECTORIES       = NO
FILE_VERSION_FILTER    =

QUIET                  = NO
WARNINGS               = YES
WARN_IF_UNDOCUMENTED   = YES
WARN_IF_DOC_ERROR      = YES
WARN_NO_PARAMDOC       = YES
WARN_FORMAT            = "$file:$line: $text           "
WARN_LOGFILE           = %(WarningFile)s

INPUT                  = %(FileList)s
INPUT_ENCODING         = UTF-8
FILE_PATTERNS          = %(Pattern)s
RECURSIVE              = NO
EXCLUDE                = *.svn
EXCLUDE_SYMLINKS       = NO
EXCLUDE_PATTERNS       = .svn
EXCLUDE_SYMBOLS        =
EXAMPLE_PATH           = %(ExamplePath)s
EXAMPLE_PATTERNS       = *
EXAMPLE_RECURSIVE      = NO
IMAGE_PATH             =
INPUT_FILTER           =
FILTER_PATTERNS        =
FILTER_SOURCE_FILES    = NO

SOURCE_BROWSER         = NO
INLINE_SOURCES         = NO
STRIP_CODE_COMMENTS    = YES
REFERENCED_BY_RELATION = YES
REFERENCES_RELATION    = YES
REFERENCES_LINK_SOURCE = NO
USE_HTAGS              = NO
VERBATIM_HEADERS       = NO

ALPHABETICAL_INDEX     = NO
COLS_IN_ALPHA_INDEX    = 5
IGNORE_PREFIX          =

GENERATE_HTML          = YES
HTML_OUTPUT            = html
HTML_FILE_EXTENSION    = .html
HTML_HEADER            =
HTML_FOOTER            =
HTML_STYLESHEET        =
HTML_ALIGN_MEMBERS     = YES
GENERATE_HTMLHELP      = %(WhetherGenerateHtmlHelp)s
HTML_DYNAMIC_SECTIONS  = NO
CHM_FILE               = index.chm
HHC_LOCATION           =
GENERATE_CHI           = NO
BINARY_TOC             = NO
TOC_EXPAND             = NO
DISABLE_INDEX          = NO
ENUM_VALUES_PER_LINE   = 4
GENERATE_TREEVIEW      = %(WhetherGenerateTreeView)s
TREEVIEW_WIDTH         = 250

GENERATE_LATEX         = NO
LATEX_OUTPUT           = latex
LATEX_CMD_NAME         = latex
MAKEINDEX_CMD_NAME     = makeindex
COMPACT_LATEX          = NO
PAPER_TYPE             = a4wide
EXTRA_PACKAGES         =
LATEX_HEADER           =
PDF_HYPERLINKS         = YES
USE_PDFLATEX           = YES
LATEX_BATCHMODE        = NO
LATEX_HIDE_INDICES     = NO

GENERATE_RTF           = NO
RTF_OUTPUT             = rtf
COMPACT_RTF            = NO
RTF_HYPERLINKS         = NO
RTF_STYLESHEET_FILE    =
RTF_EXTENSIONS_FILE    =

GENERATE_MAN           = NO
MAN_OUTPUT             = man
MAN_EXTENSION          = .3
MAN_LINKS              = NO

GENERATE_XML           = NO
XML_OUTPUT             = xml
XML_SCHEMA             =
XML_DTD                =
XML_PROGRAMLISTING     = YES

GENERATE_AUTOGEN_DEF   = NO

GENERATE_PERLMOD       = NO
PERLMOD_LATEX          = NO
PERLMOD_PRETTY         = YES
PERLMOD_MAKEVAR_PREFIX =

ENABLE_PREPROCESSING   = YES
MACRO_EXPANSION        = YES
EXPAND_ONLY_PREDEF     = YES
SEARCH_INCLUDES        = YES
INCLUDE_PATH           = %(IncludePath)s
INCLUDE_FILE_PATTERNS  = *.h
PREDEFINED             = %(PreDefined)s
EXPAND_AS_DEFINED      =
SKIP_FUNCTION_MACROS   = NO

TAGFILES               =
GENERATE_TAGFILE       =
ALLEXTERNALS           = NO
EXTERNAL_GROUPS        = YES
PERL_PATH              = /usr/bin/perl

CLASS_DIAGRAMS         = NO
MSCGEN_PATH            =
HIDE_UNDOC_RELATIONS   = YES
HAVE_DOT               = NO
CLASS_GRAPH            = YES
COLLABORATION_GRAPH    = YES
GROUP_GRAPHS           = YES
UML_LOOK               = NO
TEMPLATE_RELATIONS     = NO
INCLUDE_GRAPH          = YES
INCLUDED_BY_GRAPH      = YES
CALL_GRAPH             = NO
CALLER_GRAPH           = NO
GRAPHICAL_HIERARCHY    = YES
DIRECTORY_GRAPH        = YES
DOT_IMAGE_FORMAT       = png
DOT_PATH               =
DOTFILE_DIRS           =
DOT_GRAPH_MAX_NODES    = 50
MAX_DOT_GRAPH_DEPTH    = 1000
DOT_TRANSPARENT        = YES
DOT_MULTI_TARGETS      = NO
GENERATE_LEGEND        = YES
DOT_CLEANUP            = YES

SEARCHENGINE           = NO

"""
class DoxygenConfigFile:
    def __init__(self):
        self.mProjectName  = ''
        self.mOutputDir    = ''
        self.mFileList     = []
        self.mIncludeList  = []
        self.mStripPath    = ''
        self.mExamplePath  = ''
        self.mPattern      = ['*.c', '*.h',
                              '*.asm', '*.s', '.nasm', '*.html', '*.dox']
        self.mMode         = 'HTML'
        self.mWarningFile  = ''
        self.mPreDefined   = []
        self.mProjectVersion = 0.1

    def SetChmMode(self):
        self.mMode = 'CHM'

    def SetHtmlMode(self):
        self.mMode = 'HTML'

    def SetProjectName(self, str):
        self.mProjectName = str

    def SetProjectVersion(self, str):
        self.mProjectVersion = str

    def SetOutputDir(self, str):
        self.mOutputDir = str

    def SetStripPath(self, str):
        self.mStripPath = str

    def SetExamplePath(self, str):
        self.mExamplePath = str

    def SetWarningFilePath(self, str):
        self.mWarningFile = str.replace('\\', '/')

    def FileExists(self, path):
        if path is None:
            return False
        if len(path) == 0:
            return False

        for p in self.mFileList:
            if path.lower() == p.lower():
                return True

        return False

    def AddFile(self, path):
        if path is None:
            return

        if len(path) == 0:
            return
        path = path.replace('\\', '/')
        if not self.FileExists(path):
            self.mFileList.append(path)

    def AddIncludePath(self, path):
        path = path.replace('\\', '/')
        if path not in self.mIncludeList:
            self.mIncludeList.append(path)

    def AddPattern(self, pattern):
        self.mPattern.append(pattern)

    def AddPreDefined(self, macro):
        self.mPreDefined.append(macro)

    def Generate(self, path):
        files    = ' \\\n'.join(self.mFileList)
        includes = ' \\\n'.join(self.mIncludeList)
        patterns = ' \\\n'.join(self.mPattern)
        if self.mMode.lower() == 'html':
            sHtmlHelp = 'NO'
            sTreeView = 'YES'
        else:
            sHtmlHelp = 'YES'
            sTreeView = 'NO'

        text = doxygenConfigTemplate % {'ProjectName':self.mProjectName,
                                        'OutputDir':self.mOutputDir,
                                        'StripPath':self.mStripPath,
                                        'ExamplePath':self.mExamplePath,
                                        'FileList':files,
                                        'Pattern':patterns,
                                        'WhetherGenerateHtmlHelp':sHtmlHelp,
                                        'WhetherGenerateTreeView':sTreeView,
                                        'IncludePath':includes,
                                        'WarningFile':self.mWarningFile,
                                        'PreDefined':' '.join(self.mPreDefined),
                                        'ProjectVersion':self.mProjectVersion}
        try:
            f = open(path, 'w')
            f.write(text)
            f.close()
        except IOError as e:
            ErrorMsg ('Fail to generate doxygen config file %s' % path)
            return False

        return True

########################################################################
#                  TEST                   CODE
########################################################################
if __name__== '__main__':
    df = DoxygenFile('Platform Document', 'm:\tree')
    df.AddPage(Page('Module', 'module'))
    p = df.AddPage(Page('Library', 'library'))
    p.AddDescription(desc)
    p.AddPage(Page('PCD', 'pcds'))

    df.Generate()
    print(df)
