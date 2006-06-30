/** @file
 
 The file is used to override JLabel to provides customized interfaces 
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/

package org.tianocore.frameworkwizard.common.ui;

import javax.swing.JLabel;

/**
 The class is used to override JLabel to provides customized interfaces 
 


 **/
public class StarLabel extends JLabel {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = -6702981027831543919L;

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
    public StarLabel() {
        super();
        init();
    }

    /**
     To create a RED, BOLD and 14 size "*"
     
     **/
    private void init() {
        this.setText("*");
        this.setSize(new java.awt.Dimension(10, 20));
        this.setForeground(java.awt.Color.red);
        this.setFont(new java.awt.Font("DialogInput", java.awt.Font.BOLD, 14));
        this.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
    }
}
