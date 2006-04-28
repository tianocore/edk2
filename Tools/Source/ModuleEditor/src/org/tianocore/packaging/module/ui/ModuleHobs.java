/** @file
 
 The file is used to create, update Hob of MSA/MBD file
 
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

import org.tianocore.GuidDocument;
import org.tianocore.HobTypes;
import org.tianocore.HobUsage;
import org.tianocore.HobsDocument;
import org.tianocore.common.DataValidation;
import org.tianocore.common.Log;
import org.tianocore.common.Tools;
import org.tianocore.packaging.common.ui.IInternalFrame;
import org.tianocore.packaging.common.ui.StarLabel;

/**
 The class is used to create, update Hob of MSA/MBD file
 It extends IInternalFrame
 
 @since ModuleEditor 1.0

 **/
public class ModuleHobs extends IInternalFrame {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = -553473437579358325L;

    //
    //Define class members
    //
    private HobsDocument.Hobs hobs = null;

    private int location = -1;

    private JPanel jContentPane = null;

    private JLabel jLabel = null;

    private JTextField jTextFieldC_Name = null;

    private JLabel jLabelGuid = null;

    private JTextField jTextFieldGuid = null;

    private JLabel jLabelName = null;

    private JTextField jTextFieldName = null;

    private JLabel jLabelUsage = null;

    private JLabel jLabelHobType = null;

    private JComboBox jComboBoxUsage = null;

    private JComboBox jComboBoxHobType = null;

    private JLabel jLabelHobEnabled = null;

    private JRadioButton jRadioButtonHobEnable = null;

    private JRadioButton jRadioButtonHobDisable = null;

    private JButton jButtonOk = null;

    private JButton jButtonCancel = null;

    private JButton jButtonGenerateGuid = null;

    private JLabel jLabelOverrideID = null;

    private JTextField jTextFieldOverrideID = null;

    private StarLabel jStarLabel1 = null;

    /**
     This method initializes jTextField 
     
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
     This method initializes jTextField 
     
     @return javax.swing.JTextField jTextFieldGuid
     
     **/
    private JTextField getJTextFieldGuid() {
        if (jTextFieldGuid == null) {
            jTextFieldGuid = new JTextField();
            jTextFieldGuid.setBounds(new java.awt.Rectangle(160, 60, 250, 20));
        }
        return jTextFieldGuid;
    }

    /**
     This method initializes jTextFieldName 
     
     @return javax.swing.JTextField jTextFieldName
     
     **/
    private JTextField getJTextFieldName() {
        if (jTextFieldName == null) {
            jTextFieldName = new JTextField();
            jTextFieldName.setBounds(new java.awt.Rectangle(160, 10, 320, 20));
        }
        return jTextFieldName;
    }

    /**
     This method initializes jComboBoxUsage 
     
     @return javax.swing.JComboBox jComboBoxUsage
     
     **/
    private JComboBox getJComboBoxUsage() {
        if (jComboBoxUsage == null) {
            jComboBoxUsage = new JComboBox();
            jComboBoxUsage.setBounds(new java.awt.Rectangle(160, 85, 320, 20));
        }
        return jComboBoxUsage;
    }

    /**
     This method initializes jComboBoxHobType 
     
     @return javax.swing.JComboBox jComboBoxHobType
     
     **/
    private JComboBox getJComboBoxHobType() {
        if (jComboBoxHobType == null) {
            jComboBoxHobType = new JComboBox();
            jComboBoxHobType.setBounds(new java.awt.Rectangle(160, 110, 320, 20));
        }
        return jComboBoxHobType;
    }

    /**
     This method initializes jRadioButtonEnable 
     
     @return javax.swing.JRadioButton jRadioButtonHobEnable
     
     **/
    private JRadioButton getJRadioButtonHobEnable() {
        if (jRadioButtonHobEnable == null) {
            jRadioButtonHobEnable = new JRadioButton();
            jRadioButtonHobEnable.setText("Enable");
            jRadioButtonHobEnable.setBounds(new java.awt.Rectangle(160, 135, 90, 20));
            jRadioButtonHobEnable.setSelected(true);
            jRadioButtonHobEnable.addActionListener(this);
        }
        return jRadioButtonHobEnable;
    }

    /**
     This method initializes jRadioButtonDisable 
     
     @return javax.swing.JRadioButton jRadioButtonHobDisable
     
     **/
    private JRadioButton getJRadioButtonHobDisable() {
        if (jRadioButtonHobDisable == null) {
            jRadioButtonHobDisable = new JRadioButton();
            jRadioButtonHobDisable.setText("Disable");
            jRadioButtonHobDisable.setBounds(new java.awt.Rectangle(320, 135, 90, 20));
            jRadioButtonHobDisable.addActionListener(this);
        }
        return jRadioButtonHobDisable;
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
            jButtonGenerateGuid.setBounds(new java.awt.Rectangle(415, 60, 65, 20));
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
            jTextFieldOverrideID.setBounds(new java.awt.Rectangle(160, 160, 50, 20));
        }
        return jTextFieldOverrideID;
    }

    public static void main(String[] args) {

    }

    /**
     This is the default constructor
     
     **/
    public ModuleHobs() {
        super();
        init();
        this.setVisible(true);
    }

    /**
     This is the override edit constructor
     
     @param inHobs The input data of HobsDocument.Hobs
     
     **/
    public ModuleHobs(HobsDocument.Hobs inHobs) {
        super();
        init(inHobs);
        this.setVisible(true);
    }

    /**
     This is the override edit constructor
     
     @param inHobs The input data of HobsDocument.Hobs
     @param type The input data of node type
     @param index The input data of node index
     
     **/
    public ModuleHobs(HobsDocument.Hobs inHobs, int type, int index) {
        super();
        init(inHobs, type, index);
        this.setVisible(true);
    }

    /**
     This method initializes this
     
     @param inHobs The input data of HobsDocument.Hobs
     
     **/
    private void init(HobsDocument.Hobs inHobs) {
        init();
        this.setHobs(inHobs);
    }

    /**
     This method initializes this
     Fill values to all fields if these values are not empty
     
     @param inHobs The input data of HobsDocument.Hobs
     @param type The input data of node type
     @param index The input data of node index
     
     **/
    private void init(HobsDocument.Hobs inHobs, int type, int index) {
        init(inHobs);
        this.location = index;
        if (this.hobs.getHobList().size() > 0) {
            if (this.hobs.getHobArray(index).getName() != null) {
                this.jTextFieldName.setText(this.hobs.getHobArray(index).getName());
            }
            if (this.hobs.getHobArray(index).getCName() != null) {
                this.jTextFieldC_Name.setText(this.hobs.getHobArray(index).getCName());
            }
            if (this.hobs.getHobArray(index).getGuid() != null) {
                this.jTextFieldGuid.setText(this.hobs.getHobArray(index).getGuid().getStringValue());
            }
            if (this.hobs.getHobArray(index).getUsage() != null) {
                this.jComboBoxUsage.setSelectedItem(this.hobs.getHobArray(index).getUsage().toString());
            }
            this.jRadioButtonHobEnable.setSelected(this.hobs.getHobArray(index).getHobEnabled());
            this.jRadioButtonHobDisable.setSelected(!this.hobs.getHobArray(index).getHobEnabled());
            this.jTextFieldOverrideID.setText(String.valueOf(this.hobs.getHobArray(index).getOverrideID()));
        }
    }

    /**
     This method initializes this
     
     **/
    private void init() {
        this.setSize(500, 515);
        this.setContentPane(getJContentPane());
        this.setTitle("Hobs");
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
            this.jTextFieldName.setEnabled(!isView);
            this.jTextFieldC_Name.setEnabled(!isView);
            this.jTextFieldGuid.setEnabled(!isView);
            this.jComboBoxUsage.setEnabled(!isView);
            this.jComboBoxHobType.setEnabled(!isView);
            this.jRadioButtonHobEnable.setEnabled(!isView);
            this.jRadioButtonHobDisable.setEnabled(!isView);
            this.jTextFieldOverrideID.setEnabled(!isView);
            this.jButtonCancel.setEnabled(!isView);
            this.jButtonGenerateGuid.setEnabled(!isView);
            this.jButtonOk.setEnabled(!isView);
        }
    }

    /**
     This method initializes jContentPane
     
     @return javax.swing.JPanel jContentPane
     
     **/
    public JPanel getJContentPane() {
        if (jContentPane == null) {
            jLabelOverrideID = new JLabel();
            jLabelOverrideID.setBounds(new java.awt.Rectangle(15, 160, 140, 20));
            jLabelOverrideID.setText("Override ID");
            jLabelHobEnabled = new JLabel();
            jLabelHobEnabled.setText("Hob Enabled");
            jLabelHobEnabled.setBounds(new java.awt.Rectangle(15, 135, 140, 20));
            jLabelHobType = new JLabel();
            jLabelHobType.setText("Hob Type");
            jLabelHobType.setBounds(new java.awt.Rectangle(15, 110, 140, 20));
            jLabelUsage = new JLabel();
            jLabelUsage.setText("Usage");
            jLabelUsage.setBounds(new java.awt.Rectangle(15, 85, 140, 20));
            jLabelName = new JLabel();
            jLabelName.setText("Name");
            jLabelName.setBounds(new java.awt.Rectangle(15, 10, 140, 20));
            jLabelGuid = new JLabel();
            jLabelGuid.setText("Guid");
            jLabelGuid.setBounds(new java.awt.Rectangle(15, 60, 140, 20));
            jLabel = new JLabel();
            jLabel.setText("C_Name");
            jLabel.setBounds(new java.awt.Rectangle(15, 35, 140, 20));
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.add(jLabel, null);
            jContentPane.add(getJTextFieldC_Name(), null);
            jContentPane.add(getJTextFieldGuid(), null);
            jContentPane.add(jLabelUsage, null);
            jContentPane.add(jLabelName, null);
            jContentPane.add(getJTextFieldName(), null);
            jContentPane.add(jLabelGuid, null);
            jContentPane.add(jLabelHobType, null);
            jContentPane.add(getJComboBoxUsage(), null);
            jContentPane.add(getJComboBoxHobType(), null);
            jContentPane.add(jLabelHobEnabled, null);
            jContentPane.add(getJRadioButtonHobEnable(), null);
            jContentPane.add(getJRadioButtonHobDisable(), null);
            jContentPane.add(getJButtonOk(), null);
            jContentPane.add(getJButtonCancel(), null);
            jContentPane.add(getJButtonGenerateGuid(), null);
            jContentPane.add(jLabelOverrideID, null);
            jContentPane.add(getJTextFieldOverrideID(), null);

            jStarLabel1 = new StarLabel();
            jStarLabel1.setLocation(new java.awt.Point(0, 10));
            jContentPane.add(jStarLabel1, null);
        }
        return jContentPane;
    }

    /**
     This method initializes Usage type and Hob type
     
     **/
    private void initFrame() {
        jComboBoxHobType.addItem("PHIT");
        jComboBoxHobType.addItem("MEMORY_ALLOCATION");
        jComboBoxHobType.addItem("RESOURCE_DESCRIPTOR");
        jComboBoxHobType.addItem("GUID_EXTENSION");
        jComboBoxHobType.addItem("FIRMWARE_VOLUME");
        jComboBoxHobType.addItem("CPU");
        jComboBoxHobType.addItem("POOL");
        jComboBoxHobType.addItem("CAPSULE_VOLUME");

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
        // Contorl the selected status when click RadionButton
        // Do not use Radio Button Group
        //
        if (arg0.getSource() == jRadioButtonHobEnable) {
            if (jRadioButtonHobEnable.isSelected()) {
                jRadioButtonHobDisable.setSelected(false);
            }
            if (!jRadioButtonHobDisable.isSelected() && !jRadioButtonHobEnable.isSelected()) {
                jRadioButtonHobEnable.setSelected(true);
            }
        }

        if (arg0.getSource() == jRadioButtonHobDisable) {
            if (jRadioButtonHobDisable.isSelected()) {
                jRadioButtonHobEnable.setSelected(false);
            }
            if (!jRadioButtonHobDisable.isSelected() && !jRadioButtonHobEnable.isSelected()) {
                jRadioButtonHobDisable.setSelected(true);
            }
        }

        if (arg0.getSource() == jButtonGenerateGuid) {
            jTextFieldGuid.setText(Tools.generateUuidString());
        }
    }

    /**
     Get HobsDocument.Hobs
     
     @return HobsDocument.Hobs
     
     **/
    public HobsDocument.Hobs getHobs() {
        return hobs;
    }

    /**
     Set HobsDocument.Hobs
     
     @param hobs The input data of HobsDocument.Hobs
     
     **/
    public void setHobs(HobsDocument.Hobs hobs) {
        this.hobs = hobs;
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
        if (isEmpty(this.jTextFieldName.getText())) {
            Log.err("Name couldn't be empty");
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
        if (!isEmpty(this.jTextFieldOverrideID.getText())
            && !DataValidation.isOverrideID(this.jTextFieldOverrideID.getText())) {
            Log.err("Incorrect data type for Override ID");
            return false;
        }

        return true;
    }

    /**
     Save all components of Hobs
     if exists hobs, set the value directly
     if not exists hobs, new an instance first
     
     **/
    public void save() {
        try {
            if (this.hobs == null) {
                hobs = HobsDocument.Hobs.Factory.newInstance();
            }
            HobsDocument.Hobs.Hob hob = HobsDocument.Hobs.Hob.Factory.newInstance();
            if (!isEmpty(this.jTextFieldName.getText())) {
                hob.setName(this.jTextFieldName.getText());
            }
            if (!isEmpty(this.jTextFieldC_Name.getText())) {
                hob.setCName(this.jTextFieldC_Name.getText());
            }
            if (!isEmpty(this.jTextFieldGuid.getText())) {
                GuidDocument.Guid guid = GuidDocument.Guid.Factory.newInstance();
                guid.setStringValue(this.jTextFieldGuid.getText());
                hob.setGuid(guid);
            }
            hob.setUsage(HobUsage.Enum.forString(jComboBoxUsage.getSelectedItem().toString()));
            hob.setHobType(HobTypes.Enum.forString(jComboBoxHobType.getSelectedItem().toString()));
            hob.setHobEnabled(this.jRadioButtonHobEnable.isSelected());
            if (!isEmpty(this.jTextFieldOverrideID.getText())) {
                hob.setOverrideID(Integer.parseInt(this.jTextFieldOverrideID.getText()));
            }
            if (location > -1) {
                hobs.setHobArray(location, hob);
            } else {
                hobs.addNewHob();
                hobs.setHobArray(hobs.getHobList().size() - 1, hob);
            }
        } catch (Exception e) {
            Log.err("Update Hobs", e.getMessage());
        }
    }
}
