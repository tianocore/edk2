/** @file
 
 The file is used to define opening package list
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/
package org.tianocore.frameworkwizard.common.Identifications;

import java.util.Vector;

import javax.swing.tree.TreePath;

import org.tianocore.PackageSurfaceAreaDocument;

public class OpeningPackageList {
    private Vector<OpeningPackageType> vOpeningPackageList = new Vector<OpeningPackageType>();

    public OpeningPackageList() {

    }
    
    public Vector<OpeningPackageType> getVOpeningPackageList() {
        return vOpeningPackageList;
    }

    public void setVOpeningPackageList(Vector<OpeningPackageType> openingPackageList) {
        vOpeningPackageList = openingPackageList;
    }
    
    public void insertToOpeningPackageList(Identification id, PackageSurfaceAreaDocument.PackageSurfaceArea xmlMsa) {
        vOpeningPackageList.addElement(new OpeningPackageType(id, xmlMsa));
    }
    
    public OpeningPackageType getOpeningPackageByIndex(int index) {
        if (index > -1 && index < vOpeningPackageList.size()) {
            return vOpeningPackageList.elementAt(index);
        }
        return null;
    }
    
    public OpeningPackageType getOpeningPackageById(Identification id) {
        int index = findIndexOfListById(id);
        if (index > -1) {
            return vOpeningPackageList.elementAt(index);
        }
        return null;
    }
    
    public int findIndexOfListById(Identification id) {
        for (int index = 0; index < vOpeningPackageList.size(); index++) {
            if (vOpeningPackageList.elementAt(index).getId().equals(id)) {
                return index;
            }
        }
        return -1;
    }
    
    public void removeFromOpeningPackageListByIndex(int index) {
        if (index > -1 && index < vOpeningPackageList.size()) {
            vOpeningPackageList.removeElementAt(index);
        }
    }
    
    public void removeFromOpeningPackageListById(Identification id) {
        int index = findIndexOfListById(id);
        if (index > -1) {
            vOpeningPackageList.removeElementAt(findIndexOfListById(id));
        }
    }
    
    public void removeAllFromOpeningPackageList() {
        vOpeningPackageList.removeAllElements();
    }
    
    public PackageSurfaceAreaDocument.PackageSurfaceArea getPackageSurfaceAreaFromId(Identification id) {
        int index = findIndexOfListById(id);
        if (index > -1) {
            return vOpeningPackageList.elementAt(index).getXmlSpd();
        }
        return null;
    }
    
    public boolean existsPackage(Identification id) {
        int index = findIndexOfListById(id);
        if (index > -1) {
            return true;
        }
        return false;
    }
    
    public void setPackageSaved(Identification id, boolean isSaved) {
        setPackageSaved(findIndexOfListById(id), isSaved);
    }
    
    public void setPackageSaved(int index, boolean isSaved) {
        if (index > -1) {
            vOpeningPackageList.elementAt(index).setSaved(isSaved);
        }
    }
    
    public boolean getPackageSaved(Identification id) {
        return getPackageSaved(findIndexOfListById(id));
    }
    
    public boolean getPackageSaved(int index) {
        if (index > -1) {
            return vOpeningPackageList.elementAt(index).isSaved();
        }
        return true;
    }
    
    public void setTreePathById(Identification id, TreePath treePath) {
        int index = findIndexOfListById(id);
        if (index > -1) {
            vOpeningPackageList.elementAt(index).setTreePath(treePath);
        }
    }
    
    public TreePath getTreePathById(Identification id) {
        int index = findIndexOfListById(id);
        if (index > -1) {
            return vOpeningPackageList.elementAt(index).getTreePath();
        }
        return null;
    }
    
    public TreePath getTreePathByIndex(int index) {
        if (index > -1) {
            return vOpeningPackageList.elementAt(index).getTreePath();
        }
        return null;
    }
    
    public void setNew(Identification id, boolean isNew) {
        int index = findIndexOfListById(id);
        if (index > -1) {
            vOpeningPackageList.elementAt(index).setNew(isNew);
        }
    }
    
    public int size() {
        return vOpeningPackageList.size();
    }
    
    public boolean isSaved() {
        for (int index = 0; index < this.size(); index++) {
            if (!this.getPackageSaved(index)) {
                return false;
            }
        }
        return true;
    }
    
    public boolean isOpend() {
        if (this.size() > 0 ) {
            return true;
        }
        return false;
    }
}
