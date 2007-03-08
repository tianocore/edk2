/** @file
 
 The file is used to create, update Hob section of the MSA file
 
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
import org.tianocore.frameworkwizard.module.Identifications.Hobs.HobsIdentification;
import org.tianocore.frameworkwizard.packaging.PackageIdentification;
import org.tianocore.frameworkwizard.workspace.WorkspaceTools;

/**
 * The class is used to create, update Hob of the MSA file 
 * 
 * It extends IDialog
 * 
 */
public class HobsDlg extends IDialog {

    // /
    // / Define class Serial Version UID
    // /
    private static final long serialVersionUID = -553473437579358325L;

    //
    // Define class members
    //
    private JPanel jContentPane = null;

    private JLabel jLabelC_Name = null;

    private IComboBox iComboBoxGuidC_Name = null;

    private JLabel jLabelUsage = null;

    private JLabel jLabelHobType = null;

    private JComboBox jComboBoxUsage = null;

    private JComboBox jComboBoxHobType = null;

    private StarLabel jStarLabel1 = null;

    private StarLabel jStarLabel2 = null;

    private StarLabel jStarLabel3 = null;

    private JLabel jLabelArch = null;

    private JScrollPane jScrollPane = null;

    private JLabel jLabelFeatureFlag = null;

    private JTextField jTextFieldFeatureFlag = null;

    private JLabel jLabelHelpText = null;

    private JTextArea jTextAreaHelpText = null;

    private JScrollPane jScrollPaneHelpText = null;

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
     * This method initializes jTextField
     * 
     * @return javax.swing.JTextField jTextFieldC_Name
     * 
     */
    private IComboBox getIComboBoxGuidC_Name() {
        if (iComboBoxGuidC_Name == null) {
            iComboBoxGuidC_Name = new IComboBox();
            iComboBoxGuidC_Name.setBounds(new java.awt.Rectangle(168, 12, 320, 20));
            iComboBoxGuidC_Name.setPreferredSize(new java.awt.Dimension(320, 20));
            iComboBoxGuidC_Name.setToolTipText("Select the GUID C Name of the Hob");
        }
        return iComboBoxGuidC_Name;
    }

    /**
     * This method initializes jComboBoxHobType
     * 
     * @return javax.swing.JComboBox jComboBoxHobType
     * 
     */
    private JComboBox getJComboBoxHobType() {
        if (jComboBoxHobType == null) {
            jComboBoxHobType = new JComboBox();
            jComboBoxHobType.setBounds(new java.awt.Rectangle(168, 37, 320, 20));
            jComboBoxHobType.setPreferredSize(new java.awt.Dimension(320, 20));
            jComboBoxHobType
                            .setToolTipText("<html><table>"
                                            + "<tr><td>PHIT</td><td>EFI_HOB_TYPE_HANDOFF</td></tr>"
                                            + "<tr><td>MEMORY_ALLOCATION</td><td>EFI_HOB_TYPE_MEMORY_ALLOCATION and $BaseName</td></tr>"
                                            + "<tr><td>RESOURCE_DESCRIPTOR</td><td>EFI_HOB_TYPE_RESOURCE_DESCRIPTOR</td></tr>"
                                            + "<tr><td>GUID_EXTENTION</td><td>EFI_HOB_TYPE_GUID_EXTENSION and BaseName of GUID</td></tr>"
                                            + "<tr><td>FIRMWARE_VOLUME</td><td>EFI_HOB_TYPE_FV</td></tr>"
                                            + "<tr><td>CPU</td><td>EFI_HOB_TYPE_CPU</td></tr>"
                                            + "<tr><td>POOL</td><td>EFI_HOB_TYPE_PEI_MEMORY_POOL</td></tr>"
                                            + "<tr><td>CAPSULE_VOLUME</td><td>EFI_HOB_TYPE_CV</td></tr>"
                                            + "</table></html>");
        }
        return jComboBoxHobType;
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
                                          + "<tr><td>ALWAYS_CONSUMED</td><td>HOB must be present in the system</td></tr>"
                                          + "<tr><td>SOMETIMES_CONSUMED</td><td>HOB will be used if it's present</td></tr>"
                                          + "<tr><td>ALWAYS_PRODUCED</td><td>HOB is always produced</td></tr>"
                                          + "<tr><td>SOMETIMES_PRODUCED</td><td>HOB will sometimes be produced by the module</td></tr>"
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
     * @return javax.swing.JTextField
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
        }
        return jTextAreaHelpText;
    }

    /**
     * This method initializes jScrollPaneHelpText
     * 
     * @return javax.swing.JScrollPane
     */
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
        this.setTitle("Hobs");
        initFrame();
        this.setViewMode(false);
        this.centerWindow();
    }

    /**
     * This method initializes this Fill values to all fields if these values are
     * not empty
     * 
     * @param inHobsId
     * 
     */
    private void init(HobsIdentification inHobsId, ModuleIdentification mid) {
        init();
        this.id = inHobsId;
        
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
                    "This module hasn't defined any package dependency, so there is no guid value can be added for hob");
        }
        //
        // Init guids drop down list
        //
        Tools
             .generateComboBoxByVector(iComboBoxGuidC_Name,
                                       wt.getAllGuidDeclarationsFromPackages(vpid, EnumerationData.GUID_TYPE_HOB));
        this.iComboBoxGuidC_Name.insertItemAt(DataType.EMPTY_SELECT_ITEM, 0);
        this.iComboBoxGuidC_Name.setSelectedIndex(0);


        if (this.id != null) {
            String tmpName = id.getName();
            if (isEmpty(tmpName)) {
                tmpName = DataType.EMPTY_SELECT_ITEM;
            }
            this.iComboBoxGuidC_Name.setSelectedItem(tmpName);    
            this.jComboBoxHobType.setSelectedItem(id.getType());
            this.jComboBoxUsage.setSelectedItem(id.getUsage());
            this.jTextAreaHelpText.setText(id.getHelp());
            this.jTextFieldFeatureFlag.setText(id.getFeatureFlag());
            this.jArchCheckBox.setSelectedItems(id.getSupArchList());
        }
    }

    /**
     * This is the override edit constructor
     * 
     * @param inHobsIdentification
     * @param iFrame
     * 
     */
    public HobsDlg(HobsIdentification inHobsIdentification, IFrame iFrame, ModuleIdentification mid) {
        super(iFrame, true);
        init(inHobsIdentification, mid);
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
            this.iComboBoxGuidC_Name.setEnabled(!isView);
            this.jComboBoxUsage.setEnabled(!isView);
            this.jComboBoxHobType.setEnabled(!isView);
        }
    }

    /**
     * This method initializes jContentPane
     * 
     * @return javax.swing.JPanel jContentPane
     * 
     */
    public JPanel getJContentPane() {
        if (jContentPane == null) {
            jStarLabel1 = new StarLabel();
            jStarLabel1.setLocation(new java.awt.Point(2, 12));
            jLabelC_Name = new JLabel();
            jLabelC_Name.setText("Hob's Guid C Name");
            jLabelC_Name.setBounds(new java.awt.Rectangle(12, 12, 155, 20));

            jStarLabel2 = new StarLabel();
            jStarLabel2.setLocation(new java.awt.Point(2, 37));
            jLabelHobType = new JLabel();
            jLabelHobType.setText("Hob Type");
            jLabelHobType.setBounds(new java.awt.Rectangle(12, 37, 155, 20));

            jStarLabel3 = new StarLabel();
            jStarLabel3.setLocation(new java.awt.Point(2, 62));
            jLabelUsage = new JLabel();
            jLabelUsage.setText("Usage");
            jLabelUsage.setBounds(new java.awt.Rectangle(12, 62, 155, 20));

            jLabelHelpText = new JLabel();
            jLabelHelpText.setBounds(new java.awt.Rectangle(12, 87, 155, 20));
            jLabelHelpText.setText("Help Text");

            jLabelFeatureFlag = new JLabel();
            jLabelFeatureFlag.setBounds(new java.awt.Rectangle(12, 157, 155, 20));
            jLabelFeatureFlag.setText("Feature Flag Expression");
            jLabelFeatureFlag.setEnabled(false);

            jLabelArch = new JLabel();
            jLabelArch.setBounds(new java.awt.Rectangle(12, 132, 155, 20));
            jLabelArch.setText("Supported Architectures");
            jArchCheckBox = new ArchCheckBox();
            jArchCheckBox.setBounds(new java.awt.Rectangle(168, 132, 320, 20));
            jArchCheckBox.setPreferredSize(new java.awt.Dimension(320, 20));

            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.setPreferredSize(new java.awt.Dimension(485, 215));

            jContentPane.add(jLabelC_Name, null);
            jContentPane.add(getIComboBoxGuidC_Name(), null);
            jContentPane.add(jStarLabel2, null);
            jContentPane.add(jLabelHobType, null);
            jContentPane.add(getJComboBoxHobType(), null);
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

    /**
     * This method initializes Usage type and Hob type
     * 
     */
    private void initFrame() {
        Tools.generateComboBoxByVector(jComboBoxUsage, ed.getVHobUsage());
        Tools.generateComboBoxByVector(jComboBoxHobType, ed.getVHobType());
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
        // Check Name
        //
        String tmpName = this.iComboBoxGuidC_Name.getSelectedItem().toString();
        if (!tmpName.equals(DataType.EMPTY_SELECT_ITEM) && isEmpty(tmpName)) {
            if (!DataValidation.isC_NameType(this.iComboBoxGuidC_Name.getSelectedItem().toString())) {
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
        String arg0 = this.iComboBoxGuidC_Name.getSelectedItem().toString();
        String arg1 = this.jComboBoxHobType.getSelectedItem().toString();
        String arg2 = this.jComboBoxUsage.getSelectedItem().toString();

        String arg3 = this.jTextFieldFeatureFlag.getText();
        Vector<String> arg4 = this.jArchCheckBox.getSelectedItemsVector();
        String arg5 = this.jTextAreaHelpText.getText();
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
