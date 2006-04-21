/** @file
 
 The file is used to create, update Variable of MSA/MBD file
 
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

import org.tianocore.GuidDocument;
import org.tianocore.VariableUsage;
import org.tianocore.VariablesDocument;
import org.tianocore.common.DataValidation;
import org.tianocore.common.Log;
import org.tianocore.common.Tools;
import org.tianocore.packaging.common.ui.IInternalFrame;
import org.tianocore.packaging.common.ui.StarLabel;

/**
 The class is used to create, update Variable of MSA/MBD file
 It extends IInternalFrame
 
 @since ModuleEditor 1.0

 **/
public class ModuleVariables extends IInternalFrame {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = -6998982978030439446L;

    //
    //Define class members
    //
    private VariablesDocument.Variables variables = null;

    private int location = -1;

    private JPanel jContentPane = null;

    private JLabel jLabelGuid = null;

    private JTextField jTextFieldGuid = null;

    private JLabel jLabelString = null;

    private JTextField jTextFieldString = null;

    private JLabel jLabelBitOffset = null;

    private JTextField jTextFieldBitOffset = null;

    private JLabel jLabelByteOffset = null;

    private JTextField jTextFieldByteOffset = null;

    private JLabel jLabelOffsetBitSize = null;

    private JTextField jTextFieldOffsetBitSize = null;

    private JLabel jLabelUsage = null;

    private JComboBox jComboBoxUsage = null;

    private JButton jButtonOk = null;

    private JButton jButtonCancel = null;

    private JButton jButtonGenerateGuid = null;

    private JLabel jLabelOverrideID = null;

    private JTextField jTextFieldOverrideID = null;

    private StarLabel jStarLabel1 = null;

    private StarLabel jStarLabel2 = null;

    private JLabel jLabelByteOffsetHint = null;

    private JLabel jLabelBitOffsetHint = null;

    private JLabel jLabelOffsetBitSizeHint = null;

    /**
     This method initializes jTextFieldGuid 
     
     @return javax.swing.JTextField jTextFieldGuid
     
     **/
    private JTextField getJTextFieldGuid() {
        if (jTextFieldGuid == null) {
            jTextFieldGuid = new JTextField();
            jTextFieldGuid.setSize(new java.awt.Dimension(250, 20));
            jTextFieldGuid.setLocation(new java.awt.Point(160, 35));
        }
        return jTextFieldGuid;
    }

    /**
     This method initializes jTextFieldString 
     
     @return javax.swing.JTextField jTextFieldString
     
     **/
    private JTextField getJTextFieldString() {
        if (jTextFieldString == null) {
            jTextFieldString = new JTextField();
            jTextFieldString.setSize(new java.awt.Dimension(320, 20));
            jTextFieldString.setLocation(new java.awt.Point(160, 10));
        }
        return jTextFieldString;
    }

    /**
     This method initializes jTextFieldBitOffset 
     
     @return javax.swing.JTextField jTextFieldBitOffset
     
     **/
    private JTextField getJTextFieldBitOffset() {
        if (jTextFieldBitOffset == null) {
            jTextFieldBitOffset = new JTextField();
            jTextFieldBitOffset.setSize(new java.awt.Dimension(80, 20));
            jTextFieldBitOffset.setLocation(new java.awt.Point(160, 85));
        }
        return jTextFieldBitOffset;
    }

    /**
     This method initializes jTextFieldByteOffset 
     
     @return javax.swing.JTextField jTextFieldByteOffset
     
     **/
    private JTextField getJTextFieldByteOffset() {
        if (jTextFieldByteOffset == null) {
            jTextFieldByteOffset = new JTextField();
            jTextFieldByteOffset.setLocation(new java.awt.Point(160, 60));
            jTextFieldByteOffset.setSize(new java.awt.Dimension(80, 20));
        }
        return jTextFieldByteOffset;
    }

    /**
     This method initializes jTextFieldBitSize 
     
     @return javax.swing.JTextField jTextFieldOffsetBitSize
     
     **/
    private JTextField getJTextFieldOffsetBitSize() {
        if (jTextFieldOffsetBitSize == null) {
            jTextFieldOffsetBitSize = new JTextField();
            jTextFieldOffsetBitSize.setSize(new java.awt.Dimension(80, 20));
            jTextFieldOffsetBitSize.setLocation(new java.awt.Point(160, 110));
        }
        return jTextFieldOffsetBitSize;
    }

    /**
     This method initializes jComboBoxUsage 
     
     @return javax.swing.JComboBox jComboBoxUsage
     
     **/
    private JComboBox getJComboBoxUsage() {
        if (jComboBoxUsage == null) {
            jComboBoxUsage = new JComboBox();
            jComboBoxUsage.setBounds(new java.awt.Rectangle(160, 135, 320, 20));
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
            jButtonOk.setText("Ok");
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
            jButtonGenerateGuid.setText("GEN");
            jButtonGenerateGuid.addActionListener(this);
            jButtonGenerateGuid.setBounds(new java.awt.Rectangle(415, 35, 65, 20));
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
    public ModuleVariables() {
        super();
        init();
        this.setVisible(true);
    }

    /**
     This is the override edit constructor
     
     @param inVariables The input data of VariablesDocument.Variables
     
     **/
    public ModuleVariables(VariablesDocument.Variables inVariables) {
        super();
        init(inVariables);
        this.setVisible(true);
    }

    /**
     This is the override edit constructor
     
     @param inVariables The input data of VariablesDocument.Variables
     @param type The input data of node type
     @param index The input data of node index
     
     **/
    public ModuleVariables(VariablesDocument.Variables inVariables, int type, int index) {
        super();
        init(inVariables, type, index);
        this.setVisible(true);
    }

    /**
     This method initializes this
     
     @param inVariables The input data of VariablesDocument.Variables
     
     **/
    private void init(VariablesDocument.Variables inVariables) {
        init();
        this.setVariables(inVariables);
    }

    /**
     This method initializes this
     Fill values to all fields if these values are not empty
     
     @param inVariables The input data of VariablesDocument.Variables
     @param type The input data of node type
     @param index The input data of node index
     
     **/
    private void init(VariablesDocument.Variables inVariables, int type, int index) {
        init(inVariables);
        this.location = index;
        if (this.variables.getVariableList().size() > 0) {
            if (this.variables.getVariableArray(index).getString() != null) {
                this.jTextFieldString.setText(this.variables.getVariableArray(index).getString());
            }
            if (this.variables.getVariableArray(index).getGuid() != null) {
                this.jTextFieldGuid.setText(this.variables.getVariableArray(index).getGuid().getStringValue());
            }
            if (this.variables.getVariableArray(index).getByteOffset() != null) {
                this.jTextFieldByteOffset
                                         .setText(String
                                                        .valueOf(this.variables.getVariableArray(index).getByteOffset()));
            }
            if (String.valueOf(this.variables.getVariableArray(index).getBitOffset()) != null) {
                this.jTextFieldBitOffset.setText(String.valueOf(this.variables.getVariableArray(index).getBitOffset()));
            }
            if (String.valueOf(this.variables.getVariableArray(index).getOffsetBitSize()) != null) {
                this.jTextFieldOffsetBitSize.setText(String.valueOf(this.variables.getVariableArray(index)
                                                                                  .getOffsetBitSize()));
            }
            if (this.variables.getVariableArray(index).getUsage() != null) {
                this.jComboBoxUsage.setSelectedItem(this.variables.getVariableArray(index).getUsage().toString());
            }
            this.jTextFieldOverrideID.setText(String.valueOf(this.variables.getVariableArray(index).getOverrideID()));
        }
    }

    /**
     This method initializes this
     
     @return void
     **/
    private void init() {
        this.setSize(500, 515);
        this.setContentPane(getJContentPane());
        this.setTitle("Add Variables");
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
            this.jTextFieldString.setEnabled(!isView);
            this.jTextFieldGuid.setEnabled(!isView);
            this.jTextFieldByteOffset.setEnabled(!isView);
            this.jTextFieldBitOffset.setEnabled(!isView);
            this.jTextFieldOffsetBitSize.setEnabled(!isView);
            this.jComboBoxUsage.setEnabled(!isView);
            this.jButtonGenerateGuid.setEnabled(!isView);
            this.jTextFieldOverrideID.setEnabled(!isView);
        }
    }

    /**
     This method initializes jContentPane
     
     @return javax.swing.JPanel jContentPane
     
     **/
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jLabelByteOffsetHint = new JLabel();
            jLabelByteOffsetHint.setBounds(new java.awt.Rectangle(245,60,235,20));
            jLabelByteOffsetHint.setText("0x00");
            jLabelOffsetBitSizeHint = new JLabel();
            jLabelOffsetBitSizeHint.setBounds(new java.awt.Rectangle(245,110,235,20));
            jLabelOffsetBitSizeHint.setText("1~7");
            jLabelBitOffsetHint = new JLabel();
            jLabelBitOffsetHint.setBounds(new java.awt.Rectangle(245,85,235,20));
            jLabelBitOffsetHint.setText("0~7");
            jLabelOverrideID = new JLabel();
            jLabelOverrideID.setText("Override ID");
            jLabelOverrideID.setBounds(new java.awt.Rectangle(15, 160, 140, 20));
            jLabelUsage = new JLabel();
            jLabelUsage.setText("Usage");
            jLabelUsage.setBounds(new java.awt.Rectangle(15, 135, 140, 20));
            jLabelOffsetBitSize = new JLabel();
            jLabelOffsetBitSize.setText("Offset Bit Size");
            jLabelOffsetBitSize.setLocation(new java.awt.Point(15, 110));
            jLabelOffsetBitSize.setSize(new java.awt.Dimension(140, 20));
            jLabelByteOffset = new JLabel();
            jLabelByteOffset.setText("Byte Offset");
            jLabelByteOffset.setLocation(new java.awt.Point(15, 60));
            jLabelByteOffset.setSize(new java.awt.Dimension(140, 20));
            jLabelBitOffset = new JLabel();
            jLabelBitOffset.setText("Bit Offset");
            jLabelBitOffset.setLocation(new java.awt.Point(15, 85));
            jLabelBitOffset.setSize(new java.awt.Dimension(140, 20));
            jLabelString = new JLabel();
            jLabelString.setText("String");
            jLabelString.setLocation(new java.awt.Point(15, 10));
            jLabelString.setSize(new java.awt.Dimension(140, 20));
            jLabelGuid = new JLabel();
            jLabelGuid.setText("Guid");
            jLabelGuid.setLocation(new java.awt.Point(15, 35));
            jLabelGuid.setSize(new java.awt.Dimension(140, 20));
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.add(getJButtonGenerateGuid(), null);
            jContentPane.add(jLabelString, null);
            jContentPane.add(getJTextFieldString(), null);
            jContentPane.add(jLabelGuid, null);
            jContentPane.add(getJTextFieldGuid(), null);
            jContentPane.add(jLabelByteOffset, null);
            jContentPane.add(getJTextFieldByteOffset(), null);
            jContentPane.add(jLabelBitOffset, null);
            jContentPane.add(getJTextFieldBitOffset(), null);
            jContentPane.add(jLabelOffsetBitSize, null);
            jContentPane.add(getJTextFieldOffsetBitSize(), null);
            jContentPane.add(jLabelUsage, null);
            jContentPane.add(getJComboBoxUsage(), null);
            jContentPane.add(jLabelOverrideID, null);
            jContentPane.add(getJTextFieldOverrideID(), null);
            jContentPane.add(getJButtonOk(), null);
            jContentPane.add(getJButtonCancel(), null);

            jStarLabel1 = new StarLabel();
            jStarLabel1.setLocation(new java.awt.Point(0, 10));
            jStarLabel2 = new StarLabel();
            jStarLabel2.setLocation(new java.awt.Point(0, 35));

            jContentPane.add(jStarLabel1, null);
            jContentPane.add(jStarLabel2, null);

            jContentPane.add(jLabelByteOffsetHint, null);
            jContentPane.add(jLabelBitOffsetHint, null);
            jContentPane.add(jLabelOffsetBitSizeHint, null);
            initFrame();
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
     This method initializes Usage type
     
     **/
    private void initFrame() {
        jComboBoxUsage.addItem("ALWAYS_CONSUMED");
        jComboBoxUsage.addItem("SOMETIMES_CONSUMED");
        jComboBoxUsage.addItem("ALWAYS_PRODUCED");
        jComboBoxUsage.addItem("SOMETIMES_PRODUCED");
        jComboBoxUsage.addItem("PRIVATE");
    }

    /**
     Get VariablesDocument.Variables
     
     @return VariablesDocument.Variables
     
     **/
    public VariablesDocument.Variables getVariables() {
        return variables;
    }

    /**
     Set VariablesDocument.Variables
     
     @param variables The input data of VariablesDocument.Variables
     
     **/
    public void setVariables(VariablesDocument.Variables variables) {
        this.variables = variables;
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
        if (isEmpty(this.jTextFieldString.getText())) {
            Log.err("String couldn't be empty");
            return false;
        }
        if (isEmpty(this.jTextFieldGuid.getText())) {
            Log.err("Guid couldn't be empty");
            return false;
        }

        //
        // Check if all fields have correct data types 
        //
        if (!isEmpty(this.jTextFieldGuid.getText()) && !DataValidation.isGuid(this.jTextFieldGuid.getText())) {
            Log.err("Incorrect data type for Guid");
            return false;
        }
        if (!isEmpty(this.jTextFieldByteOffset.getText())
            && !DataValidation.isByteOffset(this.jTextFieldByteOffset.getText())) {
            Log.err("Incorrect data type for Byte Offset");
            return false;
        }
        if (!isEmpty(this.jTextFieldBitOffset.getText())
            && !DataValidation.isBitOffset(this.jTextFieldBitOffset.getText())) {
            Log.err("Incorrect data type for Bit Offset");
            return false;
        }
        if (!isEmpty(this.jTextFieldOffsetBitSize.getText())
            && !DataValidation.isOffsetBitSize(this.jTextFieldOffsetBitSize.getText())) {
            Log.err("Incorrect data type for Bit Offset");
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
     Save all components of Variables
     if exists variables, set the value directly
     if not exists variables, new an instance first
     
     **/
    public void save() {
        try {
            if (this.variables == null) {
                variables = VariablesDocument.Variables.Factory.newInstance();
            }
            VariablesDocument.Variables.Variable variable = VariablesDocument.Variables.Variable.Factory.newInstance();
            if (!isEmpty(this.jTextFieldString.getText())) {
                variable.setString(this.jTextFieldString.getText());
            }
            if (!isEmpty(this.jTextFieldGuid.getText())) {
                GuidDocument.Guid guid = GuidDocument.Guid.Factory.newInstance();
                guid.setStringValue(this.jTextFieldGuid.getText());
                variable.setGuid(guid);
            }
            if (!isEmpty(this.jTextFieldByteOffset.getText())) {
                variable.setByteOffset(this.jTextFieldByteOffset.getText());
            }
            if (!isEmpty(this.jTextFieldBitOffset.getText())) {
                variable.setBitOffset(Integer.parseInt(this.jTextFieldBitOffset.getText()));
            }
            if (!isEmpty(this.jTextFieldBitOffset.getText())) {
                variable.setOffsetBitSize(Integer.parseInt(this.jTextFieldBitOffset.getText()));
            }
            variable.setUsage(VariableUsage.Enum.forString(jComboBoxUsage.getSelectedItem().toString()));
            if (!isEmpty(this.jTextFieldOverrideID.getText())) {
                variable.setOverrideID(Integer.parseInt(this.jTextFieldOverrideID.getText()));
            }
            if (location > -1) {
                variables.setVariableArray(location, variable);
            } else {
                variables.addNewVariable();
                variables.setVariableArray(variables.getVariableList().size() - 1, variable);
            }
        } catch (Exception e) {
            Log.err("Update Variables", e.getMessage());
        }
    }
}
