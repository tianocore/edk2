/** @file
 
 The file is used to override JInternalFrame to provides customized interfaces 
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/

package org.tianocore.packaging.common.ui;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.JInternalFrame;

/**
 The class is used to override JInternalFrame to provides customized interfaces
 It extends JInternalFrame implements ActionListener
 
 @since ModuleEditor 1.0

 **/
public class IInternalFrame extends JInternalFrame implements ActionListener {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = -609841772384875886L;
    //
    //Define class members
    //
    private boolean isEdited = false;

    /**
     Main class, reserved for test
     
     @param args
     
     **/
    public static void main(String[] args) {
        // TODO Auto-generated method stub

    }

    /**
     This is the default constructor
     
     **/
    public IInternalFrame() {
        super();
        initialize();
    }

    /**
     This method initializes this
     
     **/
    private void initialize() {
        this.setBounds(new java.awt.Rectangle(0, 0, 500, 500));
    }

    /**
     Get if the InternalFrame has been edited
     
     @retval true - The InternalFrame has been edited
     @retval false - The InternalFrame hasn't been edited
     
     **/
    public boolean isEdited() {
        return isEdited;
    }

    /**
     Set if the InternalFrame has been edited
     
     @param isEdited The input data which identify if the InternalFrame has been edited
     
     **/
    public void setEdited(boolean isEdited) {
        this.isEdited = isEdited;
    }

    /**
     Check the input data is empty or not
     
     @param strValue The input data which need be checked
     
     @retval true - The input data is empty
     @retval fals - The input data is not empty
     
     **/
    public boolean isEmpty(String strValue) {
        if (strValue.length() > 0) {
            return false;
        }
        return true;
    }

    public void actionPerformed(ActionEvent arg0) {
        // TODO Auto-generated method stub
    }
}
