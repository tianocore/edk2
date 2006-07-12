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
package org.tianocore.frameworkwizard.module.Identifications.Variables;

import java.util.Vector;

public class VariablesVector {

    private Vector<VariablesIdentification> vVariables = new Vector<VariablesIdentification>();

    public int findVariables(VariablesIdentification sfi) {
        for (int index = 0; index < vVariables.size(); index++) {
            if (vVariables.elementAt(index).equals(sfi)) {
                return index;
            }
        }
        return -1;
    }

    public int findVariables(String name) {
        for (int index = 0; index < vVariables.size(); index++) {
            if (vVariables.elementAt(index).getName().equals(name)) {
                return index;
            }
        }
        return -1;
    }

    public VariablesIdentification getVariables(int index) {
        if (index > -1) {
            return vVariables.elementAt(index);
        } else {
            return null;
        }
    }

    public void addVariables(VariablesIdentification arg0) {
        vVariables.addElement(arg0);
    }

    public void setVariables(VariablesIdentification arg0, int arg1) {
        vVariables.setElementAt(arg0, arg1);
    }

    public void removeVariables(VariablesIdentification arg0) {
        int index = findVariables(arg0);
        if (index > -1) {
            vVariables.removeElementAt(index);
        }
    }

    public void removeVariables(int index) {
        if (index > -1 && index < this.size()) {
            vVariables.removeElementAt(index);
        }
    }

    public Vector<VariablesIdentification> getvVariables() {
        return vVariables;
    }

    public void setvVariables(Vector<VariablesIdentification> Variables) {
        vVariables = Variables;
    }

    public Vector<String> getVariablesName() {
        Vector<String> v = new Vector<String>();
        for (int index = 0; index < this.vVariables.size(); index++) {
            v.addElement(vVariables.get(index).getName());
        }
        return v;
    }

    public int size() {
        return this.vVariables.size();
    }
    
    public Vector<String> toStringVector(int index) {
        Vector<String> v = new Vector<String>();
        v.addElement(getVariables(index).getName());
        v.addElement(getVariables(index).getGuid());
        v.addElement(getVariables(index).getUsage());
        return v;
    }
}
