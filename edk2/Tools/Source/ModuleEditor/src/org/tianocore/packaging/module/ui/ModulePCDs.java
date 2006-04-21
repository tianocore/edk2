/** @file
 
 The file is used to create, update PCD of MSA/MBD file
 
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

import org.tianocore.PCDsDocument;
import org.tianocore.PcdDataTypes;
import org.tianocore.PcdItemTypes;
import org.tianocore.PcdUsage;
import org.tianocore.common.DataValidation;
import org.tianocore.common.Log;
import org.tianocore.packaging.common.ui.IInternalFrame;
import org.tianocore.packaging.common.ui.StarLabel;

/**
 The class is used to create, update PCD of MSA/MBD file
 It extends IInternalFrame
 
 @since ModuleEditor 1.0

 **/
public class ModulePCDs extends IInternalFrame {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = 2227717658188438696L;

    //
    //Define class members
    //
    private PCDsDocument.PCDs pcds = null;

    private int location = -1;

    private JPanel jContentPane = null;

    private JLabel jLabelItemType = null;

    private JLabel jLabelC_Name = null;

    private JComboBox jComboBoxItemType = null;

    private JTextField jTextFieldC_Name = null;

    private JLabel jLabelToken = null;

    private JTextField jTextFieldToken = null;

    private JLabel jLabelDefaultValue = null;

    private JTextField jTextFieldDefaultValue = null;

    private JLabel jLabelUsage = null;

    private JComboBox jComboBoxUsage = null;

    private JButton jButtonOk = null;

    private JButton jButtonCancel = null;

    private JLabel jLabelDatumType = null;

    private JComboBox jComboBoxDatumType = null;

    private StarLabel jStarLabel1 = null;

    private StarLabel jStarLabel2 = null;

    private StarLabel jStarLabel3 = null;

    private StarLabel jStarLabel4 = null;
    
    /**
     This method initializes jComboBoxItemType 
     
     @return javax.swing.JComboBox jComboBoxItemType
     
     **/
    private JComboBox getJComboBoxItemType() {
        if (jComboBoxItemType == null) {
            jComboBoxItemType = new JComboBox();
            jComboBoxItemType.setBounds(new java.awt.Rectangle(160, 110, 320, 20));
        }
        return jComboBoxItemType;
    }

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
     This method initializes jTextFieldToken 
     
     @return javax.swing.JTextField jTextFieldToken
     
     **/
    private JTextField getJTextFieldToken() {
        if (jTextFieldToken == null) {
            jTextFieldToken = new JTextField();
            jTextFieldToken.setBounds(new java.awt.Rectangle(160, 35, 320, 20));
        }
        return jTextFieldToken;
    }

    /**
     This method initializes jTextFieldDefaultValue 
     
     @return javax.swing.JTextField jTextFieldDefaultValue
     
     **/
    private JTextField getJTextFieldDefaultValue() {
        if (jTextFieldDefaultValue == null) {
            jTextFieldDefaultValue = new JTextField();
            jTextFieldDefaultValue.setBounds(new java.awt.Rectangle(160, 85, 320, 20));
        }
        return jTextFieldDefaultValue;
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
            jButtonOk.setText("OK");
            jButtonOk.setBounds(new java.awt.Rectangle(280, 290, 90, 20));
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
            jButtonCancel.setBounds(new java.awt.Rectangle(390, 290, 90, 20));
            jButtonCancel.addActionListener(this);
        }
        return jButtonCancel;
    }

    /**
     This method initializes jTextFieldDatumType 
     
     @return javax.swing.JTextField jComboBoxDatumType
     
     **/
    private JComboBox getJComboBoxDatumType() {
        if (jComboBoxDatumType == null) {
            jComboBoxDatumType = new JComboBox();
            jComboBoxDatumType.setBounds(new java.awt.Rectangle(160, 60, 320, 20));
        }
        return jComboBoxDatumType;
    }

    public static void main(String[] args) {

    }

    /**
     This is the default constructor
     
     **/
    public ModulePCDs() {
        super();
        init();
        this.setVisible(true);
    }

    /**
     This is the override edit constructor
     
     @param inPcds The input data of PCDsDocument.PCDs
     
     **/
    public ModulePCDs(PCDsDocument.PCDs inPcds) {
        super();
        init(inPcds);
        this.setVisible(true);
    }

    /**
     This is the override edit constructor
     
     @param inPcds The input data of PCDsDocument.PCDs
     @param type The input data of node type
     @param index The input data of node index
     
     **/
    public ModulePCDs(PCDsDocument.PCDs inPcds, int type, int index) {
        super();
        init(inPcds, type, index);
        this.setVisible(true);
    }

    /**
     This method initializes this
     
     @param inPcds The input data of PCDsDocument.PCDs
     
     **/
    private void init(PCDsDocument.PCDs inPcds) {
        init();
        this.setPcds(inPcds);
    }

    /**
     This method initializes this
     Fill values to all fields if these values are not empty
     
     @param inPcds The input data of PCDsDocument.PCDs
     @param type The input data of node type
     @param index The input data of node index
     
     **/
    private void init(PCDsDocument.PCDs inPcds, int type, int index) {
        init(inPcds);
        this.location = index;
        if (this.pcds.getPcdDataList().size() > 0) {
            if (this.pcds.getPcdDataArray(index).getCName() != null) {
                this.jTextFieldC_Name.setText(this.pcds.getPcdDataArray(index).getCName());
            }
            if (this.pcds.getPcdDataArray(index).getToken() != null) {
                this.jTextFieldToken.setText(this.pcds.getPcdDataArray(index).getToken());
            }
            if (this.pcds.getPcdDataArray(index).getDatumType() != null) {
                this.jComboBoxDatumType.setSelectedItem(this.pcds.getPcdDataArray(index).getDatumType().toString());
            }
            if (this.pcds.getPcdDataArray(index).getDefaultValue() != null) {
                this.jTextFieldDefaultValue.setText(this.pcds.getPcdDataArray(index).getDefaultValue());
            }
            if (this.pcds.getPcdDataArray(index).getPcdUsage() != null) {
                this.jComboBoxUsage.setSelectedItem(this.pcds.getPcdDataArray(index).getPcdUsage().toString());
            }
            if (this.pcds.getPcdDataArray(index).getItemType() != null) {
                this.jComboBoxItemType.setSelectedItem(this.pcds.getPcdDataArray(index).getItemType().toString());
            }
        }
    }

    /**
     This method initializes this
     
     **/
    private void init() {
        this.setSize(500, 515);
        this.setContentPane(getJContentPane());
        this.setTitle("PCDs");
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
            this.jTextFieldToken.setEnabled(!isView);
            this.jComboBoxDatumType.setEnabled(!isView);
            this.jTextFieldDefaultValue.setEnabled(!isView);
            this.jComboBoxUsage.setEnabled(!isView);
            this.jComboBoxItemType.setEnabled(!isView);
        }
    }

    /**
     This method initializes jContentPane
     
     @return javax.swing.JPanel jContentPane
     
     **/
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jLabelDatumType = new JLabel();
            jLabelDatumType.setBounds(new java.awt.Rectangle(15, 60, 140, 20));
            jLabelDatumType.setText("Datum Type");
            jLabelC_Name = new JLabel();
            jLabelC_Name.setText("C_Name");
            jLabelC_Name.setBounds(new java.awt.Rectangle(15, 10, 140, 20));
            jLabelUsage = new JLabel();
            jLabelUsage.setText("Usage");
            jLabelUsage.setBounds(new java.awt.Rectangle(15, 135, 140, 20));
            jLabelDefaultValue = new JLabel();
            jLabelDefaultValue.setText("Default Value");
            jLabelDefaultValue.setBounds(new java.awt.Rectangle(15, 85, 140, 20));
            jLabelToken = new JLabel();
            jLabelToken.setText("Token");
            jLabelToken.setBounds(new java.awt.Rectangle(15, 35, 140, 20));
            jLabelItemType = new JLabel();
            jLabelItemType.setText("Item Type");
            jLabelItemType.setBounds(new java.awt.Rectangle(15, 110, 140, 20));
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.setSize(new java.awt.Dimension(480,336));
            jContentPane.add(jLabelItemType, null);
            jContentPane.add(jLabelC_Name, null);
            jContentPane.add(getJTextFieldC_Name(), null);
            jContentPane.add(jLabelToken, null);
            jContentPane.add(getJTextFieldToken(), null);
            jContentPane.add(jLabelDefaultValue, null);
            jContentPane.add(getJTextFieldDefaultValue(), null);
            jContentPane.add(jLabelUsage, null);
            jContentPane.add(getJComboBoxUsage(), null);
            jContentPane.add(getJButtonOk(), null);
            jContentPane.add(getJButtonCancel(), null);
            jContentPane.add(getJComboBoxItemType(), null);
            jContentPane.add(jLabelDatumType, null);
            jContentPane.add(getJComboBoxDatumType(), null);

            jStarLabel1 = new StarLabel();
            jStarLabel1.setLocation(new java.awt.Point(0, 10));
            jStarLabel2 = new StarLabel();
            jStarLabel2.setLocation(new java.awt.Point(0, 35));
            jStarLabel3 = new StarLabel();
            jStarLabel3.setLocation(new java.awt.Point(0, 60));
            jStarLabel4 = new StarLabel();
            jStarLabel4.setLocation(new java.awt.Point(0, 110));

            jContentPane.add(jStarLabel1, null);
            jContentPane.add(jStarLabel2, null);
            jContentPane.add(jStarLabel3, null);
            jContentPane.add(jStarLabel4, null);
        }
        return jContentPane;
    }

    /**
     This method initializes Usage type, Item type and Datum type
     
     **/
    private void initFrame() {
        jComboBoxUsage.addItem("ALWAYS_CONSUMED");
        jComboBoxUsage.addItem("SOMETIMES_CONSUMED");
        jComboBoxUsage.addItem("ALWAYS_PRODUCED");
        jComboBoxUsage.addItem("SOMETIMES_PRODUCED");
        jComboBoxUsage.addItem("DEFAULT");

        jComboBoxItemType.addItem("FEATURE_FLAG");
        jComboBoxItemType.addItem("FIXED_AT_BUILD");
        jComboBoxItemType.addItem("PATCHABLE_IN_MODULE");
        jComboBoxItemType.addItem("DYNAMIC");
        jComboBoxItemType.addItem("DYNAMIC_EX");

        jComboBoxDatumType.addItem("UINT8");
        jComboBoxDatumType.addItem("UINT16");
        jComboBoxDatumType.addItem("UINT32");
        jComboBoxDatumType.addItem("UINT64");
        jComboBoxDatumType.addItem("VOID*");
        jComboBoxDatumType.addItem("BOOLEAN");
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

    /**
     Get PCDsDocument.PCDs
     
     @return PCDsDocument.PCDs
     
     **/
    public PCDsDocument.PCDs getPcds() {
        return pcds;
    }

    /**
     Set PCDsDocument.PCDs
     
     @param pcds The input data of PCDsDocument.PCDs
     
     **/
    public void setPcds(PCDsDocument.PCDs pcds) {
        this.pcds = pcds;
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
        if (isEmpty(this.jTextFieldToken.getText())) {
            Log.err("Token couldn't be empty");
            return false;
        }

        //
        // Check if all fields have correct data types 
        //
        if (!isEmpty(this.jTextFieldC_Name.getText()) && !DataValidation.isCName(this.jTextFieldC_Name.getText())) {
            Log.err("Incorrect data type for C_Name");
            return false;
        }
        if (!isEmpty(this.jTextFieldToken.getText()) && !DataValidation.isToken(this.jTextFieldToken.getText())) {
            Log.err("Incorrect data type for Token");
            return false;
        }
        
        return true;
    }

    /**
     Save all components of PCDs
     if exists pcds, set the value directly
     if not exists pcds, new an instance first
     
     **/
    public void save() {
        try {
            if (this.pcds == null) {
                pcds = PCDsDocument.PCDs.Factory.newInstance();
            }
            PCDsDocument.PCDs.PcdData pcdData = PCDsDocument.PCDs.PcdData.Factory.newInstance();
            if (!isEmpty(this.jTextFieldC_Name.getText())) {
                pcdData.setCName(this.jTextFieldC_Name.getText());
            }
            if (!isEmpty(this.jTextFieldToken.getText())) {
                pcdData.setToken(this.jTextFieldToken.getText());
            }
            pcdData.setDatumType(PcdDataTypes.Enum.forString(this.jComboBoxDatumType.getSelectedItem().toString()));
            if (!isEmpty(this.jTextFieldDefaultValue.getText())) {
                pcdData.setDefaultValue(this.jTextFieldDefaultValue.getText());
            }
            pcdData.setItemType(PcdItemTypes.Enum.forString(this.jComboBoxItemType.getSelectedItem().toString()));
            pcdData.setPcdUsage(PcdUsage.Enum.forString(this.jComboBoxUsage.getSelectedItem().toString()));
           

            if (location > -1) {
                pcds.setPcdDataArray(location, pcdData);
            } else {
                pcds.addNewPcdData();
                pcds.setPcdDataArray(pcds.getPcdDataList().size() - 1, pcdData);
            }
        } catch (Exception e) {
            Log.err("Update Hobs", e.getMessage());
        }
    }
}
