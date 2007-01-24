/** @file
 
 The file is used to create, update Library Class Definition of MSA/MBD file
 
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
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTable;
import javax.swing.ListSelectionModel;
import javax.swing.event.ListSelectionEvent;

import org.tianocore.LibraryClassDefinitionsDocument;
import org.tianocore.LibraryUsage;
import org.tianocore.ModuleSurfaceAreaDocument;
import org.tianocore.LibraryClassDefinitionsDocument.LibraryClassDefinitions;
import org.tianocore.LibraryClassDocument.LibraryClass;
import org.tianocore.frameworkwizard.common.DataType;
import org.tianocore.frameworkwizard.common.IDefaultTableModel;
import org.tianocore.frameworkwizard.common.Log;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.Identifications.OpeningModuleType;
import org.tianocore.frameworkwizard.common.ui.IFrame;
import org.tianocore.frameworkwizard.common.ui.IInternalFrame;
import org.tianocore.frameworkwizard.module.Identifications.LibraryClass.LibraryClassIdentification;
import org.tianocore.frameworkwizard.module.Identifications.LibraryClass.LibraryClassVector;
import org.tianocore.frameworkwizard.module.ui.dialog.LibraryClassDefsDlg;

/**
 The class is used to create, update Library Class Definition of MSA/MBD file
 It extends IInternalFrame
 
 **/
public class ModuleLibraryClassDefinitions extends IInternalFrame {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = -1743248695411382857L;

    //
    //Define class members
    //
    private JPanel jContentPane = null;

    private JButton jButtonAdd = null;

    private JButton jButtonRemove = null;

    private JButton jButtonUpdate = null;

    private JScrollPane jScrollPane = null;

    private JScrollPane jScrollPaneTable = null;

    private JTable jTable = null;

    //
    // Not for UI
    //
    private ModuleSurfaceAreaDocument.ModuleSurfaceArea msa = null;

    private LibraryClassDefinitions lcd = null;

    private LibraryClassVector vLibraryClass = new LibraryClassVector();

    private OpeningModuleType omt = null;

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
            jButtonAdd.setBounds(new java.awt.Rectangle(230, 330, 90, 20));
            jButtonAdd.setPreferredSize(new Dimension(90, 20));
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
            jButtonRemove.setBounds(new java.awt.Rectangle(400, 330, 90, 20));
            jButtonRemove.setText("Remove");
            jButtonRemove.setPreferredSize(new Dimension(90, 20));
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
            jButtonUpdate.setBounds(new java.awt.Rectangle(315, 330, 90, 20));
            jButtonUpdate.setText("Edit");
            jButtonUpdate.setPreferredSize(new Dimension(90, 20));
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
     This method initializes jScrollPaneTable	
     
     @return javax.swing.JScrollPane	
     **/
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
     This method initializes jTable	
     
     @return javax.swing.JTable	
     **/
    private JTable getJTable() {
        if (jTable == null) {
            jTable = new JTable();
            model = new IDefaultTableModel();
            jTable = new JTable(model);
            jTable.setRowHeight(20);

            model.addColumn("Library Class Name");
            model.addColumn("Usage");

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
    public ModuleLibraryClassDefinitions() {
        super();
        init();
        this.setVisible(true);
    }

    /**
     This is the override edit constructor
     
     @param 
     
     **/
    public ModuleLibraryClassDefinitions(OpeningModuleType inOmt, IFrame iFrame) {
        super();
        this.omt = inOmt;
        this.msa = omt.getXmlMsa();
        this.parentFrame = iFrame;
        init(msa.getLibraryClassDefinitions());
        this.setVisible(true);
    }

    /**
     This method initializes this
     Fill values to all fields if these values are not empty
     
     @param inLibraryClassDefinitions The input data of LibraryClassDefinitionsDocument.LibraryClassDefinitions
     
     **/
    private void init(LibraryClassDefinitionsDocument.LibraryClassDefinitions inLibraryClassDefinitions) {
        init();
        this.lcd = inLibraryClassDefinitions;
        if (this.lcd != null) {
            if (this.lcd.getLibraryClassList().size() > 0) {
                for (int index = 0; index < this.lcd.getLibraryClassList().size(); index++) {
                    String name = lcd.getLibraryClassList().get(index).getKeyword();
                    String usage = null;
                    if (lcd.getLibraryClassList().get(index).getUsage() != null) {
                        usage = lcd.getLibraryClassList().get(index).getUsage().toString();
                    }
                    String version = lcd.getLibraryClassList().get(index).getRecommendedInstanceVersion();
                    String guid = lcd.getLibraryClassList().get(index).getRecommendedInstanceGuid();
                    String featureFlag = lcd.getLibraryClassList().get(index).getFeatureFlag();
                    Vector<String> arch = Tools.convertListToVector(lcd.getLibraryClassList().get(index)
                                                                       .getSupArchList());
                    Vector<String> module = Tools.convertListToVector(lcd.getLibraryClassList().get(index)
                                                                         .getSupModuleList());
                    String help = lcd.getLibraryClassList().get(index).getHelpText();
                    LibraryClassIdentification lcid = new LibraryClassIdentification(name, usage, version, guid, arch,
                                                                                     featureFlag, module, help);
                    vLibraryClass.addLibraryClass(lcid);
                }
            }
        }
        //
        // Update the list
        //
        showTable();
    }

    /**
     This method initializes this
     
     **/
    private void init() {
        this.setContentPane(getJScrollPane());
        this.setTitle("Library Class Definitions");
        this.setBounds(new java.awt.Rectangle(0, 0, 500, 515));
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
     Save all components of Mbd Header
     if exists mbdHeader, set the value directly
     if not exists mbdHeader, new an instance first
     
     **/
    public void save() {
        try {
            int intLibraryCount = this.vLibraryClass.size();

            lcd = LibraryClassDefinitions.Factory.newInstance();
            if (intLibraryCount > 0) {
                for (int index = 0; index < intLibraryCount; index++) {
                    LibraryClass mLibraryClass = LibraryClass.Factory.newInstance();

                    mLibraryClass.setKeyword(vLibraryClass.getLibraryClass(index).getLibraryClassName());
                    mLibraryClass
                                 .setUsage(LibraryUsage.Enum.forString(vLibraryClass.getLibraryClass(index).getUsage()));
                    if (!isEmpty(vLibraryClass.getLibraryClass(index).getRecommendedInstanceVersion())) {
                        mLibraryClass.setRecommendedInstanceVersion(vLibraryClass.getLibraryClass(index)
                                                                                 .getRecommendedInstanceVersion());
                    }
                    if (!isEmpty(vLibraryClass.getLibraryClass(index).getRecommendedInstanceGuid())) {
                        mLibraryClass.setRecommendedInstanceGuid(vLibraryClass.getLibraryClass(index)
                                                                              .getRecommendedInstanceGuid());
                    }
                    if (!isEmpty(vLibraryClass.getLibraryClass(index).getFeatureFlag())) {
                        mLibraryClass.setFeatureFlag(vLibraryClass.getLibraryClass(index).getFeatureFlag());
                    }
                    if (vLibraryClass.getLibraryClass(index).getSupArchList() != null
                        && vLibraryClass.getLibraryClass(index).getSupArchList().size() > 0) {
                        mLibraryClass.setSupArchList(vLibraryClass.getLibraryClass(index).getSupArchList());
                    }
                    if (vLibraryClass.getLibraryClass(index).getSupModuleList() != null
                                    && vLibraryClass.getLibraryClass(index).getSupModuleList().size() > 0) {
                                    mLibraryClass.setSupModuleList(vLibraryClass.getLibraryClass(index).getSupModuleList());
                                }
                    if (!isEmpty(vLibraryClass.getLibraryClass(index).getHelp())) {
                        mLibraryClass.setHelpText(vLibraryClass.getLibraryClass(index).getHelp());
                    }

                    this.lcd.addNewLibraryClass();
                    this.lcd.setLibraryClassArray(index, mLibraryClass);
                }
            }

            if (msa.getLibraryClassDefinitions() == null) {
                this.msa.addNewLibraryClassDefinitions();
            }
            this.msa.setLibraryClassDefinitions(this.lcd);

            this.omt.setSaved(false);
        } catch (Exception e) {
            Log.wrn("Update Library Class Definitions", e.getMessage());
            Log.err("Update Library Class Definitions", e.getMessage());
        }
    }

    private void showEdit(int index) {
        LibraryClassDefsDlg mcdd = new LibraryClassDefsDlg(vLibraryClass.getLibraryClass(index), this.parentFrame, omt.getId());
        int result = mcdd.showDialog();
        if (result == DataType.RETURN_TYPE_OK) {
            if (index == -1) {
                this.vLibraryClass.addLibraryClass(mcdd.getLcid());
            } else {
                this.vLibraryClass.setLibraryClass(mcdd.getLcid(), index);
            }
            this.showTable();
            this.save();
            mcdd.dispose();
        }
        if (result == DataType.RETURN_TYPE_CANCEL) {
            mcdd.dispose();
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
                Log.wrn("Update Library Class Definitions", "Please select one record first.");
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
                    this.vLibraryClass.removeLibraryClass(selectedRows[index]);
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

        if (vLibraryClass.size() > 0) {
            for (int index = 0; index < vLibraryClass.size(); index++) {
                model.addRow(vLibraryClass.toStringVector(index));
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
