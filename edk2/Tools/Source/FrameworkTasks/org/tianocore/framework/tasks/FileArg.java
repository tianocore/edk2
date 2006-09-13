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
 FileArg class is defined to represent tool's argument which specifies file path.
 **/
public class FileArg extends ToolArg {
    /**
       Default constructor
     **/
    public FileArg() {
    }

    /**
       Constructor which accepts argument prefix and its value as parameters

       @param prefix    The prefix of argument
       @param value     The value of argument
     **/
    public FileArg(String prefix, String value) {
        super(prefix);
        this.setValue(value);
    }

    /**
       Set the prefix and value of an argument

       @param prefix    The prefix of argument
       @param value     The value of argument 
     **/
    public void setArg(String prefix, String value) {
        super.setPrefix(prefix);
        this.setValue(value);
    }

    /**
       Set the value of an argument

       @param value     The value of the argument
     **/
    public void setValue(String value) {
        super.setFile(value);
    }

    /**
       Add a value of an argument

       @param value     The value of the argument
     **/
    public void insValue(String value) {
        super.insFile(value);
    }
}
