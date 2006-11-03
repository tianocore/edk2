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

import java.util.Set;
import java.util.Vector;

import javax.swing.tree.TreePath;

import org.tianocore.ModuleSurfaceAreaDocument;
import org.tianocore.frameworkwizard.module.Identifications.ModuleIdentification;

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

    public void insertToOpeningModuleList(ModuleIdentification id, ModuleSurfaceAreaDocument.ModuleSurfaceArea xmlMsa) {
        vOpeningModuleList.addElement(new OpeningModuleType(id, xmlMsa));
    }

    public OpeningModuleType getOpeningModuleByIndex(int index) {
        if (index > -1 && index < vOpeningModuleList.size()) {
            return vOpeningModuleList.elementAt(index);
        }
        return null;
    }

    public OpeningModuleType getOpeningModuleById(ModuleIdentification id) {
        int index = findIndexOfListById(id);
        if (index > -1) {
            return vOpeningModuleList.elementAt(index);
        }
        return null;
    }

    public int findIndexOfListById(ModuleIdentification id) {
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

    public void removeFromOpeningModuleListById(ModuleIdentification id) {
        int index = findIndexOfListById(id);
        if (index > -1) {
            vOpeningModuleList.removeElementAt(findIndexOfListById(id));
        }
    }

    public void removeAllFromOpeningModuleList() {
        vOpeningModuleList.removeAllElements();
    }

    public ModuleSurfaceAreaDocument.ModuleSurfaceArea getModuleSurfaceAreaFromId(ModuleIdentification id) {
        int index = findIndexOfListById(id);
        if (index > -1) {
            return vOpeningModuleList.elementAt(index).getXmlMsa();
        }
        return null;
    }

    public boolean existsModule(ModuleIdentification id) {
        int index = findIndexOfListById(id);
        if (index > -1) {
            return true;
        }
        return false;
    }

    public void setModuleSaved(ModuleIdentification id, boolean isSaved) {
        setModuleSaved(findIndexOfListById(id), isSaved);
    }

    public void setModuleSaved(int index, boolean isSaved) {
        if (index > -1) {
            vOpeningModuleList.elementAt(index).setSaved(isSaved);
        }
    }

    public boolean getModuleSaved(ModuleIdentification id) {
        return getModuleSaved(findIndexOfListById(id));
    }

    public boolean getModuleSaved(int index) {
        if (index > -1) {
            return vOpeningModuleList.elementAt(index).isSaved();
        }
        return true;
    }

    public void setModuleOpen(ModuleIdentification id, boolean isOpen) {
        setModuleOpen(findIndexOfListById(id), isOpen);
    }

    public void setModuleOpen(int index, boolean isOpen) {
        if (index > -1) {
            vOpeningModuleList.elementAt(index).setOpen(isOpen);
        }
    }

    public boolean getModuleOpen(ModuleIdentification id) {
        return getModuleOpen(findIndexOfListById(id));
    }

    public boolean getModuleOpen(int index) {
        if (index > -1) {
            return vOpeningModuleList.elementAt(index).isOpen();
        }
        return false;
    }

    public void setTreePathById(ModuleIdentification id, Set<TreePath> treePath) {
        int index = findIndexOfListById(id);
        if (index > -1) {
            vOpeningModuleList.elementAt(index).setTreePath(treePath);
        }
    }

    public Set<TreePath> getTreePathById(ModuleIdentification id) {
        int index = findIndexOfListById(id);
        if (index > -1) {
            return vOpeningModuleList.elementAt(index).getTreePath();
        }
        return null;
    }

    public Set<TreePath> getTreePathByIndex(int index) {
        if (index > -1) {
            return vOpeningModuleList.elementAt(index).getTreePath();
        }
        return null;
    }

    public ModuleIdentification getIdByPath(String path) {
        ModuleIdentification id = new ModuleIdentification(null, null, null, path);
        int index = findIndexOfListById(id);
        if (index > -1) {
            return vOpeningModuleList.elementAt(index).getId();
        }
        return null;
    }

    public ModuleIdentification getIdByGuidVersion(String guid, String version) {
        for (int index = 0; index < vOpeningModuleList.size(); index++) {
            ModuleIdentification id = vOpeningModuleList.elementAt(index).getId();
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

    public void setNew(ModuleIdentification id, boolean isNew) {
        int index = findIndexOfListById(id);
        if (index > -1) {
            vOpeningModuleList.elementAt(index).setNew(isNew);
        }
    }

    public void closeAll() {
        for (int index = 0; index < this.size(); index++) {
            this.setModuleOpen(index, false);
            this.setModuleSaved(index, true);
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

    public boolean isOpen() {
        for (int index = 0; index < this.size(); index++) {
            if (this.getModuleOpen(index)) {
                return true;
            }
        }
        return false;
    }
    
    public void reload(int index) {
        if (index > -1) {
            vOpeningModuleList.elementAt(index).reload();
        }
    }
}
