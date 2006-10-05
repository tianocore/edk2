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
package org.tianocore.frameworkwizard.module.Identifications.HiiPackages;

import java.util.Vector;

public class HiiPackagesVector {

    private Vector<HiiPackagesIdentification> vHiiPackages = new Vector<HiiPackagesIdentification>();

    public int findHiiPackages(HiiPackagesIdentification sfi) {
        for (int index = 0; index < vHiiPackages.size(); index++) {
            if (vHiiPackages.elementAt(index).equals(sfi)) {
                return index;
            }
        }
        return -1;
    }

    public int findHiiPackages(String name) {
        for (int index = 0; index < vHiiPackages.size(); index++) {
            if (vHiiPackages.elementAt(index).getName().equals(name)) {
                return index;
            }
        }
        return -1;
    }

    public HiiPackagesIdentification getHiiPackages(int index) {
        if (index > -1) {
            return vHiiPackages.elementAt(index);
        } else {
            return null;
        }
    }

    public void addHiiPackages(HiiPackagesIdentification arg0) {
        vHiiPackages.addElement(arg0);
    }

    public void setHiiPackages(HiiPackagesIdentification arg0, int arg1) {
        vHiiPackages.setElementAt(arg0, arg1);
    }

    public void removeHiiPackages(HiiPackagesIdentification arg0) {
        int index = findHiiPackages(arg0);
        if (index > -1) {
            vHiiPackages.removeElementAt(index);
        }
    }

    public void removeHiiPackages(int index) {
        if (index > -1 && index < this.size()) {
            vHiiPackages.removeElementAt(index);
        }
    }

    public Vector<HiiPackagesIdentification> getvHiiPackages() {
        return vHiiPackages;
    }

    public void setvHiiPackages(Vector<HiiPackagesIdentification> HiiPackages) {
        vHiiPackages = HiiPackages;
    }

    public Vector<String> getHiiPackagesName() {
        Vector<String> v = new Vector<String>();
        for (int index = 0; index < this.vHiiPackages.size(); index++) {
            v.addElement(vHiiPackages.get(index).getName());
        }
        return v;
    }

    public int size() {
        return this.vHiiPackages.size();
    }
    
    public Vector<String> toStringVector(int index) {
        Vector<String> v = new Vector<String>();
        v.addElement(getHiiPackages(index).getName());
        v.addElement(getHiiPackages(index).getUsage());
        return v;
    }
}
