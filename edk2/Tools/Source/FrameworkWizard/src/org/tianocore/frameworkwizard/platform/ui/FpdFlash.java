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
import javax.swing.ButtonGroup;
import javax.swing.DefaultCellEditor;
import javax.swing.DefaultListModel;
import javax.swing.JDialog;
import javax.swing.JFileChooser;
import javax.swing.JFrame;
import javax.swing.JInternalFrame;
import javax.swing.JOptionPane;
import javax.swing.JTabbedPane;
import javax.swing.JButton;
import javax.swing.ListSelectionModel;

import org.tianocore.PlatformSurfaceAreaDocument;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.ui.IInternalFrame;
import org.tianocore.frameworkwizard.common.ui.StarLabel;


import java.awt.FlowLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.ComponentAdapter;
import java.awt.event.ComponentEvent;
import java.awt.event.FocusAdapter;
import java.awt.event.FocusEvent;
import java.awt.event.ItemEvent;
import java.awt.event.ItemListener;
import java.awt.event.ActionEvent;
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
import javax.swing.JList;
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

public class FpdFlash extends IInternalFrame {

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
    private JPanel jPanelFvImageW = null;
    private JPanel jPanelFvImageS = null;
    private JCheckBox jCheckBox1 = null;
    private JLabel jLabel = null;
    private JTextField jTextField = null;
    private JLabel jLabel1 = null;
    private JTextField jTextField1 = null;
    private JButton jButton = null;
    private JScrollPane jScrollPane = null;
    private JScrollPane jScrollPane1 = null;
    private JTable jTable = null;
    private JPanel jPanel4 = null;
    private JButton jButton1 = null;
    private DefaultTableModel fvPropertyTableModel = null;
    private DefaultTableModel fvImageNameTableModel = null;
    private ImageParaTableModel fvImageParaTableModel = null;
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
    private JLabel jLabel5 = null;
    private JComboBox jComboBox1 = null;
    private JCheckBox jCheckBox4 = null;
    private JCheckBox jCheckBox5 = null;
    private JCheckBox jCheckBox6 = null;
    private JCheckBox jCheckBox7 = null;
    private JCheckBox jCheckBox8 = null;
    private JCheckBox jCheckBox9 = null;
    private JCheckBox jCheckBox10 = null;
    private JCheckBox jCheckBox11 = null;
    private JCheckBox jCheckBox12 = null;
    private JCheckBox jCheckBox13 = null;
    private JPanel jPanel6 = null;
    private DefaultTableModel fdfImageDefTableModel = null;
    private DefaultTableModel fdfBlocksTableModel = null;
    private DefaultTableModel fdfRegionsTableModel = null;
    private DefaultTableModel fdfSubRegionsTableModel = null;
    
    private JLabel jLabel17 = null;
    private DefaultListModel listModel = new DefaultListModel();
    private FpdFileContents ffc = null;
    private JPanel jPanel7 = null;
    private JCheckBox jCheckBox = null;
    private JTextField jTextField3 = null;
    private JButton jButton5 = null;
    
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
     * This method initializes jPanelFvImageS	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelFvImageS() {
        if (jPanelFvImageS == null) {
            GridLayout gridLayout2 = new GridLayout();
            gridLayout2.setRows(1);
            jPanelFvImageS = new JPanel();
            jPanelFvImageS.setPreferredSize(new java.awt.Dimension(480,200));
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
            jPanel5 = new JPanel();
            jPanel5.setPreferredSize(new java.awt.Dimension(480,120));
            
            GridLayout gridLayout = new GridLayout();
            gridLayout.setRows(5);
            gridLayout.setColumns(3);
            jPanel5.setLayout(gridLayout);
            jPanel5.add(getJCheckBox3(), null);
            
            jLabel5 = new JLabel();
            jLabel5.setText("EFI_ERASE_POLARITY");
            jLabel5.setEnabled(false);
			jPanel5.add(jLabel5, null);
			jPanel5.add(getJComboBox1(), null);
			jPanel5.add(getJCheckBox4(), null);
			jPanel5.add(getJCheckBox5(), null);
			jPanel5.add(getJCheckBox6(), null);
			jPanel5.add(getJCheckBox7(), null);
			jPanel5.add(getJCheckBox8(), null);
			jPanel5.add(getJCheckBox9(), null);
			jPanel5.add(getJCheckBox10(), null);
			jPanel5.add(getJCheckBox11(), null);
			jPanel5.add(getJCheckBox12(), null);
			jPanel5.add(getJCheckBox13(), null);
            jPanel5.add(getJButton4(), null);
			jPanel5.setBorder(javax.swing.BorderFactory.createEtchedBorder(javax.swing.border.EtchedBorder.LOWERED));
			jPanel5.add(getJButton6(), null);
            
            
            			

            
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
            jButton4.setPreferredSize(new java.awt.Dimension(80,20));
//            jButton4.setEnabled(false);
            jButton4.setText("Add FV Image");
            jButton4.addActionListener(new AbstractAction() {
                public void actionPerformed(java.awt.event.ActionEvent e) {
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
                    
                    LinkedHashMap<String, String> m = new LinkedHashMap<String, String>();
                    boolean[] boolArray = {jComboBox1.getSelectedIndex()==0 ? true: false, jCheckBox6.isSelected(), jCheckBox9.isSelected(),
                                                                            jCheckBox11.isSelected(), jCheckBox12.isSelected(),
                                                                            jCheckBox13.isSelected(),jCheckBox4.isSelected(),
                                                                            jCheckBox5.isSelected(), jCheckBox7.isSelected(),
                                                                            jCheckBox8.isSelected(),jCheckBox10.isSelected()};
                    booleanToNameValue(boolArray, m);
                    ffc.genFvImagesFvImage(imageName.split(" "), jComboBox.getSelectedItem()+"", m);
                    
                    Object[] o = {imageName, jComboBox.getSelectedItem(), jComboBox1.getSelectedIndex()==0 ? true: false, 
                                  jCheckBox6.isSelected(), jCheckBox9.isSelected(),
                                  jCheckBox11.isSelected(), jCheckBox12.isSelected(),
                                  jCheckBox13.isSelected(),jCheckBox4.isSelected(),
                                  jCheckBox5.isSelected(), jCheckBox7.isSelected(),
                                  jCheckBox8.isSelected(),jCheckBox10.isSelected()};
                   fvImageParaTableModel.addRow(o); 
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
            jTable2.setAutoResizeMode(javax.swing.JTable.AUTO_RESIZE_OFF);
            fvImageParaTableModel.addColumn("FvImageNames");
            fvImageParaTableModel.addColumn("Type");
            fvImageParaTableModel.addColumn("ErasePolarity");
            fvImageParaTableModel.addColumn("ReadStatus");
            fvImageParaTableModel.addColumn("WriteStatus");
            fvImageParaTableModel.addColumn("LockStatus");
            fvImageParaTableModel.addColumn("MemoryMapped");
            fvImageParaTableModel.addColumn("StickyWrite");
            fvImageParaTableModel.addColumn("ReadDisableCap");
            fvImageParaTableModel.addColumn("ReadEnableCap");
            fvImageParaTableModel.addColumn("WriteDisableCap");
            fvImageParaTableModel.addColumn("WriteEnableCap");
            fvImageParaTableModel.addColumn("LockCap");
            
            TableColumn typeCol = jTable2.getColumnModel().getColumn(1);
            JComboBox cb = new JComboBox();
            cb.addItem("ValidImageNames");
            cb.addItem("Attributes");
            cb.addItem("Options");
            cb.addItem("Components");
            typeCol.setCellEditor(new DefaultCellEditor(cb));
            
//            TableColumn epCol = jTable2.getColumnModel().getColumn(2);
//            JComboBox cb1 = new JComboBox();
//            cb1.addItem("1");
//            cb1.addItem("0");
//            epCol.setCellEditor(new DefaultCellEditor(cb1));
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
//                        selectedRow = lsm.getMinSelectionIndex();
                    }
                }
            });
            
            jTable2.getModel().addTableModelListener(new TableModelListener(){
                public void tableChanged(TableModelEvent arg0) {
                    // TODO Auto-generated method stub
                    int row = arg0.getFirstRow();
                    TableModel m = (TableModel)arg0.getSource();
                    if (arg0.getType() == TableModelEvent.UPDATE){
                        //ToDo Check data validity before update
                        String name = m.getValueAt(row, 0) + "";
                        String type = m.getValueAt(row, 1) + "";
                        boolean[] boolArray = new boolean[11];
                        for (int i = 2; i < 13; ++i) {
                            boolArray[i-2] = (Boolean)m.getValueAt(row, i);
                        }
                        LinkedHashMap<String, String> lhm = new LinkedHashMap<String, String>();
                        booleanToNameValue(boolArray, lhm);

                        ffc.updateFvImagesFvImage(row, name.split(" "), type, lhm);
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
            jButton6.setPreferredSize(new java.awt.Dimension(150,20));
//            jButton6.setEnabled(false);
            jButton6.setText("Delete Row");
            jButton6.addActionListener(new AbstractAction() {

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
                    boolean seleted = jCheckBox3.isSelected();
                    
                        if (!jCheckBox2.isSelected() || jComboBox.getSelectedIndex() == 0) {
                            return;
                        }
                        
                        jLabel5.setEnabled(seleted);
                        jComboBox1.setEnabled(seleted);
                        
                        jCheckBox4.setEnabled(seleted);
                        jCheckBox5.setEnabled(seleted);
                        jCheckBox6.setEnabled(seleted);
                        jCheckBox7.setEnabled(seleted);
                        jCheckBox8.setEnabled(seleted);
                        jCheckBox9.setEnabled(seleted);
                        jCheckBox10.setEnabled(seleted);
                        jCheckBox11.setEnabled(seleted);
                        jCheckBox12.setEnabled(seleted);
                        jCheckBox13.setEnabled(seleted);
//                        jButton4.setEnabled(seleted);
//                        jButton6.setEnabled(seleted);
                }
                
            });
        }
        return jCheckBox3;
    }


    /**
     * This method initializes jComboBox1	
     * 	
     * @return javax.swing.JComboBox	
     */
    private JComboBox getJComboBox1() {
        if (jComboBox1 == null) {
            jComboBox1 = new JComboBox();
            jComboBox1.setPreferredSize(new java.awt.Dimension(20,20));
            jComboBox1.setEnabled(false);
            jComboBox1.addItem("1");
            jComboBox1.addItem("0");
            jComboBox1.setSelectedIndex(0);
        }
        return jComboBox1;
    }


    /**
     * This method initializes jCheckBox4	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBox4() {
        if (jCheckBox4 == null) {
            jCheckBox4 = new JCheckBox();
            jCheckBox4.setText("Read Disable CAP");
            jCheckBox4.setEnabled(false);
        }
        return jCheckBox4;
    }


    /**
     * This method initializes jCheckBox5	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBox5() {
        if (jCheckBox5 == null) {
            jCheckBox5 = new JCheckBox();
            jCheckBox5.setText("Read Enable CAP");
            jCheckBox5.setEnabled(false);
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
            jCheckBox6.setText("Read Status");
            jCheckBox6.setEnabled(false);
        }
        return jCheckBox6;
    }


    /**
     * This method initializes jCheckBox7	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBox7() {
        if (jCheckBox7 == null) {
            jCheckBox7 = new JCheckBox();
            jCheckBox7.setText("Write Disable CAP");
            jCheckBox7.setEnabled(false);
        }
        return jCheckBox7;
    }


    /**
     * This method initializes jCheckBox8	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBox8() {
        if (jCheckBox8 == null) {
            jCheckBox8 = new JCheckBox();
            jCheckBox8.setText("Write Enable CAP");
            jCheckBox8.setEnabled(false);
        }
        return jCheckBox8;
    }


    /**
     * This method initializes jCheckBox9	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBox9() {
        if (jCheckBox9 == null) {
            jCheckBox9 = new JCheckBox();
            jCheckBox9.setText("Write Status");
            jCheckBox9.setEnabled(false);
        }
        return jCheckBox9;
    }


    /**
     * This method initializes jCheckBox10	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBox10() {
        if (jCheckBox10 == null) {
            jCheckBox10 = new JCheckBox();
            jCheckBox10.setText("Lock CAP");
            jCheckBox10.setEnabled(false);
        }
        return jCheckBox10;
    }


    /**
     * This method initializes jCheckBox11	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBox11() {
        if (jCheckBox11 == null) {
            jCheckBox11 = new JCheckBox();
            jCheckBox11.setText("Lock Status");
            jCheckBox11.setEnabled(false);
        }
        return jCheckBox11;
    }


    /**
     * This method initializes jCheckBox12	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBox12() {
        if (jCheckBox12 == null) {
            jCheckBox12 = new JCheckBox();
            jCheckBox12.setText("Memory Mapped");
            jCheckBox12.setEnabled(false);
        }
        return jCheckBox12;
    }


    /**
     * This method initializes jCheckBox13	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBox13() {
        if (jCheckBox13 == null) {
            jCheckBox13 = new JCheckBox();
            jCheckBox13.setText("Sticky Write");
            jCheckBox13.setEnabled(false);
        }
        return jCheckBox13;
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
    }

    private void init(FpdFileContents ffc) {
        if (ffc.getFvImagesFvImageCount() == 0) {
            return;
        }
        String[][] saa = new String[ffc.getFvImagesFvImageCount()][2];
        ArrayList<LinkedHashMap<String, String>> options = new ArrayList<LinkedHashMap<String, String>>(ffc.getFvImagesFvImageCount());

        for (int j = 0; j < ffc.getFvImagesFvImageCount(); ++j){
            options.add(new LinkedHashMap<String, String>());
        }
        ffc.getFvImagesFvImages(saa, options);
        
        Object[] rowData = new Object[13];
        int i = 0;
        Boolean f = new Boolean("false");
        while (i < saa.length) {
            rowData[0] = saa[i][0];
            rowData[1] = saa[i][1];
            
            
            //ToDo: add alignment settings
            Boolean[] boolArray = new Boolean[11];
            int k = 0;
            while (k < 11){
                boolArray[k] = f;
                ++k;
            }
            namevalueToBoolean(options.get(i), boolArray);
            for (k = 2; k < 13; ++k) {
                rowData[k] = boolArray[k-2];
            }
            fvImageParaTableModel.addRow(rowData);
            ++i;
        }
    }
    
    private void namevalueToBoolean(Map<String, String> m, Boolean[] boolArray){
        Set<String> key = m.keySet();
        Iterator<String> ki= key.iterator();
        Boolean t = new Boolean("true");
        while(ki.hasNext()) {
            String k = ki.next();
            if (k.equals("EFI_ERASE_POLARITY") && m.get(k).equals("1")) {
                boolArray[0] = t;
            }
            if (k.equals("EFI_READ_STATUS") && m.get(k).equals("1")) {
                boolArray[1] = t;
            }
            if (k.equals("EFI_WRITE_STATUS") && m.get(k).equals("1")) {
                boolArray[2] = t;
            }
            if (k.equals("EFI_LOCK_STATUS") && m.get(k).equals("1")) {
                boolArray[3] = t;
            }
            if (k.equals("EFI_MEMORY_MAPPED") && m.get(k).equals("1")) {
                boolArray[4] = t;
            }
            if (k.equals("EFI_STICKY_WRITE") && m.get(k).equals("1")) {
                boolArray[5] = t;
            }
            if (k.equals("EFI_READ_DISABLED_CAP") && m.get(k).equals("1")) {
                boolArray[6] = t;
            }
            if (k.equals("EFI_READ_ENABLED_CAP") && m.get(k).equals("1")) {
                boolArray[7] = t;
            }
            if (k.equals("EFI_WRITE_DISABLED_CAP") && m.get(k).equals("1")) {
                boolArray[8] = t;
            }
            if (k.equals("EFI_WRITE_ENABLED_CAP") && m.get(k).equals("1")) {
                boolArray[9] = t;
            }
            if (k.equals("EFI_LOCK_CAP") && m.get(k).equals("1")) {
                boolArray[10] = t;
            }
        }
    }
    
    private void booleanToNameValue(boolean[] boolArray, Map<String, String> m){
        if (boolArray[0]) {
            m.put("EFI_ERASE_POLARITY", "1");
        }
        else {
            m.put("EFI_ERASE_POLARITY", "0");
        }
        if (boolArray[1]) {
            m.put("EFI_READ_STATUS", "1");
        }
        else {
            m.put("EFI_READ_STATUS", "0");
        }
        if (boolArray[2]) {
            m.put("EFI_WRITE_STATUS", "1");
        }
        else {
            m.put("EFI_WRITE_STATUS", "0");
        }
        if (boolArray[3]) {
            m.put("EFI_LOCK_STATUS", "1");
        }
        else {
            m.put("EFI_LOCK_STATUS", "0");
        }
        if (boolArray[4]) {
            m.put("EFI_MEMORY_MAPPED", "1");
        }
        else {
            m.put("EFI_MEMORY_MAPPED", "0");
        }
        if (boolArray[5]) {
            m.put("EFI_STICKY_WRITE", "1");
        }
        else {
            m.put("EFI_STICKY_WRITE", "0");
        }
        if (boolArray[6]) {
            m.put("EFI_READ_DISABLED_CAP", "1");
        }
        else {
            m.put("EFI_READ_DISABLED_CAP", "0");
        }
        if (boolArray[7]) {
            m.put("EFI_READ_ENABLED_CAP", "1");
        }
        else {
            m.put("EFI_READ_ENABLED_CAP", "0");
        }
        if (boolArray[8]) {
            m.put("EFI_WRITE_DISABLED_CAP", "1");
        }
        else {
            m.put("EFI_WRITE_DISABLED_CAP", "0");
        }
        if (boolArray[9]) {
            m.put("EFI_WRITE_ENABLED_CAP", "1");
        }
        else {
            m.put("EFI_WRITE_ENABLED_CAP", "0");
        }
        if (boolArray[10]) {
            m.put("EFI_LOCK_CAP", "1");
        }
        else {
            m.put("EFI_LOCK_CAP", "0");
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

    public Class getColumnClass (int c) {
        return getValueAt(0, c).getClass();
    }
}
