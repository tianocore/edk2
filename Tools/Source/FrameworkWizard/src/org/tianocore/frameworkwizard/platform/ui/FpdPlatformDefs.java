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
    private JPanel jPanel = null;
    private JPanel jPanel1 = null;
    private JPanel jPanel2 = null;
    private JPanel jPanel3 = null;
    private JTabbedPane jTabbedPane = null;
    private JButton jButtonOk = null;
    private JButton jButtonCancel = null;
    private TargetTableModel imageEntryPointTableModel = null;
    
    private SkuInfoTableModel skuInfoTableModel = null;
    private OpeningPlatformType docConsole = null;
    private FpdFileContents ffc = null;
    private JPanel jPanel4 = null;
    private JPanel jPanel5 = null;
    private JLabel jLabel = null;
    private JCheckBox jCheckBox1 = null;
    private JCheckBox jCheckBox2 = null;
    private JCheckBox jCheckBox3 = null;
    private JPanel jPanel6 = null;
    private JCheckBox jCheckBox4 = null;
    private JComboBox jComboBox = null;
    private JTable jTable = null;
    private JPanel jPanel7 = null;
    private JLabel jLabel2 = null;
    private JTextField jTextField1 = null;
    private JButton jButton2 = null;
    private JButton jButton3 = null;
    private JScrollPane jScrollPane2 = null;
    private JScrollPane jScrollPane3 = null;
    private JTable jTable2 = null;
    private JCheckBox jCheckBox = null;
    private JCheckBox jCheckBox5 = null;
    private JCheckBox jCheckBox6 = null;
    private JPanel jPanel8 = null;
    private JLabel jLabel7 = null;
    private JLabel jLabel1 = null;
    private JTextField jTextField = null;
    private JPanel jPanel9 = null;
    private JLabel jLabel3 = null;
    private JLabel jLabel4 = null;
    private JTextField jTextField2 = null;
    private JLabel jLabel5 = null;
    private JTextField jTextField3 = null;
    private JButton jButton = null;
    private JButton jButton1 = null;
    private JLabel jLabel6 = null;
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
    private JPanel getJPanel() {
        if (jPanel == null) {
            jPanel = new JPanel();
        }
        return jPanel;
    }

    /**
     * This method initializes jPanel1	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel1() {
        if (jPanel1 == null) {
            FlowLayout flowLayout = new FlowLayout();
            flowLayout.setAlignment(java.awt.FlowLayout.RIGHT);
            flowLayout.setHgap(15);
            jPanel1 = new JPanel();
            jPanel1.setLayout(flowLayout);
            jPanel1.setComponentOrientation(java.awt.ComponentOrientation.LEFT_TO_RIGHT);
            jPanel1.add(getJButtonCancel(), null);
            jPanel1.add(getJButtonOk(), null);
        }
        return jPanel1;
    }

    /**
     * This method initializes jPanel2	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel2() {
        if (jPanel2 == null) {
            jPanel2 = new JPanel();
        }
        return jPanel2;
    }

    /**
     * This method initializes jPanel3	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel3() {
        if (jPanel3 == null) {
            jPanel3 = new JPanel();
        }
        return jPanel3;
    }

    /**
     * This method initializes jTabbedPane	
     * 	
     * @return javax.swing.JTabbedPane	
     */
    private JTabbedPane getJTabbedPane() {
        if (jTabbedPane == null) {
            jTabbedPane = new JTabbedPane();
            
			jTabbedPane.addTab("General", null, getJPanel4(), null);
           
        }
        return jTabbedPane;
    }

    /**
     * This method initializes jButtonOk	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonOk() {
        if (jButtonOk == null) {
            jButtonOk = new JButton();
            jButtonOk.setPreferredSize(new java.awt.Dimension(80,20));
            jButtonOk.setText("Cancel");
            jButtonOk.setVisible(false);
        }
        return jButtonOk;
    }

    /**
     * This method initializes jButtonCancel	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonCancel() {
        if (jButtonCancel == null) {
            jButtonCancel = new JButton();
            jButtonCancel.setPreferredSize(new java.awt.Dimension(80,20));
            jButtonCancel.setText("Ok");
            jButtonCancel.setVisible(false);
        }
        return jButtonCancel;
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
                if (jTable.isEditing()) {
                    jTable.getCellEditor().stopCellEditing();
                }
                if (jTable2.isEditing()) {
                    jTable2.getCellEditor().stopCellEditing();
                }
               
            }
        });
    }

    private void init(FpdFileContents ffc) {
        Vector<Object> v = new Vector<Object>();
        ffc.getPlatformDefsSupportedArchs(v);
        showToolChain(v);
        
        imageEntryPointTableModel.setRowCount(0);
        v.removeAllElements();
        ffc.getPlatformDefsBuildTargets(v);
        for (int i = 0; i < v.size(); ++i){
            Object[] row = {v.get(i)};
            imageEntryPointTableModel.addRow(row);
        }
        
        String[][] saa = new String[ffc.getPlatformDefsSkuInfoCount()][2];
        ffc.getPlatformDefsSkuInfos(saa);
        for (int i = 0; i < saa.length; ++i) {
            skuInfoTableModel.addRow(saa[i]);
        }
        
        String interDir = ffc.getPlatformDefsInterDir();
        if (interDir != null) {
            jComboBox.setSelectedItem(interDir);
        }
        
        String outputDir = ffc.getPlatformDefsOutputDir();
        if (outputDir != null) {
            jTextField.setText(outputDir);
        }
    }
    
   private void showToolChain(Vector<Object> v) {
       if (v.contains("IA32")) {
           jCheckBox1.setSelected(true);
       }
       else{
           jCheckBox1.setSelected(false);
       }
       if (v.contains("X64")) {
           jCheckBox2.setSelected(true);
       }
       else{
           jCheckBox2.setSelected(false);
       }
       if (v.contains("IPF")) {
           jCheckBox3.setSelected(true);
       }
       else{
           jCheckBox3.setSelected(false);
       }
       if (v.contains("EBC")) {
           jCheckBox.setSelected(true);
       }
       else{
           jCheckBox.setSelected(false);
       }
       if (v.contains("ARM")) {
           jCheckBox5.setSelected(true);
       }
       else{
           jCheckBox5.setSelected(false);
       }
       if (v.contains("PPC")) {
           jCheckBox6.setSelected(true);
       }
       else{
           jCheckBox6.setSelected(false);
       }
   }
   
   private void getToolChain(Vector<Object> v) {
       if (docConsole != null){
           docConsole.setSaved(false);
       }
       v.removeAllElements();
       if (jCheckBox1.isSelected()) {
           v.add("IA32");
       }
       if (jCheckBox2.isSelected()) {
           v.add("X64");
       }
       if (jCheckBox3.isSelected()) {
           v.add("IPF");
       }
       if (jCheckBox.isSelected()) {
           v.add("EBC");
       }
       if (jCheckBox5.isSelected()) {
           v.add("ARM");
       }
       if (jCheckBox6.isSelected()) {
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
            jContentPane.add(getJPanel(), java.awt.BorderLayout.EAST);
            jContentPane.add(getJPanel1(), java.awt.BorderLayout.SOUTH);
            jContentPane.add(getJPanel2(), java.awt.BorderLayout.WEST);
            jContentPane.add(getJPanel3(), java.awt.BorderLayout.NORTH);
            jContentPane.add(getJTabbedPane(), java.awt.BorderLayout.CENTER);
        }
        return jContentPane;
    }

    /**
     * This method initializes jPanel4	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel4() {
        if (jPanel4 == null) {
            jPanel4 = new JPanel();
            jPanel4.setLayout(new BorderLayout());
            jPanel4.add(getJPanel5(), java.awt.BorderLayout.NORTH);
            jPanel4.add(getJPanel6(), java.awt.BorderLayout.SOUTH);
            jPanel4.add(getJPanel7(), java.awt.BorderLayout.CENTER);
        }
        return jPanel4;
    }

    /**
     * This method initializes jPanel5	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel5() {
        if (jPanel5 == null) {
            jLabel = new JLabel();
            jLabel.setText("Supported Archs");
            FlowLayout flowLayout2 = new FlowLayout();
            flowLayout2.setAlignment(FlowLayout.LEFT);
            flowLayout2.setHgap(12);
            jPanel5 = new JPanel();
            jPanel5.setLayout(flowLayout2);
            jPanel5.add(jLabel, null);
            jPanel5.add(getJCheckBox1(), null);
            jPanel5.add(getJCheckBox2(), null);
            jPanel5.add(getJCheckBox3(), null);
            jPanel5.add(getJCheckBox(), null);
            jPanel5.add(getJCheckBox5(), null);
            jPanel5.add(getJCheckBox6(), null);
        }
        return jPanel5;
    }

    /**
     * This method initializes jCheckBox1	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBox1() {
        if (jCheckBox1 == null) {
            jCheckBox1 = new JCheckBox();
            jCheckBox1.setText("IA32");
            jCheckBox1.addItemListener(new java.awt.event.ItemListener() {
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
        return jCheckBox1;
    }

    /**
     * This method initializes jCheckBox2	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBox2() {
        if (jCheckBox2 == null) {
            jCheckBox2 = new JCheckBox();
            jCheckBox2.setText("X64");
            jCheckBox2.addItemListener(new java.awt.event.ItemListener() {
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
        return jCheckBox2;
    }

    /**
     * This method initializes jCheckBox3	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBox3() {
        if (jCheckBox3 == null) {
            jCheckBox3 = new JCheckBox();
            jCheckBox3.setText("IPF");
            jCheckBox3.addItemListener(new java.awt.event.ItemListener() {
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
        return jCheckBox3;
    }
    
    /**
     * This method initializes jTable   
     *  
     * @return javax.swing.JTable   
     */
    private JTable getJTable() {
        if (jTable == null) {
            imageEntryPointTableModel = new TargetTableModel();
            imageEntryPointTableModel.addColumn("Build Target");
            jTable = new JTable(imageEntryPointTableModel);
            jTable.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
            
            jTable.getModel().addTableModelListener(new TableModelListener() {
                public void tableChanged(TableModelEvent arg0) {
                    // TODO Auto-generated method stub
                    if (jTable.getSelectedRow() < 0) {
                        return;
                    }
                    TableModel m = (TableModel)arg0.getSource();
                    if (arg0.getType() == TableModelEvent.UPDATE){
                        //ToDo Data Validition check.
                        Vector<Object> v = new Vector<Object>();
                        for (int i = 0; i < jTable.getRowCount(); ++i) {
                            v.add(m.getValueAt(i, 0));
                        }
                        docConsole.setSaved(false);
                        ffc.setPlatformDefsBuildTargets(v);
                    }
                }
            });
        }
        return jTable;
    }

    /**
     * This method initializes jPanel6	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel6() {
        if (jPanel6 == null) {
            FlowLayout flowLayout3 = new FlowLayout();
            flowLayout3.setAlignment(FlowLayout.LEFT);
            flowLayout3.setHgap(20);
            jPanel6 = new JPanel();
            jPanel6.setPreferredSize(new java.awt.Dimension(10,200));
            jPanel6.setLayout(flowLayout3);
            jPanel6.add(getJPanel8(), null);
        }
        return jPanel6;
    }

    /**
     * This method initializes jCheckBox4	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBox4() {
        if (jCheckBox4 == null) {
            jCheckBox4 = new JCheckBox();
            jCheckBox4.setText("Intermediate Directories");
        }
        return jCheckBox4;
    }

    /**
     * This method initializes jComboBox	
     * 	
     * @return javax.swing.JComboBox	
     */
    private JComboBox getJComboBox() {
        if (jComboBox == null) {
            jComboBox = new JComboBox();
            jComboBox.setPreferredSize(new Dimension(100, 20));
            jComboBox.addItem("UNIFIED");
            jComboBox.addItem("MODULE");
            
            jComboBox.setSelectedIndex(0);
            jComboBox.addItemListener(new java.awt.event.ItemListener() {
                public void itemStateChanged(java.awt.event.ItemEvent e) {
                    if (docConsole != null){
                        docConsole.setSaved(false);
                    }
                    ffc.setPlatformDefsInterDir(jComboBox.getSelectedItem()+"");
                }
            });
        }
        return jComboBox;
    }

    /**
     * This method initializes jPanel7	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel7() {
        if (jPanel7 == null) {
            jLabel2 = new JLabel();
            jLabel2.setPreferredSize(new Dimension(109, 16));
            jLabel2.setText("Build Targets");
            FlowLayout flowLayout4 = new FlowLayout();
            flowLayout4.setAlignment(FlowLayout.LEFT);
            flowLayout4.setHgap(20);
            jPanel7 = new JPanel();
            jPanel7.setPreferredSize(new Dimension(972, 100));
            jPanel7.setLayout(flowLayout4);
            jPanel7.add(jLabel2, null);
            jPanel7.add(getJTextField1(), null);
            jPanel7.add(getJButton2(), null);
            jPanel7.add(getJButton3(), null);
            jPanel7.add(getJScrollPane2(), null);
            jPanel7.add(getJPanel9(), null);
            jPanel7.add(getJScrollPane3(), null);
        }
        return jPanel7;
    }

    /**
     * This method initializes jTextField1	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextField1() {
        if (jTextField1 == null) {
            jTextField1 = new JTextField();
            jTextField1.setPreferredSize(new Dimension(150, 20));
        }
        return jTextField1;
    }

    /**
     * This method initializes jButton2	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButton2() {
        if (jButton2 == null) {
            jButton2 = new JButton();
            jButton2.setPreferredSize(new Dimension(70, 20));
            jButton2.setText("Add");
            jButton2.addActionListener(new java.awt.event.ActionListener() {
                public void actionPerformed(java.awt.event.ActionEvent e) {
                    if (jTextField1.getText().length() > 0) {
                        String[] row = {jTextField1.getText()};
                        imageEntryPointTableModel.addRow(row);
                        Vector<Object> v = new Vector<Object>();
                        for (int i = 0; i < jTable.getRowCount(); ++i) {
                            v.add(imageEntryPointTableModel.getValueAt(i, 0));
                        }
                        docConsole.setSaved(false);
                        ffc.setPlatformDefsBuildTargets(v);
                    }
                }
            });
        }
        return jButton2;
    }

    /**
     * This method initializes jButton3	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButton3() {
        if (jButton3 == null) {
            jButton3 = new JButton();
            jButton3.setPreferredSize(new Dimension(70, 20));
            jButton3.setText("Delete");
            jButton3.setVisible(false);
        }
        return jButton3;
    }

    /**
     * This method initializes jScrollPane2	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPane2() {
        if (jScrollPane2 == null) {
            jScrollPane2 = new JScrollPane();
            jScrollPane2.setPreferredSize(new Dimension(453, 100));
            jScrollPane2.setViewportView(getJTable());
        }
        return jScrollPane2;
    }

    /**
     * This method initializes jScrollPane3	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPane3() {
        if (jScrollPane3 == null) {
            jScrollPane3 = new JScrollPane();
            jScrollPane3.setPreferredSize(new java.awt.Dimension(453,100));
            jScrollPane3.setViewportView(getJTable2());
        }
        return jScrollPane3;
    }

    /**
     * This method initializes jTable2	
     * 	
     * @return javax.swing.JTable	
     */
    private JTable getJTable2() {
        if (jTable2 == null) {
            skuInfoTableModel = new SkuInfoTableModel();
            skuInfoTableModel.addColumn("SKU ID");
            skuInfoTableModel.addColumn("Name");
            jTable2 = new JTable(skuInfoTableModel);
            
            jTable2.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
            
            jTable2.getModel().addTableModelListener(new TableModelListener() {
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
        return jTable2;
    }

    /**
     * This method initializes jCheckBox	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBox() {
        if (jCheckBox == null) {
            jCheckBox = new JCheckBox();
            jCheckBox.setPreferredSize(new java.awt.Dimension(50,20));
            jCheckBox.setText("EBC");
            jCheckBox.addItemListener(new java.awt.event.ItemListener() {
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
        return jCheckBox;
    }

    /**
     * This method initializes jCheckBox5	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBox5() {
        if (jCheckBox5 == null) {
            jCheckBox5 = new JCheckBox();
            jCheckBox5.setPreferredSize(new java.awt.Dimension(52,20));
            jCheckBox5.setText("ARM");
            jCheckBox5.setVisible(false);
            jCheckBox5.addItemListener(new java.awt.event.ItemListener() {
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
        return jCheckBox5;
    }

    /**
     * This method initializes jCheckBox6	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBox6() {
        if (jCheckBox6 == null) {
            jCheckBox6 = new JCheckBox();
            jCheckBox6.setPreferredSize(new Dimension(50, 20));
            jCheckBox6.setText("PPC");
            jCheckBox6.setVisible(false);
            jCheckBox6.addItemListener(new java.awt.event.ItemListener() {
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
        return jCheckBox6;
    }

    /**
     * This method initializes jPanel8	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel8() {
        if (jPanel8 == null) {
            FlowLayout flowLayout1 = new FlowLayout();
            flowLayout1.setAlignment(java.awt.FlowLayout.LEFT);
            jLabel1 = new JLabel();
            jLabel1.setText("Output Directory");
            jLabel7 = new JLabel();
            jLabel7.setPreferredSize(new java.awt.Dimension(150,20));
            jLabel7.setText("");
            jPanel8 = new JPanel();
            jPanel8.setLayout(flowLayout1);
            jPanel8.setPreferredSize(new java.awt.Dimension(450,100));
            jPanel8.setBorder(javax.swing.BorderFactory.createEtchedBorder(javax.swing.border.EtchedBorder.RAISED));
            jPanel8.add(getJCheckBox4(), null);
            jPanel8.add(getJComboBox(), null);
            jPanel8.add(jLabel7, null);
            jPanel8.add(jLabel1, null);
            jPanel8.add(getJTextField(), null);
        }
        return jPanel8;
    }

    /**
     * This method initializes jTextField	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextField() {
        if (jTextField == null) {
            jTextField = new JTextField();
            jTextField.setPreferredSize(new java.awt.Dimension(300,20));
            jTextField.addFocusListener(new java.awt.event.FocusAdapter() {
                public void focusLost(java.awt.event.FocusEvent e) {
                    docConsole.setSaved(false);
                    ffc.setPlatformDefsOutputDir(jTextField.getText());
                }
            });
        }
        return jTextField;
    }

    /**
     * This method initializes jPanel9	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel9() {
        if (jPanel9 == null) {
            jLabel6 = new JLabel();
            jLabel6.setPreferredSize(new Dimension(280, 20));
            jLabel6.setText("                                                 ");
            jLabel5 = new JLabel();
            jLabel5.setPreferredSize(new Dimension(40, 20));
            jLabel5.setText("Name");
            jLabel4 = new JLabel();
            jLabel4.setPreferredSize(new Dimension(20, 20));
            jLabel4.setText("ID");
            jLabel3 = new JLabel();
            jLabel3.setPreferredSize(new java.awt.Dimension(150,20));
            jLabel3.setText("SKU Information");
            jPanel9 = new JPanel();
            jPanel9.setPreferredSize(new java.awt.Dimension(450,70));
            jPanel9.add(jLabel3, null);
            jPanel9.add(jLabel6, null);
            jPanel9.add(jLabel4, null);
            jPanel9.add(getJTextField2(), null);
            jPanel9.add(jLabel5, null);
            jPanel9.add(getJTextField3(), null);
            jPanel9.add(getJButton(), null);
            jPanel9.add(getJButton1(), null);
        }
        return jPanel9;
    }

    /**
     * This method initializes jTextField2	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextField2() {
        if (jTextField2 == null) {
            jTextField2 = new JTextField();
            jTextField2.setPreferredSize(new Dimension(50, 20));
        }
        return jTextField2;
    }

    /**
     * This method initializes jTextField3	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextField3() {
        if (jTextField3 == null) {
            jTextField3 = new JTextField();
            jTextField3.setPreferredSize(new Dimension(150, 20));
        }
        return jTextField3;
    }

    /**
     * This method initializes jButton	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButton() {
        if (jButton == null) {
            jButton = new JButton();
            jButton.setPreferredSize(new Dimension(70, 20));
            jButton.setText("Add");
            jButton.addActionListener(new java.awt.event.ActionListener() {
                public void actionPerformed(java.awt.event.ActionEvent e) {
                    if (jTextField2.getText().length() > 0) {
                        String[] row = {jTextField2.getText(), jTextField3.getText()};
                        skuInfoTableModel.addRow(row);
                        docConsole.setSaved(false);
                        ffc.genPlatformDefsSkuInfo(row[0], row[1]);
                    }
                }
            });
        }
        return jButton;
    }

    /**
     * This method initializes jButton1	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButton1() {
        if (jButton1 == null) {
            jButton1 = new JButton();
            jButton1.setPreferredSize(new Dimension(70, 20));
            jButton1.setText("Delete");
            jButton1.addActionListener(new java.awt.event.ActionListener() {
                public void actionPerformed(java.awt.event.ActionEvent e) {
                    if (jTable2.isEditing()) {
                        jTable2.getCellEditor().stopCellEditing();
                    }
                    if (jTable2.getSelectedRow() < 1) {
                        return;
                    }
                    docConsole.setSaved(false);
                    ffc.removePlatformDefsSkuInfo(jTable2.getSelectedRow());
                    skuInfoTableModel.removeRow(jTable2.getSelectedRow());
                }
            });
        }
        return jButton1;
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


