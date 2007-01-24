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

import java.awt.Dimension;
import java.awt.event.ActionEvent;
import java.awt.event.ComponentEvent;
import java.awt.event.MouseEvent;
import java.util.Vector;

import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTable;
import javax.swing.ListSelectionModel;
import javax.swing.event.ListSelectionEvent;

import org.tianocore.SourceFilesDocument;
import org.tianocore.FilenameDocument.Filename;
import org.tianocore.ModuleSurfaceAreaDocument.ModuleSurfaceArea;
import org.tianocore.SourceFilesDocument.SourceFiles;
import org.tianocore.frameworkwizard.common.DataType;
import org.tianocore.frameworkwizard.common.IDefaultTableModel;
import org.tianocore.frameworkwizard.common.Log;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.Identifications.OpeningModuleType;
import org.tianocore.frameworkwizard.common.ui.IFrame;
import org.tianocore.frameworkwizard.common.ui.IInternalFrame;
import org.tianocore.frameworkwizard.module.Identifications.SourceFiles.SourceFilesIdentification;
import org.tianocore.frameworkwizard.module.Identifications.SourceFiles.SourceFilesVector;
import org.tianocore.frameworkwizard.module.ui.dialog.SourceFilesDlg;

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

    private JPanel jContentPane = null;

    private JButton jButtonAdd = null;

    private JButton jButtonRemove = null;

    private JButton jButtonUpdate = null;

    private JCheckBox jCheckBoxArch = null;

    private JScrollPane jScrollPane = null;

    private JScrollPane jScrollPaneTable = null;

    private JTable jTable = null;

    //
    // Not used by UI
    //
    private OpeningModuleType omt = null;

    private ModuleSurfaceArea msa = null;

    private SourceFilesVector vSourceFiles = new SourceFilesVector();

    private IDefaultTableModel model = null;

    private int selectedRow = -1;
    
    private IFrame parentFrame = null;

    /**
     This method initializes jButtonAdd 
     
     @return javax.swing.JButton jButtonAdd
     
     **/
    private JButton getJButtonAdd() {
        if (jButtonAdd == null) {
            jButtonAdd = new JButton();
            jButtonAdd.setBounds(new java.awt.Rectangle(230, 220, 90, 20));
            jButtonAdd.setText("Add");
            jButtonAdd.addActionListener(this);
            jButtonAdd.setPreferredSize(new java.awt.Dimension(90, 20));
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
            jButtonRemove.setBounds(new java.awt.Rectangle(400, 220, 90, 20));
            jButtonRemove.setText("Remove");
            jButtonRemove.addActionListener(this);
            jButtonRemove.setPreferredSize(new java.awt.Dimension(90, 20));
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
            jButtonUpdate.setBounds(new java.awt.Rectangle(315, 220, 90, 20));
            jButtonUpdate.setPreferredSize(new java.awt.Dimension(90, 20));
            jButtonUpdate.setText("Edit");
            jButtonUpdate.addActionListener(this);
        }
        return jButtonUpdate;
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
     * This method initializes jScrollPaneTable 
     *  
     * @return javax.swing.JScrollPane  
     */
    private JScrollPane getJScrollPaneTable() {
        if (jScrollPaneTable == null) {
            jScrollPaneTable = new JScrollPane();
            jScrollPaneTable.setBounds(new java.awt.Rectangle(15, 10, 470, 420));
            jScrollPaneTable.setPreferredSize(new Dimension(470, 420));
            jScrollPaneTable.setViewportView(getJTable());
        }
        return jScrollPaneTable;
    }

    /**
     * This method initializes jTable   
     *  
     * @return javax.swing.JTable   
     */
    private JTable getJTable() {
        if (jTable == null) {
            jTable = new JTable();
            model = new IDefaultTableModel();
            jTable = new JTable(model);
            jTable.setRowHeight(20);

            model.addColumn("File Name");
            model.addColumn("Tag Name");
            model.addColumn("Tool Code");
            model.addColumn("Tool Chain Family");

            jTable.setSelectionMode(ListSelectionModel.MULTIPLE_INTERVAL_SELECTION);
            jTable.getSelectionModel().addListSelectionListener(this);
            jTable.getModel().addTableModelListener(this);
            jTable.addMouseListener(this);
        }
        return jTable;
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
    public ModuleSourceFiles(OpeningModuleType inOmt, IFrame iFrame) {
        super();
        this.omt = inOmt;
        this.msa = omt.getXmlMsa();
        this.parentFrame = iFrame;
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
        showTable();
    }

    /**
     This method initializes this
     
     **/
    private void init() {
        this.setSize(500, 515);
        this.setContentPane(getJScrollPane());
        this.setTitle("Source Files");
        this.setViewMode(false);
    }

    /**
     Disable all components when the mode is view
     
     @param isView true - The view mode; false - The non-view mode
     
     **/
    public void setViewMode(boolean isView) {
        if (isView) {
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
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.setPreferredSize(new java.awt.Dimension(490, 490));

            jContentPane.add(getJButtonAdd(), null);
            jContentPane.add(getJButtonRemove(), null);
            jContentPane.add(getJButtonUpdate(), null);
            jContentPane.add(getJScrollPaneTable(), null);
        }
        return jContentPane;
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
            Log.wrn("Update Source Files", e.getMessage());
            Log.err("Update Source Files", e.getMessage());
        }
    }

    private void showEdit(int index) {
        SourceFilesDlg sfd = new SourceFilesDlg(this.vSourceFiles.getSourceFiles(index), this.parentFrame, omt.getId());
        int result = sfd.showDialog();
        if (result == DataType.RETURN_TYPE_OK) {
            if (index == -1) {
                for (int indexI = 0; indexI < sfd.getSfid().length; indexI++) {
                    this.vSourceFiles.addSourceFiles(sfd.getSfid()[indexI]);
                }
            } else {
                this.vSourceFiles.setSourceFiles(sfd.getSfid()[0], index);
            }
            this.showTable();
            this.save();
            sfd.dispose();
        }
        if (result == DataType.RETURN_TYPE_CANCEL) {
            sfd.dispose();
        }
    }

    /* (non-Javadoc)
     * @see java.awt.event.ActionListener#actionPerformed(java.awt.event.ActionEvent)
     *
     * Override actionPerformed to listen all actions
     *  
     */
    public void actionPerformed(ActionEvent arg0) {
        if (arg0.getSource() == jButtonAdd) {
            showEdit(-1);
        }
        if (arg0.getSource() == jButtonUpdate) {
            if (this.selectedRow < 0) {
                Log.wrn("Update Source Files", "Please select one record first.");
                return;
            }
            showEdit(selectedRow);
        }

        if (arg0.getSource() == jButtonRemove) {
            if (jTable.isEditing()) {
                jTable.getCellEditor().stopCellEditing();
            }
            
            int selectedRows[] = this.jTable.getSelectedRows();
            
            if (selectedRows != null) {
                for (int index = selectedRows.length - 1; index > -1; index--) {
                    this.model.removeRow(selectedRows[index]);
                    this.vSourceFiles.removeSourceFiles(selectedRows[index]);
                }
                selectedRow = -1;
                this.save();
            }
        }
    }

    /**
     Clear all table rows
     
     **/
    private void clearAll() {
        if (model != null) {
            for (int index = model.getRowCount() - 1; index >= 0; index--) {
                model.removeRow(index);
            }
        }
    }

    /**
     Read content of vector and put then into table
     
     **/
    private void showTable() {
        clearAll();

        if (vSourceFiles.size() > 0) {
            for (int index = 0; index < vSourceFiles.size(); index++) {
                model.addRow(vSourceFiles.toStringVector(index));
            }
        }
        this.jTable.repaint();
        this.jTable.updateUI();
        //this.jScrollPane.setViewportView(this.jTable);
    }

    /* (non-Javadoc)
     * @see javax.swing.event.ListSelectionListener#valueChanged(javax.swing.event.ListSelectionEvent)
     *
     */
    public void valueChanged(ListSelectionEvent arg0) {
        if (arg0.getValueIsAdjusting()) {
            return;
        }
        ListSelectionModel lsm = (ListSelectionModel) arg0.getSource();
        if (lsm.isSelectionEmpty()) {
            return;
        } else {
            selectedRow = lsm.getMinSelectionIndex();
        }
    }

    /* (non-Javadoc)
     * @see java.awt.event.MouseListener#mouseClicked(java.awt.event.MouseEvent)
     *
     */
    public void mouseClicked(MouseEvent arg0) {
        if (arg0.getClickCount() == 2) {
            if (this.selectedRow < 0) {
                return;
            } else {
                showEdit(selectedRow);
            }
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

        Tools.resizeComponent(this.jScrollPaneTable, intCurrentWidth, intCurrentHeight, intPreferredWidth,
                              intPreferredHeight);
        Tools.relocateComponent(this.jButtonAdd, intCurrentWidth, intCurrentHeight, intPreferredWidth,
                                intPreferredHeight, DataType.SPACE_TO_RIGHT_FOR_ADD_BUTTON,
                                DataType.SPACE_TO_BOTTOM_FOR_ADD_BUTTON);
        Tools.relocateComponent(this.jButtonRemove, intCurrentWidth, intCurrentHeight, intPreferredWidth,
                                intPreferredHeight, DataType.SPACE_TO_RIGHT_FOR_REMOVE_BUTTON,
                                DataType.SPACE_TO_BOTTOM_FOR_REMOVE_BUTTON);
        Tools.relocateComponent(this.jButtonUpdate, intCurrentWidth, intCurrentHeight, intPreferredWidth,
                                intPreferredHeight, DataType.SPACE_TO_RIGHT_FOR_UPDATE_BUTTON,
                                DataType.SPACE_TO_BOTTOM_FOR_UPDATE_BUTTON);
    }
}
