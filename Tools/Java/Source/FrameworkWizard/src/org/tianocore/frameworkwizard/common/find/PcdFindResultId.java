/** @file

 The file is used to define Pcd Identification used by find function

 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php

 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 **/
package org.tianocore.frameworkwizard.common.find;

import java.util.Vector;

import org.tianocore.frameworkwizard.packaging.PackageIdentification;

public class PcdFindResultId extends FindResultId {

    private String tokenSpaceGuidCName = null;
    
    private String token = null;
    
    private String datumType = null;
    
    private String value = null;
    
    private String usage = null;
    
    public PcdFindResultId(String strName, String strType, Vector<String> vArch, String strHelp, Vector<String> vModuleType, PackageIdentification pDeclaredBy) {
        super(strName, strType, vArch, strHelp, vModuleType, pDeclaredBy);
    }

    public String getDatumType() {
        return datumType;
    }

    public void setDatumType(String datumType) {
        this.datumType = datumType;
    }

    public String getToken() {
        return token;
    }

    public void setToken(String token) {
        this.token = token;
    }

    public String getTokenSpaceGuidCName() {
        return tokenSpaceGuidCName;
    }

    public void setTokenSpaceGuidCName(String tokenSpaceGuidCName) {
        this.tokenSpaceGuidCName = tokenSpaceGuidCName;
    }

    public String getUsage() {
        return usage;
    }

    public void setUsage(String usage) {
        this.usage = usage;
    }

    public String getValue() {
        return value;
    }

    public void setValue(String value) {
        this.value = value;
    }
}
