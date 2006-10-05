/** @file
 
 The file is used to define Source Files Vector
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/

package org.tianocore.frameworkwizard.module.Identifications.SourceFiles;

import java.util.Vector;

public class SourceFilesVector {
    private Vector<SourceFilesIdentification> vSourceFiles = new Vector<SourceFilesIdentification>();

    public int findSourceFiles(SourceFilesIdentification sfi) {
        for (int index = 0; index < vSourceFiles.size(); index++) {
            if (vSourceFiles.elementAt(index).equals(sfi)) {
                return index;
            }
        }
        return -1;
    }

    public int findSourceFiles(String name) {
        for (int index = 0; index < vSourceFiles.size(); index++) {
            if (vSourceFiles.elementAt(index).getFilename().equals(name)) {
                return index;
            }
        }
        return -1;
    }

    public SourceFilesIdentification getSourceFiles(int index) {
        if (index > -1) {
            return vSourceFiles.elementAt(index);
        } else {
            return null;
        }
    }

    public void addSourceFiles(SourceFilesIdentification sfi) {
        if (findSourceFiles(sfi) == -1) {
            vSourceFiles.addElement(sfi);
        }
    }

    public void setSourceFiles(SourceFilesIdentification sfi, int index) {
        vSourceFiles.setElementAt(sfi, index);
    }

    public void removeSourceFiles(SourceFilesIdentification sfi) {
        int index = findSourceFiles(sfi);
        if (index > -1) {
            vSourceFiles.removeElementAt(index);
        }
    }

    public void removeSourceFiles(int index) {
        if (index > -1 && index < this.size()) {
            vSourceFiles.removeElementAt(index);
        }
    }

    public Vector<SourceFilesIdentification> getvSourceFiles() {
        return vSourceFiles;
    }

    public void setvSourceFiles(Vector<SourceFilesIdentification> SourceFiles) {
        vSourceFiles = SourceFiles;
    }
    
    public Vector<String> getSourceFilesName() {
        Vector<String> v = new Vector<String>();
        for (int index = 0; index < this.vSourceFiles.size(); index++) {
            v.addElement(vSourceFiles.get(index).getFilename());
        }
        return v;
    }

    public int size() {
        return this.vSourceFiles.size();
    }
    
    public Vector<String> toStringVector(int index) {
        Vector<String> v = new Vector<String>();
        v.addElement(getSourceFiles(index).getFilename());
        v.addElement(getSourceFiles(index).getTagName());
        v.addElement(getSourceFiles(index).getToolCode());
        v.addElement(getSourceFiles(index).getToolChainFamily());
        return v;
    }
}
