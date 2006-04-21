/** @file
 
 To show a about window with copyright information
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/

package org.tianocore.packaging.module.ui;

import java.awt.event.ActionEvent;
import java.awt.event.WindowEvent;

import javax.swing.JButton;
import javax.swing.JLabel;
import javax.swing.JPanel;

import org.tianocore.packaging.common.ui.IDialog;

/**
 The class is used to show a about window with copyright information
 It extends IDialog
 
 @since ModuleEditor 1.0

 **/
public class ModuleAbout extends IDialog {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = 2958136136667310962L;

    //
    //Define class members
    //
    private JPanel jContentPane = null;

    private JLabel jLabel = null;

    private JLabel jLabel1 = null;

    private JLabel jLabel2 = null;

    private JButton jButtonOK = null;

    /**
     This method initializes jButtonOK 
     
     @return javax.swing.JButton jButtonOK
     
     **/
    private JButton getJButtonOK() {
        if (jButtonOK == null) {
            jButtonOK = new JButton();
            jButtonOK.setBounds(new java.awt.Rectangle(105, 120, 90, 20));
            jButtonOK.setText("OK");
            jButtonOK.addActionListener(this);
        }
        return jButtonOK;
    }

    public static void main(String[] args) {
    }

    /**
     This is the default constructor
     
     **/
    public ModuleAbout() {
        super();
        init();
    }

    /**
     This method initializes this
     
     **/
    private void init() {
        this.setSize(300, 200);
        this.setContentPane(getJContentPane());
        this.setTitle("About...");
        this.getRootPane().setDefaultButton(jButtonOK);
        this.centerWindow();
        this.setVisible(true);
    }

    /**
     This method initializes jContentPane
     
     @return javax.swing.JPanel jContentPane
     
     **/
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jLabel2 = new JLabel();
            jLabel2.setBounds(new java.awt.Rectangle(15, 80, 270, 20));
            jLabel2.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
            jLabel2.setText("All rights reserved");
            jLabel1 = new JLabel();
            jLabel1.setBounds(new java.awt.Rectangle(15, 50, 270, 20));
            jLabel1.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
            jLabel1.setText("Copyright (c) 2006, Intel Corporation");
            jLabel = new JLabel();
            jLabel.setToolTipText("");
            jLabel.setBounds(new java.awt.Rectangle(15, 20, 270, 20));
            jLabel.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
            jLabel.setText("Framework Development Package System 1.0");
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.add(jLabel, null);
            jContentPane.add(jLabel1, null);
            jContentPane.add(jLabel2, null);
            jContentPane.add(getJButtonOK(), null);
        }
        return jContentPane;
    }

    /* (non-Javadoc)
     * @see java.awt.event.ActionListener#actionPerformed(java.awt.event.ActionEvent)
     * 
     * Override actionPerformed to listen all actions
     */
    public void actionPerformed(ActionEvent arg0) {
        if (arg0.getSource() == jButtonOK) {
            this.dispose();
        }
    }

    /**
     Dispose when windows is closing
     
     @param arg0
     
     **/
    public void windowClosing(WindowEvent arg0) {
        this.dispose();
    }
}
