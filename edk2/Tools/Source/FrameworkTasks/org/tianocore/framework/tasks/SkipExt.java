/** @file
This file is to define nested element which is meant for specifying skipped file list

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
 SkipExt nested element Class
 class member
     -name : extension name of skiped file 
     -file : name of file including ext
 **/
public class SkipExt implements NestElement {
    private String name = "";
    private File file;
    private List<String> nameList = new ArrayList<String>();
    
    /**
     get class member "name"
     @returns name parameter
     **/
    public String getName() {
        return this.name;
    }
    /**
     set class member "name"
     @param     name    extension name of skiped file  
     **/
    public void setName(String name) {
        this.name = " -skipext " + name;
    }

    public String toString() {
        return getName();
    }

    /**
     get class member "file"
     @returns file parameter
     **/
    public File getFile() {
        return this.file;
    }
    /**
     set class member "file"
     @param name of file including ext  
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

