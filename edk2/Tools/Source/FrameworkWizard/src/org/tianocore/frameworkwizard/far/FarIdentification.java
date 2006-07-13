/** @file
 
 The file is used to save information of Far item.
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/
package org.tianocore.frameworkwizard.far;

public class FarIdentification {

    private String guid;

    private String md5Value;

    private String path;

    public FarIdentification(String guid, String md5Value, String path) {
        this.guid = guid;
        this.md5Value = md5Value;
        this.path = path;
    }

    public String getGuid() {
        return guid;
    }

    public void setGuid(String guid) {
        this.guid = guid;
    }

    public String getMd5Value() {
        return md5Value;
    }

    public void setMd5Value(String md5Value) {
        this.md5Value = md5Value;
    }

    public String getPath() {
        return path;
    }

    public void setPath(String path) {
        this.path = path;
    }

    public String toString() {
        return path + " [" + guid + "]";
    }
}
