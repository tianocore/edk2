/** @file

 The file is used to define Build Options Identification

 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php

 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 **/

package org.tianocore.frameworkwizard.module.Identifications.BuildOptions;

import java.util.Vector;

public class BuildOptionsIdentification {

    //
    // Define class members
    //
    private String option = null;

    private Vector<String> buildTargets = null;
    
    private String toolChainFamily = null;

    private String tagName = null;
    
    private String toolCode = null;
    
    private Vector<String> supArchList = null;

    public BuildOptionsIdentification(String arg0, Vector<String> arg1, String arg2, String arg3, String arg4, Vector<String> arg5) {
        this.option = (arg0 == null ? "" : arg0);
        this.buildTargets = arg1;
        this.toolChainFamily = (arg2 == null ? "" : arg2);
        this.tagName = (arg3 == null ? "" : arg3);
        this.toolCode = (arg4 == null ? "" : arg4);
        this.supArchList = arg5;
    }

    public String getOption() {
        return option;
    }

    public void setOption(String option) {
        this.option = option;
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

    public Vector<String> getBuildTargets() {
        return buildTargets;
    }

    public void setBuildTargets(Vector<String> buildTargets) {
        this.buildTargets = buildTargets;
    }
}
