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
package org.tianocore.frameworkwizard.module.Identifications.PackageDependencies;

import java.util.Vector;

public class PackageDependenciesVector {

    private Vector<PackageDependenciesIdentification> vPackageDependencies = new Vector<PackageDependenciesIdentification>();

    public int findPackageDependencies(PackageDependenciesIdentification sfi) {
        for (int index = 0; index < vPackageDependencies.size(); index++) {
            if (vPackageDependencies.elementAt(index).equals(sfi)) {
                return index;
            }
        }
        return -1;
    }

    public int findPackageDependencies(String name) {
        for (int index = 0; index < vPackageDependencies.size(); index++) {
            if (vPackageDependencies.elementAt(index).getName().equals(name)) {
                return index;
            }
        }
        return -1;
    }

    public PackageDependenciesIdentification getPackageDependencies(int index) {
        if (index > -1) {
            return vPackageDependencies.elementAt(index);
        } else {
            return null;
        }
    }

    public void addPackageDependencies(PackageDependenciesIdentification arg0) {
        vPackageDependencies.addElement(arg0);
    }

    public void setPackageDependencies(PackageDependenciesIdentification arg0, int arg1) {
        vPackageDependencies.setElementAt(arg0, arg1);
    }

    public void removePackageDependencies(PackageDependenciesIdentification arg0) {
        int index = findPackageDependencies(arg0);
        if (index > -1) {
            vPackageDependencies.removeElementAt(index);
        }
    }

    public void removePackageDependencies(int index) {
        if (index > -1 && index < this.size()) {
            vPackageDependencies.removeElementAt(index);
        }
    }

    public Vector<PackageDependenciesIdentification> getvPackageDependencies() {
        return vPackageDependencies;
    }

    public void setvPackageDependencies(Vector<PackageDependenciesIdentification> PackageDependencies) {
        vPackageDependencies = PackageDependencies;
    }

    public Vector<String> getPackageDependenciesName() {
        Vector<String> v = new Vector<String>();
        for (int index = 0; index < this.vPackageDependencies.size(); index++) {
            v.addElement(vPackageDependencies.get(index).getName());
        }
        return v;
    }

    public int size() {
        return this.vPackageDependencies.size();
    }
    
    public Vector<String> toStringVector(int index) {
        Vector<String> v = new Vector<String>();
        v.addElement(getPackageDependencies(index).getName());
        v.addElement(getPackageDependencies(index).getVersion());
        return v;
    }

}
