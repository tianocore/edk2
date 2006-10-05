/** @file

 The file is used to define Source Files Identification

 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php

 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 **/
package org.tianocore.frameworkwizard.module.Identifications.SourceFiles;

import java.util.Vector;

public class SourceFilesIdentification {

    private String filename = null;

    private String tagName = null;

    private String toolCode = null;

    private String toolChainFamily = null;

    private Vector<String> supArchList = null;

    private String featureFlag = null;

    public SourceFilesIdentification(String strFilename, String strTagName, String strToolCode,
                                     String strToolChainFamily, String strFeatureFlag, Vector<String> arch) {
        this.filename = (strFilename == null ? "" : strFilename);
        this.tagName = (strTagName == null ? "" : strTagName);
        this.toolCode = (strToolCode == null ? "" : strToolCode);
        this.toolChainFamily = (strToolChainFamily == null ? "" : strToolChainFamily);
        this.featureFlag = (strFeatureFlag == null ? "" : strFeatureFlag);
        this.supArchList = arch;
    }

    public String getFeatureFlag() {
        return featureFlag;
    }

    public void setFeatureFlag(String featureFlag) {
        this.featureFlag = featureFlag;
    }

    public String getFilename() {
        return filename;
    }

    public void setFilename(String filename) {
        this.filename = filename;
    }

    public Vector<String> getSupArchList() {
        return supArchList;
    }

    public void setSupArchList(Vector<String> supArchList) {
        this.supArchList = supArchList;
    }

    public String getTagName() {
        return tagName;
    }

    public void setTagName(String tagName) {
        this.tagName = tagName;
    }

    public String getToolChainFamily() {
        return toolChainFamily;
    }

    public void setToolChainFamily(String toolChainFamily) {
        this.toolChainFamily = toolChainFamily;
    }

    public String getToolCode() {
        return toolCode;
    }

    public void setToolCode(String toolCode) {
        this.toolCode = toolCode;
    }

    public boolean equals(SourceFilesIdentification sfid) {
        if (this.filename.equals(sfid.filename) && this.tagName.equals(sfid.tagName)
            && this.toolCode.equals(sfid.toolCode) && this.toolChainFamily.equals(sfid.toolChainFamily)
            && this.featureFlag.equals(sfid.featureFlag)
            && this.supArchList.toString().equals(sfid.supArchList.toString())) {
            return true;
        }
        return false;
    }
}
