## @file
# This file is used to create/update/query/erase a common table
#
# Copyright (c) 2008, Intel Corporation. All rights reserved.<BR>
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
import Common.EdkLogger as EdkLogger

## TableFile
#
# This class defined a common table
# 
# @param object:     Inherited from object class
#
# @param Cursor:     Cursor of the database
# @param TableName:  Name of the table
#
class Table(object):
    def __init__(self, Cursor):
        self.Cur = Cursor
        self.Table = ''
        self.ID = 0
    
    ## Create table
    #
    # Create a table
    #
    def Create(self, SqlCommand):
        self.Cur.execute(SqlCommand)
        self.ID = 0
        EdkLogger.verbose(SqlCommand + " ... DONE!")

    ## Insert table
    #
    # Insert a record into a table
    #
    def Insert(self, SqlCommand):
        self.Exec(SqlCommand)
    
    ## Query table
    #
    # Query all records of the table
    #  
    def Query(self):
        EdkLogger.verbose("\nQuery tabel %s started ..." % self.Table)
        SqlCommand = """select * from %s""" % self.Table
        self.Cur.execute(SqlCommand)
        for Rs in self.Cur:
            EdkLogger.verbose(str(Rs))
        
        TotalCount = self.GetCount()
        EdkLogger.verbose("*** Total %s records in table %s ***" % (TotalCount, self.Table) )
        EdkLogger.verbose("Query tabel %s DONE!" % self.Table)

    ## Drop a table
    #
    # Drop the table
    #
    def Drop(self):
        SqlCommand = """drop table IF EXISTS %s""" % self.Table
        self.Cur.execute(SqlCommand)
        EdkLogger.verbose("Drop tabel %s ... DONE!" % self.Table)
    
    ## Get count
    #
    # Get a count of all records of the table
    #
    # @retval Count:  Total count of all records
    #
    def GetCount(self):
        SqlCommand = """select count(ID) from %s""" % self.Table
        self.Cur.execute(SqlCommand)
        for Item in self.Cur:
            return Item[0]
    
    ## Generate ID
    #
    # Generate an ID if input ID is -1
    #
    # @param ID:   Input ID 
    #
    # @retval ID:  New generated ID
    #
    def GenerateID(self, ID):
        if ID == -1:
            self.ID = self.ID + 1

        return self.ID
    
    ## Init the ID of the table
    #
    # Init the ID of the table
    #
    def InitID(self):
        self.ID = self.GetCount()
    
    ## Exec
    #
    # Exec Sql Command, return result
    #
    # @param SqlCommand:  The SqlCommand to be executed
    #
    # @retval RecordSet:  The result after executed
    #
    def Exec(self, SqlCommand):
        EdkLogger.debug(4, "SqlCommand: %s" % SqlCommand)
        self.Cur.execute(SqlCommand)
        RecordSet = self.Cur.fetchall()
        EdkLogger.debug(4, "RecordSet: %s" % RecordSet)
        return RecordSet
