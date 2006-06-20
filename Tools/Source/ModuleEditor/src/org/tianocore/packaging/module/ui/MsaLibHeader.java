/** @file
 
 The file is used to create, update MsaLibHeader of a MSA file
 
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
import javax.swing.JComboBox;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;
import javax.swing.JTextField;

import org.tianocore.AbstractDocument;
import org.tianocore.BaseNameDocument;
import org.tianocore.FrameworkComponentTypes;
import org.tianocore.GuidDocument;
import org.tianocore.LicenseDocument;
import org.tianocore.ModuleTypeDef;
import org.tianocore.MsaLibHeaderDocument;
import org.tianocore.SpecificationDocument;
import org.tianocore.common.DataValidation;
import org.tianocore.common.Log;
import org.tianocore.common.Tools;
import org.tianocore.packaging.common.ui.IInternalFrame;
import org.tianocore.packaging.common.ui.StarLabel;

/**
 The class is used to create, update MsaLibHeader of a MSA file
 It extends IInternalFrame
 
 @since ModuleEditor 1.0

 **/
public class MsaLibHeader extends IInternalFrame {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = 5526729363068526262L;

    //
    //Define class members
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

    private JLabel jLabelDescription = null;

    private JTextArea jTextAreaDescription = null;

    private JLabel jLabelSpecification = null;

    private JTextField jTextFieldSpecification = null;

    private JButton jButtonOk = null;

    private JButton jButtonCancel = null;

    private JScrollPane jScrollPaneLicense = null;

    private JScrollPane jScrollPaneDescription = null;

    private JLabel jLabelAbstract = null;

    private JTextField jTextFieldAbstract = null;

    private JLabel jLabelModuleType = null;

    private JLabel jLabelCompontentType = null;

    private JComboBox jComboBoxCompontentType = null;

    private JComboBox jComboBoxModuleType = null;

    private StarLabel jStarLabel1 = null;

    private StarLabel jStarLabel2 = null;

    private StarLabel jStarLabel3 = null;

    private StarLabel jStarLabel4 = null;

    private StarLabel jStarLabel5 = null;

    private StarLabel jStarLabel6 = null;

    private StarLabel jStarLabel7 = null;

    private StarLabel jStarLabel8 = null;

    private StarLabel jStarLabel9 = null;

    private MsaLibHeaderDocument.MsaLibHeader msaLibHeader = null;

    private JLabel jLabelURL = null;

    private JTextField jTextFieldAbstractURL = null;

    private JTextField jTextFieldCopyright = null;

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
            jTextFieldGuid.setBounds(new java.awt.Rectangle(160, 35, 250, 20));
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
            jButtonGenerateGuid.setBounds(new java.awt.Rectangle(415, 35, 65, 20));
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
     This method initializes jTextFieldSpecification 
     
     @return javax.swing.JTextField jTextFieldSpecification
     
     **/
    private JTextField getJTextFieldSpecification() {
        if (jTextFieldSpecification == null) {
            jTextFieldSpecification = new JTextField();
            jTextFieldSpecification.setBounds(new java.awt.Rectangle(160, 280, 320, 20));
        }
        return jTextFieldSpecification;
    }

    /**
     This method initializes jButtonOk 
     
     @return javax.swing.JButton jButtonOk
     
     **/
    private JButton getJButtonOk() {
        if (jButtonOk == null) {
            jButtonOk = new JButton();
            jButtonOk.setText("OK");
            jButtonOk.setBounds(new java.awt.Rectangle(290, 445, 90, 20));
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
            jButtonCancel.setBounds(new java.awt.Rectangle(390, 445, 90, 20));
            jButtonCancel.addActionListener(this);
        }
        return jButtonCancel;
    }

    /**
     This method initializes jScrollPaneLicense 
     
     @return javax.swing.JScrollPane  jScrollPaneLicense
     
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
     This method initializes jScrollPaneDescription 
     
     @return javax.swing.JScrollPane jScrollPaneDescription
     
     **/
    private JScrollPane getJScrollPaneDescription() {
        if (jScrollPaneDescription == null) {
            jScrollPaneDescription = new JScrollPane();
            jScrollPaneDescription.setBounds(new java.awt.Rectangle(160, 195, 320, 80));
            jScrollPaneDescription.setHorizontalScrollBarPolicy(javax.swing.JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);
            jScrollPaneDescription.setViewportView(getJTextAreaDescription());
        }
        return jScrollPaneDescription;
    }

    /**
     This method initializes jTextFieldAbstract 
     
     @return javax.swing.JTextField jTextFieldAbstract
     
     **/
    private JTextField getJTextFieldAbstract() {
        if (jTextFieldAbstract == null) {
            jTextFieldAbstract = new JTextField();
            jTextFieldAbstract.setBounds(new java.awt.Rectangle(160, 305, 320, 20));
        }
        return jTextFieldAbstract;
    }

    /**
     This method initializes jComboBoxCompontentType 
     
     @return javax.swing.JComboBox jComboBoxCompontentType
     
     **/
    private JComboBox getJComboBoxCompontentType() {
        if (jComboBoxCompontentType == null) {
            jComboBoxCompontentType = new JComboBox();
            jComboBoxCompontentType.setBounds(new java.awt.Rectangle(160, 380, 320, 20));
        }
        return jComboBoxCompontentType;
    }

    /**
     This method initializes jComboBoxModuleType 
     
     @return javax.swing.JComboBox jComboBoxModuleType
     
     **/
    private JComboBox getJComboBoxModuleType() {
        if (jComboBoxModuleType == null) {
            jComboBoxModuleType = new JComboBox();
            jComboBoxModuleType.setBounds(new java.awt.Rectangle(160, 355, 320, 20));
        }
        return jComboBoxModuleType;
    }

    /**
     This method initializes jTextFieldAbstractURL 
     
     @return javax.swing.JTextField jTextFieldAbstractURL
     
     **/
    private JTextField getJTextFieldAbstractURL() {
        if (jTextFieldAbstractURL == null) {
            jTextFieldAbstractURL = new JTextField();
            jTextFieldAbstractURL.setBounds(new java.awt.Rectangle(160,330,320,20));
        }
        return jTextFieldAbstractURL;
    }

    /**
     * This method initializes jTextFieldCopyright	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldCopyright() {
        if (jTextFieldCopyright == null) {
            jTextFieldCopyright = new JTextField();
            jTextFieldCopyright.setBounds(new java.awt.Rectangle(160,170,320,20));
        }
        return jTextFieldCopyright;
    }

    public static void main(String[] args) {

    }

    /**
     This is the default constructor
     
     **/
    public MsaLibHeader() {
        super();
        init();
        this.setVisible(true);
    }

    /**
     This is the override edit constructor
     
     @param inMsaLibHeader The input data of MsaLibHeaderDocument.MsaLibHeader
     
     **/
    public MsaLibHeader(MsaLibHeaderDocument.MsaLibHeader inMsaLibHeader) {
        super();
        init(inMsaLibHeader);
        this.setVisible(true);
        this.setViewMode(false);
    }

    /**
     This method initializes this
     
     **/
    private void init() {
        this.setSize(500, 515);
        this.setContentPane(getJContentPane());
        this.setTitle("Library Module");
        initFrame();
    }

    /**
     This method initializes this
     Fill values to all fields if these values are not empty
     
     @param inMsaLibHeader The input data of MsaLibHeaderDocument.MsaLibHeader
     
     **/
    private void init(MsaLibHeaderDocument.MsaLibHeader inMsaLibHeader) {
        init();
        this.setMsaLibHeader(inMsaLibHeader);
        if (inMsaLibHeader != null) {
            if (this.msaLibHeader.getBaseName() != null) {
                this.jTextFieldBaseName.setText(this.msaLibHeader.getBaseName().getStringValue());
            }
            if (this.msaLibHeader.getGuid() != null) {
                this.jTextFieldGuid.setText(this.msaLibHeader.getGuid().getStringValue());
            }
            if (this.msaLibHeader.getVersion() != null) {
                this.jTextFieldVersion.setText(this.msaLibHeader.getVersion());
            }
            if (this.msaLibHeader.getLicense() != null) {
                this.jTextAreaLicense.setText(this.msaLibHeader.getLicense().getStringValue());
            }
            if (this.msaLibHeader.getCopyright() != null) {
                this.jTextFieldCopyright.setText(this.msaLibHeader.getCopyright());
            }
            if (this.msaLibHeader.getDescription() != null) {
                this.jTextAreaDescription.setText(this.msaLibHeader.getDescription());
            }
            if (this.msaLibHeader.getSpecification() != null) {
                this.jTextFieldSpecification.setText(this.msaLibHeader.getSpecification());
            }
            if (this.msaLibHeader.getAbstract() != null) {
                this.jTextFieldAbstract.setText(this.msaLibHeader.getAbstract());
            }
            if (this.msaLibHeader.getModuleType() != null) {
                this.jComboBoxModuleType.setSelectedItem(this.msaLibHeader.getModuleType().toString());
            }
            if (this.msaLibHeader.getComponentType() != null) {
                this.jComboBoxCompontentType.setSelectedItem(this.msaLibHeader.getComponentType().toString());
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
            this.jTextFieldCopyright.setEnabled(!isView);
            this.jTextAreaDescription.setEnabled(!isView);
            this.jTextFieldSpecification.setEnabled(!isView);
            this.jTextFieldAbstract.setEnabled(!isView);
            this.jTextFieldAbstractURL.setEnabled(!isView);
            this.jComboBoxModuleType.setEnabled(!isView);
            this.jComboBoxCompontentType.setEnabled(!isView);
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
            jLabelURL = new JLabel();
            jLabelURL.setBounds(new java.awt.Rectangle(15,330,140,20));
            jLabelURL.setText("URL");
            jLabelCompontentType = new JLabel();
            jLabelCompontentType.setBounds(new java.awt.Rectangle(15, 380, 140, 20));
            jLabelCompontentType.setText("Compontent Type");
            jLabelModuleType = new JLabel();
            jLabelModuleType.setBounds(new java.awt.Rectangle(15, 355, 140, 20));
            jLabelModuleType.setText("Module Type");
            jLabelAbstract = new JLabel();
            jLabelAbstract.setBounds(new java.awt.Rectangle(15, 305, 140, 20));
            jLabelAbstract.setText("Abstract");
            jLabelSpecification = new JLabel();
            jLabelSpecification.setText("Specification");
            jLabelSpecification.setBounds(new java.awt.Rectangle(15, 280, 140, 20));
            jLabelDescription = new JLabel();
            jLabelDescription.setText("Description");
            jLabelDescription.setBounds(new java.awt.Rectangle(15, 195, 140, 20));
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
            jContentPane.add(jLabelSpecification, null);
            jContentPane.add(getJTextFieldSpecification(), null);
            jContentPane.add(getJButtonOk(), null);
            jContentPane.add(getJButtonCancel(), null);
            jContentPane.add(getJScrollPaneLicense(), null);
            jContentPane.add(getJScrollPaneDescription(), null);
            jContentPane.add(jLabelAbstract, null);
            jContentPane.add(getJTextFieldAbstract(), null);
            jContentPane.add(jLabelModuleType, null);
            jContentPane.add(jLabelCompontentType, null);
            jContentPane.add(getJComboBoxCompontentType(), null);
            jContentPane.add(getJComboBoxModuleType(), null);

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
            jStarLabel6.setLocation(new java.awt.Point(0, 195));
            jStarLabel7 = new StarLabel();
            jStarLabel7.setLocation(new java.awt.Point(0, 305));
            jStarLabel8 = new StarLabel();
            jStarLabel8.setLocation(new java.awt.Point(0, 355));
            jStarLabel9 = new StarLabel();
            jStarLabel9.setLocation(new java.awt.Point(0, 380));

            jContentPane.add(jStarLabel1, null);
            jContentPane.add(jStarLabel2, null);
            jContentPane.add(jStarLabel3, null);
            jContentPane.add(jStarLabel4, null);
            jContentPane.add(jStarLabel5, null);
            jContentPane.add(jStarLabel6, null);
            jContentPane.add(jStarLabel7, null);
            jContentPane.add(jStarLabel8, null);
            jContentPane.add(jStarLabel9, null);
            jContentPane.add(jLabelURL, null);
            jContentPane.add(getJTextFieldAbstractURL(), null);
            jContentPane.add(getJTextFieldCopyright(), null);
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
        if (isEmpty(this.jTextFieldCopyright.getText())) {
            Log.err("Copyright couldn't be empty");
            return false;
        }
        if (isEmpty(this.jTextAreaDescription.getText())) {
            Log.err("Description couldn't be empty");
            return false;
        }
        if (isEmpty(this.jTextFieldAbstract.getText())) {
            Log.err("Abstract couldn't be empty");
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
        if (!DataValidation.isAbstract(this.jTextFieldAbstract.getText())) {
            Log.err("Incorrect data type for Abstract");
            return false;
        }
        if (!DataValidation.isCopyright(this.jTextFieldCopyright.getText())) {
            Log.err("Incorrect data type for Copyright");
            return false;
        }
        return true;
    }

    /**
     Save all components of Msa Lib Header
     if exists msaLibHeader, set the value directly
     if not exists msaLibHeader, new an instance first
     
     **/
    public void save() {
        try {
            if (this.msaLibHeader == null) {
                msaLibHeader = MsaLibHeaderDocument.MsaLibHeader.Factory.newInstance();
            }
            if (this.msaLibHeader.getBaseName() != null) {
                this.msaLibHeader.getBaseName().setStringValue(this.jTextFieldBaseName.getText());
            } else {
                BaseNameDocument.BaseName mBaseName = BaseNameDocument.BaseName.Factory.newInstance();
                mBaseName.setStringValue(this.jTextFieldBaseName.getText());
                this.msaLibHeader.setBaseName(mBaseName);
            }

            if (this.msaLibHeader.getGuid() != null) {
                this.msaLibHeader.getGuid().setStringValue(this.jTextFieldGuid.getText());
            } else {
                GuidDocument.Guid mGuid = GuidDocument.Guid.Factory.newInstance();
                mGuid.setStringValue(this.jTextFieldGuid.getText());
                this.msaLibHeader.setGuid(mGuid);
            }

            this.msaLibHeader.setVersion(this.jTextFieldVersion.getText());

            if (this.msaLibHeader.getLicense() != null) {
                this.msaLibHeader.getLicense().setStringValue(this.jTextAreaLicense.getText());
            } else {
                LicenseDocument.License mLicense = LicenseDocument.License.Factory.newInstance();
                mLicense.setStringValue(this.jTextAreaLicense.getText());
                this.msaLibHeader.setLicense(mLicense);
            }

            this.msaLibHeader.setCopyright(this.jTextFieldCopyright.getText());
            this.msaLibHeader.setDescription(this.jTextAreaDescription.getText());

            if (this.msaLibHeader.getSpecification() != null) {
                this.msaLibHeader.setSpecification(this.jTextFieldSpecification.getText());
            } else {
                this.msaLibHeader.setSpecification(this.jTextFieldSpecification.getText());
            }

            if (this.msaLibHeader.getAbstract() != null) {
                this.msaLibHeader.setAbstract(this.jTextFieldAbstract.getText());
            } else {
                
                this.msaLibHeader.setAbstract(this.jTextFieldAbstract.getText());
            }

            this.msaLibHeader.setModuleType(ModuleTypeDef.Enum.forString(this.jComboBoxModuleType.getSelectedItem()
                                                                                                 .toString()));
            this.msaLibHeader
                             .setComponentType(FrameworkComponentTypes.Enum
                                                                           .forString(this.jComboBoxCompontentType
                                                                                                                  .getSelectedItem()
                                                                                                                  .toString()));

            if (this.msaLibHeader.getCreated() == null) {
                this.msaLibHeader.setCreated(Tools.getCurrentDateTime());
            } else {
                this.msaLibHeader.setUpdated(Tools.getCurrentDateTime());
            }
        } catch (Exception e) {
            Log.err("Save Module", e.getMessage());
        }
    }

    /**
     This method initializes module type and compontent type
     
     **/
    private void initFrame() {
        jComboBoxModuleType.addItem("BASE");
        jComboBoxModuleType.addItem("SEC");
        jComboBoxModuleType.addItem("PEI_CORE");
        jComboBoxModuleType.addItem("PEIM");
        jComboBoxModuleType.addItem("DXE_CORE");
        jComboBoxModuleType.addItem("DXE_DRIVER");
        jComboBoxModuleType.addItem("DXE_RUNTIME_DRIVER");
        jComboBoxModuleType.addItem("DXE_SMM_DRIVER");
        jComboBoxModuleType.addItem("DXE_SAL_DRIVER");
        jComboBoxModuleType.addItem("UEFI_DRIVER");
        jComboBoxModuleType.addItem("UEFI_APPLICATION");

        jComboBoxCompontentType.addItem("APRIORI");
        jComboBoxCompontentType.addItem("LIBRARY");
        jComboBoxCompontentType.addItem("FV_IMAGE_FILE");
        jComboBoxCompontentType.addItem("BS_DRIVER");
        jComboBoxCompontentType.addItem("RT_DRIVER");
        jComboBoxCompontentType.addItem("SAL_RT_DRIVER");
        jComboBoxCompontentType.addItem("PE32_PEIM");
        jComboBoxCompontentType.addItem("PIC_PEIM");
        jComboBoxCompontentType.addItem("COMBINED_PEIM_DRIVER");
        jComboBoxCompontentType.addItem("PEI_CORE");
        jComboBoxCompontentType.addItem("DXE_CORE");
        jComboBoxCompontentType.addItem("APPLICATION");
        jComboBoxCompontentType.addItem("BS_DRIVER_EFI");
        jComboBoxCompontentType.addItem("SHELLAPP");
    }

    /**
     Get MsaLibHeaderDocument.MsaLibHeader
     
     @return MsaLibHeaderDocument.MsaLibHeader
     
     **/
    public MsaLibHeaderDocument.MsaLibHeader getMsaLibHeader() {
        return msaLibHeader;
    }

    /**
     Set MsaLibHeaderDocument.MsaLibHeader
     
     @param msaLibHeader The input data of MsaLibHeaderDocument.MsaLibHeader
     
     **/
    public void setMsaLibHeader(MsaLibHeaderDocument.MsaLibHeader msaLibHeader) {
        this.msaLibHeader = msaLibHeader;
    }

}
