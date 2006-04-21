/** @file

 The file is used to create, update SystemTable of MSA/MBD file
 
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

import org.tianocore.SystemTableUsage;
import org.tianocore.SystemTablesDocument;
import org.tianocore.common.DataValidation;
import org.tianocore.common.Log;
import org.tianocore.packaging.common.ui.IInternalFrame;
import org.tianocore.packaging.common.ui.StarLabel;

/**
 The class is used to create, update SystemTable of MSA/MBD file
 It extends IInternalFrame
 
 @since ModuleEditor 1.0

 **/
public class ModuleSystemTables extends IInternalFrame {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = 7488769180379442276L;

    //
    //Define class members
    //
    private SystemTablesDocument.SystemTables systemTables = null;

    private int location = -1;

    private JPanel jContentPane = null;

    private JLabel jLabelEntry = null;

    private JTextField jTextFieldEntry = null;

    private JLabel jLabelUsage = null;

    private JComboBox jComboBoxUsage = null;

    private JButton jButtonOk = null;

    private JButton jButtonCancel = null;

    private JLabel jLabelOverrideID = null;

    private JTextField jTextFieldOverrideID = null;

    private StarLabel jStarLabel1 = null;

    /**
     This method initializes jTextFieldEntry 
     
     @return javax.swing.JTextField jTextFieldEntry
     
     **/
    private JTextField getJTextFieldEntry() {
        if (jTextFieldEntry == null) {
            jTextFieldEntry = new JTextField();
            jTextFieldEntry.setBounds(new java.awt.Rectangle(160, 10, 320, 20));
        }
        return jTextFieldEntry;
    }

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
            jButtonOk.setBounds(new java.awt.Rectangle(280, 90, 90, 20));
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
            jButtonCancel.setBounds(new java.awt.Rectangle(390, 90, 90, 20));
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
            jTextFieldOverrideID.setBounds(new java.awt.Rectangle(160, 60, 320, 20));
        }
        return jTextFieldOverrideID;
    }

    public static void main(String[] args) {

    }

    /**
     This is the default constructor
     
     **/
    public ModuleSystemTables() {
        super();
        init();
        this.setVisible(true);
    }

    /**
     This is the override edit constructor
     
     @param inSystemTables The input data of SystemTablesDocument.SystemTables
     
     **/
    public ModuleSystemTables(SystemTablesDocument.SystemTables inSystemTables) {
        super();
        init(inSystemTables);
        this.setVisible(true);
    }

    /**
     This is the override edit constructor
     
     @param inSystemTables The input data of SystemTablesDocument.SystemTables
     @param type The input data of node type
     @param index The input data of node index
     
     **/
    public ModuleSystemTables(SystemTablesDocument.SystemTables inSystemTables, int type, int index) {
        super();
        init(inSystemTables, type, index);
        this.setVisible(true);
    }

    /**
     This method initializes this
     
     @param inSystemTables The input data of SystemTablesDocument.SystemTables
     
     **/
    private void init(SystemTablesDocument.SystemTables inSystemTables) {
        init();
        this.setSystemTables(inSystemTables);
    }

    /**
     This method initializes this
     Fill values to all fields if these values are not empty
     
     @param inSystemTables The input data of SystemTablesDocument.SystemTables
     @param type The input data of node type
     @param index The input data of node index
     
     **/
    private void init(SystemTablesDocument.SystemTables inSystemTables, int type, int index) {
        init(inSystemTables);
        this.location = index;
        if (this.systemTables.getSystemTableList().size() > 0) {
            if (this.systemTables.getSystemTableArray(index).getEntry() != null) {
                this.jTextFieldEntry.setText(this.systemTables.getSystemTableArray(index).getEntry());
            }
            if (this.systemTables.getSystemTableArray(index).getUsage() != null) {
                this.jComboBoxUsage.setSelectedItem(this.systemTables.getSystemTableArray(index).getUsage().toString());
            }
            this.jTextFieldOverrideID.setText(String.valueOf(this.systemTables.getSystemTableArray(index)
                                                                              .getOverrideID()));
        }
    }

    /**
     This method initializes this
     
     @return void
     **/
    private void init() {
        this.setSize(500, 515);
        this.setContentPane(getJContentPane());
        this.setTitle("System Tables");
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
            this.jTextFieldEntry.setEnabled(!isView);
            this.jComboBoxUsage.setEnabled(!isView);
            this.jTextFieldOverrideID.setEnabled(!isView);
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
            jLabelEntry = new JLabel();
            jLabelEntry.setText("Entry");
            jLabelEntry.setBounds(new java.awt.Rectangle(15, 10, 140, 20));
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.add(jLabelEntry, null);
            jContentPane.add(getJTextFieldEntry(), null);
            jContentPane.add(jLabelUsage, null);
            jContentPane.add(getJComboBoxUsage(), null);
            jContentPane.add(getJButtonOk(), null);
            jContentPane.add(getJButtonCancel(), null);
            jContentPane.add(jLabelOverrideID, null);
            jContentPane.add(getJTextFieldOverrideID(), null);

            jStarLabel1 = new StarLabel();
            jStarLabel1.setBounds(new java.awt.Rectangle(0, 10, 10, 20));

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
     *  Override actionPerformed to listen all actions
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

    /**
     Get SystemTablesDocument.SystemTables
     
     @return SystemTablesDocument.SystemTables
     
     **/
    public SystemTablesDocument.SystemTables getSystemTables() {
        return systemTables;
    }

    /**
     Set SystemTablesDocument.SystemTables
     
     @param systemTables The input data of SystemTablesDocument.SystemTables
     
     **/
    public void setSystemTables(SystemTablesDocument.SystemTables systemTables) {
        this.systemTables = systemTables;
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
        if (isEmpty(this.jTextFieldEntry.getText())) {
            Log.err("Entry couldn't be empty");
            return false;
        }

        //
        // Check if all fields have correct data types 
        //
        if (!isEmpty(this.jTextFieldOverrideID.getText())
            && !DataValidation.isOverrideID(this.jTextFieldOverrideID.getText())) {
            Log.err("Incorrect data type for Override ID");
            return false;
        }

        return true;
    }

    /**
     Save all components of SystemTables
     if exists systemTables, set the value directly
     if not exists systemTables, new an instance first
     
     **/
    public void save() {
        try {
            if (this.systemTables == null) {
                systemTables = SystemTablesDocument.SystemTables.Factory.newInstance();
            }
            SystemTablesDocument.SystemTables.SystemTable systemTable = SystemTablesDocument.SystemTables.SystemTable.Factory
                                                                                                                             .newInstance();
            systemTable.setEntry(this.jTextFieldEntry.getText());

            systemTable.setUsage(SystemTableUsage.Enum.forString(jComboBoxUsage.getSelectedItem().toString()));
            if (!isEmpty(this.jTextFieldOverrideID.getText())) {
                systemTable.setOverrideID(Integer.parseInt(this.jTextFieldOverrideID.getText()));
            }
            if (location > -1) {
                systemTables.setSystemTableArray(location, systemTable);
            } else {
                systemTables.addNewSystemTable();
                systemTables.setSystemTableArray(systemTables.getSystemTableList().size() - 1, systemTable);
            }
        } catch (Exception e) {
            Log.err("Update System Tables", e.getMessage());
        }
    }
}
