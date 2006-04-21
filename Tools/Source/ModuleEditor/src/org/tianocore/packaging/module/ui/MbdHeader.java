/** @file
 
 This file is used to create, update MbdHeader of a MBD file
 
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

import javax.swing.JButton;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;
import javax.swing.JTextField;

import org.tianocore.BaseNameDocument;
import org.tianocore.GuidDocument;
import org.tianocore.LicenseDocument;
import org.tianocore.MbdHeaderDocument;
import org.tianocore.common.DataValidation;
import org.tianocore.common.Log;
import org.tianocore.common.Tools;
import org.tianocore.packaging.common.ui.IInternalFrame;
import org.tianocore.packaging.common.ui.StarLabel;

/**
 This class is used to create, update MbdHeader of a MBD file
 It extends IInternalFrame
 
 @since ModuleEditor 1.0

 **/
public class MbdHeader extends IInternalFrame {
    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = -2015726615436197378L;

    //
    // Define class members
    //
    private JPanel jContentPane = null;

    private JLabel jLabelBaseName = null;

    private JTextField jTextFieldBaseName = null;

    private JLabel jLabelGuid = null;

    private JTextField jTextFieldGuid = null;

    private JLabel jLabelVersion = null;

    private JTextField jTextFieldVersion = null;

    private JButton jButtonGenerateGuid = null;

    private JLabel jLabelLicense = null;

    private JTextArea jTextAreaLicense = null;

    private JLabel jLabelCopyright = null;

    private JTextArea jTextAreaCopyright = null;

    private JLabel jLabelDescription = null;

    private JTextArea jTextAreaDescription = null;

    private JButton jButtonOk = null;

    private JButton jButtonCancel = null;

    private JScrollPane jScrollPaneLicense = null;

    private JScrollPane jScrollPaneCopyright = null;

    private JScrollPane jScrollPaneDescription = null;

    private StarLabel jStarLabel1 = null;

    private StarLabel jStarLabel2 = null;

    private StarLabel jStarLabel3 = null;

    private StarLabel jStarLabel4 = null;

    private StarLabel jStarLabel5 = null;

    private StarLabel jStarLabel6 = null;

    private MbdHeaderDocument.MbdHeader mbdHeader = null;

    /**
     This method initializes jTextFieldBaseName
     
     @return javax.swing.JTextField jTextFieldBaseName
     
     **/
    private JTextField getJTextFieldBaseName() {
        if (jTextFieldBaseName == null) {
            jTextFieldBaseName = new JTextField();
            jTextFieldBaseName.setBounds(new java.awt.Rectangle(160, 10, 320, 20));
        }
        return jTextFieldBaseName;
    }

    /**
     This method initializes jTextFieldGuid 
     
     @return javax.swing.JTextField jTextFieldGuid
     
     **/
    private JTextField getJTextFieldGuid() {
        if (jTextFieldGuid == null) {
            jTextFieldGuid = new JTextField();
            jTextFieldGuid.setBounds(new java.awt.Rectangle(160, 35, 240, 20));
        }
        return jTextFieldGuid;
    }

    /**
     This method initializes jTextFieldVersion 
     
     @return javax.swing.JTextField jTextFieldVersion
     
     **/
    private JTextField getJTextFieldVersion() {
        if (jTextFieldVersion == null) {
            jTextFieldVersion = new JTextField();
            jTextFieldVersion.setBounds(new java.awt.Rectangle(160, 60, 320, 20));
        }
        return jTextFieldVersion;
    }

    /**
     This method initializes jButtonGenerateGuid 
     
     @return javax.swing.JButton jButtonGenerateGuid
     
     **/
    private JButton getJButtonGenerateGuid() {
        if (jButtonGenerateGuid == null) {
            jButtonGenerateGuid = new JButton();
            jButtonGenerateGuid.setBounds(new java.awt.Rectangle(405, 35, 75, 20));
            jButtonGenerateGuid.setText("GEN");
            jButtonGenerateGuid.addActionListener(this);
        }
        return jButtonGenerateGuid;
    }

    /**
     This method initializes jTextAreaLicense 
     
     @return javax.swing.JTextArea jTextAreaLicense
     
     **/
    private JTextArea getJTextAreaLicense() {
        if (jTextAreaLicense == null) {
            jTextAreaLicense = new JTextArea();
            jTextAreaLicense.setText("");
            jTextAreaLicense.setLineWrap(true);
        }
        return jTextAreaLicense;
    }

    /**
     This method initializes jTextAreaCopyright 
     
     @return javax.swing.JTextArea jTextAreaCopyright
     
     **/
    private JTextArea getJTextAreaCopyright() {
        if (jTextAreaCopyright == null) {
            jTextAreaCopyright = new JTextArea();
            jTextAreaCopyright.setLineWrap(true);
        }
        return jTextAreaCopyright;
    }

    /**
     This method initializes jTextAreaDescription 
     
     @return javax.swing.JTextArea jTextAreaDescription
     
     **/
    private JTextArea getJTextAreaDescription() {
        if (jTextAreaDescription == null) {
            jTextAreaDescription = new JTextArea();
            jTextAreaDescription.setLineWrap(true);
        }
        return jTextAreaDescription;
    }

    /**
     This method initializes jButtonOk 
     
     @return javax.swing.JButton jButtonOk
     
     **/
    private JButton getJButtonOk() {
        if (jButtonOk == null) {
            jButtonOk = new JButton();
            jButtonOk.setText("OK");
            jButtonOk.setBounds(new java.awt.Rectangle(290, 345, 90, 20));
            jButtonOk.addActionListener(this);
        }
        return jButtonOk;
    }

    /**
     This method initializes jButtonCancel 
     
     @return javax.swing.JButton jButtonCancel
     
     **/
    private JButton getJButtonCancel() {
        if (jButtonCancel == null) {
            jButtonCancel = new JButton();
            jButtonCancel.setText("Cancel");
            jButtonCancel.setBounds(new java.awt.Rectangle(390, 345, 90, 20));
            jButtonCancel.addActionListener(this);
        }
        return jButtonCancel;
    }

    /**
     This method initializes jScrollPaneLicense 
     
     @return javax.swing.JScrollPane jScrollPaneLicense
     
     **/
    private JScrollPane getJScrollPaneLicense() {
        if (jScrollPaneLicense == null) {
            jScrollPaneLicense = new JScrollPane();
            jScrollPaneLicense.setBounds(new java.awt.Rectangle(160, 85, 320, 80));
            jScrollPaneLicense.setHorizontalScrollBarPolicy(javax.swing.JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);
            jScrollPaneLicense.setViewportView(getJTextAreaLicense());
        }
        return jScrollPaneLicense;
    }

    /**
     This method initializes jScrollPaneCopyright 
     
     @return javax.swing.JScrollPane jScrollPaneCopyright
     
     **/
    private JScrollPane getJScrollPaneCopyright() {
        if (jScrollPaneCopyright == null) {
            jScrollPaneCopyright = new JScrollPane();
            jScrollPaneCopyright.setBounds(new java.awt.Rectangle(160, 170, 320, 80));
            jScrollPaneCopyright.setHorizontalScrollBarPolicy(javax.swing.JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);
            jScrollPaneCopyright.setViewportView(getJTextAreaCopyright());
        }
        return jScrollPaneCopyright;
    }

    /**
     This method initializes jScrollPaneDescription 
     
     @return javax.swing.JScrollPane jScrollPaneDescription
     
     **/
    private JScrollPane getJScrollPaneDescription() {
        if (jScrollPaneDescription == null) {
            jScrollPaneDescription = new JScrollPane();
            jScrollPaneDescription.setBounds(new java.awt.Rectangle(160, 255, 320, 80));
            jScrollPaneDescription.setHorizontalScrollBarPolicy(javax.swing.JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);
            jScrollPaneDescription.setViewportView(getJTextAreaDescription());
        }
        return jScrollPaneDescription;
    }

    public static void main(String[] args) {

    }

    /**
     This is the default constructor
     
     **/
    public MbdHeader() {
        super();
        init();
        this.setVisible(true);
    }

    /**
     This is the override edit constructor
     
     **/
    public MbdHeader(MbdHeaderDocument.MbdHeader inMbdHeader) {
        super();
        init(inMbdHeader);
        this.setVisible(true);
        this.setViewMode(false);
    }

    /**
     This method initializes this
     
     **/
    private void init() {
        this.setSize(500, 515);
        this.setContentPane(getJContentPane());
        this.setTitle("Module Build Description Header");
        initFrame();
    }

    /**
     This method initializes this
     Fill values to all fields if these values are not empty
     
     @param inMbdHeader The input MbdHeaderDocument.MbdHeader
     
     **/
    private void init(MbdHeaderDocument.MbdHeader inMbdHeader) {
        init();
        setMbdHeader(inMbdHeader);
        if (inMbdHeader != null) {
            if (this.mbdHeader.getBaseName() != null) {
                this.jTextFieldBaseName.setText(this.mbdHeader.getBaseName().getStringValue());
            }
            if (this.mbdHeader.getGuid() != null) {
                this.jTextFieldGuid.setText(this.mbdHeader.getGuid().getStringValue());
            }
            if (this.mbdHeader.getVersion() != null) {
                this.jTextFieldVersion.setText(this.mbdHeader.getVersion());
            }
            if (this.mbdHeader.getLicense() != null) {
                this.jTextAreaLicense.setText(this.mbdHeader.getLicense().getStringValue());
            }
            if (this.mbdHeader.getCopyright() != null) {
                this.jTextAreaCopyright.setText(this.mbdHeader.getCopyright());
            }
            if (this.mbdHeader.getDescription() != null) {
                this.jTextAreaDescription.setText(this.mbdHeader.getDescription());
            }
        }
    }

    /**
     Disable all components when the mode is view
     
     @param isView true - The view mode; false - The non-view mode
     
     **/
    public void setViewMode(boolean isView) {
        this.jButtonOk.setVisible(false);
        this.jButtonCancel.setVisible(false);
        if (isView) {
            this.jTextFieldBaseName.setEnabled(!isView);
            this.jTextFieldGuid.setEnabled(!isView);
            this.jTextFieldVersion.setEnabled(!isView);
            this.jTextAreaLicense.setEnabled(!isView);
            this.jTextAreaCopyright.setEnabled(!isView);
            this.jTextAreaDescription.setEnabled(!isView);
            this.jButtonCancel.setEnabled(!isView);
            this.jButtonGenerateGuid.setEnabled(!isView);
            this.jButtonOk.setEnabled(!isView);
        }
    }

    /**
     This method initializes jContentPane
     
     @return javax.swing.JPanel jContentPane
     
     **/
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jLabelDescription = new JLabel();
            jLabelDescription.setText("Description");
            jLabelDescription.setBounds(new java.awt.Rectangle(15, 255, 140, 20));
            jLabelCopyright = new JLabel();
            jLabelCopyright.setText("Copyright");
            jLabelCopyright.setBounds(new java.awt.Rectangle(15, 170, 140, 20));
            jLabelLicense = new JLabel();
            jLabelLicense.setText("License");
            jLabelLicense.setBounds(new java.awt.Rectangle(15, 85, 140, 20));
            jLabelVersion = new JLabel();
            jLabelVersion.setText("Version");
            jLabelVersion.setBounds(new java.awt.Rectangle(15, 60, 140, 20));
            jLabelGuid = new JLabel();
            jLabelGuid.setPreferredSize(new java.awt.Dimension(25, 15));
            jLabelGuid.setBounds(new java.awt.Rectangle(15, 35, 140, 20));
            jLabelGuid.setText("Guid");
            jLabelBaseName = new JLabel();
            jLabelBaseName.setText("Base Name");
            jLabelBaseName.setBounds(new java.awt.Rectangle(15, 10, 140, 20));
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.setLocation(new java.awt.Point(0, 0));
            jContentPane.setSize(new java.awt.Dimension(500, 524));
            jContentPane.add(jLabelBaseName, null);
            jContentPane.add(getJTextFieldBaseName(), null);
            jContentPane.add(jLabelGuid, null);
            jContentPane.add(getJTextFieldGuid(), null);
            jContentPane.add(jLabelVersion, null);
            jContentPane.add(getJTextFieldVersion(), null);
            jContentPane.add(getJButtonGenerateGuid(), null);
            jContentPane.add(jLabelLicense, null);
            jContentPane.add(jLabelCopyright, null);
            jContentPane.add(jLabelDescription, null);
            jContentPane.add(getJButtonOk(), null);
            jContentPane.add(getJButtonCancel(), null);
            jContentPane.add(getJScrollPaneLicense(), null);
            jContentPane.add(getJScrollPaneCopyright(), null);
            jContentPane.add(getJScrollPaneDescription(), null);

            jStarLabel1 = new StarLabel();
            jStarLabel1.setLocation(new java.awt.Point(0, 10));
            jStarLabel2 = new StarLabel();
            jStarLabel2.setLocation(new java.awt.Point(0, 35));
            jStarLabel3 = new StarLabel();
            jStarLabel3.setLocation(new java.awt.Point(0, 60));
            jStarLabel4 = new StarLabel();
            jStarLabel4.setLocation(new java.awt.Point(0, 85));
            jStarLabel5 = new StarLabel();
            jStarLabel5.setLocation(new java.awt.Point(0, 170));
            jStarLabel6 = new StarLabel();
            jStarLabel6.setLocation(new java.awt.Point(0, 255));

            jContentPane.add(jStarLabel1, null);
            jContentPane.add(jStarLabel2, null);
            jContentPane.add(jStarLabel3, null);
            jContentPane.add(jStarLabel4, null);
            jContentPane.add(jStarLabel5, null);
            jContentPane.add(jStarLabel6, null);
        }
        return jContentPane;
    }

    /* (non-Javadoc)
     * @see java.awt.event.ActionListener#actionPerformed(java.awt.event.ActionEvent)
     *
     * Override actionPerformed to listen all actions
     * 
     */
    public void actionPerformed(ActionEvent arg0) {
        if (arg0.getSource() == jButtonOk) {
            this.dispose();
            this.save();
            this.setEdited(true);
        }
        if (arg0.getSource() == jButtonCancel) {
            this.dispose();
            this.setEdited(false);
        }
        //
        // Generate GUID
        //
        if (arg0.getSource() == jButtonGenerateGuid) {
            jTextFieldGuid.setText(Tools.generateUuidString());
        }
    }

    /**
     Data validation for all fields
     
     @retval true - All datas are valid
     @retval false - At least one data is invalid
     
     **/
    public boolean check() {
        //
        // Check if all required fields are not empty
        //
        if (isEmpty(this.jTextFieldBaseName.getText())) {
            Log.err("Base Name couldn't be empty");
            return false;
        }
        if (isEmpty(this.jTextFieldGuid.getText())) {
            Log.err("Guid couldn't be empty");
            return false;
        }
        if (isEmpty(this.jTextFieldVersion.getText())) {
            Log.err("Version couldn't be empty");
            return false;
        }
        if (isEmpty(this.jTextAreaLicense.getText())) {
            Log.err("License couldn't be empty");
            return false;
        }
        if (isEmpty(this.jTextAreaCopyright.getText())) {
            Log.err("Copyright couldn't be empty");
            return false;
        }
        if (isEmpty(this.jTextAreaDescription.getText())) {
            Log.err("Description couldn't be empty");
            return false;
        }

        //
        // Check if all fields have correct data types 
        //
        if (!DataValidation.isBaseName(this.jTextFieldBaseName.getText())) {
            Log.err("Incorrect data type for Base Name");
            return false;
        }
        if (!DataValidation.isGuid((this.jTextFieldGuid).getText())) {
            Log.err("Incorrect data type for Guid");
            return false;
        }
        if (!DataValidation.isCopyright(this.jTextAreaCopyright.getText())) {
            Log.err("Incorrect data type for Copyright");
            return false;
        }
        return true;
    }

    /**
     Save all components of Mbd Header
     if exists mbdHeader, set the value directly
     if not exists mbdHeader, new an instance first
     
     **/
    public void save() {
        try {
            if (this.mbdHeader == null) {
                mbdHeader = MbdHeaderDocument.MbdHeader.Factory.newInstance();
            }
            if (this.mbdHeader.getBaseName() != null) {
                this.mbdHeader.getBaseName().setStringValue(this.jTextFieldBaseName.getText());
            } else {
                BaseNameDocument.BaseName mBaseName = BaseNameDocument.BaseName.Factory.newInstance();
                mBaseName.setStringValue(this.jTextFieldBaseName.getText());
                this.mbdHeader.setBaseName(mBaseName);
            }

            if (this.mbdHeader.getGuid() != null) {
                this.mbdHeader.getGuid().setStringValue(this.jTextFieldGuid.getText());
            } else {
                GuidDocument.Guid mGuid = GuidDocument.Guid.Factory.newInstance();
                mGuid.setStringValue(this.jTextFieldGuid.getText());
                this.mbdHeader.setGuid(mGuid);
            }

            this.mbdHeader.setVersion(this.jTextFieldVersion.getText());

            if (this.mbdHeader.getLicense() != null) {
                this.mbdHeader.getLicense().setStringValue(this.jTextAreaLicense.getText());
            } else {
                LicenseDocument.License mLicense = LicenseDocument.License.Factory.newInstance();
                mLicense.setStringValue(this.jTextAreaLicense.getText());
                this.mbdHeader.setLicense(mLicense);
            }

            this.mbdHeader.setCopyright(this.jTextAreaCopyright.getText());
            this.mbdHeader.setDescription(this.jTextAreaDescription.getText());

            if (this.mbdHeader.getCreated() == null) {
                this.mbdHeader.setCreated(Tools.getCurrentDateTime());
            } else {
                this.mbdHeader.setModified(Tools.getCurrentDateTime());
            }

        } catch (Exception e) {
            Log.err("Save Module Buid Description", e.getMessage());
        }
    }

    /**
     This method initializes module type and compontent type
     
     **/
    private void initFrame() {

    }

    /**
     Get MbdHeaderDocument.MbdHeader
     
     @return MbdHeaderDocument.MbdHeader mbdHeader
     
     **/
    public MbdHeaderDocument.MbdHeader getMbdHeader() {
        return mbdHeader;
    }

    /**
     Set MbdHeaderDocument.MbdHeader
     
     @param mbdHeader The input MbdHeaderDocument.MbdHeader
     
     **/
    public void setMbdHeader(MbdHeaderDocument.MbdHeader mbdHeader) {
        this.mbdHeader = mbdHeader;
    }
}
