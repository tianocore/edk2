/** @file

 The file is used to define Package Dependencies Identification

 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php

 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 **/

package org.tianocore.frameworkwizard.module.Identifications.PackageDependencies;

import java.util.Vector;

public class PackageDependenciesIdentification {

    //
    // Define class members
    //
    private String name = null;

    private String version = null;

    private String guid = null;

    private Vector<String> supArchList = null;

    private String featureFlag = null;

    public PackageDependenciesIdentification(String arg0, String arg1, String arg2, String arg3, Vector<String> arg4) {
        this.name = (arg0 == null ? "" : arg0);
        this.version = (arg1 == null ? "" : arg1);
        this.guid = (arg2 == null ? "" : arg2);
        this.featureFlag = (arg3 == null ? "" : arg3);
        this.supArchList = arg4;
    }

    public String getFeatureFlag() {
        return featureFlag;
    }

    public void setFeatureFlag(String featureFlag) {
        this.featureFlag = featureFlag;
    }

    public String getGuid() {
        return guid;
    }

    public void setGuid(String guid) {
        this.guid = guid;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public Vector<String> getSupArchList() {
        return supArchList;
    }

    public void setSupArchList(Vector<String> supArchList) {
        this.supArchList = supArchList;
    }

    public String getVersion() {
        return version;
    }

    public void setVersion(String version) {
        this.version = version;
    }
    
    public boolean equals(PackageDependenciesIdentification pdi) {
        if (this.guid.equals(pdi.guid)) {
            return true;
        }
        return false;
    }
}
