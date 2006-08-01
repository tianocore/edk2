/** @file
  UsageIdentification class.

  This class an identification for a PCD UsageInstance.
 
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php
 
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/  

package org.tianocore.pcd.entity;

/**
   The identification for a UsageInstance. 
   It should be extend from ModuleIdentification in future.

**/
public class UsageIdentification {
    ///
    /// The module CName: one key of Identification
    /// 
    public String moduleName;

    /// 
    /// The module Guid String: one key of Identification
    /// 
    public String moduleGuid;

    /// 
    /// The package CName: one key of Identification 
    ///
    public String packageName;

    /// 
    /// The package Guid: one key of Identification
    /// 
    public String packageGuid;

    /// 
    /// Module's Arch: one key of Identification
    /// 
    public String arch;

    /// 
    /// Module's version: one key of Identification
    /// 
    public String version;

    ///
    /// Module's type
    /// 
    public String moduleType;

    /**
      Constructor function for UsageIdentification class.

      @param moduleName     The key of module's name
      @param moduleGuid     The key of module's GUID string
      @param packageName    The key of package's name
      @param packageGuid    The key of package's Guid
      @param arch           The architecture string
      @param version        The version String
      @param moduleType     The module type
    **/
    public UsageIdentification (String moduleName,
                                String moduleGuid,
                                String packageName,
                                String packageGuid,
                                String arch,
                                String version,
                                String moduleType) {
        this.moduleName     = moduleName;
        this.moduleGuid     = moduleGuid;
        this.packageName    = packageName;
        this.packageGuid    = packageGuid;
        this.arch           = arch;
        this.version        = version;
        this.moduleType     = moduleType;
    }

    /**
       Generate the string for UsageIdentification

       @return the string value for UsageIdentification
    **/
    public String toString() {
        //
        // Because currently transition schema not require write moduleGuid, package Name, Packge GUID in
        // <ModuleSA> section, So currently no expect all paramter must be valid.
        // BUGBUG: Because currently we can not get version from MSA, So ignore verison.
        // 
        return(moduleName                                                                + "_" +
               ((moduleGuid  != null) ? moduleGuid.toLowerCase()    : "NullModuleGuid")  + "_" +
               ((packageName != null) ? packageName                 : "NullPackageName") + "_" +
               ((packageGuid != null) ? packageGuid.toLowerCase()   : "NullPackageGuid") + "_" +
               ((arch        != null) ? arch                        : "NullArch")        + "_" +
               "NullVersion");
    }
}
