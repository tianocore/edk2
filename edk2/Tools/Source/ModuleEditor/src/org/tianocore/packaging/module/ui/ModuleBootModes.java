/** @file
 
 The file is used to create, update BootModes of MSA/MBD file
 
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
import javax.swing.JTextField;

import org.tianocore.BootModeNames;
import org.tianocore.BootModeUsage;
import org.tianocore.BootModesDocument;
import org.tianocore.common.DataValidation;
import org.tianocore.common.Log;
import org.tianocore.common.Tools;
import org.tianocore.packaging.common.ui.IInternalFrame;
import org.tianocore.packaging.common.ui.StarLabel;

/**
 The class is used to create, update BootModes of MSA/MBD file
 It extends IInternalFrame
 
 @since ModuleEditor 1.0

 **/
public class ModuleBootModes extends IInternalFrame {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = -3888558623432442561L;

    //
    //Define class members
    //
    private BootModesDocument.BootModes bootModes = null;

    private int location = -1;

    private JPanel jContentPane = null;

    private JLabel jLabelGuid = null;

    private JTextField jTextFieldGuid = null;

    private JLabel jLabelBootModeName = null;

    private JComboBox jComboBoxBootModeName = null;

    private JLabel jLabelUsage = null;

    private JComboBox jComboBoxUsage = null;

    private JButton jButtonOk = null;

    private JButton jButtonCancel = null;

    private JLabel jLabelOverrideID = null;

    private JTextField jTextFieldOverrideID = null;

    private JButton jButtonGenerateGuid = null;

    private StarLabel jStarLabel1 = null;

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
     This method initializes jComboBoxBootModeName 
     
     @return javax.swing.JComboBox jComboBoxBootModeName
     
     **/
    private JComboBox getJComboBoxBootModeName() {
        if (jComboBoxBootModeName == null) {
            jComboBoxBootModeName = new JComboBox();
            jComboBoxBootModeName.setBounds(new java.awt.Rectangle(160, 10, 320, 20));
        }
        return jComboBoxBootModeName;
    }

    /**
     This method initializes jComboBoxUsage 
     
     @return javax.swing.JComboBox jComboBoxUsage
     
     **/
    private JComboBox getJComboBoxUsage() {
        if (jComboBoxUsage == null) {
            jComboBoxUsage = new JComboBox();
            jComboBoxUsage.setBounds(new java.awt.Rectangle(160, 60, 320, 20));
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
            jButtonOk.setBounds(new java.awt.Rectangle(280, 115, 90, 20));
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
            jButtonCancel.setBounds(new java.awt.Rectangle(390, 115, 90, 20));
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
            jTextFieldOverrideID.setBounds(new java.awt.Rectangle(160, 85, 320, 20));
        }
        return jTextFieldOverrideID;
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

    public static void main(String[] args) {
    }

    /**
     This is the default constructor
     
     **/
    public ModuleBootModes() {
        super();
        init();
        this.setVisible(true);
    }

    /**
     This is the override edit constructor
     
     @param inBootModes The input BootModesDocument.BootModes
     
     **/
    public ModuleBootModes(BootModesDocument.BootModes inBootModes) {
        super();
        init(inBootModes);
        this.setVisible(true);
    }

    /**
     This is the override edit constructor
     
     @param inBootModes The input BootModesDocument.BootModes
     @param type The input data of node type
     @param index The input data of node index
     
     **/
    public ModuleBootModes(BootModesDocument.BootModes inBootModes, int type, int index) {
        super();
        init(inBootModes, type, index);
        this.setVisible(true);
    }

    /**
     This method initializes this

     @param inBootModes BootModesDocument.BootModes
     
     **/
    private void init(BootModesDocument.BootModes inBootModes) {
        init();
        this.setBootModes(inBootModes);
    }

    /**
     This method initializes this
     Fill values to all fields if these values are not empty
     
     @param inBootModes The input BootModesDocument.BootModes
     @param type The input data of node type
     @param index The input data of node index
     
     **/
    private void init(BootModesDocument.BootModes inBootModes, int type, int index) {
        init(inBootModes);
        this.location = index;
        if (this.bootModes.getBootModeList().size() > 0) {
            if (this.bootModes.getBootModeArray(index).getBootModeName() != null) {
                this.jComboBoxBootModeName.setSelectedItem(this.bootModes.getBootModeArray(index).getBootModeName()
                                                                         .toString());
            }
            if (this.bootModes.getBootModeArray(index).getGuid() != null) {
                this.jTextFieldGuid.setText(this.bootModes.getBootModeArray(index).getGuid());
            }
            if (this.bootModes.getBootModeArray(index).getUsage() != null) {
                this.jComboBoxUsage.setSelectedItem(this.bootModes.getBootModeArray(index).getUsage().toString());
            }
            this.jTextFieldOverrideID.setText(String.valueOf(this.bootModes.getBootModeArray(index).getOverrideID()));
        }
    }

    /**
     * This method initializes this
     * 
     * @return void
     */
    private void init() {
        this.setContentPane(getJContentPane());
        this.setTitle("Boot Mode");
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
            this.jComboBoxBootModeName.setEnabled(!isView);
            this.jTextFieldGuid.setEnabled(!isView);
            this.jComboBoxUsage.setEnabled(!isView);
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
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jLabelOverrideID = new JLabel();
            jLabelOverrideID.setBounds(new java.awt.Rectangle(15, 85, 140, 20));
            jLabelOverrideID.setText("Override ID");
            jLabelUsage = new JLabel();
            jLabelUsage.setText("Usage");
            jLabelUsage.setBounds(new java.awt.Rectangle(15, 60, 140, 20));
            jLabelBootModeName = new JLabel();
            jLabelBootModeName.setText("Boot Mode Name");
            jLabelBootModeName.setBounds(new java.awt.Rectangle(15, 10, 140, 20));
            jLabelGuid = new JLabel();
            jLabelGuid.setText("Guid");
            jLabelGuid.setBounds(new java.awt.Rectangle(15, 35, 140, 20));
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.add(jLabelGuid, null);
            jContentPane.add(getJTextFieldGuid(), null);
            jContentPane.add(jLabelBootModeName, null);
            jContentPane.add(getJComboBoxBootModeName(), null);
            jContentPane.add(jLabelUsage, null);
            jContentPane.add(getJComboBoxUsage(), null);
            jContentPane.add(getJButtonOk(), null);
            jContentPane.add(getJButtonCancel(), null);
            jContentPane.add(jLabelOverrideID, null);
            jContentPane.add(getJTextFieldOverrideID(), null);
            jContentPane.add(getJButtonGenerateGuid(), null);

            jStarLabel1 = new StarLabel();
            jStarLabel1.setLocation(new java.awt.Point(0, 10));

            jContentPane.add(jStarLabel1, null);
        }
        return jContentPane;
    }

    /**
     This method initializes BootModeName groups and Usage type
     
     **/
    private void initFrame() {
        jComboBoxUsage.addItem("ALWAYS_CONSUMED");
        jComboBoxUsage.addItem("SOMETIMES_CONSUMED");
        jComboBoxUsage.addItem("ALWAYS_PRODUCED");
        jComboBoxUsage.addItem("SOMETIMES_PRODUCED");

        jComboBoxBootModeName.addItem("FULL");
        jComboBoxBootModeName.addItem("MINIMAL");
        jComboBoxBootModeName.addItem("NO_CHANGE");
        jComboBoxBootModeName.addItem("DIAGNOSTICS");
        jComboBoxBootModeName.addItem("DEFAULT");
        jComboBoxBootModeName.addItem("S2_RESUME");
        jComboBoxBootModeName.addItem("S3_RESUME");
        jComboBoxBootModeName.addItem("S4_RESUME");
        jComboBoxBootModeName.addItem("S5_RESUME");
        jComboBoxBootModeName.addItem("FLASH_UPDATE");
        jComboBoxBootModeName.addItem("RECOVERY");
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

        if (arg0.getSource() == jButtonGenerateGuid) {
            jTextFieldGuid.setText(Tools.generateUuidString());
        }
    }

    /**
     Get BootModesDocument.BootModes
     
     @return BootModesDocument.BootModes
     
     **/
    public BootModesDocument.BootModes getBootModes() {
        return bootModes;
    }

    /**
     Set BootModesDocument.BootModes
     
     @param bootModes BootModesDocument.BootModes
     
     **/
    public void setBootModes(BootModesDocument.BootModes bootModes) {
        this.bootModes = bootModes;
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
     Save all components of Mbd Header
     if exists bootModes, set the value directly
     if not exists bootModes, new an instance first
     
     **/
    public void save() {
        try {
            if (this.bootModes == null) {
                bootModes = BootModesDocument.BootModes.Factory.newInstance();
            }
            BootModesDocument.BootModes.BootMode bootMode = BootModesDocument.BootModes.BootMode.Factory.newInstance();
            bootMode.setBootModeName(BootModeNames.Enum.forString(jComboBoxBootModeName.getSelectedItem().toString()));
            if (!isEmpty(this.jTextFieldGuid.getText())) {
                bootMode.setGuid(this.jTextFieldGuid.getText());
            }
            bootMode.setUsage(BootModeUsage.Enum.forString(jComboBoxUsage.getSelectedItem().toString()));
            if (!isEmpty(this.jTextFieldOverrideID.getText())) {
                bootMode.setOverrideID(Integer.parseInt(this.jTextFieldOverrideID.getText()));
            }
            if (location > -1) {
                bootModes.setBootModeArray(location, bootMode);
            } else {
                bootModes.addNewBootMode();
                bootModes.setBootModeArray(bootModes.getBootModeList().size() - 1, bootMode);
            }
        } catch (Exception e) {
            Log.err("Update Boot Modes", e.getMessage());
        }
    }
}
