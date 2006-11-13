/** @file
 
 The file is used to define Boot Modes Vector
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/
package org.tianocore.frameworkwizard.module.Identifications.BootModes;

import java.util.Vector;

public class BootModesVector {

    private Vector<BootModesIdentification> vBootModes = new Vector<BootModesIdentification>();

    public int findBootModes(BootModesIdentification sfi) {
        for (int index = 0; index < vBootModes.size(); index++) {
            if (vBootModes.elementAt(index).equals(sfi)) {
                return index;
            }
        }
        return -1;
    }

    public int findBootModes(String name) {
        for (int index = 0; index < vBootModes.size(); index++) {
            if (vBootModes.elementAt(index).getName().equals(name)) {
                return index;
            }
        }
        return -1;
    }

    public BootModesIdentification getBootModes(int index) {
        if (index > -1) {
            return vBootModes.elementAt(index);
        } else {
            return null;
        }
    }

    public void addBootModes(BootModesIdentification arg0) {
        vBootModes.addElement(arg0);
    }

    public void setBootModes(BootModesIdentification arg0, int arg1) {
        vBootModes.setElementAt(arg0, arg1);
    }

    public void removeBootModes(BootModesIdentification arg0) {
        int index = findBootModes(arg0);
        if (index > -1) {
            vBootModes.removeElementAt(index);
        }
    }

    public void removeBootModes(int index) {
        if (index > -1 && index < this.size()) {
            vBootModes.removeElementAt(index);
        }
    }

    public Vector<BootModesIdentification> getvBootModes() {
        return vBootModes;
    }

    public void setvBootModes(Vector<BootModesIdentification> BootModes) {
        vBootModes = BootModes;
    }

    public Vector<String> getBootModesName() {
        Vector<String> v = new Vector<String>();
        for (int index = 0; index < this.vBootModes.size(); index++) {
            v.addElement(vBootModes.get(index).getName());
        }
        return v;
    }

    public int size() {
        return this.vBootModes.size();
    }
    
    public Vector<String> toStringVector(int index) {
        Vector<String> v = new Vector<String>();
        v.addElement(getBootModes(index).getName());
        v.addElement(getBootModes(index).getUsage());
        return v;
    }
}
