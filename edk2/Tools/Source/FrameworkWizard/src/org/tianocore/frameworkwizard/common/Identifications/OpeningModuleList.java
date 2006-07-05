/** @file
 
 The file is used to define opening module list
 
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

import org.tianocore.ModuleSurfaceAreaDocument;

public class OpeningModuleList {
    private Vector<OpeningModuleType> vOpeningModuleList = new Vector<OpeningModuleType>();

    public OpeningModuleList() {

    }
    public Vector<OpeningModuleType> getVOpeningModuleList() {
        return vOpeningModuleList;
    }

    public void setVOpeningModuleList(Vector<OpeningModuleType> openingModuleList) {
        vOpeningModuleList = openingModuleList;
    }
    
    public void insertToOpeningModuleList(Identification id, ModuleSurfaceAreaDocument.ModuleSurfaceArea xmlMsa) {
        vOpeningModuleList.addElement(new OpeningModuleType(id, xmlMsa));
    }
    
    public OpeningModuleType getOpeningModuleByIndex(int index) {
        if (index > -1 && index < vOpeningModuleList.size()) {
            return vOpeningModuleList.elementAt(index);
        }
        return null;
    }
    
    public OpeningModuleType getOpeningModuleById(Identification id) {
        int index = findIndexOfListById(id);
        if (index > -1) {
            return vOpeningModuleList.elementAt(index);
        }
        return null;
    }
    
    public int findIndexOfListById(Identification id) {
        for (int index = 0; index < vOpeningModuleList.size(); index++) {
            if (vOpeningModuleList.elementAt(index).getId().equals(id)) {
                return index;
            }
        }
        return -1;
    }
    
    public void removeFromOpeningModuleListByIndex(int index) {
        if (index > -1 && index < vOpeningModuleList.size()) {
            vOpeningModuleList.removeElementAt(index);
        }
    }
    
    public void removeFromOpeningModuleListById(Identification id) {
        int index = findIndexOfListById(id);
        if (index > -1) {
            vOpeningModuleList.removeElementAt(findIndexOfListById(id));
        }
    }
    
    public void removeAllFromOpeningModuleList() {
        vOpeningModuleList.removeAllElements();
    }
    
    public ModuleSurfaceAreaDocument.ModuleSurfaceArea getModuleSurfaceAreaFromId(Identification id) {
        int index = findIndexOfListById(id);
        if (index > -1) {
            return vOpeningModuleList.elementAt(index).getXmlMsa();
        }
        return null;
    }
    
    public boolean existsModule(Identification id) {
        int index = findIndexOfListById(id);
        if (index > -1) {
            return true;
        }
        return false;
    }
    
    public void setModuleSaved(Identification id, boolean isSaved) {
        setModuleSaved(findIndexOfListById(id), isSaved);
    }
    
    public void setModuleSaved(int index, boolean isSaved) {
        if (index > -1) {
            vOpeningModuleList.elementAt(index).setSaved(isSaved);
        }
    }
    
    public boolean getModuleSaved(Identification id) {
        return getModuleSaved(findIndexOfListById(id));
    }
    
    public boolean getModuleSaved(int index) {
        if (index > -1) {
            return vOpeningModuleList.elementAt(index).isSaved();
        }
        return true;
    }
    
    public void setTreePathById(Identification id, TreePath treePath) {
        int index = findIndexOfListById(id);
        if (index > -1) {
            vOpeningModuleList.elementAt(index).setTreePath(treePath);
        }
    }
    
    public TreePath getTreePathById(Identification id) {
        int index = findIndexOfListById(id);
        if (index > -1) {
            return vOpeningModuleList.elementAt(index).getTreePath();
        }
        return null;
    }
    
    public TreePath getTreePathByIndex(int index) {
        if (index > -1) {
            return vOpeningModuleList.elementAt(index).getTreePath();
        }
        return null;
    }
    
    public void setNew(Identification id, boolean isNew) {
        int index = findIndexOfListById(id);
        if (index > -1) {
            vOpeningModuleList.elementAt(index).setNew(isNew);
        }
    }
    
    public int size() {
        return vOpeningModuleList.size();
    }
    
    public boolean isSaved() {
        for (int index = 0; index < this.size(); index++) {
            if (!this.getModuleSaved(index)) {
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
