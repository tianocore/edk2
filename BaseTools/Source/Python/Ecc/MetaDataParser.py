## @file
# This file is used to define common parser functions for meta-data
#
# Copyright (c) 2008 - 2014, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

import Common.LongFilePathOs as os
from CommonDataClass.DataClass import *
from EccToolError import *
import EccGlobalData
import re
## Get the inlcude path list for a source file
#
# 1. Find the source file belongs to which inf file
# 2. Find the inf's package
# 3. Return the include path list of the package
#
def GetIncludeListOfFile(WorkSpace, Filepath, Db):
    IncludeList = []
    Filepath = os.path.normpath(Filepath)
    SqlCommand = """
                select Value1, FullPath from Inf, File where Inf.Model = %s and Inf.BelongsToFile in(
                    select distinct B.BelongsToFile from File as A left join Inf as B
                        where A.ID = B.BelongsToFile and B.Model = %s and (A.Path || '%s' || B.Value1) = '%s')
                        and Inf.BelongsToFile = File.ID""" \
                % (MODEL_META_DATA_PACKAGE, MODEL_EFI_SOURCE_FILE, '\\', Filepath)
    RecordSet = Db.TblFile.Exec(SqlCommand)
    for Record in RecordSet:
        DecFullPath = os.path.normpath(os.path.join(WorkSpace, Record[0]))
        InfFullPath = os.path.normpath(os.path.join(WorkSpace, Record[1]))
        (DecPath, DecName) = os.path.split(DecFullPath)
        (InfPath, InfName) = os.path.split(InfFullPath)
        SqlCommand = """select Value1 from Dec where BelongsToFile =
                           (select ID from File where FullPath = '%s') and Model = %s""" \
                    % (DecFullPath, MODEL_EFI_INCLUDE)
        NewRecordSet = Db.TblDec.Exec(SqlCommand)
        if InfPath not in IncludeList:
            IncludeList.append(InfPath)
        for NewRecord in NewRecordSet:
            IncludePath = os.path.normpath(os.path.join(DecPath, NewRecord[0]))
            if IncludePath not in IncludeList:
                IncludeList.append(IncludePath)

    return IncludeList

## Get the file list
#
# Search table file and find all specific type files
#
def GetFileList(FileModel, Db):
    FileList = []
    SqlCommand = """select FullPath from File where Model = %s""" % str(FileModel)
    RecordSet = Db.TblFile.Exec(SqlCommand)
    for Record in RecordSet:
        FileList.append(Record[0])

    return FileList

## Get the table list
#
# Search table file and find all small tables
#
def GetTableList(FileModelList, Table, Db):
    TableList = []
    SqlCommand = """select ID from File where Model in %s""" % str(FileModelList)
    RecordSet = Db.TblFile.Exec(SqlCommand)
    for Record in RecordSet:
        TableName = Table + str(Record[0])
        TableList.append(TableName)

    return TableList

## ParseHeaderCommentSection
#
# Parse Header comment section lines, extract Abstract, Description, Copyright
# , License lines
#
# @param CommentList:   List of (Comment, LineNumber)
# @param FileName:      FileName of the comment
#
def ParseHeaderCommentSection(CommentList, FileName = None):
    
    Abstract = ''
    Description = ''
    Copyright = ''
    License = ''
    EndOfLine = "\n"
    STR_HEADER_COMMENT_START = "@file"
    
    #
    # used to indicate the state of processing header comment section of dec, 
    # inf files
    #
    HEADER_COMMENT_NOT_STARTED = -1
    HEADER_COMMENT_STARTED     = 0
    HEADER_COMMENT_FILE        = 1
    HEADER_COMMENT_ABSTRACT    = 2
    HEADER_COMMENT_DESCRIPTION = 3
    HEADER_COMMENT_COPYRIGHT   = 4
    HEADER_COMMENT_LICENSE     = 5
    HEADER_COMMENT_END         = 6
    #
    # first find the last copyright line
    #
    Last = 0
    HeaderCommentStage = HEADER_COMMENT_NOT_STARTED
    for Index in xrange(len(CommentList)-1, 0, -1):
        Line = CommentList[Index][0]
        if _IsCopyrightLine(Line):
            Last = Index
            break
    
    for Item in CommentList:
        Line = Item[0]
        LineNo = Item[1]
        
        if not Line.startswith('#') and Line:
            SqlStatement = """ select ID from File where FullPath like '%s'""" % FileName
            ResultSet = EccGlobalData.gDb.TblFile.Exec(SqlStatement)
            for Result in ResultSet:
                Msg = 'Comment must start with #'
                EccGlobalData.gDb.TblReport.Insert(ERROR_DOXYGEN_CHECK_FILE_HEADER, Msg, "File", Result[0])
        Comment = CleanString2(Line)[1]
        Comment = Comment.strip()
        #
        # if there are blank lines between License or Description, keep them as they would be 
        # indication of different block; or in the position that Abstract should be, also keep it
        # as it indicates that no abstract
        #
        if not Comment and HeaderCommentStage not in [HEADER_COMMENT_LICENSE, \
                                                      HEADER_COMMENT_DESCRIPTION, HEADER_COMMENT_ABSTRACT]:
            continue
        
        if HeaderCommentStage == HEADER_COMMENT_NOT_STARTED:
            if Comment.startswith(STR_HEADER_COMMENT_START):
                HeaderCommentStage = HEADER_COMMENT_ABSTRACT
            else:
                License += Comment + EndOfLine
        else:
            if HeaderCommentStage == HEADER_COMMENT_ABSTRACT:
                #
                # in case there is no abstract and description
                #
                if not Comment:
                    Abstract = ''
                    HeaderCommentStage = HEADER_COMMENT_DESCRIPTION
                elif _IsCopyrightLine(Comment):                    
                    Copyright += Comment + EndOfLine
                    HeaderCommentStage = HEADER_COMMENT_COPYRIGHT
                else:                    
                    Abstract += Comment + EndOfLine
                    HeaderCommentStage = HEADER_COMMENT_DESCRIPTION
            elif HeaderCommentStage == HEADER_COMMENT_DESCRIPTION:
                #
                # in case there is no description
                #                
                if _IsCopyrightLine(Comment):                    
                    Copyright += Comment + EndOfLine
                    HeaderCommentStage = HEADER_COMMENT_COPYRIGHT
                else:
                    Description += Comment + EndOfLine                
            elif HeaderCommentStage == HEADER_COMMENT_COPYRIGHT:
                if _IsCopyrightLine(Comment):                    
                    Copyright += Comment + EndOfLine
                else:
                    #
                    # Contents after copyright line are license, those non-copyright lines in between
                    # copyright line will be discarded 
                    #
                    if LineNo > Last:
                        if License:
                            License += EndOfLine
                        License += Comment + EndOfLine
                        HeaderCommentStage = HEADER_COMMENT_LICENSE                
            else:
                if not Comment and not License:
                    continue
                License += Comment + EndOfLine
    
    if not Copyright.strip():
        SqlStatement = """ select ID from File where FullPath like '%s'""" % FileName
        ResultSet = EccGlobalData.gDb.TblFile.Exec(SqlStatement)
        for Result in ResultSet:
            Msg = 'Header comment section must have copyright information'
            EccGlobalData.gDb.TblReport.Insert(ERROR_DOXYGEN_CHECK_FILE_HEADER, Msg, "File", Result[0])

    if not License.strip():
        SqlStatement = """ select ID from File where FullPath like '%s'""" % FileName
        ResultSet = EccGlobalData.gDb.TblFile.Exec(SqlStatement)
        for Result in ResultSet:
            Msg = 'Header comment section must have license information'
            EccGlobalData.gDb.TblReport.Insert(ERROR_DOXYGEN_CHECK_FILE_HEADER, Msg, "File", Result[0])
                       
    if not Abstract.strip() or Abstract.find('Component description file') > -1:
        SqlStatement = """ select ID from File where FullPath like '%s'""" % FileName
        ResultSet = EccGlobalData.gDb.TblFile.Exec(SqlStatement)
        for Result in ResultSet:
            Msg = 'Header comment section must have Abstract information.'
            EccGlobalData.gDb.TblReport.Insert(ERROR_DOXYGEN_CHECK_FILE_HEADER, Msg, "File", Result[0])
                     
    return Abstract.strip(), Description.strip(), Copyright.strip(), License.strip()

## _IsCopyrightLine
# check whether current line is copyright line, the criteria is whether there is case insensitive keyword "Copyright" 
# followed by zero or more white space characters followed by a "(" character 
#
# @param LineContent:  the line need to be checked
# @return: True if current line is copyright line, False else
#
def _IsCopyrightLine (LineContent):
    LineContent = LineContent.upper()
    Result = False
    
    ReIsCopyrightRe = re.compile(r"""(^|\s)COPYRIGHT *\(""", re.DOTALL)
    if ReIsCopyrightRe.search(LineContent):
        Result = True
        
    return Result


## CleanString2
#
# Split comments in a string
# Remove spaces
#
# @param Line:              The string to be cleaned
# @param CommentCharacter:  Comment char, used to ignore comment content, 
#                           default is DataType.TAB_COMMENT_SPLIT
#
def CleanString2(Line, CommentCharacter='#', AllowCppStyleComment=False):
    #
    # remove whitespace
    #
    Line = Line.strip()
    #
    # Replace EDK1's comment character
    #
    if AllowCppStyleComment:
        Line = Line.replace('//', CommentCharacter)
    #
    # separate comments and statements
    #
    LineParts = Line.split(CommentCharacter, 1)
    #
    # remove whitespace again
    #
    Line = LineParts[0].strip()
    if len(LineParts) > 1:
        Comment = LineParts[1].strip()
        #
        # Remove prefixed and trailing comment characters
        #
        Start = 0
        End = len(Comment)
        while Start < End and Comment.startswith(CommentCharacter, Start, End):
            Start += 1
        while End >= 0 and Comment.endswith(CommentCharacter, Start, End):
            End -= 1
        Comment = Comment[Start:End]
        Comment = Comment.strip()
    else:
        Comment = ''

    return Line, Comment
