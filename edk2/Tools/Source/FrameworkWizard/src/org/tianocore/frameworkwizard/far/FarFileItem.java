/** @file
 
 The file is used to save information of <FarFile> item.
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/
package org.tianocore.frameworkwizard.far;

public class FarFileItem {

    private String relativeFilename;

    private String md5Value;

    private String defaultPath;

    public FarFileItem(String relativeFilename, String md5Value) {
        this.relativeFilename = relativeFilename;
        this.md5Value = md5Value;
    }

    public String getMd5Value() {
        return md5Value;
    }

    public String getRelativeFilename() {
        return relativeFilename;
    }

    public void setMd5Value(String md5Value) {
        this.md5Value = md5Value;
    }

    public void setRelativeFilename(String relativeFilename) {
        this.relativeFilename = relativeFilename;
    }

    public String getDefaultPath() {
        return defaultPath;
    }

    public void setDefaultPath(String defaultPath) {
        this.defaultPath = defaultPath;
    }

}
