/** @file
This file is to define  ModuleIdentification class.

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

package org.tianocore.build.id;

import java.io.File;

import org.tianocore.build.global.GlobalData;

public class ModuleIdentification extends Identification {
    
    private PackageIdentification packageId;
    
    private File msaFile;
    
    private String moduleType;
    
    private boolean isLibrary = false;

    public ModuleIdentification(String guid, String version){
        super(guid, version);
    }
    
    public ModuleIdentification(String guid, String version, PackageIdentification packageId){
        super(guid, version);
        this.packageId = packageId;
    }
    
    public ModuleIdentification(String name, String guid, String version){
        super(name, guid, version);
    }
    
    public ModuleIdentification(String name, String guid, String version, PackageIdentification packageId){
        super(name, guid, version);
        this.packageId = packageId;
    }
    
    public boolean isLibrary() {
        return isLibrary;
    }

    public void setLibrary(boolean isLibrary) {
        this.isLibrary = isLibrary;
    }

    public File getMsaFile() {
        prepareMsaFile();
        return msaFile;
    }
    
    public String getModuleRelativePath() {
        prepareMsaFile();
        if (msaFile.getParent().length() == packageId.getPackageDir().length()) {
            return ".";
        }
        return msaFile.getParent().substring(packageId.getPackageDir().length() + 1);
    }

    private void prepareMsaFile(){
        if (msaFile == null) {
            GlobalData.refreshModuleIdentification(this);
        }
    }

    public void setMsaFile(File msaFile) {
        this.msaFile = msaFile;
    }
   
    public boolean equals(Object obj) {
        if (obj instanceof ModuleIdentification) {
            ModuleIdentification id = (ModuleIdentification)obj;
            if (guid.equalsIgnoreCase(id.getGuid()) && packageId.equals(id.getPackage())) {
                if (version == null || id.version == null) {
                    return true;
                }
                else if (version.trim().equalsIgnoreCase("") || id.version.trim().equalsIgnoreCase("")){
                    return true;
                }
                else if (version.equalsIgnoreCase(id.version)) {
                    return true;
                }
            }
            return false;
        }
        else {
            return super.equals(obj);
        }
    }
    
    public String toString(){
        if (name == null) {
            GlobalData.refreshModuleIdentification(this);
        }
        if (version == null || version.trim().equalsIgnoreCase("")) {
            return "Module [" + name + "] in " + packageId;
        }
        else {
            return "Module [" + name + " " + version + "] in " + packageId; 
        }
    }

    public void setPackage(PackageIdentification packageId) {
        this.packageId = packageId;
    }

    public PackageIdentification getPackage() {
        return packageId;
    }

    public String getModuleType() {
        if (moduleType == null) {
            GlobalData.refreshModuleIdentification(this);
        }
        return moduleType;
    }

    public void setModuleType(String moduleType) {
        this.moduleType = moduleType;
    }
    
    public String getName() {
        if (name == null) {
            GlobalData.refreshModuleIdentification(this);
        }
        return name;
    }
}
