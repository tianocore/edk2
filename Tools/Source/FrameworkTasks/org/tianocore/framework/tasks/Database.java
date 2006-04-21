/** @file
 Database class.

 Database represents an exceplicity name list of database file. 
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 **/
package org.tianocore.framework.tasks;

import java.io.File;
import java.util.ArrayList;
import java.util.List;
import java.util.StringTokenizer;

/**
  Database
  
  Database represents an exceplicity name list of database file. 
 
**/
public class Database implements NestElement{
    ///
    /// name of database file
    ///
    private String name = "";
    ///
    /// name of file including database files
    /// 
    private File file;
    ///
    /// the database file name list
    ///
    private List<String> nameList = new ArrayList<String>();
    
    /**
      getName
      
      This function is to get class member "name".
      
      @return             class member "name".
    **/
    public String getName() {
        return this.name;
    }
    /**
      setName
      
      This function is to set class member "name".
      
      @param name : name of database file.
    **/
    public void setName(String name) {
        this.name = " -db " + name;
    }

    /**
      toString
      
      This function is to call getName() function.
      @return       class member "name".  
    **/
    public String toString() {
        return getName();
    }
    
    /**
      getFile
      
      This function is to get file which include the database file list.
      
      @return      class member "file"
      
    **/
    public File getFile() {
        return this.file;
    }
    /**
      setFile
      
      This function is to set class member "file".
      
      @param file  The file which include the database file list. 
    **/
    public void setFile(File file) {
        this.file = file;
    }
 

    public void setList(String fileNameList) {
        if (fileNameList != null && fileNameList.length() > 0) {
            StringTokenizer tokens = new StringTokenizer(fileNameList, " \t,;", false);
            while (tokens.hasMoreTokens()) {
                String fileName = tokens.nextToken().trim();
                if (fileName.length() > 0) {
                    this.nameList.add(fileName);
                }
            }
        }
    }

    public List<String> getList() {
        return nameList;
    }
}