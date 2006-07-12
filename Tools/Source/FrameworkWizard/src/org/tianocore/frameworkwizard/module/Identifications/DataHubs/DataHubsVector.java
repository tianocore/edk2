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
package org.tianocore.frameworkwizard.module.Identifications.DataHubs;

import java.util.Vector;

public class DataHubsVector {

    private Vector<DataHubsIdentification> vDataHubs = new Vector<DataHubsIdentification>();

    public int findDataHubs(DataHubsIdentification sfi) {
        for (int index = 0; index < vDataHubs.size(); index++) {
            if (vDataHubs.elementAt(index).equals(sfi)) {
                return index;
            }
        }
        return -1;
    }

    public int findDataHubs(String name) {
        for (int index = 0; index < vDataHubs.size(); index++) {
            if (vDataHubs.elementAt(index).getName().equals(name)) {
                return index;
            }
        }
        return -1;
    }

    public DataHubsIdentification getDataHubs(int index) {
        if (index > -1) {
            return vDataHubs.elementAt(index);
        } else {
            return null;
        }
    }

    public void addDataHubs(DataHubsIdentification arg0) {
        vDataHubs.addElement(arg0);
    }

    public void setDataHubs(DataHubsIdentification arg0, int arg1) {
        vDataHubs.setElementAt(arg0, arg1);
    }

    public void removeDataHubs(DataHubsIdentification arg0) {
        int index = findDataHubs(arg0);
        if (index > -1) {
            vDataHubs.removeElementAt(index);
        }
    }

    public void removeDataHubs(int index) {
        if (index > -1 && index < this.size()) {
            vDataHubs.removeElementAt(index);
        }
    }

    public Vector<DataHubsIdentification> getvDataHubs() {
        return vDataHubs;
    }

    public void setvDataHubs(Vector<DataHubsIdentification> DataHubs) {
        vDataHubs = DataHubs;
    }

    public Vector<String> getDataHubsName() {
        Vector<String> v = new Vector<String>();
        for (int index = 0; index < this.vDataHubs.size(); index++) {
            v.addElement(vDataHubs.get(index).getName());
        }
        return v;
    }

    public int size() {
        return this.vDataHubs.size();
    }
    
    public Vector<String> toStringVector(int index) {
        Vector<String> v = new Vector<String>();
        v.addElement(getDataHubs(index).getName());
        v.addElement(getDataHubs(index).getUsage());
        return v;
    }
}
