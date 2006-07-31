/** @file
  Java class FpdPlatformDefs is GUI for Flash element operation in SPD file.
 
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

import javax.swing.JFrame;
import javax.swing.JTabbedPane;
import javax.swing.JButton;
import javax.swing.ListSelectionModel;

import org.tianocore.PlatformSurfaceAreaDocument;
import org.tianocore.frameworkwizard.common.Identifications.OpeningPlatformType;
import org.tianocore.frameworkwizard.common.ui.IInternalFrame;


import java.awt.FlowLayout;


import javax.swing.JCheckBox;
import javax.swing.JOptionPane;
import javax.swing.JTextField;
import javax.swing.JLabel;
import javax.swing.JScrollPane;
import javax.swing.JTable;
import javax.swing.event.InternalFrameAdapter;
import javax.swing.event.InternalFrameEvent;
import javax.swing.event.TableModelEvent;
import javax.swing.event.TableModelListener;
import javax.swing.table.DefaultTableModel;
import javax.swing.table.TableModel;
import javax.swing.JComboBox;
import java.awt.Dimension;
import java.util.Vector;

public class FpdPlatformDefs extends IInternalFrame {

    /**
     * 
     */
    private static final long serialVersionUID = 1L;
    static JFrame frame;
    private JPanel jContentPane = null;
    private JPanel jPanelContentEast = null;
    private JPanel jPanelContentWest = null;
    private JPanel jPanelContentNorth = null;
    private JTabbedPane jTabbedPane = null;
    private TargetTableModel buildTargetTableModel = null;
    
    private SkuInfoTableModel skuInfoTableModel = null;
    private OpeningPlatformType docConsole = null;
    private FpdFileContents ffc = null;
    private JPanel jPanelGeneralTab = null;
    private JPanel jPanelGeneralTabNorth = null;
    private JLabel jLabel = null;
    private JCheckBox jCheckBoxIa32 = null;
    private JCheckBox jCheckBoxX64 = null;
    private JCheckBox jCheckBoxIpf = null;
    private JPanel jPanelGeneralTabSouth = null;
    private JCheckBox jCheckBoxInterDir = null;
    private JComboBox jComboBoxInterDir = null;
    private JTable jTableBuildTargets = null;
    private JPanel jPanelGeneralTabCenter = null;
    private JLabel jLabelBuildTargets = null;
    private JTextField jTextFieldBuildTarget = null;
    private JButton jButtonAddBuildTarget = null;
    private JButton jButtonDelBuildTarget = null;
    private JScrollPane jScrollPaneBuildTargets = null;
    private JScrollPane jScrollPaneSkuInfo = null;
    private JTable jTableSkuInfo = null;
    private JCheckBox jCheckBoxEbc = null;
    private JCheckBox jCheckBoxArm = null;
    private JCheckBox jCheckBoxPpc = null;
    private JPanel jPanelDir = null;
    private JLabel jLabelPad = null;
    private JLabel jLabelOutputDir = null;
    private JTextField jTextFieldOutputDir = null;
    private JPanel jPanelSkuInfo = null;
    private JLabel jLabelSkuInfo = null;
    private JLabel jLabelSkuId = null;
    private JTextField jTextFieldSkuId = null;
    private JLabel jLabelSkuName = null;
    private JTextField jTextFieldSkuName = null;
    private JButton jButtonSkuAdd = null;
    private JButton jButtonSkuDel = null;
    private JLabel jLabelPadd = null;
    public FpdPlatformDefs() {
        super();
        // TODO Auto-generated constructor stub

        initialize();
        this.setBounds(new java.awt.Rectangle(0, 0, 500, 370));
        this.setVisible(true);
    }

    public FpdPlatformDefs(PlatformSurfaceAreaDocument.PlatformSurfaceArea fpd){
        this();
        ffc = new FpdFileContents(fpd);
        init(ffc);
    }
    
    public FpdPlatformDefs(OpeningPlatformType opt) {
        this(opt.getXmlFpd());
        docConsole = opt;
    }
    
    /**
     * This method initializes jPanel	
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
    private JPanel getJPanelContentNorth() {
        if (jPanelContentNorth == null) {
            jPanelContentNorth = new JPanel();
        }
        return jPanelContentNorth;
    }

    /**
     * This method initializes jTabbedPane	
     * 	
     * @return javax.swing.JTabbedPane	
     */
    private JTabbedPane getJTabbedPane() {
        if (jTabbedPane == null) {
            jTabbedPane = new JTabbedPane();
            
			jTabbedPane.addTab("General", null, getJPanelGeneralTab(), null);
           
        }
        return jTabbedPane;
    }

    /**
     * This method initializes this
     * 
     * @return void
     */
    private void initialize() {
        this.setSize(518, 650);
        this.setDefaultCloseOperation(javax.swing.WindowConstants.DISPOSE_ON_CLOSE);
        this.setContentPane(getJContentPane());
        this.setTitle("FPD Platform Definitions");
        this.addInternalFrameListener(new InternalFrameAdapter(){
            public void internalFrameDeactivated(InternalFrameEvent e){
                if (jTableBuildTargets.isEditing()) {
                    jTableBuildTargets.getCellEditor().stopCellEditing();
                }
                if (jTableSkuInfo.isEditing()) {
                    jTableSkuInfo.getCellEditor().stopCellEditing();
                }
               
            }
        });
    }

    private void init(FpdFileContents ffc) {
        Vector<Object> v = new Vector<Object>();
        ffc.getPlatformDefsSupportedArchs(v);
        showToolChain(v);
        
        buildTargetTableModel.setRowCount(0);
        v.removeAllElements();
        ffc.getPlatformDefsBuildTargets(v);
        for (int i = 0; i < v.size(); ++i){
            Object[] row = {v.get(i)};
            buildTargetTableModel.addRow(row);
        }
        
        String[][] saa = new String[ffc.getPlatformDefsSkuInfoCount()][2];
        ffc.getPlatformDefsSkuInfos(saa);
        for (int i = 0; i < saa.length; ++i) {
            skuInfoTableModel.addRow(saa[i]);
        }
        
        String interDir = ffc.getPlatformDefsInterDir();
        if (interDir != null) {
            jComboBoxInterDir.setSelectedItem(interDir);
        }
        
        String outputDir = ffc.getPlatformDefsOutputDir();
        if (outputDir != null) {
            jTextFieldOutputDir.setText(outputDir);
        }
    }
    
   private void showToolChain(Vector<Object> v) {
       if (v.contains("IA32")) {
           jCheckBoxIa32.setSelected(true);
       }
       else{
           jCheckBoxIa32.setSelected(false);
       }
       if (v.contains("X64")) {
           jCheckBoxX64.setSelected(true);
       }
       else{
           jCheckBoxX64.setSelected(false);
       }
       if (v.contains("IPF")) {
           jCheckBoxIpf.setSelected(true);
       }
       else{
           jCheckBoxIpf.setSelected(false);
       }
       if (v.contains("EBC")) {
           jCheckBoxEbc.setSelected(true);
       }
       else{
           jCheckBoxEbc.setSelected(false);
       }
       if (v.contains("ARM")) {
           jCheckBoxArm.setSelected(true);
       }
       else{
           jCheckBoxArm.setSelected(false);
       }
       if (v.contains("PPC")) {
           jCheckBoxPpc.setSelected(true);
       }
       else{
           jCheckBoxPpc.setSelected(false);
       }
   }
   
   private void getToolChain(Vector<Object> v) {
       if (docConsole != null){
           docConsole.setSaved(false);
       }
       v.removeAllElements();
       if (jCheckBoxIa32.isSelected()) {
           v.add("IA32");
       }
       if (jCheckBoxX64.isSelected()) {
           v.add("X64");
       }
       if (jCheckBoxIpf.isSelected()) {
           v.add("IPF");
       }
       if (jCheckBoxEbc.isSelected()) {
           v.add("EBC");
       }
       if (jCheckBoxArm.isSelected()) {
           v.add("ARM");
       }
       if (jCheckBoxPpc.isSelected()) {
           v.add("PPC");
       }
       
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
            jContentPane.add(getJPanelContentEast(), java.awt.BorderLayout.EAST);
            jContentPane.add(getJPanelContentWest(), java.awt.BorderLayout.WEST);
            jContentPane.add(getJPanelContentNorth(), java.awt.BorderLayout.NORTH);
            jContentPane.add(getJTabbedPane(), java.awt.BorderLayout.CENTER);
        }
        return jContentPane;
    }

    /**
     * This method initializes jPanel4	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelGeneralTab() {
        if (jPanelGeneralTab == null) {
            jPanelGeneralTab = new JPanel();
            jPanelGeneralTab.setLayout(new BorderLayout());
            jPanelGeneralTab.add(getJPanelGeneralTabNorth(), java.awt.BorderLayout.NORTH);
            jPanelGeneralTab.add(getJPanelGeneralTabSouth(), java.awt.BorderLayout.SOUTH);
            jPanelGeneralTab.add(getJPanelGeneralTabCenter(), java.awt.BorderLayout.CENTER);
        }
        return jPanelGeneralTab;
    }

    /**
     * This method initializes jPanel5	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelGeneralTabNorth() {
        if (jPanelGeneralTabNorth == null) {
            jLabel = new JLabel();
            jLabel.setText("Supported Archs");
            FlowLayout flowLayout2 = new FlowLayout();
            flowLayout2.setAlignment(FlowLayout.LEFT);
            flowLayout2.setHgap(12);
            jPanelGeneralTabNorth = new JPanel();
            jPanelGeneralTabNorth.setLayout(flowLayout2);
            jPanelGeneralTabNorth.add(jLabel, null);
            jPanelGeneralTabNorth.add(getJCheckBoxIa32(), null);
            jPanelGeneralTabNorth.add(getJCheckBoxX64(), null);
            jPanelGeneralTabNorth.add(getJCheckBoxIpf(), null);
            jPanelGeneralTabNorth.add(getJCheckBoxEbc(), null);
            jPanelGeneralTabNorth.add(getJCheckBoxArm(), null);
            jPanelGeneralTabNorth.add(getJCheckBoxPpc(), null);
        }
        return jPanelGeneralTabNorth;
    }

    /**
     * This method initializes jCheckBox1	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBoxIa32() {
        if (jCheckBoxIa32 == null) {
            jCheckBoxIa32 = new JCheckBox();
            jCheckBoxIa32.setText("IA32");
            jCheckBoxIa32.addItemListener(new java.awt.event.ItemListener() {
                public void itemStateChanged(java.awt.event.ItemEvent e) {
                    Vector<Object> v = new Vector<Object>();
                    getToolChain(v);
                    if (v.size() == 0) {
                        JOptionPane.showMessageDialog(frame, "Platform must contain at least ONE supported Arch.");
                        return;
                    }
                    ffc.setPlatformDefsSupportedArchs(v);
                }
            });
        }
        return jCheckBoxIa32;
    }

    /**
     * This method initializes jCheckBox2	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBoxX64() {
        if (jCheckBoxX64 == null) {
            jCheckBoxX64 = new JCheckBox();
            jCheckBoxX64.setText("X64");
            jCheckBoxX64.addItemListener(new java.awt.event.ItemListener() {
                public void itemStateChanged(java.awt.event.ItemEvent e) {
                    Vector<Object> v = new Vector<Object>();
                    getToolChain(v);
                    if (v.size() == 0) {
                        JOptionPane.showMessageDialog(frame, "Platform must contain at least ONE supported Arch.");
                        return;
                    }
                    ffc.setPlatformDefsSupportedArchs(v);
                }
            });
        }
        return jCheckBoxX64;
    }

    /**
     * This method initializes jCheckBox3	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBoxIpf() {
        if (jCheckBoxIpf == null) {
            jCheckBoxIpf = new JCheckBox();
            jCheckBoxIpf.setText("IPF");
            jCheckBoxIpf.addItemListener(new java.awt.event.ItemListener() {
                public void itemStateChanged(java.awt.event.ItemEvent e) {
                    Vector<Object> v = new Vector<Object>();
                    getToolChain(v);
                    if (v.size() == 0) {
                        JOptionPane.showMessageDialog(frame, "Platform must contain at least ONE supported Arch.");
                        return;
                    }
                    ffc.setPlatformDefsSupportedArchs(v);
                }
            });
        }
        return jCheckBoxIpf;
    }
    
    /**
     * This method initializes jTable   
     *  
     * @return javax.swing.JTable   
     */
    private JTable getJTableBuildTargets() {
        if (jTableBuildTargets == null) {
            buildTargetTableModel = new TargetTableModel();
            buildTargetTableModel.addColumn("Build Target");
            jTableBuildTargets = new JTable(buildTargetTableModel);
            jTableBuildTargets.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
            
            jTableBuildTargets.getModel().addTableModelListener(new TableModelListener() {
                public void tableChanged(TableModelEvent arg0) {
                    // TODO Auto-generated method stub
                    if (jTableBuildTargets.getSelectedRow() < 0) {
                        return;
                    }
                    TableModel m = (TableModel)arg0.getSource();
                    if (arg0.getType() == TableModelEvent.UPDATE){
                        //ToDo Data Validition check.
                        Vector<Object> v = new Vector<Object>();
                        for (int i = 0; i < jTableBuildTargets.getRowCount(); ++i) {
                            v.add(m.getValueAt(i, 0));
                        }
                        docConsole.setSaved(false);
                        ffc.setPlatformDefsBuildTargets(v);
                    }
                }
            });
        }
        return jTableBuildTargets;
    }

    /**
     * This method initializes jPanel6	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelGeneralTabSouth() {
        if (jPanelGeneralTabSouth == null) {
            FlowLayout flowLayout3 = new FlowLayout();
            flowLayout3.setAlignment(FlowLayout.LEFT);
            flowLayout3.setHgap(20);
            jPanelGeneralTabSouth = new JPanel();
            jPanelGeneralTabSouth.setPreferredSize(new java.awt.Dimension(10,200));
            jPanelGeneralTabSouth.setLayout(flowLayout3);
            jPanelGeneralTabSouth.add(getJPanelDir(), null);
        }
        return jPanelGeneralTabSouth;
    }

    /**
     * This method initializes jCheckBox4	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBoxInterDir() {
        if (jCheckBoxInterDir == null) {
            jCheckBoxInterDir = new JCheckBox();
            jCheckBoxInterDir.setText("Intermediate Directories");
        }
        return jCheckBoxInterDir;
    }

    /**
     * This method initializes jComboBox	
     * 	
     * @return javax.swing.JComboBox	
     */
    private JComboBox getJComboBoxInterDir() {
        if (jComboBoxInterDir == null) {
            jComboBoxInterDir = new JComboBox();
            jComboBoxInterDir.setPreferredSize(new Dimension(100, 20));
            jComboBoxInterDir.addItem("UNIFIED");
            jComboBoxInterDir.addItem("MODULE");
            
            jComboBoxInterDir.setSelectedIndex(0);
            jComboBoxInterDir.addItemListener(new java.awt.event.ItemListener() {
                public void itemStateChanged(java.awt.event.ItemEvent e) {
                    if (docConsole != null){
                        docConsole.setSaved(false);
                    }
                    ffc.setPlatformDefsInterDir(jComboBoxInterDir.getSelectedItem()+"");
                }
            });
        }
        return jComboBoxInterDir;
    }

    /**
     * This method initializes jPanel7	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelGeneralTabCenter() {
        if (jPanelGeneralTabCenter == null) {
            jLabelBuildTargets = new JLabel();
            jLabelBuildTargets.setPreferredSize(new Dimension(109, 16));
            jLabelBuildTargets.setText("Build Targets");
            FlowLayout flowLayout4 = new FlowLayout();
            flowLayout4.setAlignment(FlowLayout.LEFT);
            flowLayout4.setHgap(20);
            jPanelGeneralTabCenter = new JPanel();
            jPanelGeneralTabCenter.setPreferredSize(new Dimension(972, 100));
            jPanelGeneralTabCenter.setLayout(flowLayout4);
            jPanelGeneralTabCenter.add(jLabelBuildTargets, null);
            jPanelGeneralTabCenter.add(getJTextFieldBuildTarget(), null);
            jPanelGeneralTabCenter.add(getJButtonAddBuildTarget(), null);
            jPanelGeneralTabCenter.add(getJButtonDelBuildTarget(), null);
            jPanelGeneralTabCenter.add(getJScrollPaneBuildTargets(), null);
            jPanelGeneralTabCenter.add(getJPanelSkuInfo(), null);
            jPanelGeneralTabCenter.add(getJScrollPaneSkuInfo(), null);
        }
        return jPanelGeneralTabCenter;
    }

    /**
     * This method initializes jTextField1	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldBuildTarget() {
        if (jTextFieldBuildTarget == null) {
            jTextFieldBuildTarget = new JTextField();
            jTextFieldBuildTarget.setPreferredSize(new Dimension(150, 20));
        }
        return jTextFieldBuildTarget;
    }

    /**
     * This method initializes jButton2	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonAddBuildTarget() {
        if (jButtonAddBuildTarget == null) {
            jButtonAddBuildTarget = new JButton();
            jButtonAddBuildTarget.setPreferredSize(new Dimension(70, 20));
            jButtonAddBuildTarget.setText("Add");
            jButtonAddBuildTarget.addActionListener(new java.awt.event.ActionListener() {
                public void actionPerformed(java.awt.event.ActionEvent e) {
                    if (jTextFieldBuildTarget.getText().length() > 0) {
                        String[] row = {jTextFieldBuildTarget.getText()};
                        buildTargetTableModel.addRow(row);
                        Vector<Object> v = new Vector<Object>();
                        for (int i = 0; i < jTableBuildTargets.getRowCount(); ++i) {
                            v.add(buildTargetTableModel.getValueAt(i, 0));
                        }
                        docConsole.setSaved(false);
                        ffc.setPlatformDefsBuildTargets(v);
                    }
                }
            });
        }
        return jButtonAddBuildTarget;
    }

    /**
     * This method initializes jButton3	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonDelBuildTarget() {
        if (jButtonDelBuildTarget == null) {
            jButtonDelBuildTarget = new JButton();
            jButtonDelBuildTarget.setPreferredSize(new Dimension(70, 20));
            jButtonDelBuildTarget.setText("Delete");
            jButtonDelBuildTarget.setVisible(false);
        }
        return jButtonDelBuildTarget;
    }

    /**
     * This method initializes jScrollPane2	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPaneBuildTargets() {
        if (jScrollPaneBuildTargets == null) {
            jScrollPaneBuildTargets = new JScrollPane();
            jScrollPaneBuildTargets.setPreferredSize(new Dimension(453, 100));
            jScrollPaneBuildTargets.setViewportView(getJTableBuildTargets());
        }
        return jScrollPaneBuildTargets;
    }

    /**
     * This method initializes jScrollPane3	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPaneSkuInfo() {
        if (jScrollPaneSkuInfo == null) {
            jScrollPaneSkuInfo = new JScrollPane();
            jScrollPaneSkuInfo.setPreferredSize(new java.awt.Dimension(453,100));
            jScrollPaneSkuInfo.setViewportView(getJTableSkuInfo());
        }
        return jScrollPaneSkuInfo;
    }

    /**
     * This method initializes jTable2	
     * 	
     * @return javax.swing.JTable	
     */
    private JTable getJTableSkuInfo() {
        if (jTableSkuInfo == null) {
            skuInfoTableModel = new SkuInfoTableModel();
            skuInfoTableModel.addColumn("SKU ID");
            skuInfoTableModel.addColumn("Name");
            jTableSkuInfo = new JTable(skuInfoTableModel);
            
            jTableSkuInfo.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
            
            jTableSkuInfo.getModel().addTableModelListener(new TableModelListener() {
                public void tableChanged(TableModelEvent arg0) {
                    // TODO Auto-generated method stub
                    int row = arg0.getFirstRow();
                    TableModel m = (TableModel)arg0.getSource();
                    if (arg0.getType() == TableModelEvent.UPDATE){
                        //ToDo Data Validition check.
                        String id = m.getValueAt(row, 0)+"";
                        String name = m.getValueAt(row, 1)+"";
                        docConsole.setSaved(false);
                        ffc.updatePlatformDefsSkuInfo(row, id, name);
                    }
                }
            });
        }
        return jTableSkuInfo;
    }

    /**
     * This method initializes jCheckBox	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBoxEbc() {
        if (jCheckBoxEbc == null) {
            jCheckBoxEbc = new JCheckBox();
            jCheckBoxEbc.setPreferredSize(new java.awt.Dimension(50,20));
            jCheckBoxEbc.setText("EBC");
            jCheckBoxEbc.addItemListener(new java.awt.event.ItemListener() {
                public void itemStateChanged(java.awt.event.ItemEvent e) {
                    Vector<Object> v = new Vector<Object>();
                    getToolChain(v);
                    if (v.size() == 0) {
                        JOptionPane.showMessageDialog(frame, "Platform must contain at least ONE supported Arch.");
                        return;
                    }
                    ffc.setPlatformDefsSupportedArchs(v);
                }
            });
        }
        return jCheckBoxEbc;
    }

    /**
     * This method initializes jCheckBox5	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBoxArm() {
        if (jCheckBoxArm == null) {
            jCheckBoxArm = new JCheckBox();
            jCheckBoxArm.setPreferredSize(new java.awt.Dimension(52,20));
            jCheckBoxArm.setText("ARM");
            jCheckBoxArm.setVisible(false);
            jCheckBoxArm.addItemListener(new java.awt.event.ItemListener() {
                public void itemStateChanged(java.awt.event.ItemEvent e) {
                    Vector<Object> v = new Vector<Object>();
                    getToolChain(v);
                    if (v.size() == 0) {
                        JOptionPane.showMessageDialog(frame, "Platform must contain at least ONE supported Arch.");
                        return;
                    }
                    ffc.setPlatformDefsSupportedArchs(v);
                }
            });
        }
        return jCheckBoxArm;
    }

    /**
     * This method initializes jCheckBox6	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBoxPpc() {
        if (jCheckBoxPpc == null) {
            jCheckBoxPpc = new JCheckBox();
            jCheckBoxPpc.setPreferredSize(new Dimension(50, 20));
            jCheckBoxPpc.setText("PPC");
            jCheckBoxPpc.setVisible(false);
            jCheckBoxPpc.addItemListener(new java.awt.event.ItemListener() {
                public void itemStateChanged(java.awt.event.ItemEvent e) {
                    Vector<Object> v = new Vector<Object>();
                    getToolChain(v);
                    if (v.size() == 0) {
                        JOptionPane.showMessageDialog(frame, "Platform must contain at least ONE supported Arch.");
                        return;
                    }
                    ffc.setPlatformDefsSupportedArchs(v);
                }
            });
        }
        return jCheckBoxPpc;
    }

    /**
     * This method initializes jPanel8	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelDir() {
        if (jPanelDir == null) {
            FlowLayout flowLayout1 = new FlowLayout();
            flowLayout1.setAlignment(java.awt.FlowLayout.LEFT);
            jLabelOutputDir = new JLabel();
            jLabelOutputDir.setText("Output Directory");
            jLabelPad = new JLabel();
            jLabelPad.setPreferredSize(new java.awt.Dimension(150,20));
            jLabelPad.setText("");
            jPanelDir = new JPanel();
            jPanelDir.setLayout(flowLayout1);
            jPanelDir.setPreferredSize(new java.awt.Dimension(450,100));
            jPanelDir.setBorder(javax.swing.BorderFactory.createEtchedBorder(javax.swing.border.EtchedBorder.RAISED));
            jPanelDir.add(getJCheckBoxInterDir(), null);
            jPanelDir.add(getJComboBoxInterDir(), null);
            jPanelDir.add(jLabelPad, null);
            jPanelDir.add(jLabelOutputDir, null);
            jPanelDir.add(getJTextFieldOutputDir(), null);
        }
        return jPanelDir;
    }

    /**
     * This method initializes jTextField	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldOutputDir() {
        if (jTextFieldOutputDir == null) {
            jTextFieldOutputDir = new JTextField();
            jTextFieldOutputDir.setPreferredSize(new java.awt.Dimension(300,20));
            jTextFieldOutputDir.addFocusListener(new java.awt.event.FocusAdapter() {
                public void focusLost(java.awt.event.FocusEvent e) {
                    docConsole.setSaved(false);
                    ffc.setPlatformDefsOutputDir(jTextFieldOutputDir.getText());
                }
            });
        }
        return jTextFieldOutputDir;
    }

    /**
     * This method initializes jPanel9	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelSkuInfo() {
        if (jPanelSkuInfo == null) {
            jLabelPadd = new JLabel();
            jLabelPadd.setPreferredSize(new Dimension(280, 20));
            jLabelPadd.setText("                                                 ");
            jLabelSkuName = new JLabel();
            jLabelSkuName.setPreferredSize(new Dimension(40, 20));
            jLabelSkuName.setText("Name");
            jLabelSkuId = new JLabel();
            jLabelSkuId.setPreferredSize(new Dimension(20, 20));
            jLabelSkuId.setText("ID");
            jLabelSkuInfo = new JLabel();
            jLabelSkuInfo.setPreferredSize(new java.awt.Dimension(150,20));
            jLabelSkuInfo.setText("SKU Information");
            jPanelSkuInfo = new JPanel();
            jPanelSkuInfo.setPreferredSize(new java.awt.Dimension(450,70));
            jPanelSkuInfo.add(jLabelSkuInfo, null);
            jPanelSkuInfo.add(jLabelPadd, null);
            jPanelSkuInfo.add(jLabelSkuId, null);
            jPanelSkuInfo.add(getJTextFieldSkuId(), null);
            jPanelSkuInfo.add(jLabelSkuName, null);
            jPanelSkuInfo.add(getJTextFieldSkuName(), null);
            jPanelSkuInfo.add(getJButtonSkuAdd(), null);
            jPanelSkuInfo.add(getJButtonSkuDel(), null);
        }
        return jPanelSkuInfo;
    }

    /**
     * This method initializes jTextField2	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldSkuId() {
        if (jTextFieldSkuId == null) {
            jTextFieldSkuId = new JTextField();
            jTextFieldSkuId.setPreferredSize(new Dimension(50, 20));
        }
        return jTextFieldSkuId;
    }

    /**
     * This method initializes jTextField3	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldSkuName() {
        if (jTextFieldSkuName == null) {
            jTextFieldSkuName = new JTextField();
            jTextFieldSkuName.setPreferredSize(new Dimension(150, 20));
        }
        return jTextFieldSkuName;
    }

    /**
     * This method initializes jButton	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonSkuAdd() {
        if (jButtonSkuAdd == null) {
            jButtonSkuAdd = new JButton();
            jButtonSkuAdd.setPreferredSize(new Dimension(70, 20));
            jButtonSkuAdd.setText("Add");
            jButtonSkuAdd.addActionListener(new java.awt.event.ActionListener() {
                public void actionPerformed(java.awt.event.ActionEvent e) {
                    if (jTextFieldSkuId.getText().length() > 0) {
                        String[] row = {jTextFieldSkuId.getText(), jTextFieldSkuName.getText()};
                        skuInfoTableModel.addRow(row);
                        docConsole.setSaved(false);
                        ffc.genPlatformDefsSkuInfo(row[0], row[1]);
                    }
                }
            });
        }
        return jButtonSkuAdd;
    }

    /**
     * This method initializes jButton1	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonSkuDel() {
        if (jButtonSkuDel == null) {
            jButtonSkuDel = new JButton();
            jButtonSkuDel.setPreferredSize(new Dimension(70, 20));
            jButtonSkuDel.setText("Delete");
            jButtonSkuDel.addActionListener(new java.awt.event.ActionListener() {
                public void actionPerformed(java.awt.event.ActionEvent e) {
                    if (jTableSkuInfo.isEditing()) {
                        jTableSkuInfo.getCellEditor().stopCellEditing();
                    }
                    if (jTableSkuInfo.getSelectedRow() < 1) {
                        return;
                    }
                    docConsole.setSaved(false);
                    ffc.removePlatformDefsSkuInfo(jTableSkuInfo.getSelectedRow());
                    skuInfoTableModel.removeRow(jTableSkuInfo.getSelectedRow());
                }
            });
        }
        return jButtonSkuDel;
    }


}  //  @jve:decl-index=0:visual-constraint="10,10"

class SkuInfoTableModel extends DefaultTableModel{
    /**
     * 
     */
    private static final long serialVersionUID = 1L;

    public boolean isCellEditable(int row, int column) {
        if (row == 0) {
            return false;
        }
        return true;
    }
}

class TargetTableModel extends DefaultTableModel{
    private static final long serialVersionUID = 1L;

    public boolean isCellEditable(int row, int column) {
        if (row < 2) {
            return false;
        }
        return true;
    }
}


