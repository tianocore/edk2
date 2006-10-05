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

package org.tianocore.frameworkwizard.module.Identifications.Externs;

import java.util.Vector;

import org.tianocore.frameworkwizard.common.EnumerationData;

public class ExternsIdentification {

    //
    // Define class members
    //
    private String name0 = null;

    private String name1 = null;

    private String name2 = null;

    private String name3 = null;

    private String type = null;

    private Vector<String> supArchList = null;

    private String featureFlag = null;

    public ExternsIdentification(String arg0, String arg1) {
        this.name0 = (arg0 == null ? "" : arg0);
        this.type = (arg1 == null ? "" : arg1);
    }

    public ExternsIdentification(String arg0, String arg1, String arg2, String arg3, Vector<String> arg4) {
        this.name0 = (arg0 == null ? "" : arg0);
        this.name1 = (arg1 == null ? "" : arg1);
        this.type = (arg2 == null ? "" : arg2);
        this.featureFlag = (arg3 == null ? "" : arg3);
        this.supArchList = arg4;
    }

    public ExternsIdentification(String arg0, String arg1, String arg2, String arg3, String arg4, String arg5,
                                 Vector<String> arg6) {
        this.name0 = (arg0 == null ? "" : arg0);
        this.name1 = (arg1 == null ? "" : arg1);
        this.name2 = (arg2 == null ? "" : arg2);
        this.name3 = (arg3 == null ? "" : arg3);
        this.type = (arg4 == null ? "" : arg4);
        this.featureFlag = (arg5 == null ? "" : arg5);
        this.supArchList = arg6;
    }

    public String getFeatureFlag() {
        return featureFlag;
    }

    public void setFeatureFlag(String featureFlag) {
        this.featureFlag = featureFlag;
    }

    public Vector<String> getSupArchList() {
        return supArchList;
    }

    public void setSupArchList(Vector<String> supArchList) {
        this.supArchList = supArchList;
    }

    public boolean equals(ExternsIdentification pi) {
        if (this.type.equals(pi.type)) {
            if (this.type.equals(EnumerationData.EXTERNS_SPECIFICATION)) {
                if (this.name0.equals(pi.name0)) {
                    return true;
                }
            }
            if (this.type.equals(EnumerationData.EXTERNS_IMAGE)) {
                if (this.name0.equals(pi.name0) && this.name1.equals(pi.name1)) {
                    return true;
                }
            }
            if (this.type.equals(EnumerationData.EXTERNS_LIBRARY)) {
                if (this.name0.equals(pi.name0) && this.name1.equals(pi.name1)) {
                    return true;
                }
            }
            if (this.type.equals(EnumerationData.EXTERNS_CALL_BACK)) {
                if (this.name0.equals(pi.name0) && this.name1.equals(pi.name1)) {
                    return true;
                }
            }
            if (this.type.equals(EnumerationData.EXTERNS_DRIVER)) {
                if (this.name0.equals(pi.name0) && this.name1.equals(pi.name1) && this.name2.equals(pi.name2) && this.name3.equals(pi.name3)) {
                    return true;
                }
            }
        }

        return false;
    }

    public String getType() {
        return type;
    }

    public void setType(String type) {
        this.type = type;
    }

    public String getName0() {
        return name0;
    }

    public void setName0(String name0) {
        this.name0 = name0;
    }

    public String getName1() {
        return name1;
    }

    public void setName1(String name1) {
        this.name1 = name1;
    }

    public String getName2() {
        return name2;
    }

    public void setName2(String name2) {
        this.name2 = name2;
    }

    public String getName3() {
        return name3;
    }

    public void setName3(String name3) {
        this.name3 = name3;
    }
}
