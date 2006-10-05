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
package org.tianocore.frameworkwizard.module.Identifications.Hobs;

import java.util.Vector;

public class HobsVector {

    private Vector<HobsIdentification> vHobs = new Vector<HobsIdentification>();

    public int findHobs(HobsIdentification sfi) {
        for (int index = 0; index < vHobs.size(); index++) {
            if (vHobs.elementAt(index).equals(sfi)) {
                return index;
            }
        }
        return -1;
    }

    public int findHobs(String name) {
        for (int index = 0; index < vHobs.size(); index++) {
            if (vHobs.elementAt(index).getName().equals(name)) {
                return index;
            }
        }
        return -1;
    }

    public HobsIdentification getHobs(int index) {
        if (index > -1) {
            return vHobs.elementAt(index);
        } else {
            return null;
        }
    }

    public void addHobs(HobsIdentification arg0) {
        vHobs.addElement(arg0);
    }

    public void setHobs(HobsIdentification arg0, int arg1) {
        vHobs.setElementAt(arg0, arg1);
    }

    public void removeHobs(HobsIdentification arg0) {
        int index = findHobs(arg0);
        if (index > -1) {
            vHobs.removeElementAt(index);
        }
    }

    public void removeHobs(int index) {
        if (index > -1 && index < this.size()) {
            vHobs.removeElementAt(index);
        }
    }

    public Vector<HobsIdentification> getvHobs() {
        return vHobs;
    }

    public void setvHobs(Vector<HobsIdentification> Hobs) {
        vHobs = Hobs;
    }

    public Vector<String> getHobsName() {
        Vector<String> v = new Vector<String>();
        for (int index = 0; index < this.vHobs.size(); index++) {
            v.addElement(vHobs.get(index).getName());
        }
        return v;
    }

    public int size() {
        return this.vHobs.size();
    }
    
    public Vector<String> toStringVector(int index) {
        Vector<String> v = new Vector<String>();
        v.addElement(getHobs(index).getName());
        v.addElement(getHobs(index).getType());
        v.addElement(getHobs(index).getUsage());
        return v;
    }
}
