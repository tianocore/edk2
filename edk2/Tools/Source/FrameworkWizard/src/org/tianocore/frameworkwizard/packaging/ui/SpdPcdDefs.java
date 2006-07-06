/** @file
  Java class SpdPcdDefs is GUI for create PCD definition elements of spd file.
 
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
package org.tianocore.frameworkwizard.packaging.ui;

import java.awt.Dimension;
import java.awt.event.ActionEvent;
import java.awt.event.ComponentEvent;

import javax.swing.DefaultCellEditor;
import javax.swing.JFrame;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JLabel;
import javax.swing.JTextField;
import javax.swing.JComboBox;
import javax.swing.JButton;
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

import org.tianocore.PackageSurfaceAreaDocument;

import org.tianocore.frameworkwizard.common.DataValidation;
import org.tianocore.frameworkwizard.common.Identifications.OpeningPackageType;
import org.tianocore.frameworkwizard.common.ui.IInternalFrame;
import org.tianocore.frameworkwizard.common.ui.StarLabel;
import javax.swing.JScrollPane;
import javax.swing.JTable;
import javax.swing.JCheckBox;
import org.tianocore.frameworkwizard.common.ui.iCheckBoxList.ICheckBoxList;
import org.tianocore.frameworkwizard.platform.ui.ListEditor;

import java.awt.Rectangle;
import java.util.Vector;

/**
 GUI for create PCD definition elements of spd file
  
 @since PackageEditor 1.0
**/
public class SpdPcdDefs extends IInternalFrame implements TableModelListener{

    /**
     * 
     */
    private static final long serialVersionUID = 1L;

    static JFrame frame;
    
    private JPanel jContentPane = null;  

    private JLabel jLabelItemType = null;

    private JLabel jLabelC_Name = null;

    private JTextField jTextFieldC_Name = null;

    private JLabel jLabelToken = null;

    private JTextField jTextFieldToken = null;

    private JLabel jLabelDataType = null;

    private JButton jButtonOk = null;

    private JButton jButtonCancel = null;

    private JComboBox jComboBoxDataType = null;

    private SpdFileContents sfc = null;
    
    private OpeningPackageType docConsole = null;

    private StarLabel jStarLabel2 = null;

    private StarLabel jStarLabel3 = null;

    private StarLabel jStarLabel4 = null;

    private StarLabel jStarLabel = null;

    private StarLabel jStarLabel1 = null;

    private JLabel jLabelTokenSpace = null;

    private JTextField jTextFieldTsGuid = null;

    private JLabel jLabelVarVal = null;

    private JTextField jTextField = null;

    private JLabel jLabelDefVal = null;

    private JTextField jTextFieldDefaultValue = null;

    private JButton jButtonAdd = null;
    
    private CheckboxTableModel model = null;

    private JButton jButtonRemove = null;

    private JButton jButtonClearAll = null;

    private JScrollPane jScrollPane = null;

    private JTable jTable = null;
    
    private JScrollPane topScrollPane = null;  //  @jve:decl-index=0:visual-constraint="390,10"
    
    private int selectedRow = -1;

    private StarLabel starLabel = null;

    private JCheckBox jCheckBox = null;

    private JCheckBox jCheckBox1 = null;

    private JCheckBox jCheckBox2 = null;

    private JCheckBox jCheckBox3 = null;

    private JCheckBox jCheckBox4 = null;

    private JScrollPane jScrollPane1 = null;

    private ICheckBoxList iCheckBoxList = null;

    private JScrollPane jScrollPane2 = null;

    private ICheckBoxList iCheckBoxList1 = null;

    private JLabel jLabel = null;

    private JLabel jLabel1 = null;

    /**
     This method initializes this
     
     **/
    private void initialize() {
        this.setTitle("PCD Definition");
        this.setDefaultCloseOperation(javax.swing.WindowConstants.DISPOSE_ON_CLOSE);

    }

    /**
     This method initializes jTextFieldC_Name	
     	
     @return javax.swing.JTextField	
     **/
    private JTextField getJTextFieldC_Name() {
        if (jTextFieldC_Name == null) {
            jTextFieldC_Name = new JTextField();
            jTextFieldC_Name.setBounds(new java.awt.Rectangle(156,9,317,20));
            jTextFieldC_Name.setPreferredSize(new java.awt.Dimension(315,20));
        }
        return jTextFieldC_Name;
    }

    /**
     This method initializes jTextFieldToken	
     	
     @return javax.swing.JTextField	
     **/
    private JTextField getJTextFieldToken() {
        if (jTextFieldToken == null) {
            jTextFieldToken = new JTextField();
            jTextFieldToken.setBounds(new java.awt.Rectangle(156,33,317,20));
            jTextFieldToken.setPreferredSize(new java.awt.Dimension(315,20));
        }
        return jTextFieldToken;
    }

    /**
     This method initializes jButtonOk	
     	
     @return javax.swing.JButton	
     **/
    private JButton getJButtonOk() {
        if (jButtonOk == null) {
            jButtonOk = new JButton();
            jButtonOk.setText("OK");
            jButtonOk.setBounds(new java.awt.Rectangle(279,276,90,20));
            jButtonOk.setVisible(false);
            jButtonOk.addActionListener(this);
        }
        return jButtonOk;
    }

    /**
     This method initializes jButtonCancel	
     	
     @return javax.swing.JButton	
     **/
    private JButton getJButtonCancel() {
        if (jButtonCancel == null) {
            jButtonCancel = new JButton();
            jButtonCancel.setText("Cancel");
            jButtonCancel.setBounds(new java.awt.Rectangle(389,276,82,20));
            jButtonCancel.setVisible(false);
            jButtonCancel.addActionListener(this);
        }
        return jButtonCancel;
    }

    /**
     This method initializes jComboBoxDataType	
     	
     @return javax.swing.JComboBox	
     **/
    private JComboBox getJComboBoxDataType() {
        if (jComboBoxDataType == null) {
            jComboBoxDataType = new JComboBox();
            jComboBoxDataType.setBounds(new java.awt.Rectangle(156,84,110,20));
        }
        return jComboBoxDataType;
    }

    /**
     This is the default constructor
     **/
    public SpdPcdDefs() {
        super();
        init();
        initialize();
        
    }

    public SpdPcdDefs(PackageSurfaceAreaDocument.PackageSurfaceArea inPsa) {
        this();
        sfc = new SpdFileContents(inPsa);
        init(sfc);
    }
    
    public SpdPcdDefs(OpeningPackageType opt) {
        this(opt.getXmlSpd());
        docConsole = opt;
    }
    /**
     This method initializes this
     
     @return void
     **/
    private void init() {
        this.setSize(500, 650);
        this.setContentPane(getJContentPane());
        this.addInternalFrameListener(new InternalFrameAdapter(){
            public void internalFrameDeactivated(InternalFrameEvent e){
                if (jTable.isEditing()) {
                    jTable.getCellEditor().stopCellEditing();
                }
            }
        });
        initFrame();
        this.setVisible(true);
    }

    private void init(SpdFileContents sfc){
        if (sfc.getSpdPcdDefinitionCount() == 0) {
            return ;
        }
        String[][] saa = new String[sfc.getSpdPcdDefinitionCount()][8];
        String[][] usage = new String[sfc.getSpdPcdDefinitionCount()][5];
        Object[] rowData = new Object[13];
        sfc.getSpdPcdDefinitions(saa, usage);
        int i = 0;
        while (i < saa.length) {
           
            for (int k = 0; k < 6; ++k) {
                rowData[k] = saa[i][k];
            }
            for (int m = 7; m < 11; ++m) {
                rowData[m] = new Boolean("false");
            }
            int j = 0;
            while (j < 5) {
                if (usage[i][j] == null || usage[i][j].length()==0){
                    ++j;
                    continue;
                }
                if (usage[i][j].equals("FEATURE_FLAG")){
                    rowData[6] = new Boolean("true");
                }
                if (usage[i][j].equals("FIXED_AT_BUILD")){
                    rowData[7] = new Boolean("true");
                }
                if (usage[i][j].equals("PATCHABLE_IN_MODULE")){
                    rowData[8] = new Boolean("true");
                }
                if (usage[i][j].equals("DYNAMIC")){
                    rowData[9] = new Boolean("true");
                }
                if (usage[i][j].equals("DYNAMIC_EX")){
                    rowData[10] = new Boolean("true");
                }
                
                ++j;
            }
            
            rowData[11] = saa[i][6];
            rowData[12] = saa[i][7];
            
            model.addRow(rowData);
            
            i++;
        }
        
        
        
    }
    
    private JScrollPane getJContentPane(){
        if (topScrollPane == null){
            topScrollPane = new JScrollPane();
            topScrollPane.setSize(new java.awt.Dimension(482,451));
            topScrollPane.setViewportView(getJContentPane1());
        }
        return topScrollPane;
    }
    private JPanel getJContentPane1() {
  		if (jContentPane == null) {	
           
            jLabel1 = new JLabel();
            jLabel1.setBounds(new java.awt.Rectangle(241,192,89,16));
            jLabel1.setText("Supported Arch");
            jLabel1.setEnabled(true);
            jLabel = new JLabel();
            jLabel.setBounds(new java.awt.Rectangle(15,193,103,16));
            jLabel.setText("Supported Module");
            jLabel.setEnabled(true);
            starLabel = new StarLabel();
            starLabel.setBounds(new java.awt.Rectangle(2,134,10,20));
            jLabelDefVal = new JLabel();
            jLabelDefVal.setBounds(new java.awt.Rectangle(277,84,80,20));
            jLabelDefVal.setText("Default Value");
            jLabelVarVal = new JLabel();
            jLabelVarVal.setBounds(new java.awt.Rectangle(11,133,84,20));
            jLabelVarVal.setText("Valid Usage");
            jLabelC_Name = new JLabel();
            jLabelC_Name.setText("C_Name");
            jLabelC_Name.setBounds(new java.awt.Rectangle(11,9,140,20));
            jLabelTokenSpace = new JLabel();
            jLabelTokenSpace.setBounds(new java.awt.Rectangle(11,58,140,20));
            jLabelTokenSpace.setText("Token Space");
            jLabelDataType = new JLabel();
            jLabelDataType.setText("Data Type");
            jLabelDataType.setBounds(new java.awt.Rectangle(11,83,140,20));
            jLabelToken = new JLabel();
            jLabelToken.setText("Token");
            jLabelToken.setBounds(new java.awt.Rectangle(11,33,140,20));
            jLabelItemType = new JLabel();
            jLabelItemType.setText("Help Text");
            jLabelItemType.setBounds(new java.awt.Rectangle(11,108,140,20));
            
            
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.setPreferredSize(new java.awt.Dimension(479,375));
            
            jContentPane.add(jLabelItemType, null);
            jContentPane.add(jLabelC_Name, null);
            jContentPane.add(jLabelTokenSpace, null);
            jContentPane.add(getJTextFieldTsGuid(), null);
            jContentPane.add(jLabelVarVal, null);
            jContentPane.add(getJTextFieldC_Name(), null);
            jContentPane.add(jLabelToken, null);
            jContentPane.add(getJTextFieldToken(), null);
            jContentPane.add(jLabelDataType, null);
            jContentPane.add(getJButtonOk(), null);
            jContentPane.add(getJButtonCancel(), null);
            jContentPane.add(getJComboBoxDataType(), null);
            
            jStarLabel = new StarLabel();
            jStarLabel1 = new StarLabel();
            jStarLabel1.setBounds(new java.awt.Rectangle(2,8,10,20));
            jStarLabel2 = new StarLabel();
            jStarLabel3 = new StarLabel();
            jStarLabel4 = new StarLabel();
            jStarLabel.setLocation(new java.awt.Point(2,84));
            jStarLabel4.setLocation(new java.awt.Point(2, 109));
            jStarLabel2.setLocation(new java.awt.Point(2,33));
            jStarLabel3.setLocation(new java.awt.Point(2, 58));
            jStarLabel3.setSize(new java.awt.Dimension(8,20));
            jContentPane.add(jStarLabel2, null);
            jContentPane.add(jStarLabel3, null);
            jContentPane.add(jStarLabel, null);
            jContentPane.add(jStarLabel1, null);
            jContentPane.add(jStarLabel4, null);
            jContentPane.add(getJTextField(), null);
            jContentPane.add(jLabelDefVal, null);
            jContentPane.add(getJTextFieldDefaultValue(), null);
            jContentPane.add(getJButtonAdd(), null);
            jContentPane.add(getJButtonRemove(), null);
            jContentPane.add(getJButtonClearAll(), null);
            jContentPane.add(getJScrollPane(), null);
            jContentPane.add(starLabel, null);
            jContentPane.add(getJCheckBox(), null);
            jContentPane.add(getJCheckBox1(), null);
            jContentPane.add(getJCheckBox2(), null);
            jContentPane.add(getJCheckBox3(), null);
            jContentPane.add(getJCheckBox4(), null);
            jContentPane.add(getJScrollPane1(), null);
            jContentPane.add(getJScrollPane2(), null);
            jContentPane.add(jLabel, null);
            jContentPane.add(jLabel1, null);
        }
        return jContentPane;
    }

    /**
     This method initializes comboboxes			
 			jContentPane.add(jLabelTokenSpace, null);
    
     **/
    private void initFrame() {

        jComboBoxDataType.addItem("UINT8");
        jComboBoxDataType.addItem("UINT16");
        jComboBoxDataType.addItem("UINT32");
        jComboBoxDataType.addItem("UINT64");
        jComboBoxDataType.addItem("VOID*");
        jComboBoxDataType.addItem("BOOLEAN");
        jComboBoxDataType.setSelectedIndex(0);
    }

    public void actionPerformed(ActionEvent arg0) {
        
        docConsole.setSaved(false);
            if (arg0.getSource() == jButtonOk) {
                this.save();
                this.dispose();
            }
            if (arg0.getSource() == jButtonCancel) {
                this.dispose();
            }

            if (arg0.getSource() == jButtonAdd) {
                //ToDo: check before add
                boolean[] b = {jCheckBox.isSelected(), jCheckBox1.isSelected(), jCheckBox2.isSelected(), jCheckBox3.isSelected(), jCheckBox4.isSelected()};
                if (!checkValidUsage(b)) {
                    return;
                }
                String archList = vectorToString(iCheckBoxList.getAllCheckedItemsString());
                if (archList.length() == 0) {
                    archList = null;
                }
                String modTypeList = vectorToString(iCheckBoxList1.getAllCheckedItemsString());
                if (modTypeList.length() == 0) {
                    modTypeList = null;
                }
                Object[] row = {jTextFieldC_Name.getText(), jTextFieldToken.getText(),
                                jTextFieldTsGuid.getText(), jComboBoxDataType.getSelectedItem(), 
                                jTextFieldDefaultValue.getText(), jTextField.getText(),
                                jCheckBox.isSelected(), jCheckBox1.isSelected(),
                                jCheckBox2.isSelected(), jCheckBox3.isSelected(), jCheckBox4.isSelected(),
                                archList, modTypeList};
                if (!dataValidation(row)) {
                    return;
                }
                model.addRow(row);
                
                String usage = getValidUsage(jCheckBox.isSelected(), jCheckBox1.isSelected(), jCheckBox2.isSelected(), jCheckBox3.isSelected(), jCheckBox4.isSelected());
                if (usage.length() == 0) {
                    usage = null;
                }
                sfc.genSpdPcdDefinitions(row[0]+"", row[1]+"", row[3]+"", usage, row[2]+"", row[4]+"", row[5]+"", archList, modTypeList);
            }
            //
            // remove selected line
            //
            if (arg0.getSource() == jButtonRemove) {
                if (jTable.isEditing()){
                    jTable.getCellEditor().stopCellEditing();
                }
                int rowSelected = selectedRow;
                if (rowSelected >= 0) {
                    model.removeRow(rowSelected);
                    sfc.removeSpdPcdDefinition(rowSelected);
                }
            }

            if (arg0.getSource() == jButtonClearAll) {
                if (model.getRowCount() == 0) {
                    return;
                }
                model.setRowCount(0);
                sfc.removeSpdPcdDefinition();
            }
            
    }

    protected void save() {
        
    }

    /**
     * This method initializes jTextFieldTsGuid	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldTsGuid() {
        if (jTextFieldTsGuid == null) {
            jTextFieldTsGuid = new JTextField();
            jTextFieldTsGuid.setPreferredSize(new java.awt.Dimension(315,20));
            jTextFieldTsGuid.setSize(new java.awt.Dimension(317,20));
            jTextFieldTsGuid.setLocation(new java.awt.Point(156,58));
        }
        return jTextFieldTsGuid;
    }

    /**
     * This method initializes jTextField	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextField() {
        if (jTextField == null) {
            jTextField = new JTextField();
            jTextField.setBounds(new java.awt.Rectangle(156,108,317,20));
            jTextField.setPreferredSize(new java.awt.Dimension(315,20));
        }
        return jTextField;
    }

    /**
     * This method initializes jTextFieldDefaultValue	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldDefaultValue() {
        if (jTextFieldDefaultValue == null) {
            jTextFieldDefaultValue = new JTextField();
            jTextFieldDefaultValue.setBounds(new java.awt.Rectangle(368,84,105,20));
            jTextFieldDefaultValue.setPreferredSize(new java.awt.Dimension(104,20));
        }
        return jTextFieldDefaultValue;
    }

    /**
     * This method initializes jButtonAdd	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonAdd() {
        if (jButtonAdd == null) {
            jButtonAdd = new JButton();
            jButtonAdd.setBounds(new java.awt.Rectangle(195,277,71,20));
            jButtonAdd.setPreferredSize(new java.awt.Dimension(70,20));
            jButtonAdd.setText("Add");
            jButtonAdd.addActionListener(this);
        }
        return jButtonAdd;   
        
    }   
    
    /**
     * This method initializes jButtonRemove	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonRemove() {
        if (jButtonRemove == null) {
            jButtonRemove = new JButton();
            jButtonRemove.setBounds(new java.awt.Rectangle(278,277,81,20));
            jButtonRemove.setPreferredSize(new java.awt.Dimension(70,20));
            jButtonRemove.setText("Delete");
            jButtonRemove.addActionListener(this);
        }
        return jButtonRemove;
    }

    /**
     * This method initializes jButtonClearAll	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonClearAll() {
        if (jButtonClearAll == null) {
            jButtonClearAll = new JButton();
            jButtonClearAll.setBounds(new java.awt.Rectangle(382,277,90,20));
            jButtonClearAll.setPreferredSize(new java.awt.Dimension(81,20));
            jButtonClearAll.setText("Clear All");
            jButtonClearAll.addActionListener(this);
        }
        return jButtonClearAll;
    }

    public void componentResized(ComponentEvent arg0) {
        int intPreferredWidth = 500;
        
        resizeComponentWidth(this.jTextFieldC_Name, this.getWidth(), intPreferredWidth);
        resizeComponentWidth(this.jTextFieldToken, this.getWidth(), intPreferredWidth);
        resizeComponentWidth(this.jTextFieldTsGuid, this.getWidth(), intPreferredWidth);
        resizeComponentWidth(this.jTextFieldDefaultValue, this.getWidth(), intPreferredWidth);
        resizeComponentWidth(this.jTextField, this.getWidth(), intPreferredWidth);
        resizeComponentWidth(this.jScrollPane, this.getWidth(), intPreferredWidth);
        
        resizeComponentWidth(this.jTextFieldDefaultValue, this.getWidth(), intPreferredWidth);
//        relocateComponentX(this.jButtonClearAll, this.getWidth(), DataType.SPACE_TO_RIGHT_FOR_GENERATE_BUTTON);
//        relocateComponentX(this.jButtonRemove, this.getWidth(), DataType.SPACE_TO_RIGHT_FOR_GENERATE_BUTTON);
//        relocateComponentX(this.jButtonAdd, this.getWidth(), DataType.SPACE_TO_RIGHT_FOR_GENERATE_BUTTON);
    }

    /**
     * This method initializes jScrollPane	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPane() {
        if (jScrollPane == null) {
            jScrollPane = new JScrollPane();
            jScrollPane.setBounds(new java.awt.Rectangle(5,301,1473,137));
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
            model = new CheckboxTableModel();
            jTable = new JTable(model);
            jTable.setRowHeight(20);
            jTable.setAutoResizeMode(JTable.AUTO_RESIZE_OFF);
            jTable.setSize(new Dimension(1000, 300));
            
            model.addColumn("C_Name");
            model.addColumn("Token");
            model.addColumn("TokenSpace");
            model.addColumn("DatumType");
            model.addColumn("DefaultValue");
            model.addColumn("HelpText");
            
            model.addColumn("FeatureFlag");
            model.addColumn("FixedAtBuild");
            model.addColumn("PatchableInModule");
            model.addColumn("Dynamic");
            model.addColumn("DynamicEx");
            model.addColumn("SupportedArch");
            model.addColumn("SupportedModule");
            
            //ToDo: add a valid usage editor
            
            JComboBox jComboBoxDataType = new JComboBox();
            jComboBoxDataType.addItem("UINT8");
            jComboBoxDataType.addItem("UINT16");
            jComboBoxDataType.addItem("UINT32");
            jComboBoxDataType.addItem("UINT64");
            jComboBoxDataType.addItem("VOID*");
            jComboBoxDataType.addItem("BOOLEAN");
            TableColumn dataTypeColumn = jTable.getColumnModel().getColumn(3);
            dataTypeColumn.setCellEditor(new DefaultCellEditor(jComboBoxDataType));

            Vector<String> vArch = new Vector<String>();
            vArch.add("IA32");
            vArch.add("X64");
            vArch.add("IPF");
            vArch.add("EBC");
            vArch.add("ARM");
            vArch.add("PPC");
            jTable.getColumnModel().getColumn(11).setCellEditor(new ListEditor(vArch));
            
            Vector<String> vModule = new Vector<String>();
            vModule.add("BASE");
            vModule.add("SEC");
            vModule.add("PEI_CORE");
            vModule.add("PEIM");
            vModule.add("DXE_CORE");
            vModule.add("DXE_DRIVER");
            vModule.add("DXE_RUNTIME_DRIVER");
            vModule.add("DXE_SAL_DRIVER");
            vModule.add("DXE_SMM_DRIVER");
            vModule.add("UEFI_DRIVER");
            vModule.add("UEFI_APPLICATION");
            vModule.add("USER_DEFINED");
            jTable.getColumnModel().getColumn(12).setCellEditor(new ListEditor(vModule));
            
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
                        selectedRow = lsm.getMinSelectionIndex();
                    }
                }
            });
            
            jTable.getModel().addTableModelListener(this);
        }
        return jTable;
    }
    
    public void tableChanged(TableModelEvent arg0) {
        // TODO Auto-generated method stub
        int row = arg0.getFirstRow();
        TableModel m = (TableModel)arg0.getSource();
        if (arg0.getType() == TableModelEvent.UPDATE){
            
            String cName = m.getValueAt(row, 0) + "";
            String token = m.getValueAt(row, 1) + "";
            String ts = m.getValueAt(row, 2) + "";
            String dataType = m.getValueAt(row, 3) + "";
            String defaultVal = m.getValueAt(row, 4) + "";
            String help = m.getValueAt(row, 5) + "";
            String usage = getValidUsage(new Boolean(m.getValueAt(row, 6)+""), new Boolean(m.getValueAt(row, 7)+""), new Boolean(m.getValueAt(row, 8)+""), new Boolean(m.getValueAt(row, 9)+""), new Boolean(m.getValueAt(row, 10)+""));
            String archList = vectorToString(iCheckBoxList.getAllCheckedItemsString());
            String modTypeList = vectorToString(iCheckBoxList1.getAllCheckedItemsString());
            if (usage.length() == 0) {
                JOptionPane.showMessageDialog(frame, "You must choose at least one usage for PCD entry.");
                return;
            }
            Object[] o = {cName, token, ts, dataType, defaultVal, help};
            if (!dataValidation(o)){
                return;
            }
            docConsole.setSaved(false);
            sfc.updateSpdPcdDefinition(row, cName, token, dataType, usage, ts, defaultVal, help, archList, modTypeList);
        }
    }

    /**
     * This method initializes jCheckBox	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBox() {
        if (jCheckBox == null) {
            jCheckBox = new JCheckBox();
            jCheckBox.setBounds(new java.awt.Rectangle(156,161,100,21));
            jCheckBox.setText("Feature Flag");
            jCheckBox.setPreferredSize(new java.awt.Dimension(21,20));
        }
        return jCheckBox;
    }

    /**
     * This method initializes jCheckBox1	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBox1() {
        if (jCheckBox1 == null) {
            jCheckBox1 = new JCheckBox();
            jCheckBox1.setBounds(new java.awt.Rectangle(302,133,108,20));
            jCheckBox1.setText("Fixed at Build");
            jCheckBox1.setPreferredSize(new java.awt.Dimension(21,20));
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
            jCheckBox2.setBounds(new java.awt.Rectangle(156,133,154,20));
            jCheckBox2.setText("Patchable in Module");
            jCheckBox2.setPreferredSize(new java.awt.Dimension(21,20));
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
            jCheckBox3.setBounds(new java.awt.Rectangle(278,161,80,20));
            jCheckBox3.setText("Dynamic");
            jCheckBox3.setPreferredSize(new java.awt.Dimension(21,20));
        }
        return jCheckBox3;
    }

    /**
     * This method initializes jCheckBox4	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBox4() {
        if (jCheckBox4 == null) {
            jCheckBox4 = new JCheckBox();
            jCheckBox4.setBounds(new java.awt.Rectangle(371,161,99,20));
            jCheckBox4.setText("DynamicEx");
            jCheckBox4.setPreferredSize(new java.awt.Dimension(21,20));
        }
        return jCheckBox4;
    }
    
    private String getValidUsage(boolean ff, boolean fab, boolean pim, boolean d, boolean de) {
        String usage = "";
        if (ff){
            usage += "FEATURE_FLAG ";
        }
        if (fab){
            usage += "FIXED_AT_BUILD ";
        }
        if (pim){
            usage += "PATCHABLE_IN_MODULE ";
        }
        if (d){
            usage += "DYNAMIC ";
        }
        if (de){
            usage += "DYNAMIC_EX ";
        }
        
        return usage.trim();
    }
    
    private boolean checkValidUsage(boolean[] b) {
        if (!(b[0] || b[1] || b[2] || b[3] || b[4])){
            JOptionPane.showMessageDialog(frame, "You must specify at least one usage.");
            return false;
        }
        return true;
    }
    private boolean dataValidation(Object[] row) {
        
        if (!DataValidation.isC_NameType(row[0].toString())) {
            JOptionPane.showMessageDialog(frame, "C_Name is NOT C_NameType.");
            return false;
        }
        if (!(DataValidation.isHexDoubleWordDataType(row[1].toString()) || 
                        DataValidation.isInt(row[1].toString(), 0, 0xffffffff))) {
            JOptionPane.showMessageDialog(frame, "Token is NOT correct.");
            return false;
        }
        if (!DataValidation.isC_NameType(row[2].toString())) {
            JOptionPane.showMessageDialog(frame, "Token Space is NOT C_NameType");
            return false;
        }
        if (row[5].toString().length() == 0) {
            JOptionPane.showMessageDialog(frame, "HelpText could NOT be empty.");
            return false;
        }
        return true;
    }

    /**
     * This method initializes jScrollPane1	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPane1() {
        if (jScrollPane1 == null) {
            jScrollPane1 = new JScrollPane();
            jScrollPane1.setBounds(new java.awt.Rectangle(242,213,188,54));
            jScrollPane1.setViewportView(getICheckBoxList());
            jScrollPane1.setPreferredSize(new Dimension(188, 74));
        }
        return jScrollPane1;
    }

    /**
     * This method initializes iCheckBoxList	
     * 	
     * @return org.tianocore.frameworkwizard.common.ui.iCheckBoxList.ICheckBoxList	
     */
    private ICheckBoxList getICheckBoxList() {
        if (iCheckBoxList == null) {
            iCheckBoxList = new ICheckBoxList();
            iCheckBoxList.setBounds(new Rectangle(197, 142, 188, 74));
            Vector<String> v = new Vector<String>();
            v.add("IA32");
            v.add("X64");
            v.add("IPF");
            v.add("EBC");
            v.add("ARM");
            v.add("PPC");
            iCheckBoxList.setAllItems(v);
        }
        return iCheckBoxList;
    }

    /**
     * This method initializes jScrollPane2	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPane2() {
        if (jScrollPane2 == null) {
            jScrollPane2 = new JScrollPane();
            jScrollPane2.setBounds(new java.awt.Rectangle(15,213,199,55));
            jScrollPane2.setViewportView(getICheckBoxList1());
            jScrollPane2.setPreferredSize(new Dimension(170, 74));
        }
        return jScrollPane2;
    }

    /**
     * This method initializes iCheckBoxList1	
     * 	
     * @return org.tianocore.frameworkwizard.common.ui.iCheckBoxList.ICheckBoxList	
     */
    private ICheckBoxList getICheckBoxList1() {
        if (iCheckBoxList1 == null) {
            iCheckBoxList1 = new ICheckBoxList();
            iCheckBoxList1.setBounds(new Rectangle(14, 142, 170, 74));
            Vector<String> v = new Vector<String>();
            v.add("BASE");
            v.add("SEC");
            v.add("PEI_CORE");
            v.add("PEIM");
            v.add("DXE_CORE");
            v.add("DXE_DRIVER");
            v.add("DXE_RUNTIME_DRIVER");
            v.add("DXE_SAL_DRIVER");
            v.add("DXE_SMM_DRIVER");
            v.add("UEFI_DRIVER");
            v.add("UEFI_APPLICATION");
            v.add("USER_DEFINED");
            iCheckBoxList1.setAllItems(v);
        }
        return iCheckBoxList1;
    }
    
    protected String vectorToString(Vector<String> v) {
        if (v == null) {
            return null;
        }
        String s = " ";
        for (int i = 0; i < v.size(); ++i) {
            s += v.get(i);
            s += " ";
        }
        return s.trim();
    }
} //  @jve:decl-index=0:visual-constraint="22,11"

class CheckboxTableModel extends DefaultTableModel {
    /**
     * 
     */
    private static final long serialVersionUID = 1L;

    public Class<?> getColumnClass (int c) {
        if (getValueAt(0, c) != null){
            return getValueAt(0, c).getClass();
        }
        return String.class;
    }
}
