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
package org.tianocore.frameworkwizard.module.ui;

import java.awt.event.ActionEvent;
import java.awt.event.ComponentEvent;
import java.awt.event.ItemEvent;
import java.util.Vector;

import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JComboBox;
import javax.swing.JFileChooser;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;
import javax.swing.JTextField;

import org.tianocore.SourceFilesDocument;
import org.tianocore.FilenameDocument.Filename;
import org.tianocore.ModuleSurfaceAreaDocument.ModuleSurfaceArea;
import org.tianocore.SourceFilesDocument.SourceFiles;
import org.tianocore.frameworkwizard.common.DataType;
import org.tianocore.frameworkwizard.common.DataValidation;
import org.tianocore.frameworkwizard.common.EnumerationData;
import org.tianocore.frameworkwizard.common.Log;
import org.tianocore.frameworkwizard.common.OpeningModuleType;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.ui.IInternalFrame;
import org.tianocore.frameworkwizard.common.ui.StarLabel;
import org.tianocore.frameworkwizard.common.ui.iCheckBoxList.ICheckBoxList;
import org.tianocore.frameworkwizard.module.Identification.SourceFiles.SourceFilesIdentification;
import org.tianocore.frameworkwizard.module.Identification.SourceFiles.SourceFilesVector;
import org.tianocore.frameworkwizard.workspace.Workspace;

/**
 The class is used to create, update SourceFile of MSA/MBD file
 It extends IInternalFrame
 


 **/
public class ModuleSourceFiles extends IInternalFrame {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = -6765742852142775378L;

    //
    // Define class members
    //
    private SourceFilesDocument.SourceFiles sourceFiles = null;

    private int intSelectedItemId = 0;

    private JPanel jContentPane = null;

    private JLabel jLabelFileName = null;

    private JTextField jTextFieldFileName = null;

    private JButton jButtonOpenFile = null;

    private JLabel jLabelToolChainFamily = null;

    private StarLabel jStarLabel1 = null;

    private JComboBox jComboBoxList = null;

    private JButton jButtonAdd = null;

    private JButton jButtonRemove = null;

    private JButton jButtonUpdate = null;

    private JCheckBox jCheckBoxArch = null;

    private JScrollPane jScrollPaneList = null;

    private JTextArea jTextAreaList = null;

    private JLabel jLabelArch = null;

    private JScrollPane jScrollPane = null;

    private JLabel jLabelTagName = null;

    private JTextField jTextFieldTagName = null;

    private JLabel jLabelToolCode = null;

    private JTextField jTextFieldToolCode = null;

    private JTextField jTextFieldToolChainFamily = null;

    private JLabel jLabelFeatureFlag = null;

    private JTextField jTextFieldFeatureFlag = null;

    private ICheckBoxList iCheckBoxListArch = null;

    private JScrollPane jScrollPaneArch = null;

    //
    // Not used by UI
    //
    private OpeningModuleType omt = null;
    
    private ModuleSurfaceArea msa = null;

    private SourceFilesIdentification sfid = null;

    private SourceFilesVector vSourceFiles = new SourceFilesVector();

    /**
     This method initializes jTextFieldFileName 
     
     @return javax.swing.JTextField jTextFieldFileName
     
     **/
    private JTextField getJTextFieldSourceFilesDirectory() {
        if (jTextFieldFileName == null) {
            jTextFieldFileName = new JTextField();
            jTextFieldFileName.setBounds(new java.awt.Rectangle(140, 10, 250, 20));
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
            jButtonOpenFile.setBounds(new java.awt.Rectangle(395, 10, 85, 20));
            jButtonOpenFile.setPreferredSize(new java.awt.Dimension(85, 20));
            jButtonOpenFile.addActionListener(this);
        }
        return jButtonOpenFile;
    }

    /**
     This method initializes jComboBoxFileList 
     
     @return javax.swing.JComboBox jComboBoxFileList
     
     **/
    private JComboBox getJComboBoxList() {
        if (jComboBoxList == null) {
            jComboBoxList = new JComboBox();
            jComboBoxList.setBounds(new java.awt.Rectangle(15, 220, 210, 20));
            jComboBoxList.addItemListener(this);
            jComboBoxList.addActionListener(this);
            jComboBoxList.setPreferredSize(new java.awt.Dimension(210, 20));
        }
        return jComboBoxList;
    }

    /**
     This method initializes jButtonAdd 
     
     @return javax.swing.JButton jButtonAdd
     
     **/
    private JButton getJButtonAdd() {
        if (jButtonAdd == null) {
            jButtonAdd = new JButton();
            jButtonAdd.setBounds(new java.awt.Rectangle(230, 220, 80, 20));
            jButtonAdd.setText("Add");
            jButtonAdd.addActionListener(this);
            jButtonAdd.setPreferredSize(new java.awt.Dimension(80, 20));
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
            jButtonRemove.setBounds(new java.awt.Rectangle(400, 220, 80, 20));
            jButtonRemove.setText("Remove");
            jButtonRemove.addActionListener(this);
            jButtonRemove.setPreferredSize(new java.awt.Dimension(80, 20));
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
            jButtonUpdate.setBounds(new java.awt.Rectangle(315, 220, 80, 20));
            jButtonUpdate.setPreferredSize(new java.awt.Dimension(80, 20));
            jButtonUpdate.setText("Update");
            jButtonUpdate.addActionListener(this);
        }
        return jButtonUpdate;
    }

    /**
     * This method initializes jScrollPaneFileList	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPaneList() {
        if (jScrollPaneList == null) {
            jScrollPaneList = new JScrollPane();
            jScrollPaneList.setBounds(new java.awt.Rectangle(15, 245, 465, 240));
            jScrollPaneList.setViewportView(getJTextAreaList());
            jScrollPaneList.setPreferredSize(new java.awt.Dimension(465, 240));
        }
        return jScrollPaneList;
    }

    /**
     * This method initializes jTextAreaFileList	
     * 	
     * @return javax.swing.JTextArea	
     */
    private JTextArea getJTextAreaList() {
        if (jTextAreaList == null) {
            jTextAreaList = new JTextArea();
            jTextAreaList.setEditable(false);

        }
        return jTextAreaList;
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
            jTextFieldTagName.setBounds(new java.awt.Rectangle(140, 35, 340, 20));
            jTextFieldTagName.setPreferredSize(new java.awt.Dimension(340, 20));
            jTextFieldTagName.setToolTipText("You may specify a specific tool chain tag name, such as BILL1");
        }
        return jTextFieldTagName;
    }

    /**
     * This method initializes jTextFieldToolCode	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldToolCode() {
        if (jTextFieldToolCode == null) {
            jTextFieldToolCode = new JTextField();
            jTextFieldToolCode.setBounds(new java.awt.Rectangle(140, 60, 340, 20));
            jTextFieldToolCode.setPreferredSize(new java.awt.Dimension(340, 20));
            jTextFieldToolCode.setToolTipText("You may specify a specific tool command, such as ASM");
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
            jTextFieldToolChainFamily.setBounds(new java.awt.Rectangle(140, 85, 340, 20));
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
            jTextFieldFeatureFlag.setBounds(new java.awt.Rectangle(140, 110, 340, 20));
            jTextFieldFeatureFlag.setPreferredSize(new java.awt.Dimension(340, 20));
            jTextFieldFeatureFlag.setToolTipText("RESERVED FOR FUTURE USE");
        }
        return jTextFieldFeatureFlag;
    }

    /**
     This method initializes iCheckBoxListArch   
     
     @return ICheckBoxList   
     **/
    private ICheckBoxList getICheckBoxListSupportedArchitectures() {
        if (iCheckBoxListArch == null) {
            iCheckBoxListArch = new ICheckBoxList();
            iCheckBoxListArch.addFocusListener(this);
            iCheckBoxListArch.setToolTipText(DataType.SUP_ARCH_LIST_HELP_TEXT);
        }
        return iCheckBoxListArch;
    }

    /**
     This method initializes jScrollPaneArch 
     
     @return javax.swing.JScrollPane 
     
     **/
    private JScrollPane getJScrollPaneArch() {
        if (jScrollPaneArch == null) {
            jScrollPaneArch = new JScrollPane();
            jScrollPaneArch.setBounds(new java.awt.Rectangle(140, 135, 340, 80));
            jScrollPaneArch.setPreferredSize(new java.awt.Dimension(340, 80));
            jScrollPaneArch.setViewportView(getICheckBoxListSupportedArchitectures());
        }
        return jScrollPaneArch;
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
     
     @param 
     
     **/
    public ModuleSourceFiles(OpeningModuleType inOmt) {
        super();
        this.omt = inOmt;
        this.msa = inOmt.getXmlMsa();
        init(msa.getSourceFiles());
        this.setVisible(true);
    }

    /**         
     This method initializes this
     Fill values to all fields if these values are not empty
     
     
     @param inSourceFiles The input data of SourceFilesDocument.SourceFiles
     
     **/
    private void init(SourceFilesDocument.SourceFiles inSourceFiles) {
        init();
        this.sourceFiles = inSourceFiles;

        if (this.sourceFiles != null) {
            if (this.sourceFiles.getFilenameList().size() > 0) {
                for (int index = 0; index < this.sourceFiles.getFilenameList().size(); index++) {
                    String name = sourceFiles.getFilenameList().get(index).getStringValue();
                    String tagName = sourceFiles.getFilenameList().get(index).getTagName();
                    String toolCode = sourceFiles.getFilenameList().get(index).getToolCode();
                    String tcf = sourceFiles.getFilenameList().get(index).getToolChainFamily();
                    String featureFlag = sourceFiles.getFilenameList().get(index).getFeatureFlag();
                    Vector<String> arch = Tools.convertListToVector(sourceFiles.getFilenameList().get(index)
                                                                               .getSupArchList());
                    SourceFilesIdentification sfid = new SourceFilesIdentification(name, tagName, toolCode, tcf,
                                                                                   featureFlag, arch);
                    vSourceFiles.addSourceFiles(sfid);
                }
            }
        }
        //
        // Update the list
        //
        Tools.generateComboBoxByVector(jComboBoxList, vSourceFiles.getSourceFilesName());
        reloadListArea();
    }

    /**
     This method initializes this
     
     **/
    private void init() {
        this.setSize(500, 515);
        this.setContentPane(getJScrollPane());
        this.setTitle("Source Files");
        initFrame();
        this.setViewMode(false);
    }

    /**
     Disable all components when the mode is view
     
     @param isView true - The view mode; false - The non-view mode
     
     **/
    public void setViewMode(boolean isView) {
        if (isView) {
            this.jTextFieldFileName.setEnabled(!isView);
            this.jButtonOpenFile.setEnabled(!isView);

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
            jLabelFeatureFlag = new JLabel();
            jLabelFeatureFlag.setBounds(new java.awt.Rectangle(15, 110, 120, 20));
            jLabelFeatureFlag.setText("Feature Flag");
            jLabelToolCode = new JLabel();
            jLabelToolCode.setBounds(new java.awt.Rectangle(15, 60, 120, 20));
            jLabelToolCode.setText("Tool Code");
            jLabelTagName = new JLabel();
            jLabelTagName.setBounds(new java.awt.Rectangle(15, 35, 120, 20));
            jLabelTagName.setText("Tag Name");
            jLabelArch = new JLabel();
            jLabelArch.setBounds(new java.awt.Rectangle(15, 135, 120, 20));
            jLabelArch.setText("Sup Arch List");
            jLabelToolChainFamily = new JLabel();
            jLabelToolChainFamily.setBounds(new java.awt.Rectangle(15, 85, 120, 20));
            jLabelToolChainFamily.setText("Tool Chain Family");
            jLabelFileName = new JLabel();
            jLabelFileName.setText("File Name");
            jLabelFileName.setBounds(new java.awt.Rectangle(15, 10, 120, 20));

            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.setPreferredSize(new java.awt.Dimension(490, 490));

            jContentPane.add(jLabelFileName, null);
            jContentPane.add(getJTextFieldSourceFilesDirectory(), null);
            jContentPane.add(getJButtonOpenFile(), null);
            jContentPane.add(jLabelToolChainFamily, null);
            jStarLabel1 = new StarLabel();
            jStarLabel1.setLocation(new java.awt.Point(0, 10));

            jContentPane.add(jStarLabel1, null);
            jContentPane.add(getJComboBoxList(), null);
            jContentPane.add(getJButtonAdd(), null);
            jContentPane.add(getJButtonRemove(), null);
            jContentPane.add(getJButtonUpdate(), null);
            jContentPane.add(getJScrollPaneList(), null);
            jContentPane.add(jLabelArch, null);
            jContentPane.add(jLabelTagName, null);
            jContentPane.add(getJTextFieldTagName(), null);
            jContentPane.add(jLabelToolCode, null);
            jContentPane.add(getJTextFieldToolCode(), null);
            jContentPane.add(getJTextFieldToolChainFamily(), null);
            jContentPane.add(jLabelFeatureFlag, null);
            jContentPane.add(getJTextFieldFeatureFlag(), null);
            jContentPane.add(getJScrollPaneArch(), null);
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
    }

    /**
     This method initializes Usage type and Arch type
     
     **/
    private void initFrame() {
        EnumerationData ed = new EnumerationData();

        this.iCheckBoxListArch.setAllItems(ed.getVSupportedArchitectures());
    }

    private SourceFilesIdentification getCurrentSourceFiles() {
        String name = this.jTextFieldFileName.getText();
        String tagName = this.jTextFieldTagName.getText();
        String toolCode = this.jTextFieldToolCode.getText();
        String tcf = this.jTextFieldToolChainFamily.getText();
        String featureFlag = this.jTextFieldFeatureFlag.getText();
        Vector<String> arch = this.iCheckBoxListArch.getAllCheckedItemsString();
        sfid = new SourceFilesIdentification(name, tagName, toolCode, tcf, featureFlag, arch);
        return sfid;
    }

    /**
     Add current item to Vector
     
     **/
    private void addToList() {
        intSelectedItemId = vSourceFiles.size();

        vSourceFiles.addSourceFiles(getCurrentSourceFiles());

        jComboBoxList.addItem(sfid.getFilename());
        jComboBoxList.setSelectedItem(sfid.getFilename());

        //
        // Reset select item index
        //
        intSelectedItemId = vSourceFiles.size();

        //
        // Reload all fields of selected item
        //
        reloadFromList();

        // 
        // Save to memory
        //
        save();
    }

    /**
     Remove current item from Vector
     
     **/
    private void removeFromList() {
        //
        // Check if exist items
        //
        if (this.vSourceFiles.size() < 1) {
            return;
        }

        int intTempIndex = intSelectedItemId;

        jComboBoxList.removeItemAt(intSelectedItemId);

        vSourceFiles.removeSourceFiles(intTempIndex);

        //
        // Reload all fields of selected item
        //
        reloadFromList();

        // 
        // Save to memory
        //
        save();
    }

    /**
     Update current item of Vector
     
     **/
    private void updateForList() {
        //
        // Check if exist items
        //
        if (this.vSourceFiles.size() < 1) {
            return;
        }

        //
        // Backup selected item index
        //
        int intTempIndex = intSelectedItemId;

        vSourceFiles.updateSourceFiles(getCurrentSourceFiles(), intTempIndex);

        jComboBoxList.removeAllItems();
        for (int index = 0; index < vSourceFiles.size(); index++) {
            jComboBoxList.addItem(vSourceFiles.getSourceFiles(index).getFilename());
        }

        //
        // Restore selected item index
        //
        intSelectedItemId = intTempIndex;

        //
        // Reset select item index
        //
        jComboBoxList.setSelectedIndex(intSelectedItemId);

        //
        // Reload all fields of selected item
        //
        reloadFromList();

        // 
        // Save to memory
        //
        save();
    }

    /**
     Refresh all fields' values of selected item of Vector
     
     **/
    private void reloadFromList() {
        if (vSourceFiles.size() > 0) {
            //
            // Get selected item index
            //
            intSelectedItemId = jComboBoxList.getSelectedIndex();

            this.jTextFieldFileName.setText(vSourceFiles.getSourceFiles(intSelectedItemId).getFilename());
            this.jTextFieldTagName.setText(vSourceFiles.getSourceFiles(intSelectedItemId).getTagName());
            this.jTextFieldToolCode.setText(vSourceFiles.getSourceFiles(intSelectedItemId).getToolCode());
            this.jTextFieldToolChainFamily.setText(vSourceFiles.getSourceFiles(intSelectedItemId).getToolChainFamily());
            jTextFieldFeatureFlag.setText(vSourceFiles.getSourceFiles(intSelectedItemId).getFeatureFlag());
            iCheckBoxListArch.setAllItemsUnchecked();
            iCheckBoxListArch.initCheckedItem(true, vSourceFiles.getSourceFiles(intSelectedItemId).getSupArchList());

        } else {
        }

        reloadListArea();
    }

    /**
     Update list area pane via the elements of Vector
     
     **/
    private void reloadListArea() {
        String strListItem = "";
        for (int index = 0; index < vSourceFiles.size(); index++) {
            strListItem = strListItem + vSourceFiles.getSourceFiles(index).getFilename() + DataType.UNIX_LINE_SEPARATOR;
        }
        this.jTextAreaList.setText(strListItem);
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
     Data validation for all fields
     
     @retval true - All datas are valid
     @retval false - At least one data is invalid
     
     **/
    public boolean checkAdd() {
        //
        // Check Filename
        //
        if (isEmpty(this.jTextFieldFileName.getText())) {
            Log.err("File Name couldn't be empty");
            return false;
        }
        if (!DataValidation.isFilename(this.jTextFieldFileName.getText())) {
            Log.err("Incorrect data type for File Name");
            return false;
        }
        
        //
        // Check TagName 
        //
        if (!isEmpty(this.jTextFieldTagName.getText())) {
            if (!DataValidation.isTagName(this.jTextFieldTagName.getText())) {
                Log.err("Incorrect data type for Tag Name");
                return false;
            }
        }
        
        //
        // Check ToolCode 
        //
        if (!isEmpty(this.jTextFieldToolCode.getText())) {
            if (!DataValidation.isToolCode(this.jTextFieldToolCode.getText())) {
                Log.err("Incorrect data type for Tool Code");
                return false;
            }
        }
        
        //
        // Check ToolChainFamily 
        //
        if (!isEmpty(this.jTextFieldToolChainFamily.getText())) {
            if (!DataValidation.isToolChainFamily(this.jTextFieldToolChainFamily.getText())) {
                Log.err("Incorrect data type for Tool Chain Family");
                return false;
            }
        }
        
        //
        // Check FeatureFlag
        //
        if (!isEmpty(this.jTextFieldFeatureFlag.getText())) {
            if (!DataValidation.isFeatureFlag(this.jTextFieldFeatureFlag.getText())) {
                Log.err("Incorrect data type for Feature Flag");
                return false;
            }
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
            //
            //Save as file name
            //
            int count = this.vSourceFiles.size();

            this.sourceFiles = SourceFiles.Factory.newInstance();
            if (count > 0) {
                for (int index = 0; index < count; index++) {
                    Filename mFilename = Filename.Factory.newInstance();
                    if (!isEmpty(vSourceFiles.getSourceFiles(index).getFilename())) {
                        mFilename.setStringValue(vSourceFiles.getSourceFiles(index).getFilename());
                    }
                    if (!isEmpty(vSourceFiles.getSourceFiles(index).getTagName())) {
                        mFilename.setTagName(vSourceFiles.getSourceFiles(index).getTagName());
                    }
                    if (!isEmpty(vSourceFiles.getSourceFiles(index).getToolCode())) {
                        mFilename.setToolCode(vSourceFiles.getSourceFiles(index).getToolCode());
                    }
                    if (!isEmpty(vSourceFiles.getSourceFiles(index).getToolChainFamily())) {
                        mFilename.setToolChainFamily(vSourceFiles.getSourceFiles(index).getToolChainFamily());
                    }
                    if (!isEmpty(vSourceFiles.getSourceFiles(index).getFeatureFlag())) {
                        mFilename.setFeatureFlag(vSourceFiles.getSourceFiles(index).getFeatureFlag());
                    }
                    if (vSourceFiles.getSourceFiles(index).getSupArchList() != null
                        && vSourceFiles.getSourceFiles(index).getSupArchList().size() > 0) {
                        mFilename.setSupArchList(vSourceFiles.getSourceFiles(index).getSupArchList());
                    }

                    this.sourceFiles.addNewFilename();
                    this.sourceFiles.setFilenameArray(index, mFilename);
                }
            }
            this.msa.setSourceFiles(sourceFiles);
            this.omt.setSaved(false);
        } catch (Exception e) {
            e.printStackTrace();
            Log.err("Update Source Files", e.getMessage());
        }
    }

    /**
     Display a file open browser to let user select file
     
     **/
    private void selectFile() {
        JFileChooser fc = new JFileChooser(Workspace.getCurrentWorkspace());

        int result = fc.showOpenDialog(new JPanel());
        if (result == JFileChooser.APPROVE_OPTION) {
            this.jTextFieldFileName.setText(fc.getSelectedFile().getName());
        }
    }

    /* (non-Javadoc)
     * @see java.awt.event.ComponentListener#componentResized(java.awt.event.ComponentEvent)
     * 
     * Override componentResized to resize all components when frame's size is changed
     */
    public void componentResized(ComponentEvent arg0) {
        int intCurrentWidth = this.getJContentPane().getWidth();
        int intCurrentHeight = this.getJContentPane().getHeight();
        int intPreferredWidth = this.getJContentPane().getPreferredSize().width;
        int intPreferredHeight = this.getJContentPane().getPreferredSize().height;

        resizeComponentWidth(this.jTextFieldFileName, intCurrentWidth, intPreferredWidth);
        resizeComponentWidth(this.jTextFieldTagName, intCurrentWidth, intPreferredWidth);
        resizeComponentWidth(this.jTextFieldToolCode, intCurrentWidth, intPreferredWidth);
        resizeComponentWidth(this.jTextFieldToolChainFamily, intCurrentWidth, intPreferredWidth);
        resizeComponentWidth(this.jTextFieldFeatureFlag, intCurrentWidth, intPreferredWidth);
        resizeComponentWidth(this.jScrollPaneArch, intCurrentWidth, intPreferredWidth);

        resizeComponentWidth(this.jComboBoxList, intCurrentWidth, intPreferredWidth);
        resizeComponent(this.jScrollPaneList, intCurrentWidth, intCurrentHeight, intPreferredWidth, intPreferredHeight);
        relocateComponentX(this.jButtonAdd, intCurrentWidth, intPreferredWidth, DataType.SPACE_TO_RIGHT_FOR_ADD_BUTTON);
        relocateComponentX(this.jButtonOpenFile, intCurrentWidth, intPreferredWidth,
                           DataType.SPACE_TO_RIGHT_FOR_GENERATE_BUTTON);
        relocateComponentX(this.jButtonRemove, intCurrentWidth, intPreferredWidth,
                           DataType.SPACE_TO_RIGHT_FOR_REMOVE_BUTTON);
        relocateComponentX(this.jButtonUpdate, intCurrentWidth, intPreferredWidth,
                           DataType.SPACE_TO_RIGHT_FOR_UPDATE_BUTTON);
    }
}
