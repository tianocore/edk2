/** @file

 The file is used to define GUID Identification used by find function

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

import org.tianocore.frameworkwizard.module.Identifications.ModuleIdentification;
import org.tianocore.frameworkwizard.packaging.PackageIdentification;

public class FindResultId {

    private String name = "";

    private String type = "";
    
    private Vector<String> arch = new Vector<String>();
    
    private Vector<String> moduleType = new Vector<String>();
    
    private String help = "";

    private Vector<ModuleIdentification> producedModules = new Vector<ModuleIdentification>();

    private Vector<ModuleIdentification> consumedModules = new Vector<ModuleIdentification>();
    
    private PackageIdentification declaredBy = null;
    
    public FindResultId(String strName, String strType, Vector<String> vArch, String strHelp, Vector<String> vModuleType, PackageIdentification pDeclaredBy) {
        this.name = (strName == null ? "" : strName);
        this.type = (strType == null ? "" : strType);
        this.arch = (vArch == null ? this.arch : vArch);
        this.help = (strHelp == null ? "" : strHelp);
        this.moduleType = (vModuleType == null ? this.moduleType : vModuleType);
        this.declaredBy = pDeclaredBy;
    }

    public Vector<String> getArch() {
        return arch;
    }

    public void setArch(Vector<String> arch) {
        this.arch = arch;
    }

    public Vector<ModuleIdentification> getConsumedModules() {
        return consumedModules;
    }

    public void setConsumedModules(Vector<ModuleIdentification> consumedModules) {
        this.consumedModules = consumedModules;
    }
    
    public void addConsumedModules(ModuleIdentification consumedModule) {
        if (consumedModule != null) {
            this.consumedModules.addElement(consumedModule);
        }
    }

    public PackageIdentification getDeclaredBy() {
        return declaredBy;
    }

    public void setDeclaredBy(PackageIdentification declaredBy) {
        this.declaredBy = declaredBy;
    }

    public String getHelp() {
        return help;
    }

    public void setHelp(String help) {
        this.help = help;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public Vector<ModuleIdentification> getProducedModules() {
        return producedModules;
    }

    public void setProducedModules(Vector<ModuleIdentification> producedModules) {
        this.producedModules = producedModules;
    }
    
    public void addProducedModules(ModuleIdentification producedModule) {
        if (producedModule != null) {
            this.producedModules.addElement(producedModule);
        }
    }

    public String getType() {
        return type;
    }

    public void setType(String type) {
        this.type = type;
    }

    public Vector<String> getModuleType() {
        return moduleType;
    }

    public void setModuleType(Vector<String> moduleType) {
        this.moduleType = moduleType;
    }
}
