/** @file
 
 The file is used to save <farPlatform> information.
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/
package org.tianocore.frameworkwizard.far;

import org.tianocore.frameworkwizard.platform.PlatformIdentification;

public class FarPlatformItem {
    //
    // class member
    //
    private FarFileItem farFile;

    private String guidValue;

    private String version;

    public FarFileItem getFarFile() {
        return farFile;
    }

    public void setFarFile(FarFileItem farFile) {
        this.farFile = farFile;
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

    public boolean isIdentityPlf(PlatformIdentification plfId) {
        if (plfId.getGuid() == guidValue && plfId.getVersion() == version) {
            return true;
        }
        return false;

    }
}
