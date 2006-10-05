/** @file
 
 The file is used to define Pcd Vector
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/
package org.tianocore.frameworkwizard.module.Identifications.PcdCoded;

import java.util.Vector;

public class PcdVector {

    private Vector<PcdIdentification> vPcd = new Vector<PcdIdentification>();

    public int findPcd(PcdIdentification sfi) {
        for (int index = 0; index < vPcd.size(); index++) {
            if (vPcd.elementAt(index).equals(sfi)) {
                return index;
            }
        }
        return -1;
    }

    public int findPcd(String name) {
        for (int index = 0; index < vPcd.size(); index++) {
            if (vPcd.elementAt(index).getName().equals(name)) {
                return index;
            }
        }
        return -1;
    }

    public PcdIdentification getPcd(int index) {
        if (index > -1) {
            return vPcd.elementAt(index);
        } else {
            return null;
        }
    }

    public void addPcd(PcdIdentification arg0) {
        vPcd.addElement(arg0);
    }

    public void setPcd(PcdIdentification arg0, int arg1) {
        vPcd.setElementAt(arg0, arg1);
    }

    public void removePcd(PcdIdentification arg0) {
        int index = findPcd(arg0);
        if (index > -1) {
            vPcd.removeElementAt(index);
        }
    }

    public void removePcd(int index) {
        if (index > -1 && index < this.size()) {
            vPcd.removeElementAt(index);
        }
    }

    public Vector<PcdIdentification> getvPcd() {
        return vPcd;
    }

    public void setvPcd(Vector<PcdIdentification> Pcd) {
        vPcd = Pcd;
    }

    public Vector<String> getPcdName() {
        Vector<String> v = new Vector<String>();
        for (int index = 0; index < this.vPcd.size(); index++) {
            v.addElement(vPcd.get(index).getName());
        }
        return v;
    }

    public int size() {
        return this.vPcd.size();
    }

    public void addAll(PcdVector v) {
        if (v != null) {
            for (int index = 0; index < v.size(); index++) {
                vPcd.add(v.getPcd(index));
            }
        }
    }
}
