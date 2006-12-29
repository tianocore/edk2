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

import javax.swing.JTabbedPane;
import javax.swing.JButton;
import javax.swing.ListSelectionModel;

import org.tianocore.PlatformSurfaceAreaDocument;
import org.tianocore.frameworkwizard.common.Identifications.OpeningPlatformType;
import org.tianocore.frameworkwizard.common.ui.IInternalFrame;
import org.tianocore.frameworkwizard.toolchain.ToolChainId;

import javax.swing.JCheckBox;
import javax.swing.JOptionPane;
import javax.swing.JTextField;
import javax.swing.JLabel;
import javax.swing.JScrollPane;
import javax.swing.JTable;
import javax.swing.event.InternalFrameAdapter;
import javax.swing.event.InternalFrameEvent;
//import javax.swing.event.ListSelectionEvent;
//import javax.swing.event.ListSelectionListener;
import javax.swing.event.TableModelEvent;
import javax.swing.event.TableModelListener;
import javax.swing.table.DefaultTableModel;
import javax.swing.table.TableModel;
import javax.swing.JComboBox;
import java.awt.Dimension;
import java.util.Vector;

public class FpdPlatformDefs extends IInternalFrame {

    private static boolean Debug = false;

    private final int dialogWidth = 600;

    private final int oneRowHeight = 20;

    private final int twoRowHeight = 40;

    //    private final int threeRowHeight = 60;

    private final int fourRowHeight = 80;

    private final int sepHeight = 6;

    //    private final int sepWidth = 10;

    private final int offsetWidth = 70;

    private final int buttonWidth = 90;

    private final int rowOne = 12;

    private final int rowTwo = rowOne + oneRowHeight + sepHeight * 3;

    private final int rowThree = rowTwo + oneRowHeight + sepHeight;

    private final int rowFour = rowThree + oneRowHeight + sepHeight;

    private final int rowFive = rowFour + fourRowHeight + sepHeight * 3;

    private final int rowSix = rowFive + oneRowHeight + sepHeight;

    private final int rowSeven = rowSix + oneRowHeight + sepHeight;

    private final int rowEight = rowSeven + oneRowHeight + sepHeight;

    private final int rowNine = rowEight + oneRowHeight + sepHeight;

    private final int rowTen = rowNine + fourRowHeight + sepHeight + sepHeight * 3;

    private final int rowEleven = rowTen + oneRowHeight + sepHeight;

    private final int rowTwelve = rowEleven + oneRowHeight + sepHeight;

    private final int dialogHeight = rowTwelve + twoRowHeight;

    private final int labelColumn = 12;

    private final int valueColumn = 168;

    private final int labelWidth = 155;

    private final int valueWidth = 320;

    private final int valueCenter = valueColumn + (valueWidth / 2);

    private final int tableHeight = fourRowHeight;

    private final int tableWidth = valueWidth;

    private JPanel jContentPane = null;

    private JTabbedPane jTabbedPane = null;

    private TargetTableModel buildTargetTableModel = null;

    private SkuInfoTableModel skuInfoTableModel = null;

    private OpeningPlatformType docConsole = null;

    private FpdFileContents ffc = null;

    private JPanel jPanelGeneralTab = null;

    private JPanel jPanelGeneralContainer = null;

    private JLabel jLabelSupArch = null;

    private JCheckBox jCheckBoxIa32 = null;

    private JCheckBox jCheckBoxX64 = null;

    private JCheckBox jCheckBoxIpf = null;

    private JComboBox jComboBoxInterDir = null;

    private JComboBox jBuildTargetComboBox = null;

    private JTable jTableBuildTargets = null;

    private JPanel jArchitectureSelections = null;

    private JLabel jLabelBuildTargets = null;

    //    private JTextField jTextFieldBuildTarget = null;

    private JButton jButtonAddBuildTarget = null;

    private JButton jButtonDelBuildTarget = null;

    private JScrollPane jScrollPaneBuildTargets = null;

    private JScrollPane jScrollPaneSkuInfo = null;

    private JTable jTableSkuInfo = null;

    private JCheckBox jCheckBoxEbc = null;

    private JCheckBox jCheckBoxArm = null;

    private JCheckBox jCheckBoxPpc = null;

    private JLabel jLabelSkuInfo = null;

    private JLabel jLabelSkuId = null;

    private JTextField jTextFieldSkuId = null;

    private JLabel jLabelSkuName = null;

    private JTextField jTextFieldSkuName = null;

    private JButton jButtonSkuAdd = null;

    private JButton jButtonSkuDel = null;

    private JLabel jLabelIntermediateDirs = null;

    private JLabel jLabelOutputDir = null;

    private JTextField jTextFieldOutputDir = null;

    private JLabel jLabelOutputInfo = null;

    private int selectedRow = -1;

    /**
     *  The following are not used by the UI 
     */
    private static final long serialVersionUID = 1L;

    private ToolChainId tid = new ToolChainId();

    public FpdPlatformDefs() {
        super();
        initialize();
        this.setBounds(new java.awt.Rectangle(0, 0, 500, 370));
        this.setVisible(true);
    }

    public FpdPlatformDefs(PlatformSurfaceAreaDocument.PlatformSurfaceArea fpd) {
        this();
        ffc = new FpdFileContents(fpd);
        init(ffc);
    }

    public FpdPlatformDefs(OpeningPlatformType opt) {
        this(opt.getXmlFpd());
        docConsole = opt;
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
        this.setSize(dialogWidth, dialogHeight);
        this.setDefaultCloseOperation(javax.swing.WindowConstants.DISPOSE_ON_CLOSE);
        this.setTitle("FPD Platform Definitions");
        this.setContentPane(getJContentPane());
        this.addInternalFrameListener(new InternalFrameAdapter() {
            public void internalFrameDeactivated(InternalFrameEvent e) {
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
        showSupportedArchitectures(v);
        v.removeAllElements();

        ffc.getPlatformDefsBuildTargets(v);
        buildTargetTableModel.setRowCount(0);
        for (int i = 0; i < v.size(); ++i) {
            Object[] row = { v.get(i) };
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

    private void showSupportedArchitectures(Vector<Object> v) {
        if (v.contains("IA32")) {
            jCheckBoxIa32.setSelected(true);
        } else {
            jCheckBoxIa32.setSelected(false);
        }
        if (v.contains("X64")) {
            jCheckBoxX64.setSelected(true);
        } else {
            jCheckBoxX64.setSelected(false);
        }
        if (v.contains("IPF")) {
            jCheckBoxIpf.setSelected(true);
        } else {
            jCheckBoxIpf.setSelected(false);
        }
        if (v.contains("EBC")) {
            jCheckBoxEbc.setSelected(true);
        } else {
            jCheckBoxEbc.setSelected(false);
        }
        if (v.contains("ARM")) {
            jCheckBoxArm.setSelected(true);
        } else {
            jCheckBoxArm.setSelected(false);
        }
        if (v.contains("PPC")) {
            jCheckBoxPpc.setSelected(true);
        } else {
            jCheckBoxPpc.setSelected(false);
        }
    }

    private void getSupportedArchitectures(Vector<Object> v) {
        if (docConsole != null) {
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
            jPanelGeneralTab.setBounds(new java.awt.Rectangle(0, 0, dialogWidth * 2, dialogHeight * 3));
            jPanelGeneralTab.setPreferredSize(new java.awt.Dimension(dialogWidth + 10, (dialogHeight * 3) + 10));
            jPanelGeneralTab.setAutoscrolls(true);
            jPanelGeneralTab.setLocation(0, 0);
            jPanelGeneralTab.setLayout(null);
            jPanelGeneralTab.add(getJPanelGeneralContainer(), null);
        }
        return jPanelGeneralTab;
    }

    /**
     * This method initializes jPanel5	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelGeneralContainer() {
        if (jPanelGeneralContainer == null) {
            jLabelSupArch = new JLabel();
            jLabelSupArch.setText("Supported Architectures");
            jLabelSupArch.setBounds(new java.awt.Rectangle(labelColumn, rowOne, labelWidth, oneRowHeight));
            jLabelBuildTargets = new JLabel();
            jLabelBuildTargets.setText("Build Targets");
            jLabelBuildTargets.setBounds(new java.awt.Rectangle(labelColumn, rowTwo, labelWidth, oneRowHeight));
            jLabelSkuInfo = new JLabel();
            jLabelSkuInfo.setText("SKU Information");
            jLabelSkuInfo.setBounds(new java.awt.Rectangle(labelColumn, rowFive, labelWidth, oneRowHeight));
            jLabelSkuId = new JLabel();
            jLabelSkuId.setText("SKU ID Number");
            jLabelSkuId.setBounds(new java.awt.Rectangle(labelColumn + 10, rowSix, labelWidth, oneRowHeight));
            jLabelSkuName = new JLabel();
            jLabelSkuName.setText("SKU Name");
            jLabelSkuName.setBounds(new java.awt.Rectangle(labelColumn + 10, rowSeven, labelWidth, oneRowHeight));
            jLabelOutputInfo = new JLabel();
            jLabelOutputInfo.setText("Output Directory Configuration");
            jLabelOutputInfo.setBounds(new java.awt.Rectangle(labelColumn, rowTen, valueWidth, oneRowHeight));
            jLabelIntermediateDirs = new JLabel();
            jLabelIntermediateDirs.setText("Intermediate Build Directories");
            jLabelIntermediateDirs.setBounds(new java.awt.Rectangle(labelColumn + 10, rowEleven, valueWidth,
                                                                    oneRowHeight));
            jLabelOutputDir = new JLabel();
            jLabelOutputDir.setText("Name of the Output Directory");
            jLabelOutputDir.setBounds(new java.awt.Rectangle(labelColumn + 10, rowTwelve, valueWidth, oneRowHeight));

            jPanelGeneralContainer = new JPanel();
            jPanelGeneralContainer.setLayout(null);
            jPanelGeneralContainer.setLocation(new java.awt.Point(2, 2));
            jPanelGeneralContainer.setBounds(new java.awt.Rectangle(2, 2, dialogWidth * 2, dialogHeight));
            jPanelGeneralContainer.setPreferredSize(new java.awt.Dimension(dialogWidth, dialogHeight));

            jPanelGeneralContainer.add(jLabelSupArch, null);
            jPanelGeneralContainer.add(getArchitectureSelections(), null);

            jPanelGeneralContainer.add(jLabelBuildTargets, null);
            // jPanelGeneralContainer.add(getJTextFieldBuildTarget(), null);
            jPanelGeneralContainer.add(getJBuildTargetComboBox(), null);

            jPanelGeneralContainer.add(getJButtonAddBuildTarget(), null);
            jPanelGeneralContainer.add(getJButtonDelBuildTarget(), null);
            jPanelGeneralContainer.add(getJScrollPaneBuildTargets(), null);

            jPanelGeneralContainer.add(jLabelSkuInfo, null);
            jPanelGeneralContainer.add(jLabelSkuId, null);
            jPanelGeneralContainer.add(getJTextFieldSkuId(), null);
            jPanelGeneralContainer.add(jLabelSkuName, null);
            jPanelGeneralContainer.add(getJTextFieldSkuName(), null);
            jPanelGeneralContainer.add(getJButtonSkuAdd(), null);
            jPanelGeneralContainer.add(getJButtonSkuDel(), null);
            jPanelGeneralContainer.add(getJScrollPaneSkuInfo(), null);

            jPanelGeneralContainer.add(jLabelOutputInfo, null);
            jPanelGeneralContainer.add(jLabelIntermediateDirs, null);
            jPanelGeneralContainer.add(getJComboBoxInterDir(), null);
            jPanelGeneralContainer.add(jLabelOutputDir, null);
            jPanelGeneralContainer.add(getJTextFieldOutputDir(), null);

        }
        return jPanelGeneralContainer;
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
            jArchitectureSelections.add(getJCheckBoxIa32(), null);
            jArchitectureSelections.add(getJCheckBoxX64(), null);
            jArchitectureSelections.add(getJCheckBoxIpf(), null);
            jArchitectureSelections.add(getJCheckBoxEbc(), null);
            jArchitectureSelections.add(getJCheckBoxArm(), null);
            jArchitectureSelections.add(getJCheckBoxPpc(), null);
            jArchitectureSelections.setBounds(new java.awt.Rectangle(valueColumn, rowOne, valueWidth, oneRowHeight));
            jArchitectureSelections.setPreferredSize(new java.awt.Dimension(valueWidth, oneRowHeight));
            jArchitectureSelections.setLocation(new java.awt.Point(valueColumn, rowOne));
            jArchitectureSelections.setToolTipText("<html>A Platform may support one or more architectures," 
                                                   + "<br>at least one architecture must be selected!</html>");
        }
        return jArchitectureSelections;
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
            jCheckBoxIa32.setBounds(new java.awt.Rectangle(0, 0, 55, 20));
            jCheckBoxIa32.addItemListener(new java.awt.event.ItemListener() {
                public void itemStateChanged(java.awt.event.ItemEvent e) {
                    Vector<Object> v = new Vector<Object>();
                    getSupportedArchitectures(v);
                    if (v.size() == 0) {
                        JOptionPane.showMessageDialog(FpdPlatformDefs.this, "Platform must support at least ONE Architecture.");
                        jCheckBoxIa32.setSelected(true);
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
            jCheckBoxX64.setBounds(new java.awt.Rectangle(55, 0, 53, 20));
            jCheckBoxX64.addItemListener(new java.awt.event.ItemListener() {
                public void itemStateChanged(java.awt.event.ItemEvent e) {
                    Vector<Object> v = new Vector<Object>();
                    getSupportedArchitectures(v);
                    if (v.size() == 0) {
                        JOptionPane.showMessageDialog(FpdPlatformDefs.this, "Platform must support at least ONE Architecture.");
                        jCheckBoxX64.setSelected(true);
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
            jCheckBoxIpf.setBounds(new java.awt.Rectangle(108, 0, 52, 20));
            jCheckBoxIpf.addItemListener(new java.awt.event.ItemListener() {
                public void itemStateChanged(java.awt.event.ItemEvent e) {
                    Vector<Object> v = new Vector<Object>();
                    getSupportedArchitectures(v);
                    if (v.size() == 0) {
                        JOptionPane.showMessageDialog(FpdPlatformDefs.this, "Platform must support at least ONE Architecture.");
                        jCheckBoxIpf.setSelected(true);
                        return;
                    }
                    ffc.setPlatformDefsSupportedArchs(v);
                }
            });
        }
        return jCheckBoxIpf;
    }

    /**
     * This method initializes jCheckBox    
     *  
     * @return javax.swing.JCheckBox    
     */
    private JCheckBox getJCheckBoxEbc() {
        if (jCheckBoxEbc == null) {
            jCheckBoxEbc = new JCheckBox();
            // jCheckBoxEbc.setPreferredSize(new java.awt.Dimension(50, 20));
            jCheckBoxEbc.setBounds(new java.awt.Rectangle(160, 0, 53, 20));
            jCheckBoxEbc.setText("EBC");
            jCheckBoxEbc.setVisible(true);
            jCheckBoxEbc.addItemListener(new java.awt.event.ItemListener() {
                public void itemStateChanged(java.awt.event.ItemEvent e) {
                    Vector<Object> v = new Vector<Object>();
                    getSupportedArchitectures(v);
                    if (v.size() == 0) {
                        JOptionPane.showMessageDialog(FpdPlatformDefs.this, "Platform must support at least ONE Architecture.");
                        jCheckBoxEbc.setSelected(true);
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
            // jCheckBoxArm.setPreferredSize(new java.awt.Dimension(52, 20));
            jCheckBoxArm.setBounds(new java.awt.Rectangle(213, 0, 54, 20));
            jCheckBoxArm.setText("ARM");
            jCheckBoxArm.setVisible(true);
            jCheckBoxArm.addItemListener(new java.awt.event.ItemListener() {
                public void itemStateChanged(java.awt.event.ItemEvent e) {
                    Vector<Object> v = new Vector<Object>();
                    getSupportedArchitectures(v);
                    if (v.size() == 0) {
                        JOptionPane.showMessageDialog(FpdPlatformDefs.this, "Platform must support at least ONE Architecture.");
                        jCheckBoxArm.setSelected(true);
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
            // jCheckBoxPpc.setPreferredSize(new Dimension(50, 20));
            jCheckBoxPpc.setBounds(new java.awt.Rectangle(267, 0, 53, 20));
            jCheckBoxPpc.setText("PPC");
            jCheckBoxPpc.setVisible(true);
            jCheckBoxPpc.addItemListener(new java.awt.event.ItemListener() {
                public void itemStateChanged(java.awt.event.ItemEvent e) {
                    Vector<Object> v = new Vector<Object>();
                    getSupportedArchitectures(v);
                    if (v.size() == 0) {
                        JOptionPane.showMessageDialog(FpdPlatformDefs.this, "Platform must support at least ONE  Architecture.");
                        jCheckBoxPpc.setSelected(true);
                        return;
                    }
                    ffc.setPlatformDefsSupportedArchs(v);
                }
            });
        }
        return jCheckBoxPpc;
    }

    /**
     * Use a ComboBox for BuildTargets
     * 
     * @return javax.swing.JComboBox jBuildTargetComboBox
     */
    private JComboBox getJBuildTargetComboBox() {
        if (jBuildTargetComboBox == null) {
            String toolBt = null;
            if (tid.getToolsDefTargetNames() == null)
                toolBt = "DEBUG RELEASE";
            else
                toolBt = tid.getToolsDefTargetNames().trim();

            if ((toolBt.contains("*")) || (toolBt.length() < 1)) {
                toolBt = "DEBUG RELEASE";
            }
            if (Debug)
                System.out.println("Using Build Targets: " + toolBt.trim());

            toolBt = toolBt.replaceAll(" ", ":");
            toolBt = " :" + toolBt;
            if (Debug)
                System.out.println("Using Build Targets: " + toolBt.trim());
            String[] buildTargets = toolBt.trim().split(":");

            jBuildTargetComboBox = new JComboBox(buildTargets);
            jBuildTargetComboBox.setEditable(true);
            jBuildTargetComboBox.setPreferredSize(new Dimension(valueWidth, oneRowHeight));
            jBuildTargetComboBox.setBounds(new java.awt.Rectangle(valueColumn, rowTwo, valueWidth, oneRowHeight));
            jBuildTargetComboBox.setLocation(new java.awt.Point(valueColumn, rowTwo));
            jBuildTargetComboBox.setToolTipText("<html>Select a defined Target and then click Add,<br>"
                                                + "or enter a new, one word TargetName and then click Add.<br>"
                                                + "Remember to define the Targetname in the tool defintion file."
                                                + "</html>");

        }
        return jBuildTargetComboBox;
    }

    /**
     * This method initializes jTextField1	
     * 	
     * @return javax.swing.JTextField	
     */
    //    private JTextField getJTextFieldBuildTarget() {
    //        if (jTextFieldBuildTarget == null) {
    //            jTextFieldBuildTarget = new JTextField();
    //            jTextFieldBuildTarget.setPreferredSize(new Dimension(valueWidth, oneRowHeight));
    //            jTextFieldBuildTarget.setBounds(new java.awt.Rectangle(valueColumn, rowTwo, valueWidth, oneRowHeight));
    //            jTextFieldBuildTarget.setLocation(new java.awt.Point(valueColumn, rowTwo));
    //        }
    //        return jTextFieldBuildTarget;
    //   }
    /**
     * This method initializes jButton2	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonAddBuildTarget() {
        if (jButtonAddBuildTarget == null) {
            jButtonAddBuildTarget = new JButton();
            jButtonAddBuildTarget.setPreferredSize(new Dimension(buttonWidth, oneRowHeight));
            jButtonAddBuildTarget.setBounds(new java.awt.Rectangle(valueCenter - buttonWidth - 5, rowThree,
                                                                   buttonWidth, oneRowHeight));
            jButtonAddBuildTarget.setLocation(new java.awt.Point(valueCenter - buttonWidth - 5, rowThree));
            jButtonAddBuildTarget.setText("Add");
            jButtonAddBuildTarget.setVisible(true);

            jButtonAddBuildTarget.addActionListener(new java.awt.event.ActionListener() {
                public void actionPerformed(java.awt.event.ActionEvent e) {
                    // Used with Text Field
                    // if (jTextFieldBuildTarget.getText().length() > 0) {
                    //    String[] row = { jTextFieldBuildTarget.getText() };
                    //    jTextFieldBuildTarget.setText("");
                    //    buildTargetTableModel.addRow(row);
                    //    Vector<Object> v = new Vector<Object>();
                    //    for (int i = 0; i < jTableBuildTargets.getRowCount(); ++i) {
                    //        v.add(buildTargetTableModel.getValueAt(i, 0));
                    //    }
                    //    docConsole.setSaved(false);
                    //    ffc.setPlatformDefsBuildTargets(v);
                    // }
                    // Use with ComboBox
                    if (jBuildTargetComboBox.getSelectedItem().toString().length() > 0) {
                        String[] row = { jBuildTargetComboBox.getSelectedItem().toString() };
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
            jButtonDelBuildTarget.setPreferredSize(new Dimension(buttonWidth, oneRowHeight));
            jButtonDelBuildTarget
                                 .setBounds(new java.awt.Rectangle(valueCenter + 5, rowThree, buttonWidth, oneRowHeight));
            jButtonDelBuildTarget.setLocation(new java.awt.Point(valueCenter + 5, rowThree));
            jButtonDelBuildTarget.setText("Delete");
            jButtonDelBuildTarget.setVisible(true);

            jButtonDelBuildTarget.addActionListener(new java.awt.event.ActionListener() {
                public void actionPerformed(java.awt.event.ActionEvent e) {
                    if (jTableBuildTargets.getSelectedRow() < 0) {
                        return;
                    }
                    if (jTableBuildTargets.getRowCount() == 1) {
                        JOptionPane.showMessageDialog(FpdPlatformDefs.this, "At least one build target should be set for this platform.");
                        return;
                    }
                    buildTargetTableModel.removeRow(jTableBuildTargets.getSelectedRow());
                    Vector<Object> v = new Vector<Object>();
                    for (int i = 0; i < jTableBuildTargets.getRowCount(); ++i) {
                        v.add(buildTargetTableModel.getValueAt(i, 0));
                    }
                    docConsole.setSaved(false);
                    ffc.setPlatformDefsBuildTargets(v);
                }
            });
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
            jScrollPaneBuildTargets.setPreferredSize(new Dimension(tableWidth - 20, tableHeight - 20));
            jScrollPaneBuildTargets.setBounds(new java.awt.Rectangle(valueColumn, rowFour, tableWidth - 5,
                                                                     tableHeight - 5));
            jScrollPaneBuildTargets.setLocation(new java.awt.Point(valueColumn, rowFour));
            jScrollPaneBuildTargets
                                   .setBorder(javax.swing.BorderFactory
                                                                       .createEtchedBorder(javax.swing.border.EtchedBorder.RAISED));

            jScrollPaneBuildTargets.setViewportView(getJTableBuildTargets());
        }
        return jScrollPaneBuildTargets;
    }

    /**
     * This method initializes jTable   
     *  
     * @return javax.swing.JTable   
     */
    private JTable getJTableBuildTargets() {
        if (jTableBuildTargets == null) {
            buildTargetTableModel = new TargetTableModel();

            jTableBuildTargets = new JTable(buildTargetTableModel);
            jTableBuildTargets.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
            jTableBuildTargets.setRowHeight(oneRowHeight);
            jTableBuildTargets.setToolTipText("<html>Select one of the Targets from the table and<br>"
                                              + "click Delete to remove the target from the platform.</html>");
            buildTargetTableModel.addColumn("Build Target");

            jTableBuildTargets.getModel().addTableModelListener(new TableModelListener() {
                public void tableChanged(TableModelEvent arg0) {
                    // TODO Auto-generated method stub
                    selectedRow = jTableBuildTargets.getSelectedRow();
                    if (selectedRow < 0) {
                        return;
                    }
                    TableModel m = (TableModel) arg0.getSource();
                    if (arg0.getType() == TableModelEvent.UPDATE) {
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
     * This method initializes jTextField2  
     *  
     * @return javax.swing.JTextField   
     */
    private JTextField getJTextFieldSkuId() {
        if (jTextFieldSkuId == null) {
            jTextFieldSkuId = new JTextField();
            jTextFieldSkuId.setPreferredSize(new Dimension(40, oneRowHeight));
            jTextFieldSkuId.setBounds(new java.awt.Rectangle(valueColumn, rowSix, 40, oneRowHeight));
            jTextFieldSkuId.setLocation(new java.awt.Point(valueColumn, rowSix));
            jTextFieldSkuId.setToolTipText("Enter a unique integer value.");
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
            jTextFieldSkuName.setPreferredSize(new Dimension(valueWidth, oneRowHeight));
            jTextFieldSkuName.setBounds(new java.awt.Rectangle(valueColumn, rowSeven, valueWidth, oneRowHeight));
            jTextFieldSkuName.setLocation(new java.awt.Point(valueColumn, rowSeven));
            jTextFieldSkuName.setToolTipText("<html>Enter a name to help identify this SKU.<br>"
                                             + "This entry is not used by the build system, it is<br>"
                                             + "used only by this user interface.</html>");
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

            jButtonSkuAdd.setPreferredSize(new Dimension(buttonWidth, oneRowHeight));
            jButtonSkuAdd.setBounds(new java.awt.Rectangle(valueCenter - buttonWidth - 5, rowEight, buttonWidth,
                                                           oneRowHeight));
            jButtonSkuAdd.setLocation(new java.awt.Point(valueCenter - buttonWidth - 5, rowEight));
            jButtonSkuAdd.setText("Add");
            jButtonSkuAdd.setVisible(true);

            jButtonSkuAdd.addActionListener(new java.awt.event.ActionListener() {
                public void actionPerformed(java.awt.event.ActionEvent e) {
                    if (jTextFieldSkuId.getText().length() > 0) {
                        String[] row = { jTextFieldSkuId.getText(), jTextFieldSkuName.getText() };
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
            jButtonSkuDel.setPreferredSize(new Dimension(buttonWidth, oneRowHeight));
            jButtonSkuDel.setBounds(new java.awt.Rectangle(valueCenter + 5, rowEight, buttonWidth, oneRowHeight));
            jButtonSkuDel.setLocation(new java.awt.Point(valueCenter + 5, rowEight));
            jButtonSkuDel.setText("Delete");
            jButtonSkuDel.setVisible(true);
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

    /**
     * This method initializes jScrollPane3	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPaneSkuInfo() {
        if (jScrollPaneSkuInfo == null) {
            jScrollPaneSkuInfo = new JScrollPane();
            jScrollPaneSkuInfo.setPreferredSize(new Dimension(tableWidth - 20, tableHeight - 20));
            jScrollPaneSkuInfo.setBounds(new java.awt.Rectangle(valueColumn, rowNine, tableWidth - 5, tableHeight - 5));
            jScrollPaneSkuInfo.setLocation(new java.awt.Point(valueColumn, rowNine));

            jScrollPaneSkuInfo
                              .setBorder(javax.swing.BorderFactory
                                                                  .createEtchedBorder(javax.swing.border.EtchedBorder.RAISED));

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
            jTableSkuInfo.setToolTipText("<html>SKU ID 0 must always be defined as the default.<br>"
                                         + "0 can mean either SKU disabled, or it can be the<br>"
                                         + "default value if more than one SKU is defined, and the<br>"
                                         + "platform is not jumpered to use one of the other SKU values.</html>");

            jTableSkuInfo.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);

            jTableSkuInfo.getModel().addTableModelListener(new TableModelListener() {
                public void tableChanged(TableModelEvent arg0) {
                    // TODO Auto-generated method stub
                    int row = arg0.getFirstRow();
                    TableModel m = (TableModel) arg0.getSource();
                    if (arg0.getType() == TableModelEvent.UPDATE) {
                        //ToDo Data Validition check.
                        String id = m.getValueAt(row, 0) + "";
                        String name = m.getValueAt(row, 1) + "";
                        docConsole.setSaved(false);
                        ffc.updatePlatformDefsSkuInfo(row, id, name);
                    }
                }
            });
        }
        return jTableSkuInfo;
    }

    /**
     * This method initializes jComboBox    
     *  
     * @return javax.swing.JComboBox    
     */
    private JComboBox getJComboBoxInterDir() {
        if (jComboBoxInterDir == null) {
            jComboBoxInterDir = new JComboBox();
            jComboBoxInterDir.setPreferredSize(new Dimension(75, oneRowHeight));
            jComboBoxInterDir.setBounds(new java.awt.Rectangle(valueColumn + offsetWidth, rowEleven, 95, oneRowHeight));
            jComboBoxInterDir.setLocation(new java.awt.Point(valueColumn + offsetWidth, rowEleven));
            jComboBoxInterDir.addItem("UNIFIED");
            jComboBoxInterDir.addItem("MODULE");
            jComboBoxInterDir.setSelectedIndex(0);
            jComboBoxInterDir.setToolTipText("<html>Select UNIFIED to generate intermediate directories under<br>"
                                             + "under platform directory tree.<br>"
                                             + "Select MODULE to generate intermediate directories under the<br>"
                                             + "individual module directories.</html>");
            jComboBoxInterDir.addItemListener(new java.awt.event.ItemListener() {
                public void itemStateChanged(java.awt.event.ItemEvent e) {
                    if (docConsole != null) {
                        docConsole.setSaved(false);
                    }
                    ffc.setPlatformDefsInterDir(jComboBoxInterDir.getSelectedItem() + "");
                }
            });
        }
        return jComboBoxInterDir;
    }

    /**
     * This method initializes jTextField	
     * 	
     * @return javax.swing.JTextField  Row Twelve	
     */
    private JTextField getJTextFieldOutputDir() {
        if (jTextFieldOutputDir == null) {
            jTextFieldOutputDir = new JTextField();
            jTextFieldOutputDir.setPreferredSize(new java.awt.Dimension(290, 20));
            jTextFieldOutputDir.setBounds(new java.awt.Rectangle(valueColumn + offsetWidth, rowTwelve, valueWidth - 30,
                                                                 oneRowHeight));
            jTextFieldOutputDir.setLocation(new java.awt.Point(valueColumn + offsetWidth, rowTwelve));
            jTextFieldOutputDir.setToolTipText("Select the name or URL for the output directory tree.");
            jTextFieldOutputDir.addFocusListener(new java.awt.event.FocusAdapter() {
                public void focusLost(java.awt.event.FocusEvent e) {
                    docConsole.setSaved(false);
                    ffc.setPlatformDefsOutputDir(jTextFieldOutputDir.getText());
                }
            });
        }
        return jTextFieldOutputDir;
    }

} //  @jve:decl-index=0:visual-constraint="10,10"

class SkuInfoTableModel extends DefaultTableModel {
    private static final long serialVersionUID = 1L;

    public boolean isCellEditable(int row, int column) {
        if (row == 0) {
            return false;
        }
        return true;
    }
}

class TargetTableModel extends DefaultTableModel {
    private static final long serialVersionUID = 1L;

    public boolean isCellEditable(int row, int column) {
        if (row < 2) {
            return false;
        }
        return true;
    }
}
