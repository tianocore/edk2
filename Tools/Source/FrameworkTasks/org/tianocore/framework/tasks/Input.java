/** @file
This file is used to nest elements which is meant for file path

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
 Input class is defined to be as nested elements of other elements, to specify 
 the path of file(s)
 **/
public class Input {
    private String filePath;
    
    public Input() {
        
    }

    /**
     Standard set method of ANT task, for "file" attribute

     @param     path    The path of a file
     **/
    public void setFile(String path) {
        filePath = path;
    }

    /**
     Standard get method of ANT task, for "file" attribute

     @returns   The path of current specified file.
     **/
    public String getFile() {
        return filePath;
    }
}
