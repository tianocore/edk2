/** @file
 
 The file is used to override DefaultListModel to create ICheckBoxListModel
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/
package org.tianocore.frameworkwizard.common.ui.iCheckBoxList;

import javax.swing.*;
import java.util.Vector;

public class ICheckBoxListModel extends DefaultListModel {
    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = -8617800969723991017L;

    /**
     This is the default Constructor for the CheckBoxListModel object
     
     **/
    public ICheckBoxListModel() {
    }

    /**
     override DefaultListModel's add method
     
     @param    index
     @param    item
     
     **/
    public void add(int index, ICheckBoxListItem item) {
        super.add(index, item);
    }

    /**
     Add one item at tail
     
     @param item
     
     **/
    public void addElement(ICheckBoxListItem item) {
        super.addElement(item);
    }

    /**
     Get all elements of the list
     
     **/
    public Vector<ICheckBoxListItem> getAllElements() {
        Vector<ICheckBoxListItem> items = new Vector<ICheckBoxListItem>();
        ICheckBoxListItem[] objs = new ICheckBoxListItem[this.size()];
        this.copyInto(objs);
        for (int i = 0; i < size(); i++) {
            items.addElement(objs[i]);
        }
        return items;
    }
}
