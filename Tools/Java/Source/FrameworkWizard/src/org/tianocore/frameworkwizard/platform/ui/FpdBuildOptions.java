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
import java.awt.event.ComponentEvent;

import javax.swing.DefaultCellEditor;
import javax.swing.JFileChooser;
import javax.swing.JOptionPane;
import javax.swing.JTextField;
import javax.swing.JButton;
import javax.swing.JScrollPane;
import javax.swing.JTable;
import javax.swing.JComboBox;
import javax.swing.ListSelectionModel;
import javax.swing.event.DocumentEvent;
import javax.swing.event.DocumentListener;
import javax.swing.event.InternalFrameAdapter;
import javax.swing.event.InternalFrameEvent;
import javax.swing.event.ListSelectionEvent;
import javax.swing.event.ListSelectionListener;
import javax.swing.event.TableModelEvent;
import javax.swing.event.TableModelListener;
import javax.swing.table.DefaultTableModel;
import javax.swing.table.TableModel;

import org.tianocore.PlatformSurfaceAreaDocument;
import org.tianocore.frameworkwizard.FrameworkWizardUI;
import org.tianocore.frameworkwizard.common.DataValidation;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.Identifications.OpeningPlatformType;
import org.tianocore.frameworkwizard.common.ui.IInternalFrame;
import org.tianocore.frameworkwizard.workspace.Workspace;

import java.io.File;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.Set;
import java.util.Vector;
import java.awt.Dimension;
import javax.swing.JSplitPane;
import java.awt.GridLayout;

public class FpdBuildOptions extends IInternalFrame {

    private final int oneRowHeight = 20;

    private final int twoRowHeight = 40;

    private final int sepHeight = 6;

    private final int sepWidth = 10;

    private final int buttonWidth = 90;

    private final int rowOne = 12;

    private final int dialogWidth = 600;

    private final int rowTwo = rowOne + oneRowHeight + sepHeight;

    private final int rowThree = rowTwo + oneRowHeight + sepHeight;

    private final int rowFour = rowThree + oneRowHeight + sepHeight;

    private final int rowFive = rowFour + oneRowHeight + sepHeight;

    private final int rowSix = rowFive + oneRowHeight + sepHeight;

    private final int rowSeven = rowSix + oneRowHeight + sepHeight;

    private final int buttonRow = rowSeven + oneRowHeight + sepHeight + sepHeight;

    private final int dialogHeight = buttonRow + twoRowHeight + twoRowHeight;

    private final int lastButtonXLoc = dialogWidth - buttonWidth - sepWidth;

    private final int next2LastButtonLoc = lastButtonXLoc - buttonWidth - sepWidth;

    private final int firstButtonLoc = next2LastButtonLoc - buttonWidth - sepWidth;

    private final int labelColumn = 12;

    private final int fieldColumn = 168;

    private final int labelWidth = 155;

    private final int fieldWidth = 320;

    private static final long serialVersionUID = 1L;

    private JPanel jContentPane = null;

    //    private JPanel jPanelContentSouth = null;

    //    private JPanel jPanelContentNorth = null;

    //    private JPanel jPanelContentWest = null;

    //    private JPanel jPanelContentEast = null;

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

    private JButton jButtonFfsAdd = null;

    private JButton jButtonFfsDel = null;

    private JScrollPane jScrollPaneFfsAttribs = null;

    private JTable jTableFfsAttribs = null;

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

    private JPanel jArchitectureSelections = null;

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

    private JSplitPane jSplitPaneFfsC = null;

    private JPanel jPanelFfsCTop = null;

    private JSplitPane jSplitPaneFfsCBottom = null;

    private JPanel jPanelFfsCBottomTop = null;

    private JPanel jPanelFfsCBottomBottom = null;

    private JPanel jPanelSectionN = null;

    private JPanel jPanelSectionsN = null;

    private JPanel jPanelSubSectionN = null;

    private JPanel jPanelOptionsContainer = null;

    private JPanel jPanelUserDefCenterN = null;

    private JPanel jPanelTableOptionsContainer = null;

    private JLabel jLabelTableOptionsTitle = null;
    
    private final int buildTargetWidth = 150;
    private final int toolChainFamilyWidth = 150;
    private final int supportArchWidth = 150;
    private final int toolCmdCodeWidth = 200;
    private final int tagNameWidth = 150;
    private final int argWidth = 400;
    
    private boolean ffsSelection = false;
    private int selectedFfsTableRow = -1;

    /**
     * This method initializes jPanel	
     * 	
     * @return javax.swing.JPanel	
     private JPanel getJPanelContentSouth() {
     if (jPanelContentSouth == null) {
     jPanelContentSouth = new JPanel();
     }
     return jPanelContentSouth;
     }
     */

    /**
     * This method initializes jPanel1	
     * 	
     * @return javax.swing.JPanel	
     private JPanel getJPanelContentNorth() {
     if (jPanelContentNorth == null) {
     jPanelContentNorth = new JPanel();
     }
     return jPanelContentNorth;
     }
     */

    /**
     * This method initializes jPanel2	
     * 	
     * @return javax.swing.JPanel	
     private JPanel getJPanelContentWest() {
     if (jPanelContentWest == null) {
     jPanelContentWest = new JPanel();
     }
     return jPanelContentWest;
     }
     */

    /**
     * This method initializes jPanel3	
     * 	
     * @return javax.swing.JPanel	
     private JPanel getJPanelContentEast() {
     if (jPanelContentEast == null) {
     jPanelContentEast = new JPanel();
     }
     return jPanelContentEast;
     }
     */

    /**
     * This method initializes jTabbedPane	
     * 	
     * @return javax.swing.JTabbedPane	
     */
    private JTabbedPane getJTabbedPane() {
        if (jTabbedPane == null) {
            jTabbedPane = new JTabbedPane();
            jTabbedPane.addTab("Flash Filesystem Options", null, getJPanelFfsTab(), null);
            jTabbedPane.addTab("Customize Tool Chain Configurations", null, getJPanelOptionsTab(), null);
            jTabbedPane.addTab("User Defined ANT Tasks", null, getJPanelUserDef(), null);
        }
        return jTabbedPane;
    }

    /**
     * This method initializes this
     * 
     * @return void
     */
    private void initialize() {
        this.setSize(722, 577);
        this.setTitle("Platform Build Options");
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
            //            jContentPane.add(getJPanelContentSouth(), java.awt.BorderLayout.SOUTH);
            //            jContentPane.add(getJPanelContentNorth(), java.awt.BorderLayout.NORTH);
            //            jContentPane.add(getJPanelContentWest(), java.awt.BorderLayout.WEST);
            //            jContentPane.add(getJPanelContentEast(), java.awt.BorderLayout.EAST);
            jContentPane.add(getJTabbedPane(), java.awt.BorderLayout.CENTER);
        }
        return jContentPane;
    }

    /**
     * This method initializes jPanelTableOptionsContainer	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelTableOptionsContainer() {
        if (jPanelTableOptionsContainer == null) {
            jLabelTableOptionsTitle = new JLabel();
            jLabelTableOptionsTitle.setText("  Current Argument Lines");
            jPanelTableOptionsContainer = new JPanel();
            jPanelTableOptionsContainer.setLayout(new BorderLayout());
            jPanelTableOptionsContainer.add(jLabelTableOptionsTitle, java.awt.BorderLayout.NORTH);
            jPanelTableOptionsContainer.add(getJScrollPaneOptions(), java.awt.BorderLayout.CENTER);
        }
        return jPanelTableOptionsContainer;
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
        this.addInternalFrameListener(new InternalFrameAdapter() {
            public void internalFrameDeactivated(InternalFrameEvent e) {
                if (jTableAntTasks.isEditing()) {
                    jTableAntTasks.getCellEditor().stopCellEditing();
                }
                if (jTableOptions.isEditing()) {
                    jTableOptions.getCellEditor().stopCellEditing();
                }
                stopEditingInTables ();
            }
        });
    }
    
    private void stopEditingInTables () {
        if (jTableFfs.isEditing()) {
            jTableFfs.getCellEditor().stopCellEditing();
        }
        if (jTableFfsSection.isEditing()) {
            jTableFfsSection.getCellEditor().stopCellEditing();
        }
        if (jTableFfsSections.isEditing()) {
            jTableFfsSections.getCellEditor().stopCellEditing();
        }
        if (jTableFfsSubSection.isEditing()) {
            jTableFfsSubSection.getCellEditor().stopCellEditing();
        }
        if (jTableFfsAttribs.isEditing()) {
            jTableFfsAttribs.getCellEditor().stopCellEditing();
        }
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
            jPanelFfsTabCenter.add(getJSplitPaneFfsC(), java.awt.BorderLayout.CENTER);
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
            
            jPanelFfsTabCenterN = new JPanel();
            jPanelFfsTabCenterN.setLayout(flowLayout5);
            
            
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
            jPanelFfsTabCenterS = new JPanel();
            jPanelFfsTabCenterS.setPreferredSize(new java.awt.Dimension(491, 130));
            jPanelFfsTabCenterS.setLayout(new BorderLayout());
            jPanelFfsTabCenterS.add(jLabelFfsAttribs, java.awt.BorderLayout.WEST);
            jPanelFfsTabCenterS.add(getJScrollPaneFfsAttribs(), java.awt.BorderLayout.CENTER);
            jPanelFfsTabCenterS.add(getJPanelFfsAttribButtonGroup(), java.awt.BorderLayout.EAST);
        }
        return jPanelFfsTabCenterS;
    }

    /**
     * This method initializes jTextField6	
     * 	
     * @return javax.swing.JTextField	
     */
    

    /**
     * This method initializes jButton8	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonFfsAdd() {
        if (jButtonFfsAdd == null) {
            jButtonFfsAdd = new JButton();
            jButtonFfsAdd.setPreferredSize(new java.awt.Dimension(70, 20));
            jButtonFfsAdd.setText("New");
            jButtonFfsAdd.addActionListener(new AbstractAction() {
                /**
                 * 
                 */
                private static final long serialVersionUID = -2923720717273384221L;

                public void actionPerformed(java.awt.event.ActionEvent e) {
                    
                        String[] row = { "" };
                        ffsTableModel.addRow(row);
                        docConsole.setSaved(false);
                        ffc.genBuildOptionsFfs("", "");
                        jTableFfs.changeSelection(ffsTableModel.getRowCount()-1, 0, false, false);
                    
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
            jButtonFfsDel.setPreferredSize(new java.awt.Dimension(70, 20));
            jButtonFfsDel.setText("Delete");
            jButtonFfsDel.addActionListener(new AbstractAction() {
                /**
                 * 
                 */
                private static final long serialVersionUID = -4002678939178194476L;

                public void actionPerformed(ActionEvent arg0) {
                    if (jTableFfs.getSelectedRow() < 0) {
                        return;
                    }
                    stopEditingInTables();
                    docConsole.setSaved(false);
                    ffc.removeBuildOptionsFfs(jTableFfs.getSelectedRow());
                    ffsTableModel.removeRow(jTableFfs.getSelectedRow());
                    sectionTableModel.setRowCount(0);
                    sectionsTableModel.setRowCount(0);
                    subsectionsTableModel.setRowCount(0);
                    ffsAttributesTableModel.setRowCount(0);
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
//            jScrollPaneFfsAttribs.setPreferredSize(new java.awt.Dimension(350, 100));
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
//            jTableFfsAttribs.setPreferredSize(new java.awt.Dimension(400, 80));
            jTableFfsAttribs.setRowHeight(20);
            ffsAttributesTableModel.addColumn("Name");
            ffsAttributesTableModel.addColumn("Value");

            jTableFfsAttribs.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
            jTableFfsAttribs.getModel().addTableModelListener(new TableModelListener() {
                public void tableChanged(TableModelEvent arg0) {
                    // TODO Auto-generated method stub
                    int row = arg0.getFirstRow();
                    TableModel m = (TableModel) arg0.getSource();
                    if (arg0.getType() == TableModelEvent.UPDATE) {
                        //ToDo Data Validition check.
                        String name = m.getValueAt(row, 0) + "";
                        String value = m.getValueAt(row, 1) + "";

                        if (name.length() == 0) {
                            return;
                        }
                        if (value.length() == 0) {
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

    private void initFfsTable() {
        int ffsCount = ffc.getBuildOptionsFfsCount();
        if (ffsCount < 0) {
            return;
        }
        String[][] saa = new String[ffsCount][1];
        ffc.getBuildOptionsFfsKey(saa);
        for (int i = 0; i < saa.length; ++i) {
            ffsTableModel.addRow(saa[i]);
        }
        jTableFfs.changeSelection(0, 0, false, false);
    }

    /**
     * This method initializes jButton17    
     *  
     * @return javax.swing.JButton  
     */
    private JButton getJButtonFfsAttribNew() {
        if (jButtonFfsAttribNew == null) {
            jButtonFfsAttribNew = new JButton();
            jButtonFfsAttribNew.setPreferredSize(new java.awt.Dimension(80, 20));
            jButtonFfsAttribNew.setText("New");
            jButtonFfsAttribNew.addActionListener(new AbstractAction() {
                /**
                 * 
                 */
                private static final long serialVersionUID = 1L;

                public void actionPerformed(ActionEvent arg0) {
                    if (jTableFfs.getSelectedRow() < 0) {
                        return;
                    }
                    Object[] o = { "", "" };
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
            jButtonFfsAttribRemove.setPreferredSize(new java.awt.Dimension(80, 20));
            jButtonFfsAttribRemove.setText("Remove");
            jButtonFfsAttribRemove.addActionListener(new AbstractAction() {
                /**
                 * 
                 */
                private static final long serialVersionUID = 1L;

                public void actionPerformed(ActionEvent arg0) {
                    if (jTableFfs.getSelectedRow() < 0) {
                        return;
                    }
                    stopEditingInTables();
                    if (jTableFfsAttribs.getSelectedRow() >= 0) {
                        docConsole.setSaved(false);
                        ffsAttributesTableModel.removeRow(jTableFfsAttribs.getSelectedRow());
                        ffc.removeBuildOptionsFfsAttribute(jTableFfs.getSelectedRow(),
                                                           jTableFfsAttribs.getSelectedRow());
                    }
                }
            });
        }
        return jButtonFfsAttribRemove;
    }

    /**
     * This method initializes jScrollPane  
     *  
     * @return javax.swing.JScrollPane  
     */
    private JScrollPane getJScrollPaneFfs() {
        if (jScrollPaneFfs == null) {
            jScrollPaneFfs = new JScrollPane();
            jScrollPaneFfs.setPreferredSize(new java.awt.Dimension(200,419));
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
            jTableFfs.getSelectionModel().addListSelectionListener(new ListSelectionListener() {
                public void valueChanged(ListSelectionEvent e) {

                    if (e.getValueIsAdjusting()) {
                        return;
                    }
                    ListSelectionModel lsm = (ListSelectionModel) e.getSource();
                    if (lsm.isSelectionEmpty()) {
                        return;
                    } else {
                        int row = lsm.getMinSelectionIndex();
                        sectionTableModel.setRowCount(0);
                        sectionsTableModel.setRowCount(0);
                        subsectionsTableModel.setRowCount(0);
                        ffsAttributesTableModel.setRowCount(0);
                        String[] sArray = { "", "" };
                        LinkedHashMap<String, String> lhm = new LinkedHashMap<String, String>();
                        ArrayList<String> alSections = new ArrayList<String>();
                        ArrayList<String> alSection = new ArrayList<String>();
                        ffc.getBuildOptionsFfs(row, sArray, lhm, alSections, alSection);
                        ffsSelection = true;
                        jTextFieldEncapType.setText(sArray[1]);
                        ffsSelection = false;
                        for (int i = 0; i < alSection.size(); ++i) {
                            String[] sectionRow = { alSection.get(i) };
                            sectionTableModel.addRow(sectionRow);
                        }
                        for (int j = 0; j < alSections.size(); ++j) {
                            String[] sectionsRow = { alSections.get(j) };
                            sectionsTableModel.addRow(sectionsRow);
                        }
                        if (lhm.size() <= 0) {
                            return;
                        }
                        Set<String> keySet = lhm.keySet();
                        Iterator<String> is = keySet.iterator();
                        while (is.hasNext()) {
                            String key = is.next();
                            String[] attribRow = { key, lhm.get(key) };
                            ffsAttributesTableModel.addRow(attribRow);
                        }
                        selectedFfsTableRow = row;
                    }
                }
            });

            jTableFfs.getModel().addTableModelListener(new TableModelListener() {
                public void tableChanged(TableModelEvent arg0) {
                    // TODO Auto-generated method stub
                    int row = arg0.getFirstRow();
                    TableModel m = (TableModel) arg0.getSource();
                    if (arg0.getType() == TableModelEvent.UPDATE) {
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
//            jScrollPaneFfsSection.setPreferredSize(new java.awt.Dimension(500, 80));
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
                    TableModel m = (TableModel) arg0.getSource();
                    if (arg0.getType() == TableModelEvent.UPDATE) {
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
//            jScrollPaneFfsSubSection.setPreferredSize(new java.awt.Dimension(500, 90));
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
                    TableModel m = (TableModel) arg0.getSource();
                    if (arg0.getType() == TableModelEvent.UPDATE) {
                        //ToDo Data Validition check.
                        String type = m.getValueAt(row, 0) + "";
                        docConsole.setSaved(false);
                        ffc.updateBuildOptionsFfsSectionsSectionsSection(jTableFfs.getSelectedRow(),
                                                                         jTableFfsSections.getSelectedRow(), row, type);
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
            jTextFieldEncapType.setPreferredSize(new java.awt.Dimension(200,20));
            jTextFieldEncapType.getDocument().addDocumentListener(new DocumentListener() {

                public void insertUpdate(DocumentEvent arg0) {
                    if (ffsSelection) {
//                        ffsSelection = false;
                        return;
                    }
                    if (docConsole != null) {
                        docConsole.setSaved(false);
                    }
                }

                public void removeUpdate(DocumentEvent arg0) {
                    if (ffsSelection) {
//                        ffsSelection = false;
                        return;
                    }
                    if (docConsole != null) {
                        docConsole.setSaved(false);
                    }
                }

                public void changedUpdate(DocumentEvent arg0) {
                    // TODO Auto-generated method stub
                    
                }
                
            });
            jTextFieldEncapType.addFocusListener(new java.awt.event.FocusAdapter() {
                public void focusLost(java.awt.event.FocusEvent e) {
                    if (selectedFfsTableRow < 0) {
                        return;
                    }
                    ffc.updateBuildOptionsFfsSectionsType(selectedFfsTableRow, jTextFieldEncapType.getText());
                    
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
            jPanelFfsAttribButtonGroup.setPreferredSize(new java.awt.Dimension(100, 100));
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
            jButtonFfsSectionNew.setPreferredSize(new java.awt.Dimension(80, 20));
            jButtonFfsSectionNew.setText("New");
            jButtonFfsSectionNew.addActionListener(new java.awt.event.ActionListener() {
                public void actionPerformed(java.awt.event.ActionEvent e) {
                    if (jTableFfs.getSelectedRow() < 0) {
                        return;
                    }
                    docConsole.setSaved(false);
                    String[] row = { "EFI_SECTION_RAW" };
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
            jButtonFfsSectionRemove.setPreferredSize(new java.awt.Dimension(80, 20));
            jButtonFfsSectionRemove.setText("Remove");

            jButtonFfsSectionRemove.addActionListener(new java.awt.event.ActionListener() {
                public void actionPerformed(java.awt.event.ActionEvent e) {
                    if (jTableFfs.getSelectedRow() < 0 || jTableFfsSection.getSelectedRow() < 0) {
                        return;
                    }
                    stopEditingInTables();
                    docConsole.setSaved(false);
                    sectionTableModel.removeRow(jTableFfsSection.getSelectedRow());
                    ffc.removeBuildOptionsFfsSectionsSection(jTableFfs.getSelectedRow(),
                                                             jTableFfsSection.getSelectedRow());
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
            jButtonFfsSubSectionNew.setPreferredSize(new java.awt.Dimension(80, 20));
            jButtonFfsSubSectionNew.setText("New");
            jButtonFfsSubSectionNew.addActionListener(new java.awt.event.ActionListener() {
                public void actionPerformed(java.awt.event.ActionEvent e) {
                    if (jTableFfs.getSelectedRow() < 0 || jTableFfsSections.getSelectedRow() < 0) {
                        return;
                    }
                    docConsole.setSaved(false);
                    String[] row = { "EFI_SECTION_RAW" };
                    subsectionsTableModel.addRow(row);
                    ffc.genBuildOptionsFfsSectionsSectionsSection(jTableFfs.getSelectedRow(),
                                                                  jTableFfsSections.getSelectedRow(), row[0]);

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
            jButtonFfsSubSectionRemove.setPreferredSize(new java.awt.Dimension(80, 20));
            jButtonFfsSubSectionRemove.setText("Remove");
            jButtonFfsSubSectionRemove.addActionListener(new java.awt.event.ActionListener() {
                public void actionPerformed(java.awt.event.ActionEvent e) {
                    int selectedFfsRow = jTableFfs.getSelectedRow();
                    int selectedSectionsRow = jTableFfsSections.getSelectedRow();
                    int selectedSubSectionRow = jTableFfsSubSection.getSelectedRow();
                    if (selectedFfsRow < 0 || selectedSectionsRow < 0
                        || selectedSubSectionRow < 0) {
                        return;
                    }
                    stopEditingInTables();
                    docConsole.setSaved(false);
                    subsectionsTableModel.removeRow(selectedSubSectionRow);
                    ffc.removeBuildOptionsFfsSectionsSectionsSection(selectedFfsRow,
                                                                     selectedSectionsRow,
                                                                     selectedSubSectionRow);
                    if (subsectionsTableModel.getRowCount() == 0) {
                        sectionsTableModel.removeRow(selectedSectionsRow);
                    }
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
            jButtonFfsSectionsNew.setPreferredSize(new java.awt.Dimension(80, 20));
            jButtonFfsSectionsNew.setText("New");
            jButtonFfsSectionsNew.addActionListener(new java.awt.event.ActionListener() {
                public void actionPerformed(java.awt.event.ActionEvent e) {
                    if (jTableFfs.getSelectedRow() < 0) {
                        return;
                    }
                    docConsole.setSaved(false);
                    String[] row = { "Compress" };
                    sectionsTableModel.addRow(row);
                    ffc.genBuildOptionsFfsSectionsSections(jTableFfs.getSelectedRow(), "");
                    JOptionPane.showMessageDialog(FpdBuildOptions.this, "Add Default Section Type EFI_SECTION_PE32 into the New Sections Entry.");
                    jTableFfsSections.changeSelection(sectionsTableModel.getRowCount()-1, 0, false, false);
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
            jButtonFfsSectionsRemove.setPreferredSize(new java.awt.Dimension(80, 20));
            jButtonFfsSectionsRemove.setText("Remove");
            jButtonFfsSectionsRemove.addActionListener(new java.awt.event.ActionListener() {
                public void actionPerformed(java.awt.event.ActionEvent e) {
                    if (jTableFfs.getSelectedRow() < 0 || jTableFfsSections.getSelectedRow() < 0) {
                        return;
                    }
                    stopEditingInTables();
                    docConsole.setSaved(false);
                    sectionsTableModel.removeRow(jTableFfsSections.getSelectedRow());
                    ffc.removeBuildOptionsFfsSectionsSections(jTableFfs.getSelectedRow(),
                                                              jTableFfsSections.getSelectedRow());
                    subsectionsTableModel.setRowCount(0);
                }
            });
        }
        return jButtonFfsSectionsRemove;
    }

    private JScrollPane getJScrollPaneFfsSections() {
        if (jScrollPaneFfsSections == null) {
            jScrollPaneFfsSections = new JScrollPane();
//            jScrollPaneFfsSections.setPreferredSize(new java.awt.Dimension(500, 80));
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
            jTableFfsSections.getSelectionModel().addListSelectionListener(new ListSelectionListener() {
                public void valueChanged(ListSelectionEvent e) {
                    if (e.getValueIsAdjusting()) {
                        return;
                    }
                    ListSelectionModel lsm = (ListSelectionModel) e.getSource();
                    if (lsm.isSelectionEmpty()) {
                        return;
                    } else {
                        int sectionsRow = lsm.getMinSelectionIndex();
                        if (jTableFfs.getSelectedRow() < 0) {
                            return;
                        }
                        subsectionsTableModel.setRowCount(0);
                        ArrayList<String> al = new ArrayList<String>();
                        ffc.getBuildOptionsFfsSectionsSectionsSection(jTableFfs.getSelectedRow(), sectionsRow, al);
                        for (int i = 0; i < al.size(); ++i) {
                            String[] subsectionRow = { al.get(i) };
                            subsectionsTableModel.addRow(subsectionRow);
                        }
                    }
                }
            });

            jTableFfsSections.getModel().addTableModelListener(new TableModelListener() {
                public void tableChanged(TableModelEvent arg0) {
                    // TODO Auto-generated method stub
                    int row = arg0.getFirstRow();
                    TableModel m = (TableModel) arg0.getSource();
                    if (arg0.getType() == TableModelEvent.UPDATE) {
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
     * This method initializes jSplitPaneFfsC   
     *  
     * @return javax.swing.JSplitPane   
     */
    private JSplitPane getJSplitPaneFfsC() {
        if (jSplitPaneFfsC == null) {
            jSplitPaneFfsC = new JSplitPane();
            jSplitPaneFfsC.setOrientation(javax.swing.JSplitPane.VERTICAL_SPLIT);
            jSplitPaneFfsC.setDividerLocation(130);
            jSplitPaneFfsC.setTopComponent(getJPanelFfsCTop());
            jSplitPaneFfsC.setBottomComponent(getJSplitPaneFfsCBottom());
            jSplitPaneFfsC.setDividerSize(5);
        }
        return jSplitPaneFfsC;
    }

    /**
     * This method initializes jPanelFfsCTop    
     *  
     * @return javax.swing.JPanel   
     */
    private JPanel getJPanelFfsCTop() {
        if (jPanelFfsCTop == null) {
            jPanelFfsCTop = new JPanel();
            jPanelFfsCTop.setLayout(new BorderLayout());
            jPanelFfsCTop.add(getJPanelSectionN(), java.awt.BorderLayout.NORTH);
            jPanelFfsCTop.add(getJScrollPaneFfsSection(), java.awt.BorderLayout.CENTER);
        }
        return jPanelFfsCTop;
    }

    /**
     * This method initializes jSplitPaneFfsCBottom 
     *  
     * @return javax.swing.JSplitPane   
     */
    private JSplitPane getJSplitPaneFfsCBottom() {
        if (jSplitPaneFfsCBottom == null) {
            jSplitPaneFfsCBottom = new JSplitPane();
            jSplitPaneFfsCBottom.setDividerSize(5);
            jSplitPaneFfsCBottom.setDividerLocation(130);
            jSplitPaneFfsCBottom.setTopComponent(getJPanelFfsCBottomTop());
            jSplitPaneFfsCBottom.setBottomComponent(getJPanelFfsCBottomBottom());
            jSplitPaneFfsCBottom.setOrientation(javax.swing.JSplitPane.VERTICAL_SPLIT);
        }
        return jSplitPaneFfsCBottom;
    }

    /**
     * This method initializes jPanelFfsCBottomTop  
     *  
     * @return javax.swing.JPanel   
     */
    private JPanel getJPanelFfsCBottomTop() {
        if (jPanelFfsCBottomTop == null) {
            jPanelFfsCBottomTop = new JPanel();
            jPanelFfsCBottomTop.setLayout(new BorderLayout());
            jPanelFfsCBottomTop.add(getJPanelSectionsN(), java.awt.BorderLayout.NORTH);
            jPanelFfsCBottomTop.add(getJScrollPaneFfsSections(), java.awt.BorderLayout.CENTER);
        }
        return jPanelFfsCBottomTop;
    }

    /**
     * This method initializes jPanelFfsCBottomBottom   
     *  
     * @return javax.swing.JPanel   
     */
    private JPanel getJPanelFfsCBottomBottom() {
        if (jPanelFfsCBottomBottom == null) {
            jPanelFfsCBottomBottom = new JPanel();
            jPanelFfsCBottomBottom.setLayout(new BorderLayout());
            jPanelFfsCBottomBottom.add(getJPanelSubSectionN(), java.awt.BorderLayout.NORTH);
            jPanelFfsCBottomBottom.add(getJScrollPaneFfsSubSection(), java.awt.BorderLayout.CENTER);
        }
        return jPanelFfsCBottomBottom;
    }

    /**
     * This method initializes jPanelSectionN   
     *  
     * @return javax.swing.JPanel   
     */
    private JPanel getJPanelSectionN() {
        if (jPanelSectionN == null) {
            jPanelSectionN = new JPanel();
            jLabelFfsSection = new JLabel();
            jLabelFfsSection.setText("Section");
            jPanelSectionN.add(jLabelFfsSection, null);
            jPanelSectionN.add(getJButtonFfsSectionNew(), null);
            jPanelSectionN.add(getJButtonFfsSectionRemove(), null);
        }
        return jPanelSectionN;
    }

    /**
     * This method initializes jPanelSectionsN  
     *  
     * @return javax.swing.JPanel   
     */
    private JPanel getJPanelSectionsN() {
        if (jPanelSectionsN == null) {
            jPanelSectionsN = new JPanel();
            jLabelFfsSections = new JLabel();
            jLabelFfsSections.setText("Sections");
            jPanelSectionsN.add(jLabelFfsSections, null);
            jPanelSectionsN.add(getJButtonFfsSectionsNew(), null);
            jPanelSectionsN.add(getJButtonFfsSectionsRemove(), null);
        }
        return jPanelSectionsN;
    }

    /**
     * This method initializes jPanelSubSectionN    
     *  
     * @return javax.swing.JPanel   
     */
    private JPanel getJPanelSubSectionN() {
        if (jPanelSubSectionN == null) {
            jPanelSubSectionN = new JPanel();
            jLabelFfsSubSections = new JLabel();
            jLabelFfsSubSections.setText("Sub-Sections");
            jPanelSubSectionN.add(jLabelFfsSubSections, null);
            jPanelSubSectionN.add(getJButtonFfsSubSectionNew(), null);
            jPanelSubSectionN.add(getJButtonFfsSubSectionRemove(), null);
        }
        return jPanelSubSectionN;
    }

    /**
     * The following section contains all Build Options content
     */

    /**
     * This method initializes jPanelOptionsTab
     * 	
     * This is the main Options screen
     * 
     * @return javax.swing.JPanel	jPanelOptionsTab
     */
    private JPanel getJPanelOptionsTab() {
        if (jPanelOptionsTab == null) {
            // This container holds the Options Tab content

            //            FlowLayout flowLayout9 = new FlowLayout();
            //            flowLayout9.setAlignment(java.awt.FlowLayout.LEFT);

            GridLayout gridLayout = new GridLayout();
            gridLayout.setRows(2);
            jPanelOptionsTab = new JPanel();
            jPanelOptionsTab.setLayout(gridLayout);
            jPanelOptionsTab.setBounds(new java.awt.Rectangle(0, 0, dialogWidth * 2, dialogHeight * 3));
            jPanelOptionsTab.setPreferredSize(new java.awt.Dimension(dialogWidth + 10, (dialogHeight * 3) + 10));
            jPanelOptionsTab.setAutoscrolls(true);
            jPanelOptionsTab.setLocation(0, 0);
            jPanelOptionsTab.add(getJPanelOptionsContainer(), null);
//            jPanelOptionsTab.add(getJScrollPaneOptions(), null);
            jPanelOptionsTab.add(getJPanelTableOptionsContainer(), null);
        }
        return jPanelOptionsTab;
    }

    /**
     * This method initializes jPanelOptionsContainer   
     *  
     * @return javax.swing.JPanel   
     */
    private JPanel getJPanelOptionsContainer() {
        if (jPanelOptionsContainer == null) {
            jLabelTagName = new JLabel();
            jLabelTagName.setBounds(new java.awt.Rectangle(labelColumn, rowOne, labelWidth, oneRowHeight));
            jLabelTagName.setLocation(new java.awt.Point(labelColumn, rowOne));
            jLabelTagName.setText("Tag Name");
            jLabelBuildTargets = new JLabel();
            jLabelBuildTargets.setBounds(new java.awt.Rectangle(labelColumn, rowTwo, labelWidth, oneRowHeight));
            jLabelBuildTargets.setLocation(new java.awt.Point(labelColumn, rowTwo));
            jLabelBuildTargets.setText("Build Targets");
            jLabelToolCmd = new JLabel();
            jLabelToolCmd.setBounds(new java.awt.Rectangle(labelColumn, rowThree, labelWidth, oneRowHeight));
            jLabelToolCmd.setLocation(new java.awt.Point(labelColumn, rowThree));
            jLabelToolCmd.setText("Tool Command");
            jLabelSupArch = new JLabel();
            jLabelSupArch.setBounds(new java.awt.Rectangle(labelColumn, rowFour, labelWidth, oneRowHeight));
            jLabelSupArch.setLocation(new java.awt.Point(labelColumn, rowFour));
            jLabelSupArch.setText("Supported Architectures");
            jLabelToolChainFamily = new JLabel();
            jLabelToolChainFamily.setBounds(new java.awt.Rectangle(labelColumn, rowFive, labelWidth, oneRowHeight));
            jLabelToolChainFamily.setLocation(new java.awt.Point(labelColumn, rowFive));
            jLabelToolChainFamily.setText("Tool Chain Family");

            jLabelOptionContents = new JLabel();
            jLabelOptionContents.setBounds(new java.awt.Rectangle(labelColumn, rowSix, labelWidth, oneRowHeight));
            jLabelOptionContents.setLocation(new java.awt.Point(labelColumn, rowSix));
            jLabelOptionContents.setText("Argument Strings");

            jPanelOptionsContainer = new JPanel();

            jPanelOptionsContainer.setLayout(null);
            
            jPanelOptionsContainer.setPreferredSize(new java.awt.Dimension(dialogWidth, dialogHeight));
            /*
            jPanelOptionsContainer
                                  .setBorder(javax.swing.BorderFactory
                                                                      .createEtchedBorder(javax.swing.border.EtchedBorder.RAISED));
*/
            jPanelOptionsContainer.add(jLabelTagName, null);
            jPanelOptionsContainer.add(getJTextFieldTagName(), null);

            jPanelOptionsContainer.add(jLabelBuildTargets, null);
            jPanelOptionsContainer.add(getJTextFieldBuildTargets(), null);

            jPanelOptionsContainer.add(jLabelToolChainFamily, null);
            jPanelOptionsContainer.add(getJTextFieldToolChainFamily(), null);

            jPanelOptionsContainer.add(jLabelToolCmd, null);
            jPanelOptionsContainer.add(getJTextFieldToolCmd(), null);

            jPanelOptionsContainer.add(jLabelSupArch, null);
            jPanelOptionsContainer.add(getArchitectureSelections(), null);

            jPanelOptionsContainer.add(jLabelOptionContents, null);
            jPanelOptionsContainer.add(getJTextFieldOptionContents(), null);

            jPanelOptionsContainer.add(getJButtonOptionsAdd(), null);
            jPanelOptionsContainer.add(getJButtonOptionsDel(), null);
        }
        return jPanelOptionsContainer;
    }

    /**
     * This method initializes jTextFieldOptionTagName  Row 1
     *  
     * @return javax.swing.JTextField   
     */
    private JTextField getJTextFieldTagName() {
        if (jTextFieldTagName == null) {
            jTextFieldTagName = new JTextField();
            jTextFieldTagName.setBounds(new java.awt.Rectangle(fieldColumn, rowOne, fieldWidth, oneRowHeight));
            jTextFieldTagName.setPreferredSize(new java.awt.Dimension(fieldWidth, oneRowHeight));
            jTextFieldTagName.setLocation(new java.awt.Point(fieldColumn, rowOne));
        }
        return jTextFieldTagName;
    }

    /**
     * This method initializes jTextFieldBuildTargets  Row 2 
     *  
     * @return javax.swing.JTextField   jTextFieldBuildTargets
     */
    private JTextField getJTextFieldBuildTargets() {
        if (jTextFieldBuildTargets == null) {
            jTextFieldBuildTargets = new JTextField();
            jTextFieldBuildTargets.setBounds(new java.awt.Rectangle(fieldColumn, rowTwo, fieldWidth, oneRowHeight));
            jTextFieldBuildTargets.setPreferredSize(new java.awt.Dimension(fieldWidth, oneRowHeight));
            jTextFieldBuildTargets.setLocation(new java.awt.Point(fieldColumn, rowTwo));
        }
        return jTextFieldBuildTargets;
    }

    /**
     * This method initializes jTextFieldToolCmd Row 3    
     * 
     *  This should be a dropdown box of command codes from tools_def.txt
     *      
     * @return javax.swing.JTextField   jTextFieldToolCmd
     */
    private JTextField getJTextFieldToolCmd() {
        if (jTextFieldToolCmd == null) {
            jTextFieldToolCmd = new JTextField();
            jTextFieldToolCmd.setBounds(new java.awt.Rectangle(fieldColumn, rowThree, fieldWidth, oneRowHeight));
            jTextFieldToolCmd.setPreferredSize(new java.awt.Dimension(fieldWidth, oneRowHeight));
            jTextFieldToolCmd.setLocation(new java.awt.Point(fieldColumn, rowThree));
        }
        return jTextFieldToolCmd;
    }

    /**
     * This method initializes jArchitectureSelections Row 4
     * 
     * @return jArchitectureSelections
     */
    private JPanel getArchitectureSelections() {
        if (jArchitectureSelections == null) {
            jArchitectureSelections = new JPanel();
            jArchitectureSelections.setLayout(null);
            jArchitectureSelections.add(getJCheckBoxIA32(), null);
            jArchitectureSelections.add(getJCheckBoxX64(), null);
            jArchitectureSelections.add(getJCheckBoxIpf(), null);
            jArchitectureSelections.add(getJCheckBoxEBC(), null);
            jArchitectureSelections.add(getJCheckBoxARM(), null);
            jArchitectureSelections.add(getJCheckBoxPPC(), null);
            jArchitectureSelections.setBounds(new java.awt.Rectangle(fieldColumn, rowFour, fieldWidth, oneRowHeight));
            jArchitectureSelections.setPreferredSize(new java.awt.Dimension(fieldWidth, oneRowHeight));
            jArchitectureSelections.setLocation(new java.awt.Point(fieldColumn, rowFour));
        }
        return jArchitectureSelections;
    }

    /**
     * This method initializes jCheckBoxIA32  
     *  
     * @return javax.swing.JCheckBox    jCheckBoxIA32
     */
    private JCheckBox getJCheckBoxIA32() {
        if (jCheckBoxIA32 == null) {
            jCheckBoxIA32 = new JCheckBox();
            jCheckBoxIA32.setBounds(new java.awt.Rectangle(0, 0, 55, 20));
            jCheckBoxIA32.setText("IA32");
        }
        return jCheckBoxIA32;
    }

    /**
     * This method initializes jCheckBoxX64 
     *  
     * @return javax.swing.JCheckBox    jCheckBoxX64
     */
    private JCheckBox getJCheckBoxX64() {
        if (jCheckBoxX64 == null) {
            jCheckBoxX64 = new JCheckBox();
            jCheckBoxX64.setText("X64");
            jCheckBoxX64.setBounds(new java.awt.Rectangle(55, 0, 53, 20));
        }
        return jCheckBoxX64;
    }

    /**
     * This method initializes jCheckBoxIpf
     *  
     * @return javax.swing.JCheckBox    jCheckBoxIpf
     */
    private JCheckBox getJCheckBoxIpf() {
        if (jCheckBoxIpf == null) {
            jCheckBoxIpf = new JCheckBox();
            jCheckBoxIpf.setBounds(new java.awt.Rectangle(108, 0, 52, 20));
            jCheckBoxIpf.setText("IPF");
        }
        return jCheckBoxIpf;
    }


    /**
     * This method initializes jCheckBoxEBC
     *  
     * @return javax.swing.JCheckBox    jCheckBoxEBC
     */
    private JCheckBox getJCheckBoxEBC() {
        if (jCheckBoxEBC == null) {
            jCheckBoxEBC = new JCheckBox();
            jCheckBoxEBC.setBounds(new java.awt.Rectangle(160, 0, 53, 20));
            jCheckBoxEBC.setText("EBC");
        }
        return jCheckBoxEBC;
    }

    /**
     * This method initializes jCheckBoxARM
     *  
     * @return javax.swing.JCheckBox    jCheckBoxARM
     */
    private JCheckBox getJCheckBoxARM() {
        if (jCheckBoxARM == null) {
            jCheckBoxARM = new JCheckBox();
            jCheckBoxARM.setBounds(new java.awt.Rectangle(213, 0, 54, 20));
            jCheckBoxARM.setText("ARM");
        }
        return jCheckBoxARM;
    }

    /**
     * This method initializes jCheckBoxPPC 
     *  
     * @return javax.swing.JCheckBox    jCheckBoxPPC
     */
    private JCheckBox getJCheckBoxPPC() {
        if (jCheckBoxPPC == null) {
            jCheckBoxPPC = new JCheckBox();
            jCheckBoxPPC.setBounds(new java.awt.Rectangle(267, 0, 53, 20));
            jCheckBoxPPC.setText("PPC");
        }
        return jCheckBoxPPC;
    }

    /**
     * This method initializes jTextFieldToolChainFamily  Row 5    
     *  
     * This should be a drop down for MSFT, INTEL, GCC or USER_DEFINED
     * 
     * @return javax.swing.JTextField   
     */
    private JTextField getJTextFieldToolChainFamily() {
        if (jTextFieldToolChainFamily == null) {
            jTextFieldToolChainFamily = new JTextField();
            jTextFieldToolChainFamily.setBounds(new java.awt.Rectangle(fieldColumn, rowFive, fieldWidth, oneRowHeight));
            jTextFieldToolChainFamily.setPreferredSize(new java.awt.Dimension(fieldWidth, oneRowHeight));
            jTextFieldToolChainFamily.setLocation(new java.awt.Point(fieldColumn, rowFive));
        }
        return jTextFieldToolChainFamily;
    }

    /**
     * This method initializes jTextFieldOptionContents Row 6
     * 
     * 	This is where we should put the checkbox & entry data for the command arguments
     * 
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldOptionContents() {
        if (jTextFieldOptionContents == null) {
            jTextFieldOptionContents = new JTextField();
            jTextFieldOptionContents.setPreferredSize(new java.awt.Dimension(fieldWidth, oneRowHeight));
            jTextFieldOptionContents.setBounds(fieldColumn, rowSix, fieldWidth, oneRowHeight);
            jTextFieldOptionContents.setLocation(new java.awt.Point(fieldColumn, rowSix));
        }
        return jTextFieldOptionContents;
    }

    /**
     * This method initializes jButtonOptionsAdd
     *  
     * Add entry from the top screen to the table
     * 
     * @return javax.swing.JButton  jButtonOptionsAdd
     */
    private JButton getJButtonOptionsAdd() {
        if (jButtonOptionsAdd == null) {
            jButtonOptionsAdd = new JButton();
            jButtonOptionsAdd.setText("Add");

            jButtonOptionsAdd.setPreferredSize(new java.awt.Dimension(buttonWidth, oneRowHeight));
            jButtonOptionsAdd.setBounds(new java.awt.Rectangle(firstButtonLoc, buttonRow, buttonWidth, oneRowHeight));            
            jButtonOptionsAdd.setLocation(new java.awt.Point(firstButtonLoc, buttonRow));
            jButtonOptionsAdd.addActionListener(new AbstractAction() {
                /**
                 * 
                 */
                private static final long serialVersionUID = 1L;

                public void actionPerformed(java.awt.event.ActionEvent e) {
                    boolean[] boolArray = { jCheckBoxIA32.isSelected(), jCheckBoxIpf.isSelected(),
                                           jCheckBoxX64.isSelected(), jCheckBoxEBC.isSelected(),
                                           jCheckBoxARM.isSelected(), jCheckBoxPPC.isSelected() };
                    String s = boolToList(boolArray);
                    Object[] o = { jTextFieldBuildTargets.getText(), jTextFieldToolChainFamily.getText(), s,
                                  jTextFieldToolCmd.getText(), jTextFieldTagName.getText(),
                                  jTextFieldOptionContents.getText() };
                    optionsTableModel.addRow(o);
                    docConsole.setSaved(false);
                    ffc.genBuildOptionsOpt(stringToVector(jTextFieldBuildTargets.getText().trim()),
                                           jTextFieldToolChainFamily.getText(), jTextFieldTagName.getText(),
                                           jTextFieldToolCmd.getText(), stringToVector(s.trim()),
                                           jTextFieldOptionContents.getText());
                }
            });
        }
        return jButtonOptionsAdd;
    }

    /**
     * This method initializes jButtonOptionsDel    
     *  
     * Remove a line from the table below
     * 
     * @return javax.swing.JButton  jButtonOptionsDel
     */
    private JButton getJButtonOptionsDel() {
        if (jButtonOptionsDel == null) {
            jButtonOptionsDel = new JButton();
            jButtonOptionsDel.setText("Delete");
            jButtonOptionsDel.setPreferredSize(new java.awt.Dimension(buttonWidth, oneRowHeight));
            jButtonOptionsDel.setBounds(new java.awt.Rectangle(next2LastButtonLoc, buttonRow, buttonWidth, oneRowHeight));
            jButtonOptionsDel.setLocation(new java.awt.Point(next2LastButtonLoc, buttonRow));
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

    public void componentResized(ComponentEvent arg0) {
        int intPreferredWidth = 500;
        
        Tools.resizeComponentWidth(this.jScrollPaneOptions, this.getWidth(), intPreferredWidth);
        
    }
    /**
     * This method initializes jScrollPaneOptions	
     * 	Contains the Table and is located below the data entry section
     * @return javax.swing.JScrollPane	jScrollPaneOptoins
     */
    private JScrollPane getJScrollPaneOptions() {
        if (jScrollPaneOptions == null) {
            jScrollPaneOptions = new JScrollPane();
            jScrollPaneOptions.setViewportView(getJTableOptions());
        }
        return jScrollPaneOptions;
    }

    /**
     * This method initializes jTableOptions
     * 	
     * @return javax.swing.JTable	jTableOptions
     */
    private JTable getJTableOptions() {
        if (jTableOptions == null) {
            
            optionsTableModel = new DefaultTableModel();
            jTableOptions = new JTable(optionsTableModel);
            jTableOptions.setRowHeight(20);
            optionsTableModel.addColumn("Build Targets");
            optionsTableModel.addColumn("Tool Chain Family");
            optionsTableModel.addColumn("Supported Architectures");
            optionsTableModel.addColumn("Tool Command Code");
            optionsTableModel.addColumn("TagName");
            optionsTableModel.addColumn("Arguments");
            
            jTableOptions.getColumnModel().getColumn(0).setMinWidth(buildTargetWidth);
            jTableOptions.getColumnModel().getColumn(1).setMinWidth(toolChainFamilyWidth);
            jTableOptions.getColumnModel().getColumn(2).setMinWidth(supportArchWidth);
            jTableOptions.getColumnModel().getColumn(3).setMinWidth(toolCmdCodeWidth);
            jTableOptions.getColumnModel().getColumn(4).setMinWidth(tagNameWidth);
            jTableOptions.getColumnModel().getColumn(5).setMinWidth(argWidth);

//            javax.swing.table.TableColumn toolFamilyCol = jTableOptions.getColumnModel().getColumn(1);
//            JComboBox cb = new JComboBox();
//            cb.addItem("MSFT");
//            cb.addItem("GCC");
//            cb.addItem("CYGWIN");
//            cb.addItem("INTEL");
//            cb.addItem("USER_DEFINED");
//            toolFamilyCol.setCellEditor(new DefaultCellEditor(cb));
            Vector<String> vArch = new Vector<String>();
            vArch.add("IA32");
            vArch.add("X64");
            vArch.add("IPF");
            vArch.add("EBC");
            vArch.add("ARM");
            vArch.add("PPC");
            jTableOptions.getColumnModel().getColumn(2).setCellEditor(new ListEditor(vArch, FrameworkWizardUI.getInstance()));
            
            jTableOptions.getColumnModel().getColumn(5).setCellEditor(new LongTextEditor(FrameworkWizardUI.getInstance()));
            
            jTableOptions.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
	        jTableOptions.setAutoResizeMode(javax.swing.JTable.AUTO_RESIZE_OFF);
            jTableOptions.getSelectionModel().addListSelectionListener(new ListSelectionListener() {
                public void valueChanged(ListSelectionEvent e) {
                    selectedRow = -1;
                    if (e.getValueIsAdjusting()) {
                        return;
                    }
                    ListSelectionModel lsm = (ListSelectionModel) e.getSource();
                    if (lsm.isSelectionEmpty()) {
                        return;
                    } else {
                        selectedRow = lsm.getMinSelectionIndex();
                    }
                }
            });

            jTableOptions.getModel().addTableModelListener(new TableModelListener() {
                public void tableChanged(TableModelEvent arg0) {
                    // TODO Auto-generated method stub
                    int row = arg0.getFirstRow();
                    TableModel m = (TableModel) arg0.getSource();
                    if (arg0.getType() == TableModelEvent.UPDATE) {
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

    private Vector<Object> stringToVector(String s) {
        String[] sArray = s.split(" ");
        Vector<Object> v = null;
        if (s.length() > 0 && !s.trim().equalsIgnoreCase("")) {
            v = new Vector<Object>();
            for (int i = 0; i < sArray.length; ++i) {
                v.add(sArray[i]);
            }
        }
        return v;
    }

    private String boolToList(boolean[] bool) {
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
        
        return s.trim();
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


    /**
     * Everything below should pertain to the ANT Task Tab
     */

    /**
     * This method initializes jButton12    
     *  
     * @return javax.swing.JButton  
     */
    private JButton getJButtonAntTaskFileBrowse() {
        if (jButtonAntTaskFileBrowse == null) {
            jButtonAntTaskFileBrowse = new JButton();
            jButtonAntTaskFileBrowse.setPreferredSize(new Dimension(buttonWidth, oneRowHeight));
            jButtonAntTaskFileBrowse.setText("Browse");
            jButtonAntTaskFileBrowse.addActionListener(new AbstractAction() {
                private static final long serialVersionUID = 1L;

                public void actionPerformed(ActionEvent arg0) {
                    //
                    // Select files from current workspace
                    //
                    String dirPrefix = Workspace.getCurrentWorkspace();
                    JFileChooser chooser = new JFileChooser(dirPrefix);
                    File theFile = null;
                    String headerDest = null;

                    chooser.setMultiSelectionEnabled(false);
                    chooser.setFileSelectionMode(JFileChooser.FILES_ONLY);
                    int retval = chooser.showOpenDialog(FpdBuildOptions.this);
                    if (retval == JFileChooser.APPROVE_OPTION) {

                        theFile = chooser.getSelectedFile();
                        String file = theFile.getPath();
                        if (!file.startsWith(dirPrefix)) {
                            JOptionPane.showMessageDialog(FpdBuildOptions.this, "You can only select files in current package!");
                            return;
                        }
                    } else {
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
     * This method initializes jPanelUserDefCenterN	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelUserDefCenterN() {
        if (jPanelUserDefCenterN == null) {
            jPanelUserDefCenterN = new JPanel();
            jPanelUserDefCenterN.add(jLabelAntCmdOpts, null);
            jPanelUserDefCenterN.add(getJTextFieldAntCmdOpts(), null);
            jPanelUserDefCenterN.add(getJButtonAntTaskAdd(), null);
            jPanelUserDefCenterN.add(getJButtonAntTaskDel(), null);
        }
        return jPanelUserDefCenterN;
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
            jLabelAntTaskFile.setPreferredSize(new java.awt.Dimension(80, 20));
            FlowLayout flowLayout8 = new FlowLayout();
            flowLayout8.setAlignment(java.awt.FlowLayout.CENTER);
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
            jLabelAntCmdOpts.setPreferredSize(new java.awt.Dimension(131, 20));
            jPanelUserDefCenter = new JPanel();
            jPanelUserDefCenter.setLayout(new BorderLayout());

            jPanelUserDefCenter.add(getJPanelUserDefCenterN(), java.awt.BorderLayout.NORTH);
            jPanelUserDefCenter.add(getJScrollPaneAntTasks(), java.awt.BorderLayout.CENTER);
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
            jTextFieldAntTaskFile.setPreferredSize(new java.awt.Dimension(200, 20));
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
            jTextFieldAntTaskId.setPreferredSize(new java.awt.Dimension(100, 20));
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
            jButtonAntTaskAdd.setPreferredSize(new java.awt.Dimension(90, 20));
            jButtonAntTaskAdd.setText("Add");
            jButtonAntTaskAdd.addActionListener(new ActionListener() {
                public void actionPerformed(ActionEvent e) {
                    if (!DataValidation.isInt(jTextFieldAntTaskId.getText())
                        || jTextFieldAntTaskId.getText().length() != 8) {
                        JOptionPane.showMessageDialog(FpdBuildOptions.this, "ID must be an 8-digit integer.");
                        return;
                    }
                    Object[] o = { jTextFieldAntTaskId.getText(), null, null };
                    o[1] = jTextFieldAntTaskFile.getText();
                    o[2] = jTextFieldAntCmdOpts.getText();
                    ffc.genBuildOptionsUserDefAntTask(o[0] + "", o[1] + "", o[2] + "");
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
            jButtonAntTaskDel.setPreferredSize(new java.awt.Dimension(90, 20));
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
    private JTextField getJTextFieldAntCmdOpts() {
        if (jTextFieldAntCmdOpts == null) {
            jTextFieldAntCmdOpts = new JTextField();
            jTextFieldAntCmdOpts.setPreferredSize(new java.awt.Dimension(270, 20));
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
            jScrollPaneAntTasks.setPreferredSize(new java.awt.Dimension(600, 400));
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
            jTableAntTasks.getSelectionModel().addListSelectionListener(new ListSelectionListener() {
                public void valueChanged(ListSelectionEvent e) {
                    selectedRow = -1;
                    if (e.getValueIsAdjusting()) {
                        return;
                    }
                    ListSelectionModel lsm = (ListSelectionModel) e.getSource();
                    if (lsm.isSelectionEmpty()) {
                        return;
                    } else {
                        selectedRow = lsm.getMinSelectionIndex();
                    }
                }
            });

            jTableAntTasks.getModel().addTableModelListener(new TableModelListener() {
                public void tableChanged(TableModelEvent arg0) {
                    // TODO Auto-generated method stub
                    int row = arg0.getFirstRow();
                    TableModel m = (TableModel) arg0.getSource();
                    if (arg0.getType() == TableModelEvent.UPDATE) {
                        //ToDo Data Validition check.
                        String id = m.getValueAt(row, 0) + "";
                        String file = m.getValueAt(row, 1) + "";
                        String execOrder = m.getValueAt(row, 2) + "";
                        if (id.length() == 0) {
                            return;
                        }
                        if (file.length() == 0 && execOrder.length() == 0) {
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

} //  @jve:decl-index=0:visual-constraint="10,10"
