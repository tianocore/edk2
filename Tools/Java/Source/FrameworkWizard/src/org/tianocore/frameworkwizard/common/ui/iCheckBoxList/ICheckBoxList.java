/** @file
 
 The file is used to override JList to create a List with CheckBox item 
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/
package org.tianocore.frameworkwizard.common.ui.iCheckBoxList;

import java.util.Vector;

import javax.swing.JList;

public class ICheckBoxList extends JList {
    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = -2843059688070447632L;

    protected ICheckBoxListCellRenderer cellrenderer = new ICheckBoxListCellRenderer();

    protected ICheckBoxListener listener = new ICheckBoxListener(this);

    protected ICheckBoxListModel model = new ICheckBoxListModel();

    /**
     This the default Constructor
     
     **/
    public ICheckBoxList() {
        this(null);
    }

    /**
     This the override constructor to create checkbox item with input vector
     
     @param options
     
     **/
    public ICheckBoxList(Vector<ICheckBoxListItem> items) {
        if (items != null) {
            for (int index = 0; index < items.size(); index++) {
                model.addElement(items.elementAt(index));
            }
        }
        
        //
        // If there exists at least one item, set first item selected.
        //
        if (model.size() > 0) {
            ICheckBoxListItem listItem = (ICheckBoxListItem) model.get(0);
            listItem.setSelected(true);
        }
        this.setCellRenderer(cellrenderer);
        this.setModel(model);
        this.addMouseListener(listener);
        this.addKeyListener(listener);
    }

    /**
     Set all items of the CheckBoxList component.
     
     @param items
     
     **/
    public void setAllItems(Vector<String> items) {
        if (items != null) {
            model.removeAllElements();
            for (int index = 0; index < items.size(); index++) {
                model.addElement(new ICheckBoxListItem(items.elementAt(index)));
            }
        }
        
        //
        // If there exists at least one item, set first item selected.
        //
        if (model.size() > 0) {
            ICheckBoxListItem listItem = (ICheckBoxListItem) model.get(0);
            listItem.setSelected(true);
        }
    }

    /**
     Get All Checked Items of the CheckBoxList component.
     
     @return    All Checked Items
     **/
    public Vector<ICheckBoxListItem> getAllCheckedItems() {
        Vector<ICheckBoxListItem> result = new Vector<ICheckBoxListItem>();

        for (int i = 0; i < this.getModel().getSize(); i++) {
            if (((ICheckBoxListItem) this.getModel().getElementAt(i)).isChecked()) {
                result.addElement((ICheckBoxListItem) this.getModel().getElementAt(i));
            }
        }
        return result;
    }
    
    /**
    Get All Checked Items index of the CheckBoxList component.
    
    @return    All Checked Items index
    **/
   public Vector<Integer> getAllCheckedItemsIndex() {
       Vector<Integer> result = new Vector<Integer>();

       for (int i = 0; i < this.getModel().getSize(); i++) {
           if (((ICheckBoxListItem) this.getModel().getElementAt(i)).isChecked()) {
               result.addElement(i);
           }
       }
       return result;
   }

    /**
     Get All Checked Items String of the CheckBoxList component.
     
     @return Vector
     **/
    public Vector<String> getAllCheckedItemsString() {
        Vector<String> result = new Vector<String>();

        for (int i = 0; i < this.getModel().getSize(); i++) {
            if (((ICheckBoxListItem) this.getModel().getElementAt(i)).isChecked()) {
                result.addElement(((ICheckBoxListItem) this.getModel().getElementAt(i)).text);
            }
        }
        return result;
    }
    
    /**
    Get All Items String of the CheckBoxList component.
    
    @return Vector
    **/
   public Vector<String> getAllItemsString() {
       Vector<String> result = new Vector<String>();

       for (int i = 0; i < this.getModel().getSize(); i++) {
           result.addElement(((ICheckBoxListItem) this.getModel().getElementAt(i)).text);
       }
       return result;
   }

    /**
     Set Checked status for all input items.
     
     **/
    public void initCheckedItem(boolean bool, Vector<String> items) {
        if (items != null && items.size() != 0) {
            for (int indexI = 0; indexI < items.size(); indexI++) {
                for (int indexJ = 0; indexJ < model.size(); indexJ++) {
                    if (items.elementAt(indexI).equals(model.getAllElements().elementAt(indexJ).getText())) {
                        ICheckBoxListItem listItem = (ICheckBoxListItem) model.get(indexJ);
                        listItem.setChecked(bool);
                        break;
                    }
                }
            }
        }
        
        //
        // If there exists at least one item, set first item selected.
        //
        if (model.size() > 0) {
            ICheckBoxListItem listItem = (ICheckBoxListItem) model.get(0);
            listItem.setSelected(true);
        }
        
        this.validate();
    }
    
    /**
    Set all items of the compontent checked
    
    **/
    public void setAllItemsChecked() {
        initCheckedItem(true, this.getAllItemsString());
    }
    
    /**
    Set all items of the compontent unchecked
    
    **/
    public void setAllItemsUnchecked() {
        initCheckedItem(false, this.getAllItemsString());
    }

    /**
     Remove all items of list
     
     **/
    public void removeAllItem() {
        model.removeAllElements();
    }
}
