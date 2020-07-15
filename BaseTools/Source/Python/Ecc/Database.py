## @file
# This file is used to create a database used by ECC tool
#
# Copyright (c) 2007 - 2014, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

##
# Import Modules
#
from __future__ import absolute_import
import sqlite3
import Common.LongFilePathOs as os, time

import Common.EdkLogger as EdkLogger
import CommonDataClass.DataClass as DataClass

from Table.TableDataModel import TableDataModel
from Table.TableFile import TableFile
from Table.TableFunction import TableFunction
from Table.TablePcd import TablePcd
from Table.TableIdentifier import TableIdentifier
from Table.TableReport import TableReport
from Ecc.MetaFileWorkspace.MetaFileTable import ModuleTable
from Ecc.MetaFileWorkspace.MetaFileTable import PackageTable
from Ecc.MetaFileWorkspace.MetaFileTable import PlatformTable
from Table.TableFdf import TableFdf

##
# Static definitions
#
DATABASE_PATH = "Ecc.db"

## Database
#
# This class defined the ECC database
# During the phase of initialization, the database will create all tables and
# insert all records of table DataModel
#
# @param object:    Inherited from object class
# @param DbPath:    A string for the path of the ECC database
#
# @var Conn:        Connection of the ECC database
# @var Cur:         Cursor of the connection
# @var TblDataModel:  Local instance for TableDataModel
#
class Database(object):
    def __init__(self, DbPath):
        self.DbPath = DbPath
        self.Conn = None
        self.Cur = None
        self.TblDataModel = None
        self.TblFile = None
        self.TblFunction = None
        self.TblIdentifier = None
        self.TblPcd = None
        self.TblReport = None
        self.TblInf = None
        self.TblDec = None
        self.TblDsc = None
        self.TblFdf = None

    ## Initialize ECC database
    #
    # 1. Delete all old existing tables
    # 2. Create new tables
    # 3. Initialize table DataModel
    #
    def InitDatabase(self, NewDatabase = True):
        EdkLogger.verbose("\nInitialize ECC database started ...")
        #
        # Drop all old existing tables
        #
        if NewDatabase:
            if os.path.exists(self.DbPath):
                os.remove(self.DbPath)
        self.Conn = sqlite3.connect(self.DbPath, isolation_level = 'DEFERRED')
        self.Conn.execute("PRAGMA page_size=4096")
        self.Conn.execute("PRAGMA synchronous=OFF")
        # to avoid non-ascii character conversion error
        self.Conn.text_factory = str
        self.Cur = self.Conn.cursor()

        self.TblDataModel = TableDataModel(self.Cur)
        self.TblFile = TableFile(self.Cur)
        self.TblFunction = TableFunction(self.Cur)
        self.TblIdentifier = TableIdentifier(self.Cur)
        self.TblPcd = TablePcd(self.Cur)
        self.TblReport = TableReport(self.Cur)
        self.TblInf = ModuleTable(self.Cur)
        self.TblDec = PackageTable(self.Cur)
        self.TblDsc = PlatformTable(self.Cur)
        self.TblFdf = TableFdf(self.Cur)

        #
        # Create new tables
        #
        if NewDatabase:
            self.TblDataModel.Create()
            self.TblFile.Create()
            self.TblFunction.Create()
            self.TblPcd.Create()
            self.TblReport.Create()
            self.TblInf.Create()
            self.TblDec.Create()
            self.TblDsc.Create()
            self.TblFdf.Create()

        #
        # Init each table's ID
        #
        self.TblDataModel.InitID()
        self.TblFile.InitID()
        self.TblFunction.InitID()
        self.TblPcd.InitID()
        self.TblReport.InitID()
        self.TblInf.InitID()
        self.TblDec.InitID()
        self.TblDsc.InitID()
        self.TblFdf.InitID()

        #
        # Initialize table DataModel
        #
        if NewDatabase:
            self.TblDataModel.InitTable()

        EdkLogger.verbose("Initialize ECC database ... DONE!")

    ## Query a table
    #
    # @param Table:  The instance of the table to be queried
    #
    def QueryTable(self, Table):
        Table.Query()

    ## Close entire database
    #
    # Commit all first
    # Close the connection and cursor
    #
    def Close(self):
        #
        # Commit to file
        #
        self.Conn.commit()

        #
        # Close connection and cursor
        #
        self.Cur.close()
        self.Conn.close()

    ## Insert one file information
    #
    # Insert one file's information to the database
    # 1. Create a record in TableFile
    # 2. Create functions one by one
    #    2.1 Create variables of function one by one
    #    2.2 Create pcds of function one by one
    # 3. Create variables one by one
    # 4. Create pcds one by one
    #
    def InsertOneFile(self, File):
        #
        # Insert a record for file
        #
        FileID = self.TblFile.Insert(File.Name, File.ExtName, File.Path, File.FullPath, Model = File.Model, TimeStamp = File.TimeStamp)

        if File.Model == DataClass.MODEL_FILE_C or File.Model == DataClass.MODEL_FILE_H:
            IdTable = TableIdentifier(self.Cur)
            IdTable.Table = "Identifier%s" % FileID
            IdTable.Create()
            #
            # Insert function of file
            #
            for Function in File.FunctionList:
                FunctionID = self.TblFunction.Insert(Function.Header, Function.Modifier, Function.Name, Function.ReturnStatement, \
                                        Function.StartLine, Function.StartColumn, Function.EndLine, Function.EndColumn, \
                                        Function.BodyStartLine, Function.BodyStartColumn, FileID, \
                                        Function.FunNameStartLine, Function.FunNameStartColumn)
                #
                # Insert Identifier of function
                #
                for Identifier in Function.IdentifierList:
                    IdentifierID = IdTable.Insert(Identifier.Modifier, Identifier.Type, Identifier.Name, Identifier.Value, Identifier.Model, \
                                            FileID, FunctionID, Identifier.StartLine, Identifier.StartColumn, Identifier.EndLine, Identifier.EndColumn)
                #
                # Insert Pcd of function
                #
                for Pcd in Function.PcdList:
                    PcdID = self.TblPcd.Insert(Pcd.CName, Pcd.TokenSpaceGuidCName, Pcd.Token, Pcd.DatumType, Pcd.Model, \
                                       FileID, FunctionID, Pcd.StartLine, Pcd.StartColumn, Pcd.EndLine, Pcd.EndColumn)
            #
            # Insert Identifier of file
            #
            for Identifier in File.IdentifierList:
                IdentifierID = IdTable.Insert(Identifier.Modifier, Identifier.Type, Identifier.Name, Identifier.Value, Identifier.Model, \
                                        FileID, -1, Identifier.StartLine, Identifier.StartColumn, Identifier.EndLine, Identifier.EndColumn)
            #
            # Insert Pcd of file
            #
            for Pcd in File.PcdList:
                PcdID = self.TblPcd.Insert(Pcd.CName, Pcd.TokenSpaceGuidCName, Pcd.Token, Pcd.DatumType, Pcd.Model, \
                                   FileID, -1, Pcd.StartLine, Pcd.StartColumn, Pcd.EndLine, Pcd.EndColumn)

        EdkLogger.verbose("Insert information from file %s ... DONE!" % File.FullPath)

    ## UpdateIdentifierBelongsToFunction
    #
    # Update the field "BelongsToFunction" for each Identifier
    #
    #
    def UpdateIdentifierBelongsToFunction_disabled(self):
        EdkLogger.verbose("Update 'BelongsToFunction' for Identifiers started ...")

        SqlCommand = """select ID, BelongsToFile, StartLine, EndLine, Model from Identifier"""
        EdkLogger.debug(4, "SqlCommand: %s" %SqlCommand)
        self.Cur.execute(SqlCommand)
        Records = self.Cur.fetchall()
        for Record in Records:
            IdentifierID = Record[0]
            BelongsToFile = Record[1]
            StartLine = Record[2]
            EndLine = Record[3]
            Model = Record[4]

            #
            # Check whether an identifier belongs to a function
            #
            EdkLogger.debug(4, "For common identifiers ... ")
            SqlCommand = """select ID from Function
                        where StartLine < %s and EndLine > %s
                        and BelongsToFile = %s""" % (StartLine, EndLine, BelongsToFile)
            EdkLogger.debug(4, "SqlCommand: %s" %SqlCommand)
            self.Cur.execute(SqlCommand)
            IDs = self.Cur.fetchall()
            for ID in IDs:
                SqlCommand = """Update Identifier set BelongsToFunction = %s where ID = %s""" % (ID[0], IdentifierID)
                EdkLogger.debug(4, "SqlCommand: %s" %SqlCommand)
                self.Cur.execute(SqlCommand)

            #
            # Check whether the identifier is a function header
            #
            EdkLogger.debug(4, "For function headers ... ")
            if Model == DataClass.MODEL_IDENTIFIER_COMMENT:
                SqlCommand = """select ID from Function
                        where StartLine = %s + 1
                        and BelongsToFile = %s""" % (EndLine, BelongsToFile)
                EdkLogger.debug(4, "SqlCommand: %s" %SqlCommand)
                self.Cur.execute(SqlCommand)
                IDs = self.Cur.fetchall()
                for ID in IDs:
                    SqlCommand = """Update Identifier set BelongsToFunction = %s, Model = %s where ID = %s""" % (ID[0], DataClass.MODEL_IDENTIFIER_FUNCTION_HEADER, IdentifierID)
                    EdkLogger.debug(4, "SqlCommand: %s" %SqlCommand)
                    self.Cur.execute(SqlCommand)

        EdkLogger.verbose("Update 'BelongsToFunction' for Identifiers ... DONE")


    ## UpdateIdentifierBelongsToFunction
    #
    # Update the field "BelongsToFunction" for each Identifier
    #
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
            #Data1.append(("'file%s'" % BelongsToFile, FunctionID, BelongsToFile, StartLine, EndLine))
            #Data2.append(("'file%s'" % BelongsToFile, FunctionID, DataClass.MODEL_IDENTIFIER_FUNCTION_HEADER, BelongsToFile, DataClass.MODEL_IDENTIFIER_COMMENT, StartLine - 1))

            SqlCommand = """Update Identifier%s set BelongsToFunction = %s where BelongsToFile = %s and StartLine > %s and EndLine < %s""" % \
                        (BelongsToFile, FunctionID, BelongsToFile, StartLine, EndLine)
            self.TblIdentifier.Exec(SqlCommand)

            SqlCommand = """Update Identifier%s set BelongsToFunction = %s, Model = %s where BelongsToFile = %s and Model = %s and EndLine = %s""" % \
                         (BelongsToFile, FunctionID, DataClass.MODEL_IDENTIFIER_FUNCTION_HEADER, BelongsToFile, DataClass.MODEL_IDENTIFIER_COMMENT, StartLine - 1)
            self.TblIdentifier.Exec(SqlCommand)
#       #
#       # Check whether an identifier belongs to a function
#       #
#       print Data1
#       SqlCommand = """Update ? set BelongsToFunction = ? where BelongsToFile = ? and StartLine > ? and EndLine < ?"""
#       print SqlCommand
#       EdkLogger.debug(4, "SqlCommand: %s" %SqlCommand)
#       self.Cur.executemany(SqlCommand, Data1)
#
#       #
#       # Check whether the identifier is a function header
#       #
#       EdkLogger.debug(4, "For function headers ... ")
#       SqlCommand = """Update ? set BelongsToFunction = ?, Model = ? where BelongsToFile = ? and Model = ? and EndLine = ?"""
#       EdkLogger.debug(4, "SqlCommand: %s" %SqlCommand)
#       self.Cur.executemany(SqlCommand, Data2)
#
#       EdkLogger.verbose("Update 'BelongsToFunction' for Identifiers ... DONE")


##
#
# This acts like the main() function for the script, unless it is 'import'ed into another
# script.
#
if __name__ == '__main__':
    EdkLogger.Initialize()
    #EdkLogger.SetLevel(EdkLogger.VERBOSE)
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
    Db.UpdateIdentifierBelongsToFunction()

    Db.QueryTable(Db.TblFile)
    Db.QueryTable(Db.TblFunction)
    Db.QueryTable(Db.TblPcd)
    Db.QueryTable(Db.TblIdentifier)

    Db.Close()
    EdkLogger.verbose("End at " + time.strftime('%H:%M:%S', time.localtime()))

