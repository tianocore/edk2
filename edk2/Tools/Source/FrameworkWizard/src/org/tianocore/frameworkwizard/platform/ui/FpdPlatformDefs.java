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

import javax.swing.DefaultCellEditor;
import javax.swing.DefaultListModel;
import javax.swing.JFrame;
import javax.swing.JTabbedPane;
import javax.swing.JButton;

import org.tianocore.PlatformSurfaceAreaDocument;
import org.tianocore.frameworkwizard.common.ui.IInternalFrame;


import java.awt.FlowLayout;


import javax.swing.JCheckBox;
import javax.swing.JTextField;
import javax.swing.JLabel;
import javax.swing.JScrollPane;
import javax.swing.JTable;
import javax.swing.event.InternalFrameAdapter;
import javax.swing.event.InternalFrameEvent;
import javax.swing.event.ListSelectionEvent;
import javax.swing.event.ListSelectionListener;
import javax.swing.event.TableModelEvent;
import javax.swing.event.TableModelListener;
import javax.swing.table.DefaultTableModel;
import javax.swing.table.TableColumn;
import javax.swing.table.TableModel;
import javax.swing.JComboBox;
import java.awt.Dimension;
import javax.swing.JRadioButton;
import javax.swing.JTextArea;
import java.awt.CardLayout;

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
    private JPanel jPanelFvImageW = null;
    private JScrollPane jScrollPane = null;
    private DefaultTableModel imageEntryPointTableModel = null;
    private DefaultTableModel outputDirectoryTableModel = null;
    
    private DefaultTableModel skuInfoTableModel = null;
    private DefaultTableModel fdfBlocksTableModel = null;
    private DefaultTableModel fdfRegionsTableModel = null;
    private DefaultTableModel fdfSubRegionsTableModel = null;
    
    private JLabel jLabel17 = null;
    private DefaultListModel listModel = new DefaultListModel();
    private FpdFileContents ffc = null;
    private JPanel jPanel4 = null;
    private JPanel jPanel5 = null;
    private JLabel jLabel = null;
    private JCheckBox jCheckBox1 = null;
    private JCheckBox jCheckBox2 = null;
    private JCheckBox jCheckBox3 = null;
    private JPanel jPanel6 = null;
    private JLabel jLabel1 = null;
    private JCheckBox jCheckBox4 = null;
    private JComboBox jComboBox = null;
    private JTextField jTextField = null;
    private JTable jTable = null;
    private JPanel jPanel7 = null;
    private JLabel jLabel2 = null;
    private JTextField jTextField1 = null;
    private JButton jButton2 = null;
    private JButton jButton3 = null;
    private JScrollPane jScrollPane2 = null;
    private JTable jTable1 = null;
    private JLabel jLabel3 = null;
    private JScrollPane jScrollPane3 = null;
    private JTable jTable2 = null;
    private JLabel jLabel4 = null;
    private JTextField jTextField2 = null;
    private JLabel jLabel5 = null;
    private JTextField jTextField3 = null;
    private JLabel jLabel6 = null;
    private JButton jButton4 = null;
    private JButton jButton5 = null;
    private JLabel jLabel7 = null;
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
     * This method initializes jPanelFvImageW	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelFvImageW() {
        if (jPanelFvImageW == null) {
            jPanelFvImageW = new JPanel();
            jPanelFvImageW.setPreferredSize(new java.awt.Dimension(10,2));
        }
        return jPanelFvImageW;
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
            jLabel.setText("Tool Chain Families");
            FlowLayout flowLayout2 = new FlowLayout();
            flowLayout2.setAlignment(FlowLayout.LEFT);
            flowLayout2.setHgap(20);
            jPanel5 = new JPanel();
            jPanel5.setLayout(flowLayout2);
            jPanel5.add(jLabel, null);
            jPanel5.add(getJCheckBox1(), null);
            jPanel5.add(getJCheckBox2(), null);
            jPanel5.add(getJCheckBox3(), null);
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
            jCheckBox1.setText("MSFT");
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
            jCheckBox2.setText("GCC");
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
            jCheckBox3.setText("INTC");
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
            imageEntryPointTableModel = new DefaultTableModel();
            imageEntryPointTableModel.addColumn("Build Target");
        }
        return jTable;
    }

    /**
     * This method initializes jTable1  
     *  
     * @return javax.swing.JTable   
     */
    private JTable getJTable1() {
        if (jTable1 == null) {
            outputDirectoryTableModel = new DefaultTableModel();
            outputDirectoryTableModel.addColumn("Output Directory");
            outputDirectoryTableModel.addColumn("Intermediate Directories");
            
            TableColumn imDirCol = jTable1.getColumnModel().getColumn(1);
            JComboBox cb = new JComboBox();
            cb.addItem("MODULE");
            cb.addItem("UNIFIED");
            imDirCol.setCellEditor(new DefaultCellEditor(cb));
        }
        return jTable1;
    }


    /**
     * This method initializes jPanel6	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel6() {
        if (jPanel6 == null) {
            jLabel7 = new JLabel();
            jLabel7.setPreferredSize(new java.awt.Dimension(100,20));
            jLabel7.setText("");
            jLabel1 = new JLabel();
            jLabel1.setText("Output Directory");
            FlowLayout flowLayout3 = new FlowLayout();
            flowLayout3.setAlignment(FlowLayout.LEFT);
            flowLayout3.setHgap(20);
            jPanel6 = new JPanel();
            jPanel6.setPreferredSize(new java.awt.Dimension(10,250));
            jPanel6.setLayout(flowLayout3);
            jPanel6.add(getJCheckBox4(), null);
            jPanel6.add(getJComboBox(), null);
            jPanel6.add(jLabel7, null);
            jPanel6.add(jLabel1, null);
            jPanel6.add(getJTextField(), null);
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
            jComboBox.addItem("MODULE");
            jComboBox.addItem("UNIFIED");
            jComboBox.setSelectedIndex(0);
        }
        return jComboBox;
    }

    /**
     * This method initializes jTextField	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextField() {
        if (jTextField == null) {
            jTextField = new JTextField();
            jTextField.setPreferredSize(new java.awt.Dimension(350,20));
        }
        return jTextField;
    }

    /**
     * This method initializes jPanel7	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel7() {
        if (jPanel7 == null) {
            jLabel6 = new JLabel();
            jLabel6.setPreferredSize(new java.awt.Dimension(280,20));
            jLabel6.setText("                                                 ");
            jLabel5 = new JLabel();
            jLabel5.setPreferredSize(new java.awt.Dimension(40,20));
            jLabel5.setText("Name");
            jLabel4 = new JLabel();
            jLabel4.setPreferredSize(new java.awt.Dimension(20,20));
            jLabel4.setText("ID");
            jLabel3 = new JLabel();
            jLabel3.setPreferredSize(new java.awt.Dimension(109,20));
            jLabel3.setText("SKU Information");
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
            jPanel7.add(jLabel3, null);
            jPanel7.add(jLabel4, null);
            jPanel7.add(getJTextField2(), null);
            jPanel7.add(jLabel5, null);
            jPanel7.add(getJTextField3(), null);
            jPanel7.add(getJScrollPane3(), null);
            jPanel7.add(jLabel6, null);
            jPanel7.add(getJButton4(), null);
            jPanel7.add(getJButton5(), null);
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
            skuInfoTableModel = new DefaultTableModel();
            skuInfoTableModel.addColumn("SKU ID");
            skuInfoTableModel.addColumn("Name");
            jTable2 = new JTable(skuInfoTableModel);
        }
        return jTable2;
    }

    /**
     * This method initializes jTextField2	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextField2() {
        if (jTextField2 == null) {
            jTextField2 = new JTextField();
            jTextField2.setPreferredSize(new java.awt.Dimension(50,20));
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
     * This method initializes jButton4	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButton4() {
        if (jButton4 == null) {
            jButton4 = new JButton();
            jButton4.setPreferredSize(new Dimension(70, 20));
            jButton4.setText("Add");
        }
        return jButton4;
    }

    /**
     * This method initializes jButton5	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButton5() {
        if (jButton5 == null) {
            jButton5 = new JButton();
            jButton5.setPreferredSize(new Dimension(70, 20));
            jButton5.setText("Delete");
        }
        return jButton5;
    }


}  //  @jve:decl-index=0:visual-constraint="10,10"


