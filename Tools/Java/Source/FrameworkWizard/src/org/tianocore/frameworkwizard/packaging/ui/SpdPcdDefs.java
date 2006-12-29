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
import java.awt.FontMetrics;
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
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.Identifications.OpeningPackageType;
import org.tianocore.frameworkwizard.common.ui.IInternalFrame;
import org.tianocore.frameworkwizard.common.ui.StarLabel;
import javax.swing.JScrollPane;
import javax.swing.JTable;
import javax.swing.JCheckBox;
import org.tianocore.frameworkwizard.common.ui.iCheckBoxList.ICheckBoxList;
import org.tianocore.frameworkwizard.platform.ui.ListEditor;
import org.tianocore.frameworkwizard.platform.ui.LongTextEditor;

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

    private JFrame topFrame;
    
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

    private JComboBox jComboBoxTsGuid = null;

    private JLabel jLabelVarVal = null;

    private JTextField jTextFieldHelp = null;

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

    private JCheckBox jCheckBoxFeatureFlag = null;

    private JCheckBox jCheckBoxFixedAtBuild = null;

    private JCheckBox jCheckBoxPatchInMod = null;

    private JCheckBox jCheckBoxDyn = null;

    private JCheckBox jCheckBoxDynEx = null;

    private JScrollPane jScrollPaneArch = null;

    private ICheckBoxList iCheckBoxListArch = null;

    private JScrollPane jScrollPaneMod = null;

    private ICheckBoxList iCheckBoxListMod = null;

    private JLabel jLabelSupMod = null;

    private JLabel jLabelSupArch = null;
    
    private final int pcdCNameMinWidth = 200;
    private final int pcdTokenMinWidth = 100;
    private final int pcdTokenSpaceMinWidth = 200;
    private final int datumTypeMinWidth = 80;
    private final int defaultValueMinWidth = 100;
    private final int helpTextMinWidth = 200;
    private final int usageMinWidth = 60;
    private final int supArchMinWidth = 200;
    private final int supModMinWidth = 200;
    
//    private Object boolModifyLock = new Object();
//    private boolean exclusiveUsage = false;

    /**
     This method initializes this
     
     **/
    private void initialize() {
        this.setTitle("PCD Declarations");
        this.setDefaultCloseOperation(javax.swing.WindowConstants.DISPOSE_ON_CLOSE);

        int xPos = jCheckBoxPatchInMod.getX() + jCheckBoxPatchInMod.getWidth();
        jCheckBoxFixedAtBuild.setLocation(xPos,133);
        
        xPos = jCheckBoxFeatureFlag.getX() + jCheckBoxFeatureFlag.getWidth();
        jCheckBoxDyn.setLocation(xPos,161);
        
        xPos = jCheckBoxDyn.getX() + jCheckBoxDyn.getWidth();
        jCheckBoxDynEx.setLocation(xPos,161);
        
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
    public SpdPcdDefs(JFrame frame) {
        super();
        init();
        initialize();
        topFrame = frame;
    }

    public SpdPcdDefs(PackageSurfaceAreaDocument.PackageSurfaceArea inPsa, JFrame frame) {
        this(frame);
        sfc = new SpdFileContents(inPsa);
        init(sfc);
    }
    
    public SpdPcdDefs(OpeningPackageType opt, JFrame frame) {
        this(opt.getXmlSpd(), frame);
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
        
        this.setVisible(true);
    }

    private void init(SpdFileContents sfc){
        
        if (sfc.getSpdPkgDefsRdOnly().equals("true")) {
            JOptionPane.showMessageDialog(topFrame, "This is a read-only package. You will not be able to edit contents in table.");
        }
        initFrame();
        
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
            for (int m = 6; m < 11; ++m) {
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
           
            jLabelSupArch = new JLabel();
            jLabelSupArch.setBounds(new java.awt.Rectangle(241,192,89,16));
            jLabelSupArch.setText("Supported Architectures");
            jLabelSupArch.setEnabled(true);
            FontMetrics fm = jLabelSupArch.getFontMetrics(jLabelSupArch.getFont());
            jLabelSupArch.setSize(fm.stringWidth(jLabelSupArch.getText()) + 10, 20);
            jLabelSupMod = new JLabel();
            jLabelSupMod.setBounds(new java.awt.Rectangle(15,193,103,16));
            jLabelSupMod.setText("Supported Module types");
            jLabelSupMod.setEnabled(true);
            fm = jLabelSupMod.getFontMetrics(jLabelSupMod.getFont());
            jLabelSupMod.setSize(fm.stringWidth(jLabelSupMod.getText()) + 10, 20);
            starLabel = new StarLabel();
            starLabel.setBounds(new java.awt.Rectangle(2,134,10,20));
            jLabelDefVal = new JLabel();
            jLabelDefVal.setBounds(new java.awt.Rectangle(277,84,80,20));
            jLabelDefVal.setText("Default Value");
            fm = jLabelDefVal.getFontMetrics(jLabelDefVal.getFont());
            jLabelDefVal.setSize(fm.stringWidth(jLabelDefVal.getText()) + 10, 20);
            jLabelVarVal = new JLabel();
            jLabelVarVal.setBounds(new java.awt.Rectangle(11,133,100,20));
            jLabelVarVal.setText("Valid Usage");
            jLabelC_Name = new JLabel();
            jLabelC_Name.setText("C Name");
            jLabelC_Name.setBounds(new java.awt.Rectangle(11,9,140,20));
            jLabelTokenSpace = new JLabel();
            jLabelTokenSpace.setBounds(new java.awt.Rectangle(11,58,140,20));
            jLabelTokenSpace.setText("Token Space");
            jLabelDataType = new JLabel();
            jLabelDataType.setText("Data Type");
            jLabelDataType.setBounds(new java.awt.Rectangle(11,83,140,20));
            jLabelToken = new JLabel();
            jLabelToken.setText("Token Number");
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
            jContentPane.add(getJComboBoxTsGuid(), null);
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
            jContentPane.add(getJTextFieldHelp(), null);
            jContentPane.add(jLabelDefVal, null);
            jContentPane.add(getJTextFieldDefaultValue(), null);
            jContentPane.add(getJButtonAdd(), null);
            jContentPane.add(getJButtonRemove(), null);
            jContentPane.add(getJButtonClearAll(), null);
            jContentPane.add(getJScrollPane(), null);
            jContentPane.add(starLabel, null);
            jContentPane.add(getJCheckBoxFeatureFlag(), null);
            jContentPane.add(getJCheckBoxFixedAtBuild(), null);
            jContentPane.add(getJCheckBoxPatchInMod(), null);
            jContentPane.add(getJCheckBoxDyn(), null);
            jContentPane.add(getJCheckBoxDynEx(), null);
            jContentPane.add(getJScrollPaneArch(), null);
            jContentPane.add(getJScrollPaneMod(), null);
            jContentPane.add(jLabelSupMod, null);
            jContentPane.add(jLabelSupArch, null);
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
        
        Vector<String> vGuidCName = new Vector<String>();
        sfc.getSpdGuidDeclWithType(vGuidCName, "TOKEN_SPACE_GUID");
        for (int i = 0; i < vGuidCName.size(); ++i) {
            jComboBoxTsGuid.addItem(vGuidCName.get(i));
        }
        
        boolean editable = true;
        if (sfc.getSpdPkgDefsRdOnly().equals("true")) {
            editable = false;
        }
        
        jButtonAdd.setEnabled(editable);
        jButtonRemove.setEnabled(editable);
        jButtonClearAll.setEnabled(editable);
        jTable.setEnabled(editable);
    }

    public void actionPerformed(ActionEvent arg0) {
        
        
            if (arg0.getSource() == jButtonOk) {
                this.save();
                this.dispose();
            }
            if (arg0.getSource() == jButtonCancel) {
                this.dispose();
            }

            if (arg0.getSource() == jButtonAdd) {
                //ToDo: check before add
          
                boolean[] b = {jCheckBoxFeatureFlag.isSelected(), jCheckBoxFixedAtBuild.isSelected(), jCheckBoxPatchInMod.isSelected(), jCheckBoxDyn.isSelected(), jCheckBoxDynEx.isSelected()};
                if (!checkValidUsage(b)) {
                    return;
                }
                String archList = vectorToString(iCheckBoxListArch.getAllCheckedItemsString());
                if (archList.length() == 0) {
                    archList = null;
                }
                String modTypeList = vectorToString(iCheckBoxListMod.getAllCheckedItemsString());
                if (modTypeList.length() == 0) {
                    modTypeList = null;
                }
                Object[] row = {jTextFieldC_Name.getText(), jTextFieldToken.getText(),
                                jComboBoxTsGuid.getSelectedItem(), jComboBoxDataType.getSelectedItem(), 
                                jTextFieldDefaultValue.getText(), jTextFieldHelp.getText(),
                                jCheckBoxFeatureFlag.isSelected(), jCheckBoxFixedAtBuild.isSelected(),
                                jCheckBoxPatchInMod.isSelected(), jCheckBoxDyn.isSelected(), jCheckBoxDynEx.isSelected(),
                                archList, modTypeList};
                try {
                if (!dataValidation(row)) {
                    return;
                }
                
                if (tokenCNameExisted(jTextFieldToken.getText(), jTextFieldC_Name.getText())) {
                    return;
                }
			    } catch (Exception e) {
				    JOptionPane.showMessageDialog(this, "Illegal Token:"+ e.getCause());
				    return;
			    }
                
                model.addRow(row);
                jTable.changeSelection(model.getRowCount()-1, 0, false, false);
                String usage = getValidUsage(jCheckBoxFeatureFlag.isSelected(), jCheckBoxFixedAtBuild.isSelected(), jCheckBoxPatchInMod.isSelected(), jCheckBoxDyn.isSelected(), jCheckBoxDynEx.isSelected());
                if (usage.length() == 0) {
                    usage = null;
                }
                sfc.genSpdPcdDefinitions(row[0]+"", row[1]+"", row[3]+"", usage, row[2]+"", row[4]+"", row[5]+"", archList, modTypeList);
                docConsole.setSaved(false);
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
                    docConsole.setSaved(false);
                }
            }

            if (arg0.getSource() == jButtonClearAll) {
                if (model.getRowCount() == 0) {
                    return;
                }
                model.setRowCount(0);
                sfc.removeSpdPcdDefinition();
                docConsole.setSaved(false);
            }
            
    }

    protected void save() {
        
    }

    /**
     * This method initializes jComboBoxTsGuid	
     * 	
     * @return javax.swing.JComboBox	
     */
    private JComboBox getJComboBoxTsGuid() {
        if (jComboBoxTsGuid == null) {
            jComboBoxTsGuid = new JComboBox();
            jComboBoxTsGuid.setBounds(new java.awt.Rectangle(156,58,315,20));
    
        }
        return jComboBoxTsGuid;
    }

    /**
     * This method initializes jTextField	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldHelp() {
        if (jTextFieldHelp == null) {
            jTextFieldHelp = new JTextField();
            jTextFieldHelp.setBounds(new java.awt.Rectangle(156,108,317,20));
            jTextFieldHelp.setPreferredSize(new java.awt.Dimension(315,20));
        }
        return jTextFieldHelp;
    }

    /**
     * This method initializes jTextFieldDefaultValue	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldDefaultValue() {
        if (jTextFieldDefaultValue == null) {
            jTextFieldDefaultValue = new JTextField();
            int xPos = jLabelDefVal.getX() + jLabelDefVal.getWidth();
            jTextFieldDefaultValue.setBounds(new java.awt.Rectangle(xPos,84,105,20));
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
        
        Tools.resizeComponentWidth(this.jTextFieldC_Name, this.getWidth(), intPreferredWidth);
        Tools.resizeComponentWidth(this.jTextFieldToken, this.getWidth(), intPreferredWidth);
        Tools.resizeComponentWidth(this.jComboBoxTsGuid, this.getWidth(), intPreferredWidth);
        Tools.resizeComponentWidth(this.jTextFieldDefaultValue, this.getWidth(), intPreferredWidth);
        Tools.resizeComponentWidth(this.jTextFieldHelp, this.getWidth(), intPreferredWidth);
        Tools.resizeComponentWidth(this.jScrollPane, this.getWidth(), intPreferredWidth);
        
        Tools.resizeComponentWidth(this.jTextFieldDefaultValue, this.getWidth(), intPreferredWidth);
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
            jScrollPane.setBounds(new java.awt.Rectangle(5,301,1473,259));
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
            
            TableColumn column = jTable.getColumnModel().getColumn(0);
            column.setMinWidth(this.pcdCNameMinWidth);
            column = jTable.getColumnModel().getColumn(1);
            column.setMinWidth(this.pcdTokenMinWidth);
            column = jTable.getColumnModel().getColumn(2);
            column.setMinWidth(this.pcdTokenSpaceMinWidth);
            column = jTable.getColumnModel().getColumn(3);
            column.setMinWidth(this.datumTypeMinWidth);
            column = jTable.getColumnModel().getColumn(4);
            column.setMinWidth(this.defaultValueMinWidth);
            column = jTable.getColumnModel().getColumn(5);
            column.setMinWidth(this.helpTextMinWidth);
            column = jTable.getColumnModel().getColumn(6);
            column.setMinWidth(this.usageMinWidth);
            column = jTable.getColumnModel().getColumn(7);
            column.setMinWidth(this.usageMinWidth);
            column = jTable.getColumnModel().getColumn(8);
            column.setMinWidth(this.usageMinWidth);
            column = jTable.getColumnModel().getColumn(9);
            column.setMinWidth(this.usageMinWidth);
            column = jTable.getColumnModel().getColumn(10);
            column.setMinWidth(this.usageMinWidth);
            column = jTable.getColumnModel().getColumn(11);
            column.setMinWidth(this.supArchMinWidth);
            column = jTable.getColumnModel().getColumn(12);
            column.setMinWidth(this.supModMinWidth);
            
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
            
            jTable.getColumnModel().getColumn(5).setCellEditor(new LongTextEditor(topFrame));

            Vector<String> vArch = new Vector<String>();
            vArch.add("IA32");
            vArch.add("X64");
            vArch.add("IPF");
            vArch.add("EBC");
            vArch.add("ARM");
            vArch.add("PPC");
            jTable.getColumnModel().getColumn(11).setCellEditor(new ListEditor(vArch, topFrame));
            
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
            jTable.getColumnModel().getColumn(12).setCellEditor(new ListEditor(vModule, topFrame));
            
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
        int column = arg0.getColumn();
        TableModel m = (TableModel)arg0.getSource();
        if (arg0.getType() == TableModelEvent.UPDATE){
            String[] sa = new String[9];
            sfc.getSpdPcdDeclaration(sa, row);
            Object cellData = m.getValueAt(row, column);
            if (column < 6) {
                
                if (cellData == null) {
                    cellData = "";
                }
                if (cellData.equals(sa[column])) {
                    return;
                }
                if (cellData.toString().length() == 0 && sa[column] == null) {
                    return;
                }
            }
            
            String usage = getValidUsage(new Boolean(m.getValueAt(row, 6)+""), new Boolean(m.getValueAt(row, 7)+""), new Boolean(m.getValueAt(row, 8)+""), new Boolean(m.getValueAt(row, 9)+""), new Boolean(m.getValueAt(row, 10)+""));
            if (usage.length() == 0) {
                JOptionPane.showMessageDialog(this, "You must choose at least one usage for PCD entry.");
                return;
            }

            if (column <= 10 && column >= 6) {
                Vector<String> v = stringToVector(usage);
                if (compareTwoVectors(v, stringToVector(sa[6]))) {
                    return;
                }
                if (v.contains("FEATURE_FLAG")/* && v.size() > 1 && !exclusiveUsage*/) {
                    if (v.size() > 1) {
                        JOptionPane.showMessageDialog(this, "Usage Feature Flag can NOT co-exist with others.");
                        return;
                    }
//                    synchronized (boolModifyLock){
//                        exclusiveUsage = true;
//                    }
//                    m.setValueAt(false, row, 7);
//                    m.setValueAt(false, row, 8);
//                    m.setValueAt(false, row, 9);
//                    m.setValueAt(false, row, 10);
                    else {
                        m.setValueAt("BOOLEAN", row, 3);
                    }
                    
                }
            }
            
            if (column == 11) {
                if (cellData == null) {
                    cellData = "";
                }
                if (cellData.equals(sa[7])) {
                    return;
                }
                if (cellData.toString().length() == 0 && sa[7] == null) {
                    return;
                }
            }
            
            if (column == 12) {
                if (cellData == null) {
                    cellData = "";
                }
                if (cellData.equals(sa[8])) {
                    return;
                }
                if (cellData.toString().length() == 0 && sa[8] == null) {
                    return;
                }
            }
            String cName = m.getValueAt(row, 0) + "";
            String token = m.getValueAt(row, 1) + "";
            String ts = m.getValueAt(row, 2) + "";
            String dataType = m.getValueAt(row, 3) + "";
            String defaultVal = m.getValueAt(row, 4) + "";
            String help = m.getValueAt(row, 5) + "";
            
            
            String archList = null;
            if (m.getValueAt(row, 11) != null){
                archList = m.getValueAt(row, 11).toString();
            }
            String modTypeList = null;
            if (m.getValueAt(row, 12) != null) {
                modTypeList = m.getValueAt(row, 12).toString(); 
            }
            
            Object[] o = {cName, token, ts, dataType, defaultVal, help};
            try {
            if (!dataValidation(o)){
                return;
            }
            }
            catch (Exception e) {
            	JOptionPane.showMessageDialog(this, "Illegal Token:" + e.getCause());
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
    private JCheckBox getJCheckBoxFeatureFlag() {
        if (jCheckBoxFeatureFlag == null) {
            jCheckBoxFeatureFlag = new JCheckBox();
            jCheckBoxFeatureFlag.setBounds(new java.awt.Rectangle(156,161,100,21));
            jCheckBoxFeatureFlag.setText("Feature Flag");
            FontMetrics fm = jCheckBoxFeatureFlag.getFontMetrics(jCheckBoxFeatureFlag.getFont());
            jCheckBoxFeatureFlag.setSize(fm.stringWidth(jCheckBoxFeatureFlag.getText()) + 30, 20);
            jCheckBoxFeatureFlag.addItemListener(new java.awt.event.ItemListener() {
                public void itemStateChanged(java.awt.event.ItemEvent e) {
                    if (jCheckBoxFeatureFlag.isSelected()) {
                        jCheckBoxPatchInMod.setSelected(false);
                        jCheckBoxFixedAtBuild.setSelected(false);
                        jCheckBoxDyn.setSelected(false);
                        jCheckBoxDynEx.setSelected(false);
                        jComboBoxDataType.setSelectedItem("BOOLEAN");
                    }
                }
            });
        }
        return jCheckBoxFeatureFlag;
    }

    /**
     * This method initializes jCheckBox1	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBoxFixedAtBuild() {
        if (jCheckBoxFixedAtBuild == null) {
            jCheckBoxFixedAtBuild = new JCheckBox();
            
            jCheckBoxFixedAtBuild.setText("Fixed at Build");
            FontMetrics fm = jCheckBoxFixedAtBuild.getFontMetrics(jCheckBoxFixedAtBuild.getFont());
            jCheckBoxFixedAtBuild.setSize(fm.stringWidth(jCheckBoxFixedAtBuild.getText()) + 30, 20);
            jCheckBoxFixedAtBuild.addItemListener(new java.awt.event.ItemListener() {
                public void itemStateChanged(java.awt.event.ItemEvent e) {
                    if (jCheckBoxFixedAtBuild.isSelected()) {
                        jCheckBoxFeatureFlag.setSelected(false);
                    }
                }
            });
        }
        return jCheckBoxFixedAtBuild;
    }

    /**
     * This method initializes jCheckBox2	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBoxPatchInMod() {
        if (jCheckBoxPatchInMod == null) {
            jCheckBoxPatchInMod = new JCheckBox();
            
            jCheckBoxPatchInMod.setBounds(new java.awt.Rectangle(156,133,154,20));
            jCheckBoxPatchInMod.setText("Patchable in Module");
            FontMetrics fm = jCheckBoxPatchInMod.getFontMetrics(jCheckBoxPatchInMod.getFont());
            jCheckBoxPatchInMod.setSize(fm.stringWidth(jCheckBoxPatchInMod.getText()) + 30, 20);
            jCheckBoxPatchInMod.addItemListener(new java.awt.event.ItemListener() {
                public void itemStateChanged(java.awt.event.ItemEvent e) {
                    if (jCheckBoxPatchInMod.isSelected()) {
                        jCheckBoxFeatureFlag.setSelected(false);
                    }
                }
            });
        }
        return jCheckBoxPatchInMod;
    }

    /**
     * This method initializes jCheckBox3	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBoxDyn() {
        if (jCheckBoxDyn == null) {
            jCheckBoxDyn = new JCheckBox();
            
            jCheckBoxDyn.setText("Dynamic");
            FontMetrics fm = jCheckBoxDyn.getFontMetrics(jCheckBoxDyn.getFont());
            jCheckBoxDyn.setSize(fm.stringWidth(jCheckBoxDyn.getText()) + 30, 20);
            jCheckBoxDyn.addItemListener(new java.awt.event.ItemListener() {
                public void itemStateChanged(java.awt.event.ItemEvent e) {
                    if (jCheckBoxDyn.isSelected()) {
                        jCheckBoxFeatureFlag.setSelected(false);
                    }
                }
            });
        }
        return jCheckBoxDyn;
    }

    /**
     * This method initializes jCheckBox4	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBoxDynEx() {
        if (jCheckBoxDynEx == null) {
            jCheckBoxDynEx = new JCheckBox();
            
            jCheckBoxDynEx.setText("DynamicEx");
            FontMetrics fm = jCheckBoxDynEx.getFontMetrics(jCheckBoxDynEx.getFont());
            jCheckBoxDynEx.setSize(fm.stringWidth(jCheckBoxDynEx.getText()) + 30, 20);
            jCheckBoxDynEx.addItemListener(new java.awt.event.ItemListener() {
                public void itemStateChanged(java.awt.event.ItemEvent e) {
                    if (jCheckBoxDynEx.isSelected()) {
                        jCheckBoxFeatureFlag.setSelected(false);
                    }
                }
            });
        }
        return jCheckBoxDynEx;
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
    
    private boolean tokenCNameExisted(String token, String cName) throws Exception{
        Long inputToken = Long.decode(token);
        
        for (int i = 0; i < model.getRowCount(); ++i) {
            if (model.getValueAt(i, 0).equals(cName)) {
                JOptionPane.showMessageDialog(this, "C_Name already existed in table.");
                return true;
            }
            if (model.getValueAt(i, 1).equals(token)) {
                JOptionPane.showMessageDialog(this, "Token already existed in table.");
                return true;
            }
            Long tokenValue = Long.decode(model.getValueAt(i, 1)+"");
            if (tokenValue.equals(inputToken)) {
                JOptionPane.showMessageDialog(this, "Same token value already existed in table.");
                return true;
            }
            
        }
        return false;
    }
    
    private boolean checkValidUsage(boolean[] b) {
        if (!(b[0] || b[1] || b[2] || b[3] || b[4])){
            JOptionPane.showMessageDialog(this, "You must specify at least one usage.");
            return false;
        }
        return true;
    }
    private boolean dataValidation(Object[] row) throws Exception{
        
        if (!DataValidation.isC_NameType(row[0].toString())) {
            JOptionPane.showMessageDialog(this, "C_Name is NOT C_NameType.");
            return false;
        }
        if (!DataValidation.isHexDoubleWordDataType(row[1].toString()) && 
                        !DataValidation.isLongInt(row[1].toString(), 1, Long.MAX_VALUE)) {
            JOptionPane.showMessageDialog(this, "Token is NOT correct.");
            return false;
        }
        if (!DataValidation.isC_NameType(row[2].toString())) {
            JOptionPane.showMessageDialog(this, "Token Space is NOT C_NameType");
            return false;
        }
        if (row[5].toString().length() == 0) {
            JOptionPane.showMessageDialog(this, "HelpText could NOT be empty.");
            return false;
        }
        return true;
    }

    /**
     * This method initializes jScrollPane1	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPaneArch() {
        if (jScrollPaneArch == null) {
            jScrollPaneArch = new JScrollPane();
            jScrollPaneArch.setBounds(new java.awt.Rectangle(242,213,188,54));
            jScrollPaneArch.setViewportView(getICheckBoxListArch());
            jScrollPaneArch.setPreferredSize(new Dimension(188, 74));
        }
        return jScrollPaneArch;
    }

    /**
     * This method initializes iCheckBoxList	
     * 	
     * @return org.tianocore.frameworkwizard.common.ui.iCheckBoxList.ICheckBoxList	
     */
    private ICheckBoxList getICheckBoxListArch() {
        if (iCheckBoxListArch == null) {
            iCheckBoxListArch = new ICheckBoxList();
            iCheckBoxListArch.setBounds(new Rectangle(197, 142, 188, 74));
            Vector<String> v = new Vector<String>();
            v.add("IA32");
            v.add("X64");
            v.add("IPF");
            v.add("EBC");
            v.add("ARM");
            v.add("PPC");
            iCheckBoxListArch.setAllItems(v);
        }
        return iCheckBoxListArch;
    }

    /**
     * This method initializes jScrollPane2	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPaneMod() {
        if (jScrollPaneMod == null) {
            jScrollPaneMod = new JScrollPane();
            jScrollPaneMod.setBounds(new java.awt.Rectangle(15,213,199,55));
            jScrollPaneMod.setViewportView(getICheckBoxListMod());
            jScrollPaneMod.setPreferredSize(new Dimension(170, 74));
        }
        return jScrollPaneMod;
    }

    /**
     * This method initializes iCheckBoxList1	
     * 	
     * @return org.tianocore.frameworkwizard.common.ui.iCheckBoxList.ICheckBoxList	
     */
    private ICheckBoxList getICheckBoxListMod() {
        if (iCheckBoxListMod == null) {
            iCheckBoxListMod = new ICheckBoxList();
            iCheckBoxListMod.setBounds(new Rectangle(14, 142, 170, 74));
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
            iCheckBoxListMod.setAllItems(v);
        }
        return iCheckBoxListMod;
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
    
    protected Vector<String> stringToVector(String s){
        Vector<String> v = new Vector<String>();
        if (s == null) {
            return v;
        }
        String[] sArray = s.split(" ");
        
        for (int i = 0; i < sArray.length; ++i) {
            v.add(sArray[i]);
        }
        return v;
    }
    
    private boolean compareTwoVectors(Vector v1, Vector v2) {
        if (v1.size() != v2.size()) {
            return false;
        }
        for (int i = 0; i < v1.size(); ++i) {
            if (!v2.contains(v1.get(i))) {
                return false;
            }
        }
        return true;
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
