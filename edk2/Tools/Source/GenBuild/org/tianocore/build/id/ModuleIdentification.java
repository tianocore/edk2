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
/**
  This class is used to identify a module with Module Guid, Module Version, 
  Package Guid, Package Version. 

  @since GenBuild 1.0
**/
public class ModuleIdentification extends Identification {
    
    private PackageIdentification packageId;
    
    private File msaFile;
    
    private String moduleType;
    
    private boolean isLibrary = false;

    /**
      @param guid Guid
      @param version Version
    **/
    public ModuleIdentification(String guid, String version){
        super(guid, version);
    }
    
    /**
      @param guid Guid
      @param version Version
      @param packageId Package Identification
    **/
    public ModuleIdentification(String guid, String version, PackageIdentification packageId){
        super(guid, version);
        this.packageId = packageId;
    }
    
    /**
      @param name Name
      @param guid Guid
      @param version Version
    **/
    public ModuleIdentification(String name, String guid, String version){
        super(name, guid, version);
    }
    
    /**
      @param name Name
      @param guid Guid
      @param version Version
      @param packageId PackageIdentification
    **/
    public ModuleIdentification(String name, String guid, String version, PackageIdentification packageId){
        super(name, guid, version);
        this.packageId = packageId;
    }
    
    /**
      @return boolean is this module is library
    **/
    public boolean isLibrary() {
        return isLibrary;
    }

    /**
      @param isLibrary 
    **/
    public void setLibrary(boolean isLibrary) {
        this.isLibrary = isLibrary;
    }

    /**
      @return MSA File
    **/
    public File getMsaFile() {
        prepareMsaFile();
        return msaFile;
    }
    
    /**
      @return Module relative path to package
    **/
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

    /**
      @param msaFile Set Msa File
    **/
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

    /**
      @param packageId set package identification
    **/
    public void setPackage(PackageIdentification packageId) {
        this.packageId = packageId;
    }

    /**
      @return get package identification
    **/
    public PackageIdentification getPackage() {
        return packageId;
    }

    /**
      @return get module type
    **/
    public String getModuleType() {
        if (moduleType == null) {
            GlobalData.refreshModuleIdentification(this);
        }
        return moduleType;
    }

    /**
      @param moduleType set module type
    **/
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
