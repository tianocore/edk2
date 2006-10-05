/** @file
 
 The file is used to save <farPackage> information.
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/
package org.tianocore.frameworkwizard.far;

import java.util.List;

import org.tianocore.frameworkwizard.packaging.PackageIdentification;

public class FarPackage {
    //
    // Class member
    //
    private FarFileItem farFile;

    private String guidValue;

    private String version;

    private String defaultPath;

    private List<FarPlatformItem> farPlatformList;

    private List<FarFileItem> contentList;

    public String getDefaultPath() {
        return defaultPath;
    }

    public void setDefaultPath(String defaultPath) {
        this.defaultPath = defaultPath;
    }

    public FarFileItem getFarFile() {
        return farFile;
    }

    public void setFarFile(FarFileItem farFile) {
        this.farFile = farFile;
    }

    public List<FarPlatformItem> getFarPlatformList() {
        return farPlatformList;
    }

    public void setFarPlatformList(List<FarPlatformItem> farPlatformList) {
        this.farPlatformList = farPlatformList;
    }

    public String getGuidValue() {
        return guidValue;
    }

    public void setGuidValue(String guidValue) {
        this.guidValue = guidValue;
    }

    public String getVersion() {
        return version;
    }

    public void setVersion(String version) {
        this.version = version;
    }

    public List<FarFileItem> getContentList() {
        return this.contentList;
    }

    public void setContentList(List<FarFileItem> contentList) {
        this.contentList = contentList;
    }

    public boolean isIdentityPkg(PackageIdentification pkgId) {
        //File file = new File(farFile.getRelativeFilename());
        if (pkgId.getGuid().equalsIgnoreCase(guidValue) && pkgId.getVersion().equalsIgnoreCase(version)) {
            return true;
        }
        return false;

    }
}
