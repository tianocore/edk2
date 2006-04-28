/** @file
 
 The file is used to create, update Formset of MSA/MBD file
 
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

import org.tianocore.FormSetUsage;
import org.tianocore.FormsetsDocument;
import org.tianocore.common.DataValidation;
import org.tianocore.common.Log;
import org.tianocore.common.Tools;
import org.tianocore.packaging.common.ui.IInternalFrame;
import org.tianocore.packaging.common.ui.StarLabel;

/**
 The class is used to create, update Formset of MSA/MBD file
 It extends IInternalFrame
 
 @since ModuleEditor 1.0

 **/
public class ModuleFormsets extends IInternalFrame {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = -6851574146786158116L;

    //
    //Define class members
    //
    private FormsetsDocument.Formsets formsets = null;

    private int location = -1;

    private JPanel jContentPane = null;

    private JLabel jLabelName = null;

    private JTextField jTextFieldName = null;

    private JLabel jLabelGuid = null;

    private JTextField jTextFieldGuid = null;

    private JLabel jLabelUsage = null;

    private JButton jButtonOk = null;

    private JButton jButtonCancel = null;

    private JComboBox jComboBoxUsage = null;

    private JButton jButtonGenerateGuid = null;

    private JLabel jLabelOverrideID = null;

    private JTextField jTextFieldOverrideID = null;

    private StarLabel jStarLabel1 = null;

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
     This method initializes jTextFieldOverrideID 
     
     @return javax.swing.JTextField jTextFieldOverrideID
     
     **/
    private JTextField getJTextFieldOverrideID() {
        if (jTextFieldOverrideID == null) {
            jTextFieldOverrideID = new JTextField();
            jTextFieldOverrideID.setBounds(new java.awt.Rectangle(160, 85, 50, 20));
        }
        return jTextFieldOverrideID;
    }

    public static void main(String[] args) {

    }

    /**
     This is the default constructor
     
     **/
    public ModuleFormsets() {
        super();
        init();
        this.setVisible(true);
    }

    /**
     * 
     */
    /**
     This is the override edit constructor
     
     @param inFormsets The input data of FormsetsDocument.Formsets
     
     **/
    public ModuleFormsets(FormsetsDocument.Formsets inFormsets) {
        super();
        init(inFormsets);
        this.setVisible(true);
    }

    /**
     This is the override edit constructor
     
     @param inFormsets The input data of FormsetsDocument.Formsets
     @param type The input data of node type
     @param index The input data of node index
     
     **/
    public ModuleFormsets(FormsetsDocument.Formsets inFormsets, int type, int index) {
        super();
        init(inFormsets, type, index);
        this.setVisible(true);
    }

    /**
     This method initializes this
     
     @param inFormsets The input data of FormsetsDocument.Formsets
     
     **/
    private void init(FormsetsDocument.Formsets inFormsets) {
        init();
        this.setFormsets(inFormsets);
    }

    /**
     This method initializes this
     Fill values to all fields if these values are not empty
     
     @param inFormsets The input data of FormsetsDocument.Formsets
     @param type The input data of node type
     @param index The input data of node index
     
     **/
    private void init(FormsetsDocument.Formsets inFormsets, int type, int index) {
        init(inFormsets);
        this.location = index;
        if (this.formsets.getFormsetList().size() > 0) {
            if (this.formsets.getFormsetArray(index).getStringValue() != null) {
                this.jTextFieldName.setText(this.formsets.getFormsetArray(index).getStringValue().toString());
            }
            if (this.formsets.getFormsetArray(index).getGuid() != null) {
                this.jTextFieldGuid.setText(this.formsets.getFormsetArray(index).getGuid());
            }
            if (this.formsets.getFormsetArray(index).getUsage() != null) {
                this.jComboBoxUsage.setSelectedItem(this.formsets.getFormsetArray(index).getUsage().toString());
            }
            this.jTextFieldOverrideID.setText(String.valueOf(this.formsets.getFormsetArray(index).getOverrideID()));
        }
    }

    /**
     This method initializes this
     
     **/
    private void init() {
        this.setContentPane(getJContentPane());
        this.setTitle("Form Sets");
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
            this.jTextFieldName.setEnabled(!isView);
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
            jLabelGuid = new JLabel();
            jLabelGuid.setText("Guid");
            jLabelGuid.setBounds(new java.awt.Rectangle(15, 35, 140, 20));
            jLabelName = new JLabel();
            jLabelName.setText("Name");
            jLabelName.setBounds(new java.awt.Rectangle(15, 10, 140, 20));
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.add(jLabelName, null);
            jContentPane.add(getJTextFieldName(), null);
            jContentPane.add(jLabelGuid, null);
            jContentPane.add(getJTextFieldGuid(), null);
            jContentPane.add(jLabelUsage, null);
            jContentPane.add(getJButtonOk(), null);
            jContentPane.add(getJButtonCancel(), null);
            jContentPane.add(getJComboBoxUsage(), null);
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
     This method initializes Usage type
     
     **/
    private void initFrame() {
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

        if (arg0.getSource() == jButtonGenerateGuid) {
            jTextFieldGuid.setText(Tools.generateUuidString());
        }
    }

    /**
     Set FormsetsDocument.Formsets
     
     @return FormsetsDocument.Formsets
     
     **/
    public FormsetsDocument.Formsets getFormsets() {
        return formsets;
    }

    /**
     Get FormsetsDocument.Formsets
     
     @param formsets The input FormsetsDocument.Formsets
     
     **/
    public void setFormsets(FormsetsDocument.Formsets formsets) {
        this.formsets = formsets;
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
        if (!DataValidation.isCName(this.jTextFieldName.getText())) {
            Log.err("Incorrect data type for Name");
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
     Save all components of Formsets
     if exists formset, set the value directly
     if not exists formset, new an instance first
     
     **/
    public void save() {
        try {
            if (this.formsets == null) {
                formsets = FormsetsDocument.Formsets.Factory.newInstance();
            }
            FormsetsDocument.Formsets.Formset formset = FormsetsDocument.Formsets.Formset.Factory.newInstance();
            if (!isEmpty(this.jTextFieldName.getText())) {
                formset.setStringValue(this.jTextFieldName.getText());
            }
            if (!isEmpty(this.jTextFieldGuid.getText())) {
                formset.setGuid(this.jTextFieldGuid.getText());
            }
            formset.setUsage(FormSetUsage.Enum.forString(jComboBoxUsage.getSelectedItem().toString()));
            if (!isEmpty(this.jTextFieldOverrideID.getText())) {
                formset.setOverrideID(Integer.parseInt(this.jTextFieldOverrideID.getText()));
            }
            if (location > -1) {
                formsets.setFormsetArray(location, formset);
            } else {
                formsets.addNewFormset();
                formsets.setFormsetArray(formsets.getFormsetList().size() - 1, formset);
            }
        } catch (Exception e) {
            Log.err("Update Formsets", e.getMessage());
        }
    }
}
