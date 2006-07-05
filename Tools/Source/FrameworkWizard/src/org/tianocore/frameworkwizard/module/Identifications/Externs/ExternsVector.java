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
package org.tianocore.frameworkwizard.module.Identifications.Externs;

import java.util.Vector;

public class ExternsVector {

    private Vector<ExternsIdentification> vExterns = new Vector<ExternsIdentification>();

    public int findExterns(ExternsIdentification sfi) {
        for (int index = 0; index < vExterns.size(); index++) {
            if (vExterns.elementAt(index).equals(sfi)) {
                return index;
            }
        }
        return -1;
    }

    public int findExterns(String name) {
        for (int index = 0; index < vExterns.size(); index++) {
            if (vExterns.elementAt(index).getName().equals(name)) {
                return index;
            }
        }
        return -1;
    }

    public ExternsIdentification getExterns(int index) {
        if (index > -1) {
            return vExterns.elementAt(index);
        } else {
            return null;
        }
    }

    public void addExterns(ExternsIdentification arg0) {
        vExterns.addElement(arg0);
    }

    public void updateExterns(ExternsIdentification arg0, int arg1) {
        vExterns.setElementAt(arg0, arg1);
    }

    public void removeExterns(ExternsIdentification arg0) {
        int index = findExterns(arg0);
        if (index > -1) {
            vExterns.removeElementAt(index);
        }
    }

    public void removeExterns(int index) {
        if (index > -1 && index < this.size()) {
            vExterns.removeElementAt(index);
        }
    }

    public Vector<ExternsIdentification> getvExterns() {
        return vExterns;
    }

    public void setvExterns(Vector<ExternsIdentification> Externs) {
        vExterns = Externs;
    }

    public Vector<String> getExternsName() {
        Vector<String> v = new Vector<String>();
        for (int index = 0; index < this.vExterns.size(); index++) {
            v.addElement(vExterns.get(index).getName());
        }
        return v;
    }

    public int size() {
        return this.vExterns.size();
    }

}
