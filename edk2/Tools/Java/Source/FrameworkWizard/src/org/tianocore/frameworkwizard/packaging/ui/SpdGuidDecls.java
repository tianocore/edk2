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
import java.awt.FontMetrics;
import java.awt.Point;
import java.awt.event.ActionEvent;
import java.awt.event.ComponentEvent;
import java.util.Vector;

import javax.swing.JFrame;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JTable;
import javax.swing.JTextField;
import javax.swing.JLabel;
import javax.swing.JScrollPane;
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
import org.tianocore.frameworkwizard.common.ui.iCheckBoxList.ICheckBoxList;
import org.tianocore.frameworkwizard.platform.ui.ListEditor;
import org.tianocore.frameworkwizard.platform.ui.LongTextEditor;

/**
 GUI for create library definition elements of spd file.
  
 @since PackageEditor 1.0
**/
public class SpdGuidDecls extends IInternalFrame implements TableModelListener{
    /**
     * 
     */
    private static final long serialVersionUID = 1L;
    
    private JFrame topFrame = null;
    
    private SpdFileContents sfc = null;
    
    private OpeningPackageType docConsole = null;

    private JTable jTable = null;

    private DefaultTableModel model = null;

    private JPanel jContentPane = null;

    private JTextField jTextFieldGuid = null;

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

    private JLabel jLabelCName = null;

    private GenGuidDialog guidDialog = null;

    private JTextField jTextFieldCName = null;

    private JLabel jLabelHelp = null;

    private JTextField jTextFieldHelp = null;

    private JLabel jLabelSupMod = null;

    private JLabel jLabelSupArch = null;

    private ICheckBoxList iCheckBoxListArch = null;

    private ICheckBoxList iCheckBoxListGuid = null;

    private ICheckBoxList iCheckBoxListMod = null;

    private JLabel jLabelGuidType = null;

    protected String[][] saa = null;

    protected StarLabel starLabel = null;
    
    private final int guidNameMinWidth = 200;
    private final int guidCNameMinWidth = 200;
    private final int guidValueMinWidth = 300;
    private final int helpTextMinWidth = 300;
    private final int supArchMinWidth = 200;
    private final int supModMinWidth = 200;
    private final int guidTypeMinWidth = 200;
    

    /**
      This method initializes this
     
     **/
    protected void initialize() {
        
        this.setDefaultCloseOperation(javax.swing.WindowConstants.DISPOSE_ON_CLOSE);

    }

    /**
      This method initializes jTextFieldGuid	
      	
      @return javax.swing.JTextField	
     **/
    protected JTextField getJTextFieldGuid() {
        if (jTextFieldGuid == null) {
            jTextFieldGuid = new JTextField();
            jTextFieldGuid.setBounds(new java.awt.Rectangle(137,60,336,20));
            jTextFieldGuid.setPreferredSize(new java.awt.Dimension(200,20));
            
        }
        return jTextFieldGuid;
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
           model.addColumn("The C Name");
           model.addColumn("GUID Value");
           model.addColumn("Help Text");
           model.addColumn("Supported Architectures");
           model.addColumn("Supported Module Types");
           model.addColumn("GuidTypes");
           
           TableColumn column = jTable.getColumnModel().getColumn(0);
           column.setMinWidth(this.guidNameMinWidth);
           column = jTable.getColumnModel().getColumn(1);
           column.setMinWidth(this.guidCNameMinWidth);
           column = jTable.getColumnModel().getColumn(2);
           column.setMinWidth(this.guidValueMinWidth);
           column = jTable.getColumnModel().getColumn(3);
           column.setMinWidth(this.helpTextMinWidth);
           column = jTable.getColumnModel().getColumn(4);
           column.setMinWidth(this.supArchMinWidth);
           column = jTable.getColumnModel().getColumn(5);
           column.setMinWidth(this.supModMinWidth);
           column = jTable.getColumnModel().getColumn(6);
           column.setMinWidth(this.guidTypeMinWidth);
           
           jTable.getColumnModel().getColumn(2).setCellEditor(new GuidEditor(topFrame));
           jTable.getColumnModel().getColumn(3).setCellEditor(new LongTextEditor(topFrame));

           Vector<String> vArch = new Vector<String>();
           vArch.add("IA32");
           vArch.add("X64");
           vArch.add("IPF");
           vArch.add("EBC");
           vArch.add("ARM");
           vArch.add("PPC");
           jTable.getColumnModel().getColumn(4).setCellEditor(new ListEditor(vArch, topFrame));
           
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
           jTable.getColumnModel().getColumn(5).setCellEditor(new ListEditor(vModule, topFrame));
           
           Vector<String> vGuid = new Vector<String>();
           vGuid.add("DATA_HUB_RECORD");
           vGuid.add("EFI_EVENT");
           vGuid.add("EFI_SYSTEM_CONFIGURATION_TABLE");
           vGuid.add("EFI_VARIABLE");
           vGuid.add("GUID");
           vGuid.add("HII_PACKAGE_LIST");
           vGuid.add("HOB");
           vGuid.add("TOKEN_SPACE_GUID");
           ListEditor le = new ListEditor(vGuid, topFrame);
           le.setCanNotBeEmpty(true);
           jTable.getColumnModel().getColumn(6).setCellEditor(le);
           
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
           
           updateRow(row, column, m);
       }
   }
   
   protected void updateRow(int row, int column, TableModel m){
       String[] sa = new String[7];
       sfc.getSpdGuidDeclaration(sa, row);
       Object cellData = m.getValueAt(row, column);
       if (cellData == null) {
           cellData = "";
       }
       if (cellData.equals(sa[column])) {
           return;
       }
       
       if (cellData.toString().length() == 0 && sa[column] == null) {
           return;
       }
       
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
            jButtonRemove.setText("Delete");
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
    public SpdGuidDecls(JFrame frame) {
        super();
        topFrame = frame;
        initialize();
        init();
        
    }

    public SpdGuidDecls(PackageSurfaceAreaDocument.PackageSurfaceArea inPsa, JFrame frame){
        this(frame);
        sfc = new SpdFileContents(inPsa);
        init(sfc);
    }
    
    public SpdGuidDecls(OpeningPackageType opt, JFrame frame) {
        this(opt.getXmlSpd(), frame);
        docConsole = opt;
        if (sfc.getSpdPkgDefsRdOnly().equals("true")) {
            JOptionPane.showMessageDialog(this, "This is a read-only package. You will not be able to edit contents in table.");
        }
        initFrame();
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
    }

    protected void init(SpdFileContents sfc){
        if (sfc.getSpdGuidDeclarationCount() == 0) {
            return ;
        }
        //
        // initialize table using SpdFileContents object
        //
        saa = new String[sfc.getSpdGuidDeclarationCount()][7];
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
            
            jLabelGuidType = new JLabel();
            jLabelGuidType.setBounds(new java.awt.Rectangle(420,122,103,16));
            jLabelGuidType.setText("GUID Type List");
            jLabelGuidType.setEnabled(true);
            starLabel = new StarLabel();
            starLabel.setLocation(new Point(jLabelGuidType.getX() - 10, jLabelGuidType.getY()));
            starLabel.setVisible(true);
            jLabelSupArch = new JLabel();
            jLabelSupArch.setBounds(new java.awt.Rectangle(197,122,108,16));
            jLabelSupArch.setText("Supported Architectures");
            jLabelSupArch.setEnabled(true);
            FontMetrics fm = jLabelSupArch.getFontMetrics(jLabelSupArch.getFont());
            jLabelSupArch.setSize(fm.stringWidth(jLabelSupArch.getText()) + 10, 20);
            jLabelSupMod = new JLabel();
            jLabelSupMod.setBounds(new java.awt.Rectangle(14,120,110,16));
            jLabelSupMod.setText("Supported Module Types");
            jLabelSupMod.setEnabled(true);
            fm = jLabelSupMod.getFontMetrics(jLabelSupMod.getFont());
            jLabelSupMod.setSize(fm.stringWidth(jLabelSupMod.getText()) + 10, 20);
            jLabelHelp = new JLabel();
            jLabelHelp.setText("HelpText");
            jLabelHelp.setSize(new java.awt.Dimension(109,20));
            jLabelHelp.setLocation(new java.awt.Point(14,85));
            jLabelCName = new JLabel();
            jLabelCName.setBounds(new java.awt.Rectangle(14,35,111,20));
            jLabelCName.setText("C Name");
            jLabelGuid = new JLabel();
            jLabelGuid.setBounds(new java.awt.Rectangle(15,60,112,20));
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
            jContentPane.add(jLabelCName, null);
            jContentPane.add(getJTextFieldCName(), null);
            jContentPane.add(getJTextFieldGuid(), null);
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
            jContentPane.add(jLabelHelp, null);
            jContentPane.add(getJTextFieldHelp(), null);
            jContentPane.add(jLabelSupMod, null);
            jContentPane.add(jLabelSupArch, null);
            jContentPane.add(getJScrollPaneArch(), null);
            jContentPane.add(getJScrollPaneGuid(), null);
            jContentPane.add(getJScrollPaneModule(), null);
            jContentPane.add(jLabelGuidType, null);
            jContentPane.add(starLabel, null);
        }
        return jContentPane;
    }

    /**
     fill ComboBoxes with pre-defined contents
    **/
    protected void initFrame() {
        
        this.setTitle("GUID Declarations");
        
        boolean editable = true;
        if (getSfc().getSpdPkgDefsRdOnly().equals("true")) {
            editable = false;
        }
        
        jButtonAdd.setEnabled(editable);
        jButtonRemove.setEnabled(editable);
        jButtonClearAll.setEnabled(editable);
        jTable.setEnabled(editable);
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
            row[3] = jTextFieldHelp.getText();
            row[2] = jTextFieldGuid.getText();
            row[1] = jTextFieldCName.getText();
            row[0] = jTextFieldName.getText();
            row[4] = vectorToString(iCheckBoxListArch.getAllCheckedItemsString());
            if (row[4].length() == 0) {
                row[4] = null;
            }
            row[5] = vectorToString(iCheckBoxListMod.getAllCheckedItemsString());
            if (row[5].length() == 0) {
                row[5] = null;
            }
            row[6] = vectorToString(iCheckBoxListGuid.getAllCheckedItemsString());
            if (row[6].length() == 0) {
                row[6] = null;
            }
            
            if (!dataValidation(row)) {
                return;
            }
            
            if (addRow(row) == -1) {
                return;
            }
            model.addRow(row);
            jTable.changeSelection(model.getRowCount()-1, 0, false, false);
            
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
            jTextFieldGuid.setText(Tools.generateUuidString());
        }
        
        if (arg0.getActionCommand().equals("GenGuidValue")) {
            jTextFieldGuid.setText(guidDialog.getGuid());
        }
        
    }
    
    protected boolean dataValidation(String[] row){
        if (!DataValidation.isUiNameType(row[0])) {
            JOptionPane.showMessageDialog(this, "Name must start with an alpha character.");
            return false;
        }
        if (!DataValidation.isGuid(row[2])) {
            JOptionPane.showMessageDialog(this, "Guid Value must be in registry format, 8-4-4-4-12.");
            return false;
        }
        if (!DataValidation.isC_NameType(row[1])) {
            JOptionPane.showMessageDialog(this, "C Name does not match C Name datatype.");
            return false;
        }
        if (row[3].length() == 0) {
            JOptionPane.showMessageDialog(this, "Help Text must be entered!");
            return false;
        }
        return true;
    }
    
    protected int addRow(String[] row) {
        Vector<String> vArch = iCheckBoxListArch.getAllCheckedItemsString();
        if (vArch.size() == 0) {
            vArch = null;
        }
        Vector<String> vModType = iCheckBoxListMod.getAllCheckedItemsString();
        if (vModType.size() == 0) {
            vModType = null;
        }
        Vector<String> vguidType = iCheckBoxListGuid.getAllCheckedItemsString();
        if (vguidType.size() == 0) {
            vguidType = null;
        }
        if (vguidType == null) {
            JOptionPane.showMessageDialog(this, "You must select at least one GUID type.");
            return -1;
        }
        if (docConsole != null) {
            docConsole.setSaved(false);
        }
        sfc.genSpdGuidDeclarations(row[0], row[1], row[2], row[3], vArch, vModType, vguidType);
        return 0;
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
            jButtonGen.setBounds(new java.awt.Rectangle(485,58,92,21));
            jButtonGen.setText("Gen");
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
     * This method initializes jTextFieldCName	
     * 	
     * @return javax.swing.JTextField	
     */
    protected JTextField getJTextFieldCName() {
        if (jTextFieldCName == null) {
            jTextFieldCName = new JTextField();
            jTextFieldCName.setBounds(new java.awt.Rectangle(137,35,337,20));
            jTextFieldCName.setPreferredSize(new java.awt.Dimension(335,20));
        }
        return jTextFieldCName;
    }

    public void componentResized(ComponentEvent arg0) {
        int intPreferredWidth = 500;
        
        Tools.resizeComponentWidth(this.jTextFieldName, this.getWidth(), intPreferredWidth);

        Tools.resizeComponentWidth(this.jTextFieldCName, this.getWidth(), intPreferredWidth);
        Tools.resizeComponentWidth(this.jTextFieldHelp, this.getWidth(), intPreferredWidth);
        Tools.resizeComponentWidth(this.jScrollPane, this.getWidth(), intPreferredWidth);
        Tools.relocateComponentX(this.jButtonGen, this.getWidth(), this.getPreferredSize().width, 40);
        
    }
    
    /**
     * This method initializes jTextField	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldHelp() {
        if (jTextFieldHelp == null) {
            jTextFieldHelp = new JTextField();
            jTextFieldHelp.setBounds(new java.awt.Rectangle(136,85,337,20));
            jTextFieldHelp.setPreferredSize(new Dimension(335, 20));
        }
        return jTextFieldHelp;
    }

    private JScrollPane getJScrollPaneArch() {
        if (jScrollPaneArch == null) {
            jScrollPaneArch = new JScrollPane();
            jScrollPaneArch.setBounds(new java.awt.Rectangle(197,142,188,74));
            jScrollPaneArch.setPreferredSize(new java.awt.Dimension(188, 74));
            jScrollPaneArch.setViewportView(getICheckBoxListArch());
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
            iCheckBoxListArch.setBounds(new java.awt.Rectangle(197,142,188,74));
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

    protected JScrollPane getJScrollPaneGuid() {
        if (jScrollPaneGuid== null) {
            jScrollPaneGuid = new JScrollPane();
            jScrollPaneGuid.setPreferredSize(new java.awt.Dimension(190,74));
            jScrollPaneGuid.setLocation(new java.awt.Point(400,142));
            jScrollPaneGuid.setSize(new java.awt.Dimension(260,74));
            jScrollPaneGuid.setViewportView(getICheckBoxListGuid());
        }
        return jScrollPaneGuid;
    }
    /**
     * This method initializes iCheckBoxList1	
     * 	
     * @return org.tianocore.frameworkwizard.common.ui.iCheckBoxList.ICheckBoxList	
     */
    private ICheckBoxList getICheckBoxListGuid() {
        if (iCheckBoxListGuid == null) {
            iCheckBoxListGuid = new ICheckBoxList();
            iCheckBoxListGuid.setBounds(new java.awt.Rectangle(400,142,177,74));
            Vector<String> v = new Vector<String>();
            v.add("DATA_HUB_RECORD");
            v.add("EFI_EVENT");
            v.add("EFI_SYSTEM_CONFIGURATION_TABLE");
            v.add("EFI_VARIABLE");
            v.add("GUID");
            v.add("HII_PACKAGE_LIST");
            v.add("HOB");
            v.add("TOKEN_SPACE_GUID");
          
            iCheckBoxListGuid.setAllItems(v);
        }
        return iCheckBoxListGuid;
    }

    private JScrollPane getJScrollPaneModule() {
        if (jScrollPaneModule == null) {
            jScrollPaneModule = new JScrollPane();
            jScrollPaneModule.setBounds(new java.awt.Rectangle(14,142,170,74));
            jScrollPaneModule.setPreferredSize(new java.awt.Dimension(170, 74));
            jScrollPaneModule.setViewportView(getICheckBoxListMod());
        }
        return jScrollPaneModule;
    }
    /**
     * This method initializes iCheckBoxList2	
     * 	
     * @return org.tianocore.frameworkwizard.common.ui.iCheckBoxList.ICheckBoxList	
     */
    private ICheckBoxList getICheckBoxListMod() {
        if (iCheckBoxListMod == null) {
            iCheckBoxListMod = new ICheckBoxList();
            iCheckBoxListMod.setBounds(new java.awt.Rectangle(14,142,170,74));
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

    protected JLabel getJLabelGuidType() {
        return jLabelGuidType;
    }

    /**
     * @return Returns the sfc.
     */
    protected SpdFileContents getSfc() {
        return sfc;
    }
}


