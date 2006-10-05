/** @file
 
 The file is used to show a new file chooser dialog
 
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

import javax.swing.ButtonGroup;
import javax.swing.JPanel;
import javax.swing.JButton;
import javax.swing.JRadioButton;

import org.tianocore.frameworkwizard.common.DataType;
import org.tianocore.frameworkwizard.common.ui.IDialog;
import org.tianocore.frameworkwizard.common.ui.IFrame;

public class NewFileChooser extends IDialog {

    /**
     Define class members
     
     **/
    private static final long serialVersionUID = -3657926400683618281L;

    private JPanel jContentPane = null;

    private JButton jButtonNext = null;

    private JButton jButtonCancel = null;

    private JRadioButton jRadioButtonModule = null;

    private JRadioButton jRadioButtonPackage = null;

    private JRadioButton jRadioButtonPlatform = null;

    private JRadioButton jRadioButtonBuildXml = null;

    private final ButtonGroup buttonGroup = new ButtonGroup();

    /**
     This is the default constructor
     
     **/
    public NewFileChooser() {
        super();
        init();
    }

    /**
     This is the default constructor
     
     **/
    public NewFileChooser(IFrame parentFrame, boolean modal) {
        super(parentFrame, modal);
        init();
    }

    /**
     * This method initializes jButtonOk	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonOk() {
        if (jButtonNext == null) {
            jButtonNext = new JButton();
            jButtonNext.setBounds(new java.awt.Rectangle(90, 150, 80, 20));
            jButtonNext.setText("Next");
            jButtonNext.addActionListener(this);
        }
        return jButtonNext;
    }

    /**
     * This method initializes jButtonCancel	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonCancel() {
        if (jButtonCancel == null) {
            jButtonCancel = new JButton();
            jButtonCancel.setBounds(new java.awt.Rectangle(180, 150, 80, 20));
            jButtonCancel.setText("Cancel");
            jButtonCancel.addActionListener(this);
        }
        return jButtonCancel;
    }

    /**
     * This method initializes jRadioButtonModule	
     * 	
     * @return javax.swing.JRadioButton	
     */
    private JRadioButton getJRadioButtonModule() {
        if (jRadioButtonModule == null) {
            jRadioButtonModule = new JRadioButton();
            jRadioButtonModule.setText(DataType.MODULE_SURFACE_AREA);
            jRadioButtonModule.setBounds(new java.awt.Rectangle(20, 20, 240, 20));
            jRadioButtonModule.setSelected(true);
        }
        return jRadioButtonModule;
    }

    /**
     * This method initializes jRadioButtonPackage	
     * 	
     * @return javax.swing.JRadioButton	
     */
    private JRadioButton getJRadioButtonPackage() {
        if (jRadioButtonPackage == null) {
            jRadioButtonPackage = new JRadioButton();
            jRadioButtonPackage.setText(DataType.PACKAGE_SURFACE_AREA);
            jRadioButtonPackage.setBounds(new java.awt.Rectangle(20, 50, 240, 20));
        }
        return jRadioButtonPackage;
    }

    /**
     * This method initializes jRadioButtonPlatform	
     * 	
     * @return javax.swing.JRadioButton	
     */
    private JRadioButton getJRadioButtonPlatform() {
        if (jRadioButtonPlatform == null) {
            jRadioButtonPlatform = new JRadioButton();
            jRadioButtonPlatform.setText(DataType.PLATFORM_SURFACE_AREA);
            jRadioButtonPlatform.setBounds(new java.awt.Rectangle(20, 80, 240, 20));
        }
        return jRadioButtonPlatform;
    }

    /**
     * This method initializes jRadioButtonBuildXml	
     * 	
     * @return javax.swing.JRadioButton	
     */
    private JRadioButton getJRadioButtonBuildXml() {
        if (jRadioButtonBuildXml == null) {
            jRadioButtonBuildXml = new JRadioButton();
            jRadioButtonBuildXml.setText(DataType.ANT_BUILD_FILE);
            jRadioButtonBuildXml.setBounds(new java.awt.Rectangle(20, 110, 240, 20));
            jRadioButtonBuildXml.setVisible(false);
        }
        return jRadioButtonBuildXml;
    }

    /**
     * @param args
     */
    public static void main(String[] args) {
        NewFileChooser nfc = new NewFileChooser();
        nfc.setVisible(true);
    }

    /**
     * This method initializes this
     * 
     * @return void
     */
    private void init() {
        this.setSize(310, 220);
        this.setContentPane(getJContentPane());
        this.setTitle("Select New File Type");
        this.centerWindow();
    }

    /**
     * This method initializes jContentPane
     * 
     * @return javax.swing.JPanel
     */
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.add(getJButtonOk(), null);
            jContentPane.add(getJButtonCancel(), null);
            jContentPane.add(getJRadioButtonModule(), null);
            jContentPane.add(getJRadioButtonPackage(), null);
            jContentPane.add(getJRadioButtonPlatform(), null);
            jContentPane.add(getJRadioButtonBuildXml(), null);
            buttonGroup.add(this.getJRadioButtonModule());
            buttonGroup.add(this.getJRadioButtonPackage());
            buttonGroup.add(this.getJRadioButtonPlatform());
            buttonGroup.add(this.getJRadioButtonBuildXml());
        }
        return jContentPane;
    }

    /* (non-Javadoc)
     * @see java.awt.event.ActionListener#actionPerformed(java.awt.event.ActionEvent)
     * 
     * Override actionPerformed to listen all actions
     */
    public void actionPerformed(ActionEvent arg0) {
        if (arg0.getSource() == jButtonCancel) {
            this.setVisible(false);
            returnType = DataType.RETURN_TYPE_CANCEL;
        }

        if (arg0.getSource() == jButtonNext) {
            this.setVisible(false);
            if (this.jRadioButtonModule.isSelected()) {
                returnType = DataType.RETURN_TYPE_MODULE_SURFACE_AREA;
            }
            if (this.jRadioButtonPackage.isSelected()) {
                returnType = DataType.RETURN_TYPE_PACKAGE_SURFACE_AREA;
            }
            if (this.jRadioButtonPlatform.isSelected()) {
                returnType = DataType.RETURN_TYPE_PLATFORM_SURFACE_AREA;
            }
            if (this.jRadioButtonBuildXml.isSelected()) {
                returnType = DataType.RETURN_TYPE_BUILD_XML;
            }
        }
    }
}
