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
import java.awt.Dimension;

import javax.swing.JPanel;
import javax.swing.JDialog;
import javax.swing.JTabbedPane;
import javax.swing.JCheckBox;
import javax.swing.JLabel;
import java.awt.FlowLayout;
import javax.swing.AbstractAction;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.ItemEvent;
import java.awt.event.ItemListener;

import javax.swing.ButtonGroup;
import javax.swing.DefaultCellEditor;
import javax.swing.DefaultListModel;
import javax.swing.JTextField;
import javax.swing.JButton;
import javax.swing.JScrollPane;
import javax.swing.JTable;
import javax.swing.JComboBox;
import javax.swing.JRadioButton;
import javax.swing.ListSelectionModel;
import javax.swing.event.InternalFrameAdapter;
import javax.swing.event.InternalFrameEvent;
import javax.swing.event.ListSelectionEvent;
import javax.swing.event.ListSelectionListener;
import javax.swing.event.TableModelEvent;
import javax.swing.event.TableModelListener;
import javax.swing.table.DefaultTableModel;
import javax.swing.table.TableColumn;
import javax.swing.table.TableModel;

import org.tianocore.PlatformSurfaceAreaDocument;
import org.tianocore.frameworkwizard.common.ui.IInternalFrame;
import java.awt.CardLayout;

public class FpdBuildOptions extends IInternalFrame {

    private static final long serialVersionUID = 1L;
    private JPanel jContentPane = null;
    private JPanel jPanel = null;
    private JPanel jPanel1 = null;
    private JPanel jPanel2 = null;
    private JPanel jPanel3 = null;
    private JTabbedPane jTabbedPane = null;
    private JPanel jPanel8 = null;
    private JPanel jPanel9 = null;
    private JPanel jPanel10 = null;
    private JPanel jPanel11 = null;
    private JTextField jTextField2 = null;
    private JLabel jLabel3 = null;
    private JTextField jTextField3 = null;
    private JButton jButton4 = null;
    private JButton jButton5 = null;
    private JTextField jTextField4 = null;
    private JScrollPane jScrollPane2 = null;
    private JTable jTable2 = null;
    private DefaultTableModel ffsTableModel = null;
    private DefaultTableModel imageEntryPointTableModel = null;
    private DefaultTableModel outputDirectoryTableModel = null;
    private DefaultTableModel antTaskTableModel = null;
    private DefaultTableModel ffsAttributesTableModel = null;
    private DefaultTableModel optionsTableModel = null;
    private JPanel jPanel13 = null;
    private JPanel jPanel18 = null;
    private JPanel jPanel15 = null;
    private JPanel jPanel16 = null;
    private JPanel jPanel17 = null;
    private JLabel jLabel7 = null;
    private JTextField jTextField6 = null;
    private JButton jButton8 = null;
    private JButton jButton9 = null;
    private JCheckBox jCheckBox5 = null;
    private JScrollPane jScrollPane5 = null;
    private JTable jTable4 = null;
    private JPanel jPanel19 = null;
    private JPanel jPanel20 = null;
    private JLabel jLabel9 = null;
    private JTextField jTextField7 = null;
    private JLabel jLabel10 = null;
    private JComboBox jComboBox2 = null;
    private JLabel jLabel11 = null;
    private JLabel jLabel12 = null;
    private JTextField jTextField8 = null;
    private JScrollPane jScrollPane6 = null;
    private JTable jTable5 = null;
    private JButton jButton10 = null;
    private JButton jButton11 = null;
    private JPanel jPanel21 = null;
    private JButton jButton12 = null;
    private JButton jButton13 = null;
    private JLabel jLabel8 = null;
    private JTextField jTextField9 = null;
    private JLabel jLabel13 = null;
    private JTextField jTextField10 = null;
    private JPanel jPanel22 = null;
    private JCheckBox jCheckBox6 = null;
    private JComboBox jComboBox4 = null;
    private JCheckBox jCheckBox7 = null;
    private JComboBox jComboBox5 = null;
    private JCheckBox jCheckBox8 = null;
    private JTextField jTextField11 = null;
    private JButton jButton14 = null;
    private JButton jButton15 = null;
    private JButton jButton16 = null;
    private DefaultListModel listModel = new DefaultListModel();
    private JButton jButton17 = null;
    private JButton jButton18 = null;
    private FpdFileContents ffc = null;
    private JButton jButton19 = null;
    private JCheckBox jCheckBox9 = null;
    private JCheckBox jCheckBox10 = null;
    private JCheckBox jCheckBox11 = null;
    private JCheckBox jCheckBox12 = null;
    private JCheckBox jCheckBox13 = null;
    private JCheckBox jCheckBox14 = null;
    private JLabel jLabel14 = null;
    private JTextField jTextField12 = null;
    private JTextField jTextField13 = null;
    private JLabel jLabel15 = null;
    private int selectedRow = -1;
    private JLabel jLabel = null;
    private JLabel jLabel1 = null;
    private JScrollPane jScrollPane = null;
    private JTable jTable = null;
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
            jPanel1 = new JPanel();
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
            jTabbedPane.addTab("FFS", null, getJPanel13(), null);
            jTabbedPane.addTab("Options", null, getJPanel20(), null);
            jTabbedPane.addTab("User Defined ANT Tasks", null, getJPanel8(), null);
        }
        return jTabbedPane;
    }

    
    /**
     * This method initializes jPanel8	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel8() {
        if (jPanel8 == null) {
            jPanel8 = new JPanel();
            jPanel8.setLayout(new BorderLayout());
            jPanel8.add(getJPanel9(), java.awt.BorderLayout.NORTH);
            jPanel8.add(getJPanel10(), java.awt.BorderLayout.SOUTH);
            jPanel8.add(getJPanel11(), java.awt.BorderLayout.CENTER);

        }
        return jPanel8;
    }

    /**
     * This method initializes jPanel9	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel9() {
        if (jPanel9 == null) {
            jLabel = new JLabel();
            jLabel.setText("ANT Task File");
            jLabel.setPreferredSize(new java.awt.Dimension(80,20));
            FlowLayout flowLayout8 = new FlowLayout();
            flowLayout8.setAlignment(java.awt.FlowLayout.LEFT);
            jLabel3 = new JLabel();
            jLabel3.setText("ID");
            jPanel9 = new JPanel();
            jPanel9.setLayout(flowLayout8);
            jPanel9.add(jLabel, null);
            jPanel9.add(getJTextField2(), null);
            jPanel9.add(jLabel3, null);
            jPanel9.add(getJTextField3(), null);
        }
        return jPanel9;
    }

    /**
     * This method initializes jPanel10	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel10() {
        if (jPanel10 == null) {
            jPanel10 = new JPanel();
        }
        return jPanel10;
    }

    /**
     * This method initializes jPanel11	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel11() {
        if (jPanel11 == null) {
            jLabel1 = new JLabel();
            jLabel1.setText("ANT Command Options");
            jLabel1.setPreferredSize(new java.awt.Dimension(131,20));
            FlowLayout flowLayout3 = new FlowLayout();
            flowLayout3.setHgap(5);
            flowLayout3.setAlignment(java.awt.FlowLayout.LEFT);
            jPanel11 = new JPanel();
            jPanel11.setLayout(flowLayout3);
            jPanel11.add(jLabel1, null);
            jPanel11.add(getJTextField4(), null);
            jPanel11.add(getJButton4(), null);
            jPanel11.add(getJButton5(), null);
            jPanel11.add(getJScrollPane2(), null);
        }
        return jPanel11;
    }

    /**
     * This method initializes jTextField2	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextField2() {
        if (jTextField2 == null) {
            jTextField2 = new JTextField();
            jTextField2.setPreferredSize(new java.awt.Dimension(200,20));
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
            jTextField3.setPreferredSize(new java.awt.Dimension(100,20));
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
            jButton4.setPreferredSize(new java.awt.Dimension(90,20));
            jButton4.setText("Add");
            jButton4.addActionListener(new ActionListener() {
                public void actionPerformed(ActionEvent e) {
                    Object[] o = {jTextField3.getText(), null, null};
                        o[1] = jTextField2.getText();
                        o[2] = jTextField4.getText();
                        ffc.genBuildOptionsUserDefAntTask(o[0]+"", null, o[2]+"");
                    antTaskTableModel.addRow(o);
                    
                }
            });
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
            jButton5.setPreferredSize(new java.awt.Dimension(90,20));
            jButton5.setText("Delete");
            jButton5.addActionListener(new ActionListener() {
                public void actionPerformed(ActionEvent e) {
                    if (selectedRow >= 0) {
                        antTaskTableModel.removeRow(selectedRow);
                        ffc.removeBuildOptionsUserDefAntTask(selectedRow);
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
            jTextField4.setPreferredSize(new java.awt.Dimension(270,20));
            jTextField4.setEnabled(true);
        }
        return jTextField4;
    }

    /**
     * This method initializes jScrollPane2	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPane2() {
        if (jScrollPane2 == null) {
            jScrollPane2 = new JScrollPane();
            jScrollPane2.setPreferredSize(new java.awt.Dimension(600,100));
            jScrollPane2.setViewportView(getJTable2());
        }
        return jScrollPane2;
    }

    /**
     * This method initializes jTable2	
     * 	
     * @return javax.swing.JTable	
     */
    private JTable getJTable2() {
        if (jTable2 == null) {
            antTaskTableModel = new DefaultTableModel();
            jTable2 = new JTable(antTaskTableModel);
            antTaskTableModel.addColumn("ID");
            antTaskTableModel.addColumn("Filename");
            antTaskTableModel.addColumn("ANT Command Options");
            
            jTable2.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
            jTable2.getSelectionModel().addListSelectionListener(new ListSelectionListener(){
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
            
            jTable2.getModel().addTableModelListener(new TableModelListener() {
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
        return jTable2;
    }

    /**
     * This method initializes jPanel13	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel13() {
        if (jPanel13 == null) {
            jPanel13 = new JPanel();
            jPanel13.setLayout(new BorderLayout());
            jPanel13.add(getJPanel18(), java.awt.BorderLayout.CENTER);
            jPanel13.add(getJScrollPane(), java.awt.BorderLayout.WEST);
        }
        return jPanel13;
    }

    /**
     * This method initializes jPanel18	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel18() {
        if (jPanel18 == null) {
            jPanel18 = new JPanel();
            jPanel18.setLayout(new BorderLayout());
            jPanel18.add(getJPanel15(), java.awt.BorderLayout.NORTH);
            jPanel18.add(getJPanel16(), java.awt.BorderLayout.SOUTH);
            jPanel18.add(getJPanel17(), java.awt.BorderLayout.EAST);
            jPanel18.add(getJPanel19(), java.awt.BorderLayout.CENTER);
        }
        return jPanel18;
    }

    /**
     * This method initializes jPanel15	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel15() {
        if (jPanel15 == null) {
            FlowLayout flowLayout5 = new FlowLayout();
            flowLayout5.setAlignment(java.awt.FlowLayout.RIGHT);
            jLabel7 = new JLabel();
            jLabel7.setText("Type");
            jPanel15 = new JPanel();
            jPanel15.setLayout(flowLayout5);
            jPanel15.add(jLabel7, null);
            jPanel15.add(getJTextField6(), null);
            jPanel15.add(getJButton8(), null);
            jPanel15.add(getJButton9(), null);
        }
        return jPanel15;
    }

    /**
     * This method initializes jPanel16	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel16() {
        if (jPanel16 == null) {
            FlowLayout flowLayout6 = new FlowLayout();
            flowLayout6.setHgap(5);
            flowLayout6.setAlignment(java.awt.FlowLayout.LEFT);
            jPanel16 = new JPanel();
            jPanel16.setPreferredSize(new java.awt.Dimension(491,130));
            jPanel16.setLayout(flowLayout6);
            jPanel16.add(getJCheckBox5(), null);
            jPanel16.add(getJButton17(), null);
            jPanel16.add(getJButton18(), null);
            jPanel16.add(getJScrollPane5(), null);
        }
        return jPanel16;
    }

    /**
     * This method initializes jPanel17	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel17() {
        if (jPanel17 == null) {
            jPanel17 = new JPanel();
        }
        return jPanel17;
    }

    /**
     * This method initializes jTextField6	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextField6() {
        if (jTextField6 == null) {
            jTextField6 = new JTextField();
            jTextField6.setPreferredSize(new java.awt.Dimension(150,20));
        }
        return jTextField6;
    }

    /**
     * This method initializes jButton8	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButton8() {
        if (jButton8 == null) {
            jButton8 = new JButton();
            jButton8.setPreferredSize(new java.awt.Dimension(70,20));
            jButton8.setText("Add");
            jButton8.addActionListener(new AbstractAction() {
                /**
                 * 
                 */
                private static final long serialVersionUID = -2923720717273384221L;

                public void actionPerformed(java.awt.event.ActionEvent e) {
                    if (jTextField6.getText().length() > 0) {
                        listModel.addElement(jTextField6.getText());
                    }
                }
            });
        }
        return jButton8;
    }

    /**
     * This method initializes jButton9	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButton9() {
        if (jButton9 == null) {
            jButton9 = new JButton();
            jButton9.setPreferredSize(new java.awt.Dimension(70,20));
            jButton9.setText("Delete");
            jButton9.addActionListener(new AbstractAction() {
                /**
                 * 
                 */
                private static final long serialVersionUID = -4002678939178194476L;

                public void actionPerformed(ActionEvent arg0){
                }
            });
        }
        return jButton9;
    }

    /**
     * This method initializes jCheckBox5	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBox5() {
        if (jCheckBox5 == null) {
            jCheckBox5 = new JCheckBox();
            jCheckBox5.setText("Attributes");
            jCheckBox5.setPreferredSize(new java.awt.Dimension(81,20));
        }
        return jCheckBox5;
    }

    /**
     * This method initializes jScrollPane5	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPane5() {
        if (jScrollPane5 == null) {
            jScrollPane5 = new JScrollPane();
            jScrollPane5.setPreferredSize(new java.awt.Dimension(350,100));
            jScrollPane5.setViewportView(getJTable4());
        }
        return jScrollPane5;
    }

    /**
     * This method initializes jTable4	
     * 	
     * @return javax.swing.JTable	
     */
    private JTable getJTable4() {
        if (jTable4 == null) {
            ffsAttributesTableModel = new DefaultTableModel();
            jTable4 = new JTable(ffsAttributesTableModel);
            jTable4.setPreferredSize(new java.awt.Dimension(400,80));
            ffsAttributesTableModel.addColumn("Name");
            ffsAttributesTableModel.addColumn("Value");
        }
        return jTable4;
    }

    /**
     * This method initializes jPanel19	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel19() {
        if (jPanel19 == null) {
            jPanel19 = new JPanel();
            jPanel19.setLayout(new CardLayout());
            jPanel19.add(getJPanel21(), getJPanel21().getName());
            jPanel19.add(getJPanel22(), getJPanel22().getName());
        }
        return jPanel19;
    }

    /**
     * This method initializes jPanel20	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel20() {
        if (jPanel20 == null) {
            jLabel15 = new JLabel();
            jLabel15.setText("Tag Name");
            FlowLayout flowLayout9 = new FlowLayout();
            flowLayout9.setAlignment(java.awt.FlowLayout.LEFT);
            jLabel14 = new JLabel();
            jLabel14.setText("Build Targets");
            jLabel12 = new JLabel();
            jLabel12.setText("Tool Command");
            jLabel11 = new JLabel();
            jLabel11.setText("Supported Arch");
            jLabel10 = new JLabel();
            jLabel10.setText("Tool Chain Family");
            jLabel9 = new JLabel();
            jLabel9.setText("Option Contents");
            jPanel20 = new JPanel();
            jPanel20.setLayout(flowLayout9);
            jPanel20.add(jLabel14, null);
            jPanel20.add(getJTextField12(), null);
            jPanel20.add(jLabel10, null);
            jPanel20.add(getJComboBox2(), null);
            jPanel20.add(jLabel12, null);
            jPanel20.add(getJTextField8(), null);
            jPanel20.add(jLabel11, null);
            jPanel20.add(getJCheckBox9(), null);
            jPanel20.add(getJCheckBox10(), null);
            jPanel20.add(getJCheckBox11(), null);
            jPanel20.add(getJCheckBox12(), null);
            jPanel20.add(getJCheckBox13(), null);
            jPanel20.add(getJCheckBox14(), null);
            jPanel20.add(jLabel15, null);
            jPanel20.add(getJTextField13(), null);
            jPanel20.add(jLabel9, null);
            jPanel20.add(getJTextField7(), null);
            jPanel20.add(getJButton10(), null);
            jPanel20.add(getJButton11(), null);
            jPanel20.add(getJButton19(), null);
            jPanel20.add(getJScrollPane6(), null);
        }
        return jPanel20;
    }

    /**
     * This method initializes jTextField7	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextField7() {
        if (jTextField7 == null) {
            jTextField7 = new JTextField();
            jTextField7.setPreferredSize(new java.awt.Dimension(300,20));
        }
        return jTextField7;
    }

    /**
     * This method initializes jComboBox2	
     * 	
     * @return javax.swing.JComboBox	
     */
    private JComboBox getJComboBox2() {
        if (jComboBox2 == null) {
            jComboBox2 = new JComboBox();
            jComboBox2.setPreferredSize(new java.awt.Dimension(80,20));
            jComboBox2.addItem("MSFT");
            jComboBox2.addItem("GCC");
            jComboBox2.addItem("CYGWIN");
            jComboBox2.addItem("INTEL");
            jComboBox2.setSelectedIndex(0);
        }
        return jComboBox2;
    }

    /**
     * This method initializes jTextField8	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextField8() {
        if (jTextField8 == null) {
            jTextField8 = new JTextField();
            jTextField8.setPreferredSize(new java.awt.Dimension(110,20));
        }
        return jTextField8;
    }

    /**
     * This method initializes jScrollPane6	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPane6() {
        if (jScrollPane6 == null) {
            jScrollPane6 = new JScrollPane();
            jScrollPane6.setPreferredSize(new java.awt.Dimension(630,200));
            jScrollPane6.setViewportView(getJTable5());
        }
        return jScrollPane6;
    }

    /**
     * This method initializes jTable5	
     * 	
     * @return javax.swing.JTable	
     */
    private JTable getJTable5() {
        if (jTable5 == null) {
            optionsTableModel = new DefaultTableModel();
            jTable5 = new JTable(optionsTableModel);
            optionsTableModel.addColumn("BuildTargets");
            optionsTableModel.addColumn("ToolChainFamily");
            optionsTableModel.addColumn("SupportedArch");
            optionsTableModel.addColumn("ToolCommand");
            optionsTableModel.addColumn("TagName");
            optionsTableModel.addColumn("Contents");
            
            TableColumn toolFamilyCol = jTable5.getColumnModel().getColumn(1);
            JComboBox cb = new JComboBox();
            cb.addItem("MSFT");
            cb.addItem("GCC");
            cb.addItem("CYGWIN");
            cb.addItem("INTEL");
            toolFamilyCol.setCellEditor(new DefaultCellEditor(cb));
            
            jTable5.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
            jTable5.getSelectionModel().addListSelectionListener(new ListSelectionListener(){
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
            
            jTable5.getModel().addTableModelListener(new TableModelListener() {
                public void tableChanged(TableModelEvent arg0) {
                    // TODO Auto-generated method stub
                    int row = arg0.getFirstRow();
                    TableModel m = (TableModel)arg0.getSource();
                    if (arg0.getType() == TableModelEvent.UPDATE){
                        //ToDo Data Validition check.
                        String targetName = m.getValueAt(row, 0) + "";
                        String toolChain = m.getValueAt(row, 1) + "";
                        String supArch = m.getValueAt(row, 2) + "";
                        String toolCmd = m.getValueAt(row, 3) + "";
                        String tagName = m.getValueAt(row, 4) + "";
                        String contents = m.getValueAt(row, 5) + "";
                        ffc.updateBuildOptionsOpt(row, targetName, toolChain, tagName, toolCmd, supArch, contents);
                    }
                }
            });
        }
        return jTable5;
    }

    /**
     * This method initializes jButton10	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButton10() {
        if (jButton10 == null) {
            jButton10 = new JButton();
            jButton10.setText("Add");
            jButton10.setPreferredSize(new java.awt.Dimension(70,20));
            jButton10.addActionListener(new AbstractAction() {
                /**
                 * 
                 */
                private static final long serialVersionUID = 1L;

                public void actionPerformed(java.awt.event.ActionEvent e) {
                    boolean[] boolArray = {jCheckBox9.isSelected(),jCheckBox10.isSelected(),jCheckBox11.isSelected(),
                                           jCheckBox12.isSelected(),jCheckBox13.isSelected(),jCheckBox14.isSelected()};
                    String s = boolToList(boolArray);
                    Object[] o = {jTextField12.getText(), jComboBox2.getSelectedItem(), s,
                                  jTextField8.getText(), jTextField13.getText(), jTextField7.getText()};
                    optionsTableModel.addRow(o);
                    ffc.genBuildOptionsOpt(jTextField12.getText(), jComboBox2.getSelectedItem()+"", jTextField13.getText(), jTextField8.getText(), s, jTextField7.getText());
                }
            });
        }
        return jButton10;
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
        
        return s.trim();
    }

    /**
     * This method initializes jButton11	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButton11() {
        if (jButton11 == null) {
            jButton11 = new JButton();
            jButton11.setText("Delete");
            jButton11.setPreferredSize(new java.awt.Dimension(70,20));
            jButton11.addActionListener(new AbstractAction() {
                /**
                 * 
                 */
                private static final long serialVersionUID = 1L;

                public void actionPerformed(java.awt.event.ActionEvent e) {
                    if (selectedRow >= 0) {
                        optionsTableModel.removeRow(selectedRow);
                        ffc.removeBuildOptionsOpt(selectedRow);
                    }
                }
            });
        }
        return jButton11;
    }

    /**
     * This method initializes jPanel21	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel21() {
        if (jPanel21 == null) {
            jLabel13 = new JLabel();
            jLabel13.setText("EncapsulationTag");
            jLabel8 = new JLabel();
            jLabel8.setText("EncapsulationType");
            jPanel21 = new JPanel();
            jPanel21.setName("jPanel21");
            jPanel21.add(jLabel8, null);
            jPanel21.add(getJTextField9(), null);
            jPanel21.add(jLabel13, null);
            jPanel21.add(getJTextField10(), null);
            jPanel21.add(getJButton12(), null);
            jPanel21.add(getJButton13(), null);
            jPanel21.add(getJButton16(), null);
        }
        return jPanel21;
    }

    /**
     * This method initializes jButton12	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButton12() {
        if (jButton12 == null) {
            jButton12 = new JButton();
            jButton12.setText("Add Sections");
            jButton12.setPreferredSize(new java.awt.Dimension(109,20));
            jButton12.addActionListener(new AbstractAction(){
                /**
                 * 
                 */
                private static final long serialVersionUID = 1L;

                public void actionPerformed(ActionEvent arg0){
                }
            });
        }
        return jButton12;
    }

    /**
     * This method initializes jButton13	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButton13() {
        if (jButton13 == null) {
            jButton13 = new JButton();
            jButton13.setText("Add Section");
            jButton13.setPreferredSize(new java.awt.Dimension(102,20));
            jButton13.addActionListener(new AbstractAction(){
                /**
                 * 
                 */
                private static final long serialVersionUID = 1L;

                public void actionPerformed(ActionEvent arg0){
                    CardLayout cl = (CardLayout)jPanel19.getLayout();
                    cl.last(jPanel19);
                }
            });
        }
        return jButton13;
    }

    /**
     * This method initializes jTextField9	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextField9() {
        if (jTextField9 == null) {
            jTextField9 = new JTextField();
            jTextField9.setPreferredSize(new java.awt.Dimension(250,20));
        }
        return jTextField9;
    }

    /**
     * This method initializes jTextField10	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextField10() {
        if (jTextField10 == null) {
            jTextField10 = new JTextField();
            jTextField10.setPreferredSize(new java.awt.Dimension(250,20));
        }
        return jTextField10;
    }

    /**
     * This method initializes jPanel22	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel22() {
        if (jPanel22 == null) {
            FlowLayout flowLayout7 = new FlowLayout();
            flowLayout7.setAlignment(java.awt.FlowLayout.LEFT);
            jPanel22 = new JPanel();
            jPanel22.setLayout(flowLayout7);
            jPanel22.setName("jPanel22");
            jPanel22.add(getJCheckBox6(), null);
            jPanel22.add(getJComboBox4(), null);
            jPanel22.add(getJCheckBox7(), null);
            jPanel22.add(getJComboBox5(), null);
            jPanel22.add(getJCheckBox8(), null);
            jPanel22.add(getJTextField11(), null);
            jPanel22.add(getJButton14(), null);
            jPanel22.add(getJButton15(), null);
        }
        return jPanel22;
    }

    /**
     * This method initializes jCheckBox6	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBox6() {
        if (jCheckBox6 == null) {
            jCheckBox6 = new JCheckBox();
            jCheckBox6.setText("Section Type");
            jCheckBox6.setPreferredSize(new java.awt.Dimension(98,20));
        }
        return jCheckBox6;
    }

    /**
     * This method initializes jComboBox4	
     * 	
     * @return javax.swing.JComboBox	
     */
    private JComboBox getJComboBox4() {
        if (jComboBox4 == null) {
            jComboBox4 = new JComboBox();
            jComboBox4.setPreferredSize(new java.awt.Dimension(260,20));
            
            jComboBox4.addItem("EFI_SECTION_FREEFORM_SUBTYPE_GUID");
            jComboBox4.addItem("EFI_SECTION_VERSION");
            jComboBox4.addItem("EFI_SECTION_USER_INTERFACE");
            jComboBox4.addItem("EFI_SECTION_DXE_DEPEX");
            jComboBox4.addItem("EFI_SECTION_PEI_DEPEX");
            jComboBox4.addItem("EFI_SECTION_PE32");
            jComboBox4.addItem("EFI_SECTION_PIC");
            jComboBox4.addItem("EFI_SECTION_TE");
            jComboBox4.addItem("EFI_SECTION_RAW");
            jComboBox4.addItem("EFI_SECTION_COMPRESSION");
            jComboBox4.addItem("EFI_SECTION_GUID_DEFINED");
            jComboBox4.addItem("EFI_SECTION_COMPATIBILITY16");
            jComboBox4.addItem("EFI_SECTION_FIRMWARE_VOLUME_IMAGE");
            jComboBox4.setSelectedIndex(0);
        }
        return jComboBox4;
    }

    /**
     * This method initializes jCheckBox7	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBox7() {
        if (jCheckBox7 == null) {
            jCheckBox7 = new JCheckBox();
            jCheckBox7.setPreferredSize(new java.awt.Dimension(120,20));
            jCheckBox7.setText("Compressible");
        }
        return jCheckBox7;
    }

    /**
     * This method initializes jComboBox5	
     * 	
     * @return javax.swing.JComboBox	
     */
    private JComboBox getJComboBox5() {
        if (jComboBox5 == null) {
            jComboBox5 = new JComboBox();
            jComboBox5.setPreferredSize(new java.awt.Dimension(80,20));
            
            jComboBox5.addItem("false");
            jComboBox5.addItem("true");
            jComboBox5.setSelectedIndex(0);
        }
        return jComboBox5;
    }

    /**
     * This method initializes jCheckBox8	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBox8() {
        if (jCheckBox8 == null) {
            jCheckBox8 = new JCheckBox();
            jCheckBox8.setText("Binding Order");
            jCheckBox8.setPreferredSize(new java.awt.Dimension(103,20));
        }
        return jCheckBox8;
    }

    /**
     * This method initializes jTextField11	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextField11() {
        if (jTextField11 == null) {
            jTextField11 = new JTextField();
            jTextField11.setPreferredSize(new java.awt.Dimension(150,20));
        }
        return jTextField11;
    }

    /**
     * This method initializes jButton14	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButton14() {
        if (jButton14 == null) {
            jButton14 = new JButton();
            jButton14.setText("Add");
            jButton14.setPreferredSize(new java.awt.Dimension(70,20));
            jButton14.addActionListener(new AbstractAction(){
                /**
                 * 
                 */
                private static final long serialVersionUID = 1L;

                public void actionPerformed(ActionEvent arg0){
                }
            });
        }
        return jButton14;
    }

    /**
     * This method initializes jButton15	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButton15() {
        if (jButton15 == null) {
            jButton15 = new JButton();
            jButton15.setPreferredSize(new java.awt.Dimension(70,20));
            jButton15.setText("Delete");
        }
        return jButton15;
    }

    /**
     * This method initializes jButton16	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButton16() {
        if (jButton16 == null) {
            jButton16 = new JButton();
            jButton16.setText("Delete");
            jButton16.setPreferredSize(new java.awt.Dimension(70,20));
        }
        return jButton16;
    }

    /**
     * This method initializes jButton17	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButton17() {
        if (jButton17 == null) {
            jButton17 = new JButton();
            jButton17.setPreferredSize(new java.awt.Dimension(70,20));
            jButton17.setText("Add");
            jButton17.addActionListener(new AbstractAction() {
                /**
                 * 
                 */
                private static final long serialVersionUID = 1L;

                public void actionPerformed(ActionEvent arg0){
                    Object[] o = {"", ""};
                    ffsAttributesTableModel.addRow(o);
                }
            });
        }
        return jButton17;
    }

    /**
     * This method initializes jButton18	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButton18() {
        if (jButton18 == null) {
            jButton18 = new JButton();
            jButton18.setPreferredSize(new java.awt.Dimension(70,20));
            jButton18.setText("Delete");
            jButton18.addActionListener(new AbstractAction(){
                /**
                 * 
                 */
                private static final long serialVersionUID = 1L;

                public void actionPerformed(ActionEvent arg0){
                    if (jTable4.getSelectedRow() >= 0){
                        ffsAttributesTableModel.removeRow(jTable4.getSelectedRow());
                    }
                }
            });
        }
        return jButton18;
    }

    /**
     * This method initializes jButton19	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButton19() {
        if (jButton19 == null) {
            jButton19 = new JButton();
            jButton19.setPreferredSize(new java.awt.Dimension(75,20));
            jButton19.setEnabled(false);
            jButton19.setText("Update");
        }
        return jButton19;
    }

    /**
     * This method initializes jCheckBox9	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBox9() {
        if (jCheckBox9 == null) {
            jCheckBox9 = new JCheckBox();
            jCheckBox9.setPreferredSize(new java.awt.Dimension(50,20));
            jCheckBox9.setText("IA32");
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
            jCheckBox10.setPreferredSize(new java.awt.Dimension(50,20));
            jCheckBox10.setText("IPF");
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
            jCheckBox11.setText("X64");
            jCheckBox11.setPreferredSize(new java.awt.Dimension(47,20));
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
            jCheckBox12.setPreferredSize(new java.awt.Dimension(50,20));
            jCheckBox12.setText("EBC");
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
            jCheckBox13.setPreferredSize(new java.awt.Dimension(55,20));
            jCheckBox13.setText("ARM");
        }
        return jCheckBox13;
    }

    /**
     * This method initializes jCheckBox14	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBox14() {
        if (jCheckBox14 == null) {
            jCheckBox14 = new JCheckBox();
            jCheckBox14.setPreferredSize(new java.awt.Dimension(50,20));
            jCheckBox14.setText("PPC");
        }
        return jCheckBox14;
    }

    /**
     * This method initializes jTextField12	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextField12() {
        if (jTextField12 == null) {
            jTextField12 = new JTextField();
            jTextField12.setPreferredSize(new java.awt.Dimension(150,20));
        }
        return jTextField12;
    }

    /**
     * This method initializes jTextField13	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextField13() {
        if (jTextField13 == null) {
            jTextField13 = new JTextField();
            jTextField13.setPreferredSize(new java.awt.Dimension(140,20));
        }
        return jTextField13;
    }

    /**
     * This method initializes jScrollPane	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPane() {
        if (jScrollPane == null) {
            jScrollPane = new JScrollPane();
            jScrollPane.setPreferredSize(new java.awt.Dimension(150,419));
            jScrollPane.setViewportView(getJTable());
        }
        return jScrollPane;
    }

    /**
     * This method initializes jTable	
     * 	
     * @return javax.swing.JTable	
     */
    private JTable getJTable() {
        if (jTable == null) {
            ffsTableModel = new DefaultTableModel();
            ffsTableModel.addColumn("FFS Type");
            jTable = new JTable(ffsTableModel);
            jTable.setShowGrid(false);
            jTable.setRowHeight(20);
        }
        return jTable;
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

    private void init(FpdFileContents ffc) {
        initOptionTable();
        initAntTaskTable();
        this.addInternalFrameListener(new InternalFrameAdapter(){
            public void internalFrameDeactivated(InternalFrameEvent e){
                if (jTable.isEditing()) {
                    jTable.getCellEditor().stopCellEditing();
                }
//                if (jTable1.isEditing()) {
//                    jTable1.getCellEditor().stopCellEditing();
//                }
                if (jTable2.isEditing()) {
                    jTable2.getCellEditor().stopCellEditing();
                }
//                if (jTable3.isEditing()) {
//                    jTable3.getCellEditor().stopCellEditing();
//                }
                if (jTable4.isEditing()) {
                    jTable4.getCellEditor().stopCellEditing();
                }
                if (jTable5.isEditing()) {
                    jTable5.getCellEditor().stopCellEditing();
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
    /**
     * This method initializes this
     * 
     * @return void
     */
    private void initialize() {
        this.setSize(722, 439);
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
            jContentPane.add(getJPanel(), java.awt.BorderLayout.SOUTH);
            jContentPane.add(getJPanel1(), java.awt.BorderLayout.NORTH);
            jContentPane.add(getJPanel2(), java.awt.BorderLayout.WEST);
            jContentPane.add(getJPanel3(), java.awt.BorderLayout.EAST);
            jContentPane.add(getJTabbedPane(), java.awt.BorderLayout.CENTER);
        }
        return jContentPane;
    }

}  //  @jve:decl-index=0:visual-constraint="10,10"
