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
package org.tianocore.frameworkwizard.module.ui.dialog;

import java.awt.event.ActionEvent;
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
import org.tianocore.frameworkwizard.common.ui.StarLabel;
import org.tianocore.frameworkwizard.module.Identifications.Hobs.HobsIdentification;
import org.tianocore.frameworkwizard.workspace.WorkspaceTools;

/**
 The class is used to create, update Hob of MSA/MBD file
 It extends IInternalFrame
 
 **/
public class HobsDlg extends IDialog {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = -553473437579358325L;

    //
    //Define class members
    //
    private JPanel jContentPane = null;

    private JLabel jLabel = null;

    private JComboBox jComboBoxGuidC_Name = null;

    private JLabel jLabelUsage = null;

    private JLabel jLabelHobType = null;

    private JComboBox jComboBoxUsage = null;

    private JComboBox jComboBoxHobType = null;

    private StarLabel jStarLabel1 = null;

    private JLabel jLabelArch = null;

    private JScrollPane jScrollPane = null;

    private JLabel jLabelFeatureFlag = null;

    private JTextField jTextFieldFeatureFlag = null;

    private JLabel jLabelHelpText = null;

    private JTextField jTextFieldHelpText = null;

    private ArchCheckBox jArchCheckBox = null;

    private JButton jButtonOk = null;

    private JButton jButtonCancel = null;

    //
    // Not used by UI
    //
    private HobsIdentification id = null;

    private EnumerationData ed = new EnumerationData();

    private WorkspaceTools wt = new WorkspaceTools();

    /**
     This method initializes jTextField 
     
     @return javax.swing.JTextField jTextFieldC_Name
     
     **/
    private JComboBox getJComboBoxGuidC_Name() {
        if (jComboBoxGuidC_Name == null) {
            jComboBoxGuidC_Name = new JComboBox();
            jComboBoxGuidC_Name.setBounds(new java.awt.Rectangle(160, 10, 320, 20));
            jComboBoxGuidC_Name.setPreferredSize(new java.awt.Dimension(320, 20));
            jComboBoxGuidC_Name.setToolTipText("Select the GUID C Name of the Hob");
        }
        return jComboBoxGuidC_Name;
    }

    /**
     This method initializes jComboBoxUsage 
     
     @return javax.swing.JComboBox jComboBoxUsage
     
     **/
    private JComboBox getJComboBoxUsage() {
        if (jComboBoxUsage == null) {
            jComboBoxUsage = new JComboBox();
            jComboBoxUsage.setBounds(new java.awt.Rectangle(160, 60, 320, 20));
            jComboBoxUsage.setPreferredSize(new java.awt.Dimension(320, 20));
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
            jComboBoxHobType.setBounds(new java.awt.Rectangle(160, 35, 320, 20));
            jComboBoxHobType.setPreferredSize(new java.awt.Dimension(320, 20));
        }
        return jComboBoxHobType;
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
     * This method initializes jTextFieldFeatureFlag	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldFeatureFlag() {
        if (jTextFieldFeatureFlag == null) {
            jTextFieldFeatureFlag = new JTextField();
            jTextFieldFeatureFlag.setBounds(new java.awt.Rectangle(160, 110, 320, 20));
            jTextFieldFeatureFlag.setPreferredSize(new java.awt.Dimension(320, 20));
        }
        return jTextFieldFeatureFlag;
    }

    /**
     This method initializes jTextFieldHelpText  
     
     @return javax.swing.JTextField  
     
     **/
    private JTextField getJTextFieldHelpText() {
        if (jTextFieldHelpText == null) {
            jTextFieldHelpText = new JTextField();
            jTextFieldHelpText.setBounds(new java.awt.Rectangle(160, 85, 320, 20));
            jTextFieldHelpText.setPreferredSize(new java.awt.Dimension(320, 20));
        }
        return jTextFieldHelpText;
    }

    /**
     This method initializes jButtonOk    
     
     @return javax.swing.JButton  
     
     **/
    private JButton getJButtonOk() {
        if (jButtonOk == null) {
            jButtonOk = new JButton();
            jButtonOk.setBounds(new java.awt.Rectangle(290, 165, 90, 20));
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
            jButtonCancel.setBounds(new java.awt.Rectangle(390, 165, 90, 20));
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
        this.setSize(500, 225);
        this.setContentPane(getJScrollPane());
        this.setTitle("Hobs");
        initFrame();
        this.setViewMode(false);
        this.centerWindow();
    }

    /**
     This method initializes this
     Fill values to all fields if these values are not empty
     
     @param inHobsId

     **/
    private void init(HobsIdentification inHobsId) {
        init();
        this.id = inHobsId;

        if (this.id != null) {
            this.jComboBoxGuidC_Name.setSelectedItem(id.getName());
            this.jComboBoxHobType.setSelectedItem(id.getType());
            this.jComboBoxUsage.setSelectedItem(id.getUsage());
            this.jTextFieldHelpText.setText(id.getHelp());
            this.jTextFieldFeatureFlag.setText(id.getFeatureFlag());
            this.jArchCheckBox.setSelectedItems(id.getSupArchList());
        }
    }

    /**
     This is the override edit constructor
     
     @param inHobsIdentification
     @param iFrame
     
     **/
    public HobsDlg(HobsIdentification inHobsIdentification, IFrame iFrame) {
        super(iFrame, true);
        init(inHobsIdentification);
    }

    /**
     Disable all components when the mode is view
     
     @param isView true - The view mode; false - The non-view mode
     
     **/
    public void setViewMode(boolean isView) {
        if (isView) {
            this.jComboBoxGuidC_Name.setEnabled(!isView);
            this.jComboBoxUsage.setEnabled(!isView);
            this.jComboBoxHobType.setEnabled(!isView);
        }
    }

    /**
     This method initializes jContentPane
     
     @return javax.swing.JPanel jContentPane
     
     **/
    public JPanel getJContentPane() {
        if (jContentPane == null) {
            jArchCheckBox = new ArchCheckBox();
            jArchCheckBox.setBounds(new java.awt.Rectangle(160, 135, 320, 20));
            jArchCheckBox.setPreferredSize(new java.awt.Dimension(320, 20));
            jLabelFeatureFlag = new JLabel();
            jLabelFeatureFlag.setBounds(new java.awt.Rectangle(15, 110, 140, 20));
            jLabelFeatureFlag.setText("Feature Flag");
            jLabelArch = new JLabel();
            jLabelArch.setBounds(new java.awt.Rectangle(15, 135, 140, 20));
            jLabelArch.setText("Arch");
            jLabelHobType = new JLabel();
            jLabelHobType.setText("Hob Type");
            jLabelHobType.setBounds(new java.awt.Rectangle(15, 35, 140, 20));
            jLabelUsage = new JLabel();
            jLabelUsage.setText("Usage");
            jLabelUsage.setBounds(new java.awt.Rectangle(15, 60, 140, 20));
            jLabel = new JLabel();
            jLabel.setText("Guid C_Name");
            jLabel.setBounds(new java.awt.Rectangle(15, 10, 140, 20));
            jLabelHelpText = new JLabel();
            jLabelHelpText.setBounds(new java.awt.Rectangle(14, 85, 140, 20));
            jLabelHelpText.setText("Help Text");

            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.setPreferredSize(new java.awt.Dimension(490, 190));

            jContentPane.add(jLabel, null);
            jContentPane.add(getJComboBoxGuidC_Name(), null);
            jContentPane.add(jLabelUsage, null);
            jContentPane.add(jLabelHobType, null);
            jContentPane.add(getJComboBoxUsage(), null);
            jContentPane.add(getJComboBoxHobType(), null);
            jStarLabel1 = new StarLabel();
            jStarLabel1.setLocation(new java.awt.Point(0, 10));
            jContentPane.add(jStarLabel1, null);

            jContentPane.add(jLabelArch, null);

            jContentPane.add(jLabelFeatureFlag, null);
            jContentPane.add(getJTextFieldFeatureFlag(), null);
            jContentPane.add(jLabelHelpText, null);
            jContentPane.add(getJTextFieldHelpText(), null);
            jContentPane.add(jArchCheckBox, null);
            jContentPane.add(getJButtonOk(), null);
            jContentPane.add(getJButtonCancel(), null);
        }
        return jContentPane;
    }

    /**
     This method initializes Usage type and Hob type
     
     **/
    private void initFrame() {
        Tools.generateComboBoxByVector(jComboBoxUsage, ed.getVHobUsage());
        Tools.generateComboBoxByVector(jComboBoxHobType, ed.getVHobType());
        Tools.generateComboBoxByVector(jComboBoxGuidC_Name, wt.getAllGuidDeclarationsFromWorkspace());
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
                getCurrentHobs();
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
        // Check Name 
        //
        if (isEmpty(this.jComboBoxGuidC_Name.getSelectedItem().toString())) {
            Log.wrn("Update Hobs", "Hob Name couldn't be empty");
            return false;
        }

        if (!isEmpty(this.jComboBoxGuidC_Name.getSelectedItem().toString())) {
            if (!DataValidation.isC_NameType(this.jComboBoxGuidC_Name.getSelectedItem().toString())) {
                Log.wrn("Update Hobs", "Incorrect data type for Hob Name");
                return false;
            }
        }

        //
        // Check FeatureFlag
        //
        if (!isEmpty(this.jTextFieldFeatureFlag.getText())) {
            if (!DataValidation.isFeatureFlag(this.jTextFieldFeatureFlag.getText())) {
                Log.wrn("Update Hobs", "Incorrect data type for Feature Flag");
                return false;
            }
        }

        return true;
    }

    private HobsIdentification getCurrentHobs() {
        String arg0 = this.jComboBoxGuidC_Name.getSelectedItem().toString();
        String arg1 = this.jComboBoxHobType.getSelectedItem().toString();
        String arg2 = this.jComboBoxUsage.getSelectedItem().toString();

        String arg3 = this.jTextFieldFeatureFlag.getText();
        Vector<String> arg4 = this.jArchCheckBox.getSelectedItemsVector();
        String arg5 = this.jTextFieldHelpText.getText();
        id = new HobsIdentification(arg0, arg1, arg2, arg3, arg4, arg5);
        return id;
    }

    public HobsIdentification getId() {
        return id;
    }

    public void setId(HobsIdentification id) {
        this.id = id;
    }
}
