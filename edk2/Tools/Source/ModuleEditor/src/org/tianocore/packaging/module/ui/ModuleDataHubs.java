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

import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JTextField;

import org.tianocore.DataHubUsage;
import org.tianocore.DataHubsDocument;
import org.tianocore.common.DataValidation;
import org.tianocore.common.Log;
import org.tianocore.common.Tools;
import org.tianocore.packaging.common.ui.IInternalFrame;
import org.tianocore.packaging.common.ui.StarLabel;

/**
 The class is used to create, update DataHub of MSA/MBD file
 It extends IInternalFrame
 
 @since ModuleEditor 1.0

 **/
public class ModuleDataHubs extends IInternalFrame {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = -3667906991966638892L;

    //
    //Define class members
    //
    private DataHubsDocument.DataHubs dataHubs = null;

    private int location = -1;

    private JPanel jContentPane = null;

    private JLabel jLabelGuid = null;

    private JTextField jTextFieldGuid = null;

    private JLabel jLabelUsage = null;

    private JComboBox jComboBoxUsage = null;

    private JButton jButtonOk = null;

    private JButton jButtonCancel = null;

    private JLabel jLabelDataHubRecord = null;

    private JTextField jTextFieldDataHubRecord = null;

    private JButton jButtonGenerateGuid = null;

    private JLabel jLabelOverrideID = null;

    private JTextField jTextFieldOverrideID = null;

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
     This method initializes jTextFieldDataHubRecord 
     
     @return javax.swing.JTextField jTextFieldDataHubRecord
     
     **/
    private JTextField getJTextFieldDataHubRecord() {
        if (jTextFieldDataHubRecord == null) {
            jTextFieldDataHubRecord = new JTextField();
            jTextFieldDataHubRecord.setBounds(new java.awt.Rectangle(160, 10, 320, 20));
        }
        return jTextFieldDataHubRecord;
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
            jTextFieldOverrideID.setBounds(new java.awt.Rectangle(160, 85, 320, 20));
        }
        return jTextFieldOverrideID;
    }

    public static void main(String[] args) {

    }

    /**
     This is the default constructor
     
     **/
    public ModuleDataHubs() {
        super();
        init();
        this.setVisible(true);
    }

    /**
     This is the override edit constructor
     
     @param inDataHubs The input DataHubsDocument.DataHubs
     
     **/
    public ModuleDataHubs(DataHubsDocument.DataHubs inDataHubs) {
        super();
        init(inDataHubs);
        this.setVisible(true);
    }

    /**
     This is the override edit constructor
     
     @param inDataHubs DataHubsDocument.DataHubs
     @param type The input data of node type
     @param index The input data of node index
     
     **/
    public ModuleDataHubs(DataHubsDocument.DataHubs inDataHubs, int type, int index) {
        super();
        init(inDataHubs, type, index);
        this.setVisible(true);
    }

    /**
     This method initializes this
     
     @param inDataHubs The input DataHubsDocument.DataHubs
     
     **/
    private void init(DataHubsDocument.DataHubs inDataHubs) {
        init();
        this.setDataHubs(inDataHubs);
    }

    /**
     This method initializes this
     Fill values to all fields if these values are not empty
     
     @param inDataHubs The input DataHubsDocument.DataHubs
     @param type The input data of node type
     @param index The input data of node index
     
     **/
    private void init(DataHubsDocument.DataHubs inDataHubs, int type, int index) {
        init(inDataHubs);
        this.location = index;
        if (this.dataHubs.getDataHubRecordList().size() > 0) {
            if (this.dataHubs.getDataHubRecordArray(index).getStringValue() != null) {
                this.jTextFieldDataHubRecord.setText(this.dataHubs.getDataHubRecordArray(index).getStringValue()
                                                                  .toString());
            }
            if (this.dataHubs.getDataHubRecordArray(index).getGuid() != null) {
                this.jTextFieldGuid.setText(this.dataHubs.getDataHubRecordArray(index).getGuid());
            }
            if (this.dataHubs.getDataHubRecordArray(index).getUsage() != null) {
                this.jComboBoxUsage.setSelectedItem(this.dataHubs.getDataHubRecordArray(index).getUsage().toString());
            }
            this.jTextFieldOverrideID
                                     .setText(String
                                                    .valueOf(this.dataHubs.getDataHubRecordArray(index).getOverrideID()));
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
            this.jTextFieldDataHubRecord.setEnabled(!isView);
            this.jTextFieldGuid.setEnabled(!isView);
            this.jComboBoxUsage.setEnabled(!isView);
            this.jTextFieldOverrideID.setEnabled(!isView);
            this.jButtonCancel.setEnabled(!isView);
            this.jButtonGenerateGuid.setEnabled(!isView);
            this.jButtonOk.setEnabled(!isView);
        }
    }

    /**
     This method initializes this
     
     **/
    private void init() {
        this.setSize(500, 515);
        this.setContentPane(getJContentPane());
        this.setTitle("Data Hubs");
        initFrame();
        this.setViewMode(false);
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
            jLabelDataHubRecord = new JLabel();
            jLabelDataHubRecord.setBounds(new java.awt.Rectangle(15, 10, 140, 20));
            jLabelDataHubRecord.setText("Data Hub Record");
            jLabelGuid = new JLabel();
            jLabelGuid.setText("Guid");
            jLabelGuid.setBounds(new java.awt.Rectangle(15, 35, 140, 20));
            jLabelUsage = new JLabel();
            jLabelUsage.setText("Usage");
            jLabelUsage.setBounds(new java.awt.Rectangle(15, 60, 140, 20));
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.add(jLabelGuid, null);
            jContentPane.add(getJTextFieldGuid(), null);
            jContentPane.add(jLabelUsage, null);
            jContentPane.add(getJComboBoxUsage(), null);
            jContentPane.add(getJButtonOk(), null);
            jContentPane.add(getJButtonCancel(), null);
            jContentPane.add(jLabelDataHubRecord, null);
            jContentPane.add(getJTextFieldDataHubRecord(), null);
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

        if (arg0.getSource() == jButtonGenerateGuid) {
            jTextFieldGuid.setText(Tools.generateUuidString());
        }
    }

    /**
     Get DataHubsDocument.DataHubs
     
     @return DataHubsDocument.DataHubs
     
     **/
    public DataHubsDocument.DataHubs getDataHubs() {
        return dataHubs;
    }

    /**
     Set DataHubsDocument.DataHubs
     
     @param dataHubs DataHubsDocument.DataHubs
     
     **/
    public void setDataHubs(DataHubsDocument.DataHubs dataHubs) {
        this.dataHubs = dataHubs;
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
        if (isEmpty(this.jTextFieldDataHubRecord.getText())) {
            Log.err("Data Hub Record couldn't be empty");
            return false;
        }

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
     Save all components of DataHubs
     if exists dataHubs, set the value directly
     if not exists dataHubs, new an instance first
     
     **/
    public void save() {
        try {
            if (this.dataHubs == null) {
                dataHubs = DataHubsDocument.DataHubs.Factory.newInstance();
            }
            DataHubsDocument.DataHubs.DataHubRecord dataHubRecord = DataHubsDocument.DataHubs.DataHubRecord.Factory
                                                                                                                   .newInstance();
            if (!isEmpty(this.jTextFieldDataHubRecord.getText())) {
                dataHubRecord.setStringValue(this.jTextFieldDataHubRecord.getText());
            }
            if (!isEmpty(this.jTextFieldGuid.getText())) {
                dataHubRecord.setGuid(this.jTextFieldGuid.getText());
            }
            dataHubRecord.setUsage(DataHubUsage.Enum.forString(jComboBoxUsage.getSelectedItem().toString()));
            if (!isEmpty(this.jTextFieldOverrideID.getText())) {
                dataHubRecord.setOverrideID(Integer.parseInt(this.jTextFieldOverrideID.getText()));
            }
            if (location > -1) {
                dataHubs.setDataHubRecordArray(location, dataHubRecord);
            } else {
                dataHubs.addNewDataHubRecord();
                dataHubs.setDataHubRecordArray(dataHubs.getDataHubRecordList().size() - 1, dataHubRecord);
            }
        } catch (Exception e) {
            Log.err("Update Data Hubs", e.getMessage());
        }
    }
}
