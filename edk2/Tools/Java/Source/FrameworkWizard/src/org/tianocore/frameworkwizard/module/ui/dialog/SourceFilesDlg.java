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
import java.io.File;
import java.util.Vector;

import javax.swing.JButton;
import javax.swing.JFileChooser;
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
import org.tianocore.frameworkwizard.module.Identifications.ModuleIdentification;
import org.tianocore.frameworkwizard.module.Identifications.SourceFiles.SourceFilesIdentification;
import org.tianocore.frameworkwizard.workspace.WorkspaceTools;

/**
 * The class is used to create, update SourceFiles section of the MSA file
 *
 * It extends IDialog
 * 
 **/
public class SourceFilesDlg extends IDialog {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = -6765742852142775378L;

    //
    // Define class members
    //
    private JPanel jContentPane = null;

    private JLabel jLabelFileName = null;

    private JTextField jTextFieldFileName = null;

    private JButton jButtonOpenFile = null;

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

    private JLabel jLabelFeatureFlag = null;

    private JTextField jTextFieldFeatureFlag = null;

    private ArchCheckBox jArchCheckBox = null;

    private JButton jButtonOk = null;

    private JButton jButtonCancel = null;

    //
    // Not used by UI
    //
    private SourceFilesIdentification sfid[] = null;

    private String msaFileName = "";

    private EnumerationData ed = new EnumerationData();
    
    private WorkspaceTools wt = new WorkspaceTools();

    /**
     This method initializes jTextFieldFileName 
     
     @return javax.swing.JTextField jTextFieldFileName
     
     **/
    private JTextField getJTextFieldSourceFilesDirectory() {
        if (jTextFieldFileName == null) {
            jTextFieldFileName = new JTextField();
            jTextFieldFileName.setBounds(new java.awt.Rectangle(168, 12, 250, 20));
            jTextFieldFileName.setPreferredSize(new java.awt.Dimension(250, 20));
            jTextFieldFileName.setToolTipText("Path is relative to the MSA file and must include the file name");
        }
        return jTextFieldFileName;
    }

    /**
     This method initializes jButtonOpenFile 
     
     @return javax.swing.JButton jButtonOpenFile
     
     **/
    private JButton getJButtonOpenFile() {
        if (jButtonOpenFile == null) {
            jButtonOpenFile = new JButton();
            jButtonOpenFile.setText("Browse");
            jButtonOpenFile.setBounds(new java.awt.Rectangle(422, 12, 85, 20));
            jButtonOpenFile.setPreferredSize(new java.awt.Dimension(85, 20));
            jButtonOpenFile.addActionListener(this);
        }
        return jButtonOpenFile;
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
     * This method initializes jTextFieldFeatureFlag	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldFeatureFlag() {
        if (jTextFieldFeatureFlag == null) {
            jTextFieldFeatureFlag = new JTextField();
            jTextFieldFeatureFlag.setBounds(new java.awt.Rectangle(168, 137, 340, 20));
            jTextFieldFeatureFlag.setPreferredSize(new java.awt.Dimension(340, 20));
            jTextFieldFeatureFlag.setToolTipText("RESERVED FOR FUTURE USE");
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
            jButtonOk.setBounds(new java.awt.Rectangle(317, 172, 90, 20));
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
            jButtonCancel.setBounds(new java.awt.Rectangle(412, 172, 90, 20));
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
    public SourceFilesDlg(SourceFilesIdentification inSourceFilesIdentification, IFrame iFrame, ModuleIdentification mid) {
        super(iFrame, true);
        init(inSourceFilesIdentification, mid);
    }

    /**
     This method initializes this
     
     **/
    private void init() {
        this.setSize(525, 240);
        this.setContentPane(getJScrollPane());
        this.setTitle("Source Files");
        this.setViewMode(false);
        this.centerWindow();
        Tools.generateComboBoxByVector(iComboBoxToolCode, ed.getVToolCode());
    }

    /**         
     This method initializes this
     Fill values to all fields if these values are not empty
     
     
     @param inSourceFiles The input data of SourceFilesDocument.SourceFiles
     
     **/
    private void init(SourceFilesIdentification inSourceFilesIdentifications, ModuleIdentification mid) {
        init();
        this.msaFileName = mid.getPath();
        
        //
        // Init arch with module's arch
        //
        this.jArchCheckBox.setEnabledItems(wt.getModuleArch(mid));

        if (inSourceFilesIdentifications != null) {
            this.jTextFieldFileName.setText(inSourceFilesIdentifications.getFilename());
            this.jTextFieldTagName.setText(inSourceFilesIdentifications.getTagName());

//            //
//            // Generate Tool Code selection list
//            //
//            Vector<String> v = ed.getVToolCode();
//            boolean isFind = false;
//            String strToolCode = inSourceFilesIdentifications.getToolCode();
//
//            //
//            // If the input value is not in the default list, add it to the list
//            //
//            if (strToolCode != null) {
//                for (int index = 0; index < v.size(); index++) {
//                    if (v.elementAt(index).equals(strToolCode)) {
//                        isFind = true;
//                        break;
//                    }
//                }
//                if (!isFind && !isEmpty(strToolCode)) {
//                    v.addElement(strToolCode);
//                }
//            }

            Tools.generateComboBoxByVector(iComboBoxToolCode, ed.getVToolCode());
            this.iComboBoxToolCode.setSelectedItem(inSourceFilesIdentifications.getToolCode());

            this.jTextFieldToolChainFamily.setText(inSourceFilesIdentifications.getToolChainFamily());
            jTextFieldFeatureFlag.setText(inSourceFilesIdentifications.getFeatureFlag());
            this.jArchCheckBox.setSelectedItems(inSourceFilesIdentifications.getSupArchList());
        }
    }

    /**
     Disable all components when the mode is view
     
     @param isView true - The view mode; false - The non-view mode
     
     **/
    public void setViewMode(boolean isView) {
        if (isView) {
            this.jTextFieldFileName.setEnabled(!isView);
            this.jButtonOpenFile.setEnabled(!isView);
        }
    }

    /**
     This method initializes jContentPane
     
     @return javax.swing.JPanel jContentPane
     
     **/
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jArchCheckBox = new ArchCheckBox();
            jArchCheckBox.setBounds(new java.awt.Rectangle(168, 112, 340, 20));
            jArchCheckBox.setPreferredSize(new java.awt.Dimension(340, 20));
            jLabelFeatureFlag = new JLabel();
            jLabelFeatureFlag.setBounds(new java.awt.Rectangle(12, 137, 155, 20));
            jLabelFeatureFlag.setText("Feature Flag Expression");
            jLabelFeatureFlag.setEnabled(false);
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
            jLabelFileName.setText("File Name");
            jLabelFileName.setBounds(new java.awt.Rectangle(12, 12, 155, 20));

            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.setPreferredSize(new java.awt.Dimension(505, 192));

            jContentPane.add(jLabelFileName, null);
            jContentPane.add(getJTextFieldSourceFilesDirectory(), null);
            jContentPane.add(getJButtonOpenFile(), null);
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
            jContentPane.add(jLabelFeatureFlag, null);
            jContentPane.add(getJTextFieldFeatureFlag(), null);
            jContentPane.add(jArchCheckBox, null);
            jContentPane.add(getJButtonOk(), null);
            jContentPane.add(getJButtonCancel(), null);
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
        if (arg0.getSource() == jButtonOpenFile) {
            selectFile();
        }

        if (arg0.getSource() == jButtonOk) {
            if (checkAdd()) {
                getCurrentSourceFiles();
                this.returnType = DataType.RETURN_TYPE_OK;
                this.setVisible(false);
            }
        }

        if (arg0.getSource() == jButtonCancel) {
            this.returnType = DataType.RETURN_TYPE_CANCEL;
            this.setVisible(false);
        }
    }

    private SourceFilesIdentification[] getCurrentSourceFiles() {
        String name = this.jTextFieldFileName.getText();
        String s[] = name.split(";");
        String tagName = this.jTextFieldTagName.getText();
        String toolCode = this.iComboBoxToolCode.getSelectedItem().toString();
        if (toolCode.equals(DataType.EMPTY_SELECT_ITEM)) {
            toolCode = "";
        }
        String tcf = this.jTextFieldToolChainFamily.getText();
        String featureFlag = this.jTextFieldFeatureFlag.getText();
        Vector<String> arch = this.jArchCheckBox.getSelectedItemsVector();
        sfid = new SourceFilesIdentification[s.length];
        for (int index = 0; index < s.length; index++) {
            sfid[index] = new SourceFilesIdentification(s[index], tagName, toolCode, tcf, featureFlag, arch);
        }
        return sfid;
    }

    /**
     Data validation for all fields
     
     @retval true - All datas are valid
     @retval false - At least one data is invalid
     
     **/
    public boolean checkAdd() {
        //
        // Check Filename
        //
        if (isEmpty(this.jTextFieldFileName.getText())) {
            Log.wrn("Update Source Files", "File Name must be entered!");
            return false;
        }
        if (!DataValidation.isFilename(this.jTextFieldFileName.getText())) {
            Log.wrn("Update Source Files", "Incorrect data type for File Name");
            return false;
        }

        //
        // Check TagName 
        //
        if (!isEmpty(this.jTextFieldTagName.getText())) {
            if (!DataValidation.isTagName(this.jTextFieldTagName.getText())) {
                Log.wrn("Update Source Files", "Incorrect data type for Tag Name");
                return false;
            }
        }

        //
        // Check ToolCode 
        //
        if (!isEmpty(this.jTextFieldToolCode.getText())) {
            if (!DataValidation.isToolCode(this.jTextFieldToolCode.getText())) {
                Log.wrn("Update Source Files", "Incorrect data type for Tool Code");
                return false;
            }
        }

        //
        // Check ToolChainFamily 
        //
        if (!isEmpty(this.jTextFieldToolChainFamily.getText())) {
            if (!DataValidation.isToolChainFamily(this.jTextFieldToolChainFamily.getText())) {
                Log.wrn("Update Source Files", "Incorrect data type for Tool Chain Family");
                return false;
            }
        }

        //
        // Check FeatureFlag
        //
        if (!isEmpty(this.jTextFieldFeatureFlag.getText())) {
            if (!DataValidation.isFeatureFlag(this.jTextFieldFeatureFlag.getText())) {
                Log.wrn("Update Source Files", "Incorrect data type for Feature Flag");
                return false;
            }
        }

        return true;
    }

    /**
     Display a file open browser to let user select file
     
     **/
    private void selectFile() {
        JFileChooser fc = new JFileChooser();
        fc.setCurrentDirectory(new File(Tools.getFilePathOnly(msaFileName)));
        fc.setMultiSelectionEnabled(true);
        int result = fc.showOpenDialog(new JPanel());
        if (result == JFileChooser.APPROVE_OPTION) {
            File f[] = fc.getSelectedFiles();
            String s = "";
            for (int index = 0; index < f.length; index++) {
                String relativePath = "";
                relativePath = Tools.getRelativePath(Tools.getFilePathOnly(f[index].getPath()), Tools.getFilePathOnly(msaFileName));
                if (!Tools.isEmpty(relativePath)) {
                    relativePath = relativePath + DataType.UNIX_FILE_SEPARATOR;
                }
                s = s + relativePath + f[index].getName() + ";";
            }
            this.jTextFieldFileName.setText(s);
        }
    }

    public SourceFilesIdentification[] getSfid() {
        return sfid;
    }

    public void setSfid(SourceFilesIdentification[] sfid) {
        this.sfid = sfid;
    }
}
