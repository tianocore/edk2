/** @file
 
 The file is used to define Package Dependencies Vector
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/
package org.tianocore.frameworkwizard.module.Identifications.Guids;

import java.util.Vector;

public class GuidsVector {

    private Vector<GuidsIdentification> vGuids = new Vector<GuidsIdentification>();

    public int findGuids(GuidsIdentification sfi) {
        for (int index = 0; index < vGuids.size(); index++) {
            if (vGuids.elementAt(index).equals(sfi)) {
                return index;
            }
        }
        return -1;
    }

    public int findGuids(String name) {
        for (int index = 0; index < vGuids.size(); index++) {
            if (vGuids.elementAt(index).getName().equals(name)) {
                return index;
            }
        }
        return -1;
    }

    public GuidsIdentification getGuids(int index) {
        if (index > -1) {
            return vGuids.elementAt(index);
        } else {
            return null;
        }
    }

    public void addGuids(GuidsIdentification arg0) {
        vGuids.addElement(arg0);
    }

    public void setGuids(GuidsIdentification arg0, int arg1) {
        vGuids.setElementAt(arg0, arg1);
    }

    public void removeGuids(GuidsIdentification arg0) {
        int index = findGuids(arg0);
        if (index > -1) {
            vGuids.removeElementAt(index);
        }
    }

    public void removeGuids(int index) {
        if (index > -1 && index < this.size()) {
            vGuids.removeElementAt(index);
        }
    }

    public Vector<GuidsIdentification> getvGuids() {
        return vGuids;
    }

    public void setvGuids(Vector<GuidsIdentification> Guids) {
        vGuids = Guids;
    }

    public Vector<String> getGuidsName() {
        Vector<String> v = new Vector<String>();
        for (int index = 0; index < this.vGuids.size(); index++) {
            v.addElement(vGuids.get(index).getName());
        }
        return v;
    }

    public int size() {
        return this.vGuids.size();
    }
    
    public Vector<String> toStringVector(int index) {
        Vector<String> v = new Vector<String>();
        v.addElement(getGuids(index).getName());
        v.addElement(getGuids(index).getUsage());
        return v;
    }

}
