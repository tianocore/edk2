/** @file

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
package org.tianocore.build.tools;

import org.apache.tools.ant.BuildException;
/**
  Ant element for module. 
  
  @since GenBuild 1.0
**/
public class ModuleItem {

    private String moduleName = null;
    
    private String moduleGuid = null;
    
    private String moduleVersion = null;
    
    private String packageName = null;
    
    private String packageGuid = null;
    
    private String packageVersion = null;

    public ModuleItem(){
        
    }
    
    public void execute() throws BuildException {
        
    }
    
    /**
      Get module Guid. 
      @return Module Guid
    **/
    public String getModuleGuid() {
        return moduleGuid;
    }

    /**
      Set module Guid
      @param moduleGuid Module Guid
    **/
    public void setModuleGuid(String moduleGuid) {
        this.moduleGuid = moduleGuid;
    }

    /**
      Get Module Name. 
      @return Module Name
    **/
    public String getModuleName() {
        return moduleName;
    }

    /**
      Set Module Name.
      @param moduleName Module Name
    **/
    public void setModuleName(String moduleName) {
        this.moduleName = moduleName;
    }

    /**
      Get Module Version. 
      @return Module Version
    **/
    public String getModuleVersion() {
        return moduleVersion;
    }

    /**
      Set Module Version. 
      @param moduleVersion Module version
    **/
    public void setModuleVersion(String moduleVersion) {
        this.moduleVersion = moduleVersion;
    }

    /**
      Get Package Guid. 
      @return Package Guid
    **/
    public String getPackageGuid() {
        return packageGuid;
    }

    /**
      Set Package Guid. 
      @param packageGuid Package Guid
    **/
    public void setPackageGuid(String packageGuid) {
        this.packageGuid = packageGuid;
    }

    /**
      Get Package Name. 
      @return Package Name
    **/
    public String getPackageName() {
        return packageName;
    }

    /**
      Set Package Name. 
      @param packageName Package Name
    **/
    public void setPackageName(String packageName) {
        this.packageName = packageName;
    }

    /**
      Get Package Version. 
      @return Package Version
    **/
    public String getPackageVersion() {
        return packageVersion;
    }

    /**
      Set Package Version. 
      @param packageVersion Package Version
    **/
    public void setPackageVersion(String packageVersion) {
        this.packageVersion = packageVersion;
    }
    
    
}
