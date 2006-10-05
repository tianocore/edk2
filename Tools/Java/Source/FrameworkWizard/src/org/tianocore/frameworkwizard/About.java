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

package org.tianocore.frameworkwizard;

import java.awt.event.ActionEvent;
import java.awt.event.WindowEvent;

import javax.swing.JButton;
import javax.swing.JLabel;
import javax.swing.JPanel;

import org.tianocore.frameworkwizard.common.DataType;
import org.tianocore.frameworkwizard.common.ui.IDialog;
import org.tianocore.frameworkwizard.common.ui.IFrame;
import javax.swing.ImageIcon;

/**
 The class is used to show a about window with copyright information
 It extends IDialog

 **/
public class About extends IDialog {

    ///
    /// Define Class Serial Version UID
    ///
    private static final long serialVersionUID = 2958136136667310962L;

    ///
    /// Define Class Members
    ///
    private JPanel jContentPane = null;

    private JLabel jLabel = null;

    private JLabel jLabel1 = null;

    private JLabel jLabel2 = null;

    private JButton jButtonOK = null;

    private JLabel jLabelImage = null;

    /**
     This method initializes jButtonOK 
     
     @return javax.swing.JButton jButtonOK
     
     **/
    private JButton getJButtonOK() {
        if (jButtonOK == null) {
            jButtonOK = new JButton();
            jButtonOK.setBounds(new java.awt.Rectangle(115, 200, 90, 20));
            jButtonOK.setText("OK");
            jButtonOK.addActionListener(this);
        }
        return jButtonOK;
    }

    /**
     This is the default constructor
     
     **/
    public About() {
        super();
        init();
    }

    /**
     This is the default constructor
     
     **/
    public About(IFrame parentFrame, boolean modal) {
        super(parentFrame, modal);
        init();
    }

    /**
     This method initializes this
     
     **/
    private void init() {
        this.setSize(320, 265);
        this.setContentPane(getJContentPane());
        this.setTitle("About");
        this.getRootPane().setDefaultButton(jButtonOK);
        this.centerWindow();
    }

    /**
     This method initializes jContentPane
     
     @return javax.swing.JPanel jContentPane
     
     **/
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jLabelImage = new JLabel();
            jLabelImage.setBounds(new java.awt.Rectangle(63, 20, 193, 58));
            jLabelImage.setIcon(new ImageIcon(getClass().getResource("/resources/images/logo.gif")));

            jLabel2 = new JLabel();
            jLabel2.setBounds(new java.awt.Rectangle(25, 160, 270, 20));
            jLabel2.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
            jLabel2.setText("All rights reserved");
            jLabel1 = new JLabel();
            jLabel1.setBounds(new java.awt.Rectangle(25, 130, 270, 20));
            jLabel1.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
            jLabel1.setText("Copyright (c) 2006, Intel Corporation");
            jLabel = new JLabel();
            jLabel.setToolTipText("");
            jLabel.setBounds(new java.awt.Rectangle(25, 90, 270, 20));
            jLabel.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
            jLabel.setText(DataType.PROJECT_NAME + " " + DataType.PROJECT_VERSION);
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.setSize(new java.awt.Dimension(320, 235));
            jContentPane.add(jLabel, null);
            jContentPane.add(jLabel1, null);
            jContentPane.add(jLabel2, null);
            jContentPane.add(getJButtonOK(), null);
            jContentPane.add(jLabelImage, null);
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
            returnType = DataType.RETURN_TYPE_OK;
            this.setVisible(false);
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
