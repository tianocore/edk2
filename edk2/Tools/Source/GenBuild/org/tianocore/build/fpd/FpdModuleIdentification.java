/** @file
  Java class FpdModuleIdentification is used to present a module identification
  from BaseName, GUID, Version, PackageName, and ARCH. 
 
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
package org.tianocore.build.fpd;

/**
  This class is used to identify a module with BaseName, GUID, Version, PackageName
  and ARCH.
  
  @since GenBuild 1.0
 **/
public class FpdModuleIdentification {
    
    private String arch;
    
    private String fvBinding;
    
    private String baseName;
    
    private String packageName;
    
    private String guid;
    
    private String version;
    
    private String sequence;
    
    /**
      
      @param baseName the base name of the module
      @param guid the GUID of the module
      @param arch the ARCH of the module
    **/
    public FpdModuleIdentification(String baseName, String guid, String arch){
        this.baseName = baseName;
        this.guid = guid;
        this.arch = arch;
    }
    
    /**
      Override java.lang.Object#equals. 
      
      <p>Currently, use BaseName and ARCH to identify a module. It will enhance
      in the next version. </p>
      
      @see java.lang.Object#equals(java.lang.Object)
    **/
    public boolean equals(Object obj) {
        if (obj instanceof FpdModuleIdentification) {
            FpdModuleIdentification moduleIdObj = (FpdModuleIdentification)obj;
            if ( baseName.equalsIgnoreCase(moduleIdObj.baseName) && arch.equalsIgnoreCase(moduleIdObj.arch)) {
                return true;
            }
            // TBD
            return false;
        }
        else {
            return super.equals(obj);
        }
    }
    
    public void setArch(String arch) {
        this.arch = arch;
    }

    public void setFvBinding(String fvBinding) {
        this.fvBinding = fvBinding;
    }

    public void setSequence(String sequence) {
        this.sequence = sequence;
    }

    public String toString(){
        return arch + ":" + guid + "_" + baseName;
    }

    public void setBaseName(String baseName) {
        this.baseName = baseName;
    }

    public void setGuid(String guid) {
        this.guid = guid;
    }

    public void setPackageName(String packageName) {
        this.packageName = packageName;
    }

    public void setVersion(String version) {
        this.version = version;
    }

    public String getArch() {
        return arch;
    }

    public String getBaseName() {
        return baseName;
    }

    public String getFvBinding() {
        return fvBinding;
    }

    public String getGuid() {
        return guid;
    }

    public String getPackageName() {
        return packageName;
    }

    public String getSequence() {
        return sequence;
    }

    public String getVersion() {
        return version;
    }
}
