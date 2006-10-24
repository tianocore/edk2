/** @file

 The file is used to define Package Dependencies Identification

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

import org.tianocore.frameworkwizard.module.Identifications.ModuleIdentification;
import org.tianocore.frameworkwizard.packaging.PackageIdentification;

public class PcdCodedIdentification {

    //
    // Define class members
    //
    private String name = null;

    private String guid = null;
    
    private String featureFlag = null;

    private Vector<String> supArchList = null;

    private String value = null;
    
    private String help = null;
    
    private String type = null;
    
    private String usage = null;
    
    private String tokenSpaceGuidCName = null;
    
    private String token = null;
    
    private String datumType = null;
    
    private ModuleIdentification belongModule = null;
    
    private PackageIdentification declaredBy = null;

    public PcdCodedIdentification(String arg0, String arg1, String arg2, Vector<String> arg3, String arg4, String arg5, String arg6, String arg7) {
        this.name = (arg0 == null ? "" : arg0);
        this.guid = (arg1 == null ? "" : arg1);
        this.featureFlag = (arg2 == null ? "" : arg2);
        this.supArchList = arg3;
        this.value = (arg4 == null ? "" : arg4);
        this.help = (arg5 == null ? "" : arg5);
        this.type = (arg6 == null ? "" : arg6);
        this.usage = (arg7 == null ? "" : arg7);
    }

    public String getFeatureFlag() {
        return featureFlag;
    }

    public void setFeatureFlag(String featureFlag) {
        this.featureFlag = featureFlag;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public Vector<String> getSupArchList() {
        return supArchList;
    }

    public void setSupArchList(Vector<String> supArchList) {
        this.supArchList = supArchList;
    }
    
    public boolean equals(PcdCodedIdentification pi) {
        if (this.name.equals(pi.name)) {
            return true;
        }
        return false;
    }

    public String getGuid() {
        return guid;
    }

    public void setGuid(String guid) {
        this.guid = guid;
    }

    public String getHelp() {
        return help;
    }

    public void setHelp(String help) {
        this.help = help;
    }

    public String getType() {
        return type;
    }

    public void setType(String type) {
        this.type = type;
    }

    public String getValue() {
        return value;
    }

    public void setValue(String value) {
        this.value = value;
    }

    public String getUsage() {
        return usage;
    }

    public void setUsage(String usage) {
        this.usage = usage;
    }
    
    public ModuleIdentification getBelongModule() {
        return belongModule;
    }

    public void setBelongModule(ModuleIdentification belongModule) {
        this.belongModule = belongModule;
    }

    public PackageIdentification getDeclaredBy() {
        return declaredBy;
    }

    public void setDeclaredBy(PackageIdentification declaredBy) {
        this.declaredBy = declaredBy;
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

    public String getDatumType() {
        return datumType;
    }

    public void setDatumType(String datumType) {
        this.datumType = datumType;
    }
}
