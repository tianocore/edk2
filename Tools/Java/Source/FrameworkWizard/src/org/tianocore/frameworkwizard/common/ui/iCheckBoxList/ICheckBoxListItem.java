/** @file
 
 The file is used to create list item for CheckBox list
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/
package org.tianocore.frameworkwizard.common.ui.iCheckBoxList;

public class ICheckBoxListItem {

    //
    // Define Class Members
    //
    protected String text;

    protected boolean checked;
    
    protected boolean selected;

    /**
     This is the default constructor to set check box item string
     
     @param text
     
     **/
    public ICheckBoxListItem(String text) {
        this.text = text;
    }

    /**
     This is the override constructor to set check box item string and checked status
     
     @param text
     @param checked
     
     **/
    public ICheckBoxListItem(String text, boolean checked) {
        this.text = text;
        this.checked = checked;
    }

    /**
     set the checked status
     if true, set false
     if false, set true
     
     **/
    public void invertChecked() {
        checked = !checked;
    }

    public boolean isChecked() {
        return checked;
    }

    public void setChecked(boolean checked) {
        this.checked = checked;
    }

    public String getText() {
        return text;
    }

    public void setText(String text) {
        this.text = text;
    }

    public boolean isSelected() {
        return selected;
    }

    public void setSelected(boolean selected) {
        this.selected = selected;
    }

}
