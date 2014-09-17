## @file
# This file is used to create a database used by ECC tool
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
import Common.LongFilePathOs as os

import EdkLogger as EdkLogger
from CommonDataClass.DataClass import *
from String import *
from DataType import *

from Table.TableDataModel import TableDataModel
from Table.TableFile import TableFile
from Table.TableInf import TableInf
from Table.TableDec import TableDec
from Table.TableDsc import TableDsc

## Database
#
# This class defined the build databse
# During the phase of initialization, the database will create all tables and
# insert all records of table DataModel
# 
# @param object:      Inherited from object class
# @param DbPath:      A string for the path of the ECC database
#
# @var Conn:          Connection of the ECC database
# @var Cur:           Cursor of the connection
# @var TblDataModel:  Local instance for TableDataModel
#
class Database(object):
    def __init__(self, DbPath):
        if os.path.exists(DbPath):
            os.remove(DbPath)
        self.Conn = sqlite3.connect(DbPath, isolation_level = 'DEFERRED')
        self.Conn.execute("PRAGMA page_size=8192")
        self.Conn.execute("PRAGMA synchronous=OFF")
        self.Cur = self.Conn.cursor()
        self.TblDataModel = TableDataModel(self.Cur)
        self.TblFile = TableFile(self.Cur)
        self.TblInf = TableInf(self.Cur)
        self.TblDec = TableDec(self.Cur)
        self.TblDsc = TableDsc(self.Cur)
    
    ## Initialize build database
    #
    # 1. Delete all old existing tables
    # 2. Create new tables
    # 3. Initialize table DataModel
    #
    def InitDatabase(self):
        EdkLogger.verbose("\nInitialize ECC database started ...")
        #
        # Drop all old existing tables
        #
#        self.TblDataModel.Drop()
#        self.TblDsc.Drop()
#        self.TblFile.Drop()
        
        #
        # Create new tables
        #
        self.TblDataModel.Create()
        self.TblFile.Create()
        self.TblInf.Create()
        self.TblDec.Create()
        self.TblDsc.Create()
        
        #
        # Initialize table DataModel
        #
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
        self.Conn.commit()
        self.Cur.close()
        self.Conn.close()

##
#
# This acts like the main() function for the script, unless it is 'import'ed into another
# script.
#
if __name__ == '__main__':
    EdkLogger.Initialize()
    EdkLogger.SetLevel(EdkLogger.DEBUG_0)
    
    Db = Database(DATABASE_PATH)
    Db.InitDatabase()
    Db.QueryTable(Db.TblDataModel)   
    Db.QueryTable(Db.TblFile)
    Db.QueryTable(Db.TblDsc)
    Db.Close()
    