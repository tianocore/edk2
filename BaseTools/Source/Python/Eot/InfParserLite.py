## @file
# This file is used to parse INF file of EDK project
#
# Copyright (c) 2008 - 2018, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

##
# Import Modules
#
from __future__ import print_function
from __future__ import absolute_import

import Common.LongFilePathOs as os
import Common.EdkLogger as EdkLogger
from Common.DataType import *
from CommonDataClass.DataClass import *
from Eot.Identification import Identification
from Common.StringUtils import *
from Eot.Parser import *
from Eot import Database
from Eot import EotGlobalData

## EdkInfParser() class
#
# This class defined basic INF object which is used by inheriting
#
# @param object:       Inherited from object class
#
class EdkInfParser(object):
    ## The constructor
    #
    #  @param  self: The object pointer
    #  @param  Filename: INF file name
    #  @param  Database: Eot database
    #  @param  SourceFileList: A list for all source file belonging this INF file
    #
    def __init__(self, Filename = None, Database = None, SourceFileList = None):
        self.Identification = Identification()
        self.Sources = []
        self.Macros = {}

        self.Cur = Database.Cur
        self.TblFile = Database.TblFile
        self.TblInf = Database.TblInf
        self.FileID = -1

        # Load Inf file if filename is not None
        if Filename is not None:
            self.LoadInfFile(Filename)

        if SourceFileList:
            for Item in SourceFileList:
                self.TblInf.Insert(MODEL_EFI_SOURCE_FILE, Item, '', '', '', '', 'COMMON', -1, self.FileID, -1, -1, -1, -1, 0)


    ## LoadInffile() method
    #
    #  Load INF file and insert a record in database
    #
    #  @param  self: The object pointer
    #  @param Filename:  Input value for filename of Inf file
    #
    def LoadInfFile(self, Filename = None):
        # Insert a record for file
        Filename = NormPath(Filename)
        self.Identification.FileFullPath = Filename
        (self.Identification.FileRelativePath, self.Identification.FileName) = os.path.split(Filename)

        self.FileID = self.TblFile.InsertFile(Filename, MODEL_FILE_INF)

        self.ParseInf(PreProcess(Filename, False), self.Identification.FileRelativePath, Filename)

    ## ParserSource() method
    #
    #  Parse Source section and insert records in database
    #
    #  @param self: The object pointer
    #  @param CurrentSection: current section name
    #  @param SectionItemList: the item belonging current section
    #  @param ArchList: A list for arch for this section
    #  @param ThirdList: A list for third item for this section
    #
    def ParserSource(self, CurrentSection, SectionItemList, ArchList, ThirdList):
        for Index in range(0, len(ArchList)):
            Arch = ArchList[Index]
            Third = ThirdList[Index]
            if Arch == '':
                Arch = TAB_ARCH_COMMON

            for Item in SectionItemList:
                if CurrentSection.upper() == 'defines'.upper():
                    (Name, Value) = AddToSelfMacro(self.Macros, Item[0])
                    self.TblInf.Insert(MODEL_META_DATA_HEADER, Name, Value, Third, '', '', Arch, -1, self.FileID, Item[1], -1, Item[1], -1, 0)

    ## ParseInf() method
    #
    #  Parse INF file and get sections information
    #
    #  @param self: The object pointer
    #  @param Lines: contents of INF file
    #  @param FileRelativePath: relative path of the file
    #  @param Filename: file name of INF file
    #
    def ParseInf(self, Lines = [], FileRelativePath = '', Filename = ''):
        IfDefList, SectionItemList, CurrentSection, ArchList, ThirdList, IncludeFiles = \
        [], [], TAB_UNKNOWN, [], [], []
        LineNo = 0

        for Line in Lines:
            LineNo = LineNo + 1
            if Line == '':
                continue
            if Line.startswith(TAB_SECTION_START) and Line.endswith(TAB_SECTION_END):
                self.ParserSource(CurrentSection, SectionItemList, ArchList, ThirdList)

                # Parse the new section
                SectionItemList = []
                ArchList = []
                ThirdList = []
                # Parse section name
                CurrentSection = ''
                LineList = GetSplitValueList(Line[len(TAB_SECTION_START):len(Line) - len(TAB_SECTION_END)], TAB_COMMA_SPLIT)
                for Item in LineList:
                    ItemList = GetSplitValueList(Item, TAB_SPLIT)
                    if CurrentSection == '':
                        CurrentSection = ItemList[0]
                    else:
                        if CurrentSection != ItemList[0]:
                            EdkLogger.error("Parser", PARSER_ERROR, "Different section names '%s' and '%s' are found in one section definition, this is not allowed." % (CurrentSection, ItemList[0]), File=Filename, Line=LineNo)
                    ItemList.append('')
                    ItemList.append('')
                    if len(ItemList) > 5:
                        RaiseParserError(Line, CurrentSection, Filename, '', LineNo)
                    else:
                        ArchList.append(ItemList[1].upper())
                        ThirdList.append(ItemList[2])

                continue

            # Add a section item
            SectionItemList.append([Line, LineNo])
            # End of parse

        self.ParserSource(CurrentSection, SectionItemList, ArchList, ThirdList)
        #End of For


