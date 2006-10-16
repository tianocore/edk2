/** @file
 
 The file is used to create, update Externs section of the MSA file
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/
package org.tianocore.frameworkwizard.module.ui.dialog;

import java.awt.event.ActionEvent;
import java.awt.event.ItemEvent;
import java.awt.event.ItemListener;
import java.util.Vector;

import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextField;

import org.tianocore.frameworkwizard.common.DataType;
import org.tianocore.frameworkwizard.common.DataValidation;
import org.tianocore.frameworkwizard.common.EnumerationData;
import org.tianocore.frameworkwizard.common.Log;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.ui.ArchCheckBox;
import org.tianocore.frameworkwizard.common.ui.IDialog;
import org.tianocore.frameworkwizard.common.ui.IFrame;
import org.tianocore.frameworkwizard.common.ui.StarLabel;
import org.tianocore.frameworkwizard.module.Identifications.ModuleIdentification;
import org.tianocore.frameworkwizard.module.Identifications.Externs.ExternsIdentification;
import org.tianocore.frameworkwizard.workspace.WorkspaceTools;

/**
 * The class is used to create, update Externs section of the MSA file
 * 
 * It extends IDialog
 * 
 */
public class ExternsDlg extends IDialog implements ItemListener {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = -7382008402932047191L;

    //
    //Define class members
    //
    private JPanel jContentPane = null;

    private JLabel jLabelName = null;

    private JComboBox jComboBoxType = null;

    private JScrollPane jScrollPane = null;

    private JLabel jLabelSpecification = null;

    private JTextField jTextFieldSpecification = null;

    private JLabel jLabelFeatureFlag = null;

    private JLabel jLabelArch = null;

    private JTextField jTextFieldFeatureFlag = null;

    private ArchCheckBox jArchCheckBox = null;

    private JButton jButtonOk = null;

    private JButton jButtonCancel = null;

    private StarLabel jStarLabel1 = null;

    private JPanel jPanelSpecification = null;

    private JPanel jPanelImage = null;

    private JPanel jPanelDriver = null;

    private JPanel jPanelLibrary = null;

    private JPanel jPanelCallBack = null;

    private JLabel jLabelModuleEntryPoint = null;

    private JTextField jTextFieldModuleEntryPoint = null;

    private JLabel jLabelModuleUnloadImage = null;

    private JTextField jTextFieldModuleUnloadImage = null;

    private JLabel jLabelDriverBinding = null;

    private JTextField jTextFieldDriverBinding = null;

    private JLabel jLabelComponentName = null;

    private JTextField jTextFieldComponentName = null;

    private JLabel jLabelDriverConfig = null;

    private JTextField jTextFieldDriverConfig = null;

    private JLabel jLabelDriverDiagnostic = null;

    private JTextField jTextFieldDriverDiagnostic = null;

    private JLabel jLabelConstructor = null;

    private JTextField jTextFieldConstructor = null;

    private JLabel jLabelDestructor = null;

    private JTextField jTextFieldDestructor = null;

    private JLabel jLabelVirtualAddressMap = null;

    private JTextField jTextFieldVirtualAddressMap = null;

    private JLabel jLabelExitBootServices = null;

    private JTextField jTextFieldExitBootServices = null;

    //
    // Not used by UI
    //
    private ExternsIdentification id = null;

    private EnumerationData ed = new EnumerationData();
    
    private WorkspaceTools wt = new WorkspaceTools();
    
    private Vector<String> vArchList = new Vector<String>();

    /**
     This method initializes jComboBoxType 
     
     @return javax.swing.JComboBox jComboBoxType
     
     **/
    private JComboBox getJComboBoxType() {
        if (jComboBoxType == null) {
            jComboBoxType = new JComboBox();
            jComboBoxType.setBounds(new java.awt.Rectangle(168, 12, 320, 20));
            jComboBoxType.setPreferredSize(new java.awt.Dimension(320, 20));
            jComboBoxType.addItemListener(this);
        }
        return jComboBoxType;
    }

    /**
     This method initializes jScrollPane  
     
     @return javax.swing.JScrollPane  
     */
    private JScrollPane getJScrollPane() {
        if (jScrollPane == null) {
            jScrollPane = new JScrollPane();
            jScrollPane.setViewportView(getJContentPane());
        }
        return jScrollPane;
    }

    /**
     This method initializes jTextFieldC_Name	
     
     @return javax.swing.JTextField	
     
     **/
    private JTextField getJTextFieldSpecification() {
        if (jTextFieldSpecification == null) {
            jTextFieldSpecification = new JTextField();
            jTextFieldSpecification.setPreferredSize(new java.awt.Dimension(320, 20));
            jTextFieldSpecification.setBounds(new java.awt.Rectangle(168, 0, 320, 20));
        }
        return jTextFieldSpecification;
    }

    /**
     This method initializes jTextFieldFeatureFlag    
     
     @return javax.swing.JTextField   
     
     **/
    private JTextField getJTextFieldFeatureFlag() {
        if (jTextFieldFeatureFlag == null) {
            jTextFieldFeatureFlag = new JTextField();
            jTextFieldFeatureFlag.setBounds(new java.awt.Rectangle(168, 87, 320, 20));
            jTextFieldFeatureFlag.setPreferredSize(new java.awt.Dimension(320, 20));
            jTextFieldFeatureFlag.setEnabled(false);
        }
        return jTextFieldFeatureFlag;
    }

    /**
     This method initializes jButtonOk    
     
     @return javax.swing.JButton  
     
     **/
    private JButton getJButtonOk() {
        if (jButtonOk == null) {
            jButtonOk = new JButton();
            jButtonOk.setBounds(new java.awt.Rectangle(300, 187, 90, 20));
            jButtonOk.setText("Ok");
            jButtonOk.addActionListener(this);
        }
        return jButtonOk;
    }

    /**
     This method initializes jButtonCancel    
     
     @return javax.swing.JButton  
     
     **/
    private JButton getJButtonCancel() {
        if (jButtonCancel == null) {
            jButtonCancel = new JButton();
            jButtonCancel.setBounds(new java.awt.Rectangle(400, 187, 90, 20));
            jButtonCancel.setText("Cancel");
            jButtonCancel.addActionListener(this);
        }
        return jButtonCancel;
    }

    /**
     * This method initializes jPanelSpecification	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelSpecification() {
        if (jPanelSpecification == null) {
            jPanelSpecification = new JPanel();
            jPanelSpecification.setBounds(new java.awt.Rectangle(0, 37, 505, 20));
            jPanelSpecification.setLayout(null);

            jLabelSpecification = new JLabel();
            jLabelSpecification.setBounds(new java.awt.Rectangle(12, 0, 140, 20));
            jLabelSpecification.setText("Specification Name");

            jPanelSpecification.add(jLabelSpecification, null);
            jPanelSpecification.add(getJTextFieldSpecification(), null);
        }
        return jPanelSpecification;
    }

    /**
     * This method initializes jPanelImage	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelImage() {
        if (jPanelImage == null) {
            jLabelModuleUnloadImage = new JLabel();
            jLabelModuleUnloadImage.setBounds(new java.awt.Rectangle(12, 25, 140, 20));
            jLabelModuleUnloadImage.setText("Module Unload Image");
            jLabelModuleEntryPoint = new JLabel();
            jLabelModuleEntryPoint.setBounds(new java.awt.Rectangle(12, 0, 140, 20));
            jLabelModuleEntryPoint.setText("Module Entry Point");
            jPanelImage = new JPanel();
            jPanelImage.setBounds(new java.awt.Rectangle(0, 37, 505, 45));
            jPanelImage.setLayout(null);
            jPanelImage.add(jLabelModuleEntryPoint, null);
            jPanelImage.add(getJTextFieldModuleEntryPoint(), null);
            jPanelImage.add(jLabelModuleUnloadImage, null);
            jPanelImage.add(getJTextFieldModuleUnloadImage(), null);
        }
        return jPanelImage;
    }

    /**
     * This method initializes jPanelDriver	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelDriver() {
        if (jPanelDriver == null) {
            jStarLabel1 = new StarLabel();
            jStarLabel1.setLocation(new java.awt.Point(0, 0));
            jLabelDriverDiagnostic = new JLabel();
            jLabelDriverDiagnostic.setBounds(new java.awt.Rectangle(12, 75, 140, 20));
            jLabelDriverDiagnostic.setText("Driver Diagnostic");
            jLabelDriverConfig = new JLabel();
            jLabelDriverConfig.setBounds(new java.awt.Rectangle(12, 50, 140, 20));
            jLabelDriverConfig.setText("Driver Config");
            jLabelComponentName = new JLabel();
            jLabelComponentName.setBounds(new java.awt.Rectangle(12, 25, 140, 20));
            jLabelComponentName.setText("Component Name");
            jLabelDriverBinding = new JLabel();
            jLabelDriverBinding.setBounds(new java.awt.Rectangle(12, 0, 140, 20));
            jLabelDriverBinding.setText("Driver Binding");
            jPanelDriver = new JPanel();
            jPanelDriver.setBounds(new java.awt.Rectangle(0, 37, 505, 95));
            jPanelDriver.setLayout(null);
            jPanelDriver.add(jStarLabel1, null);
            jPanelDriver.add(jLabelDriverBinding, null);
            jPanelDriver.add(getJTextFieldDriverBinding(), null);
            jPanelDriver.add(jLabelComponentName, null);
            jPanelDriver.add(getJTextFieldComponentName(), null);
            jPanelDriver.add(jLabelDriverConfig, null);
            jPanelDriver.add(getJTextFieldDriverConfig(), null);
            jPanelDriver.add(jLabelDriverDiagnostic, null);
            jPanelDriver.add(getJTextFieldDriverDiagnostic(), null);
        }
        return jPanelDriver;
    }

    /**
     * This method initializes jPanelLibrary	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelLibrary() {
        if (jPanelLibrary == null) {
            jLabelDestructor = new JLabel();
            jLabelDestructor.setBounds(new java.awt.Rectangle(12, 25, 140, 20));
            jLabelDestructor.setText("Destructor");
            jLabelConstructor = new JLabel();
            jLabelConstructor.setBounds(new java.awt.Rectangle(12, 0, 140, 20));
            jLabelConstructor.setText("Constructor");
            jPanelLibrary = new JPanel();
            jPanelLibrary.setBounds(new java.awt.Rectangle(0, 37, 505, 45));
            jPanelLibrary.setLayout(null);
            jPanelLibrary.add(jLabelConstructor, null);
            jPanelLibrary.add(getJTextFieldConstructor(), null);
            jPanelLibrary.add(jLabelDestructor, null);
            jPanelLibrary.add(getJTextFieldDestructor(), null);
        }
        return jPanelLibrary;
    }

    /**
     * This method initializes jPanelCallBack	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelCallBack() {
        if (jPanelCallBack == null) {
            jLabelExitBootServices = new JLabel();
            jLabelExitBootServices.setBounds(new java.awt.Rectangle(12, 25, 140, 20));
            jLabelExitBootServices.setText("Exit Boot Services");
            jLabelVirtualAddressMap = new JLabel();
            jLabelVirtualAddressMap.setBounds(new java.awt.Rectangle(12, 0, 140, 20));
            jLabelVirtualAddressMap.setText("Virtual Address Map");
            jPanelCallBack = new JPanel();
            jPanelCallBack.setBounds(new java.awt.Rectangle(0, 37, 505, 45));
            jPanelCallBack.setLayout(null);
            jPanelCallBack.add(jLabelVirtualAddressMap, null);
            jPanelCallBack.add(getJTextFieldVirtualAddressMap(), null);
            jPanelCallBack.add(jLabelExitBootServices, null);
            jPanelCallBack.add(getJTextFieldExitBootServices(), null);
        }
        return jPanelCallBack;
    }

    /**
     * This method initializes jTextFieldModuleEntryPoint	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldModuleEntryPoint() {
        if (jTextFieldModuleEntryPoint == null) {
            jTextFieldModuleEntryPoint = new JTextField();
            jTextFieldModuleEntryPoint.setBounds(new java.awt.Rectangle(168, 0, 320, 20));
        }
        return jTextFieldModuleEntryPoint;
    }

    /**
     * This method initializes jTextFieldModuleUnloadImage	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldModuleUnloadImage() {
        if (jTextFieldModuleUnloadImage == null) {
            jTextFieldModuleUnloadImage = new JTextField();
            jTextFieldModuleUnloadImage.setBounds(new java.awt.Rectangle(168, 25, 320, 20));
        }
        return jTextFieldModuleUnloadImage;
    }

    /**
     * This method initializes jTextFieldDriverBinding	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldDriverBinding() {
        if (jTextFieldDriverBinding == null) {
            jTextFieldDriverBinding = new JTextField();
            jTextFieldDriverBinding.setBounds(new java.awt.Rectangle(168, 0, 320, 20));
        }
        return jTextFieldDriverBinding;
    }

    /**
     * This method initializes jTextFieldComponentName	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldComponentName() {
        if (jTextFieldComponentName == null) {
            jTextFieldComponentName = new JTextField();
            jTextFieldComponentName.setBounds(new java.awt.Rectangle(168, 25, 320, 20));
        }
        return jTextFieldComponentName;
    }

    /**
     * This method initializes jTextFieldDriverConfig	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldDriverConfig() {
        if (jTextFieldDriverConfig == null) {
            jTextFieldDriverConfig = new JTextField();
            jTextFieldDriverConfig.setBounds(new java.awt.Rectangle(168, 50, 320, 20));
        }
        return jTextFieldDriverConfig;
    }

    /**
     * This method initializes jTextFieldDriverDiagnostic	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldDriverDiagnostic() {
        if (jTextFieldDriverDiagnostic == null) {
            jTextFieldDriverDiagnostic = new JTextField();
            jTextFieldDriverDiagnostic.setBounds(new java.awt.Rectangle(168, 75, 320, 20));
        }
        return jTextFieldDriverDiagnostic;
    }

    /**
     * This method initializes jTextFieldConstructor	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldConstructor() {
        if (jTextFieldConstructor == null) {
            jTextFieldConstructor = new JTextField();
            jTextFieldConstructor.setBounds(new java.awt.Rectangle(168, 0, 320, 20));
        }
        return jTextFieldConstructor;
    }

    /**
     * This method initializes jTextFieldDestructor	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldDestructor() {
        if (jTextFieldDestructor == null) {
            jTextFieldDestructor = new JTextField();
            jTextFieldDestructor.setBounds(new java.awt.Rectangle(168, 25, 320, 20));
        }
        return jTextFieldDestructor;
    }

    /**
     * This method initializes jTextFieldVirtualAddressMap	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldVirtualAddressMap() {
        if (jTextFieldVirtualAddressMap == null) {
            jTextFieldVirtualAddressMap = new JTextField();
            jTextFieldVirtualAddressMap.setBounds(new java.awt.Rectangle(168, 0, 320, 20));
        }
        return jTextFieldVirtualAddressMap;
    }

    /**
     * This method initializes jTextFieldExitBootServices	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldExitBootServices() {
        if (jTextFieldExitBootServices == null) {
            jTextFieldExitBootServices = new JTextField();
            jTextFieldExitBootServices.setBounds(new java.awt.Rectangle(168, 25, 320, 20));
        }
        return jTextFieldExitBootServices;
    }

    public static void main(String[] args) {

    }

    /**
     This method initializes this
     
     **/
    private void init() {
        this.setSize(515, 250);
        this.setContentPane(getJScrollPane());
        this.setTitle("Externs");
        initFrame();
        this.centerWindow();
    }

    /**
     This method initializes this
     Fill values to all fields if these values are not empty
     
     @param inExternsId

     **/
    private void init(ExternsIdentification inExternsId, ModuleIdentification mid) {
        init();
        this.id = inExternsId;
        
        //
        // Init arch with module's arch
        //
        this.vArchList = wt.getModuleArch(mid);

        if (this.id != null) {
            String type = id.getType();
            //
            // Filter the type to lock down the type
            //
            this.jComboBoxType.removeAllItems();
            this.jComboBoxType.addItem(type);

            this.jTextFieldFeatureFlag.setText(id.getFeatureFlag());
            this.jArchCheckBox.setSelectedItems(id.getSupArchList());

            //
            // Init specification
            //
            if (type.equals(EnumerationData.EXTERNS_SPECIFICATION)) {
                this.jTextFieldSpecification.setText(id.getName0());
            }

            //
            // Init image
            //
            if (type.equals(EnumerationData.EXTERNS_IMAGE)) {
                this.jTextFieldModuleEntryPoint.setText(id.getName0());
                this.jTextFieldModuleUnloadImage.setText(id.getName1());
            }

            //
            // Init library
            //
            if (type.equals(EnumerationData.EXTERNS_LIBRARY)) {
                this.jTextFieldConstructor.setText(id.getName0());
                this.jTextFieldDestructor.setText(id.getName1());
            }

            //
            // Init driver
            //
            if (type.equals(EnumerationData.EXTERNS_DRIVER)) {
                this.jTextFieldDriverBinding.setText(id.getName0());
                this.jTextFieldComponentName.setText(id.getName1());
                this.jTextFieldDriverConfig.setText(id.getName2());
                this.jTextFieldDriverDiagnostic.setText(id.getName3());
            }

            //
            // Init library
            //
            if (type.equals(EnumerationData.EXTERNS_CALL_BACK)) {
                this.jTextFieldVirtualAddressMap.setText(id.getName0());
                this.jTextFieldExitBootServices.setText(id.getName1());
            }
        }
    }

    /**
     This is the override edit constructor
     
     @param inBootModesIdentification
     @param iFrame
     
     **/
    public ExternsDlg(ExternsIdentification inExternsIdentification, IFrame iFrame, ModuleIdentification mid) {
        super(iFrame, true);
        init(inExternsIdentification, mid);
    }

    /**
     This method initializes jContentPane
     
     @return javax.swing.JPanel jContentPane
     
     **/
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jArchCheckBox = new ArchCheckBox();
            jArchCheckBox.setBounds(new java.awt.Rectangle(168, 62, 320, 20));
            jArchCheckBox.setPreferredSize(new java.awt.Dimension(320, 20));

            jLabelName = new JLabel();
            jLabelName.setText("Choose Extern Type");
            jLabelName.setBounds(new java.awt.Rectangle(12, 12, 155, 20));
            jLabelArch = new JLabel();
            jLabelArch.setBounds(new java.awt.Rectangle(12, 62, 155, 20));
            jLabelArch.setText("Supported Architectures");
            jLabelFeatureFlag = new JLabel();
            jLabelFeatureFlag.setBounds(new java.awt.Rectangle(12, 87, 155, 20));
            jLabelFeatureFlag.setText("Feature Flag Expression");
            jLabelFeatureFlag.setEnabled(false);

            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.setPreferredSize(new java.awt.Dimension(505, 200));

            jContentPane.add(jLabelName, null);
            jContentPane.add(getJComboBoxType(), null);
            jContentPane.add(jLabelFeatureFlag, null);
            jContentPane.add(jLabelArch, null);
            jContentPane.add(getJTextFieldFeatureFlag(), null);

            jContentPane.add(jArchCheckBox, null);
            jContentPane.add(getJButtonOk(), null);
            jContentPane.add(getJButtonCancel(), null);

            jContentPane.add(getJPanelSpecification(), null);
            jContentPane.add(getJPanelImage(), null);
            jContentPane.add(getJPanelDriver(), null);
            jContentPane.add(getJPanelLibrary(), null);
            jContentPane.add(getJPanelCallBack(), null);
        }
        return jContentPane;
    }

    /**
     This method initializes Usage type and Externs type
     
     **/
    private void initFrame() {
        Tools.generateComboBoxByVector(this.jComboBoxType, ed.getVExternTypes());
        this.jPanelSpecification.setVisible(true);
        this.jPanelImage.setVisible(false);
        this.jPanelLibrary.setVisible(false);
        this.jPanelDriver.setVisible(false);
        this.jPanelCallBack.setVisible(false);
    }

    /* (non-Javadoc)
     * @see java.awt.event.ActionListener#actionPerformed(java.awt.event.ActionEvent)
     *
     * Override actionPerformed to listen all actions
     * 
     */
    public void actionPerformed(ActionEvent arg0) {
        if (arg0.getSource() == jButtonOk) {
            if (checkAdd()) {
                this.getCurrentExterns();
                this.returnType = DataType.RETURN_TYPE_OK;
                this.setVisible(false);
            }
        }

        if (arg0.getSource() == jButtonCancel) {
            this.returnType = DataType.RETURN_TYPE_CANCEL;
            this.setVisible(false);
        }
    }

    /**
     Data validation for all fields
     
     @retval true - All datas are valid
     @retval false - At least one data is invalid
     
     **/
    public boolean checkAdd() {
        //
        // Check if all fields have correct data types 
        //

        //
        // Check specification
        //
        if (this.jComboBoxType.getSelectedItem().toString().equals(EnumerationData.EXTERNS_SPECIFICATION)) {
            if (isEmpty(this.jTextFieldSpecification.getText())) {
                Log.wrn("Update Externs", "Please type specification's name");
                return false;
            }
            if (!DataValidation.isSentence(this.jTextFieldSpecification.getText())) {
                Log.wrn("Update Externs", "Incorrect data type for Specification");
                return false;
            }

            //
            // No need to check feature flag, return true directly here.
            //
            return true;
        }

        //
        // Check image
        //
        if (this.jComboBoxType.getSelectedItem().toString().equals(EnumerationData.EXTERNS_IMAGE)) {
            if (isEmpty(this.jTextFieldModuleEntryPoint.getText())
                && isEmpty(this.jTextFieldModuleUnloadImage.getText())) {
                Log.wrn("Update Externs", "At least one of ModuleEntryPoint or ModuleUnloadImage should have a value");
                return false;
            }
            if (!isEmpty(this.jTextFieldModuleEntryPoint.getText())) {
                if (!DataValidation.isC_NameType(this.jTextFieldModuleEntryPoint.getText())) {
                    Log.wrn("Update Externs", "Incorrect data type for Module Entry Point");
                    return false;
                }
            }
            if (!isEmpty(this.jTextFieldModuleUnloadImage.getText())) {
                if (!DataValidation.isC_NameType(this.jTextFieldModuleUnloadImage.getText())) {
                    Log.wrn("Update Externs", "Incorrect data type for Module Unload Image");
                    return false;
                }
            }
        }

        //
        // Check library
        //
        if (this.jComboBoxType.getSelectedItem().toString().equals(EnumerationData.EXTERNS_LIBRARY)) {
            if (isEmpty(this.jTextFieldConstructor.getText()) && isEmpty(this.jTextFieldDestructor.getText())) {
                Log.wrn("Update Externs", "At least one of Constructor or Destructor should have a value");
                return false;
            }
            if (isEmpty(this.jTextFieldConstructor.getText()) && !isEmpty(this.jTextFieldDestructor.getText())) {
                Log.wrn("Update Externs", "You must define a Constructor at the same time when you declare a Destructor");
                return false;
            }
            if (!isEmpty(this.jTextFieldConstructor.getText())) {
                if (!DataValidation.isC_NameType(this.jTextFieldConstructor.getText())) {
                    Log.wrn("Update Externs", "Incorrect data type for Constructor");
                    return false;
                }
            }
            if (!isEmpty(this.jTextFieldDestructor.getText())) {
                if (!DataValidation.isC_NameType(this.jTextFieldDestructor.getText())) {
                    Log.wrn("Update Externs", "Incorrect data type for Destructor");
                    return false;
                }
            }
        }

        //
        // Check driver
        //
        if (this.jComboBoxType.getSelectedItem().toString().equals(EnumerationData.EXTERNS_DRIVER)) {
            if (isEmpty(this.jTextFieldDriverBinding.getText())) {
                Log.wrn("Update Externs", "DriverBinding must have a value");
                return false;
            }
            if (!isEmpty(this.jTextFieldDriverBinding.getText())) {
                if (!DataValidation.isC_NameType(this.jTextFieldDriverBinding.getText())) {
                    Log.wrn("Update Externs", "Incorrect data type for DriverBinding");
                    return false;
                }
            }
            if (!isEmpty(this.jTextFieldComponentName.getText())) {
                if (!DataValidation.isC_NameType(this.jTextFieldComponentName.getText())) {
                    Log.wrn("Update Externs", "Incorrect data type for ComponentName");
                    return false;
                }
            }
            if (!isEmpty(this.jTextFieldDriverConfig.getText())) {
                if (!DataValidation.isC_NameType(this.jTextFieldDriverConfig.getText())) {
                    Log.wrn("Update Externs", "Incorrect data type for DriverConfig");
                    return false;
                }
            }
            if (!isEmpty(this.jTextFieldDriverDiagnostic.getText())) {
                if (!DataValidation.isC_NameType(this.jTextFieldDriverDiagnostic.getText())) {
                    Log.wrn("Update Externs", "Incorrect data type for DriverDiagnostic");
                    return false;
                }
            }
        }

        //
        // Check call back
        //
        if (this.jComboBoxType.getSelectedItem().toString().equals(EnumerationData.EXTERNS_CALL_BACK)) {
            if (isEmpty(this.jTextFieldVirtualAddressMap.getText())
                && isEmpty(this.jTextFieldExitBootServices.getText())) {
                Log.wrn("Update Externs", "At least one of VirtualAddressMap or ExitBootServices should have a value");
                return false;
            }
            if (!isEmpty(this.jTextFieldVirtualAddressMap.getText())) {
                if (!DataValidation.isC_NameType(this.jTextFieldVirtualAddressMap.getText())) {
                    Log.wrn("Update Externs", "Incorrect data type for Virtual Address Map");
                    return false;
                }
            }
            if (!isEmpty(this.jTextFieldExitBootServices.getText())) {
                if (!DataValidation.isC_NameType(this.jTextFieldExitBootServices.getText())) {
                    Log.wrn("Update Externs", "Incorrect data type for Exit Boot Services");
                    return false;
                }
            }
        }

        //
        // Check FeatureFlag
        //
        if (!isEmpty(this.jTextFieldFeatureFlag.getText())) {
            if (!DataValidation.isFeatureFlag(this.jTextFieldFeatureFlag.getText())) {
                Log.wrn("Update Externs", "Incorrect data type for Feature Flag");
                return false;
            }
        }

        return true;
    }

    private ExternsIdentification getCurrentExterns() {
        String arg0 = "";
        String arg1 = "";
        String arg2 = "";
        String arg3 = "";

        String type = this.jComboBoxType.getSelectedItem().toString();
        String featureFlag = this.jTextFieldFeatureFlag.getText();
        Vector<String> arch = this.jArchCheckBox.getSelectedItemsVector();

        if (type.equals(EnumerationData.EXTERNS_SPECIFICATION)) {
            arg0 = this.jTextFieldSpecification.getText();
            id = new ExternsIdentification(arg0, type);
        }

        if (type.equals(EnumerationData.EXTERNS_IMAGE)) {
            arg0 = this.jTextFieldModuleEntryPoint.getText();
            arg1 = this.jTextFieldModuleUnloadImage.getText();
            id = new ExternsIdentification(arg0, arg1, type, featureFlag, arch);
        }

        if (type.equals(EnumerationData.EXTERNS_LIBRARY)) {
            arg0 = this.jTextFieldConstructor.getText();
            arg1 = this.jTextFieldDestructor.getText();
            id = new ExternsIdentification(arg0, arg1, type, featureFlag, arch);
        }

        if (type.equals(EnumerationData.EXTERNS_DRIVER)) {
            arg0 = this.jTextFieldDriverBinding.getText();
            arg1 = this.jTextFieldComponentName.getText();
            arg2 = this.jTextFieldDriverConfig.getText();
            arg3 = this.jTextFieldDriverDiagnostic.getText();
            id = new ExternsIdentification(arg0, arg1, arg2, arg3, type, featureFlag, arch);
        }

        if (type.equals(EnumerationData.EXTERNS_CALL_BACK)) {
            arg0 = this.jTextFieldVirtualAddressMap.getText();
            arg1 = this.jTextFieldExitBootServices.getText();
            id = new ExternsIdentification(arg0, arg1, type, featureFlag, arch);
        }

        return id;
    }

    /* (non-Javadoc)
     * @see java.awt.event.ItemListener#itemStateChanged(java.awt.event.ItemEvent)
     *
     * Reflesh the frame when selected item changed
     * 
     */
    public void itemStateChanged(ItemEvent arg0) {
        if (arg0.getSource() == jComboBoxType && arg0.getStateChange() == ItemEvent.SELECTED) {
            if (jComboBoxType.getSelectedItem().toString().equals(EnumerationData.EXTERNS_SPECIFICATION)) {
                this.jPanelSpecification.setVisible(true);
                this.jPanelImage.setVisible(false);
                this.jPanelLibrary.setVisible(false);
                this.jPanelDriver.setVisible(false);
                this.jPanelCallBack.setVisible(false);
                this.jLabelArch.setEnabled(false);
                this.jArchCheckBox.setAllItemsEnabled(false);
                this.jLabelArch.setLocation(12, 62);
                this.jArchCheckBox.setLocation(168, 62);
                this.jLabelFeatureFlag.setLocation(12, 87);
                this.jTextFieldFeatureFlag.setLocation(168, 87);
            } else if (jComboBoxType.getSelectedItem().toString().equals(EnumerationData.EXTERNS_IMAGE)) {
                this.jPanelSpecification.setVisible(false);
                this.jPanelImage.setVisible(true);
                this.jPanelLibrary.setVisible(false);
                this.jPanelDriver.setVisible(false);
                this.jPanelCallBack.setVisible(false);
                this.jLabelArch.setEnabled(true);
                this.jArchCheckBox.setEnabledItems(this.vArchList);
                this.jLabelArch.setLocation(12, 87);
                this.jArchCheckBox.setLocation(168, 87);
                this.jLabelFeatureFlag.setLocation(12, 112);
                this.jTextFieldFeatureFlag.setLocation(168, 112);
            } else if (jComboBoxType.getSelectedItem().toString().equals(EnumerationData.EXTERNS_LIBRARY)) {
                this.jPanelSpecification.setVisible(false);
                this.jPanelImage.setVisible(false);
                this.jPanelLibrary.setVisible(true);
                this.jPanelDriver.setVisible(false);
                this.jPanelCallBack.setVisible(false);
                this.jLabelArch.setEnabled(true);
                this.jArchCheckBox.setEnabledItems(this.vArchList);
                this.jLabelArch.setLocation(12, 87);
                this.jArchCheckBox.setLocation(168, 87);
                this.jLabelFeatureFlag.setLocation(12, 112);
                this.jTextFieldFeatureFlag.setLocation(168, 112);
            } else if (jComboBoxType.getSelectedItem().toString().equals(EnumerationData.EXTERNS_DRIVER)) {
                this.jPanelSpecification.setVisible(false);
                this.jPanelImage.setVisible(false);
                this.jPanelLibrary.setVisible(false);
                this.jPanelDriver.setVisible(true);
                this.jPanelCallBack.setVisible(false);
                this.jLabelArch.setEnabled(true);
                this.jArchCheckBox.setEnabledItems(this.vArchList);
                this.jLabelArch.setLocation(12, 137);
                this.jArchCheckBox.setLocation(168, 137);
                this.jLabelFeatureFlag.setLocation(12, 162);
                this.jTextFieldFeatureFlag.setLocation(168, 162);
            } else if (jComboBoxType.getSelectedItem().toString().equals(EnumerationData.EXTERNS_CALL_BACK)) {
                this.jPanelSpecification.setVisible(false);
                this.jPanelImage.setVisible(false);
                this.jPanelLibrary.setVisible(false);
                this.jPanelDriver.setVisible(false);
                this.jPanelCallBack.setVisible(true);
                this.jLabelArch.setEnabled(true);
                this.jArchCheckBox.setEnabledItems(this.vArchList);
                this.jLabelArch.setLocation(12, 87);
                this.jArchCheckBox.setLocation(168, 87);
                this.jLabelFeatureFlag.setLocation(12, 112);
                this.jTextFieldFeatureFlag.setLocation(168, 112);
            }
        }
    }

    public ExternsIdentification getId() {
        return id;
    }

    public void setId(ExternsIdentification id) {
        this.id = id;
    }
}
