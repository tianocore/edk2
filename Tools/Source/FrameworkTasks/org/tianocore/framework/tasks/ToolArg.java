/** @file
This file is to define nested element which is meant for specifying tool arguments

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
 Class ToolArg is just used to specify arguments for genffsfile tool
 **/
public class ToolArg {
    ///
    /// keep the argument string
    ///
    private String line = "";

    /**
     Get method of ANT task/datatype for attribute "line"

     @returns   The argument string
     **/
    public String getLine() {
        return line;
    }

    /**
     Set method of ANT task/datatype for attribute "line"

     @param     line    The argument string
     **/
    public void setLine(String line) {
        this.line = line;
    }   
}