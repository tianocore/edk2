/** @file
 
 The file is used to create, update BuildOptions of Fpd file
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/
package org.tianocore.frameworkwizard.platform.ui;

import java.awt.BorderLayout;

import javax.swing.JPanel;
import javax.swing.JTabbedPane;
import javax.swing.JCheckBox;
import javax.swing.JLabel;
import java.awt.FlowLayout;
import javax.swing.AbstractAction;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.DefaultCellEditor;
import javax.swing.JFileChooser;
import javax.swing.JFrame;
import javax.swing.JOptionPane;
import javax.swing.JTextField;
import javax.swing.JButton;
import javax.swing.JScrollPane;
import javax.swing.JTable;
import javax.swing.JComboBox;
import javax.swing.ListSelectionModel;
import javax.swing.event.InternalFrameAdapter;
import javax.swing.event.InternalFrameEvent;
import javax.swing.event.ListSelectionEvent;
import javax.swing.event.ListSelectionListener;
import javax.swing.event.TableModelEvent;
import javax.swing.event.TableModelListener;
import javax.swing.table.DefaultTableModel;
import javax.swing.table.TableModel;

import org.tianocore.PlatformSurfaceAreaDocument;
import org.tianocore.frameworkwizard.common.DataValidation;
import org.tianocore.frameworkwizard.common.Identifications.OpeningPlatformType;
import org.tianocore.frameworkwizard.common.ui.IInternalFrame;

import java.io.File;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.Set;
import java.util.Vector;
import java.awt.Dimension;

public class FpdBuildOptions extends IInternalFrame {

    private static final long serialVersionUID = 1L;
    static JFrame frame;
    private JPanel jContentPane = null;
    private JPanel jPanelContentSouth = null;
    private JPanel jPanelContentNorth = null;
    private JPanel jPanelContentWest = null;
    private JPanel jPanelContentEast = null;
    private JTabbedPane jTabbedPane = null;
    private JPanel jPanelUserDef = null;
    private JPanel jPanelUserDefNorth = null;
    private JPanel jPanelUserDefCenter = null;
    private JTextField jTextFieldAntTaskFile = null;
    private JLabel jLabelAntTaskId = null;
    private JTextField jTextFieldAntTaskId = null;
    private JButton jButtonAntTaskAdd = null;
    private JButton jButtonAntTaskDel = null;
    private JTextField jTextFieldAntCmdOpts = null;
    private JScrollPane jScrollPaneAntTasks = null;
    private JTable jTableAntTasks = null;
    private DefaultTableModel ffsTableModel = null;
    private DefaultTableModel sectionsTableModel = null;
    private DefaultTableModel sectionTableModel = null;
    private DefaultTableModel subsectionsTableModel = null;
    private DefaultTableModel antTaskTableModel = null;
    private DefaultTableModel ffsAttributesTableModel = null;
    private DefaultTableModel optionsTableModel = null;
    private JPanel jPanelFfsTab = null;
    private JPanel jPanelFfsTabCenter = null;
    private JPanel jPanelFfsTabCenterN = null;
    private JPanel jPanelFfsTabCenterS = null;
    private JPanel jPanelFfsTabCenterE = null;
    private JLabel jLabelFfsKey = null;
    private JTextField jTextFieldFfsKey = null;
    private JButton jButtonFfsAdd = null;
    private JButton jButtonFfsDel = null;
    private JScrollPane jScrollPaneFfsAttribs = null;
    private JTable jTableFfsAttribs = null;
    private JPanel jPanelFfsTabCenterC = null;
    private JPanel jPanelOptionsTab = null;
    private JLabel jLabelOptionContents = null;
    private JTextField jTextFieldOptionContents = null;
    private JLabel jLabelToolChainFamily = null;
    private JLabel jLabelSupArch = null;
    private JLabel jLabelToolCmd = null;
    private JTextField jTextFieldToolCmd = null;
    private JScrollPane jScrollPaneOptions = null;
    private JTable jTableOptions = null;
    private JButton jButtonOptionsAdd = null;
    private JButton jButtonOptionsDel = null;
    private JButton jButtonFfsAttribNew = null;
    private JButton jButtonFfsAttribRemove = null;
    private FpdFileContents ffc = null;
    private OpeningPlatformType docConsole = null;
    private JCheckBox jCheckBoxIA32 = null;
    private JCheckBox jCheckBoxIpf = null;
    private JCheckBox jCheckBoxX64 = null;
    private JCheckBox jCheckBoxEBC = null;
    private JCheckBox jCheckBoxARM = null;
    private JCheckBox jCheckBoxPPC = null;
    private JLabel jLabelBuildTargets = null;
    private JTextField jTextFieldBuildTargets = null;
    private JTextField jTextFieldTagName = null;
    private JLabel jLabelTagName = null;
    private int selectedRow = -1;
    private JLabel jLabelAntTaskFile = null;
    private JLabel jLabelAntCmdOpts = null;
    private JScrollPane jScrollPaneFfs = null;
    private JTable jTableFfs = null;
    private JLabel jLabelFfsSection = null;
    private JScrollPane jScrollPaneFfsSection = null;
    private JTable jTableFfsSection = null;
    private JLabel jLabelFfsSubSections = null;
    private JScrollPane jScrollPaneFfsSubSection = null;
    private JTable jTableFfsSubSection = null;
    private JLabel jLabelEncapType = null;
    private JTextField jTextFieldEncapType = null;
    private JPanel jPanelFfsAttribButtonGroup = null;
    private JLabel jLabelFfsAttribs = null;
    private JButton jButtonFfsSectionNew = null;
    private JButton jButtonFfsSectionRemove = null;
    private JButton jButtonFfsSubSectionNew = null;
    private JButton jButtonFfsSubSectionRemove = null;
    private JLabel jLabelFfsSections = null;
    private JButton jButtonFfsSectionsNew = null;
    private JButton jButtonFfsSectionsRemove = null;
    private JScrollPane jScrollPaneFfsSections = null;
    private JTable jTableFfsSections = null;
    private JButton jButtonAntTaskFileBrowse = null;
    private JTextField jTextFieldToolChainFamily = null;
    /**
     * This method initializes jPanel	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelContentSouth() {
        if (jPanelContentSouth == null) {
            jPanelContentSouth = new JPanel();
        }
        return jPanelContentSouth;
    }

    /**
     * This method initializes jPanel1	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelContentNorth() {
        if (jPanelContentNorth == null) {
            jPanelContentNorth = new JPanel();
        }
        return jPanelContentNorth;
    }

    /**
     * This method initializes jPanel2	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelContentWest() {
        if (jPanelContentWest == null) {
            jPanelContentWest = new JPanel();
        }
        return jPanelContentWest;
    }

    /**
     * This method initializes jPanel3	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelContentEast() {
        if (jPanelContentEast == null) {
            jPanelContentEast = new JPanel();
        }
        return jPanelContentEast;
    }

    /**
     * This method initializes jTabbedPane	
     * 	
     * @return javax.swing.JTabbedPane	
     */
    private JTabbedPane getJTabbedPane() {
        if (jTabbedPane == null) {
            jTabbedPane = new JTabbedPane();
            jTabbedPane.addTab("FFS", null, getJPanelFfsTab(), null);
            jTabbedPane.addTab("Options", null, getJPanelOptionsTab(), null);
            jTabbedPane.addTab("User Defined ANT Tasks", null, getJPanelUserDef(), null);
        }
        return jTabbedPane;
    }

    
    /**
     * This method initializes jPanel8	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelUserDef() {
        if (jPanelUserDef == null) {
            jPanelUserDef = new JPanel();
            jPanelUserDef.setLayout(new BorderLayout());
            jPanelUserDef.add(getJPanelUserDefNorth(), java.awt.BorderLayout.NORTH);
            jPanelUserDef.add(getJPanelUserDefCenter(), java.awt.BorderLayout.CENTER);

        }
        return jPanelUserDef;
    }

    /**
     * This method initializes jPanel9	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelUserDefNorth() {
        if (jPanelUserDefNorth == null) {
            jLabelAntTaskFile = new JLabel();
            jLabelAntTaskFile.setText("ANT Task File");
            jLabelAntTaskFile.setPreferredSize(new java.awt.Dimension(80,20));
            FlowLayout flowLayout8 = new FlowLayout();
            flowLayout8.setAlignment(java.awt.FlowLayout.LEFT);
            jLabelAntTaskId = new JLabel();
            jLabelAntTaskId.setText("ID");
            jPanelUserDefNorth = new JPanel();
            jPanelUserDefNorth.setLayout(flowLayout8);
            jPanelUserDefNorth.add(jLabelAntTaskFile, null);
            jPanelUserDefNorth.add(getJTextFieldAntTaskFile(), null);
            jPanelUserDefNorth.add(getJButtonAntTaskFileBrowse(), null);
            jPanelUserDefNorth.add(jLabelAntTaskId, null);
            jPanelUserDefNorth.add(getJTextFieldAntTaskId(), null);
        }
        return jPanelUserDefNorth;
    }

    /**
     * This method initializes jPanel11	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelUserDefCenter() {
        if (jPanelUserDefCenter == null) {
            jLabelAntCmdOpts = new JLabel();
            jLabelAntCmdOpts.setText("ANT Command Options");
            jLabelAntCmdOpts.setPreferredSize(new java.awt.Dimension(131,20));
            FlowLayout flowLayout3 = new FlowLayout();
            flowLayout3.setHgap(5);
            flowLayout3.setAlignment(java.awt.FlowLayout.LEFT);
            jPanelUserDefCenter = new JPanel();
            jPanelUserDefCenter.setLayout(flowLayout3);
            jPanelUserDefCenter.add(jLabelAntCmdOpts, null);
            jPanelUserDefCenter.add(getJTextField4(), null);
            jPanelUserDefCenter.add(getJButtonAntTaskAdd(), null);
            jPanelUserDefCenter.add(getJButtonAntTaskDel(), null);
            jPanelUserDefCenter.add(getJScrollPaneAntTasks(), null);
        }
        return jPanelUserDefCenter;
    }

    /**
     * This method initializes jTextField2	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldAntTaskFile() {
        if (jTextFieldAntTaskFile == null) {
            jTextFieldAntTaskFile = new JTextField();
            jTextFieldAntTaskFile.setPreferredSize(new java.awt.Dimension(200,20));
        }
        return jTextFieldAntTaskFile;
    }

    /**
     * This method initializes jTextField3	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldAntTaskId() {
        if (jTextFieldAntTaskId == null) {
            jTextFieldAntTaskId = new JTextField();
            jTextFieldAntTaskId.setPreferredSize(new java.awt.Dimension(100,20));
        }
        return jTextFieldAntTaskId;
    }

    /**
     * This method initializes jButton4	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonAntTaskAdd() {
        if (jButtonAntTaskAdd == null) {
            jButtonAntTaskAdd = new JButton();
            jButtonAntTaskAdd.setPreferredSize(new java.awt.Dimension(90,20));
            jButtonAntTaskAdd.setText("Add");
            jButtonAntTaskAdd.addActionListener(new ActionListener() {
                public void actionPerformed(ActionEvent e) {
                    if (!DataValidation.isInt(jTextFieldAntTaskId.getText()) || jTextFieldAntTaskId.getText().length() != 8) {
                        JOptionPane.showMessageDialog(frame, "ID must be an 8-digit integer.");
                        return;
                    }
                    Object[] o = {jTextFieldAntTaskId.getText(), null, null};
                        o[1] = jTextFieldAntTaskFile.getText();
                        o[2] = jTextFieldAntCmdOpts.getText();
                        ffc.genBuildOptionsUserDefAntTask(o[0]+"", o[1]+"", o[2]+"");
                    antTaskTableModel.addRow(o);
                    docConsole.setSaved(false);
                }
            });
        }
        return jButtonAntTaskAdd;
    }

    /**
     * This method initializes jButton5	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonAntTaskDel() {
        if (jButtonAntTaskDel == null) {
            jButtonAntTaskDel = new JButton();
            jButtonAntTaskDel.setPreferredSize(new java.awt.Dimension(90,20));
            jButtonAntTaskDel.setText("Delete");
            jButtonAntTaskDel.addActionListener(new ActionListener() {
                public void actionPerformed(ActionEvent e) {
                    if (selectedRow >= 0) {
                        docConsole.setSaved(false);
                        antTaskTableModel.removeRow(selectedRow);
                        ffc.removeBuildOptionsUserDefAntTask(selectedRow);
                    }
                }
            });
        }
        return jButtonAntTaskDel;
    }

    /**
     * This method initializes jTextField4	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextField4() {
        if (jTextFieldAntCmdOpts == null) {
            jTextFieldAntCmdOpts = new JTextField();
            jTextFieldAntCmdOpts.setPreferredSize(new java.awt.Dimension(270,20));
            jTextFieldAntCmdOpts.setEnabled(true);
        }
        return jTextFieldAntCmdOpts;
    }

    /**
     * This method initializes jScrollPane2	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPaneAntTasks() {
        if (jScrollPaneAntTasks == null) {
            jScrollPaneAntTasks = new JScrollPane();
            jScrollPaneAntTasks.setPreferredSize(new java.awt.Dimension(600,100));
            jScrollPaneAntTasks.setViewportView(getJTableAntTasks());
        }
        return jScrollPaneAntTasks;
    }

    /**
     * This method initializes jTable2	
     * 	
     * @return javax.swing.JTable	
     */
    private JTable getJTableAntTasks() {
        if (jTableAntTasks == null) {
            antTaskTableModel = new DefaultTableModel();
            jTableAntTasks = new JTable(antTaskTableModel);
            jTableAntTasks.setRowHeight(20);
            antTaskTableModel.addColumn("ID");
            antTaskTableModel.addColumn("Filename");
            antTaskTableModel.addColumn("ANT Command Options");
            
            jTableAntTasks.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
            jTableAntTasks.getSelectionModel().addListSelectionListener(new ListSelectionListener(){
                public void valueChanged(ListSelectionEvent e) {
                    selectedRow = -1;
                    if (e.getValueIsAdjusting()){
                        return;
                    }
                    ListSelectionModel lsm = (ListSelectionModel)e.getSource();
                    if (lsm.isSelectionEmpty()) {
                        return;
                    }
                    else{
                        selectedRow = lsm.getMinSelectionIndex();
                    }
                }
            });
            
            jTableAntTasks.getModel().addTableModelListener(new TableModelListener() {
                public void tableChanged(TableModelEvent arg0) {
                    // TODO Auto-generated method stub
                    int row = arg0.getFirstRow();
                    TableModel m = (TableModel)arg0.getSource();
                    if (arg0.getType() == TableModelEvent.UPDATE){
                        //ToDo Data Validition check.
                        String id = m.getValueAt(row, 0) + "";
                        String file = m.getValueAt(row, 1) + "";
                        String execOrder = m.getValueAt(row, 2) + "";
                        if (id.length() == 0) {
                            return;
                        }
                        if (file.length() == 0 && execOrder.length() == 0){
                            return;
                        }
                        if (file.length() == 0) {
                            file = null;
                        }
                        if (execOrder.length() == 0) {
                            execOrder = null;
                        }
                        ffc.updateBuildOptionsUserDefAntTask(row, id, file, execOrder);
                        
                    }
                }
            });
        }
        return jTableAntTasks;
    }

    /**
     * This method initializes jPanel13	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelFfsTab() {
        if (jPanelFfsTab == null) {
            jPanelFfsTab = new JPanel();
            jPanelFfsTab.setLayout(new BorderLayout());
            jPanelFfsTab.add(getJPanelFfsTabCenter(), java.awt.BorderLayout.CENTER);
            jPanelFfsTab.add(getJScrollPaneFfs(), java.awt.BorderLayout.WEST);
        }
        return jPanelFfsTab;
    }

    /**
     * This method initializes jPanel18	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelFfsTabCenter() {
        if (jPanelFfsTabCenter == null) {
            jPanelFfsTabCenter = new JPanel();
            jPanelFfsTabCenter.setLayout(new BorderLayout());
            jPanelFfsTabCenter.add(getJPanelFfsTabCenterN(), java.awt.BorderLayout.NORTH);
            jPanelFfsTabCenter.add(getJPanelFfsTabCenterS(), java.awt.BorderLayout.SOUTH);
            jPanelFfsTabCenter.add(getJPanelFfsTabCenterE(), java.awt.BorderLayout.EAST);
            jPanelFfsTabCenter.add(getJPanelFfsTabCenterC(), java.awt.BorderLayout.CENTER);
        }
        return jPanelFfsTabCenter;
    }

    /**
     * This method initializes jPanel15	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelFfsTabCenterN() {
        if (jPanelFfsTabCenterN == null) {
            jLabelEncapType = new JLabel();
            jLabelEncapType.setText("Encapsulation Type");
            FlowLayout flowLayout5 = new FlowLayout();
            flowLayout5.setAlignment(java.awt.FlowLayout.RIGHT);
            jLabelFfsKey = new JLabel();
            jLabelFfsKey.setText("FFS Key");
            jPanelFfsTabCenterN = new JPanel();
            jPanelFfsTabCenterN.setLayout(flowLayout5);
            jPanelFfsTabCenterN.add(jLabelFfsKey, null);
            jPanelFfsTabCenterN.add(getJTextFieldFfsKey(), null);
            jPanelFfsTabCenterN.add(jLabelEncapType, null);
            jPanelFfsTabCenterN.add(getJTextFieldEncapType(), null);
            jPanelFfsTabCenterN.add(getJButtonFfsAdd(), null);
            jPanelFfsTabCenterN.add(getJButtonFfsDel(), null);
        }
        return jPanelFfsTabCenterN;
    }

    /**
     * This method initializes jPanel16	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelFfsTabCenterS() {
        if (jPanelFfsTabCenterS == null) {
            jLabelFfsAttribs = new JLabel();
            jLabelFfsAttribs.setText("Attributes");
            FlowLayout flowLayout6 = new FlowLayout();
            flowLayout6.setHgap(5);
            flowLayout6.setVgap(20);
            flowLayout6.setAlignment(java.awt.FlowLayout.CENTER);
            jPanelFfsTabCenterS = new JPanel();
            jPanelFfsTabCenterS.setPreferredSize(new java.awt.Dimension(491,130));
            jPanelFfsTabCenterS.setLayout(flowLayout6);
            jPanelFfsTabCenterS.add(jLabelFfsAttribs, null);
            jPanelFfsTabCenterS.add(getJScrollPaneFfsAttribs(), null);
            jPanelFfsTabCenterS.add(getJPanelFfsAttribButtonGroup(), null);
        }
        return jPanelFfsTabCenterS;
    }

    /**
     * This method initializes jPanel17	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelFfsTabCenterE() {
        if (jPanelFfsTabCenterE == null) {
            jPanelFfsTabCenterE = new JPanel();
        }
        return jPanelFfsTabCenterE;
    }

    /**
     * This method initializes jTextField6	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldFfsKey() {
        if (jTextFieldFfsKey == null) {
            jTextFieldFfsKey = new JTextField();
            jTextFieldFfsKey.setPreferredSize(new java.awt.Dimension(100,20));
            jTextFieldFfsKey.setEditable(true);
            jTextFieldFfsKey.addFocusListener(new java.awt.event.FocusAdapter() {
                public void focusLost(java.awt.event.FocusEvent e) {
                    if (jTableFfs.getSelectedRow() < 0) {
                        return;
                    }
//                    ffc.updateBuildOptionsFfsKey(jTable.getSelectedRow(), jTextField6.getText());
                }
            });
        }
        return jTextFieldFfsKey;
    }

    /**
     * This method initializes jButton8	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonFfsAdd() {
        if (jButtonFfsAdd == null) {
            jButtonFfsAdd = new JButton();
            jButtonFfsAdd.setPreferredSize(new java.awt.Dimension(70,20));
            jButtonFfsAdd.setText("Add");
            jButtonFfsAdd.addActionListener(new AbstractAction() {
                /**
                 * 
                 */
                private static final long serialVersionUID = -2923720717273384221L;

                public void actionPerformed(java.awt.event.ActionEvent e) {
                    if (jTextFieldFfsKey.getText().length() > 0) {
                        String[] row = {jTextFieldFfsKey.getText()};
                        ffsTableModel.addRow(row);
                        docConsole.setSaved(false);
                        ffc.genBuildOptionsFfs(jTextFieldFfsKey.getText(), jTextFieldEncapType.getText());
                    }
                }
            });
        }
        return jButtonFfsAdd;
    }

    /**
     * This method initializes jButton9	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonFfsDel() {
        if (jButtonFfsDel == null) {
            jButtonFfsDel = new JButton();
            jButtonFfsDel.setPreferredSize(new java.awt.Dimension(70,20));
            jButtonFfsDel.setText("Delete");
            jButtonFfsDel.addActionListener(new AbstractAction() {
                /**
                 * 
                 */
                private static final long serialVersionUID = -4002678939178194476L;

                public void actionPerformed(ActionEvent arg0){
                    if (jTableFfs.getSelectedRow() < 0) {
                        return;
                    }
                    docConsole.setSaved(false);
                    ffc.removeBuildOptionsFfs(jTableFfs.getSelectedRow());
                }
            });
        }
        return jButtonFfsDel;
    }

    /**
     * This method initializes jScrollPane5	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPaneFfsAttribs() {
        if (jScrollPaneFfsAttribs == null) {
            jScrollPaneFfsAttribs = new JScrollPane();
            jScrollPaneFfsAttribs.setPreferredSize(new java.awt.Dimension(350,100));
            jScrollPaneFfsAttribs.setViewportView(getJTableFfsAttribs());
        }
        return jScrollPaneFfsAttribs;
    }

    /**
     * This method initializes jTable4	
     * 	
     * @return javax.swing.JTable	
     */
    private JTable getJTableFfsAttribs() {
        if (jTableFfsAttribs == null) {
            ffsAttributesTableModel = new DefaultTableModel();
            jTableFfsAttribs = new JTable(ffsAttributesTableModel);
            jTableFfsAttribs.setPreferredSize(new java.awt.Dimension(400,80));
            jTableFfsAttribs.setRowHeight(20);
            ffsAttributesTableModel.addColumn("Name");
            ffsAttributesTableModel.addColumn("Value");
            
            jTableFfsAttribs.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
            jTableFfsAttribs.getModel().addTableModelListener(new TableModelListener() {
                public void tableChanged(TableModelEvent arg0) {
                    // TODO Auto-generated method stub
                    int row = arg0.getFirstRow();
                    TableModel m = (TableModel)arg0.getSource();
                    if (arg0.getType() == TableModelEvent.UPDATE){
                        //ToDo Data Validition check.
                        String name = m.getValueAt(row, 0) + "";
                        String value = m.getValueAt(row, 1) + "";
                        
                        if (name.length() == 0) {
                            return;
                        }
                        if (value.length() == 0){
                            return;
                        }
                        docConsole.setSaved(false);
                        ffc.updateBuildOptionsFfsAttribute(jTableFfs.getSelectedRow(), row, name, value);
                    }
                }
            });
        }
        return jTableFfsAttribs;
    }

    /**
     * This method initializes jPanel19	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelFfsTabCenterC() {
        if (jPanelFfsTabCenterC == null) {
            jLabelFfsSections = new JLabel();
            jLabelFfsSections.setText("Sections");
            jLabelFfsSubSections = new JLabel();
            jLabelFfsSubSections.setText("Sub-Sections");
            jLabelFfsSection = new JLabel();
            jLabelFfsSection.setText("Section");
            jPanelFfsTabCenterC = new JPanel();
            jPanelFfsTabCenterC.setLayout(new FlowLayout());
            jPanelFfsTabCenterC.add(jLabelFfsSection, null);
            jPanelFfsTabCenterC.add(getJButtonFfsSectionNew(), null);
            jPanelFfsTabCenterC.add(getJButtonFfsSectionRemove(), null);
            jPanelFfsTabCenterC.add(getJScrollPaneFfsSection(), null);
            jPanelFfsTabCenterC.add(jLabelFfsSections, null);
            jPanelFfsTabCenterC.add(getJButtonFfsSectionsNew(), null);
            jPanelFfsTabCenterC.add(getJButtonFfsSectionsRemove(), null);
            jPanelFfsTabCenterC.add(getJScrollPaneFfsSections(), null);
            jPanelFfsTabCenterC.add(jLabelFfsSubSections, null);
            jPanelFfsTabCenterC.add(getJButtonFfsSubSectionNew(), null);
            jPanelFfsTabCenterC.add(getJButtonFfsSubSectionRemove(), null);
            jPanelFfsTabCenterC.add(getJScrollPaneFfsSubSection(), null);
        }
        return jPanelFfsTabCenterC;
    }

    /**
     * This method initializes jPanel20	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelOptionsTab() {
        if (jPanelOptionsTab == null) {
            jLabelTagName = new JLabel();
            jLabelTagName.setText("Tag Name");
            FlowLayout flowLayout9 = new FlowLayout();
            flowLayout9.setAlignment(java.awt.FlowLayout.LEFT);
            jLabelBuildTargets = new JLabel();
            jLabelBuildTargets.setText("Build Targets");
            jLabelToolCmd = new JLabel();
            jLabelToolCmd.setText("Tool Command");
            jLabelSupArch = new JLabel();
            jLabelSupArch.setText("Supported Arch");
            jLabelToolChainFamily = new JLabel();
            jLabelToolChainFamily.setText("Tool Chain Family");
            jLabelOptionContents = new JLabel();
            jLabelOptionContents.setText("Option Contents");
            jPanelOptionsTab = new JPanel();
            jPanelOptionsTab.setLayout(flowLayout9);
            jPanelOptionsTab.add(jLabelBuildTargets, null);
            jPanelOptionsTab.add(getJTextFieldBuildTargets(), null);
            jPanelOptionsTab.add(jLabelToolChainFamily, null);
            jPanelOptionsTab.add(getJTextFieldToolChainFamily(), null);
            jPanelOptionsTab.add(jLabelToolCmd, null);
            jPanelOptionsTab.add(getJTextFieldToolCmd(), null);
            jPanelOptionsTab.add(jLabelSupArch, null);
            jPanelOptionsTab.add(getJCheckBoxIA32(), null);
            jPanelOptionsTab.add(getJCheckBoxIpf(), null);
            jPanelOptionsTab.add(getJCheckBoxX64(), null);
            jPanelOptionsTab.add(getJCheckBoxEBC(), null);
            jPanelOptionsTab.add(getJCheckBoxARM(), null);
            jPanelOptionsTab.add(getJCheckBoxPPC(), null);
            jPanelOptionsTab.add(jLabelTagName, null);
            jPanelOptionsTab.add(getJTextFieldTagName(), null);
            jPanelOptionsTab.add(jLabelOptionContents, null);
            jPanelOptionsTab.add(getJTextFieldOptionContents(), null);
            jPanelOptionsTab.add(getJButtonOptionsAdd(), null);
            jPanelOptionsTab.add(getJButtonOptionsDel(), null);
            jPanelOptionsTab.add(getJScrollPaneOptions(), null);
        }
        return jPanelOptionsTab;
    }

    /**
     * This method initializes jTextField7	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldOptionContents() {
        if (jTextFieldOptionContents == null) {
            jTextFieldOptionContents = new JTextField();
            jTextFieldOptionContents.setPreferredSize(new java.awt.Dimension(300,20));
        }
        return jTextFieldOptionContents;
    }

    /**
     * This method initializes jTextField8	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldToolCmd() {
        if (jTextFieldToolCmd == null) {
            jTextFieldToolCmd = new JTextField();
            jTextFieldToolCmd.setPreferredSize(new java.awt.Dimension(110,20));
        }
        return jTextFieldToolCmd;
    }

    /**
     * This method initializes jScrollPane6	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPaneOptions() {
        if (jScrollPaneOptions == null) {
            jScrollPaneOptions = new JScrollPane();
            jScrollPaneOptions.setPreferredSize(new java.awt.Dimension(630,200));
            jScrollPaneOptions.setViewportView(getJTableOptions());
        }
        return jScrollPaneOptions;
    }

    /**
     * This method initializes jTable5	
     * 	
     * @return javax.swing.JTable	
     */
    private JTable getJTableOptions() {
        if (jTableOptions == null) {
            optionsTableModel = new DefaultTableModel();
            jTableOptions = new JTable(optionsTableModel);
            jTableOptions.setRowHeight(20);
            optionsTableModel.addColumn("BuildTargets");
            optionsTableModel.addColumn("ToolChainFamily");
            optionsTableModel.addColumn("SupportedArch");
            optionsTableModel.addColumn("ToolCommand");
            optionsTableModel.addColumn("TagName");
            optionsTableModel.addColumn("Contents");
            
//            TableColumn toolFamilyCol = jTable5.getColumnModel().getColumn(1);
//            JComboBox cb = new JComboBox();
//            cb.addItem("MSFT");
//            cb.addItem("GCC");
//            cb.addItem("CYGWIN");
//            cb.addItem("INTEL");
//            toolFamilyCol.setCellEditor(new DefaultCellEditor(cb));
            Vector<String> vArch = new Vector<String>();
            vArch.add("IA32");
            vArch.add("X64");
            vArch.add("IPF");
            vArch.add("EBC");
            vArch.add("ARM");
            vArch.add("PPC");
            jTableOptions.getColumnModel().getColumn(2).setCellEditor(new ListEditor(vArch));
            jTableOptions.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
            jTableOptions.getSelectionModel().addListSelectionListener(new ListSelectionListener(){
                public void valueChanged(ListSelectionEvent e) {
                    selectedRow = -1;
                    if (e.getValueIsAdjusting()){
                        return;
                    }
                    ListSelectionModel lsm = (ListSelectionModel)e.getSource();
                    if (lsm.isSelectionEmpty()) {
                        return;
                    }
                    else{
                        selectedRow = lsm.getMinSelectionIndex();
                    }
                }
            });
            
            jTableOptions.getModel().addTableModelListener(new TableModelListener() {
                public void tableChanged(TableModelEvent arg0) {
                    // TODO Auto-generated method stub
                    int row = arg0.getFirstRow();
                    TableModel m = (TableModel)arg0.getSource();
                    if (arg0.getType() == TableModelEvent.UPDATE){
                        //ToDo Data Validition check.
                        String targets = m.getValueAt(row, 0) + "";
                        Vector<Object> targetName = new Vector<Object>();
                        String[] sArray = targets.split("( )+");
                        for (int i = 0; i < sArray.length; ++i) {
                            targetName.add(sArray[i]);
                        }
                        String toolChain = m.getValueAt(row, 1) + "";
                        String archs = m.getValueAt(row, 2) + "";
                        Vector<Object> supArch = null;
                        if (archs.length() > 0) {
                            supArch = new Vector<Object>();
                            String[] sArray1 = archs.split("( )+");
                            for (int i = 0; i < sArray1.length; ++i) {
                                supArch.add(sArray1[i]);
                            }
                        }
                        
                        String toolCmd = m.getValueAt(row, 3) + "";
                        String tagName = m.getValueAt(row, 4) + "";
                        String contents = m.getValueAt(row, 5) + "";
                        docConsole.setSaved(false);
                        ffc.updateBuildOptionsOpt(row, targetName, toolChain, tagName, toolCmd, supArch, contents);
                    }
                }
            });
        }
        return jTableOptions;
    }

    /**
     * This method initializes jButton10	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonOptionsAdd() {
        if (jButtonOptionsAdd == null) {
            jButtonOptionsAdd = new JButton();
            jButtonOptionsAdd.setText("Add");
            jButtonOptionsAdd.setPreferredSize(new java.awt.Dimension(90,20));
            jButtonOptionsAdd.addActionListener(new AbstractAction() {
                /**
                 * 
                 */
                private static final long serialVersionUID = 1L;

                public void actionPerformed(java.awt.event.ActionEvent e) {
                    boolean[] boolArray = {jCheckBoxIA32.isSelected(),jCheckBoxIpf.isSelected(),jCheckBoxX64.isSelected(),
                                           jCheckBoxEBC.isSelected(),jCheckBoxARM.isSelected(),jCheckBoxPPC.isSelected()};
                    String s = boolToList(boolArray);
                    Object[] o = {jTextFieldBuildTargets.getText(), jTextFieldToolChainFamily.getText(), s,
                                  jTextFieldToolCmd.getText(), jTextFieldTagName.getText(), jTextFieldOptionContents.getText()};
                    optionsTableModel.addRow(o);
                    docConsole.setSaved(false);
                    ffc.genBuildOptionsOpt(stringToVector(jTextFieldBuildTargets.getText()), jTextFieldToolChainFamily.getText(), jTextFieldTagName.getText(), jTextFieldToolCmd.getText(),  stringToVector(s), jTextFieldOptionContents.getText());
                }
            });
        }
        return jButtonOptionsAdd;
    }
    
    private Vector<Object> stringToVector(String s) {
        String[] sArray = s.split(" ");
        Vector<Object> v = null;
        if (s.length() > 0) {
            v = new Vector<Object>();
            for (int i = 0; i < sArray.length; ++i) {
                v.add(sArray[i]);
            } 
        }
        return v;
    }
    
    private String boolToList (boolean[] bool) {
        String s = " ";
        if (bool[0]) {
            s += "IA32 ";
        }
        if (bool[1]) {
            s += "IPF ";
        }
        if (bool[2]) {
            s += "X64 ";
        }
        if (bool[3]) {
            s += "EBC ";
        }
        if (bool[4]) {
            s += "ARM ";
        }
        if (bool[5]) {
            s += "PPC ";
        }
        if (s == " ") {
            s += "IA32";
        }
        return s.trim();
    }

    /**
     * This method initializes jButton11	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonOptionsDel() {
        if (jButtonOptionsDel == null) {
            jButtonOptionsDel = new JButton();
            jButtonOptionsDel.setText("Delete");
            jButtonOptionsDel.setPreferredSize(new java.awt.Dimension(90,20));
            jButtonOptionsDel.addActionListener(new AbstractAction() {
                /**
                 * 
                 */
                private static final long serialVersionUID = 1L;

                public void actionPerformed(java.awt.event.ActionEvent e) {
                    if (selectedRow >= 0) {
                        optionsTableModel.removeRow(selectedRow);
                        docConsole.setSaved(false);
                        ffc.removeBuildOptionsOpt(selectedRow);
                    }
                }
            });
        }
        return jButtonOptionsDel;
    }

    /**
     * This method initializes jButton17	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonFfsAttribNew() {
        if (jButtonFfsAttribNew == null) {
            jButtonFfsAttribNew = new JButton();
            jButtonFfsAttribNew.setPreferredSize(new java.awt.Dimension(80,20));
            jButtonFfsAttribNew.setText("New");
            jButtonFfsAttribNew.addActionListener(new AbstractAction() {
                /**
                 * 
                 */
                private static final long serialVersionUID = 1L;

                public void actionPerformed(ActionEvent arg0){
                    if (jTableFfs.getSelectedRow() < 0) {
                        return;
                    }
                    Object[] o = {"", ""};
                    ffsAttributesTableModel.addRow(o);
                    docConsole.setSaved(false);
                    ffc.genBuildOptionsFfsAttribute(jTableFfs.getSelectedRow(), "", "");
                }
            });
        }
        return jButtonFfsAttribNew;
    }

    /**
     * This method initializes jButton18	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonFfsAttribRemove() {
        if (jButtonFfsAttribRemove == null) {
            jButtonFfsAttribRemove = new JButton();
            jButtonFfsAttribRemove.setPreferredSize(new java.awt.Dimension(80,20));
            jButtonFfsAttribRemove.setText("Remove");
            jButtonFfsAttribRemove.addActionListener(new AbstractAction(){
                /**
                 * 
                 */
                private static final long serialVersionUID = 1L;

                public void actionPerformed(ActionEvent arg0){
                    if (jTableFfs.getSelectedRow() < 0) {
                        return;
                    }
                    if (jTableFfsAttribs.getSelectedRow() >= 0){
                        docConsole.setSaved(false);
                        ffsAttributesTableModel.removeRow(jTableFfsAttribs.getSelectedRow());
                        ffc.removeBuildOptionsFfsAttribute(jTableFfs.getSelectedRow(), jTableFfsAttribs.getSelectedRow());
                    }
                }
            });
        }
        return jButtonFfsAttribRemove;
    }

    /**
     * This method initializes jCheckBox9	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBoxIA32() {
        if (jCheckBoxIA32 == null) {
            jCheckBoxIA32 = new JCheckBox();
            jCheckBoxIA32.setPreferredSize(new java.awt.Dimension(50,20));
            jCheckBoxIA32.setText("IA32");
        }
        return jCheckBoxIA32;
    }

    /**
     * This method initializes jCheckBox10	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBoxIpf() {
        if (jCheckBoxIpf == null) {
            jCheckBoxIpf = new JCheckBox();
            jCheckBoxIpf.setPreferredSize(new java.awt.Dimension(50,20));
            jCheckBoxIpf.setText("IPF");
        }
        return jCheckBoxIpf;
    }

    /**
     * This method initializes jCheckBox11	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBoxX64() {
        if (jCheckBoxX64 == null) {
            jCheckBoxX64 = new JCheckBox();
            jCheckBoxX64.setText("X64");
            jCheckBoxX64.setPreferredSize(new java.awt.Dimension(47,20));
        }
        return jCheckBoxX64;
    }

    /**
     * This method initializes jCheckBox12	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBoxEBC() {
        if (jCheckBoxEBC == null) {
            jCheckBoxEBC = new JCheckBox();
            jCheckBoxEBC.setPreferredSize(new java.awt.Dimension(50,20));
            jCheckBoxEBC.setText("EBC");
        }
        return jCheckBoxEBC;
    }

    /**
     * This method initializes jCheckBox13	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBoxARM() {
        if (jCheckBoxARM == null) {
            jCheckBoxARM = new JCheckBox();
            jCheckBoxARM.setPreferredSize(new java.awt.Dimension(55,20));
            jCheckBoxARM.setText("ARM");
        }
        return jCheckBoxARM;
    }

    /**
     * This method initializes jCheckBox14	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBoxPPC() {
        if (jCheckBoxPPC == null) {
            jCheckBoxPPC = new JCheckBox();
            jCheckBoxPPC.setPreferredSize(new java.awt.Dimension(50,20));
            jCheckBoxPPC.setText("PPC");
        }
        return jCheckBoxPPC;
    }

    /**
     * This method initializes jTextField12	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldBuildTargets() {
        if (jTextFieldBuildTargets == null) {
            jTextFieldBuildTargets = new JTextField();
            jTextFieldBuildTargets.setPreferredSize(new java.awt.Dimension(150,20));
        }
        return jTextFieldBuildTargets;
    }

    /**
     * This method initializes jTextField13	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldTagName() {
        if (jTextFieldTagName == null) {
            jTextFieldTagName = new JTextField();
            jTextFieldTagName.setPreferredSize(new java.awt.Dimension(140,20));
        }
        return jTextFieldTagName;
    }

    /**
     * This method initializes jScrollPane	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPaneFfs() {
        if (jScrollPaneFfs == null) {
            jScrollPaneFfs = new JScrollPane();
            jScrollPaneFfs.setPreferredSize(new java.awt.Dimension(150,419));
            jScrollPaneFfs.setViewportView(getJTableFfs());
        }
        return jScrollPaneFfs;
    }

    /**
     * This method initializes jTable	
     * 	
     * @return javax.swing.JTable	
     */
    private JTable getJTableFfs() {
        if (jTableFfs == null) {
            ffsTableModel = new DefaultTableModel();
            ffsTableModel.addColumn("FFS Type");
            jTableFfs = new JTable(ffsTableModel);
            jTableFfs.setShowGrid(false);
            jTableFfs.setRowHeight(20);
            
            jTableFfs.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
            jTableFfs.getSelectionModel().addListSelectionListener(new ListSelectionListener(){
                public void valueChanged(ListSelectionEvent e) {
                    
                    if (e.getValueIsAdjusting()){
                        return;
                    }
                    ListSelectionModel lsm = (ListSelectionModel)e.getSource();
                    if (lsm.isSelectionEmpty()) {
                        return;
                    }
                    else{
                        int row = lsm.getMinSelectionIndex();
                        sectionTableModel.setRowCount(0);
                        sectionsTableModel.setRowCount(0);
                        subsectionsTableModel.setRowCount(0);
                        ffsAttributesTableModel.setRowCount(0);
                        String[] sArray = {"", ""};
                        LinkedHashMap<String, String> lhm = new LinkedHashMap<String, String>();
                        ArrayList<String> alSections = new ArrayList<String>();
                        ArrayList<String> alSection = new ArrayList<String>();
                        ffc.getBuildOptionsFfs(row, sArray, lhm, alSections, alSection);
                        jTextFieldFfsKey.setText(sArray[0]);
                        jTextFieldEncapType.setText(sArray[1]);
                        for (int i = 0; i < alSection.size(); ++i) {
                            String[] sectionRow = {alSection.get(i)};
                            sectionTableModel.addRow(sectionRow);
                        }
                        for (int j = 0; j < alSections.size(); ++j) {
                            String[] sectionsRow = {alSections.get(j)};
                            sectionsTableModel.addRow(sectionsRow);
                        }
                        if (lhm.size() <= 0 ) {
                            return;
                        }
                        Set<String> keySet = lhm.keySet();
                        Iterator<String> is = keySet.iterator();
                        while(is.hasNext()) {
                            String key = is.next();
                            String[] attribRow = {key, lhm.get(key)};
                            ffsAttributesTableModel.addRow(attribRow);
                        }
                    }
                }
            });
            
            jTableFfs.getModel().addTableModelListener(new TableModelListener() {
                public void tableChanged(TableModelEvent arg0) {
                    // TODO Auto-generated method stub
                    int row = arg0.getFirstRow();
                    TableModel m = (TableModel)arg0.getSource();
                    if (arg0.getType() == TableModelEvent.UPDATE){
                        //ToDo Data Validition check.
                        String id = m.getValueAt(row, 0) + "";
                        
                        if (id.length() == 0) {
                            return;
                        }
                        docConsole.setSaved(false);
                        ffc.updateBuildOptionsFfsKey(row, id);
                    }
                }
            });
        }
        return jTableFfs;
    }

    /**
     * This method initializes jScrollPane1	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPaneFfsSection() {
        if (jScrollPaneFfsSection == null) {
            jScrollPaneFfsSection = new JScrollPane();
            jScrollPaneFfsSection.setPreferredSize(new java.awt.Dimension(500,80));
            jScrollPaneFfsSection.setViewportView(getJTableFfsSection());
        }
        return jScrollPaneFfsSection;
    }

    /**
     * This method initializes jTable1	
     * 	
     * @return javax.swing.JTable	
     */
    private JTable getJTableFfsSection() {
        if (jTableFfsSection == null) {
            sectionTableModel = new DefaultTableModel();
            sectionTableModel.addColumn("SectionType");
            
            jTableFfsSection = new JTable(sectionTableModel);
            jTableFfsSection.setRowHeight(20);
            JComboBox cb = new JComboBox();
            cb.addItem("EFI_SECTION_FREEFORM_SUBTYPE_GUID");
            cb.addItem("EFI_SECTION_VERSION");
            cb.addItem("EFI_SECTION_USER_INTERFACE");
            cb.addItem("EFI_SECTION_DXE_DEPEX");
            cb.addItem("EFI_SECTION_PEI_DEPEX");
            cb.addItem("EFI_SECTION_PE32");
            cb.addItem("EFI_SECTION_PIC");
            cb.addItem("EFI_SECTION_TE");
            cb.addItem("EFI_SECTION_RAW");
            cb.addItem("EFI_SECTION_COMPRESSION");
            cb.addItem("EFI_SECTION_GUID_DEFINED");
            cb.addItem("EFI_SECTION_COMPATIBILITY16");
            cb.addItem("EFI_SECTION_FIRMWARE_VOLUME_IMAGE");
            jTableFfsSection.getColumnModel().getColumn(0).setCellEditor(new DefaultCellEditor(cb));
            
            jTableFfsSection.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
            
            jTableFfsSection.getModel().addTableModelListener(new TableModelListener() {
                public void tableChanged(TableModelEvent arg0) {
                    // TODO Auto-generated method stub
                    if (jTableFfs.getSelectedRow() < 0) {
                        return;
                    }
                    int row = arg0.getFirstRow();
                    TableModel m = (TableModel)arg0.getSource();
                    if (arg0.getType() == TableModelEvent.UPDATE){
                        //ToDo Data Validition check.
                        String type = m.getValueAt(row, 0) + "";
                        docConsole.setSaved(false);
                       ffc.updateBuildOptionsFfsSectionsSection(jTableFfs.getSelectedRow(), row, type);
                    }
                }
            });
        }
        return jTableFfsSection;
    }

    /**
     * This method initializes jScrollPane3	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPaneFfsSubSection() {
        if (jScrollPaneFfsSubSection == null) {
            jScrollPaneFfsSubSection = new JScrollPane();
            jScrollPaneFfsSubSection.setPreferredSize(new java.awt.Dimension(500,90));
            jScrollPaneFfsSubSection.setViewportView(getJTableFfsSubSection());
        }
        return jScrollPaneFfsSubSection;
    }

    /**
     * This method initializes jTable3	
     * 	
     * @return javax.swing.JTable	
     */
    private JTable getJTableFfsSubSection() {
        if (jTableFfsSubSection == null) {
            subsectionsTableModel = new DefaultTableModel();
            subsectionsTableModel.addColumn("SectionType");
            jTableFfsSubSection = new JTable(subsectionsTableModel);
            jTableFfsSubSection.setRowHeight(20);
            JComboBox cb = new JComboBox();
            cb.addItem("EFI_SECTION_FREEFORM_SUBTYPE_GUID");
            cb.addItem("EFI_SECTION_VERSION");
            cb.addItem("EFI_SECTION_USER_INTERFACE");
            cb.addItem("EFI_SECTION_DXE_DEPEX");
            cb.addItem("EFI_SECTION_PEI_DEPEX");
            cb.addItem("EFI_SECTION_PE32");
            cb.addItem("EFI_SECTION_PIC");
            cb.addItem("EFI_SECTION_TE");
            cb.addItem("EFI_SECTION_RAW");
            cb.addItem("EFI_SECTION_COMPRESSION");
            cb.addItem("EFI_SECTION_GUID_DEFINED");
            cb.addItem("EFI_SECTION_COMPATIBILITY16");
            cb.addItem("EFI_SECTION_FIRMWARE_VOLUME_IMAGE");
            jTableFfsSubSection.getColumnModel().getColumn(0).setCellEditor(new DefaultCellEditor(cb));
            jTableFfsSubSection.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
            
            jTableFfsSubSection.getModel().addTableModelListener(new TableModelListener() {
                public void tableChanged(TableModelEvent arg0) {
                    // TODO Auto-generated method stub
                    if (jTableFfs.getSelectedRow() < 0 || jTableFfsSections.getSelectedRow() < 0) {
                        return;
                    }
                    int row = arg0.getFirstRow();
                    TableModel m = (TableModel)arg0.getSource();
                    if (arg0.getType() == TableModelEvent.UPDATE){
                        //ToDo Data Validition check.
                        String type = m.getValueAt(row, 0) + "";
                        docConsole.setSaved(false);
                        ffc.updateBuildOptionsFfsSectionsSectionsSection(jTableFfs.getSelectedRow(), jTableFfsSections.getSelectedRow(), row, type);
                    }
                }
            });
        }
        return jTableFfsSubSection;
    }

    /**
     * This method initializes jTextField	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldEncapType() {
        if (jTextFieldEncapType == null) {
            jTextFieldEncapType = new JTextField();
            jTextFieldEncapType.setPreferredSize(new java.awt.Dimension(100,20));
            jTextFieldEncapType.addFocusListener(new java.awt.event.FocusAdapter() {
                public void focusLost(java.awt.event.FocusEvent e) {
                    if (jTableFfs.getSelectedRow() < 0) {
                        return;
                    }
                    ffc.updateBuildOptionsFfsSectionsType(jTableFfs.getSelectedRow(), jTextFieldEncapType.getText());
                }
            });
        }
        return jTextFieldEncapType;
    }

    /**
     * This method initializes jPanel4	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelFfsAttribButtonGroup() {
        if (jPanelFfsAttribButtonGroup == null) {
            jPanelFfsAttribButtonGroup = new JPanel();
            jPanelFfsAttribButtonGroup.setPreferredSize(new java.awt.Dimension(100,100));
            jPanelFfsAttribButtonGroup.add(getJButtonFfsAttribNew(), null);
            jPanelFfsAttribButtonGroup.add(getJButtonFfsAttribRemove(), null);
        }
        return jPanelFfsAttribButtonGroup;
    }

    /**
     * This method initializes jButton	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonFfsSectionNew() {
        if (jButtonFfsSectionNew == null) {
            jButtonFfsSectionNew = new JButton();
            jButtonFfsSectionNew.setPreferredSize(new java.awt.Dimension(80,20));
            jButtonFfsSectionNew.setText("New");
            jButtonFfsSectionNew.addActionListener(new java.awt.event.ActionListener() {
                public void actionPerformed(java.awt.event.ActionEvent e) {
                    if (jTableFfs.getSelectedRow() < 0) {
                        return;
                    }
                    docConsole.setSaved(false);
                    String[] row = {"EFI_SECTION_RAW"};
                    sectionTableModel.addRow(row);
                    ffc.genBuildOptionsFfsSectionsSection(jTableFfs.getSelectedRow(), row[0]);
                }
            });
        }
        return jButtonFfsSectionNew;
    }

    /**
     * This method initializes jButton1	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonFfsSectionRemove() {
        if (jButtonFfsSectionRemove == null) {
            jButtonFfsSectionRemove = new JButton();
            jButtonFfsSectionRemove.setPreferredSize(new java.awt.Dimension(80,20));
            jButtonFfsSectionRemove.setText("Remove");
          
            jButtonFfsSectionRemove.addActionListener(new java.awt.event.ActionListener() {
                public void actionPerformed(java.awt.event.ActionEvent e) {
                    if (jTableFfs.getSelectedRow() < 0 || jTableFfsSection.getSelectedRow() < 0) {
                        return;
                    }
                    docConsole.setSaved(false);
                    sectionTableModel.removeRow(jTableFfsSection.getSelectedRow());
                    ffc.removeBuildOptionsFfsSectionsSection(jTableFfs.getSelectedRow(), jTableFfsSection.getSelectedRow());
                }
            });
        }
        return jButtonFfsSectionRemove;
    }

    /**
     * This method initializes jButton2	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonFfsSubSectionNew() {
        if (jButtonFfsSubSectionNew == null) {
            jButtonFfsSubSectionNew = new JButton();
            jButtonFfsSubSectionNew.setPreferredSize(new java.awt.Dimension(80,20));
            jButtonFfsSubSectionNew.setText("New");
            jButtonFfsSubSectionNew.addActionListener(new java.awt.event.ActionListener() {
                public void actionPerformed(java.awt.event.ActionEvent e) {
                    if (jTableFfs.getSelectedRow() < 0 || jTableFfsSections.getSelectedRow() < 0) {
                        return;
                    }
                    docConsole.setSaved(false);
                    String[] row = {"EFI_SECTION_RAW"};
                    subsectionsTableModel.addRow(row);
                    ffc.genBuildOptionsFfsSectionsSectionsSection(jTableFfs.getSelectedRow(), jTableFfsSections.getSelectedRow(), row[0]);
                    
                }
            });
        }
        return jButtonFfsSubSectionNew;
    }

    /**
     * This method initializes jButton3	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonFfsSubSectionRemove() {
        if (jButtonFfsSubSectionRemove == null) {
            jButtonFfsSubSectionRemove = new JButton();
            jButtonFfsSubSectionRemove.setPreferredSize(new java.awt.Dimension(80,20));
            jButtonFfsSubSectionRemove.setText("Remove");
            jButtonFfsSubSectionRemove.addActionListener(new java.awt.event.ActionListener() {
                public void actionPerformed(java.awt.event.ActionEvent e) {
                    if (jTableFfs.getSelectedRow() < 0 || jTableFfsSections.getSelectedRow() < 0 || jTableFfsSubSection.getSelectedRow() < 0) {
                        return;
                    }
                    docConsole.setSaved(false);
                    subsectionsTableModel.removeRow(jTableFfsSubSection.getSelectedRow());
                    ffc.removeBuildOptionsFfsSectionsSectionsSection(jTableFfs.getSelectedRow(), jTableFfsSections.getSelectedRow(), jTableFfsSubSection.getSelectedRow());
                }
            });
        }
        return jButtonFfsSubSectionRemove;
    }

    /**
     * This method initializes jButton6	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonFfsSectionsNew() {
        if (jButtonFfsSectionsNew == null) {
            jButtonFfsSectionsNew = new JButton();
            jButtonFfsSectionsNew.setPreferredSize(new java.awt.Dimension(80,20));
            jButtonFfsSectionsNew.setText("New");
            jButtonFfsSectionsNew.addActionListener(new java.awt.event.ActionListener() {
                public void actionPerformed(java.awt.event.ActionEvent e) {
                    if (jTableFfs.getSelectedRow() < 0) {
                        return;
                    }
                    docConsole.setSaved(false);
                    String[] row = {""};
                    sectionsTableModel.addRow(row);
                    ffc.genBuildOptionsFfsSectionsSections(jTableFfs.getSelectedRow(), "");
                }
            });
        }
        return jButtonFfsSectionsNew;
    }

    /**
     * This method initializes jButton7	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonFfsSectionsRemove() {
        if (jButtonFfsSectionsRemove == null) {
            jButtonFfsSectionsRemove = new JButton();
            jButtonFfsSectionsRemove.setPreferredSize(new java.awt.Dimension(80,20));
            jButtonFfsSectionsRemove.setText("Remove");
            jButtonFfsSectionsRemove.addActionListener(new java.awt.event.ActionListener() {
                public void actionPerformed(java.awt.event.ActionEvent e) {
                    if (jTableFfs.getSelectedRow() < 0 || jTableFfsSections.getSelectedRow() < 0) {
                        return;
                    }
                    docConsole.setSaved(false);
                    sectionsTableModel.removeRow(jTableFfsSections.getSelectedRow());
                    ffc.removeBuildOptionsFfsSectionsSections(jTableFfs.getSelectedRow(), jTableFfsSections.getSelectedRow());
                }
            });
        }
        return jButtonFfsSectionsRemove;
    }

    /**
     * This method initializes jScrollPane4	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPaneFfsSections() {
        if (jScrollPaneFfsSections == null) {
            jScrollPaneFfsSections = new JScrollPane();
            jScrollPaneFfsSections.setPreferredSize(new java.awt.Dimension(500,80));
            jScrollPaneFfsSections.setViewportView(getJTableFfsSections());
        }
        return jScrollPaneFfsSections;
    }

    /**
     * This method initializes jTable6	
     * 	
     * @return javax.swing.JTable	
     */
    private JTable getJTableFfsSections() {
        if (jTableFfsSections == null) {
            sectionsTableModel = new DefaultTableModel();
            sectionsTableModel.addColumn("EncapsulationType");
            jTableFfsSections = new JTable(sectionsTableModel);
            jTableFfsSections.setRowHeight(20);
            jTableFfsSections.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
            jTableFfsSections.getSelectionModel().addListSelectionListener(new ListSelectionListener(){
                public void valueChanged(ListSelectionEvent e) {
                    if (e.getValueIsAdjusting()){
                        return;
                    }
                    ListSelectionModel lsm = (ListSelectionModel)e.getSource();
                    if (lsm.isSelectionEmpty()) {
                        return;
                    }
                    else{
                        int sectionsRow = lsm.getMinSelectionIndex();
                        if (jTableFfs.getSelectedRow() < 0) {
                            return;
                        }
                        subsectionsTableModel.setRowCount(0);
                        ArrayList<String> al = new ArrayList<String>();
                        ffc.getBuildOptionsFfsSectionsSectionsSection(jTableFfs.getSelectedRow(), sectionsRow, al);
                        for (int i = 0; i < al.size(); ++i) {
                            String[] subsectionRow = {al.get(i)};
                            subsectionsTableModel.addRow(subsectionRow);
                        }
                    }
                }
            });
            
            jTableFfsSections.getModel().addTableModelListener(new TableModelListener() {
                public void tableChanged(TableModelEvent arg0) {
                    // TODO Auto-generated method stub
                    int row = arg0.getFirstRow();
                    TableModel m = (TableModel)arg0.getSource();
                    if (arg0.getType() == TableModelEvent.UPDATE){
                        //ToDo Data Validition check.
                        String encapType = m.getValueAt(row, 0) + "";
                        docConsole.setSaved(false);
                        ffc.updateBuildOptionsFfsSectionsSections(jTableFfs.getSelectedRow(), row, encapType);
                    }
                }
            });
        }
        return jTableFfsSections;
    }

    /**
     * This method initializes jButton12	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonAntTaskFileBrowse() {
        if (jButtonAntTaskFileBrowse == null) {
            jButtonAntTaskFileBrowse = new JButton();
            jButtonAntTaskFileBrowse.setPreferredSize(new Dimension(90, 20));
            jButtonAntTaskFileBrowse.setText("Browse");
            jButtonAntTaskFileBrowse.addActionListener(new AbstractAction() {
                private static final long serialVersionUID = 1L;

                public void actionPerformed(ActionEvent arg0) {
                    //
                    // Select files from current workspace
                    //
                    String dirPrefix = System.getenv("WORKSPACE");
                    JFileChooser chooser = new JFileChooser(dirPrefix);
                    File theFile = null;
                    String headerDest = null;
                    
                    chooser.setMultiSelectionEnabled(false);
                    chooser.setFileSelectionMode(JFileChooser.FILES_ONLY);
                    int retval = chooser.showOpenDialog(frame);
                    if (retval == JFileChooser.APPROVE_OPTION) {

                        theFile = chooser.getSelectedFile();
                        String file = theFile.getPath();
                        if (!file.startsWith(dirPrefix)) {
                            JOptionPane.showMessageDialog(frame, "You can only select files in current package!");
                            return;
                        }
                    }
                    else {
                        return;
                    }
                    
                    headerDest = theFile.getPath();
                    jTextFieldAntTaskFile.setText(headerDest.substring(dirPrefix.length()).replace('\\', '/'));
               
                }

            });
        }
        return jButtonAntTaskFileBrowse;
    }

    /**
     * This method initializes jTextField1	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldToolChainFamily() {
        if (jTextFieldToolChainFamily == null) {
            jTextFieldToolChainFamily = new JTextField();
            jTextFieldToolChainFamily.setPreferredSize(new java.awt.Dimension(85,20));
        }
        return jTextFieldToolChainFamily;
    }

    /**
     * @param args
     */
    public static void main(String[] args) {
        // TODO Auto-generated method stub
        new FpdBuildOptions().setVisible(true);
    }

    /**
     * This is the default constructor
     */
    public FpdBuildOptions() {
        super();
        initialize();
        this.setVisible(true);
    }
    
    public FpdBuildOptions(PlatformSurfaceAreaDocument.PlatformSurfaceArea fpd) {
        this();
        ffc = new FpdFileContents(fpd);
        init(ffc);
    }
    
    public FpdBuildOptions(OpeningPlatformType opt) {
        this(opt.getXmlFpd());
        docConsole = opt;
    }

    private void init(FpdFileContents ffc) {
        initOptionTable();
        initAntTaskTable();
        initFfsTable();
        this.addInternalFrameListener(new InternalFrameAdapter(){
            public void internalFrameDeactivated(InternalFrameEvent e){
                if (jTableFfs.isEditing()) {
                    jTableFfs.getCellEditor().stopCellEditing();
                }
                if (jTableFfsSection.isEditing()) {
                    jTableFfsSection.getCellEditor().stopCellEditing();
                }
                if (jTableAntTasks.isEditing()) {
                    jTableAntTasks.getCellEditor().stopCellEditing();
                }
                if (jTableFfsSubSection.isEditing()) {
                    jTableFfsSubSection.getCellEditor().stopCellEditing();
                }
                if (jTableFfsAttribs.isEditing()) {
                    jTableFfsAttribs.getCellEditor().stopCellEditing();
                }
                if (jTableOptions.isEditing()) {
                    jTableOptions.getCellEditor().stopCellEditing();
                }
            }
        });
    }
    
    private void initOptionTable() {
        if (ffc.getBuildOptionsOptCount() == 0) {
            //ToDo get default options from *.txt file
            return;
        }
        String[][] saa = new String[ffc.getBuildOptionsOptCount()][6];
        ffc.getBuildOptionsOpts(saa);
        for (int i = 0; i < saa.length; ++i) {
            optionsTableModel.addRow(saa[i]);
        }
    }
    
    private void initAntTaskTable() {
        if (ffc.getBuildOptionsUserDefAntTaskCount() == 0) {
            return;
        }
        String[][] saa = new String[ffc.getBuildOptionsUserDefAntTaskCount()][3];
        ffc.getBuildOptionsUserDefAntTasks(saa);
        for (int i = 0; i < saa.length; ++i) {
            antTaskTableModel.addRow(saa[i]);
        }
    }
    
    private void initFfsTable(){
        int ffsCount = ffc.getBuildOptionsFfsCount();
        if (ffsCount < 0) {
            return;
        }
        String[][] saa = new String[ffsCount][1];
        ffc.getBuildOptionsFfsKey(saa);
        for (int i = 0; i < saa.length; ++i) {
            ffsTableModel.addRow(saa[i]);
        }
    }
    /**
     * This method initializes this
     * 
     * @return void
     */
    private void initialize() {
        this.setSize(722, 577);
        this.setTitle("FPD Build Options");
        this.setContentPane(getJContentPane());
    }

    /**
     * This method initializes jContentPane
     * 
     * @return javax.swing.JPanel
     */
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jContentPane = new JPanel();
            jContentPane.setLayout(new BorderLayout());
            jContentPane.add(getJPanelContentSouth(), java.awt.BorderLayout.SOUTH);
            jContentPane.add(getJPanelContentNorth(), java.awt.BorderLayout.NORTH);
            jContentPane.add(getJPanelContentWest(), java.awt.BorderLayout.WEST);
            jContentPane.add(getJPanelContentEast(), java.awt.BorderLayout.EAST);
            jContentPane.add(getJTabbedPane(), java.awt.BorderLayout.CENTER);
        }
        return jContentPane;
    }

}  //  @jve:decl-index=0:visual-constraint="10,10"
