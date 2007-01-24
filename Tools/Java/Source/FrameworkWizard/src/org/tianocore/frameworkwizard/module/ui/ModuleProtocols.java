/** @file
 
 The file is used to create, update Protocol of MSA/MBD file
 
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

import org.tianocore.ProtocolNotifyUsage;
import org.tianocore.ProtocolUsage;
import org.tianocore.ProtocolsDocument;
import org.tianocore.ModuleSurfaceAreaDocument.ModuleSurfaceArea;
import org.tianocore.ProtocolsDocument.Protocols;
import org.tianocore.ProtocolsDocument.Protocols.Protocol;
import org.tianocore.ProtocolsDocument.Protocols.ProtocolNotify;
import org.tianocore.frameworkwizard.common.DataType;
import org.tianocore.frameworkwizard.common.EnumerationData;
import org.tianocore.frameworkwizard.common.IDefaultTableModel;
import org.tianocore.frameworkwizard.common.Log;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.Identifications.OpeningModuleType;
import org.tianocore.frameworkwizard.common.ui.IFrame;
import org.tianocore.frameworkwizard.common.ui.IInternalFrame;
import org.tianocore.frameworkwizard.module.Identifications.Protocols.ProtocolsIdentification;
import org.tianocore.frameworkwizard.module.Identifications.Protocols.ProtocolsVector;
import org.tianocore.frameworkwizard.module.ui.dialog.ProtocolsDlg;

/**
 The class is used to create, update Protocol of MSA/MBD file
 It extends IInternalFrame
 


 **/
public class ModuleProtocols extends IInternalFrame {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = -9084913640747858848L;

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

    private ProtocolsDocument.Protocols protocols = null;

    private ProtocolsIdentification id = null;

    private ProtocolsVector vid = new ProtocolsVector();

    private EnumerationData ed = new EnumerationData();

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
        this.setTitle("Protocols");
    }

    /**
     This method initializes this
     Fill values to all fields if these values are not empty
     
     @param inPackageDependencies

     **/
    private void init(Protocols inProtocols) {
        init();
        this.protocols = inProtocols;

        if (this.protocols != null) {
            if (this.protocols.getProtocolList().size() > 0) {
                for (int index = 0; index < this.protocols.getProtocolList().size(); index++) {
                    String arg0 = protocols.getProtocolList().get(index).getProtocolCName();
                    String arg1 = ed.getVProtocolType().get(0);
                    String arg2 = null;
                    if (protocols.getProtocolList().get(index).getUsage() != null) {
                        arg2 = protocols.getProtocolList().get(index).getUsage().toString();
                    }

                    String arg3 = protocols.getProtocolList().get(index).getFeatureFlag();
                    Vector<String> arg4 = Tools.convertListToVector(protocols.getProtocolList().get(index)
                                                                             .getSupArchList());
                    String arg5 = protocols.getProtocolList().get(index).getHelpText();
                    id = new ProtocolsIdentification(arg0, arg1, arg2, arg3, arg4, arg5);
                    vid.addProtocols(id);
                }
            }
            if (this.protocols.getProtocolNotifyList().size() > 0) {
                for (int index = 0; index < this.protocols.getProtocolNotifyList().size(); index++) {
                    String arg0 = protocols.getProtocolNotifyList().get(index).getProtocolNotifyCName();
                    String arg1 = ed.getVProtocolType().get(1);
                    String arg2 = null;
                    if (protocols.getProtocolNotifyList().get(index).getUsage() != null) {
                        arg2 = protocols.getProtocolNotifyList().get(index).getUsage().toString();
                    }

                    String arg3 = protocols.getProtocolNotifyList().get(index).getFeatureFlag();
                    Vector<String> arg4 = Tools.convertListToVector(protocols.getProtocolNotifyList().get(index)
                                                                             .getSupArchList());
                    String arg5 = protocols.getProtocolNotifyList().get(index).getHelpText();
                    id = new ProtocolsIdentification(arg0, arg1, arg2, arg3, arg4, arg5);
                    vid.addProtocols(id);
                }
            }
        }
        showTable();
    }

    /**
     This is the default constructor
     
     **/
    public ModuleProtocols() {
        super();
        init();
        this.setVisible(true);
    }

    /**
     This is the override edit constructor
     
     @param inProtocol The input data of ProtocolsDocument.Protocols
     
     **/
    public ModuleProtocols(OpeningModuleType inOmt, IFrame iFrame) {
        super();
        this.omt = inOmt;
        this.msa = omt.getXmlMsa();
        this.parentFrame = iFrame;
        init(msa.getProtocols());
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
        ProtocolsDlg dlg = new ProtocolsDlg(vid.getProtocols(index), this.parentFrame, omt.getId());
        int result = dlg.showDialog();
        if (result == DataType.RETURN_TYPE_OK) {
            if (index == -1) {
                this.vid.addProtocols(dlg.getId());
            } else {
                this.vid.setProtocols(dlg.getId(), index);
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
                Log.wrn("Update Protocols", "Please select one record first.");
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
                    this.vid.removeProtocols(selectedRows[index]);
                }
                selectedRow = -1;
                this.save();
            }
        }
    }

    /**
     Save all components of Protocols
     if exists protocols, set the value directly
     if not exists protocols, new an instance first
     
     **/
    public void save() {
        try {
            int count = this.vid.size();

            this.protocols = Protocols.Factory.newInstance();
            if (count > 0) {
                for (int index = 0; index < count; index++) {
                    if (vid.getProtocols(index).getType().equals(ed.getVProtocolType().get(0))) {
                        Protocol p = Protocol.Factory.newInstance();
                        if (!isEmpty(vid.getProtocols(index).getName())) {
                            p.setProtocolCName(vid.getProtocols(index).getName());
                        }
                        if (!isEmpty(vid.getProtocols(index).getUsage())) {
                            p.setUsage(ProtocolUsage.Enum.forString(vid.getProtocols(index).getUsage()));
                        }
                        if (!isEmpty(vid.getProtocols(index).getFeatureFlag())) {
                            p.setFeatureFlag(vid.getProtocols(index).getFeatureFlag());
                        }
                        if (vid.getProtocols(index).getSupArchList() != null
                            && vid.getProtocols(index).getSupArchList().size() > 0) {
                            p.setSupArchList(vid.getProtocols(index).getSupArchList());
                        }
                        if (!isEmpty(vid.getProtocols(index).getHelp())) {
                            p.setHelpText(vid.getProtocols(index).getHelp());
                        }
                        this.protocols.addNewProtocol();
                        this.protocols.setProtocolArray(protocols.getProtocolList().size() - 1, p);
                    }
                    if (vid.getProtocols(index).getType().equals(ed.getVProtocolType().get(1))) {
                        ProtocolNotify p = ProtocolNotify.Factory.newInstance();
                        if (!isEmpty(vid.getProtocols(index).getName())) {
                            p.setProtocolNotifyCName(vid.getProtocols(index).getName());
                        }
                        if (!isEmpty(vid.getProtocols(index).getUsage())) {
                            p.setUsage(ProtocolNotifyUsage.Enum.forString(vid.getProtocols(index).getUsage()));
                        }
                        if (!isEmpty(vid.getProtocols(index).getFeatureFlag())) {
                            p.setFeatureFlag(vid.getProtocols(index).getFeatureFlag());
                        }
                        if (vid.getProtocols(index).getSupArchList() != null
                            && vid.getProtocols(index).getSupArchList().size() > 0) {
                            p.setSupArchList(vid.getProtocols(index).getSupArchList());
                        }
                        if (!isEmpty(vid.getProtocols(index).getHelp())) {
                            p.setHelpText(vid.getProtocols(index).getHelp());
                        }
                        this.protocols.addNewProtocolNotify();
                        this.protocols.setProtocolNotifyArray(protocols.getProtocolNotifyList().size() - 1, p);
                    }
                }
            }

            this.msa.setProtocols(protocols);
            this.omt.setSaved(false);
        } catch (Exception e) {
            Log.wrn("Update Protocols", e.getMessage());
            Log.err("Update Protocols", e.getMessage());
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
