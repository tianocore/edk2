/** @file
 
 The file is used to define common opening file type
  
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/

package org.tianocore.frameworkwizard.common.Identifications;

import javax.swing.tree.TreePath;


public class OpeningFileType {
    //
    // Define class members
    //
    private boolean isSaved = true;
    
    private boolean isNew = false;
    
    private boolean isOpen = false;
    
    private TreePath treePath = null;
    
    public OpeningFileType() {
        
    }
    
    public OpeningFileType(TreePath treePathValue) {
        this.treePath = treePathValue;
    }

    public boolean isNew() {
        return isNew;
    }

    public void setNew(boolean isNew) {
        this.isNew = isNew;
    }

    public boolean isSaved() {
        return isSaved;
    }

    public void setSaved(boolean isSaved) {
        this.isSaved = isSaved;
    }

    public TreePath getTreePath() {
        return treePath;
    }

    public void setTreePath(TreePath treePath) {
        this.treePath = treePath;
    }

    public boolean isOpen() {
        return isOpen;
    }

    public void setOpen(boolean isOpen) {
        this.isOpen = isOpen;
    }
}
