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

    private boolean isBinary = false;

    private String constructor = "";

    private String destructor = "";

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
      @return boolean is this module is binary
    **/
    public boolean isBinary() {
        return isBinary;
    }

    /**
      @param isBinary
    **/
    public void setBinary(boolean isBinary) {
        this.isBinary = isBinary;
    }

    /**
      @return MSA File
    **/
    public File getMsaFile() {
        return msaFile;
    }
    
    /**
      @return Module relative path to package
    **/
    public String getModuleRelativePath() {
        if (msaFile.getParent().length() == packageId.getPackageDir().length()) {
            return ".";
        }
        return msaFile.getParent().substring(packageId.getPackageDir().length() + 1);
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
    
    public String toString() {
        String nameString;
        String versionString;
        String packageString;

        if (name != null && name != "") {
            nameString = name;
        } else {
            if (guid != null && guid != "") {
                nameString = guid;
            } else {
                nameString = "UNKNOWN";
            }
        }

        if (version != null) {
            versionString = version;
        } else {
            versionString = ""; 
        }

        if (packageId != null) {
            packageString = packageId.toString();
        } else {
            packageString = "Package [UNKNOWN]";
        }

        return "Module [" + nameString + versionString + "] in " + packageString; 
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
        return moduleType;
    }

    /**
      @param moduleType set module type
    **/
    public void setModuleType(String moduleType) {
        this.moduleType = moduleType;
    }

    /**
       @return String The module name
     **/
    public String getName() {
        return name;
    }

    /**
       @return boolean
     **/
    public boolean hasConstructor() {
        return constructor != "";
    }

    /**
       @return boolean
     */
    public boolean hasDestructor() {
        return destructor != "";
    }

    /**
       Set the constructor function name if this module is a library

       @param name
     */
    public void setConstructor(String name) {
        if (name != null) {
            constructor = name;
        }
    }

    /**
       Set the destructor function name if this module is a library

       @param name
     */
    public void setDestructor(String name) {
        if (name != null) {
            destructor = name;
        }
    }
}
