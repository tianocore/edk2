/** @file
 
 The file is used to create, update Guids of MSA/MBD file
 
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

import org.tianocore.ConditionalExpressionDocument;
import org.tianocore.DefaultValueDocument;
import org.tianocore.GuidUsage;
import org.tianocore.GuidsDocument;
import org.tianocore.common.DataValidation;
import org.tianocore.common.Log;
import org.tianocore.common.Tools;
import org.tianocore.packaging.common.ui.IComboBox;
import org.tianocore.packaging.common.ui.IInternalFrame;
import org.tianocore.packaging.common.ui.StarLabel;

/**
 The class is used to create, update Guids of MSA/MBD file
 It extends IInternalFrame
 
 @since ModuleEditor 1.0

 **/
public class ModuleGuids extends IInternalFrame {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = 6710858997766979803L;

    //
    //Define class members
    //
    private GuidsDocument.Guids guids = null;

    private int location = -1;

    private JPanel jContentPane = null;

    private JLabel jLabelC_Name = null;

    private JTextField jTextFieldC_Name = null;

    private JLabel jLabelGuidValue = null;

    private JTextField jTextFieldGuidValue = null;

    private JLabel jLabelFeatureFlag = null;

    private IComboBox iComboBoxFeatureFlag = null;

    private JLabel jLabelConditionalExpression = null;

    private IComboBox iComboBoxConditionalExpression = null;

    private JLabel jLabelDefault = null;

    private JTextField jTextFieldDefaultValue = null;

    private JLabel jLabelHelpText = null;

    private JTextField jTextFieldHelpText = null;

    private JLabel jLabelEnableFeature = null;

    private JRadioButton jRadioButtonEnableFeature = null;

    private JRadioButton jRadioButtonDisableFeature = null;

    private JLabel jLabelUsage = null;

    private JComboBox jComboBoxUsage = null;

    private JButton jButtonOk = null;

    private JButton jButtonCancel = null;

    private JButton jButtonGenerateGuid = null;

    private JLabel jLabelOverrideID = null;

    private JTextField jTextFieldOverrideID = null;

    /**
     This method initializes jTextFieldC_Name 
     
     @return javax.swing.JTextField jTextFieldC_Name
     
     **/
    private JTextField getJTextFieldC_Name() {
        if (jTextFieldC_Name == null) {
            jTextFieldC_Name = new JTextField();
            jTextFieldC_Name.setBounds(new java.awt.Rectangle(160, 10, 320, 20));
        }
        return jTextFieldC_Name;
    }

    /**
     This method initializes jTextFieldGuidValsue 
     
     @return javax.swing.JTextField jTextFieldGuidValue
     
     **/
    private JTextField getJTextFieldGuidValsue() {
        if (jTextFieldGuidValue == null) {
            jTextFieldGuidValue = new JTextField();
            jTextFieldGuidValue.setBounds(new java.awt.Rectangle(160, 35, 240, 20));
        }
        return jTextFieldGuidValue;
    }

    /**
     This method initializes jTextFieldFeatureFlag 
     
     @return javax.swing.JTextField iComboBoxFeatureFlag
     
     **/
    private IComboBox getIComboBoxFeatureFlag() {
        if (iComboBoxFeatureFlag == null) {
            iComboBoxFeatureFlag = new IComboBox();
            iComboBoxFeatureFlag.setBounds(new java.awt.Rectangle(160, 60, 320, 20));
        }
        return iComboBoxFeatureFlag;
    }

    /**
     This method initializes jTextFieldConditionalExpression 
     
     @return javax.swing.JTextField iComboBoxConditionalExpression
     
     **/
    private IComboBox getIComboBoxConditionalExpression() {
        if (iComboBoxConditionalExpression == null) {
            iComboBoxConditionalExpression = new IComboBox();
            iComboBoxConditionalExpression.setBounds(new java.awt.Rectangle(160, 85, 320, 20));
        }
        return iComboBoxConditionalExpression;
    }

    /**
     This method initializes jTextFieldDefault 
     
     @return javax.swing.JTextField jTextFieldDefaultValue
     
     **/
    private JTextField getJTextFieldDefaultValue() {
        if (jTextFieldDefaultValue == null) {
            jTextFieldDefaultValue = new JTextField();
            jTextFieldDefaultValue.setBounds(new java.awt.Rectangle(160, 110, 320, 20));
        }
        return jTextFieldDefaultValue;
    }

    /**
     This method initializes jTextFieldHelpText 
     
     @return javax.swing.JTextField jTextFieldHelpText
     
     **/
    private JTextField getJTextFieldHelpText() {
        if (jTextFieldHelpText == null) {
            jTextFieldHelpText = new JTextField();
            jTextFieldHelpText.setBounds(new java.awt.Rectangle(160, 135, 320, 20));
        }
        return jTextFieldHelpText;
    }

    /**
     This method initializes jRadioButtonEnableFeature 
     
     @return javax.swing.JRadioButton jRadioButtonEnableFeature
     
     **/
    private JRadioButton getJRadioButtonEnableFeature() {
        if (jRadioButtonEnableFeature == null) {
            jRadioButtonEnableFeature = new JRadioButton();
            jRadioButtonEnableFeature.setText("Enable");
            jRadioButtonEnableFeature.setBounds(new java.awt.Rectangle(160, 160, 90, 20));
            jRadioButtonEnableFeature.setSelected(true);
            jRadioButtonEnableFeature.addActionListener(this);
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
            jRadioButtonDisableFeature.setBounds(new java.awt.Rectangle(320, 160, 90, 20));
            jRadioButtonDisableFeature.addActionListener(this);
        }
        return jRadioButtonDisableFeature;
    }

    /**
     This method initializes jComboBoxUsage 
     
     @return javax.swing.JComboBox jComboBoxUsage
     
     **/
    private JComboBox getJComboBoxUsage() {
        if (jComboBoxUsage == null) {
            jComboBoxUsage = new JComboBox();
            jComboBoxUsage.setBounds(new java.awt.Rectangle(160, 185, 320, 20));
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
            jButtonOk.setBounds(new java.awt.Rectangle(290, 240, 90, 20));
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
            jButtonCancel.setBounds(new java.awt.Rectangle(390, 240, 90, 20));
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
            jButtonGenerateGuid.setBounds(new java.awt.Rectangle(405, 35, 75, 20));
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
            jTextFieldOverrideID.setBounds(new java.awt.Rectangle(160, 210, 320, 20));
        }
        return jTextFieldOverrideID;
    }

    public static void main(String[] args) {

    }

    /**
     This is the default constructor
     
     **/
    public ModuleGuids() {
        super();
        init();
        this.setVisible(true);
    }

    /**
     This is the override edit constructor
     
     @param inGuids The input data of GuidsDocument.Guids
     
     **/
    public ModuleGuids(GuidsDocument.Guids inGuids) {
        super();
        init(inGuids);
        this.setVisible(true);
    }

    /**
     This is the override edit constructor
     
     @param inGuids The input data of GuidsDocument.Guids
     @param type The input data of node type
     @param index The input data of node index
     
     **/
    public ModuleGuids(GuidsDocument.Guids inGuids, int type, int index) {
        super();
        init(inGuids, type, index);
        this.setVisible(true);
    }

    /**
     This method initializes this
     
     @param inGuids The input data of GuidsDocument.Guids
     
     **/
    private void init(GuidsDocument.Guids inGuids) {
        init();
        this.setGuids(inGuids);
    }

    /**
     This method initializes this
     Fill values to all fields if these values are not empty
     
     @param inGuids The input data of GuidsDocument.Guids
     @param type The input data of node type
     @param index The input data of node index
     
     **/
    private void init(GuidsDocument.Guids inGuids, int type, int index) {
        init(inGuids);
        this.location = index;
        if (this.guids.getGuidEntryList().size() > 0) {
            if (this.guids.getGuidEntryArray(index).getCName() != null) {
                this.jTextFieldC_Name.setText(this.guids.getGuidEntryArray(index).getCName());
            }
            if (this.guids.getGuidEntryArray(index).getGuidValue() != null) {
                this.jTextFieldGuidValue.setText(this.guids.getGuidEntryArray(index).getGuidValue());
            }
            if (this.guids.getGuidEntryArray(index).getFeatureFlagList().size() > 0) {
                for (int indexI = 0; indexI < this.guids.getGuidEntryArray(index).getFeatureFlagList().size(); indexI++) {
                    this.iComboBoxFeatureFlag.addItem(this.guids.getGuidEntryArray(index).getFeatureFlagArray(indexI));
                }
            }
            if (this.guids.getGuidEntryArray(index).getConditionalExpressionList().size() > 0) {
                for (int indexI = 0; indexI < this.guids.getGuidEntryArray(index).getConditionalExpressionArray(0)
                                                        .getConditionList().size(); indexI++) {
                    this.iComboBoxConditionalExpression.addItem(this.guids.getGuidEntryArray(index)
                                                                          .getConditionalExpressionArray(0)
                                                                          .getConditionArray(indexI));
                }
            }
            if (this.guids.getGuidEntryArray(index).getDefaultValue() != null) {
                this.jTextFieldDefaultValue.setText(this.guids.getGuidEntryArray(index).getDefaultValue()
                                                              .getStringValue());
            }
            if (this.guids.getGuidEntryArray(index).getHelpText() != null) {
                this.jTextFieldHelpText.setText(this.guids.getGuidEntryArray(index).getHelpText());
            }
            if (this.guids.getGuidEntryArray(index).getUsage() != null) {
                this.jComboBoxUsage.setSelectedItem(this.guids.getGuidEntryArray(index).getUsage().toString());
            }
            this.jRadioButtonEnableFeature.setSelected(this.guids.getGuidEntryArray(index).getEnableFeature());
            this.jRadioButtonDisableFeature.setSelected(!this.guids.getGuidEntryArray(index).getEnableFeature());
            this.jTextFieldOverrideID.setText(String.valueOf(this.guids.getGuidEntryArray(index).getOverrideID()));
        }
    }

    /**
     This method initializes this
     
     **/
    private void init() {
        this.setSize(500, 515);
        this.setContentPane(getJContentPane());
        this.setTitle("Guids");
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
            this.jTextFieldC_Name.setEnabled(!isView);
            this.jTextFieldGuidValue.setEnabled(!isView);
            this.iComboBoxFeatureFlag.setEnabled(!isView);
            this.iComboBoxConditionalExpression.setEnabled(!isView);
            this.jTextFieldDefaultValue.setEnabled(!isView);
            this.jTextFieldHelpText.setEnabled(!isView);
            this.jComboBoxUsage.setEnabled(!isView);
            this.jRadioButtonEnableFeature.setEnabled(!isView);
            this.jRadioButtonDisableFeature.setEnabled(!isView);
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
            jLabelOverrideID.setBounds(new java.awt.Rectangle(15, 210, 140, 20));
            jLabelOverrideID.setText("Override ID");
            jLabelUsage = new JLabel();
            jLabelUsage.setText("Usage");
            jLabelUsage.setBounds(new java.awt.Rectangle(15, 185, 140, 20));
            jLabelEnableFeature = new JLabel();
            jLabelEnableFeature.setText("Enable Feature");
            jLabelEnableFeature.setBounds(new java.awt.Rectangle(15, 160, 140, 20));
            jLabelHelpText = new JLabel();
            jLabelHelpText.setText("Help Text");
            jLabelHelpText.setBounds(new java.awt.Rectangle(15, 135, 140, 20));
            jLabelDefault = new JLabel();
            jLabelDefault.setText("Default Value");
            jLabelDefault.setBounds(new java.awt.Rectangle(15, 110, 140, 20));
            jLabelConditionalExpression = new JLabel();
            jLabelConditionalExpression.setText("Conditional Expression");
            jLabelConditionalExpression.setBounds(new java.awt.Rectangle(15, 85, 140, 20));
            jLabelFeatureFlag = new JLabel();
            jLabelFeatureFlag.setText("Feature Flag");
            jLabelFeatureFlag.setBounds(new java.awt.Rectangle(15, 60, 140, 20));
            jLabelGuidValue = new JLabel();
            jLabelGuidValue.setText("Guid Value");
            jLabelGuidValue.setBounds(new java.awt.Rectangle(15, 35, 140, 20));
            jLabelC_Name = new JLabel();
            jLabelC_Name.setText("C_Name");
            jLabelC_Name.setBounds(new java.awt.Rectangle(15, 10, 140, 20));
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.add(jLabelC_Name, null);
            jContentPane.add(getJTextFieldC_Name(), null);
            jContentPane.add(jLabelGuidValue, null);
            jContentPane.add(getJTextFieldGuidValsue(), null);
            jContentPane.add(jLabelFeatureFlag, null);
            jContentPane.add(getIComboBoxFeatureFlag(), null);
            jContentPane.add(jLabelConditionalExpression, null);
            jContentPane.add(getIComboBoxConditionalExpression(), null);
            jContentPane.add(jLabelDefault, null);
            jContentPane.add(getJTextFieldDefaultValue(), null);
            jContentPane.add(jLabelHelpText, null);
            jContentPane.add(getJTextFieldHelpText(), null);
            jContentPane.add(jLabelEnableFeature, null);
            jContentPane.add(getJRadioButtonEnableFeature(), null);
            jContentPane.add(getJRadioButtonDisableFeature(), null);
            jContentPane.add(jLabelUsage, null);
            jContentPane.add(getJComboBoxUsage(), null);
            jContentPane.add(getJButtonOk(), null);
            jContentPane.add(getJButtonCancel(), null);
            jContentPane.add(getJButtonGenerateGuid(), null);
            jContentPane.add(jLabelOverrideID, null);
            jContentPane.add(getJTextFieldOverrideID(), null);

            StarLabel jStarLabel1 = new StarLabel();
            jStarLabel1.setLocation(new java.awt.Point(0, 10));

            jContentPane.add(jStarLabel1, null);

            initFrame();

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
        jComboBoxUsage.addItem("DEFAULT");
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
            jTextFieldGuidValue.setText(Tools.generateUuidString());
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
    }

    /**
     Get GuidsDocument.Guids
     
     @return GuidsDocument.Guids
     
     **/
    public GuidsDocument.Guids getGuids() {
        return guids;
    }

    /**
     Set GuidsDocument.Guids
     
     @param guids The input GuidsDocument.Guids
     
     **/
    public void setGuids(GuidsDocument.Guids guids) {
        this.guids = guids;
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
        if (!isEmpty(this.jTextFieldGuidValue.getText()) && !DataValidation.isGuid(this.jTextFieldGuidValue.getText())) {
            Log.err("Incorrect data type for Guid Value");
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
     Save all components of Guids
     if exists guids, set the value directly
     if not exists guids, new an instance first
     
     **/
    public void save() {
        try {
            if (this.guids == null) {
                guids = GuidsDocument.Guids.Factory.newInstance();
            }
            GuidsDocument.Guids.GuidEntry guid = GuidsDocument.Guids.GuidEntry.Factory.newInstance();
            if (!isEmpty(this.jTextFieldC_Name.getText())) {
                guid.setCName(this.jTextFieldC_Name.getText());
            }
            if (!isEmpty(this.jTextFieldGuidValue.getText())) {
                guid.setGuidValue(this.jTextFieldGuidValue.getText());
            }
            if (this.iComboBoxFeatureFlag.getItemCount() > 0) {
                for (int index = 0; index < this.iComboBoxFeatureFlag.getItemCount(); index++) {
                    guid.addNewFeatureFlag();
                    guid.setFeatureFlagArray(index, this.iComboBoxFeatureFlag.getItemAt(index).toString());
                }
            }
            if (this.iComboBoxConditionalExpression.getItemCount() > 0) {
                ConditionalExpressionDocument.ConditionalExpression ce = ConditionalExpressionDocument.ConditionalExpression.Factory
                                                                                                                                    .newInstance();
                for (int index = 0; index < this.iComboBoxConditionalExpression.getItemCount(); index++) {
                    ce.addCondition(this.iComboBoxConditionalExpression.getItemAt(index).toString());
                }
                if (guid.getConditionalExpressionList().size() < 1) {
                    guid.addNewConditionalExpression();
                }
                guid.setConditionalExpressionArray(0, ce);
            }
            if (!isEmpty(this.jTextFieldDefaultValue.getText())) {
                DefaultValueDocument.DefaultValue dv = DefaultValueDocument.DefaultValue.Factory.newInstance();
                dv.setStringValue(this.jTextFieldDefaultValue.getText());
                guid.setDefaultValue(dv);
            }
            if (!isEmpty(this.jTextFieldHelpText.getText())) {
                guid.setHelpText(this.jTextFieldHelpText.getText());
            }
            guid.setUsage(GuidUsage.Enum.forString(jComboBoxUsage.getSelectedItem().toString()));
            guid.setEnableFeature(this.jRadioButtonEnableFeature.isSelected());
            if (!isEmpty(this.jTextFieldOverrideID.getText())) {
                guid.setOverrideID(Integer.parseInt(this.jTextFieldOverrideID.getText()));
            }

            if (location > -1) {
                guids.setGuidEntryArray(location, guid);
            } else {
                guids.addNewGuidEntry();
                guids.setGuidEntryArray(guids.getGuidEntryList().size() - 1, guid);
            }
        } catch (Exception e) {
            Log.err("Update Guids", e.getMessage());
        }
    }
}
