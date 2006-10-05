/** @file
 
 To show a splash screen when starting
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/

package org.tianocore.frameworkwizard;

import javax.swing.ImageIcon;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JRootPane;

import org.tianocore.frameworkwizard.common.DataType;
import org.tianocore.frameworkwizard.common.ui.IFrame;

public class SplashScreen extends IFrame {

    ///
    /// Serial Version UID
    ///
    private static final long serialVersionUID = 1077736364497801470L;

    private JPanel jContentPane = null;  //  @jve:decl-index=0:visual-constraint="10,54"

    private JLabel jLabelImage = null;
    
    private JLabel jLabel = null;

    private JLabel jLabel1 = null;

    private JLabel jLabel2 = null;


    /**
     * This is the default constructor
     */
    public SplashScreen() {
        super();
        init();
    }

    /**
     * This method initializes this
     * 
     * @return void
     */
    private void init() {
        this.setSize(320, 205);
        this.setUndecorated(true);
        this.setContentPane(getJContentPane());
        this.getRootPane().setWindowDecorationStyle(JRootPane.NONE);
        this.setTitle("Init");
        this.centerWindow();
    }

    /**
     * This method initializes jContentPane
     * 
     * @return javax.swing.JPanel
     */
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jLabelImage = new JLabel();
            jLabelImage.setBounds(new java.awt.Rectangle(63, 20, 193, 58));
            jLabelImage.setIcon(new ImageIcon(getClass().getResource("/resources/images/logo.gif")));

            jLabel2 = new JLabel();
            jLabel2.setBounds(new java.awt.Rectangle(25, 120, 270, 20));
            jLabel2.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
            jLabel2.setFont(new java.awt.Font("Dialog", java.awt.Font.BOLD, 14));
            jLabel2.setText("Initializing...");
            jLabel1 = new JLabel();
            jLabel1.setBounds(new java.awt.Rectangle(3,166,270,20));
            jLabel1.setHorizontalAlignment(javax.swing.SwingConstants.LEFT);
            jLabel1.setFont(new java.awt.Font("Dialog", java.awt.Font.BOLD, 8));
            jLabel1.setText("Copyright (c) 2006, Intel Corporation");
            jLabel = new JLabel();
            jLabel.setToolTipText("");
            jLabel.setBounds(new java.awt.Rectangle(25, 90, 270, 20));
            jLabel.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
            jLabel.setFont(new java.awt.Font("Dialog", java.awt.Font.BOLD, 18));
            jLabel.setText(DataType.PROJECT_NAME + " " + DataType.PROJECT_VERSION);
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.setSize(new java.awt.Dimension(320,199));
            jContentPane.setBackground(java.awt.SystemColor.inactiveCaptionText);
            jContentPane.setBorder(javax.swing.BorderFactory.createBevelBorder(javax.swing.border.BevelBorder.RAISED));
            jContentPane.add(jLabel, null);
            jContentPane.add(jLabel1, null);
            jContentPane.add(jLabel2, null);
            jContentPane.add(jLabelImage, null);
        }
        return jContentPane;
    }

}
