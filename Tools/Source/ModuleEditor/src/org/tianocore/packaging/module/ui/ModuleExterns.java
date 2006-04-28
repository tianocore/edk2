/** @file
 
 The file is used to create, update DataHub of MSA/MBD file
 
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
import java.awt.event.ItemEvent;
import java.awt.event.ItemListener;

import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JTextField;

import org.tianocore.ExternUsage;
import org.tianocore.ExternsDocument;
import org.tianocore.common.DataValidation;
import org.tianocore.common.Log;
import org.tianocore.packaging.common.ui.IComboBox;
import org.tianocore.packaging.common.ui.IInternalFrame;

/**
 The class is used to create, update DataHub of MSA/MBD file 
 It extends IInternalFrame
 
 @since ModuleEditor 1.0

 **/
public class ModuleExterns extends IInternalFrame implements ItemListener {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = -7382008402932047191L;

    //
    //Define class members
    //
    private ExternsDocument.Externs externs = null;

    private int location = -1;

    private JPanel jContentPane = null;

    private JLabel jLabelName = null;

    private JLabel jLabelUsage = null;

    private JComboBox jComboBoxUsage = null;

    private JButton jButtonOk = null;

    private JButton jButtonCancel = null;

    private JLabel jLabelOverrideID = null;

    private JTextField jTextFieldOverrideID = null;

    private JPanel jPanelType1 = null;

    private JLabel jLabelModuleEntryPoint = null;

    private JLabel jLabelModuleUnloadImage = null;

    private IComboBox iComboBoxModuleEntryPoint = null;

    private IComboBox iComboBoxModuleUnloadImage = null;

    private JPanel jPanelType2 = null;

    private JLabel jLabelConstructor = null;

    private JTextField jTextFieldConstructor = null;

    private JLabel jLabelDestructor = null;

    private JTextField jTextFieldDestructor = null;

    private JComboBox jComboBoxType = null;

    private JPanel jPanelType3 = null;

    private JLabel jLabelDriverBinding = null;

    private JLabel jLabelComponentName = null;

    private IComboBox iComboBoxComponentName = null;

    private IComboBox iComboBoxDriverBinding = null;

    private JLabel jLabelDriverConfig = null;

    private JLabel jLabelDriverDiag = null;

    private IComboBox iComboBoxDriverDiag = null;

    private IComboBox iComboBoxDriverConfig = null;

    private JPanel jPanelType4 = null;

    private JLabel jLabelSetVirtualAddressMapCallBack = null;

    private IComboBox iComboBoxSetVirtualAddressMapCallBack = null;

    private JLabel jLabelExitBootServicesCallBack = null;

    private IComboBox iComboBoxExitBootServicesCallBack = null;

    private JPanel jPanelType5 = null;

    private JLabel jLabelUserDefined = null;

    private IComboBox iComboBoxUserDefined = null;

    /**
     This method initializes jComboBoxUsage 
     
     @return javax.swing.JComboBox jComboBoxUsage
     
     **/
    private JComboBox getJComboBoxUsage() {
        if (jComboBoxUsage == null) {
            jComboBoxUsage = new JComboBox();
            jComboBoxUsage.setBounds(new java.awt.Rectangle(160, 35, 320, 20));
        }
        return jComboBoxUsage;
    }

    /**
     This method initializes jButtonOk 
     
     @return javax.swing.JButton jButtonOk
     
     **/
    private JButton getJButtonOk() {
        if (jButtonOk == null) {
            jButtonOk = new JButton();
            jButtonOk.setText("OK");
            jButtonOk.setLocation(new java.awt.Point(290, 215));
            jButtonOk.setSize(new java.awt.Dimension(90, 20));
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
            jButtonCancel.setLocation(new java.awt.Point(390, 215));
            jButtonCancel.setSize(new java.awt.Dimension(90, 20));
            jButtonCancel.addActionListener(this);
        }
        return jButtonCancel;
    }

    /**
     This method initializes jTextFieldOverrideID 
     
     @return javax.swing.JTextField jTextFieldOverrideID
     
     **/
    private JTextField getJTextFieldOverrideID() {
        if (jTextFieldOverrideID == null) {
            jTextFieldOverrideID = new JTextField();
            jTextFieldOverrideID.setBounds(new java.awt.Rectangle(160, 60, 50, 20));
        }
        return jTextFieldOverrideID;
    }

    /**
     This method initializes jPanelType1 
     
     @return javax.swing.JPanel jPanelType1
     
     **/
    private JPanel getJPanelType1() {
        if (jPanelType1 == null) {
            jLabelModuleUnloadImage = new JLabel();
            jLabelModuleUnloadImage.setBounds(new java.awt.Rectangle(15, 30, 140, 20));
            jLabelModuleUnloadImage.setText("Module Unload Image");
            jLabelModuleEntryPoint = new JLabel();
            jLabelModuleEntryPoint.setBounds(new java.awt.Rectangle(15, 5, 140, 20));
            jLabelModuleEntryPoint.setText("Module Entry Point");
            jPanelType1 = new JPanel();
            jPanelType1.setLayout(null);
            jPanelType1.setBounds(new java.awt.Rectangle(0, 105, 490, 55));
            jPanelType1.add(jLabelModuleEntryPoint, null);
            jPanelType1.add(jLabelModuleUnloadImage, null);
            jPanelType1.add(getIComboBoxModuleUnloadImage(), null);
            jPanelType1.add(getIComboBoxModuleEntryPoint(), null);
        }
        return jPanelType1;
    }

    /**
     This method initializes jComboBoxModuleEntryPoint 
     
     @return javax.swing.JComboBox iComboBoxModuleEntryPoint
     
     **/
    private IComboBox getIComboBoxModuleEntryPoint() {
        if (iComboBoxModuleEntryPoint == null) {
            iComboBoxModuleEntryPoint = new IComboBox();
            iComboBoxModuleEntryPoint.setBounds(new java.awt.Rectangle(160, 5, 320, 20));
        }
        return iComboBoxModuleEntryPoint;
    }

    /**
     This method initializes jComboBoxModuleUnloadImage 
     
     @return javax.swing.JComboBox iComboBoxModuleUnloadImage
     
     **/
    private IComboBox getIComboBoxModuleUnloadImage() {
        if (iComboBoxModuleUnloadImage == null) {
            iComboBoxModuleUnloadImage = new IComboBox();
            iComboBoxModuleUnloadImage.setBounds(new java.awt.Rectangle(160, 30, 320, 20));
        }
        return iComboBoxModuleUnloadImage;
    }

    /**
     This method initializes jPanelType2 
     
     @return javax.swing.JPanel jPanelType2
     
     **/
    private JPanel getJPanelType2() {
        if (jPanelType2 == null) {
            jLabelDestructor = new JLabel();
            jLabelDestructor.setBounds(new java.awt.Rectangle(15, 30, 140, 20));
            jLabelDestructor.setText("Destructor");
            jLabelConstructor = new JLabel();
            jLabelConstructor.setBounds(new java.awt.Rectangle(15, 5, 140, 20));
            jLabelConstructor.setText("Constructor");
            jPanelType2 = new JPanel();
            jPanelType2.setLayout(null);
            jPanelType2.setBounds(new java.awt.Rectangle(0, 105, 490, 55));
            jPanelType2.add(jLabelConstructor, null);
            jPanelType2.add(getJTextFieldConstructor(), null);
            jPanelType2.add(jLabelDestructor, null);
            jPanelType2.add(getJTextFieldDestructor(), null);
        }
        return jPanelType2;
    }

    /**
     This method initializes jTextFieldConstructor 
     
     @return javax.swing.JTextField jTextFieldConstructor
     
     **/
    private JTextField getJTextFieldConstructor() {
        if (jTextFieldConstructor == null) {
            jTextFieldConstructor = new JTextField();
            jTextFieldConstructor.setBounds(new java.awt.Rectangle(160, 5, 320, 20));
        }
        return jTextFieldConstructor;
    }

    /**
     This method initializes jTextFieldDestructor 
     
     @return javax.swing.JTextField jTextFieldDestructor
     
     **/
    private JTextField getJTextFieldDestructor() {
        if (jTextFieldDestructor == null) {
            jTextFieldDestructor = new JTextField();
            jTextFieldDestructor.setBounds(new java.awt.Rectangle(160, 30, 320, 20));
        }
        return jTextFieldDestructor;
    }

    /**
     This method initializes jComboBoxType 
     
     @return javax.swing.JComboBox jComboBoxType
     
     **/
    private JComboBox getJComboBoxType() {
        if (jComboBoxType == null) {
            jComboBoxType = new JComboBox();
            jComboBoxType.setBounds(new java.awt.Rectangle(160, 10, 320, 20));
            jComboBoxType.addItemListener(this);
        }
        return jComboBoxType;
    }

    /**
     This method initializes jPanelType3 
     
     @return javax.swing.JPanel jPanelType3
     
     **/
    private JPanel getJPanelType3() {
        if (jPanelType3 == null) {
            jLabelDriverDiag = new JLabel();
            jLabelDriverDiag.setBounds(new java.awt.Rectangle(15, 80, 140, 20));
            jLabelDriverDiag.setText("Driver Diag");
            jLabelDriverConfig = new JLabel();
            jLabelDriverConfig.setBounds(new java.awt.Rectangle(15, 55, 140, 20));
            jLabelDriverConfig.setText("Driver Config");
            jLabelComponentName = new JLabel();
            jLabelComponentName.setBounds(new java.awt.Rectangle(15, 30, 140, 20));
            jLabelComponentName.setText("Component Name");
            jLabelDriverBinding = new JLabel();
            jLabelDriverBinding.setBounds(new java.awt.Rectangle(15, 5, 140, 20));
            jLabelDriverBinding.setText("Driver Binding");
            jPanelType3 = new JPanel();
            jPanelType3.setLayout(null);
            jPanelType3.setBounds(new java.awt.Rectangle(0, 105, 490, 105));
            jPanelType3.add(jLabelDriverBinding, null);
            jPanelType3.add(jLabelComponentName, null);
            jPanelType3.add(getIComboBoxComponentName(), null);
            jPanelType3.add(getIComboBoxDriverBinding(), null);
            jPanelType3.add(jLabelDriverConfig, null);
            jPanelType3.add(jLabelDriverDiag, null);
            jPanelType3.add(getIComboBoxDriverDiag(), null);
            jPanelType3.add(getIComboBoxDriverConfig(), null);
        }
        return jPanelType3;
    }

    /**
     This method initializes jComboBoxComponentName 
     
     @return javax.swing.JComboBox iComboBoxComponentName
     
     **/
    private IComboBox getIComboBoxComponentName() {
        if (iComboBoxComponentName == null) {
            iComboBoxComponentName = new IComboBox();
            iComboBoxComponentName.setBounds(new java.awt.Rectangle(160, 30, 320, 20));
        }
        return iComboBoxComponentName;
    }

    /**
     This method initializes jComboBoxDriverBinding 
     
     @return javax.swing.JComboBox iComboBoxDriverBinding
     
     **/
    private IComboBox getIComboBoxDriverBinding() {
        if (iComboBoxDriverBinding == null) {
            iComboBoxDriverBinding = new IComboBox();
            iComboBoxDriverBinding.setBounds(new java.awt.Rectangle(160, 5, 320, 20));
        }
        return iComboBoxDriverBinding;
    }

    /**
     This method initializes jComboBoxDriverDiag 
     
     @return javax.swing.JComboBox iComboBoxDriverDiag
     
     **/
    private IComboBox getIComboBoxDriverDiag() {
        if (iComboBoxDriverDiag == null) {
            iComboBoxDriverDiag = new IComboBox();
            iComboBoxDriverDiag.setBounds(new java.awt.Rectangle(160, 80, 320, 20));
        }
        return iComboBoxDriverDiag;
    }

    /**
     This method initializes jComboBoxDriverConfig 
     
     @return javax.swing.JComboBox iComboBoxDriverConfig
     
     */
    private IComboBox getIComboBoxDriverConfig() {
        if (iComboBoxDriverConfig == null) {
            iComboBoxDriverConfig = new IComboBox();
            iComboBoxDriverConfig.setBounds(new java.awt.Rectangle(160, 55, 320, 20));
        }
        return iComboBoxDriverConfig;
    }

    /**
     This method initializes jPanelType4 
     
     @return javax.swing.JPanel jPanelType4
     
     **/
    private JPanel getJPanelType4() {
        if (jPanelType4 == null) {
            jLabelExitBootServicesCallBack = new JLabel();
            jLabelExitBootServicesCallBack.setBounds(new java.awt.Rectangle(15, 30, 200, 20));
            jLabelExitBootServicesCallBack.setText("Exit Boot Services Call Back");
            jLabelSetVirtualAddressMapCallBack = new JLabel();
            jLabelSetVirtualAddressMapCallBack.setBounds(new java.awt.Rectangle(15, 5, 200, 20));
            jLabelSetVirtualAddressMapCallBack.setText("Set Virtual Address Map Call Back");
            jPanelType4 = new JPanel();
            jPanelType4.setLayout(null);
            jPanelType4.setBounds(new java.awt.Rectangle(0, 105, 490, 55));
            jPanelType4.add(jLabelSetVirtualAddressMapCallBack, null);
            jPanelType4.add(getIComboBoxSetVirtualAddressMapCallBack(), null);
            jPanelType4.add(jLabelExitBootServicesCallBack, null);
            jPanelType4.add(getIComboBoxExitBootServicesCallBack(), null);
        }
        return jPanelType4;
    }

    /**
     This method initializes jComboBoxSetVirtualAddressMapCallBack 
     
     @return javax.swing.JComboBox iComboBoxSetVirtualAddressMapCallBack
     
     **/
    private IComboBox getIComboBoxSetVirtualAddressMapCallBack() {
        if (iComboBoxSetVirtualAddressMapCallBack == null) {
            iComboBoxSetVirtualAddressMapCallBack = new IComboBox();
            iComboBoxSetVirtualAddressMapCallBack.setBounds(new java.awt.Rectangle(220, 5, 260, 20));
        }
        return iComboBoxSetVirtualAddressMapCallBack;
    }

    /**
     This method initializes jComboBoxExitBootServicesCallBack 
     
     @return javax.swing.JComboBox iComboBoxExitBootServicesCallBack
     
     **/
    private IComboBox getIComboBoxExitBootServicesCallBack() {
        if (iComboBoxExitBootServicesCallBack == null) {
            iComboBoxExitBootServicesCallBack = new IComboBox();
            iComboBoxExitBootServicesCallBack.setBounds(new java.awt.Rectangle(220, 30, 260, 20));
        }
        return iComboBoxExitBootServicesCallBack;
    }

    /**
     This method initializes jPanelType5 
     
     @return javax.swing.JPanel jPanelType5
     
     **/
    private JPanel getJPanelType5() {
        if (jPanelType5 == null) {
            jLabelUserDefined = new JLabel();
            jLabelUserDefined.setBounds(new java.awt.Rectangle(15, 5, 140, 20));
            jLabelUserDefined.setText("User Defined");
            jPanelType5 = new JPanel();
            jPanelType5.setLayout(null);
            jPanelType5.setBounds(new java.awt.Rectangle(0, 105, 490, 30));
            jPanelType5.add(jLabelUserDefined, null);
            jPanelType5.add(getIComboBoxUserDefined(), null);
        }
        return jPanelType5;
    }

    /**
     This method initializes jComboBoxUserDefined 
     
     @return javax.swing.JComboBox iComboBoxUserDefined
     
     **/
    private IComboBox getIComboBoxUserDefined() {
        if (iComboBoxUserDefined == null) {
            iComboBoxUserDefined = new IComboBox();
            iComboBoxUserDefined.setBounds(new java.awt.Rectangle(160, 5, 320, 20));
        }
        return iComboBoxUserDefined;
    }

    public static void main(String[] args) {

    }

    /**
     This is the default constructor
     
     **/
    public ModuleExterns() {
        super();
        init();
        this.setVisible(true);
    }

    /**
     This is the override edit constructor
     
     @param inExterns The input data of ExternsDocument.Externs
     
     **/
    public ModuleExterns(ExternsDocument.Externs inExterns) {
        super();
        init(inExterns);
        this.setVisible(true);
    }

    /**
     This is the override edit constructor
     
     @param inExterns The input data of ExternsDocument.Externs
     @param type The input data of node type
     @param index The input data of node index
     
     **/
    public ModuleExterns(ExternsDocument.Externs inExterns, int type, int index) {
        super();
        init(inExterns, type, index);
        this.setVisible(true);
    }

    /**
     This method initializes this
     
     @param inExterns The input data of ExternsDocument.Externs
     
     **/
    private void init(ExternsDocument.Externs inExterns) {
        init();
        this.setExterns(inExterns);
    }

    /**
     This method initializes this
     Fill values to all fields if these values are not empty
     
     @param inExterns The input data of ExternsDocument.Externs
     @param type The input data of node type
     @param index The input data of node index
     
     **/
    private void init(ExternsDocument.Externs inExterns, int type, int index) {
        init(inExterns);
        this.location = index;
        if (this.externs.getExternList().size() > 0) {
            //
            //Get common fields
            //
            if (this.externs.getExternArray(index).getUsage() != null) {
                this.jComboBoxUsage.setSelectedItem(this.externs.getExternArray(index).getUsage().toString());
            }
            this.jTextFieldOverrideID.setText(String.valueOf(this.externs.getExternArray(index).getOverrideID()));
            //
            //Type 1
            //
            if (this.externs.getExternArray(index).getModuleEntryPointList().size() > 0) {
                this.jComboBoxType.setSelectedIndex(0);
                for (int indexI = 0; indexI < this.externs.getExternArray(index).getModuleEntryPointList().size(); indexI++) {
                    this.iComboBoxModuleEntryPoint.addItem(this.externs.getExternArray(index)
                                                                       .getModuleEntryPointArray(indexI));
                }
            }
            if (this.externs.getExternArray(index).getModuleUnloadImageList().size() > 0) {
                this.jComboBoxType.setSelectedIndex(0);
                for (int indexI = 0; indexI < this.externs.getExternArray(index).getModuleUnloadImageList().size(); indexI++) {
                    this.iComboBoxModuleUnloadImage.addItem(this.externs.getExternArray(index)
                                                                        .getModuleUnloadImageArray(indexI));
                }
            }

            //
            //Type 2
            //
            if (this.externs.getExternArray(index).getConstructor() != null) {
                this.jComboBoxType.setSelectedIndex(1);
                this.jTextFieldConstructor.setText(this.externs.getExternArray(index).getConstructor());
            }
            if (this.externs.getExternArray(index).getDestructor() != null) {
                this.jComboBoxType.setSelectedIndex(1);
                this.jTextFieldDestructor.setText(this.externs.getExternArray(index).getDestructor());
            }

            //
            //Type 3
            //
            if (this.externs.getExternArray(index).getDriverBindingList().size() > 0) {
                this.jComboBoxType.setSelectedIndex(2);
                for (int indexI = 0; indexI < this.externs.getExternArray(index).getDriverBindingList().size(); indexI++) {
                    this.iComboBoxDriverBinding.addItem(this.externs.getExternArray(index)
                                                                    .getDriverBindingArray(indexI));
                }
            }
            if (this.externs.getExternArray(index).getComponentNameList().size() > 0) {
                this.jComboBoxType.setSelectedIndex(2);
                for (int indexI = 0; indexI < this.externs.getExternArray(index).getComponentNameList().size(); indexI++) {
                    this.iComboBoxComponentName.addItem(this.externs.getExternArray(index)
                                                                    .getComponentNameArray(indexI));
                }
            }
            if (this.externs.getExternArray(index).getDriverConfigList().size() > 0) {
                this.jComboBoxType.setSelectedIndex(2);
                for (int indexI = 0; indexI < this.externs.getExternArray(index).getDriverConfigList().size(); indexI++) {
                    this.iComboBoxDriverConfig.addItem(this.externs.getExternArray(index).getDriverConfigArray(indexI));
                }
            }
            if (this.externs.getExternArray(index).getDriverDiagList().size() > 0) {
                this.jComboBoxType.setSelectedIndex(2);
                for (int indexI = 0; indexI < this.externs.getExternArray(index).getDriverDiagList().size(); indexI++) {
                    this.iComboBoxDriverDiag.addItem(this.externs.getExternArray(index).getDriverDiagArray(indexI));
                }
            }

            //
            //Type 4
            //
            if (this.externs.getExternArray(index).getSetVirtualAddressMapCallBackList().size() > 0) {
                this.jComboBoxType.setSelectedIndex(3);
                for (int indexI = 0; indexI < this.externs.getExternArray(index).getSetVirtualAddressMapCallBackList()
                                                          .size(); indexI++) {
                    this.iComboBoxSetVirtualAddressMapCallBack
                                                              .addItem(this.externs
                                                                                   .getExternArray(index)
                                                                                   .getSetVirtualAddressMapCallBackArray(
                                                                                                                         indexI));
                }
            }
            if (this.externs.getExternArray(index).getExitBootServicesCallBackList().size() > 0) {
                this.jComboBoxType.setSelectedIndex(3);
                for (int indexI = 0; indexI < this.externs.getExternArray(index).getExitBootServicesCallBackList()
                                                          .size(); indexI++) {
                    this.iComboBoxExitBootServicesCallBack
                                                          .addItem(this.externs
                                                                               .getExternArray(index)
                                                                               .getExitBootServicesCallBackArray(indexI));
                }
            }

            //
            //Type 5
            //
            if (this.externs.getExternArray(index).getUserDefinedList().size() > 0) {
                this.jComboBoxType.setSelectedIndex(4);
                for (int indexI = 0; indexI < this.externs.getExternArray(index).getUserDefinedList().size(); indexI++) {
                    this.iComboBoxUserDefined.addItem(this.externs.getExternArray(index).getUserDefinedArray(indexI));
                }
            }

            this.jComboBoxType.setEnabled(false);
            switchType();
        }
    }

    /**
     This method initializes this
     
     **/
    private void init() {
        this.setSize(500, 515);
        this.setContentPane(getJContentPane());
        this.setTitle("Externs");
        initFrame();
        this.setViewMode(false);
    }

    /**
     Disable all components when the mode is view
     
     @param isView true - The view mode; false - The non-view mode
     
     **/
    public void setViewMode(boolean isView) {
        this.jButtonOk.setVisible(false);
        this.jButtonCancel.setVisible(false);
        if (isView) {
            this.jComboBoxUsage.setEnabled(!isView);
            this.jTextFieldOverrideID.setEnabled(!isView);
            //
            //Type 1
            //
            this.iComboBoxModuleEntryPoint.setEnabled(!isView);
            this.iComboBoxModuleUnloadImage.setEnabled(!isView);

            //
            //Type 2
            //
            this.jTextFieldConstructor.setEnabled(!isView);
            this.jTextFieldDestructor.setEnabled(!isView);

            //
            //Type 3
            //
            this.iComboBoxDriverBinding.setEnabled(!isView);
            this.iComboBoxComponentName.setEnabled(!isView);
            this.iComboBoxDriverConfig.setEnabled(!isView);
            this.iComboBoxDriverDiag.setEnabled(!isView);

            //
            //Type 4
            //
            this.iComboBoxSetVirtualAddressMapCallBack.setEnabled(!isView);
            this.iComboBoxExitBootServicesCallBack.setEnabled(!isView);

            //
            //Type 5
            //
            this.iComboBoxUserDefined.setEnabled(!isView);

            this.jComboBoxType.setEnabled(!isView);
            this.jButtonCancel.setEnabled(!isView);
            this.jButtonOk.setEnabled(!isView);
        }
    }

    /**
     This method initializes jContentPane
     
     @return javax.swing.JPanel jContentPane
     
     **/
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jLabelOverrideID = new JLabel();
            jLabelOverrideID.setBounds(new java.awt.Rectangle(15, 60, 140, 20));
            jLabelOverrideID.setText("Override ID");
            jLabelUsage = new JLabel();
            jLabelUsage.setText("Usage");
            jLabelUsage.setBounds(new java.awt.Rectangle(15, 35, 140, 20));
            jLabelName = new JLabel();
            jLabelName.setText("Choose Type");
            jLabelName.setBounds(new java.awt.Rectangle(15, 10, 140, 20));
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.setSize(new java.awt.Dimension(490, 244));
            jContentPane.add(getJPanelType2(), null);
            jContentPane.add(jLabelName, null);
            jContentPane.add(getJComboBoxType(), null);
            jContentPane.add(getJPanelType3(), null);
            jContentPane.add(getJPanelType4(), null);
            jContentPane.add(getJPanelType5(), null);
            jContentPane.add(jLabelUsage, null);
            jContentPane.add(getJComboBoxUsage(), null);
            jContentPane.add(getJButtonOk(), null);
            jContentPane.add(getJButtonCancel(), null);
            jContentPane.add(jLabelOverrideID, null);
            jContentPane.add(getJTextFieldOverrideID(), null);

            jContentPane.add(getJPanelType1(), null);
        }
        return jContentPane;
    }

    /**
     This method initializes Usage type and Externs type
     
     **/
    private void initFrame() {
        jComboBoxUsage.addItem("ALWAYS_CONSUMED");
        jComboBoxUsage.addItem("ALWAYS_PRODUCED");

        jComboBoxType.addItem("Entry/Unload");
        jComboBoxType.addItem("Library");
        jComboBoxType.addItem("Driver Bindings");
        jComboBoxType.addItem("Call Backs");
        jComboBoxType.addItem("Other");

        jPanelType1.setVisible(true);
        jPanelType2.setVisible(false);
        jPanelType3.setVisible(false);
        jPanelType4.setVisible(false);
        jPanelType5.setVisible(false);
    }

    /* (non-Javadoc)
     * @see java.awt.event.ActionListener#actionPerformed(java.awt.event.ActionEvent)
     *
     * Override actionPerformed to listen all actions
     * 
     */
    public void actionPerformed(ActionEvent arg0) {
        if (arg0.getSource() == jButtonOk) {
            this.setEdited(true);
            this.save();
            this.dispose();
        }

        if (arg0.getSource() == jButtonCancel) {
            this.dispose();
        }
    }

    public void itemStateChanged(ItemEvent arg0) {
        if (arg0.getSource() == jComboBoxType) {
            if (arg0.getStateChange() == ItemEvent.SELECTED) {
                switchType();
            }
        }
    }

    /**
     Show/Hide relevant fields via select different types
     
     **/
    private void switchType() {
        if (jComboBoxType.getSelectedIndex() == 0) {
            jPanelType1.setVisible(true);
            jPanelType2.setVisible(false);
            jPanelType3.setVisible(false);
            jPanelType4.setVisible(false);
            jPanelType5.setVisible(false);
        }
        if (jComboBoxType.getSelectedIndex() == 1) {
            jPanelType1.setVisible(false);
            jPanelType2.setVisible(true);
            jPanelType3.setVisible(false);
            jPanelType4.setVisible(false);
            jPanelType5.setVisible(false);
        }
        if (jComboBoxType.getSelectedIndex() == 2) {
            jPanelType1.setVisible(false);
            jPanelType2.setVisible(false);
            jPanelType3.setVisible(true);
            jPanelType4.setVisible(false);
            jPanelType5.setVisible(false);
        }
        if (jComboBoxType.getSelectedIndex() == 3) {
            jPanelType1.setVisible(false);
            jPanelType2.setVisible(false);
            jPanelType3.setVisible(false);
            jPanelType4.setVisible(true);
            jPanelType5.setVisible(false);
        }
        if (jComboBoxType.getSelectedIndex() == 4) {
            jPanelType1.setVisible(false);
            jPanelType2.setVisible(false);
            jPanelType3.setVisible(false);
            jPanelType4.setVisible(false);
            jPanelType5.setVisible(true);
        }
    }

    /**
     Set ExternsDocument.Externs
     
     @return ExternsDocument.Externs
     
     
     **/
    public ExternsDocument.Externs getExterns() {
        return externs;
    }

    /**
     Get ExternsDocument.Externs
     
     @param externs The input ExternsDocument.Externs
     
     **/
    public void setExterns(ExternsDocument.Externs externs) {
        this.externs = externs;
    }

    /**
     Data validation for all fields
     
     @retval true - All datas are valid
     @retval false - At least one data is invalid
     
     **/
    public boolean check() {
        //
        // Check if all fields have correct data types 
        //
        if (this.jComboBoxType.getSelectedIndex() == 1) {
            if (!isEmpty(this.jTextFieldConstructor.getText())
                && !DataValidation.isConstructor(this.jTextFieldConstructor.getText())) {
                Log.err("Incorrect data type for Constructor");
                return false;
            }
            if (!isEmpty(this.jTextFieldDestructor.getText())
                && !DataValidation.isDestructor(this.jTextFieldDestructor.getText())) {
                Log.err("Incorrect data type for Destructor");
                return false;
            }
        }

        if (!isEmpty(this.jTextFieldOverrideID.getText())
            && !DataValidation.isOverrideID(this.jTextFieldOverrideID.getText())) {
            Log.err("Incorrect data type for Override ID");
            return false;
        }

        return true;
    }

    /**
     Save all components of Externs
     if exists externs, set the value directly
     if not exists externs, new an instance first
     
     **/
    public void save() {
        try {
            if (this.externs == null) {
                externs = ExternsDocument.Externs.Factory.newInstance();
            }
            ExternsDocument.Externs.Extern extern = ExternsDocument.Externs.Extern.Factory.newInstance();
            //
            //Save common fields
            //
            extern.setUsage(ExternUsage.Enum.forString(jComboBoxUsage.getSelectedItem().toString()));
            if (!isEmpty(this.jTextFieldOverrideID.getText())) {
                extern.setOverrideID(Integer.parseInt(this.jTextFieldOverrideID.getText()));
            }

            //
            //Save type 1
            //
            if (this.jComboBoxType.getSelectedIndex() == 0) {
                if (this.iComboBoxModuleEntryPoint.getItemCount() > 0) {
                    for (int index = 0; index < this.iComboBoxModuleEntryPoint.getItemCount(); index++) {
                        extern.addNewModuleEntryPoint();
                        extern.setModuleEntryPointArray(index, this.iComboBoxModuleEntryPoint.getItemAt(index)
                                                                                             .toString());
                    }
                }
                if (this.iComboBoxModuleEntryPoint.getItemCount() > 0) {
                    for (int index = 0; index < this.iComboBoxModuleUnloadImage.getItemCount(); index++) {
                        extern.addNewModuleUnloadImage();
                        extern.setModuleUnloadImageArray(index, this.iComboBoxModuleUnloadImage.getItemAt(index)
                                                                                               .toString());
                    }
                }
            }

            //
            //Save type 2
            //
            if (this.jComboBoxType.getSelectedIndex() == 1) {
                if (!isEmpty(this.jTextFieldConstructor.getText())) {
                    extern.setConstructor(this.jTextFieldConstructor.getText());
                }
                if (!isEmpty(this.jTextFieldDestructor.getText())) {
                    extern.setDestructor(this.jTextFieldDestructor.getText());
                }
            }

            //
            //Save type 3
            //
            if (this.jComboBoxType.getSelectedIndex() == 2) {
                if (this.iComboBoxDriverBinding.getItemCount() > 0) {
                    for (int index = 0; index < this.iComboBoxDriverBinding.getItemCount(); index++) {
                        extern.addNewDriverBinding();
                        extern.setDriverBindingArray(index, this.iComboBoxDriverBinding.getItemAt(index).toString());
                    }
                }
                if (this.iComboBoxComponentName.getItemCount() > 0) {
                    for (int index = 0; index < this.iComboBoxComponentName.getItemCount(); index++) {
                        extern.addNewComponentName();
                        extern.setComponentNameArray(index, this.iComboBoxComponentName.getItemAt(index).toString());
                    }
                }
                if (this.iComboBoxDriverConfig.getItemCount() > 0) {
                    for (int index = 0; index < this.iComboBoxDriverConfig.getItemCount(); index++) {
                        extern.addNewDriverConfig();
                        extern.setDriverConfigArray(index, this.iComboBoxDriverConfig.getItemAt(index).toString());
                    }
                }
                if (this.iComboBoxDriverDiag.getItemCount() > 0) {
                    for (int index = 0; index < this.iComboBoxDriverDiag.getItemCount(); index++) {
                        extern.addNewDriverDiag();
                        extern.setDriverDiagArray(index, this.iComboBoxDriverDiag.getItemAt(index).toString());
                    }
                }
            }

            //
            //Save type 4
            //
            if (this.jComboBoxType.getSelectedIndex() == 3) {
                if (this.iComboBoxSetVirtualAddressMapCallBack.getItemCount() > 0) {
                    for (int index = 0; index < this.iComboBoxSetVirtualAddressMapCallBack.getItemCount(); index++) {
                        extern.addNewSetVirtualAddressMapCallBack();
                        extern
                              .setSetVirtualAddressMapCallBackArray(
                                                                    index,
                                                                    this.iComboBoxSetVirtualAddressMapCallBack
                                                                                                              .getItemAt(
                                                                                                                         index)
                                                                                                              .toString());
                    }
                }
                if (this.iComboBoxExitBootServicesCallBack.getItemCount() > 0) {
                    for (int index = 0; index < this.iComboBoxExitBootServicesCallBack.getItemCount(); index++) {
                        extern.addNewExitBootServicesCallBack();
                        extern.setExitBootServicesCallBackArray(index,
                                                                this.iComboBoxExitBootServicesCallBack.getItemAt(index)
                                                                                                      .toString());
                    }
                }
            }
            //
            //Save type 5
            //
            if (this.jComboBoxType.getSelectedIndex() == 4) {
                if (this.iComboBoxUserDefined.getItemCount() > 0) {
                    for (int index = 0; index < this.iComboBoxUserDefined.getItemCount(); index++) {
                        extern.addNewUserDefined();
                        extern.setUserDefinedArray(index, this.iComboBoxUserDefined.getItemAt(index).toString());
                    }
                }
            }

            if (location > -1) {
                externs.setExternArray(location, extern);
            } else {
                externs.addNewExtern();
                externs.setExternArray(externs.getExternList().size() - 1, extern);
            }
        } catch (Exception e) {
            Log.err("Update Externs", e.getMessage());
        }
    }
}
