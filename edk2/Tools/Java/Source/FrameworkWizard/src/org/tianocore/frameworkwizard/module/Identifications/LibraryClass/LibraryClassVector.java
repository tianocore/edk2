/** @file
 
 The file is used to define Library Class Vector
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/
package org.tianocore.frameworkwizard.module.Identifications.LibraryClass;

import java.util.Vector;

public class LibraryClassVector {
    private Vector<LibraryClassIdentification> vLibraryClass = new Vector<LibraryClassIdentification>();

    public int findLibraryClass(LibraryClassIdentification lib) {
        for (int index = 0; index < vLibraryClass.size(); index++) {
            if (vLibraryClass.elementAt(index).getLibraryClassName().equals(lib.getLibraryClassName())
                && vLibraryClass.elementAt(index).getUsage().equals(lib.getUsage())) {
                if (vLibraryClass.elementAt(index).getBelongModule() != null && lib.getBelongModule() != null) {
                    if (vLibraryClass.elementAt(index).getBelongModule().equals(lib.getBelongModule())) {
                        return index;
                    }
                }
                if (vLibraryClass.elementAt(index).getBelongModule() == null && lib.getBelongModule() == null) {
                    return index;
                }
            }
        }
        return -1;
    }

    public int findLibraryClass(String name) {
        for (int index = 0; index < vLibraryClass.size(); index++) {
            if (vLibraryClass.elementAt(index).getLibraryClassName().equals(name)) {
                return index;
            }
        }
        return -1;
    }

    public LibraryClassIdentification getLibraryClass(int index) {
        if (index > -1) {
            return vLibraryClass.elementAt(index);
        } else {
            return null;
        }
    }

    public void addLibraryClass(LibraryClassIdentification lib) {
        if (findLibraryClass(lib) == -1) {
            vLibraryClass.addElement(lib);
        }
    }

    public void setLibraryClass(LibraryClassIdentification lib, int index) {
        vLibraryClass.setElementAt(lib, index);
    }

    public void removeLibraryClass(LibraryClassIdentification lib) {
        int index = findLibraryClass(lib);
        if (index > -1) {
            vLibraryClass.removeElementAt(index);
        }
    }

    public void removeLibraryClass(int index) {
        if (index > -1 && index < this.size()) {
            vLibraryClass.removeElementAt(index);
        }
    }

    public Vector<LibraryClassIdentification> getVLibraryClass() {
        return vLibraryClass;
    }

    public void setVLibraryClass(Vector<LibraryClassIdentification> libraryClass) {
        vLibraryClass = libraryClass;
    }

    public Vector<String> getLibraryClassName() {
        Vector<String> v = new Vector<String>();
        for (int index = 0; index < this.vLibraryClass.size(); index++) {
            v.addElement(vLibraryClass.get(index).getLibraryClassName());
        }
        return v;
    }

    public int size() {
        return this.vLibraryClass.size();
    }

    public Vector<String> toStringVector(int index) {
        Vector<String> v = new Vector<String>();
        v.addElement(getLibraryClass(index).getLibraryClassName());
        v.addElement(getLibraryClass(index).getUsage());
        return v;
    }
}
