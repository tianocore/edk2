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

import org.tianocore.PlatformSurfaceAreaDocument;

public class OpeningPlatformList {
    
    private Vector<OpeningPlatformType> vOpeningPlatformList = new Vector<OpeningPlatformType>();

    public OpeningPlatformList() {

    }
    
    public Vector<OpeningPlatformType> getVOpeningPlatformList() {
        return vOpeningPlatformList;
    }

    public void setVOpeningPlatformList(Vector<OpeningPlatformType> openingPlatformList) {
        vOpeningPlatformList = openingPlatformList;
    }
    
    public void insertToOpeningPlatformList(Identification id, PlatformSurfaceAreaDocument.PlatformSurfaceArea xmlFpd) {
        vOpeningPlatformList.addElement(new OpeningPlatformType(id, xmlFpd));
    }
    
    public OpeningPlatformType getOpeningPlatformByIndex(int index) {
        if (index > -1 && index < vOpeningPlatformList.size()) {
            return vOpeningPlatformList.elementAt(index);
        }
        return null;
    }
    
    public OpeningPlatformType getOpeningPlatformById(Identification id) {
        int index = findIndexOfListById(id);
        if (index > -1) {
            return vOpeningPlatformList.elementAt(index);
        }
        return null;
    }
    
    public int findIndexOfListById(Identification id) {
        for (int index = 0; index < vOpeningPlatformList.size(); index++) {
            if (vOpeningPlatformList.elementAt(index).getId().equals(id)) {
                return index;
            }
        }
        return -1;
    }
    
    public void removeFromOpeningPlatformListByIndex(int index) {
        if (index > -1 && index < vOpeningPlatformList.size()) {
            vOpeningPlatformList.removeElementAt(index);
        }
    }
    
    public void removeFromOpeningPlatformListById(Identification id) {
        int index = findIndexOfListById(id);
        if (index > -1) {
            vOpeningPlatformList.removeElementAt(findIndexOfListById(id));
        }
    }
    
    public void removeAllFromOpeningPlatformList() {
        vOpeningPlatformList.removeAllElements();
    }
    
    public PlatformSurfaceAreaDocument.PlatformSurfaceArea getPlatformSurfaceAreaFromId(Identification id) {
        int index = findIndexOfListById(id);
        if (index > -1) {
            return vOpeningPlatformList.elementAt(index).getXmlFpd();
        }
        return null;
    }
    
    public boolean existsPlatform(Identification id) {
        int index = findIndexOfListById(id);
        if (index > -1) {
            return true;
        }
        return false;
    }
    
    
    public void setPlatformSaved(Identification id, boolean isSaved) {
        setPlatformSaved(findIndexOfListById(id), isSaved);
    }
    
    public void setPlatformSaved(int index, boolean isSaved) {
        if (index > -1) {
            vOpeningPlatformList.elementAt(index).setSaved(isSaved);
        }
    }
    
    public boolean getPlatformSaved(Identification id) {
        return getPlatformSaved(findIndexOfListById(id));
    }
    
    public boolean getPlatformSaved(int index) {
        if (index > -1) {
            return vOpeningPlatformList.elementAt(index).isSaved();
        }
        return true;
    }
    
    public void setTreePathById(Identification id, TreePath treePath) {
        int index = findIndexOfListById(id);
        if (index > -1) {
            vOpeningPlatformList.elementAt(index).setTreePath(treePath);
        }
    }
    
    public TreePath getTreePathById(Identification id) {
        int index = findIndexOfListById(id);
        if (index > -1) {
            return vOpeningPlatformList.elementAt(index).getTreePath();
        }
        return null;
    }
    
    public TreePath getTreePathByIndex(int index) {
        if (index > -1) {
            return vOpeningPlatformList.elementAt(index).getTreePath();
        }
        return null;
    }
    
    public void setNew(Identification id, boolean isNew) {
        int index = findIndexOfListById(id);
        if (index > -1) {
            vOpeningPlatformList.elementAt(index).setNew(isNew);
        }
    }
    
    public int size() {
        return vOpeningPlatformList.size();
    }
    
    public boolean isSaved() {
        for (int index = 0; index < this.size(); index++) {
            if (!this.getPlatformSaved(index)) {
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
