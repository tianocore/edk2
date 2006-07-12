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
package org.tianocore.frameworkwizard.module.Identifications.Protocols;

import java.util.Vector;

public class ProtocolsVector {

    private Vector<ProtocolsIdentification> vProtocols = new Vector<ProtocolsIdentification>();

    public int findProtocols(ProtocolsIdentification sfi) {
        for (int index = 0; index < vProtocols.size(); index++) {
            if (vProtocols.elementAt(index).equals(sfi)) {
                return index;
            }
        }
        return -1;
    }

    public int findProtocols(String name) {
        for (int index = 0; index < vProtocols.size(); index++) {
            if (vProtocols.elementAt(index).getName().equals(name)) {
                return index;
            }
        }
        return -1;
    }

    public ProtocolsIdentification getProtocols(int index) {
        if (index > -1) {
            return vProtocols.elementAt(index);
        } else {
            return null;
        }
    }

    public void addProtocols(ProtocolsIdentification arg0) {
        vProtocols.addElement(arg0);
    }

    public void setProtocols(ProtocolsIdentification arg0, int arg1) {
        vProtocols.setElementAt(arg0, arg1);
    }

    public void removeProtocols(ProtocolsIdentification arg0) {
        int index = findProtocols(arg0);
        if (index > -1) {
            vProtocols.removeElementAt(index);
        }
    }

    public void removeProtocols(int index) {
        if (index > -1 && index < this.size()) {
            vProtocols.removeElementAt(index);
        }
    }

    public Vector<ProtocolsIdentification> getvProtocols() {
        return vProtocols;
    }

    public void setvProtocols(Vector<ProtocolsIdentification> Protocols) {
        vProtocols = Protocols;
    }

    public Vector<String> getProtocolsName() {
        Vector<String> v = new Vector<String>();
        for (int index = 0; index < this.vProtocols.size(); index++) {
            v.addElement(vProtocols.get(index).getName());
        }
        return v;
    }

    public int size() {
        return this.vProtocols.size();
    }
    
    public Vector<String> toStringVector(int index) {
        Vector<String> v = new Vector<String>();
        v.addElement(getProtocols(index).getName());
        v.addElement(getProtocols(index).getType());
        v.addElement(getProtocols(index).getUsage());
        return v;
    }
}
