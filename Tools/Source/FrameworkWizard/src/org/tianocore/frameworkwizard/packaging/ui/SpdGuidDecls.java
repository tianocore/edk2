/** @file
  Java class SpdGuidDecls is GUI for create library definition elements of spd file.
 
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
import java.util.Vector;

import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JTable;
import javax.swing.JTextField;
import javax.swing.JLabel;
import javax.swing.JScrollPane;
import javax.swing.JButton;
import javax.swing.JFrame;
import javax.swing.ListSelectionModel;
import javax.swing.event.InternalFrameAdapter;
import javax.swing.event.InternalFrameEvent;
import javax.swing.event.ListSelectionEvent;
import javax.swing.event.ListSelectionListener;
import javax.swing.event.TableModelEvent;
import javax.swing.event.TableModelListener;
import javax.swing.table.DefaultTableModel;
import javax.swing.table.TableModel;

import org.tianocore.PackageSurfaceAreaDocument;
import org.tianocore.frameworkwizard.common.DataValidation;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.Identifications.OpeningPackageType;
import org.tianocore.frameworkwizard.common.ui.IInternalFrame;
import org.tianocore.frameworkwizard.common.ui.StarLabel;
import org.tianocore.frameworkwizard.common.ui.iCheckBoxList.ICheckBoxList;
import org.tianocore.frameworkwizard.platform.ui.ListEditor;

/**
 GUI for create library definition elements of spd file.
  
 @since PackageEditor 1.0
**/
public class SpdGuidDecls extends IInternalFrame implements TableModelListener{
    /**
     * 
     */
    private static final long serialVersionUID = 1L;

    static JFrame frame;
    
    private SpdFileContents sfc = null;
    
    private OpeningPackageType docConsole = null;

    private JTable jTable = null;

    private DefaultTableModel model = null;

    private JPanel jContentPane = null;

    private JTextField jTextFieldAdd = null;

    private JScrollPane jScrollPane = null;

    private JButton jButtonAdd = null;

    private JButton jButtonRemove = null;

    private JButton jButtonClearAll = null;

    private JButton jButtonCancel = null;

    private JButton jButtonOk = null;

    private JButton jButtonGen = null;
    
    private StarLabel jStarLabel1 = null;
    
    private StarLabel jStarLabel2 = null;
    
    private StarLabel jStarLabel3 = null;
    
    private StarLabel jStarLabel4 = null;

    protected int selectedRow = -1;

    private JLabel jLabelName = null;

    private JScrollPane jScrollPaneModule = null;

    private JTextField jTextFieldName = null;

    private JScrollPane jScrollPaneArch = null;
    
    private JScrollPane jScrollPaneGuid = null;

    private JLabel jLabelGuid = null;

    private JScrollPane topScrollPane = null;  //  @jve:decl-index=0:visual-constraint="10,213"

    private JLabel jLabelVer = null;

    private GenGuidDialog guidDialog = null;

    private JTextField jTextFieldVersion = null;

    private JLabel jLabel = null;

    private JTextField jTextField = null;

    private JLabel jLabel1 = null;

    private JLabel jLabel2 = null;

    private ICheckBoxList iCheckBoxList = null;

    private ICheckBoxList iCheckBoxList1 = null;

    private ICheckBoxList iCheckBoxList2 = null;

    private JLabel jLabel3 = null;



    /**
      This method initializes this
     
     **/
    protected void initialize() {
        
        this.setDefaultCloseOperation(javax.swing.WindowConstants.DISPOSE_ON_CLOSE);

    }

    /**
      This method initializes jTextFieldAdd	
      	
      @return javax.swing.JTextField	
     **/
    protected JTextField getJTextFieldAdd() {
        if (jTextFieldAdd == null) {
            jTextFieldAdd = new JTextField();
            jTextFieldAdd.setBounds(new java.awt.Rectangle(137,35,337,20));
            jTextFieldAdd.setPreferredSize(new java.awt.Dimension(335,20));
            
        }
        return jTextFieldAdd;
    }

    /**
      This method initializes jScrollPane	
      	
      @return javax.swing.JScrollPane	
     **/
    protected JScrollPane getJScrollPane() {
        if (jScrollPane == null) {
            jScrollPane = new JScrollPane();
            jScrollPane.setBounds(new java.awt.Rectangle(5,256,472,292));
            jScrollPane.setViewportView(getJTable());
        }
        return jScrollPane;
    }

    /**
    This method initializes jTable  
        
    @return javax.swing.JTable  
    **/
   protected JTable getJTable() {
       if (jTable == null) {
           model = new DefaultTableModel();
           jTable = new JTable(model);
           jTable.setRowHeight(20);
           jTable.setAutoResizeMode(javax.swing.JTable.AUTO_RESIZE_OFF);
           model.addColumn("Name");
           model.addColumn("C_Name");
           model.addColumn("GUID Value");
           model.addColumn("HelpText");
           model.addColumn("SupportedArch");
           model.addColumn("SupportedModuleType");
           model.addColumn("GuidTypes");
           jTable.getColumnModel().getColumn(2).setCellEditor(new GuidEditor());

           Vector<String> vArch = new Vector<String>();
           vArch.add("IA32");
           vArch.add("X64");
           vArch.add("IPF");
           vArch.add("EBC");
           vArch.add("ARM");
           vArch.add("PPC");
           jTable.getColumnModel().getColumn(4).setCellEditor(new ListEditor(vArch));
           
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
           jTable.getColumnModel().getColumn(5).setCellEditor(new ListEditor(vModule));
           
           Vector<String> vGuid = new Vector<String>();
           vGuid.add("DATA_HUB_RECORD");
           vGuid.add("EFI_EVENT");
           vGuid.add("EFI_SYSTEM_CONFIGURATION_TABLE");
           vGuid.add("EFI_VARIABLE");
           vGuid.add("GUID");
           vGuid.add("HII_PACKAGE_LIST");
           vGuid.add("HOB");
           vGuid.add("TOKEN_SPACE_GUID");
           jTable.getColumnModel().getColumn(6).setCellEditor(new ListEditor(vGuid));
           
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
           
           updateRow(row, m);
       }
   }
   
   protected void updateRow(int row, TableModel m){
       String name = m.getValueAt(row, 0) + "";
       String cName = m.getValueAt(row, 1) + "";
       String guid = m.getValueAt(row, 2) + "";
       String help = m.getValueAt(row, 3) + "";
       String archList = null;
       if (m.getValueAt(row, 4) != null){
           archList = m.getValueAt(row, 4).toString();
       }
       String modTypeList = null;
       if (m.getValueAt(row, 5) != null) {
           modTypeList = m.getValueAt(row, 5).toString(); 
       }
       String guidTypeList = null;
       if (m.getValueAt(row, 6) != null){
           guidTypeList = m.getValueAt(row, 6).toString();
       }
       String[] rowData = {name, cName, guid, help};
       if (!dataValidation(rowData)){
           return;
       }
       if (docConsole != null) {
           docConsole.setSaved(false);
       }
       sfc.updateSpdGuidDecl(row, name, cName, guid, help, archList, modTypeList, guidTypeList);
   }
    /**
      This method initializes jButtonAdd	
      	
      @return javax.swing.JButton	
     **/
    protected JButton getJButtonAdd() {
        if (jButtonAdd == null) {
            jButtonAdd = new JButton();
            jButtonAdd.setBounds(new java.awt.Rectangle(167,227,90,20));
            jButtonAdd.setText("Add");
            jButtonAdd.addActionListener(this);
        }
        return jButtonAdd;
    }

    /**
      This method initializes jButtonRemove	
      	
      @return javax.swing.JButton	
     **/
    protected JButton getJButtonRemove() {
        if (jButtonRemove == null) {
            jButtonRemove = new JButton();
            jButtonRemove.setBounds(new java.awt.Rectangle(270,227,90,20));
            jButtonRemove.setText("Remove");
            jButtonRemove.addActionListener(this);
        }
        return jButtonRemove;
    }

    /**
      This method initializes jButtonRemoveAll	
      	
      @return javax.swing.JButton	
     **/
    protected JButton getJButtonClearAll() {
        if (jButtonClearAll == null) {
            jButtonClearAll = new JButton();
            jButtonClearAll.setBounds(new java.awt.Rectangle(380,227,90,20));
            jButtonClearAll.setText("Clear All");
            jButtonClearAll.addActionListener(this);
        }
        return jButtonClearAll;
    }

    /**
      This method initializes jButtonCancel	
      	
      @return javax.swing.JButton	
     **/
    protected JButton getJButtonCancel() {
        if (jButtonCancel == null) {
            jButtonCancel = new JButton();
            jButtonCancel.setPreferredSize(new java.awt.Dimension(90, 20));
            jButtonCancel.setLocation(new java.awt.Point(390, 305));
            jButtonCancel.setText("Cancel");
            jButtonCancel.setSize(new java.awt.Dimension(90, 20));
            jButtonCancel.setVisible(false);
            jButtonCancel.addActionListener(this);
        }
        return jButtonCancel;
    }

    /**
      This method initializes jButton	
      	
      @return javax.swing.JButton	
     **/
    protected JButton getJButtonOk() {
        if (jButtonOk == null) {
            jButtonOk = new JButton();
            jButtonOk.setSize(new java.awt.Dimension(90, 20));
            jButtonOk.setText("OK");
            jButtonOk.setLocation(new java.awt.Point(290, 305));
            jButtonOk.setVisible(false);
            jButtonOk.addActionListener(this);
        }
        return jButtonOk;
    }

    /**
      This is the default constructor
     **/
    public SpdGuidDecls() {
        super();
        initialize();
        init();
        
    }

    public SpdGuidDecls(PackageSurfaceAreaDocument.PackageSurfaceArea inPsa){
        this();
        sfc = new SpdFileContents(inPsa);
        init(sfc);
    }
    
    public SpdGuidDecls(OpeningPackageType opt) {
        this(opt.getXmlSpd());
        docConsole = opt;
    }
    /**
      This method initializes this
      
      @return void
     **/
    protected void init() {
        this.setContentPane(getJContentPane());
        this.addInternalFrameListener(new InternalFrameAdapter(){
            public void internalFrameDeactivated(InternalFrameEvent e){
                if (jTable.isEditing()) {
                    jTable.getCellEditor().stopCellEditing();
                }
            }
        });
        this.setBounds(new java.awt.Rectangle(0, 0, 500, 370));
        this.setVisible(true);
        initFrame();
    }

    protected void init(SpdFileContents sfc){
        if (sfc.getSpdGuidDeclarationCount() == 0) {
            return ;
        }
        //
        // initialize table using SpdFileContents object
        //
        String[][] saa = new String[sfc.getSpdGuidDeclarationCount()][7];
        sfc.getSpdGuidDeclarations(saa);
        int i = 0;
        while (i < saa.length) {
            model.addRow(saa[i]);
            i++;
        }
    }
    
    protected JScrollPane getJContentPane(){
        if (topScrollPane == null){
            topScrollPane = new JScrollPane();
            topScrollPane.setSize(new java.awt.Dimension(617,500));
            topScrollPane.setPreferredSize(new java.awt.Dimension(498,500));
            topScrollPane.setViewportView(getJContentPane1());
        }
        return topScrollPane;
    }
    
    /**
      This method initializes jContentPane
      
      @return javax.swing.JPanel
     **/
    protected JPanel getJContentPane1() {
        if (jContentPane == null) {
            jLabel3 = new JLabel();
            jLabel3.setBounds(new java.awt.Rectangle(400,122,103,16));
            jLabel3.setText("GUID Type List");
            jLabel3.setEnabled(true);
            jLabel2 = new JLabel();
            jLabel2.setBounds(new java.awt.Rectangle(197,122,108,16));
            jLabel2.setText("Supported Arch");
            jLabel2.setEnabled(true);
            jLabel1 = new JLabel();
            jLabel1.setBounds(new java.awt.Rectangle(14,120,110,16));
            jLabel1.setText("Supported Module");
            jLabel1.setEnabled(true);
            jLabel = new JLabel();
            jLabel.setText("HelpText");
            jLabel.setSize(new java.awt.Dimension(109,20));
            jLabel.setLocation(new java.awt.Point(14,85));
            jLabelVer = new JLabel();
            jLabelVer.setBounds(new java.awt.Rectangle(14,60,111,20));
            jLabelVer.setText("C_Name");
            jLabelGuid = new JLabel();
            jLabelGuid.setBounds(new java.awt.Rectangle(15,35,112,20));
            jLabelGuid.setText("Guid Value");
            jLabelName = new JLabel();
            jLabelName.setBounds(new java.awt.Rectangle(15,10,113,20));
            jLabelName.setText("Name");
            jStarLabel1 = new StarLabel();
            jStarLabel1.setLocation(new java.awt.Point(0, 10));
            jStarLabel3 = new StarLabel();
            jStarLabel3.setLocation(new java.awt.Point(0, 35));
            jStarLabel4 = new StarLabel();
            jStarLabel4.setLocation(new java.awt.Point(0, 60));
            jStarLabel2 = new StarLabel();
            jStarLabel2.setLocation(new java.awt.Point(0, 85));
            jStarLabel2.setVisible(true);
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.setPreferredSize(new Dimension(480, 375));
            jContentPane.add(jStarLabel1, null);
            jContentPane.add(jStarLabel2, null);
            jContentPane.add(jStarLabel3, null);
            jContentPane.add(jStarLabel4, null);
            jContentPane.add(jLabelVer, null);
            jContentPane.add(getJTextFieldVersion(), null);
            jContentPane.add(getJTextFieldAdd(), null);
            jContentPane.add(getJScrollPane(), null);
            jContentPane.add(getJButtonAdd(), null);
            jContentPane.add(getJButtonRemove(), null);
            jContentPane.add(getJButtonClearAll(), null);
            jContentPane.add(getJButtonCancel(), null);
            jContentPane.add(getJButtonOk(), null);
            
            jContentPane.add(getJButtonGen(), null);
            jContentPane.add(jLabelName, null);
            jContentPane.add(getJTextFieldName(), null);
            jContentPane.add(jLabelGuid, null);
            jContentPane.add(jLabel, null);
            jContentPane.add(getJTextField(), null);
            jContentPane.add(jLabel1, null);
            jContentPane.add(jLabel2, null);
            jContentPane.add(getJScrollPaneArch(), null);
            jContentPane.add(getJScrollPaneGuid(), null);
            jContentPane.add(getJScrollPaneModule(), null);
            jContentPane.add(jLabel3, null);
        }
        return jContentPane;
    }

    /**
     fill ComboBoxes with pre-defined contents
    **/
    protected void initFrame() {
        
        this.setTitle("GUID Declarations");

    }

    /* (non-Javadoc)
     * @see java.awt.event.ActionListener#actionPerformed(java.awt.event.ActionEvent)
     */
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
            String[] row = {"", "", "", "", "", "", ""};
            row[3] = jTextField.getText();
            row[2] = jTextFieldAdd.getText();
            row[1] = jTextFieldVersion.getText();
            row[0] = jTextFieldName.getText();
            row[4] = vectorToString(iCheckBoxList.getAllCheckedItemsString());
            if (row[4].length() == 0) {
                row[4] = null;
            }
            row[5] = vectorToString(iCheckBoxList2.getAllCheckedItemsString());
            if (row[5].length() == 0) {
                row[5] = null;
            }
            row[6] = vectorToString(iCheckBoxList1.getAllCheckedItemsString());
            if (row[6].length() == 0) {
                row[6] = null;
            }
            
            if (!dataValidation(row)) {
                return;
            }
            
            model.addRow(row);
            jTable.changeSelection(model.getRowCount()-1, 0, false, false);
            addRow(row);
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
                removeRow(rowSelected);
            }
        }

        if (arg0.getSource() == jButtonClearAll) {
            if (model.getRowCount() == 0) {
                return;
            }
            model.setRowCount(0);
            clearAllRow();
        }
        
        if (arg0.getSource() == jButtonGen) {
            jTextFieldAdd.setText(Tools.generateUuidString());
        }
        
        if (arg0.getActionCommand().equals("GenGuidValue")) {
            jTextFieldAdd.setText(guidDialog.getGuid());
        }
        
    }
    
    protected boolean dataValidation(String[] row){
        if (!DataValidation.isUiNameType(row[0])) {
            JOptionPane.showMessageDialog(this, "Name is NOT UiNameType.");
            return false;
        }
        if (!DataValidation.isGuid(row[2])) {
            JOptionPane.showMessageDialog(this, "Guid Value is NOT GuidType.");
            return false;
        }
        if (!DataValidation.isC_NameType(row[1])) {
            JOptionPane.showMessageDialog(this, "C_Name is NOT C_NameType.");
            return false;
        }
        if (row[3].length() == 0) {
            JOptionPane.showMessageDialog(this, "HelpText could NOT be empty.");
            return false;
        }
        return true;
    }
    
    protected void addRow(String[] row) {
        Vector<String> vArch = iCheckBoxList.getAllCheckedItemsString();
        if (vArch.size() == 0) {
            vArch = null;
        }
        Vector<String> vModType = iCheckBoxList2.getAllCheckedItemsString();
        if (vModType.size() == 0) {
            vModType = null;
        }
        Vector<String> vguidType = iCheckBoxList1.getAllCheckedItemsString();
        if (vguidType.size() == 0) {
            vguidType = null;
        }
        if (docConsole != null) {
            docConsole.setSaved(false);
        }
        sfc.genSpdGuidDeclarations(row[0], row[1], row[2], row[3], vArch, vModType, vguidType);
    }
    
    protected void removeRow(int i){
        sfc.removeSpdGuidDeclaration(i);
        if (docConsole != null) {
            docConsole.setSaved(false);
        }
    }
    
    protected void clearAllRow(){
        sfc.removeSpdGuidDeclaration();
        if (docConsole != null) {
            docConsole.setSaved(false);
        }
    }

    /**
     Add contents in list to sfc
    **/
    protected void save() {
        
    }

    /**
      This method initializes jButtonBrowse	
      	
      @return javax.swing.JButton	
     **/
    protected JButton getJButtonGen() {
        if (jButtonGen == null) {
            jButtonGen = new JButton();
            jButtonGen.setBounds(new java.awt.Rectangle(379,58,92,21));
            jButtonGen.setText("Gen GUID");
            jButtonGen.setPreferredSize(new java.awt.Dimension(80,20));
            jButtonGen.addActionListener(this);
        }
        return jButtonGen;
    }
    
    /**
     * This method initializes jTextFieldName	
     * 	
     * @return javax.swing.JTextField	
     */
    protected JTextField getJTextFieldName() {
        if (jTextFieldName == null) {
            jTextFieldName = new JTextField();
            jTextFieldName.setBounds(new java.awt.Rectangle(138,10,337,20));
            jTextFieldName.setPreferredSize(new java.awt.Dimension(335,20));
        }
        return jTextFieldName;
    }

    /**
     * This method initializes jTextFieldVersion	
     * 	
     * @return javax.swing.JTextField	
     */
    protected JTextField getJTextFieldVersion() {
        if (jTextFieldVersion == null) {
            jTextFieldVersion = new JTextField();
            jTextFieldVersion.setBounds(new java.awt.Rectangle(137,60,225,20));
            jTextFieldVersion.setPreferredSize(new java.awt.Dimension(225,20));
        }
        return jTextFieldVersion;
    }

    public void componentResized(ComponentEvent arg0) {
        int intPreferredWidth = 500;
        
        resizeComponentWidth(this.jTextFieldName, this.getWidth(), intPreferredWidth);
        resizeComponentWidth(this.jTextFieldAdd, this.getWidth(), intPreferredWidth);
        resizeComponentWidth(this.jTextFieldVersion, this.getWidth(), intPreferredWidth);
        resizeComponentWidth(this.jTextField, this.getWidth(), intPreferredWidth);
        resizeComponentWidth(this.jScrollPane, this.getWidth(), intPreferredWidth);
        relocateComponentX(this.jButtonGen, this.getWidth(), this.getPreferredSize().width, 40);
        
    }
    
    /**
     * This method initializes jTextField	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextField() {
        if (jTextField == null) {
            jTextField = new JTextField();
            jTextField.setBounds(new java.awt.Rectangle(136,85,337,20));
            jTextField.setPreferredSize(new Dimension(335, 20));
        }
        return jTextField;
    }

    private JScrollPane getJScrollPaneArch() {
        if (jScrollPaneArch == null) {
            jScrollPaneArch = new JScrollPane();
            jScrollPaneArch.setBounds(new java.awt.Rectangle(197,142,188,74));
            jScrollPaneArch.setPreferredSize(new java.awt.Dimension(188, 74));
            jScrollPaneArch.setViewportView(getICheckBoxList());
        }
        return jScrollPaneArch;
    }
    /**
     * This method initializes iCheckBoxList	
     * 	
     * @return org.tianocore.frameworkwizard.common.ui.iCheckBoxList.ICheckBoxList	
     */
    private ICheckBoxList getICheckBoxList() {
        if (iCheckBoxList == null) {
            iCheckBoxList = new ICheckBoxList();
            iCheckBoxList.setBounds(new java.awt.Rectangle(197,142,188,74));
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

    protected JScrollPane getJScrollPaneGuid() {
        if (jScrollPaneGuid== null) {
            jScrollPaneGuid = new JScrollPane();
            jScrollPaneGuid.setPreferredSize(new java.awt.Dimension(190,74));
            jScrollPaneGuid.setLocation(new java.awt.Point(400,142));
            jScrollPaneGuid.setSize(new java.awt.Dimension(260,74));
            jScrollPaneGuid.setViewportView(getICheckBoxList1());
        }
        return jScrollPaneGuid;
    }
    /**
     * This method initializes iCheckBoxList1	
     * 	
     * @return org.tianocore.frameworkwizard.common.ui.iCheckBoxList.ICheckBoxList	
     */
    private ICheckBoxList getICheckBoxList1() {
        if (iCheckBoxList1 == null) {
            iCheckBoxList1 = new ICheckBoxList();
            iCheckBoxList1.setBounds(new java.awt.Rectangle(400,142,177,74));
            Vector<String> v = new Vector<String>();
            v.add("DATA_HUB_RECORD");
            v.add("EFI_EVENT");
            v.add("EFI_SYSTEM_CONFIGURATION_TABLE");
            v.add("EFI_VARIABLE");
            v.add("GUID");
            v.add("HII_PACKAGE_LIST");
            v.add("HOB");
            v.add("TOKEN_SPACE_GUID");
          
            iCheckBoxList1.setAllItems(v);
        }
        return iCheckBoxList1;
    }

    private JScrollPane getJScrollPaneModule() {
        if (jScrollPaneModule == null) {
            jScrollPaneModule = new JScrollPane();
            jScrollPaneModule.setBounds(new java.awt.Rectangle(14,142,170,74));
            jScrollPaneModule.setPreferredSize(new java.awt.Dimension(170, 74));
            jScrollPaneModule.setViewportView(getICheckBoxList2());
        }
        return jScrollPaneModule;
    }
    /**
     * This method initializes iCheckBoxList2	
     * 	
     * @return org.tianocore.frameworkwizard.common.ui.iCheckBoxList.ICheckBoxList	
     */
    private ICheckBoxList getICheckBoxList2() {
        if (iCheckBoxList2 == null) {
            iCheckBoxList2 = new ICheckBoxList();
            iCheckBoxList2.setBounds(new java.awt.Rectangle(14,142,170,74));
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
            iCheckBoxList2.setAllItems(v);
        }
        return iCheckBoxList2;
    }

    public static void main(String[] args){
        new SpdGuidDecls().setVisible(true);
    }

    protected DefaultTableModel getModel() {
        return model;
    }

    protected void setModel(DefaultTableModel model) {
        this.model = model;
    }
    
    protected String vectorToString(Vector<String> v) {
        String s = " ";
        for (int i = 0; i < v.size(); ++i) {
            s += v.get(i);
            s += " ";
        }
        return s.trim();
    }
    
    protected Vector<String> stringToVector(String s){
        if (s == null) {
            return null;
        }
        String[] sArray = s.split(" ");
        Vector<String> v = new Vector<String>();
        for (int i = 0; i < sArray.length; ++i) {
            v.add(sArray[i]);
        }
        return v;
    }

    protected JLabel getJLabel3() {
        return jLabel3;
    }
}


