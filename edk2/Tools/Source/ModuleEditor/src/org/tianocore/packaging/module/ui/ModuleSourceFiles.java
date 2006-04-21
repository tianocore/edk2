/** @file
 
 The file is used to create, update SourceFile of MSA/MBD file
 
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
import java.awt.event.ItemEvent;
import java.awt.event.ItemListener;
import java.util.Vector;

import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JComboBox;
import javax.swing.JFileChooser;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JTextField;

import org.tianocore.FilenameDocument;
import org.tianocore.SourceFilesDocument;
import org.tianocore.SupportedArchitectures;
import org.tianocore.ToolChains;
import org.tianocore.common.DataType;
import org.tianocore.common.DataValidation;
import org.tianocore.common.Log;
import org.tianocore.common.Tools;
import org.tianocore.packaging.common.ui.IDefaultMutableTreeNode;
import org.tianocore.packaging.common.ui.IInternalFrame;
import org.tianocore.packaging.common.ui.StarLabel;
import org.tianocore.packaging.workspace.common.Workspace;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;

/**
 The class is used to create, update SourceFile of MSA/MBD file
 It extends IInternalFrame
 
 @since ModuleEditor 1.0

 **/
public class ModuleSourceFiles extends IInternalFrame implements ItemListener {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = -6765742852142775378L;

    //
    // Define class members
    //
    private SourceFilesDocument.SourceFiles sourceFiles = null;

    private int location = -1;
    
    private int intSelectedItemId = 0;

    //
    // 1 - Add; 2 - Update
    //
    private int operation = -1;

    private Workspace ws = new Workspace();

    private Vector<String> vFileName = new Vector<String>();

    private Vector<String> vGuid = new Vector<String>();

    private Vector<String> vPath = new Vector<String>();

    private Vector<String> vFileType = new Vector<String>();

    private Vector<String> vToolChain = new Vector<String>();

    private Vector<String> vOverrideID = new Vector<String>();

    private JPanel jContentPane = null;

    private JButton jButtonOk = null;

    private JButton jButtonCancel = null;

    private JLabel jLabelFileName = null;

    private JTextField jTextFieldFileName = null;

    private JButton jButtonOpenFile = null;

    private JLabel jLabelGuid = null;

    private JTextField jTextFieldGuid = null;

    private JButton jButtonGenerateGuid = null;

    private JComboBox jComboBoxArch = null;

    private JLabel jLabelPath = null;

    private JTextField jTextFieldPath = null;

    private JLabel jLabelFileType = null;

    private JTextField jTextFieldFileType = null;

    private JLabel jLabelOverrideID = null;

    private JTextField jTextFieldOverrideID = null;

    private StarLabel jStarLabel1 = null;

    private JComboBox jComboBoxFileList = null;

    private JButton jButtonAdd = null;

    private JButton jButtonRemove = null;

    private JButton jButtonUpdate = null;

    private JCheckBox jCheckBoxArch = null;

    private JLabel jLabelToolChain = null;

    private JComboBox jComboBoxToolChain = null;

    private JScrollPane jScrollPaneFileList = null;

    private JTextArea jTextAreaFileList = null;

    /**
     This method initializes jButtonOk 
     
     @return javax.swing.JButton jButtonOk
     
     **/
    private JButton getJButton() {
        if (jButtonOk == null) {
            jButtonOk = new JButton();
            jButtonOk.setText("OK");
            jButtonOk.setBounds(new java.awt.Rectangle(290, 340, 90, 20));
            jButtonOk.addActionListener(this);
        }
        return jButtonOk;
    }

    /**
     This method initializes jButtonCancel 
     
     @return javax.swing.JButton jButtonCancel
     
     **/
    private JButton getJButton1() {
        if (jButtonCancel == null) {
            jButtonCancel = new JButton();
            jButtonCancel.setText("Cancel");
            jButtonCancel.setBounds(new java.awt.Rectangle(390, 340, 90, 20));
            jButtonCancel.setPreferredSize(new java.awt.Dimension(90, 20));
            jButtonCancel.addActionListener(this);
        }
        return jButtonCancel;
    }

    /**
     This method initializes jTextFieldFileName 
     
     @return javax.swing.JTextField jTextFieldFileName
     
     **/
    private JTextField getJTextFieldSourceFilesDirectory() {
        if (jTextFieldFileName == null) {
            jTextFieldFileName = new JTextField();
            jTextFieldFileName.setBounds(new java.awt.Rectangle(140,10,250,20));
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
            jButtonOpenFile.setBounds(new java.awt.Rectangle(395,10,85,20));
            jButtonOpenFile.addActionListener(this);
        }
        return jButtonOpenFile;
    }

    /**
     This method initializes jTextFieldGuid 
     
     @return javax.swing.JTextField jTextFieldGuid
     
     **/
    private JTextField getJTextFieldGuid() {
        if (jTextFieldGuid == null) {
            jTextFieldGuid = new JTextField();
            jTextFieldGuid.setBounds(new java.awt.Rectangle(140,35,250,20));
        }
        return jTextFieldGuid;
    }

    /**
     This method initializes jButtonGenerateGuid 
     
     @return javax.swing.JButton jButtonGenerateGuid
     
     **/
    private JButton getJButtonGenerateGuid() {
        if (jButtonGenerateGuid == null) {
            jButtonGenerateGuid = new JButton();
            jButtonGenerateGuid.setBounds(new java.awt.Rectangle(395,35,85,20));
            jButtonGenerateGuid.setText("GEN");
            jButtonGenerateGuid.addActionListener(this);
        }
        return jButtonGenerateGuid;
    }

    /**
     This method initializes jComboBoxArch 
     
     @return javax.swing.JComboBox jComboBoxArch
     
     **/
    private JComboBox getJComboBoxArch() {
        if (jComboBoxArch == null) {
            jComboBoxArch = new JComboBox();
            jComboBoxArch.setBounds(new java.awt.Rectangle(140, 160, 340, 20));
            jComboBoxArch.setEnabled(false);
        }
        return jComboBoxArch;
    }

    /**
     This method initializes jTextFieldPath 
     
     @return javax.swing.JTextField jTextFieldPath
     
     **/
    private JTextField getJTextFieldPath() {
        if (jTextFieldPath == null) {
            jTextFieldPath = new JTextField();
            jTextFieldPath.setBounds(new java.awt.Rectangle(140, 60, 340, 20));
        }
        return jTextFieldPath;
    }

    /**
     This method initializes jTextFieldFileType 
     
     @return javax.swing.JTextField jTextFieldFileType
     
     **/
    private JTextField getJTextFieldFileType() {
        if (jTextFieldFileType == null) {
            jTextFieldFileType = new JTextField();
            jTextFieldFileType.setBounds(new java.awt.Rectangle(140, 85, 340, 20));
        }
        return jTextFieldFileType;
    }

    /**
     This method initializes jTextFieldOverrideID 
     
     @return javax.swing.JTextField jTextFieldOverrideID
     
     **/
    private JTextField getJTextFieldOverrideID() {
        if (jTextFieldOverrideID == null) {
            jTextFieldOverrideID = new JTextField();
            jTextFieldOverrideID.setBounds(new java.awt.Rectangle(140, 135, 50, 20));
        }
        return jTextFieldOverrideID;
    }

    /**
     This method initializes jComboBoxFileList 
     
     @return javax.swing.JComboBox jComboBoxFileList
     
     **/
    private JComboBox getJComboBoxFileList() {
        if (jComboBoxFileList == null) {
            jComboBoxFileList = new JComboBox();
            jComboBoxFileList.setBounds(new java.awt.Rectangle(15, 185, 210, 20));
            jComboBoxFileList.addItemListener(this);
            jComboBoxFileList.addActionListener(this);
        }
        return jComboBoxFileList;
    }

    /**
     This method initializes jButtonAdd 
     
     @return javax.swing.JButton jButtonAdd
     
     **/
    private JButton getJButtonAdd() {
        if (jButtonAdd == null) {
            jButtonAdd = new JButton();
            jButtonAdd.setBounds(new java.awt.Rectangle(230, 185, 80, 20));
            jButtonAdd.setText("Add");
            jButtonAdd.addActionListener(this);
        }
        return jButtonAdd;
    }

    /**
     This method initializes jButtonRemove 
     
     @return javax.swing.JButton jButtonRemove
     
     **/
    private JButton getJButtonRemove() {
        if (jButtonRemove == null) {
            jButtonRemove = new JButton();
            jButtonRemove.setBounds(new java.awt.Rectangle(400, 185, 80, 20));
            jButtonRemove.setText("Remove");
            jButtonRemove.addActionListener(this);
        }
        return jButtonRemove;
    }

    /**
     This method initializes jButtonUpdate 
     
     @return javax.swing.JButton jButtonUpdate
     
     **/
    private JButton getJButtonUpdate() {
        if (jButtonUpdate == null) {
            jButtonUpdate = new JButton();
            jButtonUpdate.setBounds(new java.awt.Rectangle(315, 185, 80, 20));
            jButtonUpdate.setText("Update");
            jButtonUpdate.addActionListener(this);
        }
        return jButtonUpdate;
    }

    /**
     This method initializes jCheckBoxArch 
     
     @return javax.swing.JCheckBox jCheckBoxArch
     
     **/
    private JCheckBox getJCheckBoxArch() {
        if (jCheckBoxArch == null) {
            jCheckBoxArch = new JCheckBox();
            jCheckBoxArch.setBounds(new java.awt.Rectangle(15, 160, 120, 20));
            jCheckBoxArch.setText("Specific Arch");
            jCheckBoxArch.addActionListener(this);
        }
        return jCheckBoxArch;
    }

    /**
     This method initializes jComboBoxToolChain 
     
     @return javax.swing.JComboBox jComboBoxToolChain
     
     **/
    private JComboBox getJComboBoxToolChain() {
        if (jComboBoxToolChain == null) {
            jComboBoxToolChain = new JComboBox();
            jComboBoxToolChain.setBounds(new java.awt.Rectangle(140, 110, 340, 20));
        }
        return jComboBoxToolChain;
    }

    /**
     * This method initializes jScrollPaneFileList	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPaneFileList() {
        if (jScrollPaneFileList == null) {
            jScrollPaneFileList = new JScrollPane();
            jScrollPaneFileList.setBounds(new java.awt.Rectangle(15,210,465,240));
            jScrollPaneFileList.setViewportView(getJTextAreaFileList());
        }
        return jScrollPaneFileList;
    }

    /**
     * This method initializes jTextAreaFileList	
     * 	
     * @return javax.swing.JTextArea	
     */
    private JTextArea getJTextAreaFileList() {
        if (jTextAreaFileList == null) {
            jTextAreaFileList = new JTextArea();
            jTextAreaFileList.setEditable(false);
        }
        return jTextAreaFileList;
    }

    public static void main(String[] args) {

    }

    /**
     This is the default constructor
     
     **/
    public ModuleSourceFiles() {
        super();
        init();
        this.setVisible(true);
    }

    /**
     This is the override edit constructor
     
     @param inSourceFiles The input data of SourceFilesDocument.SourceFiles
     
     **/
    public ModuleSourceFiles(SourceFilesDocument.SourceFiles inSourceFiles) {
        super();
        init(inSourceFiles);
        this.setVisible(true);
    }

    /**
     This is the override edit constructor
     
     @param inSourceFiles The input data of SourceFilesDocument.SourceFiles
     @param type The input data of node type
     @param index The input data of node index
     @param inOperation The input data of current operation type
     
     **/
    public ModuleSourceFiles(SourceFilesDocument.SourceFiles inSourceFiles, int type, int index, int inOperation) {
        super();
        init(inSourceFiles, type, index, inOperation);
        this.operation = inOperation;
        this.setVisible(true);
    }

    /**
     This method initializes this
     
     @param inSourceFiles The input data of SourceFilesDocument.SourceFiles
     
     **/
    private void init(SourceFilesDocument.SourceFiles inSourceFiles) {
        init();
        this.setSourceFiles(inSourceFiles);
    }

    /**         
     This method initializes this
     Fill values to all fields if these values are not empty
     
     
     @param inSourceFiles The input data of SourceFilesDocument.SourceFiles
     @param type The input data of node type
     @param index The input data of node index
     @param inOperation The input data of current operation type
     
     **/
    private void init(SourceFilesDocument.SourceFiles inSourceFiles, int type, int index, int inOperation) {
        init(inSourceFiles);
        this.location = index;
        this.operation = inOperation;

        //
        // When update
        //
        if (operation == 2) {
            this.jCheckBoxArch.setEnabled(false);
            this.jComboBoxArch.setEnabled(false);

            if (type == IDefaultMutableTreeNode.SOURCEFILES_FILENAME) {
                if (this.sourceFiles.getFilenameList().size() > 0) {
                    for (int indexI = 0; indexI < this.sourceFiles.getFilenameList().size(); indexI++) {
                        if (this.sourceFiles.getFilenameArray(indexI).getStringValue() != null) {
                            vFileName.addElement(this.sourceFiles.getFilenameArray(indexI).getStringValue());
                        } else {
                            vFileName.addElement("");
                        }
                        if (this.sourceFiles.getFilenameArray(indexI).getGuid() != null) {
                            vGuid.addElement(this.sourceFiles.getFilenameArray(indexI).getGuid());
                        } else {
                            vGuid.addElement("");
                        }
                        if (this.sourceFiles.getFilenameArray(indexI).getPath() != null) {
                            vPath.addElement(this.sourceFiles.getFilenameArray(indexI).getPath());
                        } else {
                            vPath.addElement("");
                        }
                        if (this.sourceFiles.getFilenameArray(indexI).getFileType() != null) {
                            vFileType.addElement(this.sourceFiles.getFilenameArray(indexI).getFileType());
                        } else {
                            vFileType.addElement("");
                        }
                        if (this.sourceFiles.getFilenameArray(indexI).getToolChain() != null) {
                            vToolChain.addElement(this.sourceFiles.getFilenameArray(indexI).getToolChain().toString());
                        } else {
                            vToolChain.addElement(DataType.EMPTY_SELECT_ITEM);
                        }
                        vOverrideID.addElement(String.valueOf(this.sourceFiles.getFilenameArray(indexI).getOverrideID()));
                        jComboBoxFileList.addItem(this.sourceFiles.getFilenameArray(indexI).getStringValue());
                    }
                }
            }
            if (type == IDefaultMutableTreeNode.SOURCEFILES_ARCH_ITEM) {
                this.jCheckBoxArch.setSelected(true);
                this.jComboBoxArch.setSelectedItem(this.sourceFiles.getArchArray(index).getArchType().toString());
                for (int indexI = 0; indexI < this.sourceFiles.getArchArray(index).getFilenameList().size(); indexI++) {
                    if (this.sourceFiles.getArchArray(index).getFilenameArray(indexI).getStringValue() != null) {
                        vFileName.addElement(this.sourceFiles.getArchArray(index).getFilenameArray(indexI)
                                                             .getStringValue());
                    } else {
                        vFileName.addElement("");
                    }
                    if (this.sourceFiles.getArchArray(index).getFilenameArray(indexI).getGuid() != null) {
                        vGuid.addElement(this.sourceFiles.getArchArray(index).getFilenameArray(indexI).getGuid());
                    } else {
                        vGuid.addElement("");
                    }
                    if (this.sourceFiles.getArchArray(index).getFilenameArray(indexI).getPath() != null) {
                        vPath.addElement(this.sourceFiles.getArchArray(index).getFilenameArray(indexI).getPath());
                    } else {
                        vPath.addElement("");
                    }
                    if (this.sourceFiles.getArchArray(index).getFilenameArray(indexI).getFileType() != null) {
                        vFileType.addElement(this.sourceFiles.getArchArray(index).getFilenameArray(indexI)
                                                             .getFileType());
                    } else {
                        vFileType.addElement("");
                    }
                    if (this.sourceFiles.getArchArray(index).getFilenameArray(indexI).getToolChain() != null) {
                        vToolChain.addElement(this.sourceFiles.getArchArray(index).getFilenameArray(indexI)
                                                              .getToolChain().toString());
                    } else {
                        vToolChain.addElement("");
                    }
                    vOverrideID.addElement(String.valueOf(this.sourceFiles.getArchArray(index).getFilenameArray(indexI)
                                                           .getOverrideID()));
                    jComboBoxFileList.addItem(this.sourceFiles.getArchArray(index).getFilenameArray(indexI)
                                                              .getStringValue());
                }
            }
            //
            // Update the file list
            //
            reloadFileListArea();
        }
    }

    /**
     This method initializes this
     
     **/
    private void init() {
        this.setSize(500, 515);
        this.setContentPane(getJContentPane());
        this.setTitle("Source Files");
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
            this.jButtonOk.setEnabled(!isView);
            this.jButtonCancel.setEnabled(!isView);
            this.jTextFieldFileName.setEnabled(!isView);
            this.jButtonOpenFile.setEnabled(!isView);
            this.jTextFieldGuid.setEnabled(!isView);
            this.jButtonGenerateGuid.setEnabled(!isView);
            this.jTextFieldPath.setEnabled(!isView);
            this.jTextFieldFileType.setEnabled(!isView);
            this.jComboBoxToolChain.setEnabled(!isView);
            this.jTextFieldOverrideID.setEnabled(!isView);

            this.jButtonAdd.setEnabled(!isView);
            this.jButtonRemove.setEnabled(!isView);
            this.jButtonUpdate.setEnabled(!isView);
            this.jCheckBoxArch.setEnabled(!isView);
        }
    }

    /**
     This method initializes jContentPane
     
     @return javax.swing.JPanel jContentPane
     
     **/
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jLabelToolChain = new JLabel();
            jLabelToolChain.setBounds(new java.awt.Rectangle(15, 110, 120, 20));
            jLabelToolChain.setText("Tool Chain");
            jLabelOverrideID = new JLabel();
            jLabelOverrideID.setBounds(new java.awt.Rectangle(15, 135, 120, 20));
            jLabelOverrideID.setText("Override ID");
            jLabelFileType = new JLabel();
            jLabelFileType.setBounds(new java.awt.Rectangle(15, 85, 120, 20));
            jLabelFileType.setText("File Type");
            jLabelPath = new JLabel();
            jLabelPath.setBounds(new java.awt.Rectangle(15, 60, 120, 20));
            jLabelPath.setText("Path");
            jLabelGuid = new JLabel();
            jLabelGuid.setBounds(new java.awt.Rectangle(15, 35, 120, 20));
            jLabelGuid.setText("Guid");
            jLabelFileName = new JLabel();
            jLabelFileName.setText("File Name");
            jLabelFileName.setBounds(new java.awt.Rectangle(15, 10, 120, 20));
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.setSize(new java.awt.Dimension(480,400));
            jContentPane.add(getJButton(), null);
            jContentPane.add(getJButton1(), null);
            jContentPane.add(jLabelFileName, null);
            jContentPane.add(getJTextFieldSourceFilesDirectory(), null);
            jContentPane.add(getJButtonOpenFile(), null);
            jContentPane.add(jLabelGuid, null);
            jContentPane.add(getJTextFieldGuid(), null);
            jContentPane.add(getJButtonGenerateGuid(), null);
            jContentPane.add(getJComboBoxArch(), null);
            jContentPane.add(jLabelPath, null);
            jContentPane.add(getJTextFieldPath(), null);
            jContentPane.add(jLabelFileType, null);
            jContentPane.add(getJTextFieldFileType(), null);
            jContentPane.add(jLabelOverrideID, null);
            jContentPane.add(getJTextFieldOverrideID(), null);
            jStarLabel1 = new StarLabel();
            jStarLabel1.setLocation(new java.awt.Point(0, 10));

            jContentPane.add(jStarLabel1, null);
            jContentPane.add(getJComboBoxFileList(), null);
            jContentPane.add(getJButtonAdd(), null);
            jContentPane.add(getJButtonRemove(), null);
            jContentPane.add(getJButtonUpdate(), null);
            jContentPane.add(getJCheckBoxArch(), null);
            jContentPane.add(jLabelToolChain, null);
            jContentPane.add(getJComboBoxToolChain(), null);

            jContentPane.add(getJScrollPaneFileList(), null);
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
            this.setEdited(true);
            this.save();
            this.dispose();
        }
        if (arg0.getSource() == jButtonCancel) {
            this.dispose();
        }
        if (arg0.getSource() == jButtonGenerateGuid) {
            jTextFieldGuid.setText(Tools.generateUuidString());
        }
        if (arg0.getSource() == jButtonOpenFile) {
            selectFile();
        }
        if (arg0.getSource() == jButtonAdd) {
            if (!checkAdd()) {
                return;
            }
            addToList();
        }
        if (arg0.getSource() == jButtonRemove) {
            removeFromList();
        }
        if (arg0.getSource() == jButtonUpdate) {
            if (!checkAdd()) {
                return;
            }
            updateForList();
        }

        //
        //When and only when checked Arch box then can choose different arch types
        //
        if (arg0.getSource() == jCheckBoxArch) {
            if (this.jCheckBoxArch.isSelected()) {
                this.jComboBoxArch.setEnabled(true);
            } else {
                this.jComboBoxArch.setEnabled(false);
            }
        }
    }

    /**
     This method initializes Usage type and Arch type
     
     **/
    private void initFrame() {
        jComboBoxArch.addItem("ALL");
        jComboBoxArch.addItem("EBC");
        jComboBoxArch.addItem("ARM");
        jComboBoxArch.addItem("IA32");
        jComboBoxArch.addItem("X64");
        jComboBoxArch.addItem("IPF");
        jComboBoxArch.addItem("PPC");

        jComboBoxToolChain.addItem(DataType.EMPTY_SELECT_ITEM);
        jComboBoxToolChain.addItem("MSFT");
        jComboBoxToolChain.addItem("INTEL");
        jComboBoxToolChain.addItem("GCC");
        jComboBoxToolChain.addItem("CYGWIN");
    }

    /**
     Add current item to Vector
     
     **/
    private void addToList() {
        intSelectedItemId = vFileName.size();
        vFileName.addElement(this.jTextFieldFileName.getText());
        vGuid.addElement(this.jTextFieldGuid.getText());
        vPath.addElement(this.jTextFieldPath.getText());
        vFileType.addElement(this.jTextFieldFileType.getText());
        vToolChain.addElement(this.jComboBoxToolChain.getSelectedItem().toString());
        vOverrideID.addElement(this.jTextFieldOverrideID.getText());
        jComboBoxFileList.addItem(this.jTextFieldFileName.getText());
        jComboBoxFileList.setSelectedItem(this.jTextFieldFileName.getText());
        
        //
        // Reset select item index
        //
        intSelectedItemId = vFileName.size();
        
        //
        // Reload all fields of selected item
        //
        reloadFromList();
    }

    /**
     Remove current item from Vector
     
     **/
    private void removeFromList() {
        int intTempIndex = intSelectedItemId;
        if (vFileName.size() < 1) {
            return;
        }
        
        jComboBoxFileList.removeItemAt(intSelectedItemId);
        
        vFileName.removeElementAt(intTempIndex);
        vGuid.removeElementAt(intTempIndex);
        vPath.removeElementAt(intTempIndex);
        vFileType.removeElementAt(intTempIndex);
        vToolChain.removeElementAt(intTempIndex);
        vOverrideID.removeElementAt(intTempIndex);
        
        //
        // Reload all fields of selected item
        //
        reloadFromList();
    }

    /**
     Update current item of Vector
     
     **/
    private void updateForList() {
        //
        // Backup selected item index
        //
        int intTempIndex = intSelectedItemId;
        vFileName.setElementAt(this.jTextFieldFileName.getText(), intSelectedItemId);
        vGuid.setElementAt(this.jTextFieldGuid.getText(), intSelectedItemId);
        vPath.setElementAt(this.jTextFieldPath.getText(), intSelectedItemId);
        vFileType.setElementAt(this.jTextFieldFileType.getText(), intSelectedItemId);
        vToolChain.setElementAt(this.jComboBoxToolChain.getSelectedItem().toString(), intSelectedItemId);
        vOverrideID.setElementAt(this.jTextFieldOverrideID.getText(), intSelectedItemId);
        jComboBoxFileList.removeAllItems();
        for (int index = 0; index < vFileName.size(); index++) {
            jComboBoxFileList.addItem(vFileName.elementAt(index));
        }
        
        //
        // Restore selected item index
        //
        intSelectedItemId = intTempIndex;
        
        //
        // Reset select item index
        //
        jComboBoxFileList.setSelectedIndex(intSelectedItemId);
        
        //
        // Reload all fields of selected item
        //
        reloadFromList();
    }

    /**
     Refresh all fields' values of selected item of Vector
     
     **/
    private void reloadFromList() {
        if (vFileName.size() > 0) {
            //
            // Get selected item index
            //
            intSelectedItemId = jComboBoxFileList.getSelectedIndex();
            
            this.jTextFieldFileName.setText(vFileName.elementAt(intSelectedItemId).toString());
            this.jTextFieldGuid.setText(vGuid.elementAt(intSelectedItemId).toString());
            this.jTextFieldPath.setText(vPath.elementAt(intSelectedItemId).toString());
            this.jTextFieldFileType.setText(vFileType.elementAt(intSelectedItemId).toString());
            this.jComboBoxToolChain.setSelectedItem(vToolChain.elementAt(intSelectedItemId).toString());
            this.jTextFieldOverrideID.setText(vOverrideID.elementAt(intSelectedItemId).toString());    
        } else {
            this.jTextFieldFileName.setText("");
            this.jTextFieldGuid.setText("");
            this.jTextFieldPath.setText("");
            this.jTextFieldFileType.setText("");
            this.jComboBoxToolChain.setSelectedItem(DataType.EMPTY_SELECT_ITEM);
            this.jTextFieldOverrideID.setText("");
        }
        
        reloadFileListArea();
    }

    /* (non-Javadoc)
     * @see java.awt.event.ItemListener#itemStateChanged(java.awt.event.ItemEvent)
     *
     * Reflesh the frame when selected item changed
     * 
     */
    public void itemStateChanged(ItemEvent arg0) {
        if (arg0.getStateChange() == ItemEvent.SELECTED) {
            reloadFromList();
        }
    }

    /**
     Get SourceFilesDocument.SourceFiles
     
     @return SourceFilesDocument.SourceFiles
     
     **/
    public SourceFilesDocument.SourceFiles getSourceFiles() {
        return sourceFiles;
    }

    /**
     Set SourceFilesDocument.SourceFiles
     
     @param sourceFiles The input data of SourceFilesDocument.SourceFiles
     
     **/
    public void setSourceFiles(SourceFilesDocument.SourceFiles sourceFiles) {
        this.sourceFiles = sourceFiles;
    }

    /**
     Data validation for all fields
     
     @retval true - All datas are valid
     @retval false - At least one data is invalid
     
     **/
    public boolean check() {
        if (this.jComboBoxFileList.getItemCount() < 1) {
            Log.err("Must have one file at least!");
            return false;
        }
        return true;
    }

    /**
     Data validation for all fields
     
     @retval true - All datas are valid
     @retval false - At least one data is invalid
     
     **/
    public boolean checkAdd() {
        //
        // Check if all required fields are not empty
        //
        if (isEmpty(this.jTextFieldFileName.getText())) {
            Log.err("File Name couldn't be empty");
            return false;
        }

        //
        // Check if all fields have correct data types 
        //
        if (!DataValidation.isFileName(this.jTextFieldFileName.getText())) {
            Log.err("Incorrect data type for File Name");
            return false;
        }
        if (!isEmpty(this.jTextFieldGuid.getText()) && !DataValidation.isGuid(this.jTextFieldGuid.getText())) {
            Log.err("Incorrect data type for Guid");
            return false;
        }
        if (!isEmpty(this.jTextFieldPath.getText()) && !DataValidation.isPath(this.jTextFieldPath.getText())) {
            Log.err("Incorrect data type for Path");
            return false;
        }
        if (!isEmpty(this.jTextFieldOverrideID.getText())
            && !DataValidation.isOverrideID(this.jTextFieldOverrideID.getText())) {
            Log.err("Incorrect data type for Override ID");
            return false;
        }

        return true;
    }

    /**
     Save all components of SourceFiles
     if exists sourceFiles, set the value directly
     if not exists sourceFiles, new an instance first
     
     **/
    public void save() {
        try {
            if (this.sourceFiles == null) {
                sourceFiles = SourceFilesDocument.SourceFiles.Factory.newInstance();
            }
            //
            //Save as file name
            //
            if (!this.jCheckBoxArch.isSelected()) {
                if (this.operation == 2) { //Add new filename
                    //
                    //First remove all existed filename
                    //
                    if (sourceFiles.getFilenameList().size() > 0) {
                        for (int index = sourceFiles.getFilenameList().size() - 1; index >= 0; index--) {
                            sourceFiles.removeFilename(index);
                        }
                    }
                }
                for (int index = 0; index < vFileName.size(); index++) {
                    FilenameDocument.Filename filename = FilenameDocument.Filename.Factory.newInstance();
                    if (!isEmpty(vFileName.elementAt(index).toString())) {
                        filename.setStringValue(vFileName.elementAt(index).toString());
                    }
                    if (!isEmpty(vGuid.elementAt(index).toString())) {
                        filename.setGuid(vGuid.elementAt(index).toString());
                    }
                    if (!isEmpty(vPath.elementAt(index).toString())) {
                        filename.setPath(vPath.elementAt(index).toString());
                    }
                    if (!isEmpty(vFileType.elementAt(index).toString())) {
                        filename.setFileType(vFileType.elementAt(index).toString());
                    }
                    if (!vToolChain.elementAt(index).toString().equals(DataType.EMPTY_SELECT_ITEM)) {
                        filename.setToolChain(ToolChains.Enum.forString(vToolChain.elementAt(index).toString()));
                    }
                    if (!isEmpty(vOverrideID.elementAt(index).toString())) {
                        filename.setOverrideID(Integer.parseInt(vOverrideID.elementAt(index).toString()));
                    }
                    sourceFiles.addNewFilename();
                    sourceFiles.setFilenameArray(sourceFiles.getFilenameList().size() - 1, filename);
                }
            }
            //
            //Save as Arch
            //
            if (this.jCheckBoxArch.isSelected()) {
                SourceFilesDocument.SourceFiles.Arch arch = SourceFilesDocument.SourceFiles.Arch.Factory.newInstance();
                if (this.operation == 2) {
                    //
                    //First remove all existed filename
                    //
                    for (int index = sourceFiles.getArchArray(location).getFilenameList().size() - 1; index >= 0; index--) {
                        sourceFiles.getArchArray(location).removeFilename(index);
                    }
                }
                for (int index = 0; index < vFileName.size(); index++) {
                    FilenameDocument.Filename filename = FilenameDocument.Filename.Factory.newInstance();
                    if (!isEmpty(vFileName.elementAt(index).toString())) {
                        filename.setStringValue(vFileName.elementAt(index).toString());
                    }
                    if (!isEmpty(vGuid.elementAt(index).toString())) {
                        filename.setGuid(vGuid.elementAt(index).toString());
                    }
                    if (!isEmpty(vPath.elementAt(index).toString())) {
                        filename.setPath(vPath.elementAt(index).toString());
                    }
                    if (!isEmpty(vFileType.elementAt(index).toString())) {
                        filename.setFileType(vFileType.elementAt(index).toString());
                    }
                    if (!vToolChain.elementAt(index).toString().equals(DataType.EMPTY_SELECT_ITEM)) {
                        filename.setToolChain(ToolChains.Enum.forString(vToolChain.elementAt(index).toString()));
                    }
                    if (!isEmpty(vOverrideID.elementAt(index).toString())) {
                        filename.setOverrideID(Integer.parseInt(vOverrideID.elementAt(index).toString()));
                    }
                    arch.addNewFilename();
                    arch.setFilenameArray(arch.getFilenameList().size() - 1, filename);
                }
                arch
                    .setArchType(SupportedArchitectures.Enum.forString(this.jComboBoxArch.getSelectedItem().toString()));
                if (location > -1) {
                    sourceFiles.setArchArray(location, arch);
                } else {
                    sourceFiles.addNewArch();
                    sourceFiles.setArchArray(sourceFiles.getArchList().size() - 1, arch);
                }
            }
        } catch (Exception e) {
            Log.err("Update Source Files", e.getMessage());
        }
    }

    /**
     Display a file open browser to let user select file
     
     **/
    private void selectFile() {
        JFileChooser fc = new JFileChooser(ws.getCurrentWorkspace());

        int result = fc.showOpenDialog(new JPanel());
        if (result == JFileChooser.APPROVE_OPTION) {
            this.jTextFieldFileName.setText(fc.getSelectedFile().getName());
        }
    }
    
    /**
    Update file list pane via the elements of Vector
    
    **/
    private void reloadFileListArea() {
        String strFileList = "";
        for (int index = 0; index < vFileName.size(); index++) {
            strFileList = strFileList + vFileName.elementAt(index).toString() + DataType.UNXI_LINE_SEPARATOR;
        }
        this.jTextAreaFileList.setText(strFileList);
    }
}
