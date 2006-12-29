/** @file
 
 The file is used to create, update Library Class Definition section of the MSA file
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/
package org.tianocore.frameworkwizard.module.ui.dialog;

import java.awt.Dimension;
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
import org.tianocore.frameworkwizard.common.find.Find;
import org.tianocore.frameworkwizard.common.ui.ArchCheckBox;
import org.tianocore.frameworkwizard.common.ui.IComboBox;
import org.tianocore.frameworkwizard.common.ui.IDialog;
import org.tianocore.frameworkwizard.common.ui.IFrame;
import org.tianocore.frameworkwizard.common.ui.StarLabel;
import org.tianocore.frameworkwizard.common.ui.iCheckBoxList.ICheckBoxList;
import org.tianocore.frameworkwizard.module.Identifications.ModuleIdentification;
import org.tianocore.frameworkwizard.module.Identifications.LibraryClass.LibraryClassIdentification;
import org.tianocore.frameworkwizard.module.Identifications.LibraryClass.LibraryClassVector;
import org.tianocore.frameworkwizard.packaging.PackageIdentification;
import org.tianocore.frameworkwizard.workspace.WorkspaceTools;

/**
 * The class is used to create, update Library Class Definition section of the MSA file
 * 
 * It extends IDialog
 * 
 */
public class LibraryClassDefsDlg extends IDialog {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = -1743248695411382857L;

    //
    //Define class members
    //
    private JPanel jContentPane = null;

    private IComboBox iComboBoxLibraryClassName = null;

    private JLabel jLabelUsage = null;

    private JComboBox jComboBoxUsage = null;

    private JLabel jLabelLibraryClassName = null;

    private JScrollPane jScrollPane = null;

    private StarLabel jStarLabel1 = null;

    private StarLabel jStarLabel2 = null;

    private JLabel jLabelRecommendedInstanceVersion = null;

    private JTextField jTextFieldRecommendedInstanceVersion = null;

    private JLabel jLabelRecommendedInstanceGuid = null;

    private JTextField jTextFieldRecommendedInstanceGuid = null;

    private JButton jButtonGenerateGuid = null;

    private JLabel jLabelFeatureFlag = null;

    private JTextField jTextFieldFeatureFlag = null;

    private JLabel jLabelArch = null;

    private JLabel jLabelModuleList = null;

    private JScrollPane jScrollPaneModuleList = null;

    private ICheckBoxList iCheckBoxListModule = null;

    private JLabel jLabelHelpText = null;

    private JTextField jTextFieldHelpText = null;

    private ArchCheckBox jArchCheckBox = null;

    private JButton jButtonOk = null;

    private JButton jButtonCancel = null;

    //
    // Not for UI
    //
    private EnumerationData ed = new EnumerationData();

    private WorkspaceTools wt = new WorkspaceTools();

    private LibraryClassIdentification lcid = null;

    /**
     This method initializes jComboBoxSelect 
     
     @return javax.swing.JComboBox jComboBoxSelect
     
     **/
    private IComboBox getIComboBoxLibraryClassName() {
        if (iComboBoxLibraryClassName == null) {
            iComboBoxLibraryClassName = new IComboBox();
            iComboBoxLibraryClassName.setBounds(new java.awt.Rectangle(168, 12, 320, 20));
            iComboBoxLibraryClassName.setPreferredSize(new Dimension(320, 20));
            iComboBoxLibraryClassName.setEnabled(true);
        }
        return iComboBoxLibraryClassName;
    }

    /**
     This method initializes jComboBoxUsage 
     
     @return javax.swing.JComboBox jComboBoxUsage
     
     **/
    private JComboBox getJComboBoxUsage() {
        if (jComboBoxUsage == null) {
            jComboBoxUsage = new JComboBox();
            jComboBoxUsage.setBounds(new java.awt.Rectangle(168, 37, 320, 20));
            jComboBoxUsage.setPreferredSize(new Dimension(320, 20));
        }
        return jComboBoxUsage;
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
     * This method initializes jTextFieldRecommendedInstanceVersion	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldRecommendedInstanceVersion() {
        if (jTextFieldRecommendedInstanceVersion == null) {
            jTextFieldRecommendedInstanceVersion = new JTextField();
            jTextFieldRecommendedInstanceVersion.setPreferredSize(new java.awt.Dimension(260, 20));
            jTextFieldRecommendedInstanceVersion.setSize(new java.awt.Dimension(260, 20));
            jTextFieldRecommendedInstanceVersion.setLocation(new java.awt.Point(220, 85));
            jTextFieldRecommendedInstanceVersion.setVisible(false);
        }
        return jTextFieldRecommendedInstanceVersion;
    }

    /**
     * This method initializes jTextFieldRecommendedInstanceGuid	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldRecommendedInstanceGuid() {
        if (jTextFieldRecommendedInstanceGuid == null) {
            jTextFieldRecommendedInstanceGuid = new JTextField();
            jTextFieldRecommendedInstanceGuid.setBounds(new java.awt.Rectangle(220, 110, 190, 20));
            jTextFieldRecommendedInstanceGuid.setPreferredSize(new java.awt.Dimension(190, 20));
            jTextFieldRecommendedInstanceGuid.setVisible(false);
        }
        return jTextFieldRecommendedInstanceGuid;
    }

    /**
     * This method initializes jButtonGenerateGuid	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonGenerateGuid() {
        if (jButtonGenerateGuid == null) {
            jButtonGenerateGuid = new JButton();
            jButtonGenerateGuid.setBounds(new java.awt.Rectangle(415, 112, 65, 20));
            jButtonGenerateGuid.setPreferredSize(new java.awt.Dimension(65, 20));
            jButtonGenerateGuid.setText("GEN");
            jButtonGenerateGuid.setVisible(false);
            jButtonGenerateGuid.addActionListener(this);
        }
        return jButtonGenerateGuid;
    }

    /**
     * This method initializes jTextFieldFeatureFlag	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldFeatureFlag() {
        if (jTextFieldFeatureFlag == null) {
            jTextFieldFeatureFlag = new JTextField();
            jTextFieldFeatureFlag.setBounds(new java.awt.Rectangle(168, 197, 320, 20));
            jTextFieldFeatureFlag.setPreferredSize(new java.awt.Dimension(320, 20));
            jTextFieldFeatureFlag.setToolTipText("Postfix expression that must evaluate to TRUE or FALSE");
            jTextFieldFeatureFlag.setEnabled(false);
        }
        return jTextFieldFeatureFlag;
    }

    /**
     This method initializes iCheckBoxListArch   
     
     @return ICheckBoxList   
     **/
    private ICheckBoxList getICheckBoxListSupModuleList() {
        if (iCheckBoxListModule == null) {
            iCheckBoxListModule = new ICheckBoxList();
        }
        return iCheckBoxListModule;
    }

    /**
     This method initializes jScrollPaneModuleList	
     
     @return javax.swing.JScrollPane	
     
     **/
    private JScrollPane getJScrollPaneModuleList() {
        if (jScrollPaneModuleList == null) {
            jScrollPaneModuleList = new JScrollPane();
            jScrollPaneModuleList.setBounds(new java.awt.Rectangle(168, 112, 320, 80));
            jScrollPaneModuleList.setPreferredSize(new java.awt.Dimension(320, 80));
            jScrollPaneModuleList.setViewportView(getICheckBoxListSupModuleList());
        }
        return jScrollPaneModuleList;
    }

    /**
     This method initializes jTextFieldHelpText	
     
     @return javax.swing.JTextField	
     
     **/
    private JTextField getJTextFieldHelpText() {
        if (jTextFieldHelpText == null) {
            jTextFieldHelpText = new JTextField();
            jTextFieldHelpText.setBounds(new java.awt.Rectangle(168, 62, 320, 20));
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
            jButtonOk.setBounds(new java.awt.Rectangle(300, 232, 90, 20));
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
            jButtonCancel.setBounds(new java.awt.Rectangle(400, 232, 90, 20));
            jButtonCancel.setText("Cancel");
            jButtonCancel.addActionListener(this);
        }
        return jButtonCancel;
    }

    public static void main(String[] args) {

    }

    /**
     This is the default constructor
     
     **/
    public LibraryClassDefsDlg(LibraryClassIdentification inLibraryClassIdentification, IFrame iFrame,
                               ModuleIdentification mid) {
        super(iFrame, true);
        init(inLibraryClassIdentification, mid);
    }

    /**
     This method initializes this
     
     **/
    private void init() {
        this.setContentPane(getJScrollPane());
        this.setTitle("Library Class Definitions");
        this.setBounds(new java.awt.Rectangle(0, 0, 505, 305));
        initFrame();
        this.setViewMode(false);
        this.centerWindow();
    }

    /**
     This method initializes this
     
     **/
    private void init(LibraryClassIdentification inLibraryClassIdentification, ModuleIdentification mid) {
        init();
        this.lcid = inLibraryClassIdentification;

        //
        // Init arch with module's arch
        //
        this.jArchCheckBox.setEnabledItems(wt.getModuleArch(mid));

        //
        // Get defined library classes from dependent packages
        //
        Vector<PackageIdentification> vpid = wt.getPackageDependenciesOfModule(mid);
        if (vpid.size() <= 0) {
            Log.wrn("Init Library Class",
                    "This module hasn't defined any package dependency, so there is no library class can be added");
        }

        Tools
             .generateComboBoxByVector(
                                       this.iComboBoxLibraryClassName,
                                       wt
                                         .getAllLibraryClassDefinitionsFromPackages(wt
                                                                                      .getPackageDependenciesOfModule(mid)));

        if (lcid != null) {
            this.iComboBoxLibraryClassName.setSelectedItem(lcid.getLibraryClassName());
            this.jComboBoxUsage.setSelectedItem(lcid.getUsage());
            this.jTextFieldRecommendedInstanceVersion.setText(lcid.getRecommendedInstanceVersion());
            this.jTextFieldRecommendedInstanceGuid.setText(lcid.getRecommendedInstanceGuid());
            this.jTextFieldFeatureFlag.setText(lcid.getFeatureFlag());
            this.jArchCheckBox.setSelectedItems(lcid.getSupArchList());
            this.iCheckBoxListModule.setAllItemsUnchecked();
            this.iCheckBoxListModule.initCheckedItem(true, lcid.getSupModuleList());
            this.jTextFieldHelpText.setText(lcid.getHelp());
        }
    }

    /**
     Disable all components when the mode is view
     
     @param isView true - The view mode; false - The non-view mode
     
     **/
    public void setViewMode(boolean isView) {
        if (isView) {
            this.iComboBoxLibraryClassName.setEnabled(!isView);
            this.jComboBoxUsage.setEnabled(!isView);
        }
    }

    /**
     This method initializes jContentPane
     
     @return javax.swing.JPanel jContentPane
     
     **/
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jArchCheckBox = new ArchCheckBox();
            jArchCheckBox.setBounds(new java.awt.Rectangle(168, 87, 320, 20));
            jArchCheckBox.setPreferredSize(new java.awt.Dimension(320, 20));
            jLabelHelpText = new JLabel();
            jLabelHelpText.setBounds(new java.awt.Rectangle(12, 62, 168, 20));
            jLabelHelpText.setText("Help Text");
            jLabelModuleList = new JLabel();
            jLabelModuleList.setBounds(new java.awt.Rectangle(12, 112, 168, 20));
            jLabelModuleList.setText("Supported Module Types");
            jLabelArch = new JLabel();
            jLabelArch.setBounds(new java.awt.Rectangle(12, 87, 168, 20));
            jLabelArch.setText("Supported Architectures");
            jLabelFeatureFlag = new JLabel();
            jLabelFeatureFlag.setBounds(new java.awt.Rectangle(12, 197, 168, 20));
            jLabelFeatureFlag.setText("Feature Flag Expression");
            jLabelFeatureFlag.setEnabled(false);
            jLabelRecommendedInstanceGuid = new JLabel();
            jLabelRecommendedInstanceGuid.setBounds(new java.awt.Rectangle(12, 87, 200, 20));
            jLabelRecommendedInstanceGuid.setText("Recommended Instance Guid");
            jLabelRecommendedInstanceGuid.setVisible(false);
            jLabelRecommendedInstanceVersion = new JLabel();
            jLabelRecommendedInstanceVersion.setBounds(new java.awt.Rectangle(12, 87, 200, 20));
            jLabelRecommendedInstanceVersion.setText("Recommended Instance Version");
            jLabelRecommendedInstanceVersion.setVisible(false);
            jLabelLibraryClassName = new JLabel();
            jLabelLibraryClassName.setBounds(new java.awt.Rectangle(12, 12, 168, 20));
            jLabelLibraryClassName.setText("Library Class Name");
            jLabelUsage = new JLabel();
            jLabelUsage.setBounds(new java.awt.Rectangle(12, 37, 168, 20));
            jLabelUsage.setText("Usage");
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.setPreferredSize(new java.awt.Dimension(495, 255));

            jContentPane.add(getIComboBoxLibraryClassName(), null);
            jContentPane.add(jLabelUsage, null);
            jContentPane.add(getJComboBoxUsage(), null);
            jContentPane.add(jLabelLibraryClassName, null);
            jContentPane.add(jLabelRecommendedInstanceVersion, null);
            jContentPane.add(getJTextFieldRecommendedInstanceVersion(), null);
            jContentPane.add(jLabelRecommendedInstanceGuid, null);
            jContentPane.add(getJTextFieldRecommendedInstanceGuid(), null);
            jContentPane.add(getJButtonGenerateGuid(), null);
            jContentPane.add(jLabelFeatureFlag, null);
            jContentPane.add(getJTextFieldFeatureFlag(), null);
            jContentPane.add(jLabelArch, null);
            jStarLabel1 = new StarLabel();
            jStarLabel1.setLocation(new java.awt.Point(2, 10));
            jStarLabel2 = new StarLabel();
            jStarLabel2.setLocation(new java.awt.Point(2, 35));

            jContentPane.add(jStarLabel1, null);
            jContentPane.add(jStarLabel2, null);
            jContentPane.add(jLabelModuleList, null);
            jContentPane.add(getJScrollPaneModuleList(), null);
            jContentPane.add(jLabelHelpText, null);
            jContentPane.add(getJTextFieldHelpText(), null);
            jContentPane.add(jArchCheckBox, null);
            jContentPane.add(getJButtonOk(), null);
            jContentPane.add(getJButtonCancel(), null);
        }
        return jContentPane;
    }

    /**
     This method initializes all existing libraries and usage types
     
     **/
    private void initFrame() {
        Tools.generateComboBoxByVector(jComboBoxUsage, ed.getVLibraryUsage());
        this.iCheckBoxListModule.setAllItems(ed.getVFrameworkModuleTypes());
    }

    /* (non-Javadoc)
     * @see java.awt.event.ActionListener#actionPerformed(java.awt.event.ActionEvent)
     *
     * Override actionPerformed to listen all actions
     * 
     */
    public void actionPerformed(ActionEvent arg0) {
        if (arg0.getSource() == jButtonGenerateGuid) {
            this.jTextFieldRecommendedInstanceGuid.setText(Tools.generateUuidString());
        }

        if (arg0.getSource() == jButtonOk) {
            if (checkAdd()) {
                getCurrentLibraryClass();
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
        // Check LibraryClass
        //
        if (this.iComboBoxLibraryClassName.getSelectedItem() == null) {
            Log.wrn("Update Library Class Definitions", "Please select one Library Class");
            return false;
        }
        if (!DataValidation.isLibraryClass(this.iComboBoxLibraryClassName.getSelectedItem().toString())) {
            Log.wrn("Update Library Class Definitions", "Incorrect data type for Library Class");
            return false;
        }
        
        //
        // Check RecommendedInstanceVersion
        //
        //        if (!isEmpty(this.jTextFieldRecommendedInstanceVersion.getText())) {
        //            if (!DataValidation.isRecommendedInstanceVersion(this.jTextFieldRecommendedInstanceVersion.getText())) {
        //                Log.err("Incorrect data type for Recommended Instance Version");
        //                return false;
        //            }
        //        }

        //
        // Check RecommendedInstanceGuid
        //
        //        if (!isEmpty(this.jTextFieldRecommendedInstanceGuid.getText())) {
        //            if (!DataValidation.isGuid(this.jTextFieldRecommendedInstanceGuid.getText())) {
        //                Log.err("Incorrect data type for Recommended Instance Guid");
        //                return false;
        //            }
        //        }

        //
        // Check FeatureFlag
        //
        if (!isEmpty(this.jTextFieldFeatureFlag.getText())) {
            if (!DataValidation.isFeatureFlag(this.jTextFieldFeatureFlag.getText())) {
                Log.wrn("Update Library Class Definitions", "Incorrect data type for Feature Flag");
                return false;
            }
        }
        
        //
        // Check if the library is produced
        //
        String strUsage = this.jComboBoxUsage.getSelectedItem().toString();
        //
        // Check only when the library class is consumed
        //
        if (strUsage.equals(DataType.USAGE_TYPE_ALWAYS_CONSUMED) || strUsage.equals(DataType.USAGE_TYPE_SOMETIMES_CONSUMED)) {
            LibraryClassVector v = Find.getAllLibraryClassVector();
            boolean isFind = false;
            for (int index = 0; index < v.size(); index++) {
                LibraryClassIdentification lid = v.getLibraryClass(index);
                if (lid.getLibraryClassName().equals(this.iComboBoxLibraryClassName.getSelectedItem().toString())) {
                    if (lid.getUsage().equals(DataType.USAGE_TYPE_ALWAYS_PRODUCED)
                        || lid.getUsage().equals(DataType.USAGE_TYPE_SOMETIMES_PRODUCED)) {
                        isFind = true;
                        break;
                    }
                }
            }
            if (!isFind) {
                Log.wrn("Update Library Class Definitions", "This Library Class has no instance yet. It may have some errors in build time.");
                //return false;
            }
        }

        return true;
    }

    private LibraryClassIdentification getCurrentLibraryClass() {
        String name = this.iComboBoxLibraryClassName.getSelectedItem().toString();
        String usage = this.jComboBoxUsage.getSelectedItem().toString();
        String version = this.jTextFieldRecommendedInstanceVersion.getText();
        String guid = this.jTextFieldRecommendedInstanceGuid.getText();
        String featureFlag = this.jTextFieldFeatureFlag.getText();
        Vector<String> arch = this.jArchCheckBox.getSelectedItemsVector();
        Vector<String> module = this.iCheckBoxListModule.getAllCheckedItemsString();
        String help = this.jTextFieldHelpText.getText();
        lcid = new LibraryClassIdentification(name, usage, version, guid, arch, featureFlag, module, help);
        return lcid;
    }

    public LibraryClassIdentification getLcid() {
        return lcid;
    }

    public void setLcid(LibraryClassIdentification lcid) {
        this.lcid = lcid;
    }
}
