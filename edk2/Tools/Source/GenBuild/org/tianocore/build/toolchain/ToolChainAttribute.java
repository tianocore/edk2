/** @file
ToolChainAttribute class

This file is to define enumeration value for tool chain attribute names.

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

package org.tianocore.build.toolchain;

/**

 ToolChainAttribute is used to define the enumeration value for the attributes
 used in tool chain definition file. 

 **/
public class ToolChainAttribute {
    private static int nextValue = 0;

    ///
    /// "NAME", "PATH", "DPATH", "SPATH", "EXT", "FAMILY", "FLAGS"
    /// 
    public final static ToolChainAttribute NAME = new ToolChainAttribute("NAME");
    public final static ToolChainAttribute PATH = new ToolChainAttribute("PATH");
    public final static ToolChainAttribute DPATH = new ToolChainAttribute("DPATH");
    public final static ToolChainAttribute SPATH = new ToolChainAttribute("SPATH");
    public final static ToolChainAttribute EXT = new ToolChainAttribute("EXT");
    public final static ToolChainAttribute FAMILY = new ToolChainAttribute("FAMILY");
    public final static ToolChainAttribute FLAGS = new ToolChainAttribute("FLAGS");

    private final String name;
    public final int value = nextValue++;

    /**
     * Default constructor
     */
    private ToolChainAttribute(String name) {
        this.name = name;
    }

    public String toString() {
        return name;
    }
}

