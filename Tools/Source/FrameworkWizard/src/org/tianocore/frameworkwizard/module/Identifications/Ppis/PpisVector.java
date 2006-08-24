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
package org.tianocore.frameworkwizard.module.Identifications.Ppis;

import java.util.Vector;

public class PpisVector {

    private Vector<PpisIdentification> vPpis = new Vector<PpisIdentification>();

    public int findPpis(PpisIdentification sfi) {
        for (int index = 0; index < vPpis.size(); index++) {
            if (vPpis.elementAt(index).equals(sfi)) {
                return index;
            }
        }
        return -1;
    }

    public int findPpis(String name) {
        for (int index = 0; index < vPpis.size(); index++) {
            if (vPpis.elementAt(index).getName().equals(name)) {
                return index;
            }
        }
        return -1;
    }

    public PpisIdentification getPpis(int index) {
        if (index > -1) {
            return vPpis.elementAt(index);
        } else {
            return null;
        }
    }

    public void addPpis(PpisIdentification arg0) {
        vPpis.addElement(arg0);
    }

    public void setPpis(PpisIdentification arg0, int arg1) {
        vPpis.setElementAt(arg0, arg1);
    }

    public void removePpis(PpisIdentification arg0) {
        int index = findPpis(arg0);
        if (index > -1) {
            vPpis.removeElementAt(index);
        }
    }

    public void removePpis(int index) {
        if (index > -1 && index < this.size()) {
            vPpis.removeElementAt(index);
        }
    }

    public Vector<PpisIdentification> getvPpis() {
        return vPpis;
    }

    public void setvPpis(Vector<PpisIdentification> Ppis) {
        vPpis = Ppis;
    }

    public Vector<String> getPpisName() {
        Vector<String> v = new Vector<String>();
        for (int index = 0; index < this.vPpis.size(); index++) {
            v.addElement(vPpis.get(index).getName());
        }
        return v;
    }

    public int size() {
        return this.vPpis.size();
    }
    
    public Vector<String> toStringVector(int index) {
        Vector<String> v = new Vector<String>();
        v.addElement(getPpis(index).getName());
        v.addElement(getPpis(index).getType());
        v.addElement(getPpis(index).getUsage());
        return v;
    }
}
