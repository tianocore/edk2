/** @file
This file is used to nest elements which is meant for include path name

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

public class IncludePath implements NestElement {
    /**
     IncludePath nested element Class
     class member
         -name : name of include path 
         -file : name of file including include path
     **/
    private String path = "";
    private File file;
    private List<String> nameList = new ArrayList<String>();
    
    /**
     get class member "file"

     @returns   The File object
     **/
    public File getFile() {
        return this.file;
    }

    /**
     set class member "file"

     @param     file    The name of include path
     **/
    public void setFile(File file) {
        this.file = file;
    }
    
    /**
     get class member "file"

     @returns   The name of include path
     **/
    public String getPath() {
        return this.path;
    }
    
    /**
     get class member "name"

     @returns   The name of include path
     **/
    public String getName() {
        return this.path;
    }
    
    /**
     set class member "name"

     @param     name    The name of include path
     **/
    public void setName(String name){
        this.path = "-I " + name;
    }

    /**
     set class member "path"

     @param     name    name of file including include paths 
     **/
    public void setPath(String name) {
        this.path = " -I " + name;
    }

    /**
     override Object.toString()

     @returns   name of file including include paths 
     **/
    public String toString() {
        return getPath();
    }
 
    /**
     set class member "list"

     @param     fileNameList    name list of include paths, sperated by space, tab,
                                comma or semi-comma
     **/
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

    /**
     get class member "list"

     @returns   The include paths list.
     **/
    public List<String> getList() {
        return nameList;
    }
}

