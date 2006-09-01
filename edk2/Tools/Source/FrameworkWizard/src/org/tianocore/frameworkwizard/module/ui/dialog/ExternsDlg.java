/** @file
 
 The file is used to create, update Externs section of the MSA file
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/
package org.tianocore.frameworkwizard.module.ui.dialog;

import java.awt.event.ActionEvent;
import java.awt.event.ItemEvent;
import java.awt.event.ItemListener;
import java.util.Vector;

import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextField;

import org.tianocore.frameworkwizard.common.DataType;
import org.tianocore.frameworkwizard.common.DataValidation;
import org.tianocore.frameworkwizard.common.EnumerationData;
import org.tianocore.frameworkwizard.common.Log;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.ui.ArchCheckBox;
import org.tianocore.frameworkwizard.common.ui.IDialog;
import org.tianocore.frameworkwizard.common.ui.IFrame;
import org.tianocore.frameworkwizard.module.Identifications.Externs.ExternsIdentification;

/**
 * The class is used to create, update Externs section of the MSA file
 * 
 * It extends IDialog
 * 
 */
public class ExternsDlg extends IDialog implements ItemListener {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = -7382008402932047191L;

    //
    //Define class members
    //
    private JPanel jContentPane = null;

    private JLabel jLabelName = null;

    private JComboBox jComboBoxType = null;

    private JScrollPane jScrollPane = null;

    private JComboBox jComboBoxPcdIsDriver = null;

    private JLabel jLabelC_Name = null;

    private JTextField jTextFieldC_Name = null;

    private JLabel jLabelFeatureFlag = null;

    private JLabel jLabelArch = null;

    private JTextField jTextFieldFeatureFlag = null;

    private ArchCheckBox jArchCheckBox = null;

    private JButton jButtonOk = null;

    private JButton jButtonCancel = null;

    //
    // Not used by UI
    //
    private ExternsIdentification id = null;

    private EnumerationData ed = new EnumerationData();

    /**
     This method initializes jComboBoxType 
     
     @return javax.swing.JComboBox jComboBoxType
     
     **/
    private JComboBox getJComboBoxType() {
        if (jComboBoxType == null) {
            jComboBoxType = new JComboBox();
            jComboBoxType.setBounds(new java.awt.Rectangle(168, 12, 320, 20));
            jComboBoxType.setPreferredSize(new java.awt.Dimension(320, 20));
            jComboBoxType.addItemListener(this);
        }
        return jComboBoxType;
    }

    /**
     This method initializes jScrollPane  
     
     @return javax.swing.JScrollPane  
     */
    private JScrollPane getJScrollPane() {
        if (jScrollPane == null) {
            jScrollPane = new JScrollPane();
            jScrollPane.setViewportView(getJContentPane());
        }
        return jScrollPane;
    }

    /**
     * This method initializes jComboBoxPcdIsDriver	
     * 	
     * @return javax.swing.JComboBox	
     */
    private JComboBox getJComboBoxPcdIsDriver() {
        if (jComboBoxPcdIsDriver == null) {
            jComboBoxPcdIsDriver = new JComboBox();
            jComboBoxPcdIsDriver.setLocation(new java.awt.Point(168, 37));
            jComboBoxPcdIsDriver.setPreferredSize(new java.awt.Dimension(320, 20));
            jComboBoxPcdIsDriver.setSize(new java.awt.Dimension(320, 20));
            jComboBoxPcdIsDriver.addItemListener(this);
        }
        return jComboBoxPcdIsDriver;
    }

    /**
     This method initializes jTextFieldC_Name	
     
     @return javax.swing.JTextField	
     
     **/
    private JTextField getJTextFieldC_Name() {
        if (jTextFieldC_Name == null) {
            jTextFieldC_Name = new JTextField();
            jTextFieldC_Name.setBounds(new java.awt.Rectangle(168, 37, 320, 20));
            jTextFieldC_Name.setPreferredSize(new java.awt.Dimension(320, 20));
        }
        return jTextFieldC_Name;
    }

    /**
     This method initializes jTextFieldFeatureFlag    
     
     @return javax.swing.JTextField   
     
     **/
    private JTextField getJTextFieldFeatureFlag() {
        if (jTextFieldFeatureFlag == null) {
            jTextFieldFeatureFlag = new JTextField();
            jTextFieldFeatureFlag.setBounds(new java.awt.Rectangle(168, 87, 320, 20));
            jTextFieldFeatureFlag.setPreferredSize(new java.awt.Dimension(320, 20));
            jTextFieldFeatureFlag.setEnabled(false);
        }
        return jTextFieldFeatureFlag;
    }

    /**
     This method initializes jButtonOk    
     
     @return javax.swing.JButton  
     
     **/
    private JButton getJButtonOk() {
        if (jButtonOk == null) {
            jButtonOk = new JButton();
            jButtonOk.setBounds(new java.awt.Rectangle(300, 122, 90, 20));
            jButtonOk.setText("Ok");
            jButtonOk.addActionListener(this);
        }
        return jButtonOk;
    }

    /**
     This method initializes jButtonCancel    
     
     @return javax.swing.JButton  
     
     **/
    private JButton getJButtonCancel() {
        if (jButtonCancel == null) {
            jButtonCancel = new JButton();
            jButtonCancel.setBounds(new java.awt.Rectangle(400, 122, 90, 20));
            jButtonCancel.setText("Cancel");
            jButtonCancel.addActionListener(this);
        }
        return jButtonCancel;
    }

    public static void main(String[] args) {

    }

    /**
     This method initializes this
     
     **/
    private void init() {
        this.setSize(515, 200);
        this.setContentPane(getJScrollPane());
        this.setTitle("Externs");
        initFrame();
        this.centerWindow();
    }

    /**
     This method initializes this
     Fill values to all fields if these values are not empty
     
     @param inExternsId

     **/
    private void init(ExternsIdentification inExternsId) {
        init();
        this.id = inExternsId;

        if (this.id != null) {
            if (id.getType().equals(EnumerationData.EXTERNS_PCD_IS_DRIVER)) {
                this.jComboBoxPcdIsDriver.setSelectedItem(id.getName());
            } else {
                this.jTextFieldC_Name.setText(id.getName());
            }
            this.jComboBoxType.setSelectedItem(id.getType());
            this.jTextFieldFeatureFlag.setText(id.getFeatureFlag());
            this.jArchCheckBox.setSelectedItems(id.getSupArchList());
        }
    }

    /**
     This is the override edit constructor
     
     @param inBootModesIdentification
     @param iFrame
     
     **/
    public ExternsDlg(ExternsIdentification inExternsIdentification, IFrame iFrame) {
        super(iFrame, true);
        init(inExternsIdentification);
    }

    /**
     This method initializes jContentPane
     
     @return javax.swing.JPanel jContentPane
     
     **/
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jArchCheckBox = new ArchCheckBox();
            jArchCheckBox.setBounds(new java.awt.Rectangle(168, 62, 320, 20));
            jArchCheckBox.setPreferredSize(new java.awt.Dimension(320, 20));
            jLabelC_Name = new JLabel();
            jLabelC_Name.setBounds(new java.awt.Rectangle(12, 37, 140, 20));
            jLabelC_Name.setText("Enter Value");
            jLabelName = new JLabel();
            jLabelName.setText("Choose Extern Type");
            jLabelName.setBounds(new java.awt.Rectangle(12, 12, 168, 20));
            jLabelArch = new JLabel();
            jLabelArch.setBounds(new java.awt.Rectangle(12, 62, 168, 20));
            jLabelArch.setText("Supported Architectures");
            jLabelFeatureFlag = new JLabel();
            jLabelFeatureFlag.setBounds(new java.awt.Rectangle(12, 87, 168, 20));
            jLabelFeatureFlag.setText("Feature Flag Expression");
            jLabelFeatureFlag.setEnabled(false);

            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.setPreferredSize(new java.awt.Dimension(505, 155));

            jContentPane.add(jLabelName, null);
            jContentPane.add(getJComboBoxType(), null);
            jContentPane.add(jLabelFeatureFlag, null);
            jContentPane.add(jLabelArch, null);
            jContentPane.add(getJTextFieldFeatureFlag(), null);

            jContentPane.add(getJComboBoxPcdIsDriver(), null);
            jContentPane.add(jLabelC_Name, null);
            jContentPane.add(getJTextFieldC_Name(), null);
            jContentPane.add(jArchCheckBox, null);
            jContentPane.add(getJButtonOk(), null);
            jContentPane.add(getJButtonCancel(), null);
        }
        return jContentPane;
    }

    /**
     This method initializes Usage type and Externs type
     
     **/
    private void initFrame() {
        Tools.generateComboBoxByVector(this.jComboBoxType, ed.getVExternTypes());
        Tools.generateComboBoxByVector(this.jComboBoxPcdIsDriver, ed.getVPcdDriverTypes());
    }

    /* (non-Javadoc)
     * @see java.awt.event.ActionListener#actionPerformed(java.awt.event.ActionEvent)
     *
     * Override actionPerformed to listen all actions
     * 
     */
    public void actionPerformed(ActionEvent arg0) {
        if (arg0.getSource() == jButtonOk) {
            if (checkAdd()) {
                getCurrentExterns();
                this.returnType = DataType.RETURN_TYPE_OK;
                this.setVisible(false);
            }
        }

        if (arg0.getSource() == jButtonCancel) {
            this.returnType = DataType.RETURN_TYPE_CANCEL;
            this.setVisible(false);
        }
    }

    /**
     Data validation for all fields
     
     @retval true - All datas are valid
     @retval false - At least one data is invalid
     
     **/
    public boolean checkAdd() {
        //
        // Check if all fields have correct data types 
        //

        //
        // When and only When type is not "Pcd Is Driver"
        //
        if (!this.jComboBoxType.getSelectedItem().toString().equals(EnumerationData.EXTERNS_PCD_IS_DRIVER)) {
            //
            // Check CName 
            //
            if (isEmpty(this.jTextFieldC_Name.getText())) {
                Log.wrn("Update Externs", "Value couldn't be empty");
                return false;
            }

            if (!isEmpty(this.jTextFieldC_Name.getText())) {
                if (this.jComboBoxType.getSelectedItem().toString().equals(EnumerationData.EXTERNS_SPECIFICATION)) {
                    if (!DataValidation.isSentence(this.jTextFieldC_Name.getText())) {
                        Log.wrn("Update Externs", "Incorrect data type for Specification");
                        return false;
                    }
                } else {
                    if (!DataValidation.isC_NameType(this.jTextFieldC_Name.getText())) {
                        Log.wrn("Update Externs", "Incorrect data type for C Name");
                        return false;
                    }
                }
            }

            //
            // Check FeatureFlag
            //
            if (!isEmpty(this.jTextFieldFeatureFlag.getText())) {
                if (!DataValidation.isFeatureFlag(this.jTextFieldFeatureFlag.getText())) {
                    Log.wrn("Update Externs", "Incorrect data type for Feature Flag");
                    return false;
                }
            }
        } else {
            if (this.jComboBoxPcdIsDriver.getSelectedItem().toString().equals(DataType.EMPTY_SELECT_ITEM)) {
                Log.wrn("Update Externs", "You must select one PCD DRIVER type");
                return false;
            }
        }

        return true;
    }

    private ExternsIdentification getCurrentExterns() {
        String arg0 = "";
        if (this.jComboBoxType.getSelectedItem().toString().equals(EnumerationData.EXTERNS_PCD_IS_DRIVER)) {
            arg0 = this.jComboBoxPcdIsDriver.getSelectedItem().toString();
        } else {
            arg0 = this.jTextFieldC_Name.getText();
        }
        String arg1 = this.jComboBoxType.getSelectedItem().toString();

        String arg2 = this.jTextFieldFeatureFlag.getText();
        Vector<String> arg3 = this.jArchCheckBox.getSelectedItemsVector();

        id = new ExternsIdentification(arg0, arg1, arg2, arg3);
        return id;
    }

    /* (non-Javadoc)
     * @see java.awt.event.ItemListener#itemStateChanged(java.awt.event.ItemEvent)
     *
     * Reflesh the frame when selected item changed
     * 
     */
    public void itemStateChanged(ItemEvent arg0) {
        if (arg0.getSource() == jComboBoxType && arg0.getStateChange() == ItemEvent.SELECTED) {
            if (jComboBoxType.getSelectedItem().toString().equals(EnumerationData.EXTERNS_PCD_IS_DRIVER)) {
                this.jComboBoxPcdIsDriver.setVisible(true);
                this.jTextFieldC_Name.setVisible(false);
                this.jLabelArch.setEnabled(false);
                this.jArchCheckBox.setAllItemsEnabled(false);
            } else if (jComboBoxType.getSelectedItem().toString().equals(EnumerationData.EXTERNS_SPECIFICATION)) {
                this.jLabelArch.setEnabled(false);
                this.jArchCheckBox.setAllItemsEnabled(false);
            } else {
                this.jComboBoxPcdIsDriver.setVisible(false);
                this.jTextFieldC_Name.setVisible(true);
                this.jLabelArch.setEnabled(true);
                this.jArchCheckBox.setAllItemsEnabled(true);
            }
        }
    }

    public ExternsIdentification getId() {
        return id;
    }

    public void setId(ExternsIdentification id) {
        this.id = id;
    }
}
