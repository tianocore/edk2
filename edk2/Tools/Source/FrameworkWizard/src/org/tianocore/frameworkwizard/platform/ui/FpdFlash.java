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
import javax.swing.JComponent;
import javax.swing.JFileChooser;
import javax.swing.JFrame;
import javax.swing.JOptionPane;
import javax.swing.JTabbedPane;
import javax.swing.JButton;
import javax.swing.ListSelectionModel;

import org.tianocore.PlatformSurfaceAreaDocument;
import org.tianocore.frameworkwizard.common.Identifications.OpeningPlatformType;
import org.tianocore.frameworkwizard.common.ui.IInternalFrame;
import org.tianocore.frameworkwizard.common.ui.StarLabel;


import java.awt.FlowLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ComponentAdapter;
import java.awt.event.ComponentEvent;
import java.awt.event.ItemEvent;
import java.awt.event.ItemListener;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.Set;
import java.util.Vector;

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
import javax.swing.JSplitPane;

public class FpdFlash extends IInternalFrame {

    /**
     * 
     */
    private static final long serialVersionUID = 1L;
    static JFrame frame;
    private JPanel jContentPane = null;
    private JPanel jPanelContentEast = null;
    private JPanel jPanelContentSouth = null;
    private JPanel jPanelContentWest = null;
    private JPanel jPanelContentNorth = null;
    private JTabbedPane jTabbedPane = null;
    private JPanel jPanelFvImages = null;
    private JPanel jPanelFvImageN = null;
    private JPanel jPanelFvImageS = null;
    private JCheckBox jCheckBoxFvProperty = null;
    private JLabel jLabelFvPropName = null;
    private JTextField jTextFieldFvPropName = null;
    private JLabel jLabelFvPropValue = null;
    private JTextField jTextFieldFvPropValue = null;
    private JButton jButtonFvPropAdd = null;
    private JScrollPane jScrollPaneFvProp = null;
    private JTable jTableFvProp = null;
    private JPanel jPanelFvPropButtonGroup = null;
    private JButton jButtonFvPropDel = null;
    private DefaultTableModel fvPropertyTableModel = null;
    private DefaultTableModel fvImageNameTableModel = null;
    private ImageParaTableModel fvImageParaTableModel = null;
    private DefaultTableModel fvOptionTableModel = null;
    private JPanel jPanelFvImageC = null;
    private JCheckBox jCheckBoxFvImagePara = null;
    private JLabel jLabelFvParaType = null;
    private JComboBox jComboBoxFvParaType = null;
    private JLabel jLabelFvImageNames = null;
    private JLabel jLabelFvParaName = null;
    private JTextField jTextFieldFvParaName = null;
    private JButton jButtonFvParaAdd = null;
    private JScrollPane jScrollPaneFvImageNames = null;
    private JTable jTableFvImageNames = null;
    private JButton jButtonFvParaDel = null;
    private JPanel jPanelFvImageOpts = null;
    private JButton jButtonAddFvImage = null;
    private JScrollPane jScrollPaneFvInfo = null;
    private JTable jTableFvInfo = null;
    private JButton jButtonDelFvImage = null;
    private JCheckBox jCheckBoxFvImageOpts = null;
    private JPanel jPanelFvImagePara = null;
    private OpeningPlatformType docConsole = null;
    private FpdFileContents ffc = null;
    private JPanel jPanelFdf = null;
    private JCheckBox jCheckBoxFdf = null;
    private JTextField jTextFieldFdf = null;
    private JButton jButtonFdfBrowse = null;
    private JLabel jLabelFvImageOptName = null;
    private JTextField jTextFieldFvImageOptName = null;
    private JLabel jLabelFvImageOptValue = null;
    private JTextField jTextFieldFvImageOptValue = null;
    private JPanel jPanelFvImageOptsButtonGroup = null;
    private JButton jButtonFvImageOptAdd = null;
    private JButton jButtonFvImageOptDel = null;
    private JScrollPane jScrollPane = null;
    private JTable jTableFvImageOpts = null;
    private JButton jButtonUpdateFvImage = null;
    private JButton jButtonTest = null;
    private JPanel jPanelFdfN = null;
    private JPanel jPanelFdfS = null;
    private JSplitPane jSplitPaneFdfC = null;
    private JPanel jPanelFdfCTop = null;
    private JPanel jPanelFdfCBottom = null;
    private JPanel jPanelFdfCTopN = null;
    private JPanel jPanelFdfCTopS = null;
    private JPanel jPanelFdfCTopC = null;
    private JPanel jPanelFdfCBottomN = null;
    private JPanel jPanelFdfCBottomC = null;
    private JLabel jLabelFvInFdf = null;
    private JLabel jLabelFvAdditional = null;
    private JScrollPane jScrollPaneFvInFdf = null;
    private JTable jTableFvInFdf = null;
    private NonEditableTableModel fvInFdfTableModel = null;  //  @jve:decl-index=0:visual-constraint=""
    private JButton jButtonFvInFdfOptions = null;
    private JScrollPane jScrollPaneFvAdditional = null;
    private JTable jTableFvAdditional = null;
    private DefaultTableModel fvAddtionalTableModel = null;  //  @jve:decl-index=0:visual-constraint=""
    private JButton jButtonAddFv = null;
    private JButton jButtonDelFv = null;
    private JButton jButtonAddFvOptions = null;
    
    private NonEditableTableModel nonEditableTableModel = null;  //  @jve:decl-index=0:visual-constraint=""
    
    private JPanel jPanelModOrder = null;
    private JPanel jPanelModOrderN = null;
    private JPanel jPanelModOrderS = null;
    private JPanel jPanelModOrderC = null;
    private JScrollPane jScrollPaneModInFv = null;
    private JTable jTableModInFv = null;
    private JPanel jPanelController = null;
    private JScrollPane jScrollPaneFpdModules = null;
    private JTable jTableFpdModules = null;
    private JButton jButtonUp = null;
    private JButton jButtonInsert = null;
    private JButton jButtonRemove = null;
    private JButton jButtonDown = null;
    private JButton jButtonOk = null;
    private JButton jButtonCancel = null;
    
    /**
     * This method initializes jPanelModOrder   
     *  
     * @return javax.swing.JPanel   
     */
    private JPanel getJPanelModOrder() {
//        if (jPanelModOrder == null) {
            jPanelModOrder = new JPanel();
            jPanelModOrder.setLayout(new BorderLayout());
            jPanelModOrder.add(getJPanelModOrderN(), java.awt.BorderLayout.NORTH);
            jPanelModOrder.add(getJPanelModOrderS(), java.awt.BorderLayout.SOUTH);
            jPanelModOrder.add(getJPanelModOrderC(), java.awt.BorderLayout.CENTER);
            jPanelModOrder.addComponentListener(new java.awt.event.ComponentAdapter() {
                public void componentShown(java.awt.event.ComponentEvent e) {
                    System.out.println("componentShown()" + jTabbedPane.getTitleAt(jTabbedPane.getSelectedIndex())); // TODO Auto-generated Event stub componentShown()
                }
            });
//        }
        return jPanelModOrder;
    }

    /**
     * This method initializes jPanelModOrderN  
     *  
     * @return javax.swing.JPanel   
     */
    private JPanel getJPanelModOrderN() {
//        if (jPanelModOrderN == null) {
            jPanelModOrderN = new JPanel();
//        }
        return jPanelModOrderN;
    }

    /**
     * This method initializes jPanelModOrderS  
     *  
     * @return javax.swing.JPanel   
     */
    private JPanel getJPanelModOrderS() {
//        if (jPanelModOrderS == null) {
            FlowLayout flowLayout6 = new FlowLayout();
            flowLayout6.setAlignment(java.awt.FlowLayout.RIGHT);
            jPanelModOrderS = new JPanel();
            jPanelModOrderS.setLayout(flowLayout6);
            jPanelModOrderS.add(getJButtonOk(), null);
            jPanelModOrderS.add(getJButtonCancel(), null);
//        }
        return jPanelModOrderS;
    }

    /**
     * This method initializes jPanelModOrderC  
     *  
     * @return javax.swing.JPanel   
     */
    private JPanel getJPanelModOrderC() {
//        if (jPanelModOrderC == null) {
            jPanelModOrderC = new JPanel();
            jPanelModOrderC.add(getJScrollPaneModInFv(), null);
            jPanelModOrderC.add(getJPanelController(), null);
            jPanelModOrderC.add(getJScrollPaneFpdModules(), null);
//        }
        return jPanelModOrderC;
    }

    /**
     * This method initializes jScrollPaneModInFv   
     *  
     * @return javax.swing.JScrollPane  
     */
    private JScrollPane getJScrollPaneModInFv() {
//        if (jScrollPaneModInFv == null) {
            jScrollPaneModInFv = new JScrollPane();
            jScrollPaneModInFv.setPreferredSize(new java.awt.Dimension(150,500));
            jScrollPaneModInFv.setViewportView(getJTableModInFv());
//        }
        return jScrollPaneModInFv;
    }

    /**
     * This method initializes jTableModInFv    
     *  
     * @return javax.swing.JTable   
     */
    private JTable getJTableModInFv() {
//        if (jTableModInFv == null) {
            NonEditableTableModel modInFvTableModel = new NonEditableTableModel();
            modInFvTableModel.addColumn("Module Orders in FV");
            jTableModInFv = new JTable(modInFvTableModel);
            jTableModInFv.setRowHeight(20);
//        }
        return jTableModInFv;
    }

    /**
     * This method initializes jPanelController 
     *  
     * @return javax.swing.JPanel   
     */
    private JPanel getJPanelController() {
//        if (jPanelController == null) {
            FlowLayout flowLayout5 = new FlowLayout();
            flowLayout5.setVgap(50);
            flowLayout5.setHgap(50);
            jPanelController = new JPanel();
            jPanelController.setLayout(flowLayout5);
            jPanelController.setPreferredSize(new java.awt.Dimension(150,500));
            jPanelController.add(getJButtonUp(), null);
            jPanelController.add(getJButtonInsert(), null);
            jPanelController.add(getJButtonRemove(), null);
            jPanelController.add(getJButtonDown(), null);
//        }
        return jPanelController;
    }

    /**
     * This method initializes jScrollPaneFpdModules    
     *  
     * @return javax.swing.JScrollPane  
     */
    private JScrollPane getJScrollPaneFpdModules() {
//        if (jScrollPaneFpdModules == null) {
            jScrollPaneFpdModules = new JScrollPane();
            jScrollPaneFpdModules.setPreferredSize(new java.awt.Dimension(150,500));
            jScrollPaneFpdModules.setViewportView(getJTableFpdModules());
//        }
        return jScrollPaneFpdModules;
    }

    /**
     * This method initializes jTableFpdModules 
     *  
     * @return javax.swing.JTable   
     */
    private JTable getJTableFpdModules() {
//        if (jTableFpdModules == null) {
            NonEditableTableModel fpdModTableModel = new NonEditableTableModel();
            fpdModTableModel.addColumn("Modules in Platform");
            jTableFpdModules = new JTable(fpdModTableModel);
            jTableFpdModules.setRowHeight(20);
//        }
        return jTableFpdModules;
    }

    /**
     * This method initializes jButtonUp    
     *  
     * @return javax.swing.JButton  
     */
    private JButton getJButtonUp() {
//        if (jButtonUp == null) {
            jButtonUp = new JButton();
            jButtonUp.setPreferredSize(new java.awt.Dimension(60,20));
            jButtonUp.setFont(new java.awt.Font("Dialog", java.awt.Font.BOLD, 14));
            jButtonUp.setText("^");
//        }
        return jButtonUp;
    }

    /**
     * This method initializes jButtonInsert    
     *  
     * @return javax.swing.JButton  
     */
    private JButton getJButtonInsert() {
//        if (jButtonInsert == null) {
            jButtonInsert = new JButton();
            jButtonInsert.setText("<<");
            jButtonInsert.setPreferredSize(new java.awt.Dimension(60,20));
//        }
        return jButtonInsert;
    }

    /**
     * This method initializes jButtonRemove    
     *  
     * @return javax.swing.JButton  
     */
    private JButton getJButtonRemove() {
//        if (jButtonRemove == null) {
            jButtonRemove = new JButton();
            jButtonRemove.setPreferredSize(new java.awt.Dimension(60,20));
            jButtonRemove.setText(">>");
//        }
        return jButtonRemove;
    }

    /**
     * This method initializes jButtonDown  
     *  
     * @return javax.swing.JButton  
     */
    private JButton getJButtonDown() {
//        if (jButtonDown == null) {
            jButtonDown = new JButton();
            jButtonDown.setPreferredSize(new java.awt.Dimension(60,20));
            jButtonDown.setFont(new java.awt.Font("Dialog", java.awt.Font.BOLD, 10));
            jButtonDown.setText("v");
//        }
        return jButtonDown;
    }
    
    /**
     * This method initializes jButtonOk    
     *  
     * @return javax.swing.JButton  
     */
    private JButton getJButtonOk() {
//        if (jButtonOk == null) {
            jButtonOk = new JButton();
            jButtonOk.setPreferredSize(new java.awt.Dimension(80,20));
            jButtonOk.setText("Ok");
//        }
        return jButtonOk;
    }

    /**
     * This method initializes jButtonCancel    
     *  
     * @return javax.swing.JButton  
     */
    private JButton getJButtonCancel() {
//        if (jButtonCancel == null) {
            jButtonCancel = new JButton();
            jButtonCancel.setPreferredSize(new java.awt.Dimension(80,20));
            jButtonCancel.setText("Cancel");
            jButtonCancel.addActionListener(new java.awt.event.ActionListener() {
                public void actionPerformed(java.awt.event.ActionEvent e) {
                    getJTabbedPane().setSelectedIndex(0);
                }
            });
//        }
        return jButtonCancel;
    }

    
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
    
    public FpdFlash(OpeningPlatformType opt) {
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
     * This method initializes jPanel1	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelContentSouth() {
        if (jPanelContentSouth == null) {
            FlowLayout flowLayout = new FlowLayout();
            flowLayout.setAlignment(java.awt.FlowLayout.RIGHT);
            flowLayout.setHgap(15);
            jPanelContentSouth = new JPanel();
            jPanelContentSouth.setLayout(flowLayout);
            jPanelContentSouth.setComponentOrientation(java.awt.ComponentOrientation.LEFT_TO_RIGHT);
        }
        return jPanelContentSouth;
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
            jTabbedPane.addTab("General", null, getJPanelFdf(), null);
            jTabbedPane.addTab("FV Parameters", null, getJPanelFvImages(), null);
            
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
     * This method initializes jPanelFvImageN	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelFvImageN() {
        if (jPanelFvImageN == null) {
            jLabelFvPropValue = new JLabel();
            jLabelFvPropValue.setText("Value");
            jLabelFvPropValue.setEnabled(false);
            jLabelFvPropValue.setPreferredSize(new java.awt.Dimension(38,20));
            jLabelFvPropName = new JLabel();
            jLabelFvPropName.setText("Name");
            jLabelFvPropName.setEnabled(false);
            jLabelFvPropName.setPreferredSize(new java.awt.Dimension(38,20));
            FlowLayout flowLayout2 = new FlowLayout();
            flowLayout2.setAlignment(java.awt.FlowLayout.LEFT);
            flowLayout2.setHgap(15);
            jPanelFvImageN = new JPanel();
            jPanelFvImageN.setPreferredSize(new java.awt.Dimension(576,100));
            jPanelFvImageN.setBorder(javax.swing.BorderFactory.createBevelBorder(javax.swing.border.BevelBorder.RAISED));
            jPanelFvImageN.setLayout(flowLayout2);
            jPanelFvImageN.add(getJCheckBoxFvProperty(), null);
            jPanelFvImageN.add(jLabelFvPropName, null);
            jPanelFvImageN.add(getJTextFieldFvPropName(), null);
            jPanelFvImageN.add(jLabelFvPropValue, null);
            jPanelFvImageN.add(getJTextFieldFvPropValue(), null);
            jPanelFvImageN.add(getJScrollPaneFvProp(), null);
            jPanelFvImageN.add(getJPanelFvPropButtonGroup(), null);
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
            jPanelFvImageS.add(getJScrollPaneFvInfo(), null);
        }
        return jPanelFvImageS;
    }


    /**
     * This method initializes jCheckBox1	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBoxFvProperty() {
        if (jCheckBoxFvProperty == null) {
            jCheckBoxFvProperty = new JCheckBox();
            jCheckBoxFvProperty.setText("Global FV Variables");
            jCheckBoxFvProperty.addItemListener(new ItemListener(){

                public void itemStateChanged(ItemEvent arg0) {
                    // TODO Auto-generated method stub
                    boolean seleted = jCheckBoxFvProperty.isSelected();
                    
                        jLabelFvPropName.setEnabled(seleted);
                        jTextFieldFvPropName.setEnabled(seleted);
                        jLabelFvPropValue.setEnabled(seleted);
                        jTextFieldFvPropValue.setEnabled(seleted);
                        jTableFvProp.setEnabled(seleted);
                        jButtonFvPropAdd.setEnabled(seleted);
                        jButtonFvPropDel.setEnabled(seleted);
                    
                   
                }
                
            });
        }
        return jCheckBoxFvProperty;
    }


    /**
     * This method initializes jTextField	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldFvPropName() {
        if (jTextFieldFvPropName == null) {
            jTextFieldFvPropName = new JTextField();
            jTextFieldFvPropName.setPreferredSize(new java.awt.Dimension(100,20));
            jTextFieldFvPropName.setEnabled(false);
        }
        return jTextFieldFvPropName;
    }


    /**
     * This method initializes jTextField1	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldFvPropValue() {
        if (jTextFieldFvPropValue == null) {
            jTextFieldFvPropValue = new JTextField();
            jTextFieldFvPropValue.setPreferredSize(new java.awt.Dimension(100,20));
            jTextFieldFvPropValue.setEnabled(false);
        }
        return jTextFieldFvPropValue;
    }


    /**
     * This method initializes jButton	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonFvPropAdd() {
        if (jButtonFvPropAdd == null) {
            jButtonFvPropAdd = new JButton();
            jButtonFvPropAdd.setPreferredSize(new java.awt.Dimension(80,20));
            jButtonFvPropAdd.setEnabled(false);
            jButtonFvPropAdd.setText("Add");
            jButtonFvPropAdd.addActionListener(new AbstractAction() {
                /**
                 * 
                 */
                private static final long serialVersionUID = 1L;

                public void actionPerformed(ActionEvent e) {
                    if (jTextFieldFvPropName.getText().length() > 0 && jTextFieldFvPropValue.getText().length() > 0){
                        String[] row = {jTextFieldFvPropName.getText(), jTextFieldFvPropValue.getText()};                        
                        fvPropertyTableModel.addRow(row);
                        docConsole.setSaved(false);
                        ffc.genFvImagesNameValue(row[0], row[1]);
                    }
                }
            });
        }
        return jButtonFvPropAdd;
    }


    /**
     * This method initializes jScrollPane1	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPaneFvProp() {
        if (jScrollPaneFvProp == null) {
            jScrollPaneFvProp = new JScrollPane();
            jScrollPaneFvProp.setPreferredSize(new java.awt.Dimension(350,55));
            jScrollPaneFvProp.setViewportView(getJTableFvProp());
        }
        return jScrollPaneFvProp;
    }


    /**
     * This method initializes jTable	
     * 	
     * @return javax.swing.JTable	
     */
    private JTable getJTableFvProp() {
        if (jTableFvProp == null) {
            fvPropertyTableModel = new DefaultTableModel();
            jTableFvProp = new JTable(fvPropertyTableModel);
            fvPropertyTableModel.addColumn("Name");
            fvPropertyTableModel.addColumn("Value");
            jTableFvProp.setRowHeight(20);
            jTableFvProp.setEnabled(false);
            
            jTableFvProp.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
            jTableFvProp.getSelectionModel().addListSelectionListener(new ListSelectionListener(){
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
            
            jTableFvProp.getModel().addTableModelListener(new TableModelListener(){
                public void tableChanged(TableModelEvent arg0) {
                    // TODO Auto-generated method stub
                    int row = arg0.getFirstRow();
                    TableModel m = (TableModel)arg0.getSource();
                    if (arg0.getType() == TableModelEvent.UPDATE){
                        
                        String name = m.getValueAt(row, 0) + "";
                        String value = m.getValueAt(row, 1) + "";
                        docConsole.setSaved(false);
                        ffc.updateFvImagesNameValue(row, name, value);
                    }
                }
            });
        }
        return jTableFvProp;
    }


    /**
     * This method initializes jPanel4	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelFvPropButtonGroup() {
        if (jPanelFvPropButtonGroup == null) {
            jPanelFvPropButtonGroup = new JPanel();
            jPanelFvPropButtonGroup.setPreferredSize(new java.awt.Dimension(80,55));
            
            jPanelFvPropButtonGroup.add(getJButtonFvPropAdd(), null);
            jPanelFvPropButtonGroup.add(getJButtonFvPropDel(), null);
        }
        return jPanelFvPropButtonGroup;
    }


    /**
     * This method initializes jButton1	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonFvPropDel() {
        if (jButtonFvPropDel == null) {
            jButtonFvPropDel = new JButton();
            jButtonFvPropDel.setPreferredSize(new java.awt.Dimension(80,20));
            jButtonFvPropDel.setEnabled(false);
            jButtonFvPropDel.setText("Delete");
            jButtonFvPropDel.addActionListener(new AbstractAction() {
                /**
                 * 
                 */
                private static final long serialVersionUID = 1L;

                public void actionPerformed(ActionEvent e) {
                    if (jTableFvProp.getSelectedRow() >= 0){
                        fvPropertyTableModel.removeRow(jTableFvProp.getSelectedRow());
                        docConsole.setSaved(false);
                        ffc.removeFvImagesNameValue(jTableFvProp.getSelectedRow());
                    }
                }
            });
        }
        return jButtonFvPropDel;
    }


    /**
     * This method initializes jPanelFvImageC	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelFvImageC() {
        if (jPanelFvImageC == null) {
            jLabelFvParaName = new JLabel();
            jLabelFvParaName.setPreferredSize(new Dimension(38, 20));
            jLabelFvParaName.setEnabled(false);
            jLabelFvParaName.setText("Name");
            jLabelFvImageNames = new JLabel();
            jLabelFvImageNames.setText("FV Image Names");
            jLabelFvImageNames.setEnabled(false);
            jLabelFvImageNames.setPreferredSize(new java.awt.Dimension(150,20));
            jLabelFvParaType = new JLabel();
            jLabelFvParaType.setText("Type");
            jLabelFvParaType.setEnabled(false);
            jLabelFvParaType.setPreferredSize(new java.awt.Dimension(70,20));
            FlowLayout flowLayout3 = new FlowLayout();
            flowLayout3.setAlignment(java.awt.FlowLayout.LEFT);
            flowLayout3.setHgap(5);
            jPanelFvImageC = new JPanel();
            jPanelFvImageC.setLayout(flowLayout3);
            jPanelFvImageC.add(getJPanelFvImagePara(), null);
            
            jPanelFvImageC.add(getJPanelFvImageOpts(), null);
            
        }
        return jPanelFvImageC;
    }


    /**
     * This method initializes jCheckBox2	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBoxFvImagePara() {
        if (jCheckBoxFvImagePara == null) {
            jCheckBoxFvImagePara = new JCheckBox();
            jCheckBoxFvImagePara.setText("FV Image Parameters");
            jCheckBoxFvImagePara.setPreferredSize(new java.awt.Dimension(200,20));
            jCheckBoxFvImagePara.addItemListener(new ItemListener(){

                public void itemStateChanged(ItemEvent arg0) {
                    // TODO Auto-generated method stub
                    boolean seleted = jCheckBoxFvImagePara.isSelected();
                    
                        jLabelFvParaType.setEnabled(seleted);
                        jLabelFvImageNames.setEnabled(seleted);
                        jLabelFvParaName.setEnabled(seleted);
                        jComboBoxFvParaType.setEnabled(seleted);
                        jTextFieldFvParaName.setEnabled(seleted);
                        
                        jTableFvImageNames.setEnabled(seleted);
                        jButtonFvParaAdd.setEnabled(seleted);
                        jButtonFvParaDel.setEnabled(seleted);
                    
                   
                }
                
            });
        }
        return jCheckBoxFvImagePara;
    }


    /**
     * This method initializes jComboBox	
     * 	
     * @return javax.swing.JComboBox	
     */
    private JComboBox getJComboBoxFvParaType() {
        if (jComboBoxFvParaType == null) {
            jComboBoxFvParaType = new JComboBox();
            jComboBoxFvParaType.addItem("ImageName");
            jComboBoxFvParaType.addItem("Attributes");
            jComboBoxFvParaType.addItem("Options");
            jComboBoxFvParaType.addItem("Components");
            jComboBoxFvParaType.setPreferredSize(new java.awt.Dimension(180,20));
            jComboBoxFvParaType.setEnabled(false);
            jComboBoxFvParaType.addItemListener(new ItemListener() {

                public void itemStateChanged(ItemEvent arg0) {
                    // TODO disable attribute settings when ValidImageNames selected.
                    
                }
                
            });
        }
        return jComboBoxFvParaType;
    }


    /**
     * This method initializes jTextField2	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldFvParaName() {
        if (jTextFieldFvParaName == null) {
            jTextFieldFvParaName = new JTextField();
            jTextFieldFvParaName.setPreferredSize(new java.awt.Dimension(140,20));
            jTextFieldFvParaName.setEnabled(false);
        }
        return jTextFieldFvParaName;
    }


    /**
     * This method initializes jButton2	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonFvParaAdd() {
        if (jButtonFvParaAdd == null) {
            jButtonFvParaAdd = new JButton();
            jButtonFvParaAdd.setPreferredSize(new Dimension(80, 20));
            jButtonFvParaAdd.setEnabled(false);
            jButtonFvParaAdd.setText("Add");
            jButtonFvParaAdd.addActionListener(new AbstractAction() {
                /**
                 * 
                 */
                private static final long serialVersionUID = 1L;

                public void actionPerformed(java.awt.event.ActionEvent e) {
                    if (jTextFieldFvParaName.getText().length() > 0){
                        String[] row = {jTextFieldFvParaName.getText()};                        
                        fvImageNameTableModel.addRow(row);
                    }
                }
            });
        }
        return jButtonFvParaAdd;
    }


    /**
     * This method initializes jScrollPane2	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPaneFvImageNames() {
        if (jScrollPaneFvImageNames == null) {
            jScrollPaneFvImageNames = new JScrollPane();
            jScrollPaneFvImageNames.setPreferredSize(new java.awt.Dimension(350,50));
            jScrollPaneFvImageNames.setViewportView(getJTableFvImageNames());
        }
        return jScrollPaneFvImageNames;
    }


    /**
     * This method initializes jTable1	
     * 	
     * @return javax.swing.JTable	
     */
    private JTable getJTableFvImageNames() {
        if (jTableFvImageNames == null) {
            fvImageNameTableModel = new DefaultTableModel();
            jTableFvImageNames = new JTable(fvImageNameTableModel);
            jTableFvImageNames.setEnabled(false);
            fvImageNameTableModel.addColumn("FV Image Name");
        }
        return jTableFvImageNames;
    }


    /**
     * This method initializes jButton3	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonFvParaDel() {
        if (jButtonFvParaDel == null) {
            jButtonFvParaDel = new JButton();
            jButtonFvParaDel.setPreferredSize(new Dimension(80, 20));
            jButtonFvParaDel.setEnabled(false);
            jButtonFvParaDel.setText("Delete");
            jButtonFvParaDel.addActionListener(new AbstractAction() {
                /**
                 * 
                 */
                private static final long serialVersionUID = 1L;

                public void actionPerformed(java.awt.event.ActionEvent e) {
                    if (jTableFvImageNames.getSelectedRow() >= 0){
                                   
                        fvImageNameTableModel.removeRow(jTableFvImageNames.getSelectedRow());
                    }
                }
            });
        }
        return jButtonFvParaDel;
    }



  			
  private JPanel getJPanelFvImageOpts() {
       			

 if (jPanelFvImageOpts == null) {
     //ToDo add ButtonGroup for RadioButtons
            jLabelFvImageOptValue = new JLabel();
            jLabelFvImageOptValue.setEnabled(true);
            jLabelFvImageOptValue.setText("Value");
            jLabelFvImageOptValue.setPreferredSize(new Dimension(38, 20));
            jLabelFvImageOptName = new JLabel();
            jLabelFvImageOptName.setEnabled(true);
            jLabelFvImageOptName.setText("Name");
            jLabelFvImageOptName.setPreferredSize(new Dimension(38, 20));
            jPanelFvImageOpts = new JPanel();
            jPanelFvImageOpts.setPreferredSize(new java.awt.Dimension(480,150));
            
            jPanelFvImageOpts.setLayout(new FlowLayout());
            
			jPanelFvImageOpts.setBorder(javax.swing.BorderFactory.createEtchedBorder(javax.swing.border.EtchedBorder.LOWERED));
			jPanelFvImageOpts.add(getJCheckBoxFvImageOpts(), null);
			jPanelFvImageOpts.add(jLabelFvImageOptName, null);
			jPanelFvImageOpts.add(getJTextFieldFvImageOptName(), null);
			jPanelFvImageOpts.add(jLabelFvImageOptValue, null);
			jPanelFvImageOpts.add(getJTextFieldFvImageOptValue(), null);
			jPanelFvImageOpts.add(getJScrollPane(), null);
			jPanelFvImageOpts.add(getJPanelFvImageOptsButtonGroup(), null);
			jPanelFvImageOpts.add(getJButtonAddFvImage(), null);
			jPanelFvImageOpts.add(getJButtonDelFvImage(), null);
			jPanelFvImageOpts.add(getJButtonUpdateFvImage(), null);
            
            
            			

            
        }
        return jPanelFvImageOpts;
    }


    /**
     * This method initializes jButton4	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonAddFvImage() {
        if (jButtonAddFvImage == null) {
            jButtonAddFvImage = new JButton();
            jButtonAddFvImage.setPreferredSize(new java.awt.Dimension(120,20));
//            jButton4.setEnabled(false);
            jButtonAddFvImage.setText("Add FV Image");
            jButtonAddFvImage.addActionListener(new AbstractAction() {
                /**
                 * 
                 */
                private static final long serialVersionUID = 1L;

                public void actionPerformed(java.awt.event.ActionEvent e) {
                    if (jTableFvInfo.isEditing()) {
                        jTableFvInfo.getCellEditor().stopCellEditing();
                    }
                    if (jTableFvImageOpts.isEditing()) {
                        jTableFvImageOpts.getCellEditor().stopCellEditing();
                    }
                    if (jTableFvImageNames.getRowCount()== 0){
                        return;
                    }
                    String imageName = " ";
                    for (int i = 0; i < jTableFvImageNames.getRowCount(); ++i){
                        imageName += (String)jTableFvImageNames.getValueAt(i, 0);
                        imageName += " ";
                    }
                    imageName = imageName.trim();
                    
                    if (!jCheckBoxFvImageOpts.isSelected() && jComboBoxFvParaType.getSelectedIndex() != 0){
                        return;
                      
                    }
                    
                    LinkedHashMap<String, String> m = null;
                    if (jCheckBoxFvImageOpts.isSelected()) {
                        m = new LinkedHashMap<String, String>();
                        getOptionNameValue(m);
                    }
                    ffc.genFvImagesFvImage(imageName.split(" "), jComboBoxFvParaType.getSelectedItem()+"", m);
                    docConsole.setSaved(false);
                    Object[] row = {imageName, jComboBoxFvParaType.getSelectedItem()};
                    fvImageParaTableModel.addRow(row); 
                }
            });
        }
        return jButtonAddFvImage;
    }


    /**
     * This method initializes jScrollPane3	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPaneFvInfo() {
        if (jScrollPaneFvInfo == null) {
            jScrollPaneFvInfo = new JScrollPane();
            jScrollPaneFvInfo.setPreferredSize(new java.awt.Dimension(480,150));
            jScrollPaneFvInfo.setViewportView(getJTableFvInfo());
        }
        return jScrollPaneFvInfo;
    }


    /**
     * This method initializes jTable2	
     * 	
     * @return javax.swing.JTable	
     */
    private JTable getJTableFvInfo() {
        if (jTableFvInfo == null) {
            fvImageParaTableModel = new ImageParaTableModel();
            jTableFvInfo = new JTable(fvImageParaTableModel);
            fvImageParaTableModel.addColumn("FvImageNames");
            fvImageParaTableModel.addColumn("Type");
            
            
            TableColumn typeCol = jTableFvInfo.getColumnModel().getColumn(1);
            JComboBox cb = new JComboBox();
            cb.addItem("ValidImageNames");
            cb.addItem("Attributes");
            cb.addItem("Options");
            cb.addItem("Components");
            typeCol.setCellEditor(new DefaultCellEditor(cb));
            
            jTableFvInfo.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
            jTableFvInfo.getSelectionModel().addListSelectionListener(new ListSelectionListener(){
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
        return jTableFvInfo;
    }


    /**
     * This method initializes jButton6	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonDelFvImage() {
        if (jButtonDelFvImage == null) {
            jButtonDelFvImage = new JButton();
            jButtonDelFvImage.setPreferredSize(new java.awt.Dimension(120,20));
//            jButton6.setEnabled(false);
            jButtonDelFvImage.setText("Delete Row");
            jButtonDelFvImage.addActionListener(new AbstractAction() {
                /**
                 * 
                 */
                private static final long serialVersionUID = 1L;

                public void actionPerformed(ActionEvent arg0) {
                    // TODO Auto-generated method stub
                    if (jTableFvInfo.getSelectedRow() >= 0 ) {
                        ffc.removeFvImagesFvImage(jTableFvInfo.getSelectedRow());
                        fvImageParaTableModel.removeRow(jTableFvInfo.getSelectedRow());
                        docConsole.setSaved(false);
                    }
                }
                
            });
        }
        return jButtonDelFvImage;
    }


    /**
     * This method initializes jCheckBox3	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBoxFvImageOpts() {
        if (jCheckBoxFvImageOpts == null) {
            jCheckBoxFvImageOpts = new JCheckBox();
            jCheckBoxFvImageOpts.setText("FV Image Options");
            jCheckBoxFvImageOpts.addItemListener(new ItemListener(){

                public void itemStateChanged(ItemEvent arg0) {
                    // TODO Auto-generated method stub
//                    boolean selected = jCheckBox3.isSelected();
                    
                        if (!jCheckBoxFvImagePara.isSelected() || jComboBoxFvParaType.getSelectedIndex() == 0) {
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
        return jCheckBoxFvImageOpts;
    }


    /**
     * This method initializes jPanel6	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelFvImagePara() {
        if (jPanelFvImagePara == null) {
            StarLabel starLabel = new StarLabel();
            starLabel.setVisible(false);
            jPanelFvImagePara = new JPanel();
            jPanelFvImagePara.setPreferredSize(new java.awt.Dimension(480,120));
            jPanelFvImagePara.setBorder(javax.swing.BorderFactory.createEtchedBorder(javax.swing.border.EtchedBorder.RAISED));
            jPanelFvImagePara.add(getJCheckBoxFvImagePara(), null);
            jPanelFvImagePara.add(jLabelFvParaType, null);
            jPanelFvImagePara.add(getJComboBoxFvParaType(), null);
            jPanelFvImagePara.add(new StarLabel(), null);
            jPanelFvImagePara.add(jLabelFvImageNames, null);
            jPanelFvImagePara.add(jLabelFvParaName, null);
            jPanelFvImagePara.add(getJTextFieldFvParaName(), null);
            jPanelFvImagePara.add(getJButtonFvParaAdd(), null);
            jPanelFvImagePara.add(getJScrollPaneFvImageNames(), null);
            jPanelFvImagePara.add(getJButtonFvParaDel(), null);
        }
        return jPanelFvImagePara;
    }

    /**
     * This method initializes jPanel7	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelFdf() {
        if (jPanelFdf == null) {
            jPanelFdf = new JPanel();
            jPanelFdf.setLayout(new BorderLayout());

            jPanelFdf.add(getJPanelFdfN(), java.awt.BorderLayout.NORTH);
            jPanelFdf.add(getJPanelFdfS(), java.awt.BorderLayout.SOUTH);
            jPanelFdf.add(getJSplitPaneFdfC(), java.awt.BorderLayout.CENTER);
//            jPanelFdf.addComponentListener(new ComponentAdapter(){
//                public void componentShown(ComponentEvent e) {
//                    if (ffc.getFlashDefinitionFile() != null) {
//                        jTextFieldFdf.setText(ffc.getFlashDefinitionFile());
//                    }
//                }
//                public void componentHidden(ComponentEvent e) {
//                    if (jCheckBoxFdf.isSelected()) {
//                        ffc.genFlashDefinitionFile(jTextFieldFdf.getText());
//                    }
//                }
//            });
        }
        return jPanelFdf;
    }

    /**
     * This method initializes jCheckBox	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBoxFdf() {
        if (jCheckBoxFdf == null) {
            jCheckBoxFdf = new JCheckBox();
            jCheckBoxFdf.setText("Flash Definition File");
            jCheckBoxFdf.addItemListener(new ItemListener() {

                public void itemStateChanged(ItemEvent arg0) {
                    // TODO Auto-generated method stub
                    if (jCheckBoxFdf.isSelected()){
                        jTextFieldFdf.setEnabled(true);
                        jButtonFdfBrowse.setEnabled(true);
                    }
                    else {
                        
                        jTextFieldFdf.setEnabled(false);
                        jButtonFdfBrowse.setEnabled(false);
                    }
                }
            });
        }
        return jCheckBoxFdf;
    }

    /**
     * This method initializes jTextField3	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldFdf() {
        if (jTextFieldFdf == null) {
            jTextFieldFdf = new JTextField();
            jTextFieldFdf.setEnabled(false);
            jTextFieldFdf.setPreferredSize(new Dimension(300, 20));
            jTextFieldFdf.addFocusListener(new java.awt.event.FocusAdapter() {
                public void focusLost(java.awt.event.FocusEvent e) {
                    ffc.genFlashDefinitionFile(jTextFieldFdf.getText());
                }
            });
        }
        return jTextFieldFdf;
    }

    /**
     * This method initializes jButton5	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonFdfBrowse() {
        if (jButtonFdfBrowse == null) {
            jButtonFdfBrowse = new JButton();
            jButtonFdfBrowse.setEnabled(false);
            jButtonFdfBrowse.setText("Browse");
            jButtonFdfBrowse.setPreferredSize(new Dimension(78, 20));
            jButtonFdfBrowse.addActionListener(new AbstractAction(){
                /**
                 * 
                 */
                private static final long serialVersionUID = 1L;

                public void actionPerformed(ActionEvent e) {
                    // TODO Auto-generated method stub
                    String wsDir = System.getenv("WORKSPACE");
                    JFileChooser chooser = new JFileChooser(wsDir);
                    chooser.setMultiSelectionEnabled(false);
                    chooser.setFileSelectionMode(JFileChooser.FILES_AND_DIRECTORIES);
                    int retval = chooser.showOpenDialog(frame);
                    if (retval == JFileChooser.APPROVE_OPTION) {

                        File theFile = chooser.getSelectedFile();
                        String filePath = theFile.getPath();
                        if (!filePath.startsWith(wsDir)) {
                            JOptionPane.showMessageDialog(frame, "You can only select files in current WORKSPACE.");
                            return;
                        }
                        jTextFieldFdf.setText(filePath.substring(wsDir.length() + 1).replace('\\', '/'));
                        ffc.genFlashDefinitionFile(jTextFieldFdf.getText());
                        docConsole.setSaved(false);
                        Vector<FvInfoFromFdf> vFvInfo = new Vector<FvInfoFromFdf>();
                        getFvInfoFromFdf(filePath, vFvInfo);
                        getFvInFdfTableModel().setRowCount(0);
                        for (int j = 0; j < vFvInfo.size(); ++j) {
                            FvInfoFromFdf fvInfo = vFvInfo.get(j);
                            String[] row = {fvInfo.getFvName(), fvInfo.getSize(), fvInfo.getEfiFileName()};
                            getFvInFdfTableModel().addRow(row);
                        }
                    }
                }
                
            });
        }
        return jButtonFdfBrowse;
    }

    /**
     * This method initializes jTextField4	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldFvImageOptName() {
        if (jTextFieldFvImageOptName == null) {
            jTextFieldFvImageOptName = new JTextField();
            jTextFieldFvImageOptName.setEnabled(true);
            jTextFieldFvImageOptName.setPreferredSize(new Dimension(100, 20));
        }
        return jTextFieldFvImageOptName;
    }

    /**
     * This method initializes jTextField5	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldFvImageOptValue() {
        if (jTextFieldFvImageOptValue == null) {
            jTextFieldFvImageOptValue = new JTextField();
            jTextFieldFvImageOptValue.setEnabled(true);
            jTextFieldFvImageOptValue.setPreferredSize(new Dimension(100, 20));
        }
        return jTextFieldFvImageOptValue;
    }

    /**
     * This method initializes jPanel8	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelFvImageOptsButtonGroup() {
        if (jPanelFvImageOptsButtonGroup == null) {
            jPanelFvImageOptsButtonGroup = new JPanel();
            jPanelFvImageOptsButtonGroup.setPreferredSize(new Dimension(80, 55));
            jPanelFvImageOptsButtonGroup.add(getJButtonFvImageOptAdd(), null);
            jPanelFvImageOptsButtonGroup.add(getJButtonFvImageOptDel(), null);
        }
        return jPanelFvImageOptsButtonGroup;
    }

    /**
     * This method initializes jButton7	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonFvImageOptAdd() {
        if (jButtonFvImageOptAdd == null) {
            jButtonFvImageOptAdd = new JButton();
            jButtonFvImageOptAdd.setEnabled(true);
            jButtonFvImageOptAdd.setText("Add");
            jButtonFvImageOptAdd.setPreferredSize(new Dimension(80, 20));
            jButtonFvImageOptAdd.addActionListener(new AbstractAction() {
                /**
                 * 
                 */
                private static final long serialVersionUID = 1L;

                public void actionPerformed(ActionEvent e) {
                    if (jTextFieldFvImageOptName.getText().length() > 0 && jTextFieldFvImageOptValue.getText().length() > 0){
                        String[] row = {jTextFieldFvImageOptName.getText(), jTextFieldFvImageOptValue.getText()};                        
                        fvOptionTableModel.addRow(row);
                    }
                }
            });
        }
        return jButtonFvImageOptAdd;
    }

    /**
     * This method initializes jButton8	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonFvImageOptDel() {
        if (jButtonFvImageOptDel == null) {
            jButtonFvImageOptDel = new JButton();
            jButtonFvImageOptDel.setEnabled(true);
            jButtonFvImageOptDel.setText("Delete");
            jButtonFvImageOptDel.setPreferredSize(new Dimension(80, 20));
            jButtonFvImageOptDel.addActionListener(new AbstractAction() {
                /**
                 * 
                 */
                private static final long serialVersionUID = 1L;

                public void actionPerformed(ActionEvent e) {
                    if (jTableFvImageOpts.getSelectedRow() >= 0){
                        fvOptionTableModel.removeRow(jTableFvProp.getSelectedRow());
                    }
                }
            });
        }
        return jButtonFvImageOptDel;
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
            jScrollPane.setViewportView(getJTableFvImageOpts());
        }
        return jScrollPane;
    }

    /**
     * This method initializes jTable3	
     * 	
     * @return javax.swing.JTable	
     */
    private JTable getJTableFvImageOpts() {
        if (jTableFvImageOpts == null) {
            fvOptionTableModel = new DefaultTableModel();
            fvOptionTableModel.addColumn("Name");
            fvOptionTableModel.addColumn("Value");
            jTableFvImageOpts = new JTable(fvOptionTableModel);

            jTableFvImageOpts.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
            jTableFvImageOpts.setRowHeight(20);
            
        }
        return jTableFvImageOpts;
    }

    /**
     * This method initializes jButton9	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonUpdateFvImage() {
        if (jButtonUpdateFvImage == null) {
            jButtonUpdateFvImage = new JButton();
            jButtonUpdateFvImage.setPreferredSize(new Dimension(120, 20));
            jButtonUpdateFvImage.setActionCommand("Update");
            jButtonUpdateFvImage.setText("Update FV");
            jButtonUpdateFvImage.addActionListener(new java.awt.event.ActionListener() {
                public void actionPerformed(java.awt.event.ActionEvent e) {
                    int row = jTableFvInfo.getSelectedRow();
                    if (jTableFvInfo.isEditing()) {
                        jTableFvInfo.getCellEditor().stopCellEditing();
                    }
                    if (jTableFvImageOpts.isEditing()) {
                        jTableFvImageOpts.getCellEditor().stopCellEditing();
                    }
                    
                        //ToDo Check data validity before update
                        String name = fvImageParaTableModel.getValueAt(row, 0) + "";
                        String type = fvImageParaTableModel.getValueAt(row, 1) + "";
                        
                        LinkedHashMap<String, String> lhm = new LinkedHashMap<String, String>();
                        getOptionNameValue(lhm);
                        
                        docConsole.setSaved(false);
                        ffc.updateFvImagesFvImage(row, name.split(" "), type, lhm);
                    
                }
            });
        }
        return jButtonUpdateFvImage;
    }

    /**
     * This method initializes jButtonTest	
     * 	
     * @return javax.swing.JButton	
     */
//    private JButton getJButtonTest() {
//        if (jButtonTest == null) {
//            jButtonTest = new JButton();
//            jButtonTest.addActionListener(new java.awt.event.ActionListener() {
//                public void actionPerformed(java.awt.event.ActionEvent e) {
//                    docConsole.setSaved(false);
//                    String[][] includeModules = {{"1", "a"}, {"2", "b"}};
//                    ffc.genBuildOptionsUserExtensions("FV_RECOVERY", "FvRecovery.inf", null, includeModules);
//                    int i = ffc.getUserExtsIncModCount("FV_RECOVERY");
//                    String[][] saa = new String[i][2];
//                    ffc.getUserExtsIncMods("FV_RECOVERY", saa);
//                    jTextFieldFdf.setText(saa[0][0] + saa[0][1] + saa[1][0] + saa[1][1]);
//                    
//                }
//            });
//        }
//        return jButtonTest;
//    }

    /**
     * This method initializes jPanelFdfN	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelFdfN() {
        if (jPanelFdfN == null) {
            jPanelFdfN = new JPanel();
            jPanelFdfN.add(getJCheckBoxFdf(), null);
            jPanelFdfN.add(getJTextFieldFdf(), null);
            jPanelFdfN.add(getJButtonFdfBrowse(), null);
        }
        return jPanelFdfN;
    }

    /**
     * This method initializes jPanelFdfS	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelFdfS() {
        if (jPanelFdfS == null) {
            FlowLayout flowLayout4 = new FlowLayout();
            flowLayout4.setAlignment(java.awt.FlowLayout.RIGHT);
            jPanelFdfS = new JPanel();
            jPanelFdfS.setLayout(flowLayout4);
            jPanelFdfS.add(getJButtonAddFv(), null);
            jPanelFdfS.add(getJButtonDelFv(), null);
            jPanelFdfS.add(getJButtonAddFvOptions(), null);
        }
        return jPanelFdfS;
    }

    /**
     * This method initializes jSplitPaneFdfC	
     * 	
     * @return javax.swing.JSplitPane	
     */
    private JSplitPane getJSplitPaneFdfC() {
        if (jSplitPaneFdfC == null) {
            jSplitPaneFdfC = new JSplitPane();
            jSplitPaneFdfC.setOrientation(javax.swing.JSplitPane.VERTICAL_SPLIT);
            jSplitPaneFdfC.setDividerSize(5);
            jSplitPaneFdfC.setTopComponent(getJPanelFdfCTop());
            jSplitPaneFdfC.setBottomComponent(getJPanelFdfCBottom());
            jSplitPaneFdfC.setDividerLocation(280);
        }
        return jSplitPaneFdfC;
    }

    /**
     * This method initializes jPanelFdfCTop	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelFdfCTop() {
        if (jPanelFdfCTop == null) {
            jPanelFdfCTop = new JPanel();
            jPanelFdfCTop.setLayout(new BorderLayout());
            jPanelFdfCTop.add(getJPanelFdfCTopN(), java.awt.BorderLayout.NORTH);
            jPanelFdfCTop.add(getJPanelFdfCTopS(), java.awt.BorderLayout.SOUTH);
            jPanelFdfCTop.add(getJPanelFdfCTopC(), java.awt.BorderLayout.CENTER);
        }
        return jPanelFdfCTop;
    }

    /**
     * This method initializes jPanelFdfCBottom	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelFdfCBottom() {
        if (jPanelFdfCBottom == null) {
            jPanelFdfCBottom = new JPanel();
            jPanelFdfCBottom.setLayout(new BorderLayout());
            jPanelFdfCBottom.add(getJPanelFdfCBottomN(), java.awt.BorderLayout.NORTH);
            jPanelFdfCBottom.add(getJPanelFdfCBottomC(), java.awt.BorderLayout.CENTER);
        }
        return jPanelFdfCBottom;
    }

    /**
     * This method initializes jPanelFdfCTopN	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelFdfCTopN() {
        if (jPanelFdfCTopN == null) {
            jLabelFvInFdf = new JLabel();
            jLabelFvInFdf.setText("FVs in Flash Definition File");
            jPanelFdfCTopN = new JPanel();
            jPanelFdfCTopN.add(jLabelFvInFdf, null);
        }
        return jPanelFdfCTopN;
    }

    /**
     * This method initializes jPanelFdfCTopS	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelFdfCTopS() {
        if (jPanelFdfCTopS == null) {
            FlowLayout flowLayout1 = new FlowLayout();
            flowLayout1.setAlignment(java.awt.FlowLayout.RIGHT);
            jPanelFdfCTopS = new JPanel();
            jPanelFdfCTopS.setLayout(flowLayout1);
            jPanelFdfCTopS.add(getJButtonFvInFdfOptions(), null);
        }
        return jPanelFdfCTopS;
    }

    /**
     * This method initializes jPanelFdfCTopC	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelFdfCTopC() {
        if (jPanelFdfCTopC == null) {
            jPanelFdfCTopC = new JPanel();
            jPanelFdfCTopC.add(getJScrollPaneFvInFdf(), null);
        }
        return jPanelFdfCTopC;
    }

    /**
     * This method initializes jPanelFdfCBottomN	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelFdfCBottomN() {
        if (jPanelFdfCBottomN == null) {
            jLabelFvAdditional = new JLabel();
            jLabelFvAdditional.setText("Additional FVs");
            jPanelFdfCBottomN = new JPanel();
            jPanelFdfCBottomN.add(jLabelFvAdditional, null);
        }
        return jPanelFdfCBottomN;
    }

    /**
     * This method initializes jPanelFdfCBottomC	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelFdfCBottomC() {
        if (jPanelFdfCBottomC == null) {
            jPanelFdfCBottomC = new JPanel();
            jPanelFdfCBottomC.add(getJScrollPaneFvAdditional(), null);
        }
        return jPanelFdfCBottomC;
    }

    /**
     * This method initializes jScrollPaneFvInFdf	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPaneFvInFdf() {
        if (jScrollPaneFvInFdf == null) {
            jScrollPaneFvInFdf = new JScrollPane();
            jScrollPaneFvInFdf.setPreferredSize(new java.awt.Dimension(653,200));
            jScrollPaneFvInFdf.setViewportView(getJTableFvInFdf());
        }
        return jScrollPaneFvInFdf;
    }

    /**
     * This method initializes jTableFvInFdf	
     * 	
     * @return javax.swing.JTable	
     */
    private JTable getJTableFvInFdf() {
        if (jTableFvInFdf == null) {
            jTableFvInFdf = new JTable();
            jTableFvInFdf.setRowHeight(20);
            jTableFvInFdf.setSelectionMode(javax.swing.ListSelectionModel.SINGLE_SELECTION);
            jTableFvInFdf.setModel(getFvInFdfTableModel());
        }
        return jTableFvInFdf;
    }

    /**
     * This method initializes fvInFdfTableModel	
     * 	
     * @return org.tianocore.frameworkwizard.platform.ui.NonEditableTableModel	
     */
    private NonEditableTableModel getFvInFdfTableModel() {
        if (fvInFdfTableModel == null) {
            fvInFdfTableModel = new NonEditableTableModel();
            fvInFdfTableModel.addColumn("FV Name");
            fvInFdfTableModel.addColumn("Size");
            fvInFdfTableModel.addColumn("Corresponding File Name");
        }
        return fvInFdfTableModel;
    }

    /**
     * This method initializes jButtonFvInFdfOptions	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonFvInFdfOptions() {
        if (jButtonFvInFdfOptions == null) {
            jButtonFvInFdfOptions = new JButton();
            jButtonFvInFdfOptions.setPreferredSize(new java.awt.Dimension(80,20));
            jButtonFvInFdfOptions.setText("Options");
            jButtonFvInFdfOptions.addActionListener(new java.awt.event.ActionListener() {
                public void actionPerformed(java.awt.event.ActionEvent e) {
                    int selectedRow = jTableFvInFdf.getSelectedRow();
                    if (selectedRow <= 0) {
                        return;
                    }
                    fvInFdfTableModel.moveRow(selectedRow, selectedRow, selectedRow - 1);
                    jTableFvInFdf.changeSelection(selectedRow - 1, 0, false, false);
                }
            });
        }
        return jButtonFvInFdfOptions;
    }

    /**
     * This method initializes jScrollPaneFvAdditional	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPaneFvAdditional() {
        if (jScrollPaneFvAdditional == null) {
            jScrollPaneFvAdditional = new JScrollPane();
            jScrollPaneFvAdditional.setPreferredSize(new java.awt.Dimension(653,200));
            jScrollPaneFvAdditional.setViewportView(getJTableFvAdditional());
        }
        return jScrollPaneFvAdditional;
    }

    /**
     * This method initializes jTableFvAdditional	
     * 	
     * @return javax.swing.JTable	
     */
    private JTable getJTableFvAdditional() {
        if (jTableFvAdditional == null) {
            jTableFvAdditional = new JTable();
            jTableFvAdditional.setRowHeight(20);
            jTableFvAdditional.setSelectionMode(javax.swing.ListSelectionModel.SINGLE_SELECTION);
            jTableFvAdditional.setModel(getFvAddtionalTableModel());
        }
        return jTableFvAdditional;
    }

    /**
     * This method initializes fvAddtionalTableModel	
     * 	
     * @return javax.swing.table.DefaultTableModel	
     */
    private DefaultTableModel getFvAddtionalTableModel() {
        if (fvAddtionalTableModel == null) {
            fvAddtionalTableModel = new DefaultTableModel();
            fvAddtionalTableModel.addColumn("FV Name");
            fvAddtionalTableModel.addColumn("Size");
            fvAddtionalTableModel.addColumn("Corresponding File Name");
        }
        return fvAddtionalTableModel;
    }

    /**
     * This method initializes jButtonAddFv	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonAddFv() {
        if (jButtonAddFv == null) {
            jButtonAddFv = new JButton();
            jButtonAddFv.setPreferredSize(new java.awt.Dimension(80,20));
            jButtonAddFv.setText("New");
        }
        return jButtonAddFv;
    }

    /**
     * This method initializes jButtonDelFv	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonDelFv() {
        if (jButtonDelFv == null) {
            jButtonDelFv = new JButton();
            jButtonDelFv.setPreferredSize(new java.awt.Dimension(80,20));
            jButtonDelFv.setText("Delete");
        }
        return jButtonDelFv;
    }

    /**
     * This method initializes jButtonAddFvOptions	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonAddFvOptions() {
        if (jButtonAddFvOptions == null) {
            jButtonAddFvOptions = new JButton();
            jButtonAddFvOptions.setPreferredSize(new java.awt.Dimension(80,20));
            jButtonAddFvOptions.setText("Options");
        }
        return jButtonAddFvOptions;
    }

    

    /**
     * This method initializes nonEditableTableModel	
     * 	
     * @return org.tianocore.frameworkwizard.platform.ui.NonEditableTableModel	
     */
    private NonEditableTableModel getNonEditableTableModel() {
        if (nonEditableTableModel == null) {
            nonEditableTableModel = new NonEditableTableModel();
        }
        return nonEditableTableModel;
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
                if (jTableFvProp.isEditing()) {
                    jTableFvProp.getCellEditor().stopCellEditing();
                }
                if (jTableFvImageNames.isEditing()) {
                    jTableFvImageNames.getCellEditor().stopCellEditing();
                }
                if (jTableFvInfo.isEditing()) {
                    jTableFvInfo.getCellEditor().stopCellEditing();
                }
            }
        });
    }

    private void init(FpdFileContents ffc) {
        if (ffc.getFvImagesFvImageCount() == 0) {
            return;
        }
        String[][] saa = new String[ffc.getFvImagesFvImageCount()][2];
        ffc.getFvImagesFvImages(saa);
       
        int i = 0;
        while (i < saa.length) {
            
            fvImageParaTableModel.addRow(saa[i]);
            ++i;
        }
        
        saa = new String[ffc.getFvImagesNameValueCount()][2];
        ffc.getFvImagesNameValues(saa);
        for (int m = 0; m < saa.length; ++m) {
            fvPropertyTableModel.addRow(saa[m]);
        }
        
        jTextFieldFdf.setText("");
        String fdfFile = ffc.getFlashDefinitionFile();
        if (fdfFile != null) {
            jTextFieldFdf.setText(fdfFile);
        }
        
        String fdfPath = System.getenv("WORKSPACE") + File.separator + fdfFile;
        Vector<FvInfoFromFdf> vFvInfo = new Vector<FvInfoFromFdf>();
        getFvInfoFromFdf(fdfPath, vFvInfo);
        getFvInFdfTableModel().setRowCount(0);
        for (int j = 0; j < vFvInfo.size(); ++j) {
            FvInfoFromFdf fvInfo = vFvInfo.get(j);
            String[] row = {fvInfo.getFvName(), fvInfo.getSize(), fvInfo.getEfiFileName()};
            getFvInFdfTableModel().addRow(row);
        }
        

        for (int k = 0; k < vFvInfo.size(); ++k) {
            FvInfoFromFdf fvInfo = vFvInfo.get(k);
            getJTabbedPane().addTab(fvInfo.getFvName(), null, getJPanelModOrder(), null);

        }
    }
    
    private void getOptionNameValue(Map<String, String> m){
        for (int i = 0; i < jTableFvImageOpts.getRowCount(); ++i) {
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
            jContentPane.add(getJPanelContentEast(), java.awt.BorderLayout.EAST);
            jContentPane.add(getJPanelContentSouth(), java.awt.BorderLayout.SOUTH);
            jContentPane.add(getJPanelContentWest(), java.awt.BorderLayout.WEST);
            jContentPane.add(getJPanelContentNorth(), java.awt.BorderLayout.NORTH);
            jContentPane.add(getJTabbedPane(), java.awt.BorderLayout.CENTER);
        }
        return jContentPane;
    }
    
    private void getFvInfoFromFdf(String fdfPath, Vector<FvInfoFromFdf> vFvInfo) {
        File fdf = new File(fdfPath);
        if (!fdf.exists()) {
            return;
        }
        int lines = 0;

        try {
            FileReader reader = new FileReader(fdf);
            BufferedReader in = new BufferedReader(reader);
            String str;

            while ((str = in.readLine()) != null) {
                ++lines;
                str = str.trim();
                //
                // skip empty line, comment (start with //) 
                //
                if (str.length() == 0 || str.startsWith("//")) {
                    continue;
                }
                //
                // dig into Region {} section, create FvInfoFromFdf object for it.
                //
                if (str.startsWith("Region") && str.endsWith("{")) {
                    FvInfoFromFdf fvInfo = new FvInfoFromFdf();
                    boolean nameFound = false;
                    boolean sizeFound = false;
                    while ((str = in.readLine()) != null) {
                        ++lines;
                        str = str.trim();
                        //
                        // skip empty line, comment (start with //) 
                        //
                        if (str.length() == 0 || str.startsWith("//")) {
                            continue;
                        }
                        
                        if (str.startsWith("Name")) {
                            int firstQuote = str.indexOf("\"");
                            int lastQuote = str.lastIndexOf("\"");
                            fvInfo.setFvName(str.substring(firstQuote + 1, lastQuote));
                            nameFound = true;
                        }
                        
                        if (str.startsWith("Size")) {
                            int equalIndex = str.indexOf("=");
                            int commaIndex = str.indexOf(",");
                            fvInfo.setSize(str.substring(equalIndex + 1, commaIndex).trim());
                            sizeFound = true;
                        }
                        
                        if (nameFound && sizeFound) {
                            break;
                        }
                    }
                    
                    vFvInfo.add(fvInfo);
                }
                //
                // dig into File {} section, supply file name information to existing FvInfoFromFdf object.
                //
                if (str.startsWith("File")) {
                    boolean fileNameFound = false;
                    boolean fvFound = false;
                    String fileName = "";
                    String fvName = "";
                    while ((str = in.readLine()) != null) {
                        ++lines;
                        str = str.trim();
                        //
                        // skip empty line, comment (start with //) 
                        //
                        if (str.length() == 0 || str.startsWith("//")) {
                            continue;
                        }
                        
                        if (str.startsWith("Name")) {
                            int firstQuote = str.indexOf("\"");
                            int lastQuote = str.lastIndexOf("\"");
                            fileName = str.substring(firstQuote + 1, lastQuote);
                            fileNameFound = true;
                        }
                        
                        if (str.startsWith("Region")) {
                            int firstQuote = str.indexOf("\"");
                            int lastQuote = str.lastIndexOf("\"");
                            fvName = str.substring(firstQuote + 1, lastQuote);
                            fvFound = true;
                        }
                        
                        if (fileNameFound && fvFound) {
                            break;
                        }
                    }
                    
                    for (int i = 0; i <vFvInfo.size(); ++i) {
                        if (vFvInfo.get(i).getFvName().equals(fvName)) {
                            vFvInfo.get(i).setEfiFileName(fileName);
                        }
                    }
                }
   
            }
        } catch (Exception e) {
           
        }

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

class FvInfoFromFdf {
    private String fvName;
    private String size;
    private String efiFileName;
    
    public FvInfoFromFdf () {
        fvName = "";
        size = "";
        efiFileName = "";
    }
    public FvInfoFromFdf (String f, String s, String e) {
        this();
        fvName = f;
        size = s;
        efiFileName = e;
    }
    public String getEfiFileName() {
        return efiFileName;
    }
    public void setEfiFileName(String efiFileName) {
        this.efiFileName = efiFileName;
    }
    public String getFvName() {
        return fvName;
    }
    public void setFvName(String fvName) {
        this.fvName = fvName;
    }
    public String getSize() {
        return size;
    }
    public void setSize(String size) {
        this.size = size;
    }
    
}
