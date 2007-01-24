/** @file
 
 The file is used to create, update Hob of MSA/MBD file
 
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

import org.tianocore.HobTypes;
import org.tianocore.HobUsage;
import org.tianocore.HobsDocument;
import org.tianocore.HobsDocument.Hobs;
import org.tianocore.ModuleSurfaceAreaDocument.ModuleSurfaceArea;
import org.tianocore.frameworkwizard.common.DataType;
import org.tianocore.frameworkwizard.common.IDefaultTableModel;
import org.tianocore.frameworkwizard.common.Log;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.Identifications.OpeningModuleType;
import org.tianocore.frameworkwizard.common.ui.IFrame;
import org.tianocore.frameworkwizard.common.ui.IInternalFrame;
import org.tianocore.frameworkwizard.module.Identifications.Hobs.HobsIdentification;
import org.tianocore.frameworkwizard.module.Identifications.Hobs.HobsVector;
import org.tianocore.frameworkwizard.module.ui.dialog.HobsDlg;

/**
 The class is used to create, update Hob of MSA/MBD file
 It extends IInternalFrame
 


 **/
public class ModuleHobs extends IInternalFrame {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = -553473437579358325L;

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
    // Not used by UI
    //
    private OpeningModuleType omt = null;

    private ModuleSurfaceArea msa = null;

    private HobsDocument.Hobs hobs = null;

    private HobsIdentification id = null;

    private HobsVector vid = new HobsVector();

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

            model.addColumn("Guid C_Name");
            model.addColumn("Type");
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
     This method initializes this
     
     **/
    private void init() {
        this.setSize(500, 515);
        this.setContentPane(getJScrollPane());
        this.setTitle("Hobs");
    }

    /**
     This method initializes this
     Fill values to all fields if these values are not empty
     
     @param inPackageDependencies

     **/
    private void init(Hobs inHobs) {
        init();
        this.hobs = inHobs;

        if (this.hobs != null) {
            if (this.hobs.getHobTypesList().size() > 0) {
                for (int index = 0; index < this.hobs.getHobTypesList().size(); index++) {
                    String arg0 = hobs.getHobTypesList().get(index).getHobGuidCName();
                    String arg1 = null;
                    if (hobs.getHobTypesList().get(index).getHobType() != null) {
                        arg1 = hobs.getHobTypesList().get(index).getHobType().toString();
                    }
                    String arg2 = null;
                    if (hobs.getHobTypesList().get(index).getUsage() != null) {
                        arg2 = hobs.getHobTypesList().get(index).getUsage().toString();
                    }

                    String arg3 = hobs.getHobTypesList().get(index).getFeatureFlag();
                    Vector<String> arg4 = Tools.convertListToVector(hobs.getHobTypesList().get(index).getSupArchList());
                    String arg5 = hobs.getHobTypesList().get(index).getHelpText();
                    id = new HobsIdentification(arg0, arg1, arg2, arg3, arg4, arg5);
                    vid.addHobs(id);
                }
            }
        }
        showTable();
    }

    /**
     This is the default constructor
     
     **/
    public ModuleHobs() {
        super();
        init();
        this.setVisible(true);
    }

    /**
     This is the override edit constructor
     
     @param inHobs The input data of HobsDocument.Hobs
     
     **/
    public ModuleHobs(OpeningModuleType inOmt, IFrame iFrame) {
        super();
        this.omt = inOmt;
        this.msa = omt.getXmlMsa();
        this.parentFrame = iFrame;
        init(msa.getHobs());
        this.setVisible(true);
    }

    /**
     This method initializes jContentPane
     
     @return javax.swing.JPanel jContentPane
     
     **/
    public JPanel getJContentPane() {
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

    private void showEdit(int index) {
        HobsDlg dlg = new HobsDlg(vid.getHobs(index), this.parentFrame, omt.getId());
        int result = dlg.showDialog();
        if (result == DataType.RETURN_TYPE_OK) {
            if (index == -1) {
                this.vid.addHobs(dlg.getId());
            } else {
                this.vid.setHobs(dlg.getId(), index);
            }
            this.showTable();
            this.save();
            dlg.dispose();
        }
        if (result == DataType.RETURN_TYPE_CANCEL) {
            dlg.dispose();
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

        if (vid.size() > 0) {
            for (int index = 0; index < vid.size(); index++) {
                model.addRow(vid.toStringVector(index));
            }
        }
        this.jTable.repaint();
        this.jTable.updateUI();
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
                Log.wrn("Update Hobs", "Please select one record first.");
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
                    this.vid.removeHobs(selectedRows[index]);
                }
                selectedRow = -1;
                this.save();
            }
        }
    }

    /**
     Save all components of Hobs
     if exists hobs, set the value directly
     if not exists hobs, new an instance first
     
     **/
    public void save() {
        try {
            int count = this.vid.size();

            this.hobs = Hobs.Factory.newInstance();
            if (count > 0) {
                for (int index = 0; index < count; index++) {
                    HobsDocument.Hobs.HobTypes p = HobsDocument.Hobs.HobTypes.Factory.newInstance();
                    if (!isEmpty(vid.getHobs(index).getName())) {
                        p.setHobGuidCName(vid.getHobs(index).getName());
                    }
                    if (!isEmpty(vid.getHobs(index).getUsage())) {
                        p.setUsage(HobUsage.Enum.forString(vid.getHobs(index).getUsage()));
                    }
                    if (!isEmpty(vid.getHobs(index).getType())) {
                        p.setHobType(HobTypes.Enum.forString(vid.getHobs(index).getType()));
                    }
                    if (!isEmpty(vid.getHobs(index).getFeatureFlag())) {
                        p.setFeatureFlag(vid.getHobs(index).getFeatureFlag());
                    }
                    if (vid.getHobs(index).getSupArchList() != null && vid.getHobs(index).getSupArchList().size() > 0) {
                        p.setSupArchList(vid.getHobs(index).getSupArchList());
                    }
                    this.hobs.addNewHobTypes();
                    this.hobs.setHobTypesArray(hobs.getHobTypesList().size() - 1, p);
                }
            }

            this.msa.setHobs(hobs);
            this.omt.setSaved(false);
        } catch (Exception e) {
            Log.wrn("Update Hobs", e.getMessage());
            Log.err("Update Hobs", e.getMessage());
        }
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
