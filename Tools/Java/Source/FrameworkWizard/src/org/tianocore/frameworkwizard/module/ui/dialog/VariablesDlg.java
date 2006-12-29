/** @file
 
 The file is used to create, update Variables section of the MSA file
 
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
import javax.swing.JTextArea;

import org.tianocore.frameworkwizard.common.DataType;
import org.tianocore.frameworkwizard.common.DataValidation;
import org.tianocore.frameworkwizard.common.EnumerationData;
import org.tianocore.frameworkwizard.common.Log;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.ui.ArchCheckBox;
import org.tianocore.frameworkwizard.common.ui.IComboBox;
import org.tianocore.frameworkwizard.common.ui.IDialog;
import org.tianocore.frameworkwizard.common.ui.IFrame;
import org.tianocore.frameworkwizard.common.ui.StarLabel;
import org.tianocore.frameworkwizard.module.Identifications.ModuleIdentification;
import org.tianocore.frameworkwizard.module.Identifications.Variables.VariablesIdentification;
import org.tianocore.frameworkwizard.packaging.PackageIdentification;
import org.tianocore.frameworkwizard.workspace.WorkspaceTools;

/**
 * The class is used to create, update Variables section of the MSA file
 * 
 * It extends IDialog
 * 
 */
public class VariablesDlg extends IDialog {

    // /
    // / Define class Serial Version UID
    // /
    private static final long serialVersionUID = -6998982978030439446L;

    //
    // Define class members
    //
    private JPanel jContentPane = null;

    private JLabel jLabelVariableName = null;

    private JTextField jTextFieldVariableName = null;

    private JLabel jLabelUsage = null;

    private JComboBox jComboBoxUsage = null;

    private StarLabel jStarLabel1 = null;

    private StarLabel jStarLabel2 = null;

    private StarLabel jStarLabel3 = null;

    private JScrollPane jScrollPane = null;

    private JLabel jLabelGuidCName = null;

    private IComboBox iComboBoxGuidC_Name = null;

    private JTextField jTextFieldFeatureFlag = null;

    private JLabel jLabelFeatureFlag = null;

    private JLabel jLabelArch = null;

    private JLabel jLabelHelpText = null;

    private JTextArea jTextAreaHelpText = null;

    private JScrollPane jScrollPaneHelpText = null;

    private ArchCheckBox jArchCheckBox = null;

    private JButton jButtonOk = null;

    private JButton jButtonCancel = null;

    //
    // Not used by UI
    //
    private VariablesIdentification id = null;

    private EnumerationData ed = new EnumerationData();

    private WorkspaceTools wt = new WorkspaceTools();

    /**
     * This method initializes jTextFieldString
     * 
     * @return javax.swing.JTextField jTextFieldString
     * 
     */
    private JTextField getJTextFieldString() {
        if (jTextFieldVariableName == null) {
            jTextFieldVariableName = new JTextField();
            jTextFieldVariableName.setSize(new java.awt.Dimension(320, 20));
            jTextFieldVariableName.setPreferredSize(new java.awt.Dimension(320, 20));
            jTextFieldVariableName.setLocation(new java.awt.Point(168, 12));
            jTextFieldVariableName.setToolTipText("Enter a string; the tool will convert to Unicode hex");
        }
        return jTextFieldVariableName;
    }

    /**
     * This method initializes jComboBoxUsage
     * 
     * @return javax.swing.JComboBox jComboBoxUsage
     * 
     */
    private JComboBox getJComboBoxUsage() {
        if (jComboBoxUsage == null) {
            jComboBoxUsage = new JComboBox();
            jComboBoxUsage.setBounds(new java.awt.Rectangle(168, 62, 320, 20));
            jComboBoxUsage.setPreferredSize(new java.awt.Dimension(320, 20));
            jComboBoxUsage
                          .setToolTipText("<html><table>"
                                          + "<tr><td>ALWAYS_CONSUMED</td><td>The module requires the variable entry to be set</td></tr>"
                                          + "<tr><td>SOMETIMES_CONSUMED</td><td>The module will use the variable entry if it is set.</td></tr>"
                                          + "<tr><td>ALWAYS_PRODUCED</td><td>The module will always write the variable.</td></tr>"
                                          + "<tr><td>SOMETIMES_PRODUCED</td><td>The module will sometimes write the variable.</td></tr>"
                                          + "</table></html>");
        }
        return jComboBoxUsage;
    }

    /**
     * This method initializes jScrollPane
     * 
     * @return javax.swing.JScrollPane
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
     * @return javax.swing.JTextField jTextFieldFeatureFlag
     * 
     */
    private JTextField getJTextFieldFeatureFlag() {
        if (jTextFieldFeatureFlag == null) {
            jTextFieldFeatureFlag = new JTextField();
            jTextFieldFeatureFlag.setBounds(new java.awt.Rectangle(168, 157, 320, 20));
            jTextFieldFeatureFlag.setPreferredSize(new java.awt.Dimension(320, 20));
            jTextFieldFeatureFlag.setToolTipText("Postfix expression that must evaluate to TRUE or FALSE");
            jTextFieldFeatureFlag.setEnabled(false);
        }
        return jTextFieldFeatureFlag;
    }

    /**
     * This method initializes jTextFieldHelpText
     * 
     * @return javax.swing.JTextField
     * 
     */
    private JTextArea getJTextAreaHelpText() {
        if (jTextAreaHelpText == null) {
            jTextAreaHelpText = new JTextArea();
            jTextAreaHelpText.setLineWrap(true);
            jTextAreaHelpText.setWrapStyleWord(true);
            jTextAreaHelpText.setToolTipText("Enter information on how to use this Variable.");
        }
        return jTextAreaHelpText;
    }

    private JScrollPane getJScrollPaneHelpText() {
        if (jScrollPaneHelpText == null) {
            jScrollPaneHelpText = new JScrollPane();
            jScrollPaneHelpText.setHorizontalScrollBarPolicy(javax.swing.JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);
            jScrollPaneHelpText.setSize(new java.awt.Dimension(320, 40));
            jScrollPaneHelpText.setPreferredSize(new java.awt.Dimension(320, 40));
            jScrollPaneHelpText.setLocation(new java.awt.Point(168, 87));
            jScrollPaneHelpText.setViewportView(getJTextAreaHelpText());
        }
        return jScrollPaneHelpText;
    }

    /**
     * This method initializes iComboBoxGuidC_Name
     * 
     * @return javax.swing.JComboBox iComboBoxGuidC_Name
     * 
     */
    private IComboBox getIComboBoxGuidC_Name() {
        if (iComboBoxGuidC_Name == null) {
            iComboBoxGuidC_Name = new IComboBox();
            iComboBoxGuidC_Name.setBounds(new java.awt.Rectangle(168, 37, 320, 20));
            iComboBoxGuidC_Name.setPreferredSize(new java.awt.Dimension(320, 20));
            iComboBoxGuidC_Name.setToolTipText("Select the GUID C Name of the Variable.");
        }
        return iComboBoxGuidC_Name;
    }

    /**
     * This method initializes jButtonOk
     * 
     * @return javax.swing.JButton
     * 
     */
    private JButton getJButtonOk() {
        if (jButtonOk == null) {
            jButtonOk = new JButton();
            jButtonOk.setBounds(new java.awt.Rectangle(290, 187, 90, 20));
            jButtonOk.setText("Ok");
            jButtonOk.addActionListener(this);
        }
        return jButtonOk;
    }

    /**
     * This method initializes jButtonCancel
     * 
     * @return javax.swing.JButton
     * 
     */
    private JButton getJButtonCancel() {
        if (jButtonCancel == null) {
            jButtonCancel = new JButton();
            jButtonCancel.setBounds(new java.awt.Rectangle(390, 187, 90, 20));
            jButtonCancel.setText("Cancel");
            jButtonCancel.addActionListener(this);
        }
        return jButtonCancel;
    }

    public static void main(String[] args) {

    }

    /**
     * This method initializes this
     * 
     */
    private void init() {
        this.setSize(505, 260);
        this.setContentPane(getJScrollPane());
        this.setTitle("Variables");
        initFrame();
        this.setViewMode(false);
        this.centerWindow();
    }

    /**
     * This method initializes this Fill values to all fields if these values are
     * not empty
     * 
     * @param inVariablesId
     * 
     */
    private void init(VariablesIdentification inVariablesId, ModuleIdentification mid) {
        init();
        this.id = inVariablesId;

        //
        // Init arch with module's arch
        //
        this.jArchCheckBox.setEnabledItems(wt.getModuleArch(mid));

        //
        // Get defined guids from dependent packages
        //
        Vector<PackageIdentification> vpid = wt.getPackageDependenciesOfModule(mid);
        if (vpid.size() <= 0) {
            Log
               .wrn("Init Guid",
                    "This module hasn't defined any package dependency, so there is no guid value can be added for variable");
        }
        //
        // Init guids drop down list
        //
        Tools
             .generateComboBoxByVector(iComboBoxGuidC_Name,
                                       wt.getAllGuidDeclarationsFromPackages(vpid, EnumerationData.GUID_TYPE_EFI_VARIABLE));

        if (this.id != null) {
            this.jTextFieldVariableName.setText(id.getName());
            this.iComboBoxGuidC_Name.setSelectedItem(id.getGuid());
            this.jComboBoxUsage.setSelectedItem(id.getUsage());
            this.jTextAreaHelpText.setText(id.getHelp());
            this.jTextFieldFeatureFlag.setText(id.getFeatureFlag());
            this.jArchCheckBox.setSelectedItems(id.getSupArchList());
        }
    }

    /**
     * This is the override edit constructor
     * 
     * @param inVariablesIdentification
     * @param iFrame
     * 
     */
    public VariablesDlg(VariablesIdentification inVariablesIdentification, IFrame iFrame, ModuleIdentification mid) {
        super(iFrame, true);
        init(inVariablesIdentification, mid);
    }

    /**
     * Disable all components when the mode is view
     * 
     * @param isView
     *          true - The view mode; false - The non-view mode
     * 
     */
    public void setViewMode(boolean isView) {
        if (isView) {
            this.jTextFieldVariableName.setEnabled(!isView);
            this.jComboBoxUsage.setEnabled(!isView);
        }
    }

    /**
     * This method initializes jContentPane
     * 
     * @return javax.swing.JPanel jContentPane
     * 
     */
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jStarLabel1 = new StarLabel();
            jStarLabel1.setLocation(new java.awt.Point(2, 12));
            jLabelVariableName = new JLabel();
            jLabelVariableName.setText("Variable Name");
            jLabelVariableName.setBounds(new java.awt.Rectangle(12, 12, 168, 20));

            jStarLabel2 = new StarLabel();
            jStarLabel2.setLocation(new java.awt.Point(2, 37));
            jLabelGuidCName = new JLabel();
            jLabelGuidCName.setBounds(new java.awt.Rectangle(12, 37, 168, 20));
            jLabelGuidCName.setText("Variable Guid C Name");

            jStarLabel3 = new StarLabel();
            jStarLabel3.setLocation(new java.awt.Point(2, 62));
            jLabelUsage = new JLabel();
            jLabelUsage.setText("Usage");
            jLabelUsage.setBounds(new java.awt.Rectangle(12, 62, 168, 20));

            jLabelHelpText = new JLabel();
            jLabelHelpText.setBounds(new java.awt.Rectangle(12, 87, 168, 20));
            jLabelHelpText.setText("Help Text");

            jLabelFeatureFlag = new JLabel();
            jLabelFeatureFlag.setText("Feature Flag Expression");
            jLabelFeatureFlag.setBounds(new java.awt.Rectangle(12, 157, 168, 20));
            jLabelFeatureFlag.setEnabled(false);

            jLabelArch = new JLabel();
            jLabelArch.setBounds(new java.awt.Rectangle(12, 132, 168, 20));
            jLabelArch.setText("Supported Architectures");
            jArchCheckBox = new ArchCheckBox();
            jArchCheckBox.setBounds(new java.awt.Rectangle(168, 132, 320, 20));
            jArchCheckBox.setPreferredSize(new java.awt.Dimension(320, 20));

            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.setPreferredSize(new java.awt.Dimension(485, 215));

            jContentPane.add(jStarLabel1, null);
            jContentPane.add(jLabelVariableName, null);
            jContentPane.add(getJTextFieldString(), null);
            jContentPane.add(jStarLabel2, null);
            jContentPane.add(jLabelGuidCName, null);
            jContentPane.add(getIComboBoxGuidC_Name(), null);
            jContentPane.add(jStarLabel3, null);
            jContentPane.add(jLabelUsage, null);
            jContentPane.add(getJComboBoxUsage(), null);
            jContentPane.add(jLabelHelpText, null);
            jContentPane.add(getJScrollPaneHelpText(), null);
            jContentPane.add(jLabelFeatureFlag, null);
            jContentPane.add(getJTextFieldFeatureFlag(), null);
            jContentPane.add(jLabelArch, null);
            jContentPane.add(jArchCheckBox, null);
            jContentPane.add(getJButtonOk(), null);
            jContentPane.add(getJButtonCancel(), null);
        }
        return jContentPane;
    }

    /*
     * (non-Javadoc)
     * 
     * @see java.awt.event.ActionListener#actionPerformed(java.awt.event.ActionEvent)
     * 
     * Override actionPerformed to listen all actions
     * 
     */
    public void actionPerformed(ActionEvent arg0) {
        if (arg0.getSource() == jButtonOk) {
            if (checkAdd()) {
                getCurrentVariables();
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
     * This method initializes Usage type
     * 
     */
    private void initFrame() {
        Tools.generateComboBoxByVector(jComboBoxUsage, ed.getVPpiUsage());
    }

    /**
     * Data validation for all fields
     * 
     * @retval true - All datas are valid
     * @retval false - At least one data is invalid
     * 
     */
    public boolean checkAdd() {
        //
        // Check if all fields have correct data types
        //
        
        //
        // Check VariableName
        //
        if (isEmpty(this.jTextFieldVariableName.getText())) {
            Log.wrn("Update Variables", "Variable Name must be entered!");
            return false;
        }
        
        //
        // Check Guid Value
        //
        if (this.iComboBoxGuidC_Name.getSelectedItem() == null) {
            Log.wrn("Update Guids", "Please select one Varibale Guid value");
            return false;
        }

        //
        // Check FeatureFlag
        //
        if (!isEmpty(this.jTextFieldFeatureFlag.getText())) {
            if (!DataValidation.isFeatureFlag(this.jTextFieldFeatureFlag.getText())) {
                Log.wrn("Update Variables", "Incorrect data type for Feature Flag");
                return false;
            }
        }

        return true;
    }

    private VariablesIdentification getCurrentVariables() {
        String arg0 = this.jTextFieldVariableName.getText();
        String arg1 = this.iComboBoxGuidC_Name.getSelectedItem().toString();
        String arg2 = this.jComboBoxUsage.getSelectedItem().toString();

        String arg3 = this.jTextFieldFeatureFlag.getText();
        Vector<String> arg4 = this.jArchCheckBox.getSelectedItemsVector();
        String arg5 = this.jTextAreaHelpText.getText();

        id = new VariablesIdentification(arg0, arg1, arg2, arg3, arg4, arg5);
        return id;
    }

    public VariablesIdentification getId() {
        return id;
    }

    public void setId(VariablesIdentification id) {
        this.id = id;
    }
}
