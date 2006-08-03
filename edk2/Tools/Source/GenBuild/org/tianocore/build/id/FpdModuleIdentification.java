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
package org.tianocore.build.id;


/**
  This class is used to identify a module with Module Guid, Module Version, 
  Package Guid, Package Version and Arch. 
  
  @since GenBuild 1.0
**/
public class FpdModuleIdentification {
    
    private String arch;
    
    private String fvBinding = "NULL"; // Optional
    
    private ModuleIdentification module;
    
    /**
      Constructor Method. 
      
      @param arch Build Arch
      @param fvBinding Belong to what FVs
      @param module ModuleIdentification
    **/
    public FpdModuleIdentification(String arch, String fvBinding, ModuleIdentification module){
        this.arch = arch;
        this.fvBinding = fvBinding;
        this.module = module;
    }
    
    /**
      Constructor Method. 
    
      @param arch Build Arch
      @param module ModuleIdentification
    **/
    public FpdModuleIdentification(ModuleIdentification module, String arch){
        this.arch = arch;
        this.module = module;
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
            if ( module.equals(moduleIdObj.module) && arch.equalsIgnoreCase(moduleIdObj.arch)) {
                return true;
            }
            return false;
        }
        else {
            return false;
        }
    }

    /**
      @param fvBinding
    **/
    public void setFvBinding(String fvBinding) {
        this.fvBinding = fvBinding;
    }

    /* (non-Javadoc)
      @see java.lang.Object#toString()
    **/
    public String toString(){
        return arch + ":" + module;
    }

    /**
      @return String fvBinding
    **/
    public String getFvBinding() {
        return fvBinding;
    }

    /**
      @return ModuleIdentification module ID
    **/
    public ModuleIdentification getModule() {
        return module;
    }

    /**
      @param module Module Identification
    **/
    public void setModule(ModuleIdentification module) {
        this.module = module;
    }

    /**
      @return String arch
    **/
    public String getArch() {
        return arch;
    }

    /**
      @param arch build ARCH
    **/
    public void setArch(String arch) {
        this.arch = arch;
    }
    
    /* (non-Javadoc)
      @see java.lang.Object#hashCode()
    **/
    public int hashCode(){
        return module.hashCode();
    }
}
