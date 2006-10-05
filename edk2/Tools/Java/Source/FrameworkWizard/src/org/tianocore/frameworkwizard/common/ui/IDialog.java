/** @file
 
 The file is used to override Dialog to provides customized interfaces 
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/

package org.tianocore.frameworkwizard.common.ui;

import java.awt.Dimension;
import java.awt.Toolkit;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.JDialog;

import org.tianocore.frameworkwizard.common.DataType;
import org.tianocore.frameworkwizard.common.Tools;

/**
 The class is used to override Dialog to provides customized interfaces
 It extends JDialog implements ActionListener
 

 
 **/
public class IDialog extends JDialog implements ActionListener {
    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = -7692623863358631984L;
    //
    //Define class members
    //
    private boolean isEdited = false;
    
    public int returnType = DataType.RETURN_TYPE_CANCEL;

    public void actionPerformed(ActionEvent arg0) {
        // TODO Auto-generated method stub

    }

    /**
     Main class, used for test
     
     @param args
     
     **/
    public static void main(String[] args) {
        IDialog id = new IDialog();
        id.setVisible(true);
    }

    /**
     This is the default constructor
     **/
    public IDialog() {
        super();
        initialize();
    }

    /**
     This is the override constructor
     
     @param parentFrame The parent frame which open the dialog
     @param modal true means the dialog is modal dialog; false means the dialog is not modal dialog
     **/
    public IDialog(IFrame parentFrame, boolean modal) {
        super(parentFrame, modal);
        initialize();
    }
    
    /**
     This is the override constructor
     
     @param parentFrame The parent frame which open the dialog
     @param modal true means the dialog is modal dialog; false means the dialog is not modal dialog
     **/
    public IDialog(IDialog parentFrame, boolean modal) {
        super(parentFrame, modal);
        initialize();
    }

    /**
     This method initializes this
     
     **/
    private void initialize() {
        this.setResizable(false);
    }

    /**
     Start the dialog at the center of screen
     
     @param intWidth The width of the dialog
     @param intHeight The height of the dialog
     
     **/
    protected void centerWindow(int intWidth, int intHeight) {
        Dimension d = Toolkit.getDefaultToolkit().getScreenSize();
        this.setLocation((d.width - intWidth) / 2, (d.height - intHeight) / 2);
    }

    /**
     Start the dialog at the center of screen
     
     **/
    protected void centerWindow() {
        centerWindow(this.getSize().width, this.getSize().height);
    }

    /**
     Get if the dialog has been edited
     
     @retval true - The dialog has been edited
     @retval false - The dialog hasn't been edited
     
     **/
    public boolean isEdited() {
        return isEdited;
    }

    /**
     Set if the dialog has been edited
     
     @param isEdited The input data which identify if the dialog has been edited
     
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
        return Tools.isEmpty(strValue);
    }
    
    /**
     Display the dialog
     
     **/
    public int showDialog() {
        this.setVisible(true);
        return returnType;
    }
}
