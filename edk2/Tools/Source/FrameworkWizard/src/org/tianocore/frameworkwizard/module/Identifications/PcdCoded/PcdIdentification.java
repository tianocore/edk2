/** @file

 The file is used to define Pcd Item Identification

 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php

 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 **/

package org.tianocore.frameworkwizard.module.Identifications.PcdCoded;

import java.util.Vector;

public class PcdIdentification {
    //
    // Define class members
    //
    private String name = null;

    private String guidCName = null;

    private String help = null;
    
    private Vector<String> type = null;
    
    public PcdIdentification(String arg0, String arg1, String arg2, Vector<String> arg3) {
        this.name = (arg0 == null ? "" : arg0);
        this.guidCName = (arg1 == null ? "" : arg1);
        this.help = (arg2 == null ? "" : arg2);
        this.type = arg3;
    }

    public String getHelp() {
        return help;
    }

    public void setHelp(String help) {
        this.help = help;
    }

    public String getGuidCName() {
        return guidCName;
    }

    public void setGuidCName(String guidCName) {
        this.guidCName = guidCName;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public Vector<String> getType() {
        return type;
    }

    public void setType(Vector<String> type) {
        this.type = type;
    }
    
    public String toString() {
        return name;
    }
}
