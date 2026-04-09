## @file
# This file is used to create/update/query/erase table for ECC reports
#
# Copyright (c) 2008 - 2018, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

##
# Import Modules
#
from __future__ import absolute_import
import Common.EdkLogger as EdkLogger
import Common.LongFilePathOs as os, time
from Table.Table import Table
from Common.StringUtils import ConvertToSqlString2
import Ecc.EccToolError as EccToolError
import Ecc.EccGlobalData as EccGlobalData
from Common.LongFilePathSupport import OpenLongFilePath as open

## TableReport
#
# This class defined a table used for data model
#
# @param object:       Inherited from object class
#
#
class TableReport(Table):
    def __init__(self, Cursor):
        Table.__init__(self, Cursor)
        self.Table = 'Report'

    ## Create table
    #
    # Create table report
    #
    # @param ID:             ID of an Error
    # @param ErrorID:        ID of an Error TypeModel of a Report item
    # @param OtherMsg:       Other error message besides the standard error message
    # @param BelongsToItem:  The error belongs to which item
    # @param Enabled:        If this error enabled
    # @param Corrected:      if this error corrected
    #
    def Create(self):
        SqlCommand = """create table IF NOT EXISTS %s (ID INTEGER PRIMARY KEY,
                                                       ErrorID INTEGER NOT NULL,
                                                       OtherMsg TEXT,
                                                       BelongsToTable TEXT NOT NULL,
                                                       BelongsToItem SINGLE NOT NULL,
                                                       Enabled INTEGER DEFAULT 0,
                                                       Corrected INTEGER DEFAULT -1
                                                      )""" % self.Table
        Table.Create(self, SqlCommand)

    ## Insert table
    #
    # Insert a record into table report
    #
    # @param ID:             ID of an Error
    # @param ErrorID:        ID of an Error TypeModel of a report item
    # @param OtherMsg:       Other error message besides the standard error message
    # @param BelongsToTable: The error item belongs to which table
    # @param BelongsToItem:  The error belongs to which item
    # @param Enabled:        If this error enabled
    # @param Corrected:      if this error corrected
    #
    def Insert(self, ErrorID, OtherMsg='', BelongsToTable='', BelongsToItem= -1, Enabled=0, Corrected= -1):
        self.ID = self.ID + 1
        SqlCommand = """insert into %s values(%s, %s, '%s', '%s', %s, %s, %s)""" \
                     % (self.Table, self.ID, ErrorID, ConvertToSqlString2(OtherMsg), BelongsToTable, BelongsToItem, Enabled, Corrected)
        Table.Insert(self, SqlCommand)

        return self.ID

    ## Query table
    #
    # @retval:       A recordSet of all found records
    #
    def Query(self):
        SqlCommand = """select ID, ErrorID, OtherMsg, BelongsToTable, BelongsToItem, Corrected from %s
                        where Enabled > -1 order by ErrorID, BelongsToItem""" % (self.Table)
        return self.Exec(SqlCommand)

    ## Update table
    #
    def UpdateBelongsToItemByFile(self, ItemID=-1, File=""):
        SqlCommand = """update Report set BelongsToItem=%s where BelongsToTable='File' and BelongsToItem=-2
                        and OtherMsg like '%%%s%%'""" % (ItemID, File)
        return self.Exec(SqlCommand)

    ## Convert to CSV
    #
    # Get all enabled records from table report and save them to a .csv file
    #
    # @param Filename:  To filename to save the report content
    #
    def ToCSV(self, Filename='Report.csv'):
        try:
            File = open(Filename, 'w+')
            File.write("""No, Error Code, Error Message, File, LineNo, Other Error Message\n""")
            RecordSet = self.Query()
            Index = 0
            for Record in RecordSet:
                Index = Index + 1
                ErrorID = Record[1]
                OtherMsg = Record[2]
                BelongsToTable = Record[3]
                BelongsToItem = Record[4]
                IsCorrected = Record[5]
                SqlCommand = ''
                if BelongsToTable == 'File':
                    SqlCommand = """select 1, FullPath from %s where ID = %s
                             """ % (BelongsToTable, BelongsToItem)
                else:
                    SqlCommand = """select A.StartLine, B.FullPath from %s as A, File as B
                                    where A.ID = %s and B.ID = A.BelongsToFile
                                 """ % (BelongsToTable, BelongsToItem)
                NewRecord = self.Exec(SqlCommand)
                if NewRecord != []:
                    File.write("""%s,%s,"%s",%s,%s,"%s"\n""" % (Index, ErrorID, EccToolError.gEccErrorMessage[ErrorID], NewRecord[0][1], NewRecord[0][0], OtherMsg))
                    EdkLogger.quiet("%s(%s): [%s]%s %s" % (NewRecord[0][1], NewRecord[0][0], ErrorID, EccToolError.gEccErrorMessage[ErrorID], OtherMsg))

            File.close()
        except IOError:
            NewFilename = 'Report_' + time.strftime("%Y%m%d_%H%M%S.csv", time.localtime())
            EdkLogger.warn("ECC", "The report file %s is locked by other progress, use %s instead!" % (Filename, NewFilename))
            self.ToCSV(NewFilename)

