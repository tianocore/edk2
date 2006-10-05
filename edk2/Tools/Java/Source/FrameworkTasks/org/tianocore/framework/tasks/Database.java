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

/**
  Database represents an exceplicity name list of database file. 
**/
public class Database extends NestElement {
    /**
      Override NestElement.toString() to return a string with leading "-db"
      
      @return String 
    **/
    public String toString() {
        return super.toString(" -db ");
    }    
}