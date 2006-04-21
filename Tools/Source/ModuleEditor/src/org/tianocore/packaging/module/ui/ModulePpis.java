/** @file
 
 The file is used to create, update Ppi of MSA/MBD file
 
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
import javax.swing.JRadioButton;
import javax.swing.JTextField;

import org.tianocore.PPIsDocument;
import org.tianocore.PpiNotifyUsage;
import org.tianocore.PpiUsage;
import org.tianocore.common.DataValidation;
import org.tianocore.common.Log;
import org.tianocore.common.Tools;
import org.tianocore.packaging.common.ui.IDefaultMutableTreeNode;
import org.tianocore.packaging.common.ui.IInternalFrame;
import org.tianocore.packaging.common.ui.StarLabel;

/**
 The class is used to create, update Ppi of MSA/MBD file
 It extends IInternalFrame
 
 @since ModuleEditor 1.0

 **/
public class ModulePpis extends IInternalFrame {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = -4284901202357037724L;

    //
    //Define class members
    //
    private PPIsDocument.PPIs ppis = null;

    private int location = -1;

    private static int PPI = 1;

    private static int PPI_NOTIFY = 2;

    private JPanel jContentPane = null;

    private JRadioButton jRadioButtonPpi = null;

    private JRadioButton jRadioButtonPpiNotify = null;

    private JLabel jLabelC_Name = null;

    private JTextField jTextFieldC_Name = null;

    private JLabel jLabelGuid = null;

    private JTextField jTextFieldGuid = null;

    private JTextField jTextFieldFeatureFlag = null;

    private JLabel jLabelFeatureFlag = null;

    private JLabel jLabelUsage = null;

    private JComboBox jComboBoxUsage = null;

    private JLabel jLabelEnableFeature = null;

    private JRadioButton jRadioButtonEnableFeature = null;

    private JRadioButton jRadioButtonDisableFeature = null;

    private JButton jButtonOk = null;

    private JButton jButtonCancel = null;

    private JLabel jLabelPpiType = null;

    private JButton jButtonGenerateGuid = null;

    private StarLabel jStarLabel1 = null;

    private StarLabel jStarLabel2 = null;

    private JLabel jLabelOverrideID = null;

    private JTextField jTextFieldOverrideID = null;

    /**
     This method initializes jRadioButtonPpi 
     
     @return javax.swing.JRadioButton jRadioButtonPpi
     
     **/
    private JRadioButton getJRadioButtonPpiType() {
        if (jRadioButtonPpi == null) {
            jRadioButtonPpi = new JRadioButton();
            jRadioButtonPpi.setText("Ppi");
            jRadioButtonPpi.setBounds(new java.awt.Rectangle(160, 10, 100, 20));
            jRadioButtonPpi.addActionListener(this);
            jRadioButtonPpi.setSelected(true);
        }
        return jRadioButtonPpi;
    }

    /**
     This method initializes jRadioButtonPpiNotify 
     
     @return javax.swing.JRadioButton jRadioButtonPpiNotify
     
     **/
    private JRadioButton getJRadioButtonPpiNotify() {
        if (jRadioButtonPpiNotify == null) {
            jRadioButtonPpiNotify = new JRadioButton();
            jRadioButtonPpiNotify.setText("Ppi Notify");
            jRadioButtonPpiNotify.setBounds(new java.awt.Rectangle(320, 10, 100, 20));
            jRadioButtonPpiNotify.addActionListener(this);
        }
        return jRadioButtonPpiNotify;
    }

    /**
     This method initializes jTextFieldC_Name 
     
     @return javax.swing.JTextField jTextFieldC_Name
     
     **/
    private JTextField getJTextFieldC_Name() {
        if (jTextFieldC_Name == null) {
            jTextFieldC_Name = new JTextField();
            jTextFieldC_Name.setBounds(new java.awt.Rectangle(160, 35, 320, 20));
        }
        return jTextFieldC_Name;
    }

    /**
     This method initializes jTextFieldGuid 
     
     @return javax.swing.JTextField jTextFieldGuid
     
     **/
    private JTextField getJTextFieldGuid() {
        if (jTextFieldGuid == null) {
            jTextFieldGuid = new JTextField();
            jTextFieldGuid.setBounds(new java.awt.Rectangle(160, 60, 240, 20));
        }
        return jTextFieldGuid;
    }

    /**
     This method initializes jTextFieldFeatureFlag 
     
     @return javax.swing.JTextField jTextFieldFeatureFlag
     
     **/
    private JTextField getJTextFieldFeatureFlag() {
        if (jTextFieldFeatureFlag == null) {
            jTextFieldFeatureFlag = new JTextField();
            jTextFieldFeatureFlag.setBounds(new java.awt.Rectangle(160, 135, 320, 20));
        }
        return jTextFieldFeatureFlag;
    }

    /**
     This method initializes jComboBox
     
     @return javax.swing.JComboBox jComboBoxUsage
     
     **/
    private JComboBox getJComboBox() {
        if (jComboBoxUsage == null) {
            jComboBoxUsage = new JComboBox();
            jComboBoxUsage.setBounds(new java.awt.Rectangle(160, 85, 320, 20));
        }
        return jComboBoxUsage;
    }

    /**
     This method initializes jRadioButtonEnableFeature 
     
     @return javax.swing.JRadioButton jRadioButtonEnableFeature
     
     **/
    private JRadioButton getJRadioButtonEnableFeature() {
        if (jRadioButtonEnableFeature == null) {
            jRadioButtonEnableFeature = new JRadioButton();
            jRadioButtonEnableFeature.setText("Enable");
            jRadioButtonEnableFeature.setBounds(new java.awt.Rectangle(160, 110, 90, 20));
            jRadioButtonEnableFeature.addActionListener(this);
            jRadioButtonEnableFeature.setSelected(true);
        }
        return jRadioButtonEnableFeature;
    }

    /**
     This method initializes jRadioButtonDisableFeature 
     
     @return javax.swing.JRadioButton jRadioButtonDisableFeature
     
     **/
    private JRadioButton getJRadioButtonDisableFeature() {
        if (jRadioButtonDisableFeature == null) {
            jRadioButtonDisableFeature = new JRadioButton();
            jRadioButtonDisableFeature.setText("Disable");
            jRadioButtonDisableFeature.setBounds(new java.awt.Rectangle(320, 110, 90, 20));
            jRadioButtonDisableFeature.addActionListener(this);
        }
        return jRadioButtonDisableFeature;
    }

    /**
     This method initializes jButtonOk 
     
     @return javax.swing.JButton jButtonOk
     
     **/
    private JButton getJButtonOk() {
        if (jButtonOk == null) {
            jButtonOk = new JButton();
            jButtonOk.setText("OK");
            jButtonOk.setBounds(new java.awt.Rectangle(290, 190, 90, 20));
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
            jButtonCancel.setBounds(new java.awt.Rectangle(390, 190, 90, 20));
            jButtonCancel.addActionListener(this);
        }
        return jButtonCancel;
    }

    /**
     This method initializes jButtonGenerateGuid 
     
     @return javax.swing.JButton jButtonGenerateGuid
     
     **/
    private JButton getJButtonGenerateGuid() {
        if (jButtonGenerateGuid == null) {
            jButtonGenerateGuid = new JButton();
            jButtonGenerateGuid.setBounds(new java.awt.Rectangle(405, 60, 75, 20));
            jButtonGenerateGuid.setText("GEN");
            jButtonGenerateGuid.addActionListener(this);
        }
        return jButtonGenerateGuid;
    }

    /**
     This method initializes jTextFieldOverrideID 
     
     @return javax.swing.JTextField jTextFieldOverrideID
     
     **/
    private JTextField getJTextFieldOverrideID() {
        if (jTextFieldOverrideID == null) {
            jTextFieldOverrideID = new JTextField();
            jTextFieldOverrideID.setBounds(new java.awt.Rectangle(160, 160, 320, 20));
        }
        return jTextFieldOverrideID;
    }

    public static void main(String[] args) {

    }

    /**
     This is the default constructor
     
     **/
    public ModulePpis() {
        super();
        init();
        this.setVisible(true);
    }

    /**
     This is the override edit constructor
     
     @param inPpis The input data of PPIsDocument.PPIs
     
     **/
    public ModulePpis(PPIsDocument.PPIs inPpis) {
        super();
        init(inPpis);
        this.setVisible(true);
    }

    /**
     This is the override edit constructor
     
     @param inPpis The input data of PPIsDocument.PPIs
     @param type The input data of node type
     @param index The input data of node index
     
     **/
    public ModulePpis(PPIsDocument.PPIs inPpis, int type, int index) {
        super();
        init(inPpis, type, index);
        this.setVisible(true);
    }

    /**
     This method initializes this
     
     @param inPpis The input data of PPIsDocument.PPIs
     
     **/
    private void init(PPIsDocument.PPIs inPpis) {
        init();
        this.setPpis(inPpis);
    }

    /**
     This method initializes this
     Fill values to all fields if these values are not empty
     
     @param inPpis The input data of PPIsDocument.PPIs
     @param type The input data of node type
     @param index The input data of node index
     
     **/
    private void init(PPIsDocument.PPIs inPpis, int type, int index) {
        init(inPpis);
        this.location = index;
        if (type == IDefaultMutableTreeNode.PPIS_PPI_ITEM) {
            initUsage(ModulePpis.PPI);
            this.jRadioButtonPpi.setSelected(true);
            this.jRadioButtonPpiNotify.setSelected(false);
            if (this.ppis.getPpiArray(index).getStringValue() != null) {
                this.jTextFieldC_Name.setText(this.ppis.getPpiArray(index).getStringValue());
            }
            if (this.ppis.getPpiArray(index).getGuid() != null) {
                this.jTextFieldGuid.setText(this.ppis.getPpiArray(index).getGuid());
            }
            if (this.ppis.getPpiArray(index).getUsage() != null) {
                this.jComboBoxUsage.setSelectedItem(this.ppis.getPpiArray(index).getUsage().toString());
            }
            this.jRadioButtonEnableFeature.setSelected(this.ppis.getPpiArray(index).getEnableFeature());
            this.jRadioButtonDisableFeature.setSelected(!this.ppis.getPpiArray(index).getEnableFeature());
            if (this.ppis.getPpiArray(index).getFeatureFlag() != null) {
                this.jTextFieldFeatureFlag.setText(this.ppis.getPpiArray(index).getFeatureFlag());
            }
            this.jTextFieldOverrideID.setText(String.valueOf(this.ppis.getPpiArray(index).getOverrideID()));
        } else if (type == IDefaultMutableTreeNode.PPIS_PPINOTIFY_ITEM) {
            initUsage(ModulePpis.PPI_NOTIFY);
            this.jRadioButtonPpi.setSelected(false);
            this.jRadioButtonPpiNotify.setSelected(true);
            if (this.ppis.getPpiNotifyArray(index).getStringValue() != null) {
                this.jTextFieldC_Name.setText(this.ppis.getPpiNotifyArray(index).getStringValue());
            }
            if (this.ppis.getPpiNotifyArray(index).getGuid() != null) {
                this.jTextFieldGuid.setText(this.ppis.getPpiNotifyArray(index).getGuid());
            }
            if (this.ppis.getPpiNotifyArray(index).getUsage() != null) {
                this.jComboBoxUsage.setSelectedItem(this.ppis.getPpiNotifyArray(index).getUsage().toString());
            }
            this.jRadioButtonEnableFeature.setSelected(this.ppis.getPpiNotifyArray(index).getEnableFeature());
            this.jRadioButtonDisableFeature.setSelected(!this.ppis.getPpiNotifyArray(index).getEnableFeature());
            if (this.ppis.getPpiNotifyArray(index).getFeatureFlag() != null) {
                this.jTextFieldFeatureFlag.setText(this.ppis.getPpiNotifyArray(index).getFeatureFlag());
            }
            this.jTextFieldOverrideID.setText(String.valueOf(this.ppis.getPpiNotifyArray(index).getOverrideID()));
        }
        this.jRadioButtonPpi.setEnabled(false);
        this.jRadioButtonPpiNotify.setEnabled(false);
    }

    /**
     This method initializes this
     
     **/
    private void init() {
        this.setContentPane(getJContentPane());
        this.setTitle("Ppis");
        this.setBounds(new java.awt.Rectangle(0, 0, 500, 515));
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
            this.jRadioButtonPpi.setEnabled(!isView);
            this.jRadioButtonPpiNotify.setEnabled(!isView);
            this.jTextFieldC_Name.setEnabled(!isView);
            this.jTextFieldGuid.setEnabled(!isView);
            this.jComboBoxUsage.setEnabled(!isView);
            this.jRadioButtonEnableFeature.setEnabled(!isView);
            this.jRadioButtonDisableFeature.setEnabled(!isView);
            this.jTextFieldFeatureFlag.setEnabled(!isView);
            this.jTextFieldOverrideID.setEnabled(!isView);
            this.jButtonGenerateGuid.setEnabled(!isView);
        }
    }

    /**
     This method initializes jContentPane
     
     @return javax.swing.JPanel jContentPane
     
     **/
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jLabelOverrideID = new JLabel();
            jLabelOverrideID.setBounds(new java.awt.Rectangle(15, 160, 140, 20));
            jLabelOverrideID.setText("Override ID");
            jLabelPpiType = new JLabel();
            jLabelPpiType.setBounds(new java.awt.Rectangle(15, 10, 140, 20));
            jLabelPpiType.setText("Ppi Type");
            jLabelEnableFeature = new JLabel();
            jLabelEnableFeature.setText("Enable Feature");
            jLabelEnableFeature.setBounds(new java.awt.Rectangle(15, 110, 140, 20));
            jLabelUsage = new JLabel();
            jLabelUsage.setText("Usage");
            jLabelUsage.setBounds(new java.awt.Rectangle(15, 85, 140, 20));
            jLabelFeatureFlag = new JLabel();
            jLabelFeatureFlag.setText("Feature Flag");
            jLabelFeatureFlag.setBounds(new java.awt.Rectangle(15, 135, 140, 20));
            jLabelGuid = new JLabel();
            jLabelGuid.setText("Guid");
            jLabelGuid.setBounds(new java.awt.Rectangle(15, 60, 140, 20));
            jLabelC_Name = new JLabel();
            jLabelC_Name.setText("C_Name");
            jLabelC_Name.setBounds(new java.awt.Rectangle(15, 35, 140, 20));
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.add(getJRadioButtonPpiType(), null);
            jContentPane.add(jLabelC_Name, null);
            jContentPane.add(getJTextFieldC_Name(), null);
            jContentPane.add(jLabelGuid, null);
            jContentPane.add(getJTextFieldGuid(), null);
            jContentPane.add(getJTextFieldFeatureFlag(), null);
            jContentPane.add(jLabelFeatureFlag, null);
            jContentPane.add(jLabelUsage, null);
            jContentPane.add(getJComboBox(), null);
            jContentPane.add(jLabelEnableFeature, null);
            jContentPane.add(getJRadioButtonEnableFeature(), null);
            jContentPane.add(getJRadioButtonDisableFeature(), null);
            jContentPane.add(getJButtonOk(), null);
            jContentPane.add(getJButtonCancel(), null);
            jContentPane.add(getJRadioButtonPpiNotify(), null);
            jContentPane.add(jLabelPpiType, null);
            jContentPane.add(getJButtonGenerateGuid(), null);

            jStarLabel1 = new StarLabel();
            jStarLabel1.setBounds(new java.awt.Rectangle(0, 10, 10, 20));
            jStarLabel2 = new StarLabel();
            jStarLabel2.setBounds(new java.awt.Rectangle(0, 35, 10, 20));

            jContentPane.add(jStarLabel1, null);
            jContentPane.add(jStarLabel2, null);
            jContentPane.add(jLabelOverrideID, null);
            jContentPane.add(getJTextFieldOverrideID(), null);
        }
        return jContentPane;
    }

    /**
     This method initializes Usage type
     
     **/
    private void initFrame() {
        jComboBoxUsage.addItem("ALWAYS_CONSUMED");
        jComboBoxUsage.addItem("SOMETIMES_CONSUMED");
        jComboBoxUsage.addItem("ALWAYS_PRODUCED");
        jComboBoxUsage.addItem("SOMETIMES_PRODUCED");
        jComboBoxUsage.addItem("PRIVATE");
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

        //
        //Contorl the selected status when click RadionButton
        //Do not use Radio Button Group
        //
        if (arg0.getSource() == jRadioButtonPpi) {
            if (jRadioButtonPpi.isSelected()) {
                jRadioButtonPpiNotify.setSelected(false);
                initUsage(ModulePpis.PPI);
            }
            if (!jRadioButtonPpiNotify.isSelected() && !jRadioButtonPpi.isSelected()) {
                jRadioButtonPpi.setSelected(true);
                initUsage(ModulePpis.PPI);
            }
        }

        if (arg0.getSource() == jRadioButtonPpiNotify) {
            if (jRadioButtonPpiNotify.isSelected()) {
                jRadioButtonPpi.setSelected(false);
                initUsage(ModulePpis.PPI_NOTIFY);
            }
            if (!jRadioButtonPpiNotify.isSelected() && !jRadioButtonPpi.isSelected()) {
                jRadioButtonPpiNotify.setSelected(true);
                initUsage(ModulePpis.PPI_NOTIFY);
            }
        }

        //
        //Contorl the selected status when click RadionButton
        //Do not use Radio Button Group
        //
        if (arg0.getSource() == jRadioButtonEnableFeature) {
            if (jRadioButtonEnableFeature.isSelected()) {
                jRadioButtonDisableFeature.setSelected(false);
            }
            if (!jRadioButtonDisableFeature.isSelected() && !jRadioButtonEnableFeature.isSelected()) {
                jRadioButtonEnableFeature.setSelected(true);
            }
        }

        if (arg0.getSource() == jRadioButtonDisableFeature) {
            if (jRadioButtonDisableFeature.isSelected()) {
                jRadioButtonEnableFeature.setSelected(false);
            }
            if (!jRadioButtonDisableFeature.isSelected() && !jRadioButtonEnableFeature.isSelected()) {
                jRadioButtonDisableFeature.setSelected(true);
            }
        }

        if (arg0.getSource() == jButtonGenerateGuid) {
            jTextFieldGuid.setText(Tools.generateUuidString());
        }
    }

    /**
     Get PPIsDocument.PPIs
     
     @return PPIsDocument.PPIs
     
     **/
    public PPIsDocument.PPIs getPpis() {
        return ppis;
    }

    /**
     Set PPIsDocument.PPIs
     
     @param ppis The input data of PPIsDocument.PPIs
     
     **/
    public void setPpis(PPIsDocument.PPIs ppis) {
        this.ppis = ppis;
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
        if (isEmpty(this.jTextFieldC_Name.getText())) {
            Log.err("C_Name couldn't be empty");
            return false;
        }

        //
        // Check if all fields have correct data types 
        //
        if (!DataValidation.isCName(this.jTextFieldC_Name.getText())) {
            Log.err("Incorrect data type for C_Name");
            return false;
        }
        if (!isEmpty(this.jTextFieldGuid.getText()) && !DataValidation.isGuid(this.jTextFieldGuid.getText())) {
            Log.err("Incorrect data type for Guid");
            return false;
        }
        if (!isEmpty(this.jTextFieldFeatureFlag.getText())
            && !DataValidation.isFeatureFlag(this.jTextFieldFeatureFlag.getText())) {
            Log.err("Incorrect data type for Feature Flag");
            return false;
        }
        if (!isEmpty(this.jTextFieldOverrideID.getText())
            && !DataValidation.isOverrideID(this.jTextFieldOverrideID.getText())) {
            Log.err("Incorrect data type for Override ID");
            return false;
        }

        return true;
    }

    /**
     Save all components of PPIs
     if exists ppis, set the value directly
     if not exists ppis, new an instance first
     
     **/
    public void save() {
        try {
            if (this.ppis == null) {
                ppis = PPIsDocument.PPIs.Factory.newInstance();
            }
            if (this.jRadioButtonPpi.isSelected()) {
                PPIsDocument.PPIs.Ppi ppi = PPIsDocument.PPIs.Ppi.Factory.newInstance();
                ppi.setStringValue(this.jTextFieldC_Name.getText());
                if (!isEmpty(this.jTextFieldGuid.getText())) {
                    ppi.setGuid(this.jTextFieldGuid.getText());
                }
                ppi.setUsage(PpiUsage.Enum.forString(jComboBoxUsage.getSelectedItem().toString()));
                ppi.setEnableFeature(this.jRadioButtonEnableFeature.isSelected());
                if (!isEmpty(this.jTextFieldFeatureFlag.getText())) {
                    ppi.setFeatureFlag(this.jTextFieldFeatureFlag.getText());
                }
                if (!isEmpty(this.jTextFieldOverrideID.getText())) {
                    ppi.setOverrideID(Integer.parseInt(this.jTextFieldOverrideID.getText()));
                }
                if (location > -1) {
                    ppis.setPpiArray(location, ppi);
                } else {
                    ppis.addNewPpi();
                    ppis.setPpiArray(ppis.getPpiList().size() - 1, ppi);
                }
            }
            if (this.jRadioButtonPpiNotify.isSelected()) {
                PPIsDocument.PPIs.PpiNotify ppiNotify = PPIsDocument.PPIs.PpiNotify.Factory.newInstance();
                ppiNotify.setStringValue(this.jTextFieldC_Name.getText());
                if (!isEmpty(this.jTextFieldGuid.getText())) {
                    ppiNotify.setGuid(this.jTextFieldGuid.getText());
                }
                ppiNotify.setUsage(PpiNotifyUsage.Enum.forString(jComboBoxUsage.getSelectedItem().toString()));
                ppiNotify.setEnableFeature(this.jRadioButtonEnableFeature.isSelected());
                if (!isEmpty(this.jTextFieldFeatureFlag.getText())) {
                    ppiNotify.setFeatureFlag(this.jTextFieldFeatureFlag.getText());
                }
                if (!isEmpty(this.jTextFieldOverrideID.getText())) {
                    ppiNotify.setOverrideID(Integer.parseInt(this.jTextFieldOverrideID.getText()));
                }
                if (location > -1) {
                    ppis.setPpiNotifyArray(location, ppiNotify);
                } else {
                    ppis.addNewPpiNotify();
                    ppis.setPpiNotifyArray(ppis.getPpiNotifyList().size() - 1, ppiNotify);
                }
            }
        } catch (Exception e) {
            Log.err("Update Protocols", e.getMessage());
        }
    }

    /**
     Enable/Disable relevant fields via different PPI types
     
     @param intType The input data of PPI type
     
     **/
    private void initUsage(int intType) {
        jComboBoxUsage.removeAllItems();
        if (intType == ModulePpis.PPI) {
            jComboBoxUsage.addItem("ALWAYS_CONSUMED");
            jComboBoxUsage.addItem("SOMETIMES_CONSUMED");
            jComboBoxUsage.addItem("ALWAYS_PRODUCED");
            jComboBoxUsage.addItem("SOMETIMES_PRODUCED");
            jComboBoxUsage.addItem("PRIVATE");
        }
        if (intType == ModulePpis.PPI_NOTIFY) {
            jComboBoxUsage.addItem("SOMETIMES_CONSUMED");
        }
    }
}
