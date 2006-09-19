/** @file
This file is used to nest elements which is meant for tool's argument

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
 ToolArg class is defined to represent the argument of a tool. The argument 
 includes the prefix (e.g. -I, -o) and the value.
 **/
public class ToolArg extends NestElement {
    ///
    /// A constant which is used to represent an empty argument
    /// 
    public final static ToolArg EMPTY_ARG = new ToolArg();

    //
    // Keep track the prefix of this argument
    // 
    private String prefix = "";

    /**
       Default constructor
     **/
    public ToolArg() {
    }

    /**
       Constructor which will initialize the prefix of this argument

       @param prefix    The string of prefix
     */
    public ToolArg(String prefix) {
        this.prefix = prefix;
    }

    /**
       Constructor which will initialize both the prefix and value of this argument
       
       @param prefix    The prefix of this argument
       @param value     The value of this argument
     */
    public ToolArg(String prefix, String value) {
        setArg(prefix, value);
    }

    /**
       Set the prefix and value of this argument

       @param prefix    The prefix of this argument
       @param value     The value of this argument 
     */
    public void setArg(String prefix, String value) {
        this.prefix = prefix;
        super.setName(value);
    }

    /**
       Set the prefix of this argument

       @param prefix    The prefix of this argument
     */
    public void setPrefix(String prefix) {
        this.prefix = prefix;
    }

    /**
       Get the prefix of this argument

       @return String   The prefix of this argument
     */
    public String getPrefix() {
        return this.prefix.trim();
    }

    /**
       Set the value of this argument

       @param value     The value of this argument
     */
    public void setValue(String value) {
        super.setName(value);
    }

    /**
       Add a value for this argument

       @param value     The value of this argument
     */
    public void insValue(String value) {
        super.insName(value);
    }

    /**
       Get the value list of this argument, separated by space

       @return String   The value list
     */
    public String getValue() {
        return super.toString(" ").trim();
    }

    /**
       Set the argument as a whole

       @param line      The argument string line
     */
    public void setLine(String line) {
        //
        // Since the prefix is in the "line", we don't need another prefix.
        // 
        this.prefix = " ";
        super.setName(line);
    }

    /**
       Get the argument line

       @return String   The argument string line
     */
    public String getLine() {
        return this.toString();
    }

    /**
       Compose a complete argument string.

       @return String   The complete argument
     */
    public String toString() {
        return super.toString(prefix);
    }

    /**
       Check if the argument is empty or not

       @return boolean
     **/
    public boolean isEmpty() {
        return (prefix.length() == 0) && (nameList.isEmpty());
    }
}
