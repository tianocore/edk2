/** @file
 
 The file is used to create, update PcdCode section of the MSA file
 
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
import javax.swing.JTextArea;

import org.tianocore.frameworkwizard.common.DataType;
import org.tianocore.frameworkwizard.common.DataValidation;
import org.tianocore.frameworkwizard.common.EnumerationData;
import org.tianocore.frameworkwizard.common.Log;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.ui.ArchCheckBox;
import org.tianocore.frameworkwizard.common.ui.IDialog;
import org.tianocore.frameworkwizard.common.ui.IFrame;
import org.tianocore.frameworkwizard.common.ui.StarLabel;
import org.tianocore.frameworkwizard.module.Identifications.ModuleIdentification;
import org.tianocore.frameworkwizard.module.Identifications.PcdCoded.PcdCodedIdentification;
import org.tianocore.frameworkwizard.module.Identifications.PcdCoded.PcdVector;
import org.tianocore.frameworkwizard.packaging.PackageIdentification;
import org.tianocore.frameworkwizard.workspace.WorkspaceTools;

/**
 * The class is used to create, update PcdCoded section of the MSA file
 * 
 * It extends IDialog
 * 
 */
public class PCDsDlg extends IDialog implements ItemListener {

    // /
    // / Define class Serial Version UID
    // /
    private static final long serialVersionUID = 2227717658188438696L;

    //
    // Define class members
    //
    private JPanel jContentPane = null;

    private JLabel jLabelItemType = null;

    private JLabel jLabelC_Name = null;

    private JComboBox jComboBoxItemType = null;

    private JComboBox jComboBoxCName = null;

    private JLabel jLabelDefaultValue = null;

    private JTextField jTextFieldDefaultValue = null;

    private StarLabel jStarLabel1 = null;

    private StarLabel jStarLabel2 = null;

    private StarLabel jStarLabel3 = null;

    private JLabel jLabelHelpText = null;

    private JTextArea jTextAreaHelpText = null;

    private JScrollPane jScrollPane = null;

    private JLabel jLabelTokenSpaceGuid = null;

    private JTextField jTextFieldTokenSpaceGuid = null;

    private JLabel jLabelFeatureFlag = null;

    private JTextField jTextFieldFeatureFlag = null;

    private JLabel jLabelArch = null;

    private ArchCheckBox jArchCheckBox = null;

    private JButton jButtonOk = null;

    private JButton jButtonCancel = null;

    private JScrollPane jScrollPaneHelpText = null;

    private JLabel jLabelUsage = null;

    private JComboBox jComboBoxUsage = null;

    //
    // Not used by UI
    //
    private PcdCodedIdentification id = null;

    private EnumerationData ed = new EnumerationData();

    private WorkspaceTools wt = new WorkspaceTools();

    private PcdVector pcd = wt.getAllPcdDeclarationsFromWorkspace();

    /**
     * This method initializes jComboBoxItemType
     * 
     * @return javax.swing.JComboBox jComboBoxItemType
     * 
     */
    private JComboBox getJComboBoxItemType() {
        if (jComboBoxItemType == null) {
            jComboBoxItemType = new JComboBox();
            jComboBoxItemType.setBounds(new java.awt.Rectangle(168, 37, 320, 20));
            jComboBoxItemType.setPreferredSize(new java.awt.Dimension(320, 20));
        }
        return jComboBoxItemType;
    }

    /**
     * This method initializes jTextFieldC_Name
     * 
     * @return javax.swing.JTextField jTextFieldC_Name
     * 
     */
    private JComboBox getJComboBoxCName() {
        if (jComboBoxCName == null) {
            jComboBoxCName = new JComboBox();
            jComboBoxCName.setBounds(new java.awt.Rectangle(168, 12, 320, 20));
            jComboBoxCName.setPreferredSize(new java.awt.Dimension(320, 20));
            jComboBoxCName.addItemListener(this);
        }
        return jComboBoxCName;
    }

    /**
     * This method initializes jTextFieldDefaultValue
     * 
     * @return javax.swing.JTextField jTextFieldDefaultValue
     * 
     */
    private JTextField getJTextFieldDefaultValue() {
        if (jTextFieldDefaultValue == null) {
            jTextFieldDefaultValue = new JTextField();
            jTextFieldDefaultValue.setBounds(new java.awt.Rectangle(168, 62, 320, 20));
            jTextFieldDefaultValue.setPreferredSize(new java.awt.Dimension(320, 20));
        }
        return jTextFieldDefaultValue;
    }

    /**
     * This method initializes jTextAreaHelpText
     * 
     * @return javax.swing.JTextArea
     */
    private JTextArea getJTextAreaHelpText() {
        if (jTextAreaHelpText == null) {
            jTextAreaHelpText = new JTextArea();
            jTextAreaHelpText.setLineWrap(true);
            jTextAreaHelpText.setWrapStyleWord(true);
        }
        return jTextAreaHelpText;
    }

    /**
     * This method initializes jScrollPaneHelpText
     * 
     * @return javax.swing.JScrollPane
     * 
     */
    private JScrollPane getJScrollPaneHelpText() {
        if (jScrollPaneHelpText == null) {
            jScrollPaneHelpText = new JScrollPane();
            jScrollPaneHelpText.setHorizontalScrollBarPolicy(javax.swing.JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);
            jScrollPaneHelpText.setSize(new java.awt.Dimension(320, 40));
            jScrollPaneHelpText.setPreferredSize(new java.awt.Dimension(320, 40));
            jScrollPaneHelpText.setLocation(new java.awt.Point(168, 112));
            jScrollPaneHelpText.setViewportView(getJTextAreaHelpText());
        }
        return jScrollPaneHelpText;
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
     * This method initializes jTextFieldTokenSpaceGuid
     * 
     * @return javax.swing.JTextField
     */
    private JTextField getJTextFieldTokenSpaceGuid() {
        if (jTextFieldTokenSpaceGuid == null) {
            jTextFieldTokenSpaceGuid = new JTextField();
            jTextFieldTokenSpaceGuid.setBounds(new java.awt.Rectangle(168, 62, 320, 20));
            jTextFieldTokenSpaceGuid.setPreferredSize(new java.awt.Dimension(320, 20));
            jTextFieldTokenSpaceGuid.setVisible(false);
        }
        return jTextFieldTokenSpaceGuid;
    }

    /**
     * This method initializes jTextFieldFeatureFlag
     * 
     * @return javax.swing.JTextField
     */
    private JTextField getJTextFieldFeatureFlag() {
        if (jTextFieldFeatureFlag == null) {
            jTextFieldFeatureFlag = new JTextField();
            jTextFieldFeatureFlag.setBounds(new java.awt.Rectangle(168, 182, 320, 20));
            jTextFieldFeatureFlag.setPreferredSize(new java.awt.Dimension(320, 20));
            jTextFieldFeatureFlag.setToolTipText("Postfix expression that must evaluate to TRUE or FALSE");
            jTextFieldFeatureFlag.setEnabled(false);
        }
        return jTextFieldFeatureFlag;
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
            jButtonOk.setBounds(new java.awt.Rectangle(290, 212, 90, 20));
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
            jButtonCancel.setBounds(new java.awt.Rectangle(390, 212, 90, 20));
            jButtonCancel.setText("Cancel");
            jButtonCancel.addActionListener(this);
        }
        return jButtonCancel;
    }

    /**
     * This method initializes jComboBoxUsage	
     * 	
     * @return javax.swing.JComboBox	
     */
    private JComboBox getJComboBoxUsage() {
        if (jComboBoxUsage == null) {
            jComboBoxUsage = new JComboBox();
            jComboBoxUsage.setBounds(new java.awt.Rectangle(168, 87, 320, 20));
            jComboBoxUsage.setPreferredSize(new java.awt.Dimension(320, 20));
        }
        return jComboBoxUsage;
    }

    public static void main(String[] args) {

    }

    /**
     * This method initializes this
     * 
     */
    private void init() {
        this.setSize(505, 280);
        this.setContentPane(getJScrollPane());
        this.setTitle("Pcd Coded");
        initFrame();
        this.setViewMode(false);
        this.centerWindow();
    }

    /**
     * This method initializes this Fill values to all fields if these values are
     * not empty
     * 
     * @param inPcdCodedId
     * 
     */
    private void init(PcdCodedIdentification inPcdCodedId, ModuleIdentification mid) {
        init();
        this.id = inPcdCodedId;
        
        //
        // Init arch with module's arch
        //
        this.jArchCheckBox.setEnabledItems(wt.getModuleArch(mid));
        
        //
        // Get defined pcds from dependent packages
        //
        Vector<PackageIdentification> vpid = wt.getPackageDependenciesOfModule(mid);
        if (vpid.size() <= 0) {
            Log.wrn("Init Pcd", "This module hasn't defined any package dependency, so there is no pcd can be added");
        }

        pcd = wt.getAllPcdDeclarationsFromPackages(wt.getPackageDependenciesOfModule(mid));
        for (int index = 0; index < pcd.size(); index++) {
            jComboBoxCName.addItem(pcd.getPcd(index));
        }

        if (this.id != null) {
            for (int index = 0; index < this.jComboBoxCName.getItemCount(); index++) {
                if (this.jComboBoxCName.getItemAt(index).toString().equals(id.getName())) {
                    this.jComboBoxCName.setSelectedIndex(index);
                    break;
                }
            }
            this.jTextFieldTokenSpaceGuid.setText(id.getGuid());
            this.jTextFieldDefaultValue.setText(id.getValue());
            this.jComboBoxUsage.setSelectedItem(id.getUsage());
            this.jTextAreaHelpText.setText(id.getHelp());
            this.jComboBoxItemType.setSelectedItem(id.getType());
            this.jTextFieldFeatureFlag.setText(id.getFeatureFlag());
            this.jArchCheckBox.setSelectedItems(id.getSupArchList());
        }
    }

    /**
     * This is the override edit constructor
     * 
     * @param inPcdCodedId
     * @param iFrame
     * 
     */
    public PCDsDlg(PcdCodedIdentification inPcdCodedId, IFrame iFrame, ModuleIdentification mid) {
        super(iFrame, true);
        init(inPcdCodedId, mid);
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
            this.jTextFieldDefaultValue.setEnabled(!isView);
            this.jComboBoxItemType.setEnabled(!isView);
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
            jLabelUsage = new JLabel();
            jLabelUsage.setBounds(new java.awt.Rectangle(12, 87, 168, 20));
            jLabelUsage.setText("Usge");
            jStarLabel1 = new StarLabel();
            jStarLabel1.setLocation(new java.awt.Point(2, 12));
            jLabelC_Name = new JLabel();
            jLabelC_Name.setText("PCD C Name");
            jLabelC_Name.setBounds(new java.awt.Rectangle(12, 12, 168, 20));

            jStarLabel2 = new StarLabel();
            jStarLabel2.setLocation(new java.awt.Point(2, 37));
            jLabelItemType = new JLabel();
            jLabelItemType.setText("PCD Item Type");
            jLabelItemType.setBounds(new java.awt.Rectangle(12, 37, 168, 20));

            jLabelDefaultValue = new JLabel();
            jLabelDefaultValue.setText("Default Value");
            jLabelDefaultValue.setBounds(new java.awt.Rectangle(12, 62, 168, 20));

            jStarLabel3 = new StarLabel();
            jStarLabel3.setLocation(new java.awt.Point(2, 112));
            jLabelHelpText = new JLabel();
            jLabelHelpText.setText("Help Text");
            jLabelHelpText.setBounds(new java.awt.Rectangle(12, 112, 168, 20));

            jLabelFeatureFlag = new JLabel();
            jLabelFeatureFlag.setText("Feature Flag Expression");
            jLabelFeatureFlag.setBounds(new java.awt.Rectangle(12, 182, 168, 20));
            jLabelFeatureFlag.setEnabled(false);

            jLabelTokenSpaceGuid = new JLabel();
            jLabelTokenSpaceGuid.setText("Token Space C Name");
            jLabelTokenSpaceGuid.setVisible(false);

            jLabelArch = new JLabel();
            jLabelArch.setText("Supported Architectures");
            jLabelArch.setBounds(new java.awt.Rectangle(12, 157, 168, 20));
            jArchCheckBox = new ArchCheckBox();
            jArchCheckBox.setPreferredSize(new java.awt.Dimension(320, 20));
            jArchCheckBox.setBounds(new java.awt.Rectangle(168, 157, 320, 20));

            // Hidden field
            jLabelTokenSpaceGuid.setBounds(new java.awt.Rectangle(12, 62, 140, 20));

            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.setPreferredSize(new java.awt.Dimension(485, 235));

            jContentPane.add(jStarLabel1, null);
            jContentPane.add(jLabelC_Name, null);
            jContentPane.add(getJComboBoxCName(), null);
            jContentPane.add(jStarLabel2, null);
            jContentPane.add(jLabelItemType, null);
            jContentPane.add(getJComboBoxItemType(), null);
            jContentPane.add(jLabelDefaultValue, null);
            jContentPane.add(getJTextFieldDefaultValue(), null);
            jContentPane.add(jStarLabel3, null);
            jContentPane.add(jLabelHelpText, null);
            jContentPane.add(getJScrollPaneHelpText(), null);
            jContentPane.add(jLabelFeatureFlag, null);
            jContentPane.add(getJTextFieldFeatureFlag(), null);
            jContentPane.add(jLabelArch, null);
            jContentPane.add(jArchCheckBox, null);
            // Hidden
            jContentPane.add(getJTextFieldTokenSpaceGuid(), null);
            jContentPane.add(jLabelTokenSpaceGuid, null);
            jContentPane.add(getJButtonOk(), null);
            jContentPane.add(getJButtonCancel(), null);
            jContentPane.add(jLabelUsage, null);
            jContentPane.add(getJComboBoxUsage(), null);
        }
        return jContentPane;
    }

    /**
     * This method initializes Usage type, Item type and Datum type
     * 
     */
    private void initFrame() {
        Tools.generateComboBoxByVector(jComboBoxUsage, ed.getVPcdUsage());
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
                getCurrentPcdCoded();
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
        // Check C_Name
        //
        if (this.jComboBoxCName.getSelectedItem() == null) {
            Log.wrn("Update Pcd", "Please select one Pcd Name");
            return false;
        }

        if (!isEmpty(this.jComboBoxCName.getSelectedItem().toString())) {
            if (!DataValidation.isC_NameType(this.jComboBoxCName.getSelectedItem().toString())) {
                Log.wrn("Update PcdCoded", "Incorrect data type for C Name");
                return false;
            }
        }

        //
        // Check TokenSpaceGuid
        //
        if (!isEmpty(this.jTextFieldTokenSpaceGuid.getText())) {
            if (!DataValidation.isC_NameType(this.jTextFieldTokenSpaceGuid.getText())) {
                Log.wrn("Update PcdCoded",
                        "Incorrect data type for the selected pcd entry, please check the datatype specified in the spd file");
                return false;
            }
        }

        //
        // Check DefaultValue
        //
        if (!isEmpty(this.jTextFieldDefaultValue.getText())) {
            if (!DataValidation.isDefaultValueType(this.jTextFieldDefaultValue.getText())) {
                Log.wrn("Update PcdCoded", "Incorrect data type for Default Value");
                return false;
            }
        }

        //
        // Check HelpText
        //
        if (isEmpty(this.jTextAreaHelpText.getText())) {
            Log.wrn("Update PcdCoded", "Help Text should not be empty");
            return false;
        }

        //
        // Check FeatureFlag
        //
        if (!isEmpty(this.jTextFieldFeatureFlag.getText())) {
            if (!DataValidation.isFeatureFlag(this.jTextFieldFeatureFlag.getText())) {
                Log.wrn("Update PcdCoded", "Incorrect data type for Feature Flag");
                return false;
            }
        }

        return true;
    }

    private PcdCodedIdentification getCurrentPcdCoded() {
        String arg0 = this.jComboBoxCName.getSelectedItem().toString();
        String arg1 = this.jTextFieldTokenSpaceGuid.getText();

        String arg2 = this.jTextFieldFeatureFlag.getText();
        Vector<String> arg3 = this.jArchCheckBox.getSelectedItemsVector();

        String arg4 = this.jTextFieldDefaultValue.getText();
        String arg5 = this.jTextAreaHelpText.getText();
        String arg6 = this.jComboBoxItemType.getSelectedItem().toString();
        String arg7 = this.jComboBoxUsage.getSelectedItem().toString();
        if (arg7.equals(DataType.EMPTY_SELECT_ITEM)) {
            arg7 = "";
        }

        id = new PcdCodedIdentification(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
        return id;
    }

    public PcdCodedIdentification getId() {
        return id;
    }

    public void setId(PcdCodedIdentification id) {
        this.id = id;
    }

    /*
     * (non-Javadoc)
     * 
     * @see java.awt.event.ItemListener#itemStateChanged(java.awt.event.ItemEvent)
     * 
     * Reflesh the frame when selected item changed
     * 
     */
    public void itemStateChanged(ItemEvent arg0) {
        int index = this.jComboBoxCName.getSelectedIndex();
        if (arg0.getSource() == this.jComboBoxCName && arg0.getStateChange() == ItemEvent.SELECTED) {
            if (pcd.getPcd(index).getGuidCName() == null || isEmpty(pcd.getPcd(index).getGuidCName())
                || pcd.getPcd(index).getType() == null || pcd.getPcd(index).getHelp() == null
                || isEmpty(pcd.getPcd(index).getHelp())) {
                Log.wrn("select pcd entry when editing msa",
                        "The selected value is defined incorrectly.\r\nPlease check it in the spd file");
            } else {
                this.jTextFieldTokenSpaceGuid.setText(pcd.getPcd(index).getGuidCName());
                Tools.generateComboBoxByVector(this.jComboBoxItemType, pcd.getPcd(index).getType());
                this.jTextAreaHelpText.setText(pcd.getPcd(index).getHelp());
                this.jTextAreaHelpText.setSelectionStart(0);
                this.jTextAreaHelpText.setSelectionEnd(0);
            }
        }
    }
}
