## @file
# This file is used to create a database used by EOT tool
#
# Copyright (c) 2007 - 2014, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

##
# Import Modules
#
import sqlite3
import Common.LongFilePathOs as os, time

import Common.EdkLogger as EdkLogger
import CommonDataClass.DataClass as DataClass

from Table.TableDataModel import TableDataModel
from Table.TableFile import TableFile
from Table.TableFunction import TableFunction
from Table.TableIdentifier import TableIdentifier
from Table.TableEotReport import TableEotReport
from Table.TableInf import TableInf
from Table.TableDec import TableDec
from Table.TableDsc import TableDsc
from Table.TableFdf import TableFdf
from Table.TableQuery import TableQuery

##
# Static definitions
#
DATABASE_PATH = "Eot.db"

## Database class
#
# This class defined the EOT databse
# During the phase of initialization, the database will create all tables and
# insert all records of table DataModel
#
class Database(object):
    ## The constructor
    #
    #   @param  self:      The object pointer
    #   @param  DbPath:    The file path of the database
    #
    def __init__(self, DbPath):
        self.DbPath = DbPath
        self.Conn = None
        self.Cur = None
        self.TblDataModel = None
        self.TblFile = None
        self.TblFunction = None
        self.TblIdentifier = None
        self.TblReport = None
        self.TblInf = None
        self.TblDec = None
        self.TblDsc = None
        self.TblFdf = None
        self.TblQuery = None
        self.TblQuery2 = None

    ## InitDatabase() method
    #  1. Delete all old existing tables
    #  2. Create new tables
    #  3. Initialize table DataModel
    #
    #  @param self: The object pointer
    #  @param NewDatabase: Check if it needs to create a new database
    #
    def InitDatabase(self, NewDatabase = True):
        EdkLogger.verbose("\nInitialize EOT database started ...")
        #
        # Drop all old existing tables
        #
        if NewDatabase:
            if os.path.exists(self.DbPath):
                os.remove(self.DbPath)
        self.Conn = sqlite3.connect(self.DbPath, isolation_level = 'DEFERRED')
        self.Conn.execute("PRAGMA page_size=8192")
        self.Conn.execute("PRAGMA synchronous=OFF")
        # to avoid non-ascii charater conversion error
        self.Conn.text_factory = str
        self.Cur = self.Conn.cursor()

        self.TblDataModel = TableDataModel(self.Cur)
        self.TblFile = TableFile(self.Cur)
        self.TblFunction = TableFunction(self.Cur)
        self.TblIdentifier = TableIdentifier(self.Cur)
        self.TblReport = TableEotReport(self.Cur)
        self.TblInf = TableInf(self.Cur)
        self.TblDec = TableDec(self.Cur)
        self.TblDsc = TableDsc(self.Cur)
        self.TblFdf = TableFdf(self.Cur)
        self.TblQuery = TableQuery(self.Cur)
        self.TblQuery2 = TableQuery(self.Cur)
        self.TblQuery2.Table = 'Query2'

        # Create new tables
        if NewDatabase:
            self.TblDataModel.Create()
            self.TblFile.Create()
            self.TblFunction.Create()
            self.TblReport.Create()
            self.TblInf.Create()
            self.TblDec.Create()
            self.TblDsc.Create()
            self.TblFdf.Create()
            self.TblQuery.Create()
            self.TblQuery2.Create()

        # Init each table's ID
        self.TblDataModel.InitID()
        self.TblFile.InitID()
        self.TblFunction.InitID()
        self.TblReport.InitID()
        self.TblInf.InitID()
        self.TblDec.InitID()
        self.TblDsc.InitID()
        self.TblFdf.InitID()
        self.TblQuery.Drop()
        self.TblQuery.Create()
        self.TblQuery.InitID()
        self.TblQuery2.Drop()
        self.TblQuery2.Create()
        self.TblQuery2.InitID()

        # Initialize table DataModel
        if NewDatabase:
            self.TblDataModel.InitTable()

        EdkLogger.verbose("Initialize EOT database ... DONE!")

    ## QueryTable() method
    #
    #  Query a table
    #
    #  @param self: The object pointer
    #  @param Table: The instance of the table to be queried
    #
    def QueryTable(self, Table):
        Table.Query()

    ## Close() method
    #
    # Commit all first
    # Close the connection and cursor
    #
    def Close(self):
        # Commit to file
        self.Conn.commit()

        # Close connection and cursor
        self.Cur.close()
        self.Conn.close()

    ## InsertOneFile() method
    #
    # Insert one file's information to the database
    # 1. Create a record in TableFile
    # 2. Create functions one by one
    #    2.1 Create variables of function one by one
    #    2.2 Create pcds of function one by one
    # 3. Create variables one by one
    # 4. Create pcds one by one
    #
    # @param self: The object pointer
    # @param File: The object of the file to be inserted
    #
    def InsertOneFile(self, File):
        # Insert a record for file
        FileID = self.TblFile.Insert(File.Name, File.ExtName, File.Path, File.FullPath, Model = File.Model, TimeStamp = File.TimeStamp)
        IdTable = TableIdentifier(self.Cur)
        IdTable.Table = "Identifier%s" % FileID
        IdTable.Create()

        # Insert function of file
        for Function in File.FunctionList:
            FunctionID = self.TblFunction.Insert(Function.Header, Function.Modifier, Function.Name, Function.ReturnStatement, \
                                    Function.StartLine, Function.StartColumn, Function.EndLine, Function.EndColumn, \
                                    Function.BodyStartLine, Function.BodyStartColumn, FileID, \
                                    Function.FunNameStartLine, Function.FunNameStartColumn)

            # Insert Identifier of function
            for Identifier in Function.IdentifierList:
                IdentifierID = IdTable.Insert(Identifier.Modifier, Identifier.Type, Identifier.Name, Identifier.Value, Identifier.Model, \
                                        FileID, FunctionID, Identifier.StartLine, Identifier.StartColumn, Identifier.EndLine, Identifier.EndColumn)
        # Insert Identifier of file
        for Identifier in File.IdentifierList:
            IdentifierID = IdTable.Insert(Identifier.Modifier, Identifier.Type, Identifier.Name, Identifier.Value, Identifier.Model, \
                                    FileID, -1, Identifier.StartLine, Identifier.StartColumn, Identifier.EndLine, Identifier.EndColumn)

        EdkLogger.verbose("Insert information from file %s ... DONE!" % File.FullPath)

    ## UpdateIdentifierBelongsToFunction() method
    #
    #  Update the field "BelongsToFunction" for each Indentifier
    #
    #  @param self: The object pointer
    #
    def UpdateIdentifierBelongsToFunction(self):
        EdkLogger.verbose("Update 'BelongsToFunction' for Identifiers started ...")

        SqlCommand = """select ID, BelongsToFile, StartLine, EndLine from Function"""
        Records = self.TblFunction.Exec(SqlCommand)
        Data1 = []
        Data2 = []
        for Record in Records:
            FunctionID = Record[0]
            BelongsToFile = Record[1]
            StartLine = Record[2]
            EndLine = Record[3]

            SqlCommand = """Update Identifier%s set BelongsToFunction = %s where BelongsToFile = %s and StartLine > %s and EndLine < %s""" % \
                        (BelongsToFile, FunctionID, BelongsToFile, StartLine, EndLine)
            self.TblIdentifier.Exec(SqlCommand)

            SqlCommand = """Update Identifier%s set BelongsToFunction = %s, Model = %s where BelongsToFile = %s and Model = %s and EndLine = %s""" % \
                         (BelongsToFile, FunctionID, DataClass.MODEL_IDENTIFIER_FUNCTION_HEADER, BelongsToFile, DataClass.MODEL_IDENTIFIER_COMMENT, StartLine - 1)
            self.TblIdentifier.Exec(SqlCommand)


##
#
# This acts like the main() function for the script, unless it is 'import'ed into another
# script.
#
if __name__ == '__main__':
    EdkLogger.Initialize()
    EdkLogger.SetLevel(EdkLogger.DEBUG_0)
    EdkLogger.verbose("Start at " + time.strftime('%H:%M:%S', time.localtime()))

    Db = Database(DATABASE_PATH)
    Db.InitDatabase()
    Db.QueryTable(Db.TblDataModel)

    identifier1 = DataClass.IdentifierClass(-1, '', '', "i''1", 'aaa', DataClass.MODEL_IDENTIFIER_COMMENT, 1, -1, 32,  43,  54,  43)
    identifier2 = DataClass.IdentifierClass(-1, '', '', 'i1', 'aaa', DataClass.MODEL_IDENTIFIER_COMMENT, 1, -1, 15,  43,  20,  43)
    identifier3 = DataClass.IdentifierClass(-1, '', '', 'i1', 'aaa', DataClass.MODEL_IDENTIFIER_COMMENT, 1, -1, 55,  43,  58,  43)
    identifier4 = DataClass.IdentifierClass(-1, '', '', "i1'", 'aaa', DataClass.MODEL_IDENTIFIER_COMMENT, 1, -1, 77,  43,  88,  43)
    fun1 = DataClass.FunctionClass(-1, '', '', 'fun1', '', 21, 2, 60,  45, 1, 23, 0, [], [])
    file = DataClass.FileClass(-1, 'F1', 'c', 'C:\\', 'C:\\F1.exe', DataClass.MODEL_FILE_C, '2007-12-28', [fun1], [identifier1, identifier2, identifier3, identifier4], [])
    Db.InsertOneFile(file)

    Db.QueryTable(Db.TblFile)
    Db.QueryTable(Db.TblFunction)
    Db.QueryTable(Db.TblIdentifier)

    Db.Close()
    EdkLogger.verbose("End at " + time.strftime('%H:%M:%S', time.localtime()))

