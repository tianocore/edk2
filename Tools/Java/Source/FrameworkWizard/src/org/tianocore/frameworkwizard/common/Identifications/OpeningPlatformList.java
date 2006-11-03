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

import org.tianocore.PlatformSurfaceAreaDocument;
import org.tianocore.frameworkwizard.platform.PlatformIdentification;

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
    
    public void insertToOpeningPlatformList(PlatformIdentification id, PlatformSurfaceAreaDocument.PlatformSurfaceArea xmlFpd) {
        vOpeningPlatformList.addElement(new OpeningPlatformType(id, xmlFpd));
    }
    
    public OpeningPlatformType getOpeningPlatformByIndex(int index) {
        if (index > -1 && index < vOpeningPlatformList.size()) {
            return vOpeningPlatformList.elementAt(index);
        }
        return null;
    }
    
    public OpeningPlatformType getOpeningPlatformById(PlatformIdentification id) {
        int index = findIndexOfListById(id);
        if (index > -1) {
            return vOpeningPlatformList.elementAt(index);
        }
        return null;
    }
    
    public int findIndexOfListById(PlatformIdentification id) {
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
    
    public void removeFromOpeningPlatformListById(PlatformIdentification id) {
        int index = findIndexOfListById(id);
        if (index > -1) {
            vOpeningPlatformList.removeElementAt(findIndexOfListById(id));
        }
    }
    
    public void removeAllFromOpeningPlatformList() {
        vOpeningPlatformList.removeAllElements();
    }
    
    public PlatformSurfaceAreaDocument.PlatformSurfaceArea getPlatformSurfaceAreaFromId(PlatformIdentification id) {
        int index = findIndexOfListById(id);
        if (index > -1) {
            return vOpeningPlatformList.elementAt(index).getXmlFpd();
        }
        return null;
    }
    
    public boolean existsPlatform(PlatformIdentification id) {
        int index = findIndexOfListById(id);
        if (index > -1) {
            return true;
        }
        return false;
    }
    
    
    public void setPlatformSaved(PlatformIdentification id, boolean isSaved) {
        setPlatformSaved(findIndexOfListById(id), isSaved);
    }
    
    public void setPlatformSaved(int index, boolean isSaved) {
        if (index > -1) {
            vOpeningPlatformList.elementAt(index).setSaved(isSaved);
        }
    }
    
    public boolean getPlatformSaved(PlatformIdentification id) {
        return getPlatformSaved(findIndexOfListById(id));
    }
    
    public boolean getPlatformSaved(int index) {
        if (index > -1) {
            return vOpeningPlatformList.elementAt(index).isSaved();
        }
        return true;
    }
    
    public void setPlatformOpen(PlatformIdentification id, boolean isOpen) {
        setPlatformOpen(findIndexOfListById(id), isOpen);
    }
    
    public void setPlatformOpen(int index, boolean isOpen) {
        if (index > -1) {
            vOpeningPlatformList.elementAt(index).setOpen(isOpen);
        }
    }
    
    public boolean getPlatformOpen(PlatformIdentification id) {
        return getPlatformOpen(findIndexOfListById(id));
    }
    
    public boolean getPlatformOpen(int index) {
        if (index > -1) {
            return vOpeningPlatformList.elementAt(index).isOpen();
        }
        return false;
    }
    
    public void setTreePathById(PlatformIdentification id, Set<TreePath> treePath) {
        int index = findIndexOfListById(id);
        if (index > -1) {
            vOpeningPlatformList.elementAt(index).setTreePath(treePath);
        }
    }
    
    public Set<TreePath> getTreePathById(PlatformIdentification id) {
        int index = findIndexOfListById(id);
        if (index > -1) {
            return vOpeningPlatformList.elementAt(index).getTreePath();
        }
        return null;
    }
    
    public Set<TreePath> getTreePathByIndex(int index) {
        if (index > -1) {
            return vOpeningPlatformList.elementAt(index).getTreePath();
        }
        return null;
    }
    
    public PlatformIdentification getIdByPath(String path) {
        PlatformIdentification id = new PlatformIdentification(null, null, null, path);
        int index = findIndexOfListById(id);
        if (index > -1) {
            return vOpeningPlatformList.elementAt(index).getId();
        }
        return null;
    }
    
    public void setNew(PlatformIdentification id, boolean isNew) {
        int index = findIndexOfListById(id);
        if (index > -1) {
            vOpeningPlatformList.elementAt(index).setNew(isNew);
        }
    }
    
    public void closeAll() {
        for (int index = 0; index < this.size(); index++) {
           this.setPlatformOpen(index, false);
           this.setPlatformSaved(index, true);
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
    
    public boolean isOpen() {
        for (int index = 0; index < this.size(); index++) {
            if (this.getPlatformOpen(index)) {
                return true;
            }
        }
        return false;
    }
    
    public void reload(int index) {
        if (index > -1) {
            vOpeningPlatformList.elementAt(index).reload();
        }
    }
}
