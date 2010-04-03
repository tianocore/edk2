## @file
# This file is used to define common parser functions for meta-data
#
# Copyright (c) 2008, Intel Corporation
# All rights reserved. This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

import os
from CommonDataClass.DataClass import *


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

