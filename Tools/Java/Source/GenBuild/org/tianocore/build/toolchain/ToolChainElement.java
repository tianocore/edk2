/** @file
ToolChainElement class

ToolChainElement class is defining enumeration value of key part names.

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

 This class is an enumeration definition for key elements in tool chain definition
 file.
 
 **/
public class ToolChainElement {
    private static int nextValue = 0;

    //
    // "TARGET", "TOOLCHAIN", "ARCH", "TOOLCODE", "ATTRIBUTE"
    // 
    public final static ToolChainElement TARGET = new ToolChainElement("TARGET");
    public final static ToolChainElement TOOLCHAIN = new ToolChainElement("TOOLCHAIN");
    public final static ToolChainElement ARCH = new ToolChainElement("ARCH");
    public final static ToolChainElement TOOLCODE = new ToolChainElement("TOOLCODE");
    public final static ToolChainElement ATTRIBUTE = new ToolChainElement("ATTRIBUTE");

    private final String name;
    public final int value = nextValue++;

    /**
     * Default constructor
     */
    private ToolChainElement(String name) {
        this.name = name;
    }

    public String toString() {
        return name;
    }
}





