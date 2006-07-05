/** @file

 The file is used to define Tool Chain Configuration Identification

 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php

 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 **/
package org.tianocore.frameworkwizard.common.Identifications;

public class ToolChainConfigId {

    public static final String COMMENTS = "#";
    
    public static final String EQUALS = "=";
    
    private String name = "";
    
    private String value = "";
    
    public ToolChainConfigId(String strName, String strValue) {
        this.name = strName;
        this.value = strValue;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public String getValue() {
        return value;
    }

    public void setValue(String value) {
        this.value = value;
    }
    
    public boolean equals(ToolChainConfigId id) {
        if (this.name.equals(id.name)) {
            return true;
        } else {
            return false;
        }
    }
}
