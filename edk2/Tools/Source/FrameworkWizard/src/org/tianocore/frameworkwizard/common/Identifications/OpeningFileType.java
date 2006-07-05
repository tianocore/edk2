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
    
    private Identification id = null;
    
    private boolean isSaved = true;
    
    private boolean isNew = false;
    
    private TreePath treePath = null;
    
    public OpeningFileType() {
        
    }
    
    public OpeningFileType(Identification identification) {
        this.id = identification;
    }
    
    public OpeningFileType(Identification identification, TreePath treePathValue) {
        this.id = identification;
        this.treePath = treePathValue;
    }

    public Identification getId() {
        return id;
    }

    public void setId(Identification id) {
        this.id = id;
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
}
