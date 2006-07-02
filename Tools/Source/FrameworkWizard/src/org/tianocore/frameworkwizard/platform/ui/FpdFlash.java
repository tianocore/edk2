/** @file
  Java class FpdFlash is GUI for Flash element operation in SPD file.
 
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

import javax.swing.AbstractAction;
import javax.swing.DefaultCellEditor;
import javax.swing.JFileChooser;
import javax.swing.JFrame;
import javax.swing.JTabbedPane;
import javax.swing.JButton;
import javax.swing.ListSelectionModel;

import org.tianocore.FvImagesDocument;
import org.tianocore.PlatformSurfaceAreaDocument;
import org.tianocore.frameworkwizard.common.ui.IInternalFrame;
import org.tianocore.frameworkwizard.common.ui.StarLabel;


import java.awt.FlowLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ComponentAdapter;
import java.awt.event.ComponentEvent;
import java.awt.event.ItemEvent;
import java.awt.event.ItemListener;
import java.io.File;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.Set;

import javax.swing.JCheckBox;
import javax.swing.JTextField;
import java.awt.GridLayout;
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

public class FpdFlash extends IInternalFrame {

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
    private JPanel jPanelFvImages = null;
    private JButton jButtonOk = null;
    private JButton jButtonCancel = null;
    private JPanel jPanelFvImageN = null;
    private JPanel jPanelFvImageS = null;
    private JCheckBox jCheckBox1 = null;
    private JLabel jLabel = null;
    private JTextField jTextField = null;
    private JLabel jLabel1 = null;
    private JTextField jTextField1 = null;
    private JButton jButton = null;
    private JScrollPane jScrollPane1 = null;
    private JTable jTable = null;
    private JPanel jPanel4 = null;
    private JButton jButton1 = null;
    private DefaultTableModel fvPropertyTableModel = null;
    private DefaultTableModel fvImageNameTableModel = null;
    private ImageParaTableModel fvImageParaTableModel = null;
    private DefaultTableModel fvOptionTableModel = null;
    private JPanel jPanelFvImageC = null;
    private JCheckBox jCheckBox2 = null;
    private JLabel jLabel2 = null;
    private JComboBox jComboBox = null;
    private JLabel jLabel3 = null;
    private JLabel jLabel4 = null;
    private JTextField jTextField2 = null;
    private JButton jButton2 = null;
    private JScrollPane jScrollPane2 = null;
    private JTable jTable1 = null;
    private JButton jButton3 = null;
    private JPanel jPanel5 = null;
    private JButton jButton4 = null;
    private JScrollPane jScrollPane3 = null;
    private JTable jTable2 = null;
    private JButton jButton6 = null;
    private JCheckBox jCheckBox3 = null;
    private JPanel jPanel6 = null;
    
    private FpdFileContents ffc = null;
    private JPanel jPanel7 = null;
    private JCheckBox jCheckBox = null;
    private JTextField jTextField3 = null;
    private JButton jButton5 = null;
    private JLabel jLabel5 = null;
    private JTextField jTextField4 = null;
    private JLabel jLabel6 = null;
    private JTextField jTextField5 = null;
    private JPanel jPanel8 = null;
    private JButton jButton7 = null;
    private JButton jButton8 = null;
    private JScrollPane jScrollPane = null;
    private JTable jTable3 = null;
    private JButton jButton9 = null;
    public FpdFlash() {
        super();
        // TODO Auto-generated constructor stub

        initialize();
        this.setBounds(new java.awt.Rectangle(0, 0, 500, 370));
        this.setVisible(true);
    }

    public FpdFlash(PlatformSurfaceAreaDocument.PlatformSurfaceArea fpd){
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
            jTabbedPane.addTab("FV Images", null, getJPanelFvImages(), null);
            jTabbedPane.addTab("Flash Definition File", null, getJPanel7(), null);
            
        }
        return jTabbedPane;
    }

    /**
     * This method initializes jPanelFvImages	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelFvImages() {
        if (jPanelFvImages == null) {
            jPanelFvImages = new JPanel();
            jPanelFvImages.setLayout(new BorderLayout());
            jPanelFvImages.add(getJPanelFvImageN(), java.awt.BorderLayout.NORTH);
//            jPanelFvImages.add(getJPanelFvImageW(), java.awt.BorderLayout.WEST);
            jPanelFvImages.add(getJPanelFvImageS(), java.awt.BorderLayout.SOUTH);
            jPanelFvImages.add(getJPanelFvImageC(), java.awt.BorderLayout.CENTER);
            
        }
        return jPanelFvImages;
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
     * This method initializes jPanelFvImageN	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelFvImageN() {
        if (jPanelFvImageN == null) {
            jLabel1 = new JLabel();
            jLabel1.setText("Value");
            jLabel1.setEnabled(false);
            jLabel1.setPreferredSize(new java.awt.Dimension(38,20));
            jLabel = new JLabel();
            jLabel.setText("Name");
            jLabel.setEnabled(false);
            jLabel.setPreferredSize(new java.awt.Dimension(38,20));
            FlowLayout flowLayout2 = new FlowLayout();
            flowLayout2.setAlignment(java.awt.FlowLayout.CENTER);
            flowLayout2.setHgap(15);
            jPanelFvImageN = new JPanel();
            jPanelFvImageN.setPreferredSize(new java.awt.Dimension(576,100));
            jPanelFvImageN.setBorder(javax.swing.BorderFactory.createBevelBorder(javax.swing.border.BevelBorder.RAISED));
            jPanelFvImageN.setLayout(flowLayout2);
            jPanelFvImageN.add(getJCheckBox1(), null);
            jPanelFvImageN.add(jLabel, null);
            jPanelFvImageN.add(getJTextField(), null);
            jPanelFvImageN.add(jLabel1, null);
            jPanelFvImageN.add(getJTextField1(), null);
            jPanelFvImageN.add(getJScrollPane1(), null);
            jPanelFvImageN.add(getJPanel4(), null);
        }
        return jPanelFvImageN;
    }

    /**
     * This method initializes jPanelFvImageS	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelFvImageS() {
        if (jPanelFvImageS == null) {
            GridLayout gridLayout2 = new GridLayout();
            gridLayout2.setRows(1);
            jPanelFvImageS = new JPanel();
            jPanelFvImageS.setPreferredSize(new java.awt.Dimension(480,190));
            jPanelFvImageS.setLayout(gridLayout2);
            jPanelFvImageS.add(getJScrollPane3(), null);
        }
        return jPanelFvImageS;
    }


    /**
     * This method initializes jCheckBox1	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBox1() {
        if (jCheckBox1 == null) {
            jCheckBox1 = new JCheckBox();
            jCheckBox1.setText("FV Properties");
            jCheckBox1.addItemListener(new ItemListener(){

                public void itemStateChanged(ItemEvent arg0) {
                    // TODO Auto-generated method stub
                    boolean seleted = jCheckBox1.isSelected();
                    
                        jLabel.setEnabled(seleted);
                        jTextField.setEnabled(seleted);
                        jLabel1.setEnabled(seleted);
                        jTextField1.setEnabled(seleted);
                        jTable.setEnabled(seleted);
                        jButton.setEnabled(seleted);
                        jButton1.setEnabled(seleted);
                    
                   
                }
                
            });
        }
        return jCheckBox1;
    }


    /**
     * This method initializes jTextField	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextField() {
        if (jTextField == null) {
            jTextField = new JTextField();
            jTextField.setPreferredSize(new java.awt.Dimension(100,20));
            jTextField.setEnabled(false);
        }
        return jTextField;
    }


    /**
     * This method initializes jTextField1	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextField1() {
        if (jTextField1 == null) {
            jTextField1 = new JTextField();
            jTextField1.setPreferredSize(new java.awt.Dimension(100,20));
            jTextField1.setEnabled(false);
        }
        return jTextField1;
    }


    /**
     * This method initializes jButton	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButton() {
        if (jButton == null) {
            jButton = new JButton();
            jButton.setPreferredSize(new java.awt.Dimension(80,20));
            jButton.setEnabled(false);
            jButton.setText("Add");
            jButton.addActionListener(new AbstractAction() {
                /**
                 * 
                 */
                private static final long serialVersionUID = 1L;

                public void actionPerformed(ActionEvent e) {
                    if (jTextField.getText().length() > 0 && jTextField1.getText().length() > 0){
                        String[] row = {jTextField.getText(), jTextField1.getText()};                        
                        fvPropertyTableModel.addRow(row);
                        ffc.genFvImagesNameValue(row[0], row[1]);
                    }
                }
            });
        }
        return jButton;
    }


    /**
     * This method initializes jScrollPane1	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPane1() {
        if (jScrollPane1 == null) {
            jScrollPane1 = new JScrollPane();
            jScrollPane1.setPreferredSize(new java.awt.Dimension(350,55));
            jScrollPane1.setViewportView(getJTable());
        }
        return jScrollPane1;
    }


    /**
     * This method initializes jTable	
     * 	
     * @return javax.swing.JTable	
     */
    private JTable getJTable() {
        if (jTable == null) {
            fvPropertyTableModel = new DefaultTableModel();
            jTable = new JTable(fvPropertyTableModel);
            fvPropertyTableModel.addColumn("Name");
            fvPropertyTableModel.addColumn("Value");
            jTable.setRowHeight(20);
            jTable.setEnabled(false);
            
            jTable.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
            jTable.getSelectionModel().addListSelectionListener(new ListSelectionListener(){
                public void valueChanged(ListSelectionEvent e) {
                    if (e.getValueIsAdjusting()){
                        return;
                    }
                    ListSelectionModel lsm = (ListSelectionModel)e.getSource();
                    if (lsm.isSelectionEmpty()) {
                        return;
                    }
                    else{
//                        selectedRow = lsm.getMinSelectionIndex();
                    }
                }
            });
            
            jTable.getModel().addTableModelListener(new TableModelListener(){
                public void tableChanged(TableModelEvent arg0) {
                    // TODO Auto-generated method stub
                    int row = arg0.getFirstRow();
                    TableModel m = (TableModel)arg0.getSource();
                    if (arg0.getType() == TableModelEvent.UPDATE){
                        
                        String name = m.getValueAt(row, 0) + "";
                        String value = m.getValueAt(row, 1) + "";

                        ffc.updateFvImagesNameValue(row, name, value);
                    }
                }
            });
        }
        return jTable;
    }


    /**
     * This method initializes jPanel4	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel4() {
        if (jPanel4 == null) {
            jPanel4 = new JPanel();
            jPanel4.setPreferredSize(new java.awt.Dimension(80,55));
            
            jPanel4.add(getJButton(), null);
            jPanel4.add(getJButton1(), null);
        }
        return jPanel4;
    }


    /**
     * This method initializes jButton1	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButton1() {
        if (jButton1 == null) {
            jButton1 = new JButton();
            jButton1.setPreferredSize(new java.awt.Dimension(80,20));
            jButton1.setEnabled(false);
            jButton1.setText("Delete");
            jButton1.addActionListener(new AbstractAction() {
                /**
                 * 
                 */
                private static final long serialVersionUID = 1L;

                public void actionPerformed(ActionEvent e) {
                    if (jTable.getSelectedRow() >= 0){
                        fvPropertyTableModel.removeRow(jTable.getSelectedRow());
                        ffc.removeFvImagesNameValue(jTable.getSelectedRow());
                    }
                }
            });
        }
        return jButton1;
    }


    /**
     * This method initializes jPanelFvImageC	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelFvImageC() {
        if (jPanelFvImageC == null) {
            jLabel4 = new JLabel();
            jLabel4.setPreferredSize(new Dimension(38, 20));
            jLabel4.setEnabled(false);
            jLabel4.setText("Name");
            jLabel3 = new JLabel();
            jLabel3.setText("FV Image Names");
            jLabel3.setEnabled(false);
            jLabel3.setPreferredSize(new java.awt.Dimension(150,20));
            jLabel2 = new JLabel();
            jLabel2.setText("Type");
            jLabel2.setEnabled(false);
            jLabel2.setPreferredSize(new java.awt.Dimension(70,20));
            FlowLayout flowLayout3 = new FlowLayout();
            flowLayout3.setAlignment(java.awt.FlowLayout.LEFT);
            flowLayout3.setHgap(5);
            jPanelFvImageC = new JPanel();
            jPanelFvImageC.setLayout(flowLayout3);
            jPanelFvImageC.add(getJPanel6(), null);
            
            jPanelFvImageC.add(getJPanel5(), null);
            
        }
        return jPanelFvImageC;
    }


    /**
     * This method initializes jCheckBox2	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBox2() {
        if (jCheckBox2 == null) {
            jCheckBox2 = new JCheckBox();
            jCheckBox2.setText("FV Image Parameters");
            jCheckBox2.setPreferredSize(new java.awt.Dimension(200,20));
            jCheckBox2.addItemListener(new ItemListener(){

                public void itemStateChanged(ItemEvent arg0) {
                    // TODO Auto-generated method stub
                    boolean seleted = jCheckBox2.isSelected();
                    
                        jLabel2.setEnabled(seleted);
                        jLabel3.setEnabled(seleted);
                        jLabel4.setEnabled(seleted);
                        jComboBox.setEnabled(seleted);
                        jTextField2.setEnabled(seleted);
                        
                        jTable1.setEnabled(seleted);
                        jButton2.setEnabled(seleted);
                        jButton3.setEnabled(seleted);
                    
                   
                }
                
            });
        }
        return jCheckBox2;
    }


    /**
     * This method initializes jComboBox	
     * 	
     * @return javax.swing.JComboBox	
     */
    private JComboBox getJComboBox() {
        if (jComboBox == null) {
            jComboBox = new JComboBox();
            jComboBox.addItem("ImageName");
            jComboBox.addItem("Attributes");
            jComboBox.addItem("Options");
            jComboBox.addItem("Components");
            jComboBox.setPreferredSize(new java.awt.Dimension(180,20));
            jComboBox.setEnabled(false);
            jComboBox.addItemListener(new ItemListener() {

                public void itemStateChanged(ItemEvent arg0) {
                    // TODO disable attribute settings when ValidImageNames selected.
                    
                }
                
            });
        }
        return jComboBox;
    }


    /**
     * This method initializes jTextField2	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextField2() {
        if (jTextField2 == null) {
            jTextField2 = new JTextField();
            jTextField2.setPreferredSize(new java.awt.Dimension(140,20));
            jTextField2.setEnabled(false);
        }
        return jTextField2;
    }


    /**
     * This method initializes jButton2	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButton2() {
        if (jButton2 == null) {
            jButton2 = new JButton();
            jButton2.setPreferredSize(new Dimension(80, 20));
            jButton2.setEnabled(false);
            jButton2.setText("Add");
            jButton2.addActionListener(new AbstractAction() {
                /**
                 * 
                 */
                private static final long serialVersionUID = 1L;

                public void actionPerformed(java.awt.event.ActionEvent e) {
                    if (jTextField2.getText().length() > 0){
                        String[] row = {jTextField2.getText()};                        
                        fvImageNameTableModel.addRow(row);
                    }
                }
            });
        }
        return jButton2;
    }


    /**
     * This method initializes jScrollPane2	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPane2() {
        if (jScrollPane2 == null) {
            jScrollPane2 = new JScrollPane();
            jScrollPane2.setPreferredSize(new java.awt.Dimension(350,50));
            jScrollPane2.setViewportView(getJTable1());
        }
        return jScrollPane2;
    }


    /**
     * This method initializes jTable1	
     * 	
     * @return javax.swing.JTable	
     */
    private JTable getJTable1() {
        if (jTable1 == null) {
            fvImageNameTableModel = new DefaultTableModel();
            jTable1 = new JTable(fvImageNameTableModel);
            jTable1.setEnabled(false);
            fvImageNameTableModel.addColumn("FV Image Name");
        }
        return jTable1;
    }


    /**
     * This method initializes jButton3	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButton3() {
        if (jButton3 == null) {
            jButton3 = new JButton();
            jButton3.setPreferredSize(new Dimension(80, 20));
            jButton3.setEnabled(false);
            jButton3.setText("Delete");
            jButton3.addActionListener(new AbstractAction() {
                /**
                 * 
                 */
                private static final long serialVersionUID = 1L;

                public void actionPerformed(java.awt.event.ActionEvent e) {
                    if (jTable1.getSelectedRow() >= 0){
                                   
                        fvImageNameTableModel.removeRow(jTable1.getSelectedRow());
                    }
                }
            });
        }
        return jButton3;
    }



  			
  private JPanel getJPanel5() {
       			

 if (jPanel5 == null) {
     //ToDo add ButtonGroup for RadioButtons
            jLabel6 = new JLabel();
            jLabel6.setEnabled(false);
            jLabel6.setText("Value");
            jLabel6.setPreferredSize(new Dimension(38, 20));
            jLabel5 = new JLabel();
            jLabel5.setEnabled(false);
            jLabel5.setText("Name");
            jLabel5.setPreferredSize(new Dimension(38, 20));
            jPanel5 = new JPanel();
            jPanel5.setPreferredSize(new java.awt.Dimension(480,150));
            
            jPanel5.setLayout(new FlowLayout());
            
			jPanel5.setBorder(javax.swing.BorderFactory.createEtchedBorder(javax.swing.border.EtchedBorder.LOWERED));
			jPanel5.add(getJCheckBox3(), null);
			jPanel5.add(jLabel5, null);
			jPanel5.add(getJTextField4(), null);
			jPanel5.add(jLabel6, null);
			jPanel5.add(getJTextField5(), null);
			jPanel5.add(getJScrollPane(), null);
			jPanel5.add(getJPanel8(), null);
			jPanel5.add(getJButton4(), null);
			jPanel5.add(getJButton6(), null);
			jPanel5.add(getJButton9(), null);
            
            
            			

            
        }
        return jPanel5;
    }


    /**
     * This method initializes jButton4	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButton4() {
        if (jButton4 == null) {
            jButton4 = new JButton();
            jButton4.setPreferredSize(new java.awt.Dimension(120,20));
//            jButton4.setEnabled(false);
            jButton4.setText("Add FV Image");
            jButton4.addActionListener(new AbstractAction() {
                /**
                 * 
                 */
                private static final long serialVersionUID = 1L;

                public void actionPerformed(java.awt.event.ActionEvent e) {
                    if (jTable2.isEditing()) {
                        jTable2.getCellEditor().stopCellEditing();
                    }
                    if (jTable3.isEditing()) {
                        jTable3.getCellEditor().stopCellEditing();
                    }
                    if (jTable1.getRowCount()== 0){
                        return;
                    }
                    String imageName = " ";
                    for (int i = 0; i < jTable1.getRowCount(); ++i){
                        imageName += (String)jTable1.getValueAt(i, 0);
                        imageName += " ";
                    }
                    imageName = imageName.trim();
                    
                    if (!jCheckBox3.isSelected() && jComboBox.getSelectedIndex() != 0){
                        return;
                      
                    }
                    
                    LinkedHashMap<String, String> m = null;
                    if (jCheckBox3.isSelected()) {
                        m = new LinkedHashMap<String, String>();
                        getOptionNameValue(m);
                    }
                    ffc.genFvImagesFvImage(imageName.split(" "), jComboBox.getSelectedItem()+"", m);
                    
                    Object[] row = {imageName, jComboBox.getSelectedItem()};
                    fvImageParaTableModel.addRow(row); 
                }
            });
        }
        return jButton4;
    }


    /**
     * This method initializes jScrollPane3	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPane3() {
        if (jScrollPane3 == null) {
            jScrollPane3 = new JScrollPane();
            jScrollPane3.setPreferredSize(new java.awt.Dimension(480,150));
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
            fvImageParaTableModel = new ImageParaTableModel();
            jTable2 = new JTable(fvImageParaTableModel);
            fvImageParaTableModel.addColumn("FvImageNames");
            fvImageParaTableModel.addColumn("Type");
            
            
            TableColumn typeCol = jTable2.getColumnModel().getColumn(1);
            JComboBox cb = new JComboBox();
            cb.addItem("ValidImageNames");
            cb.addItem("Attributes");
            cb.addItem("Options");
            cb.addItem("Components");
            typeCol.setCellEditor(new DefaultCellEditor(cb));
            
            jTable2.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
            jTable2.getSelectionModel().addListSelectionListener(new ListSelectionListener(){
                public void valueChanged(ListSelectionEvent e) {
                    if (e.getValueIsAdjusting()){
                        return;
                    }
                    ListSelectionModel lsm = (ListSelectionModel)e.getSource();
                    if (lsm.isSelectionEmpty()) {
                        return;
                    }
                    else{
                        int selectedRow = lsm.getMinSelectionIndex();
                        LinkedHashMap<String, String> optionMap = new LinkedHashMap<String, String>();
                        ffc.getFvImagesFvImageOptions(selectedRow, optionMap);
                        if (optionMap.size() > 0){
                            fvOptionTableModel.setRowCount(0);
                            Set<String> key = optionMap.keySet();
                            Iterator<String> i = key.iterator();
                            while (i.hasNext()) {
                                
                                String k = (String)i.next();
                                String[] row = {k, optionMap.get(k)};
                                
                                fvOptionTableModel.addRow(row);
                            }
                        }
                        
                    }
                }
            });
            
            
        }
        return jTable2;
    }


    /**
     * This method initializes jButton6	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButton6() {
        if (jButton6 == null) {
            jButton6 = new JButton();
            jButton6.setPreferredSize(new java.awt.Dimension(120,20));
//            jButton6.setEnabled(false);
            jButton6.setText("Delete Row");
            jButton6.addActionListener(new AbstractAction() {
                /**
                 * 
                 */
                private static final long serialVersionUID = 1L;

                public void actionPerformed(ActionEvent arg0) {
                    // TODO Auto-generated method stub
                    if (jTable2.getSelectedRow() >= 0 ) {
                        fvImageParaTableModel.removeRow(jTable2.getSelectedRow());
                        ffc.removeFvImagesFvImage(jTable2.getSelectedRow());
                    }
                }
                
            });
        }
        return jButton6;
    }


    /**
     * This method initializes jCheckBox3	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBox3() {
        if (jCheckBox3 == null) {
            jCheckBox3 = new JCheckBox();
            jCheckBox3.setText("FV Image Options");
            jCheckBox3.addItemListener(new ItemListener(){

                public void itemStateChanged(ItemEvent arg0) {
                    // TODO Auto-generated method stub
                    boolean selected = jCheckBox3.isSelected();
                    
                        if (!jCheckBox2.isSelected() || jComboBox.getSelectedIndex() == 0) {
                            return;
                        }
                        
//                        jLabel5.setEnabled(selected);
//                        jTextField4.setEnabled(selected);
//                        jLabel6.setEnabled(selected);
//                        jTextField5.setEnabled(selected);
//                        jButton7.setEnabled(selected);
//                        jButton8.setEnabled(selected);

                }
                
            });
        }
        return jCheckBox3;
    }


    /**
     * This method initializes jPanel6	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel6() {
        if (jPanel6 == null) {
            StarLabel starLabel = new StarLabel();
            starLabel.setVisible(false);
            jPanel6 = new JPanel();
            jPanel6.setPreferredSize(new java.awt.Dimension(480,120));
            jPanel6.setBorder(javax.swing.BorderFactory.createEtchedBorder(javax.swing.border.EtchedBorder.RAISED));
            jPanel6.add(getJCheckBox2(), null);
            jPanel6.add(jLabel2, null);
            jPanel6.add(getJComboBox(), null);
            jPanel6.add(new StarLabel(), null);
            jPanel6.add(jLabel3, null);
            jPanel6.add(jLabel4, null);
            jPanel6.add(getJTextField2(), null);
            jPanel6.add(getJButton2(), null);
            jPanel6.add(getJScrollPane2(), null);
            jPanel6.add(getJButton3(), null);
        }
        return jPanel6;
    }

    /**
     * This method initializes jPanel7	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel7() {
        if (jPanel7 == null) {
            FlowLayout flowLayout1 = new FlowLayout();
            flowLayout1.setAlignment(FlowLayout.LEFT);
            jPanel7 = new JPanel();
            jPanel7.setLayout(flowLayout1);
            jPanel7.add(getJCheckBox(), null);
            jPanel7.add(getJTextField3(), null);
            jPanel7.add(getJButton5(), null);
            jPanel7.addComponentListener(new ComponentAdapter(){
                public void componentShown(ComponentEvent e) {
                    if (ffc.getFlashDefinitionFile() != null) {
                        jTextField3.setText(ffc.getFlashDefinitionFile());
                    }
                }
                public void componentHidden(ComponentEvent e) {
                    if (jCheckBox.isSelected()) {
                        ffc.genFlashDefinitionFile(jTextField3.getText());
                    }
                }
            });
        }
        return jPanel7;
    }

    /**
     * This method initializes jCheckBox	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBox() {
        if (jCheckBox == null) {
            jCheckBox = new JCheckBox();
            jCheckBox.setText("Flash Definition File");
            jCheckBox.addItemListener(new ItemListener() {

                public void itemStateChanged(ItemEvent arg0) {
                    // TODO Auto-generated method stub
                    if (jCheckBox.isSelected()){
                        jTextField3.setEnabled(true);
                        jButton5.setEnabled(true);
                    }
                    else {
                        
                        jTextField3.setEnabled(false);
                        jButton5.setEnabled(false);
                    }
                }
            });
        }
        return jCheckBox;
    }

    /**
     * This method initializes jTextField3	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextField3() {
        if (jTextField3 == null) {
            jTextField3 = new JTextField();
            jTextField3.setEnabled(false);
            jTextField3.setPreferredSize(new Dimension(300, 20));
        }
        return jTextField3;
    }

    /**
     * This method initializes jButton5	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButton5() {
        if (jButton5 == null) {
            jButton5 = new JButton();
            jButton5.setEnabled(false);
            jButton5.setText("Browse");
            jButton5.setPreferredSize(new Dimension(78, 20));
            jButton5.addActionListener(new AbstractAction(){
                /**
                 * 
                 */
                private static final long serialVersionUID = 1L;

                public void actionPerformed(ActionEvent e) {
                    // TODO Auto-generated method stub
                    JFileChooser chooser = new JFileChooser();
                    chooser.setMultiSelectionEnabled(false);
                    chooser.setFileSelectionMode(JFileChooser.FILES_AND_DIRECTORIES);
                    int retval = chooser.showOpenDialog(frame);
                    if (retval == JFileChooser.APPROVE_OPTION) {

                        File theFile = chooser.getSelectedFile();
                        jTextField3.setText(theFile.getPath());
                    }
                }
                
            });
        }
        return jButton5;
    }

    /**
     * This method initializes jTextField4	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextField4() {
        if (jTextField4 == null) {
            jTextField4 = new JTextField();
            jTextField4.setEnabled(false);
            jTextField4.setPreferredSize(new Dimension(100, 20));
        }
        return jTextField4;
    }

    /**
     * This method initializes jTextField5	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextField5() {
        if (jTextField5 == null) {
            jTextField5 = new JTextField();
            jTextField5.setEnabled(false);
            jTextField5.setPreferredSize(new Dimension(100, 20));
        }
        return jTextField5;
    }

    /**
     * This method initializes jPanel8	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel8() {
        if (jPanel8 == null) {
            jPanel8 = new JPanel();
            jPanel8.setPreferredSize(new Dimension(80, 55));
            jPanel8.add(getJButton7(), null);
            jPanel8.add(getJButton8(), null);
        }
        return jPanel8;
    }

    /**
     * This method initializes jButton7	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButton7() {
        if (jButton7 == null) {
            jButton7 = new JButton();
            jButton7.setEnabled(true);
            jButton7.setText("Add");
            jButton7.setPreferredSize(new Dimension(80, 20));
            jButton7.addActionListener(new AbstractAction() {
                /**
                 * 
                 */
                private static final long serialVersionUID = 1L;

                public void actionPerformed(ActionEvent e) {
                    if (jTextField4.getText().length() > 0 && jTextField5.getText().length() > 0){
                        String[] row = {jTextField4.getText(), jTextField5.getText()};                        
                        fvOptionTableModel.addRow(row);
                    }
                }
            });
        }
        return jButton7;
    }

    /**
     * This method initializes jButton8	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButton8() {
        if (jButton8 == null) {
            jButton8 = new JButton();
            jButton8.setEnabled(true);
            jButton8.setText("Delete");
            jButton8.setPreferredSize(new Dimension(80, 20));
            jButton8.addActionListener(new AbstractAction() {
                /**
                 * 
                 */
                private static final long serialVersionUID = 1L;

                public void actionPerformed(ActionEvent e) {
                    if (jTable3.getSelectedRow() >= 0){
                        fvOptionTableModel.removeRow(jTable.getSelectedRow());
                    }
                }
            });
        }
        return jButton8;
    }

    /**
     * This method initializes jScrollPane	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPane() {
        if (jScrollPane == null) {
            jScrollPane = new JScrollPane();
            jScrollPane.setPreferredSize(new java.awt.Dimension(350,80));
            jScrollPane.setViewportView(getJTable3());
        }
        return jScrollPane;
    }

    /**
     * This method initializes jTable3	
     * 	
     * @return javax.swing.JTable	
     */
    private JTable getJTable3() {
        if (jTable3 == null) {
            fvOptionTableModel = new DefaultTableModel();
            fvOptionTableModel.addColumn("Name");
            fvOptionTableModel.addColumn("Value");
            jTable3 = new JTable(fvOptionTableModel);

            jTable3.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
            jTable3.setRowHeight(20);
            
        }
        return jTable3;
    }

    /**
     * This method initializes jButton9	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButton9() {
        if (jButton9 == null) {
            jButton9 = new JButton();
            jButton9.setPreferredSize(new Dimension(120, 20));
            jButton9.setActionCommand("Update");
            jButton9.setText("Update FV");
            jButton9.addActionListener(new java.awt.event.ActionListener() {
                public void actionPerformed(java.awt.event.ActionEvent e) {
                    int row = jTable2.getSelectedRow();
                    if (jTable2.isEditing()) {
                        jTable2.getCellEditor().stopCellEditing();
                    }
                    if (jTable3.isEditing()) {
                        jTable3.getCellEditor().stopCellEditing();
                    }
                    
                        //ToDo Check data validity before update
                        String name = fvImageParaTableModel.getValueAt(row, 0) + "";
                        String type = fvImageParaTableModel.getValueAt(row, 1) + "";
                        
                        LinkedHashMap<String, String> lhm = new LinkedHashMap<String, String>();
                        getOptionNameValue(lhm);
                        

                        ffc.updateFvImagesFvImage(row, name.split(" "), type, lhm);
                    
                }
            });
        }
        return jButton9;
    }

    /**
     * @param args
     */
    public static void main(String[] args) {
        // TODO Auto-generated method stub
        new FpdFlash().setVisible(true);
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
        this.setTitle("FPD Flash Definitions");
        this.addInternalFrameListener(new InternalFrameAdapter(){
            public void internalFrameDeactivated(InternalFrameEvent e){
                if (jTable.isEditing()) {
                    jTable.getCellEditor().stopCellEditing();
                }
                if (jTable1.isEditing()) {
                    jTable1.getCellEditor().stopCellEditing();
                }
                if (jTable2.isEditing()) {
                    jTable2.getCellEditor().stopCellEditing();
                }
            }
        });
    }

    private void init(FpdFileContents ffc) {
        if (ffc.getFvImagesFvImageCount() == 0) {
            return;
        }
        String[][] saa = new String[ffc.getFvImagesFvImageCount()][2];
//        ArrayList<LinkedHashMap<String, String>> options = new ArrayList<LinkedHashMap<String, String>>(ffc.getFvImagesFvImageCount());
//
//        for (int j = 0; j < ffc.getFvImagesFvImageCount(); ++j){
//            options.add(new LinkedHashMap<String, String>());
//        }
        ffc.getFvImagesFvImages(saa);
        
       
        int i = 0;
        while (i < saa.length) {
            
            fvImageParaTableModel.addRow(saa[i]);
            ++i;
        }
    }
    
    private void getOptionNameValue(Map<String, String> m){
        for (int i = 0; i < jTable3.getRowCount(); ++i) {
            m.put(fvOptionTableModel.getValueAt(i, 0)+"", fvOptionTableModel.getValueAt(i, 1)+"");
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

}  //  @jve:decl-index=0:visual-constraint="10,10"

class ImageParaTableModel extends DefaultTableModel {

    private static final long serialVersionUID = 1L;
    
   public boolean isCellEditable(int row, int col) {
        if (getValueAt(row, 1).equals("ImageName") && col >=1) {
            return false;
        }
        return true;
    }
}

