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

package org.tianocore.frameworkwizard.module.Identifications.Protocols;

import java.util.Vector;

import org.tianocore.frameworkwizard.module.Identifications.ModuleIdentification;
import org.tianocore.frameworkwizard.packaging.PackageIdentification;

public class ProtocolsIdentification {

    //
    // Define class members
    //
    private String name = null;

    private String type = null;

    private String usage = null;

    private Vector<String> supArchList = null;

    private String featureFlag = null;
    
    private String help = null;
    
    private ModuleIdentification belongModule = null;
    
    private PackageIdentification declaredBy = null;

    public ProtocolsIdentification(String arg0, String arg1, String arg2, String arg3, Vector<String> arg4, String arg5) {
        this.name = (arg0 == null ? "" : arg0);
        this.type = (arg1 == null ? "" : arg1);
        this.usage = (arg2 == null ? "" : arg2);
        this.featureFlag = (arg3 == null ? "" : arg3);
        this.supArchList = arg4;
        this.help = (arg5 == null ? "" : arg5);
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
    
    public boolean equals(ProtocolsIdentification pi) {
        if (this.name.equals(pi.name)) {
            return true;
        }
        return false;
    }

    public String getType() {
        return type;
    }

    public void setType(String type) {
        this.type = type;
    }

    public String getUsage() {
        return usage;
    }

    public void setUsage(String usage) {
        this.usage = usage;
    }

    public String getHelp() {
        return help;
    }

    public void setHelp(String help) {
        this.help = help;
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
}
