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
package org.tianocore.frameworkwizard.platform.ui.id;


/**
  This class is used to identify a module with BaseName, GUID, Version, PackageName
  and ARCH.
  
  @since GenBuild 1.0
 **/
public class FpdModuleIdentification {
    
    private String arch;
    
    private String fvBinding = "NULL"; // Optional
    
    private String sequence = "0"; // Optional
    
    private ModuleIdentification module;
    
    private String target; // Optional
    
    private String toolchain; // Optional
    
    public FpdModuleIdentification(String arch, String fvBinding, String sequence, ModuleIdentification module){
        this.arch = arch;
        this.fvBinding = fvBinding;
        this.sequence = sequence;
        this.module = module;
    }
    
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
            return super.equals(obj);
        }
    }

    public void setFvBinding(String fvBinding) {
        this.fvBinding = fvBinding;
    }

    public void setSequence(String sequence) {
        this.sequence = sequence;
    }

    public String toString(){
        return arch + ":" + module;
    }

    public String getFvBinding() {
        return fvBinding;
    }

    public String getSequence() {
        return sequence;
    }

    public ModuleIdentification getModule() {
        return module;
    }

    public void setModule(ModuleIdentification module) {
        this.module = module;
    }

    public String getArch() {
        return arch;
    }

    public void setArch(String arch) {
        this.arch = arch;
    }
    
    public int hashCode(){
        return module.hashCode();
    }
}
