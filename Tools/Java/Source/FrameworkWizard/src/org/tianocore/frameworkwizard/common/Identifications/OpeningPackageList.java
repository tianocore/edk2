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

import java.util.Set;
import java.util.Vector;

import javax.swing.tree.TreePath;

import org.tianocore.PackageSurfaceAreaDocument;
import org.tianocore.frameworkwizard.packaging.PackageIdentification;

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
    
    public void insertToOpeningPackageList(PackageIdentification id, PackageSurfaceAreaDocument.PackageSurfaceArea xmlMsa) {
        vOpeningPackageList.addElement(new OpeningPackageType(id, xmlMsa));
    }
    
    public OpeningPackageType getOpeningPackageByIndex(int index) {
        if (index > -1 && index < vOpeningPackageList.size()) {
            return vOpeningPackageList.elementAt(index);
        }
        return null;
    }
    
    public OpeningPackageType getOpeningPackageById(PackageIdentification id) {
        int index = findIndexOfListById(id);
        if (index > -1) {
            return vOpeningPackageList.elementAt(index);
        }
        return null;
    }
    
    public int findIndexOfListById(PackageIdentification id) {
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
    
    public void removeFromOpeningPackageListById(PackageIdentification id) {
        int index = findIndexOfListById(id);
        if (index > -1) {
            vOpeningPackageList.removeElementAt(findIndexOfListById(id));
        }
    }
    
    public void removeAllFromOpeningPackageList() {
        vOpeningPackageList.removeAllElements();
    }
    
    public PackageSurfaceAreaDocument.PackageSurfaceArea getPackageSurfaceAreaFromId(PackageIdentification id) {
        int index = findIndexOfListById(id);
        if (index > -1) {
            return vOpeningPackageList.elementAt(index).getXmlSpd();
        }
        return null;
    }
    
    public boolean existsPackage(PackageIdentification id) {
        int index = findIndexOfListById(id);
        if (index > -1) {
            return true;
        }
        return false;
    }
    
    public void setPackageSaved(PackageIdentification id, boolean isSaved) {
        setPackageSaved(findIndexOfListById(id), isSaved);
    }
    
    public void setPackageSaved(int index, boolean isSaved) {
        if (index > -1) {
            vOpeningPackageList.elementAt(index).setSaved(isSaved);
        }
    }
    
    public boolean getPackageSaved(PackageIdentification id) {
        return getPackageSaved(findIndexOfListById(id));
    }
    
    public boolean getPackageSaved(int index) {
        if (index > -1) {
            return vOpeningPackageList.elementAt(index).isSaved();
        }
        return true;
    }
    
    public void setPackageOpen(PackageIdentification id, boolean isOpem) {
        setPackageOpen(findIndexOfListById(id), isOpem);
    }
    
    public void setPackageOpen(int index, boolean isOpem) {
        if (index > -1) {
            vOpeningPackageList.elementAt(index).setOpen(isOpem);
        }
    }
    
    public boolean getPackageOpen(PackageIdentification id) {
        return getPackageOpen(findIndexOfListById(id));
    }
    
    public boolean getPackageOpen(int index) {
        if (index > -1) {
            return vOpeningPackageList.elementAt(index).isOpen();
        }
        return false;
    }
    
    public void setTreePathById(PackageIdentification id, Set<TreePath> treePath) {
        int index = findIndexOfListById(id);
        if (index > -1) {
            vOpeningPackageList.elementAt(index).setTreePath(treePath);
        }
    }
    
    public Set<TreePath> getTreePathById(PackageIdentification id) {
        int index = findIndexOfListById(id);
        if (index > -1) {
            return vOpeningPackageList.elementAt(index).getTreePath();
        }
        return null;
    }
    
    public Set<TreePath> getTreePathByIndex(int index) {
        if (index > -1) {
            return vOpeningPackageList.elementAt(index).getTreePath();
        }
        return null;
    }
    
    public PackageIdentification getIdByPath(String path) {
        PackageIdentification id = new PackageIdentification(null, null, null, path);
        int index = findIndexOfListById(id);
        if (index > -1) {
            return vOpeningPackageList.elementAt(index).getId();
        }
        return null;
    }
    
    public PackageIdentification getIdByGuidVersion(String guid, String version) {
        for (int index = 0; index < vOpeningPackageList.size(); index++) {
            PackageIdentification id = vOpeningPackageList.elementAt(index).getId();
            if (version != null) {
                if (id.getGuid().equals(guid) && id.getVersion().equals(version)) {
                    return id;
                }
            } else {
                if (id.getGuid().equals(guid)) {
                    return id;
                }
            }
        }
        return null;
    }
    
    public void setNew(PackageIdentification id, boolean isNew) {
        int index = findIndexOfListById(id);
        if (index > -1) {
            vOpeningPackageList.elementAt(index).setNew(isNew);
        }
    }
    
    public void closeAll() {
        for (int index = 0; index < this.size(); index++) {
           this.setPackageOpen(index, false);
           this.setPackageSaved(index, true);
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
    
    public boolean isOpen() {
        for (int index = 0; index < this.size(); index++) {
            if (this.getPackageOpen(index)) {
                return true;
            }
        }
        return false;
    }
    
    public void reload(int index) {
        if (index > -1) {
            vOpeningPackageList.elementAt(index).reload();
        }
    }
}
