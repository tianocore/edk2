/** @file
 
 The file is used to create, update DataHub of MSA/MBD file
 
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
import java.awt.event.ItemListener;
import java.awt.event.MouseEvent;
import java.util.Vector;

import javax.swing.JButton;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTable;
import javax.swing.ListSelectionModel;
import javax.swing.event.ListSelectionEvent;

import org.tianocore.ExternsDocument;
import org.tianocore.PcdDriverTypes;
import org.tianocore.ExternsDocument.Externs;
import org.tianocore.ExternsDocument.Externs.Extern;
import org.tianocore.ModuleSurfaceAreaDocument.ModuleSurfaceArea;
import org.tianocore.frameworkwizard.common.DataType;
import org.tianocore.frameworkwizard.common.EnumerationData;
import org.tianocore.frameworkwizard.common.IDefaultTableModel;
import org.tianocore.frameworkwizard.common.Log;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.Identifications.OpeningModuleType;
import org.tianocore.frameworkwizard.common.ui.IFrame;
import org.tianocore.frameworkwizard.common.ui.IInternalFrame;
import org.tianocore.frameworkwizard.module.Identifications.Externs.ExternsIdentification;
import org.tianocore.frameworkwizard.module.Identifications.Externs.ExternsVector;
import org.tianocore.frameworkwizard.module.ui.dialog.ExternsDlg;

/**
 The class is used to create, update DataHub of MSA/MBD file 
 It extends IInternalFrame
 


 **/
public class ModuleExterns extends IInternalFrame implements ItemListener {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = -7382008402932047191L;

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

    private ExternsDocument.Externs externs = null;

    private ExternsIdentification id = null;

    private ExternsVector vid = new ExternsVector();

    private IDefaultTableModel model = null;

    private int selectedRow = -1;

    /**
     This method initializes jButtonAdd 
     
     @return javax.swing.JButton jButtonAdd
     
     **/
    private JButton getJButtonAdd() {
        if (jButtonAdd == null) {
            jButtonAdd = new JButton();
            jButtonAdd.setBounds(new java.awt.Rectangle(230, 195, 80, 20));
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
            jButtonRemove.setBounds(new java.awt.Rectangle(400, 195, 80, 20));
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
            jButtonUpdate.setBounds(new java.awt.Rectangle(315, 195, 80, 20));
            jButtonUpdate.setPreferredSize(new java.awt.Dimension(80, 20));
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

            model.addColumn("Name");
            model.addColumn("Type");

            jTable.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
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
        this.setTitle("Externs");
    }

    /**
     This method initializes this
     Fill values to all fields if these values are not empty
     
     @param inPackageDependencies

     **/
    private void init(Externs inExterns) {
        init();
        this.externs = inExterns;

        if (this.externs != null) {
            //
            // Get PcdIsDriver
            //
            if (this.externs.getPcdIsDriver() != null) {
                String arg0 = this.externs.getPcdIsDriver().toString();
                String arg1 = EnumerationData.EXTERNS_PCD_IS_DRIVER;

                id = new ExternsIdentification(arg0, arg1, null, null);
                vid.addExterns(id);
            }

            //
            // Get specification
            //
            if (this.externs.getSpecificationList().size() > 0) {
                for (int index = 0; index < this.externs.getSpecificationList().size(); index++) {
                    String arg0 = externs.getSpecificationList().get(index);
                    String arg1 = EnumerationData.EXTERNS_SPECIFICATION;

                    id = new ExternsIdentification(arg0, arg1, null, null);
                    vid.addExterns(id);
                }
            }

            //
            // Get Externs list
            //
            if (this.externs.getExternList().size() > 0) {
                for (int index = 0; index < this.externs.getExternList().size(); index++) {
                    String arg0 = null;
                    String arg1 = null;
                    if (this.externs.getExternList().get(index).getModuleEntryPoint() != null) {
                        arg0 = this.externs.getExternList().get(index).getModuleEntryPoint();
                        arg1 = EnumerationData.EXTERNS_MODULE_ENTRY_POINT;
                    }
                    if (this.externs.getExternList().get(index).getModuleUnloadImage() != null) {
                        arg0 = this.externs.getExternList().get(index).getModuleUnloadImage();
                        arg1 = EnumerationData.EXTERNS_MODULE_UNLOAD_IMAGE;
                    }

                    if (this.externs.getExternList().get(index).getConstructor() != null) {
                        arg0 = this.externs.getExternList().get(index).getConstructor();
                        arg1 = EnumerationData.EXTERNS_CONSTRUCTOR;
                    }
                    if (this.externs.getExternList().get(index).getDestructor() != null) {
                        arg0 = this.externs.getExternList().get(index).getDestructor();
                        arg1 = EnumerationData.EXTERNS_DESTRUCTOR;
                    }

                    if (this.externs.getExternList().get(index).getDriverBinding() != null) {
                        arg0 = this.externs.getExternList().get(index).getDriverBinding();
                        arg1 = EnumerationData.EXTERNS_DRIVER_BINDING;
                    }
                    if (this.externs.getExternList().get(index).getComponentName() != null) {
                        arg0 = this.externs.getExternList().get(index).getComponentName();
                        arg1 = EnumerationData.EXTERNS_COMPONENT_NAME;
                    }
                    if (this.externs.getExternList().get(index).getDriverConfig() != null) {
                        arg0 = this.externs.getExternList().get(index).getDriverConfig();
                        arg1 = EnumerationData.EXTERNS_DRIVER_CONFIG;
                    }
                    if (this.externs.getExternList().get(index).getDriverDiag() != null) {
                        arg0 = this.externs.getExternList().get(index).getDriverDiag();
                        arg1 = EnumerationData.EXTERNS_DRIVER_DIAG;
                    }

                    if (this.externs.getExternList().get(index).getSetVirtualAddressMapCallBack() != null) {
                        arg0 = this.externs.getExternList().get(index).getSetVirtualAddressMapCallBack();
                        arg1 = EnumerationData.EXTERNS_SET_VIRTUAL_ADDRESS_MAP_CALL_BACK;
                    }
                    if (this.externs.getExternList().get(index).getExitBootServicesCallBack() != null) {
                        arg0 = this.externs.getExternList().get(index).getExitBootServicesCallBack();
                        arg1 = EnumerationData.EXTERNS_EXIT_BOOT_SERVICES_CALL_BACK;
                    }

                    String arg2 = externs.getExternList().get(index).getFeatureFlag();
                    Vector<String> arg3 = Tools
                                               .convertListToVector(externs.getExternList().get(index).getSupArchList());

                    id = new ExternsIdentification(arg0, arg1, arg2, arg3);
                    vid.addExterns(id);
                }
            }
        }
        showTable();
    }

    /**
     This is the default constructor
     
     **/
    public ModuleExterns() {
        super();
        init();
        this.setVisible(true);
    }

    /**
     This is the override edit constructor
     
     @param inExterns The input data of ExternsDocument.Externs
     
     **/
    public ModuleExterns(OpeningModuleType inOmt) {
        super();
        this.omt = inOmt;
        this.msa = omt.getXmlMsa();
        init(msa.getExterns());
        this.setVisible(true);
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

    private void showEdit(int index) {
        ExternsDlg dlg = new ExternsDlg(vid.getExterns(index), new IFrame());
        int result = dlg.showDialog();
        if (result == DataType.RETURN_TYPE_OK) {
            if (index == -1) {
                this.vid.addExterns(dlg.getId());
            } else {
                this.vid.setExterns(dlg.getId(), index);
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
                Log.err("Please select one record first.");
                return;
            }
            showEdit(selectedRow);
        }

        if (arg0.getSource() == jButtonRemove) {
            if (jTable.isEditing()) {
                jTable.getCellEditor().stopCellEditing();
            }
            if (selectedRow > -1) {
                this.model.removeRow(selectedRow);
                this.vid.removeExterns(selectedRow);
                selectedRow = -1;
                this.save();
            }
        }
    }

    /**
     Save all components of Externs
     if exists externs, set the value directly
     if not exists externs, new an instance first
     
     **/
    public void save() {
        try {
            int count = this.vid.size();

            this.externs = Externs.Factory.newInstance();
            //            //
            //            // Save PcdIsDriver first
            //            //
            //            if (!this.jComboBoxPcdIsDriver.getSelectedItem().toString().equals(DataType.EMPTY_SELECT_ITEM)) {
            //                externs.setPcdIsDriver(PcdDriverTypes.Enum.forString(this.jComboBoxPcdIsDriver.getSelectedItem()
            //                                                                                              .toString()));
            //            }

            if (count > 0) {
                for (int index = 0; index < count; index++) {
                    //
                    // Save Pcd Is Driver
                    //
                    if (vid.getExterns(index).getType().equals(EnumerationData.EXTERNS_PCD_IS_DRIVER)) {
                        externs.setPcdIsDriver(PcdDriverTypes.Enum.forString(vid.getExterns(index).getName()));
                        continue;
                    }

                    //
                    // Save specfication
                    //
                    if (vid.getExterns(index).getType().equals(EnumerationData.EXTERNS_SPECIFICATION)) {
                        if (!isEmpty(vid.getExterns(index).getName())) {
                            this.externs.addNewSpecification();
                            this.externs.setSpecificationArray(externs.getSpecificationList().size() - 1,
                                                               vid.getExterns(index).getName());
                        }
                    } else {
                        //
                        // Save extern
                        //
                        Extern e = Extern.Factory.newInstance();

                        if (vid.getExterns(index).getType().equals(EnumerationData.EXTERNS_MODULE_ENTRY_POINT)) {
                            if (!isEmpty(vid.getExterns(index).getName())) {
                                e.setModuleEntryPoint(vid.getExterns(index).getName());
                            }
                        }
                        if (vid.getExterns(index).getType().equals(EnumerationData.EXTERNS_MODULE_UNLOAD_IMAGE)) {
                            if (!isEmpty(vid.getExterns(index).getName())) {
                                e.setModuleUnloadImage(vid.getExterns(index).getName());
                            }
                        }

                        if (vid.getExterns(index).getType().equals(EnumerationData.EXTERNS_CONSTRUCTOR)) {
                            if (!isEmpty(vid.getExterns(index).getName())) {
                                e.setConstructor(vid.getExterns(index).getName());
                            }
                        }
                        if (vid.getExterns(index).getType().equals(EnumerationData.EXTERNS_DESTRUCTOR)) {
                            if (!isEmpty(vid.getExterns(index).getName())) {
                                e.setDestructor(vid.getExterns(index).getName());
                            }
                        }

                        if (vid.getExterns(index).getType().equals(EnumerationData.EXTERNS_DRIVER_BINDING)) {
                            if (!isEmpty(vid.getExterns(index).getName())) {
                                e.setDriverBinding(vid.getExterns(index).getName());
                            }
                        }
                        if (vid.getExterns(index).getType().equals(EnumerationData.EXTERNS_COMPONENT_NAME)) {
                            if (!isEmpty(vid.getExterns(index).getName())) {
                                e.setComponentName(vid.getExterns(index).getName());
                            }
                        }
                        if (vid.getExterns(index).getType().equals(EnumerationData.EXTERNS_DRIVER_CONFIG)) {
                            if (!isEmpty(vid.getExterns(index).getName())) {
                                e.setDriverConfig(vid.getExterns(index).getName());
                            }
                        }
                        if (vid.getExterns(index).getType().equals(EnumerationData.EXTERNS_DRIVER_DIAG)) {
                            if (!isEmpty(vid.getExterns(index).getName())) {
                                e.setDriverDiag(vid.getExterns(index).getName());
                            }
                        }

                        if (vid.getExterns(index).getType()
                               .equals(EnumerationData.EXTERNS_SET_VIRTUAL_ADDRESS_MAP_CALL_BACK)) {
                            if (!isEmpty(vid.getExterns(index).getName())) {
                                e.setSetVirtualAddressMapCallBack(vid.getExterns(index).getName());
                            }
                        }
                        if (vid.getExterns(index).getType()
                               .equals(EnumerationData.EXTERNS_EXIT_BOOT_SERVICES_CALL_BACK)) {
                            if (!isEmpty(vid.getExterns(index).getName())) {
                                e.setExitBootServicesCallBack(vid.getExterns(index).getName());
                            }
                        }

                        if (!isEmpty(vid.getExterns(index).getFeatureFlag())) {
                            e.setFeatureFlag(vid.getExterns(index).getFeatureFlag());
                        }
                        if (vid.getExterns(index).getSupArchList() != null
                            && vid.getExterns(index).getSupArchList().size() > 0) {
                            e.setSupArchList(vid.getExterns(index).getSupArchList());
                        }

                        this.externs.addNewExtern();
                        this.externs.setExternArray(this.externs.getExternList().size() - 1, e);
                    }
                }
            }

            this.msa.setExterns(externs);
            this.omt.setSaved(false);
        } catch (Exception e) {
            Log.err("Update Externs", e.getMessage());
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

        resizeComponent(this.jScrollPaneTable, intCurrentWidth, intCurrentHeight, intPreferredWidth, intPreferredHeight);
        relocateComponent(this.jButtonAdd, intCurrentWidth, intCurrentHeight, intPreferredWidth, intPreferredHeight,
                          DataType.SPACE_TO_RIGHT_FOR_ADD_BUTTON, DataType.SPACE_TO_BOTTOM_FOR_ADD_BUTTON);
        relocateComponent(this.jButtonRemove, intCurrentWidth, intCurrentHeight, intPreferredWidth, intPreferredHeight,
                          DataType.SPACE_TO_RIGHT_FOR_REMOVE_BUTTON, DataType.SPACE_TO_BOTTOM_FOR_REMOVE_BUTTON);
        relocateComponent(this.jButtonUpdate, intCurrentWidth, intCurrentHeight, intPreferredWidth, intPreferredHeight,
                          DataType.SPACE_TO_RIGHT_FOR_UPDATE_BUTTON, DataType.SPACE_TO_BOTTOM_FOR_UPDATE_BUTTON);
    }
}
