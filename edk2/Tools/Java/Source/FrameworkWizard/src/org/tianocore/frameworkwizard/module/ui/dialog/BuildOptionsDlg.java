/** @file
 
 The file is used to create, update SourceFiles section of the MSA file
 
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
import org.tianocore.frameworkwizard.common.ui.IComboBox;
import org.tianocore.frameworkwizard.common.ui.IDialog;
import org.tianocore.frameworkwizard.common.ui.IFrame;
import org.tianocore.frameworkwizard.common.ui.StarLabel;
import org.tianocore.frameworkwizard.common.ui.iCheckBoxList.ICheckBoxList;
import org.tianocore.frameworkwizard.module.Identifications.ModuleIdentification;
import org.tianocore.frameworkwizard.module.Identifications.BuildOptions.BuildOptionsIdentification;
import org.tianocore.frameworkwizard.workspace.WorkspaceTools;

/**
 * The class is used to create, update SourceFiles section of the MSA file
 *
 * It extends IDialog
 * 
 **/
public class BuildOptionsDlg extends IDialog {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = -6765742852142775378L;

    //
    // Define class members
    //
    private JPanel jContentPane = null;

    private JLabel jLabelFileName = null;

    private JTextField jTextFieldFileOption = null;

    private JLabel jLabelToolChainFamily = null;

    private StarLabel jStarLabel1 = null;

    private JLabel jLabelArch = null;

    private JScrollPane jScrollPane = null;

    private JLabel jLabelTagName = null;

    private JTextField jTextFieldTagName = null;

    private JLabel jLabelToolCode = null;

    private JTextField jTextFieldToolCode = null;

    private IComboBox iComboBoxToolCode = null;

    private JTextField jTextFieldToolChainFamily = null;

    private ArchCheckBox jArchCheckBox = null;

    private JButton jButtonOk = null;

    private JButton jButtonCancel = null;

    private JScrollPane jScrollPaneBuildTargets = null;

    private ICheckBoxList iCheckBoxListBuildTargets = null;

    //
    // Not used by UI
    //
    private BuildOptionsIdentification id = null;

    private EnumerationData ed = new EnumerationData();

    private WorkspaceTools wt = new WorkspaceTools();

    private JLabel jLabelBuildTargets = null;

    /**
     This method initializes jTextFieldFileOption 
     
     @return javax.swing.JTextField jTextFieldFileOption
     
     **/
    private JTextField getJTextFieldSourceFilesDirectory() {
        if (jTextFieldFileOption == null) {
            jTextFieldFileOption = new JTextField();
            jTextFieldFileOption.setBounds(new java.awt.Rectangle(168, 12, 340, 20));
            jTextFieldFileOption.setPreferredSize(new java.awt.Dimension(340, 20));
            jTextFieldFileOption.setToolTipText("Path is relative to the MSA file and must include the file name");
        }
        return jTextFieldFileOption;
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
     * This method initializes jTextFieldTagName    
     *  
     * @return javax.swing.JTextField   
     */
    private JTextField getJTextFieldTagName() {
        if (jTextFieldTagName == null) {
            jTextFieldTagName = new JTextField();
            jTextFieldTagName.setBounds(new java.awt.Rectangle(168, 37, 340, 20));
            jTextFieldTagName.setPreferredSize(new java.awt.Dimension(340, 20));
            jTextFieldTagName.setToolTipText("You may specify a specific tool chain tag name, such as BILL1");
        }
        return jTextFieldTagName;
    }

    private IComboBox getIComboBoxToolCode() {
        if (iComboBoxToolCode == null) {
            iComboBoxToolCode = new IComboBox();
            iComboBoxToolCode.setBounds(new java.awt.Rectangle(168, 62, 340, 20));
            iComboBoxToolCode.setPreferredSize(new java.awt.Dimension(340, 20));
            iComboBoxToolCode.setToolTipText("<html>You may select a specific tool command from drop down list,<br>"
                                             + "or you can DOUBLE-CLICK this field to enter your customized<br>"
                                             + "tool command.<br>"
                                             + "Press ENTER to save your input or press ESCAPE to quit</html>");
        }
        return iComboBoxToolCode;
    }

    /**
     * This method initializes jTextFieldToolCode   
     *  
     * @return javax.swing.JTextField   
     */
    private JTextField getJTextFieldToolCode() {
        if (jTextFieldToolCode == null) {
            jTextFieldToolCode = new JTextField();
            jTextFieldToolCode.setBounds(new java.awt.Rectangle(168, 62, 340, 20));
            jTextFieldToolCode.setPreferredSize(new java.awt.Dimension(340, 20));
            jTextFieldToolCode.setToolTipText("You may specify a specific tool command, such as ASM");
            jTextFieldToolCode.setVisible(false);
        }
        return jTextFieldToolCode;
    }

    /**
     * This method initializes jTextFieldToolChainFamily    
     *  
     * @return javax.swing.JTextField   
     */
    private JTextField getJTextFieldToolChainFamily() {
        if (jTextFieldToolChainFamily == null) {
            jTextFieldToolChainFamily = new JTextField();
            jTextFieldToolChainFamily.setBounds(new java.awt.Rectangle(168, 87, 340, 20));
            jTextFieldToolChainFamily.setPreferredSize(new java.awt.Dimension(340, 20));
            jTextFieldToolChainFamily.setToolTipText("You may specify a specific tool chain family, such as GCC");
        }
        return jTextFieldToolChainFamily;
    }

    /**
     This method initializes jButtonOk    
     
     @return javax.swing.JButton  
     
     **/
    private JButton getJButtonOk() {
        if (jButtonOk == null) {
            jButtonOk = new JButton();
            jButtonOk.setBounds(new java.awt.Rectangle(317, 202, 90, 20));
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
            jButtonCancel.setBounds(new java.awt.Rectangle(412, 202, 90, 20));
            jButtonCancel.setText("Cancel");
            jButtonCancel.addActionListener(this);
        }
        return jButtonCancel;
    }

    /**
     This method initializes iCheckBoxListArch   
     
     @return ICheckBoxList   
     **/
    private ICheckBoxList getICheckBoxListSupModuleList() {
        if (iCheckBoxListBuildTargets == null) {
            iCheckBoxListBuildTargets = new ICheckBoxList();
        }
        return iCheckBoxListBuildTargets;
    }

    /**
     This method initializes jScrollPaneBuildTargets    
     
     @return javax.swing.JScrollPane  
     
     **/
    private JScrollPane getJScrollPaneBuildTargets() {
        if (jScrollPaneBuildTargets == null) {
            jScrollPaneBuildTargets = new JScrollPane();
            jScrollPaneBuildTargets.setBounds(new java.awt.Rectangle(168, 137, 340, 40));
            jScrollPaneBuildTargets.setPreferredSize(new java.awt.Dimension(340, 40));
            jScrollPaneBuildTargets.setViewportView(getICheckBoxListSupModuleList());
        }
        return jScrollPaneBuildTargets;
    }

    public static void main(String[] args) {

    }

    /**
     This is the default constructor
     
     **/
    public BuildOptionsDlg(BuildOptionsIdentification inBuildOptionsIdentification, IFrame iFrame,
                           ModuleIdentification mid) {
        super(iFrame, true);
        init(inBuildOptionsIdentification, mid);
    }

    /**
     This method initializes this
     
     **/
    private void init() {
        this.setSize(525, 270);
        this.setContentPane(getJScrollPane());
        this.setTitle("Source Files");
        this.setViewMode(false);
        this.centerWindow();
        Tools.generateComboBoxByVector(iComboBoxToolCode, ed.getVToolCode());
        this.iCheckBoxListBuildTargets.setAllItems(ed.getVBuildTargets());
    }

    /**         
     This method initializes this
     Fill values to all fields if these values are not empty
     
     
     @param inSourceFiles The input data of SourceFilesDocument.SourceFiles
     
     **/
    private void init(BuildOptionsIdentification inBuildOptionsIdentification, ModuleIdentification mid) {
        init();

        //
        // Init arch with module's arch
        //
        this.jArchCheckBox.setEnabledItems(wt.getModuleArch(mid));

        if (inBuildOptionsIdentification != null) {
            this.jTextFieldFileOption.setText(inBuildOptionsIdentification.getOption());
            this.jTextFieldTagName.setText(inBuildOptionsIdentification.getTagName());

            //
            // Generate Tool Code selection list
            //
            Vector<String> v = ed.getVToolCode();
            boolean isFind = false;
            String strToolCode = inBuildOptionsIdentification.getToolCode();

            //
            // If the input value is not in the default list, add it to the list
            //
            if (strToolCode != null) {
                for (int index = 0; index < v.size(); index++) {
                    if (v.elementAt(index).equals(strToolCode)) {
                        isFind = true;
                        break;
                    }
                }
                if (!isFind && !isEmpty(strToolCode)) {
                    v.addElement(strToolCode);
                }
            }

            Tools.generateComboBoxByVector(iComboBoxToolCode, v);
            this.iComboBoxToolCode.setSelectedItem(strToolCode);

            this.jTextFieldToolChainFamily.setText(inBuildOptionsIdentification.getToolChainFamily());

            this.jArchCheckBox.setSelectedItems(inBuildOptionsIdentification.getSupArchList());

            this.iCheckBoxListBuildTargets.setAllItemsUnchecked();
            this.iCheckBoxListBuildTargets.initCheckedItem(true, inBuildOptionsIdentification.getBuildTargets());
        }
    }

    /**
     Disable all components when the mode is view
     
     @param isView true - The view mode; false - The non-view mode
     
     **/
    public void setViewMode(boolean isView) {
        if (isView) {
            this.jTextFieldFileOption.setEnabled(!isView);
        }
    }

    /**
     This method initializes jContentPane
     
     @return javax.swing.JPanel jContentPane
     
     **/
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jLabelBuildTargets = new JLabel();
            jLabelBuildTargets.setBounds(new java.awt.Rectangle(12, 137, 155, 20));
            jLabelBuildTargets.setText("Build Targets");
            jArchCheckBox = new ArchCheckBox();
            jArchCheckBox.setBounds(new java.awt.Rectangle(168, 112, 340, 20));
            jArchCheckBox.setPreferredSize(new java.awt.Dimension(340, 20));
            jLabelToolCode = new JLabel();
            jLabelToolCode.setBounds(new java.awt.Rectangle(12, 62, 155, 20));
            jLabelToolCode.setText("Tool Code");
            jLabelTagName = new JLabel();
            jLabelTagName.setBounds(new java.awt.Rectangle(12, 37, 155, 20));
            jLabelTagName.setText("Tag Name");
            jLabelArch = new JLabel();
            jLabelArch.setBounds(new java.awt.Rectangle(12, 112, 155, 20));
            jLabelArch.setText("Supported Architectures");
            jLabelToolChainFamily = new JLabel();
            jLabelToolChainFamily.setBounds(new java.awt.Rectangle(12, 87, 155, 20));
            jLabelToolChainFamily.setText("Tool Chain Family");
            jLabelFileName = new JLabel();
            jLabelFileName.setText("Option String");
            jLabelFileName.setBounds(new java.awt.Rectangle(12, 12, 155, 20));

            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.setPreferredSize(new java.awt.Dimension(505, 222));

            jContentPane.add(jLabelFileName, null);
            jContentPane.add(getJTextFieldSourceFilesDirectory(), null);
            jContentPane.add(jLabelToolChainFamily, null);
            jStarLabel1 = new StarLabel();
            jStarLabel1.setLocation(new java.awt.Point(0, 10));

            jContentPane.add(jStarLabel1, null);
            jContentPane.add(jLabelArch, null);
            jContentPane.add(jLabelTagName, null);
            jContentPane.add(getJTextFieldTagName(), null);
            jContentPane.add(jLabelToolCode, null);
            jContentPane.add(getJTextFieldToolCode(), null);
            jContentPane.add(getIComboBoxToolCode(), null);
            jContentPane.add(getJTextFieldToolChainFamily(), null);
            jContentPane.add(jArchCheckBox, null);
            jContentPane.add(getJButtonOk(), null);
            jContentPane.add(getJButtonCancel(), null);
            jContentPane.add(jLabelBuildTargets, null);
            jContentPane.add(getJScrollPaneBuildTargets(), null);
        }
        return jContentPane;
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
                getCurrentId();
                this.returnType = DataType.RETURN_TYPE_OK;
                this.setVisible(false);
            }
        }

        if (arg0.getSource() == jButtonCancel) {
            this.returnType = DataType.RETURN_TYPE_CANCEL;
            this.setVisible(false);
        }
    }

    private BuildOptionsIdentification getCurrentId() {
        String arg0 = this.jTextFieldFileOption.getText();
        Vector<String> arg1 = this.iCheckBoxListBuildTargets.getAllCheckedItemsString();
        String arg2 = this.jTextFieldToolChainFamily.getText();
        String arg3 = this.jTextFieldTagName.getText();
        String arg4 = this.iComboBoxToolCode.getSelectedItem().toString();
        if (arg4.equals(DataType.EMPTY_SELECT_ITEM)) {
            arg4 = "";
        }
        Vector<String> arg5 = this.jArchCheckBox.getSelectedItemsVector();
        
        id = new BuildOptionsIdentification(arg0, arg1, arg2, arg3, arg4, arg5);
        return id;
    }

    /**
     Data validation for all fields
     
     @retval true - All datas are valid
     @retval false - At least one data is invalid
     
     **/
    public boolean checkAdd() {
        //
        // Check Option
        //
        if (isEmpty(this.jTextFieldFileOption.getText())) {
            Log.wrn("Update Build Options", "Option String must be entered!");
            return false;
        }

        //
        // Check TagName 
        //
        if (!isEmpty(this.jTextFieldTagName.getText())) {
            if (!DataValidation.isTagName(this.jTextFieldTagName.getText())) {
                Log.wrn("Update Build Options", "Incorrect data type for Tag Name");
                return false;
            }
        }

        //
        // Check ToolCode 
        //
        if (!isEmpty(this.jTextFieldToolCode.getText())) {
            if (!DataValidation.isToolCode(this.jTextFieldToolCode.getText())) {
                Log.wrn("Update Build Options", "Incorrect data type for Tool Code");
                return false;
            }
        }

        //
        // Check ToolChainFamily 
        //
        if (!isEmpty(this.jTextFieldToolChainFamily.getText())) {
            if (!DataValidation.isToolChainFamily(this.jTextFieldToolChainFamily.getText())) {
                Log.wrn("Update Build Options", "Incorrect data type for Tool Chain Family");
                return false;
            }
        }

        return true;
    }

    public BuildOptionsIdentification getId() {
        return id;
    }

    public void setId(BuildOptionsIdentification id) {
        this.id = id;
    }
}
