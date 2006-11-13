/** @file
 
 The file is used to define Build Options Vector
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/
package org.tianocore.frameworkwizard.module.Identifications.BuildOptions;

import java.util.Vector;

public class BuildOptionsVector {

    private Vector<BuildOptionsIdentification> vBuildOptions = new Vector<BuildOptionsIdentification>();

    public int findBuildOptions(BuildOptionsIdentification sfi) {
        for (int index = 0; index < vBuildOptions.size(); index++) {
            if (vBuildOptions.elementAt(index).equals(sfi)) {
                return index;
            }
        }
        return -1;
    }

    public int findBuildOptions(String name) {
        for (int index = 0; index < vBuildOptions.size(); index++) {
            if (vBuildOptions.elementAt(index).getOption().equals(name)) {
                return index;
            }
        }
        return -1;
    }

    public BuildOptionsIdentification getBuildOptions(int index) {
        if (index > -1) {
            return vBuildOptions.elementAt(index);
        } else {
            return null;
        }
    }

    public void addBuildOptions(BuildOptionsIdentification arg0) {
        vBuildOptions.addElement(arg0);
    }

    public void setBuildOptions(BuildOptionsIdentification arg0, int arg1) {
        vBuildOptions.setElementAt(arg0, arg1);
    }

    public void removeBuildOptions(BuildOptionsIdentification arg0) {
        int index = findBuildOptions(arg0);
        if (index > -1) {
            vBuildOptions.removeElementAt(index);
        }
    }

    public void removeBuildOptions(int index) {
        if (index > -1 && index < this.size()) {
            vBuildOptions.removeElementAt(index);
        }
    }

    public Vector<BuildOptionsIdentification> getvBuildOptions() {
        return vBuildOptions;
    }

    public void setvBuildOptions(Vector<BuildOptionsIdentification> BuildOptions) {
        vBuildOptions = BuildOptions;
    }

    public Vector<String> getBuildOptionsName() {
        Vector<String> v = new Vector<String>();
        for (int index = 0; index < this.vBuildOptions.size(); index++) {
            v.addElement(vBuildOptions.get(index).getOption());
        }
        return v;
    }

    public int size() {
        return this.vBuildOptions.size();
    }
    
    public Vector<String> toStringVector(int index) {
        Vector<String> v = new Vector<String>();
        v.addElement(getBuildOptions(index).getOption());
        return v;
    }
}
