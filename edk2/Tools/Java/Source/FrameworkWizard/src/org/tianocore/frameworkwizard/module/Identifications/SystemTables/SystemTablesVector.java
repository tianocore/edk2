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
package org.tianocore.frameworkwizard.module.Identifications.SystemTables;

import java.util.Vector;

public class SystemTablesVector {

    private Vector<SystemTablesIdentification> vSystemTables = new Vector<SystemTablesIdentification>();

    public int findSystemTables(SystemTablesIdentification sfi) {
        for (int index = 0; index < vSystemTables.size(); index++) {
            if (vSystemTables.elementAt(index).equals(sfi)) {
                return index;
            }
        }
        return -1;
    }

    public int findSystemTables(String name) {
        for (int index = 0; index < vSystemTables.size(); index++) {
            if (vSystemTables.elementAt(index).getName().equals(name)) {
                return index;
            }
        }
        return -1;
    }

    public SystemTablesIdentification getSystemTables(int index) {
        if (index > -1) {
            return vSystemTables.elementAt(index);
        } else {
            return null;
        }
    }

    public void addSystemTables(SystemTablesIdentification arg0) {
        vSystemTables.addElement(arg0);
    }

    public void setSystemTables(SystemTablesIdentification arg0, int arg1) {
        vSystemTables.setElementAt(arg0, arg1);
    }

    public void removeSystemTables(SystemTablesIdentification arg0) {
        int index = findSystemTables(arg0);
        if (index > -1) {
            vSystemTables.removeElementAt(index);
        }
    }

    public void removeSystemTables(int index) {
        if (index > -1 && index < this.size()) {
            vSystemTables.removeElementAt(index);
        }
    }

    public Vector<SystemTablesIdentification> getvSystemTables() {
        return vSystemTables;
    }

    public void setvSystemTables(Vector<SystemTablesIdentification> SystemTables) {
        vSystemTables = SystemTables;
    }

    public Vector<String> getSystemTablesName() {
        Vector<String> v = new Vector<String>();
        for (int index = 0; index < this.vSystemTables.size(); index++) {
            v.addElement(vSystemTables.get(index).getName());
        }
        return v;
    }

    public int size() {
        return this.vSystemTables.size();
    }
    
    public Vector<String> toStringVector(int index) {
        Vector<String> v = new Vector<String>();
        v.addElement(getSystemTables(index).getName());
        v.addElement(getSystemTables(index).getUsage());
        return v;
    }
}
