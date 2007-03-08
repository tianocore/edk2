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

package org.tianocore.frameworkwizard.module.Identifications.Events;

import java.util.Vector;

import org.tianocore.frameworkwizard.common.DataType;

public class EventsIdentification {

    //
    // Define class members
    //
    private String name = null;

    private String type = null;

    private String usage = null;

    private Vector<String> supArchList = null;

    private String featureFlag = null;
    
    private String help = null;
    
    private String group = null;

    public EventsIdentification(String arg0, String arg1, String arg2, String arg3, Vector<String> arg4, String arg5, String arg6) {
        this.name = (arg0 == null ? "" : arg0);
        this.name = (this.name == DataType.EMPTY_SELECT_ITEM ? "" : this.name);
        this.type = (arg1 == null ? "" : arg1);
        this.usage = (arg2 == null ? "" : arg2);
        this.featureFlag = (arg3 == null ? "" : arg3);
        this.supArchList = arg4;
        this.help = (arg5 == null ? "" : arg5);
        this.group = (arg6 == null ? "" : arg6);
    }

    public String getFeatureFlag() {
        return featureFlag;
    }

    public void setFeatureFlag(String featureFlag) {
        this.featureFlag = featureFlag;
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
    
    public boolean equals(EventsIdentification pi) {
        if (this.name.equals(pi.name)) {
            return true;
        }
        return false;
    }

    public String getType() {
        return type;
    }

    public void setType(String type) {
        this.type = type;
    }

    public String getUsage() {
        return usage;
    }

    public void setUsage(String usage) {
        this.usage = usage;
    }

    public String getHelp() {
        return help;
    }

    public void setHelp(String help) {
        this.help = help;
    }

    public String getGroup() {
        return group;
    }

    public void setGroup(String group) {
        this.group = group;
    }
}
