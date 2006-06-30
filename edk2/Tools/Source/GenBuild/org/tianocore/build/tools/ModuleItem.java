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

public class ModuleItem {

    private String moduleName;
    
    private String moduleGuid;
    
    private String moduleVersion;
    
    private String packageName;
    
    private String packageGuid;
    
    private String packageVersion;

    public ModuleItem(){
        
    }
    
    public void execute() throws BuildException {
        
    }
    
    public String getModuleGuid() {
        return moduleGuid;
    }

    public void setModuleGuid(String moduleGuid) {
        this.moduleGuid = moduleGuid;
    }

    public String getModuleName() {
        return moduleName;
    }

    public void setModuleName(String moduleName) {
        this.moduleName = moduleName;
    }

    public String getModuleVersion() {
        return moduleVersion;
    }

    public void setModuleVersion(String moduleVersion) {
        this.moduleVersion = moduleVersion;
    }

    public String getPackageGuid() {
        return packageGuid;
    }

    public void setPackageGuid(String packageGuid) {
        this.packageGuid = packageGuid;
    }

    public String getPackageName() {
        return packageName;
    }

    public void setPackageName(String packageName) {
        this.packageName = packageName;
    }

    public String getPackageVersion() {
        return packageVersion;
    }

    public void setPackageVersion(String packageVersion) {
        this.packageVersion = packageVersion;
    }
    
    
}
