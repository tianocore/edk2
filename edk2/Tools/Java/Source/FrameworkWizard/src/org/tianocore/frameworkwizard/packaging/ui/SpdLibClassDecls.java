/** @file
  Java class SpdLibClassDecls is GUI for create library definition elements of spd file.
 
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
import java.io.File;
import java.util.HashMap;
//import java.util.Iterator;
//import java.util.Set;
import java.util.Vector;

import javax.swing.AbstractAction;
import javax.swing.JFileChooser;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JTable;
import javax.swing.JTextField;
import javax.swing.JTextArea;
import javax.swing.JComboBox;
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
import javax.swing.table.TableColumn;
import javax.swing.table.TableModel;
import org.tianocore.PackageSurfaceAreaDocument;
import org.tianocore.frameworkwizard.common.DataType;
import org.tianocore.frameworkwizard.common.DataValidation;
//import org.tianocore.frameworkwizard.common.GlobalData;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.Identifications.OpeningPackageType;
import org.tianocore.frameworkwizard.common.ui.IInternalFrame;
import org.tianocore.frameworkwizard.common.ui.StarLabel;
import org.tianocore.frameworkwizard.common.ui.iCheckBoxList.ICheckBoxList;
import org.tianocore.frameworkwizard.platform.ui.ListEditor;
import org.tianocore.frameworkwizard.platform.ui.LongTextEditor;
//import org.tianocore.frameworkwizard.platform.ui.global.SurfaceAreaQuery;
//import org.tianocore.frameworkwizard.module.Identifications.ModuleIdentification;


/**
 GUI for create library definition elements of spd file.
  
 @since PackageEditor 1.0
**/
public class SpdLibClassDecls extends IInternalFrame implements TableModelListener{
    /**
     * 
     */
    private static final long serialVersionUID = 1L;

    private JFrame topFrame;
    
    private JTable jTable = null;

    private DefaultTableModel model = null;

    private JPanel jContentPane = null;

    private JTextField jTextFieldAddClass = null;

    private JComboBox jComboBoxSelect = null;

    private JScrollPane jScrollPaneTable = null;

    private JButton jButtonAdd = null;

    private JButton jButtonRemove = null;

    private JButton jButtonRemoveAll = null;

    private JLabel jLabelHdr = null;

    private JTextField jTextFieldHdr = null;

    private JButton jButtonBrowse = null;
    
    private StarLabel starLabel1 = null;
    
    private StarLabel starLabel3 = null;
    
    private SpdFileContents sfc = null;
    
    private OpeningPackageType docConsole = null;

    private JLabel jLabel1ClassName = null;
    
    private JScrollPane topScrollPane = null;  //  @jve:decl-index=0:visual-constraint="10,53"
    
    private int selectedRow = -1;

    private StarLabel starLabel2 = null;

    private JLabel jLabel2HelpText = null;

    private JTextArea jTextAreaHelp = null;

    private JScrollPane jHelpTextScrollPane = null;

    private JLabel jLabel5SupArchList = null;

    private JLabel jLabel6SupModList = null;
    
    private JScrollPane jScrollPaneModules = null;
    
    private JScrollPane jScrollPane1Arch = null;
    
    private ICheckBoxList iCheckBoxListModules = null;

    private ICheckBoxList iCheckBoxListArch = null;

    private int cnClassName = 0;
    private int cnHdrFile = 1;
    private int cnHelpText = 2;
//    private int cnRecInstName = 3;
//    private int cnRecInstVer = 4;
    private int cnSupArch = 5;
    private int cnSupMod = 6;
    
    private final int classNameMinWidth = 200;
    private final int hdrFileMinWidth = 300;
    private final int helpTextMinWidth = 300;
    private final int supArchMinWidth = 200;
    private final int supModMinWidth = 200;

    private final int shortLabel = 90;
    private final int longLabel = 220;
    private final int labelCol = 12;
    private final int shortValueCol = labelCol + shortLabel + 6;
    private final int longValueCol = labelCol + longLabel + 6;
    private final int longValueWidth = 347;
    private final int shortWidth = 140;
    private final int medWidth = 240;
   
    private final int buttonWidth = 99;

    private final int addButtonCol = shortValueCol + 10;
    private final int removeButtonCol = addButtonCol + buttonWidth + 10;
    private final int removeAllButtonCol = removeButtonCol + buttonWidth + 10;

    private final int rowOne = 12;
    private final int rowTwo = rowOne + 25;
    private final int rowThree = rowTwo + 60 + 25;
    private final int rowFour = rowThree + 25;
    private final int rowFive = rowFour + 40 + 25;
    private final int rowSix = rowFive + 40 + 25;
    private final int rowSeven = rowSix;
    private final int rowEight = rowSeven + 30;

    HashMap<String, String> libNameGuidMap = new HashMap<String, String>();


    /**
      This method initializes this
     
     **/
    private void initialize() {
        
        this.setDefaultCloseOperation(javax.swing.WindowConstants.DISPOSE_ON_CLOSE);

    }

    /**
      This method initializes jTextFieldAddClass	
      	
      @return javax.swing.JTextField	
     **/
    private JTextField getJTextFieldAddClass() {
        if (jTextFieldAddClass == null) {
            jTextFieldAddClass = new JTextField();
            jTextFieldAddClass.setBounds(new java.awt.Rectangle(shortValueCol,rowOne,longValueWidth,20));
            jTextFieldAddClass.setPreferredSize(new java.awt.Dimension(longValueWidth,20));
            jTextFieldAddClass.setEnabled(true);
        }
        return jTextFieldAddClass;
    }

    /**
      This method initializes jComboBoxSelect	
      	
      @return javax.swing.JComboBox	
     **/
    private JComboBox getJComboBoxSelect() {
        if (jComboBoxSelect == null) {
            jComboBoxSelect = new JComboBox();
            jComboBoxSelect.setBounds(new java.awt.Rectangle(220, 10, 260, 20));
            jComboBoxSelect.setPreferredSize(new java.awt.Dimension(260,22));
            jComboBoxSelect.setEnabled(true);
            jComboBoxSelect.setVisible(false);
        }
        return jComboBoxSelect;
    }

    /**
      This method initializes jScrollPaneTable	
      	
      @return javax.swing.JScrollPane	

      Used for the Table of Library Classes that are provided by this package

     **/
    private JScrollPane getJScrollPaneTable() {
        if (jScrollPaneTable == null) {
            jScrollPaneTable = new JScrollPane();
            jScrollPaneTable.setBounds(new java.awt.Rectangle(labelCol,rowEight,400,253));
            jScrollPaneTable.setViewportView(getJTable());
        }
        return jScrollPaneTable;
    }

    /**
    This method initializes jTable  
        
    @return javax.swing.JTable  
    **/
   private JTable getJTable() {
       if (jTable == null) {
           model = new DefaultTableModel();
           jTable = new JTable(model);
           jTable.setRowHeight(20);
           jTable.setAutoResizeMode(javax.swing.JTable.AUTO_RESIZE_OFF);
           jTable.setColumnSelectionAllowed(false);
           model.addColumn("Class Name");
           model.addColumn("Header");
           model.addColumn("Help Text");
           model.addColumn("Recommended Instance");
           model.addColumn("Version");
           model.addColumn("Supported Architectures");
           model.addColumn("Supported Module Types");
           
           jTable.getColumnModel().getColumn(cnHelpText).setCellEditor(new LongTextEditor(topFrame));
           
           jTable.removeColumn(jTable.getColumnModel().getColumn(3));
           jTable.removeColumn(jTable.getColumnModel().getColumn(3));
           
           Vector<String> vArch = new Vector<String>();
           vArch.add("IA32");
           vArch.add("X64");
           vArch.add("IPF");
           vArch.add("EBC");
           vArch.add("ARM");
           vArch.add("PPC");
           jTable.getColumnModel().getColumn(cnSupArch - 2).setCellEditor(new ListEditor(vArch, topFrame));
           
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

           jTable.getColumnModel().getColumn(cnSupMod - 2).setCellEditor(new ListEditor(vModule, topFrame));
           
           TableColumn column = jTable.getColumnModel().getColumn(this.cnClassName);
           column.setMinWidth(this.classNameMinWidth);
           column = jTable.getColumnModel().getColumn(this.cnHdrFile);
           column.setMinWidth(this.hdrFileMinWidth);
           column = jTable.getColumnModel().getColumn(this.cnHelpText);
           column.setMinWidth(this.helpTextMinWidth);
           column = jTable.getColumnModel().getColumn(this.cnSupArch - 2);
           column.setMinWidth(this.supArchMinWidth);
           column = jTable.getColumnModel().getColumn(this.cnSupMod - 2);
           column.setMinWidth(this.supModMinWidth);
           
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
            
            String lib = m.getValueAt(row, cnClassName) + "";
            String hdr = m.getValueAt(row, cnHdrFile) + "";
            String hlp = m.getValueAt(row, cnHelpText) + "";
            String name = null;
//            if (m.getValueAt(row, cnRecInstName) != null) {
//                name = m.getValueAt(row, cnRecInstName).toString();
//            } 
//            String ver = null;
//            if (m.getValueAt(row, cnRecInstVer) != null){
//                ver = m.getValueAt(row, cnRecInstVer).toString();
//            }
            String arch = null;
            if (m.getValueAt(row, cnSupArch) != null) {
               arch = m.getValueAt(row, cnSupArch).toString();
            }
            String module = null;
            if (m.getValueAt(row, cnSupMod) != null) {
                module = m.getValueAt(row, cnSupMod).toString();
            }
            String[] rowData = {lib, hdr, hlp, name};
            if (!dataValidation(rowData)) {
                return;
            }
            
            String guid = null;
//            if (name != null && name.length() > 0) {
//                getLibInstances(lib);
//                guid = nameToGuid(name);
//                if (guid == null){
//                  JOptionPane.showMessageDialog(frame, "Recommended Instance does not exist.");
//                  return;
//                }
//            }
            
            String[] sa = new String[7];
            sfc.getSpdLibClassDeclaration(sa, row);
            Object cellData = m.getValueAt(row, column);
            if (cellData == null) {
                cellData = "";
            }
//            if (column == cnRecInstName) {
//                if (guid == null) {
//                    if (sa[cnRecInstName] == null) {
//                        return;
//                    }
//                }
//                else {
//                    if (guid.equals(sa[cnRecInstName])) {
//                        return;
//                    }
//                }
//            }
            if (cellData.equals(sa[column])) {
                return;
            }
            if (cellData.toString().length() == 0 && sa[column] == null) {
                return;
            }
            docConsole.setSaved(false);
            sfc.updateSpdLibClass(row, lib, hdr, hlp, guid, null, arch, module);
        }
    }

    /**
      This method initializes jButtonAdd	
      	
      @return javax.swing.JButton	
     **/
    private JButton getJButtonAdd() {
        if (jButtonAdd == null) {
            jButtonAdd = new JButton();
            jButtonAdd.setText("Add");
            jButtonAdd.setSize(new java.awt.Dimension(buttonWidth,20));
            jButtonAdd.setBounds(new java.awt.Rectangle(addButtonCol,rowSeven,buttonWidth,20));
            jButtonAdd.addActionListener(this);
        }
        return jButtonAdd;
    }

    /**
      This method initializes jButtonRemove	
      	
      @return javax.swing.JButton	
     **/
    private JButton getJButtonRemove() {
        if (jButtonRemove == null) {
            jButtonRemove = new JButton();
            jButtonRemove.setText("Delete");
            jButtonRemove.setSize(new java.awt.Dimension(buttonWidth,20));
            jButtonRemove.setBounds(new java.awt.Rectangle(removeButtonCol,rowSeven,buttonWidth,20));
            jButtonRemove.addActionListener(this);
        }
        return jButtonRemove;
    }

    /**
      This method initializes jButtonRemoveAll	
      	
      @return javax.swing.JButton	
     **/
    private JButton getJButtonRemoveAll() {
        if (jButtonRemoveAll == null) {
            jButtonRemoveAll = new JButton();
            jButtonRemoveAll.setText("Clear All");
            jButtonRemoveAll.setLocation(removeAllButtonCol,rowSeven);
            FontMetrics fm = jButtonRemoveAll.getFontMetrics(jButtonRemoveAll.getFont());
            jButtonRemoveAll.setSize(fm.stringWidth(jButtonRemoveAll.getText()) + 50, 20);
            jButtonRemoveAll.addActionListener(this);
        }
        return jButtonRemoveAll;
    }

    /**
      This is the default constructor
     **/
    public SpdLibClassDecls(JFrame frame) {
        super();
        topFrame = frame;
        initialize();
        init();
        
    }

    public SpdLibClassDecls(PackageSurfaceAreaDocument.PackageSurfaceArea inPsa, JFrame frame){
        this(frame);
        sfc = new SpdFileContents(inPsa);
        init(sfc);
    }
    
    public SpdLibClassDecls(OpeningPackageType opt, JFrame frame) {
        this(opt.getXmlSpd(), frame);
        docConsole = opt;
    }
    /**
      This method initializes this
      
      @return void
     **/
    private void init() {
        
        this.setContentPane(getJContentPane());
        this.setTitle("Library Class Declarations");
        this.setBounds(new java.awt.Rectangle(0, 0, 500, 370));
        this.setVisible(true);
        this.addInternalFrameListener(new InternalFrameAdapter(){
            public void internalFrameDeactivated(InternalFrameEvent e){
                if (jTable.isEditing()) {
                    jTable.getCellEditor().stopCellEditing();
                }
            }
        });
    }

    private void init(SpdFileContents sfc) {

        if (sfc.getSpdPkgDefsRdOnly().equals("true")) {
            JOptionPane.showMessageDialog(topFrame, "This is a read-only package. You will not be able to edit contents in table.");
        }
        initFrame();
        
        if (sfc.getSpdLibClassDeclarationCount() == 0) {
            return ;
        }
        //
        // initialize table using SpdFileContents object
        //
        String[][] saa = new String[sfc.getSpdLibClassDeclarationCount()][7];
        sfc.getSpdLibClassDeclarations(saa);
        int i = 0;
        while (i < saa.length) {
//            if (saa[i][3] != null && saa[i][3].length() > 0) {
//                getLibInstances(saa[i][0]);
//                saa[i][3] = guidToName(saa[i][3]);
//            }
            
            model.addRow(saa[i]);
            i++;
        }
        
    }
    
    private void initFrame() {
        boolean editable = true;
        if (sfc.getSpdPkgDefsRdOnly().equals("true")) {
            editable = false;
        }
        
        jButtonAdd.setEnabled(editable);
        jButtonRemove.setEnabled(editable);
        jButtonRemoveAll.setEnabled(editable);
        jTable.setEnabled(editable);
    }

    private JScrollPane getJContentPane(){
        if (topScrollPane == null){
          topScrollPane = new JScrollPane();
          topScrollPane.setViewportView(getJContentPane1());
        }
        return topScrollPane;
    }
    /**
      This method initializes jContentPane
      
      @return javax.swing.JPanel
     **/
    private JPanel getJContentPane1() {
        if (jContentPane == null) {
            // Library Class
            starLabel1 = new StarLabel();
            starLabel1.setLocation(new java.awt.Point(1,rowOne));
            jLabel1ClassName = new JLabel();
            jLabel1ClassName.setBounds(new java.awt.Rectangle(labelCol,rowOne,shortLabel,20));
            jLabel1ClassName.setText("Library Class");

            // Help Text
            starLabel2 = new StarLabel();
            starLabel2.setBounds(new java.awt.Rectangle(1,rowTwo,10,20));
            jLabel2HelpText = new JLabel();
            jLabel2HelpText.setBounds(new java.awt.Rectangle(labelCol,rowTwo,shortLabel,20));
            jLabel2HelpText.setText("Help Text");

            // Header File
            starLabel3 = new StarLabel();
            starLabel3.setLocation(new java.awt.Point(1,rowThree));
            jLabelHdr = new JLabel();
            jLabelHdr.setBounds(new java.awt.Rectangle(labelCol,rowThree,longLabel,20));
            jLabelHdr.setText("Include Header for Specified Class");

            jLabel6SupModList = new JLabel();
            jLabel6SupModList.setBounds(new java.awt.Rectangle(labelCol,rowFive,longLabel,20));
            jLabel6SupModList.setText("Supported Module Types");
            jLabel6SupModList.setEnabled(true);

            jLabel5SupArchList = new JLabel();
            jLabel5SupArchList.setBounds(new java.awt.Rectangle(labelCol,rowFour,longLabel,20));
            jLabel5SupArchList.setText("Supported Architectures");
            jLabel5SupArchList.setEnabled(true);

            jContentPane = new JPanel();
            jContentPane.setPreferredSize(new Dimension(680, 600));
            jContentPane.setLayout(null);
            jContentPane.add(jLabelHdr, null);
            jContentPane.add(starLabel1, null);
            jContentPane.add(starLabel3, null);
            jContentPane.add(getJTextFieldAddClass(), null);
            jContentPane.add(getJComboBoxSelect(), null);
            jContentPane.add(getJScrollPaneTable(), null);
            jContentPane.add(getJButtonAdd(), null);
            jContentPane.add(getJButtonRemove(), null);
            jContentPane.add(getJButtonRemoveAll(), null);
            
            jContentPane.add(getJTextFieldHdr(), null);
            jContentPane.add(getJButtonBrowse(), null);
            jContentPane.add(jLabel1ClassName, null);
            jContentPane.add(starLabel2, null);
            jContentPane.add(jLabel2HelpText, null);
            jContentPane.add(getJHelpTextScrollPane(), null);

            jContentPane.add(jLabel5SupArchList, null);
            jContentPane.add(jLabel6SupModList, null);
            
            jContentPane.add(getJScrollPaneModules(), null);
            jContentPane.add(getJScrollPane1Arch(), null);
            
        }
        
        return jContentPane;
    }


    /* (non-Javadoc)
     * @see java.awt.event.ActionListener#actionPerformed(java.awt.event.ActionEvent)
     */
    public void actionPerformed(ActionEvent arg0) {
        
        if (arg0.getSource() == jButtonAdd) {
            
            //ToDo: check before add
            // LAH WAS String[] row = {null, null, null, jComboBox.getSelectedItem()+"", jTextField2RecInstVer.getText(), null, null};
            String[] row = {null, null, null, null, null, null, null};
            row[cnClassName] = jTextFieldAddClass.getText();
            row[cnHdrFile] = jTextFieldHdr.getText().replace('\\', '/');
            row[cnHelpText] = jTextAreaHelp.getText();
//            row[cnRecInstName] = jComboBox.getSelectedItem()+"";
//            row[cnRecInstVer] = jTextField2RecInstVer.getText();
            row[cnSupArch] = vectorToString(iCheckBoxListArch.getAllCheckedItemsString());
            if (row[cnSupArch].length() == 0) {
                row[cnSupArch] = null;
            }
            row[cnSupMod] = vectorToString(iCheckBoxListModules.getAllCheckedItemsString());
            if (row[cnSupMod].length() == 0){
                row[cnSupMod] = null;
            }
            if (!dataValidation(row)) {
                return;
            }
            //
            //convert to GUID before storing recommended lib instance.
            //
//            getLibInstances(row[cnClassName]);
//            String recommendGuid = nameToGuid(row[cnRecInstName]);
//            if (row[cnRecInstName].equals("null")) {
//                row[cnRecInstName] = null;
//            }
//            else{
//                if (recommendGuid == null) {
//                  JOptionPane.showMessageDialog(frame, "Recommended Instance does not exist.");
//                  return;
//                }
//            }

            sfc.genSpdLibClassDeclarations(row[cnClassName], null, row[cnHdrFile], row[cnHelpText], row[cnSupArch], null, null, null, null, row[cnSupMod]);
            model.addRow(row);
            jTable.changeSelection(model.getRowCount()-1, 0, false, false);
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
                docConsole.setSaved(false);
                sfc.removeSpdLibClass(rowSelected);
            }
        }

        if (arg0.getSource() == jButtonRemoveAll) {
            if (model.getRowCount() == 0) {
                return;
            }
            docConsole.setSaved(false);
            model.setRowCount(0);
            sfc.removeSpdLibClass();
        }
    }

    private boolean dataValidation(String[] row) {
        if (!DataValidation.isKeywordType(row[cnClassName])) {
            JOptionPane.showMessageDialog(this, "Library Class name entered does not match KeyWord datatype.");
            return false;
        }
        if (!DataValidation.isPathAndFilename(row[cnHdrFile])) {
            JOptionPane.showMessageDialog(this, "Include Header does not match the PathAndFilename datatype.");
            return false;
        }
        if (row[cnHelpText].length() == 0) {
            JOptionPane.showMessageDialog(this, "Help Text must be entered!");
            return false;
        }
//        if (row[cnRecInstVer] != null && row[cnRecInstVer].length() > 0) {
//            if (row[cnRecInstName] == null || row[cnRecInstName].length() == 0) {
//                JOptionPane.showMessageDialog(frame, "Recommended Instance Version must associate with the Instance Name.");
//                return false;
//            }
//            
//            if (!DataValidation.isVersionDataType(row[cnRecInstVer])) {
//                JOptionPane.showMessageDialog(frame, "Recommended Instance Version does not match Version datatype.");
//                return false;
//            }
//        }
        return true;
    }
    /**
     Add contents in list to sfc
    **/
    protected void save() {
        
    }

    /**
      This method initializes jTextField	
      	
      @return javax.swing.JTextField	
     **/
    private JTextField getJTextFieldHdr() {
        if (jTextFieldHdr == null) {
            jTextFieldHdr = new JTextField();
            jTextFieldHdr.setPreferredSize(new java.awt.Dimension(shortWidth,20));
            jTextFieldHdr.setLocation(new java.awt.Point(longValueCol,rowThree));
            jTextFieldHdr.setSize(new java.awt.Dimension(shortWidth,20));
        }
        return jTextFieldHdr;
    }

    /**
      This method initializes jButtonBrowse	
      	
      @return javax.swing.JButton	
     **/
    private JButton getJButtonBrowse() {
        if (jButtonBrowse == null) {
            jButtonBrowse = new JButton();
            jButtonBrowse.setBounds(new java.awt.Rectangle(longValueCol + shortWidth + 7,rowThree,90,20));
            jButtonBrowse.setText("Browse");
            jButtonBrowse.setPreferredSize(new java.awt.Dimension(99,20));
            jButtonBrowse.addActionListener(new AbstractAction() {
                
                /**
                 * 
                 */
                private static final long serialVersionUID = 1L;

                public void actionPerformed(ActionEvent arg0) {
                    //
                    // Select files from current pkg
                    //
                    String dirPrefix = Tools.dirForNewSpd.substring(0, Tools.dirForNewSpd.lastIndexOf(File.separator));
                    JFileChooser chooser = new JFileChooser(dirPrefix);
                    File theFile = null;
                    String headerDest = null;
                    
                    chooser.setMultiSelectionEnabled(false);
                    chooser.setFileSelectionMode(JFileChooser.FILES_AND_DIRECTORIES);
                    int retval = chooser.showOpenDialog(topFrame);
                    if (retval == JFileChooser.APPROVE_OPTION) {

                        theFile = chooser.getSelectedFile();
                        String file = theFile.getPath();
                        if (!file.startsWith(dirPrefix)) {
                            JOptionPane.showMessageDialog(topFrame, "You can only select files in current package directory structure!");
                            return;
                        }
                        
                        
                    }
                    else {
                        return;
                    }
                    
                    headerDest = theFile.getPath();
                    int fileIndex = headerDest.indexOf(System.getProperty("file.separator"), dirPrefix.length());
                    jTextFieldHdr.setText(headerDest.substring(fileIndex + 1).replace('\\', '/'));
               
                }

            });
        }
        return jButtonBrowse;
    }
    
    public void componentResized(ComponentEvent arg0) {
        int intPreferredWidth = 500;
        
        Tools.resizeComponentWidth(this.jTextFieldAddClass, this.getWidth(), intPreferredWidth-28);
        Tools.resizeComponentWidth(this.jHelpTextScrollPane, this.getWidth(), intPreferredWidth-28);
        Tools.resizeComponentWidth(this.jScrollPaneTable, this.getWidth(), intPreferredWidth-10);
        Tools.resizeComponentWidth(this.jTextFieldHdr, this.getWidth(), intPreferredWidth - 7);
        Tools.relocateComponentX(this.jButtonBrowse, this.getWidth(), intPreferredWidth,
                                  DataType.SPACE_TO_RIGHT_FOR_GENERATE_BUTTON); 
    }

    /**
     * This method initializes jHelpTextScrollPane
     *
     * @return javax.swing.JScrollPane jHelpTextScrollPane
     */
    private JScrollPane getJHelpTextScrollPane() {
        if (jHelpTextScrollPane == null) {
            jHelpTextScrollPane = new JScrollPane();
            jHelpTextScrollPane.setHorizontalScrollBarPolicy(javax.swing.JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);
            jHelpTextScrollPane.setPreferredSize(new java.awt.Dimension(longValueWidth, 80));
            jHelpTextScrollPane.setSize(new java.awt.Dimension(longValueWidth, 80));
            jHelpTextScrollPane.setLocation(new java.awt.Point(shortValueCol,rowTwo));
            jHelpTextScrollPane.setViewportView(getJTextAreaHelp());
        }
        return jHelpTextScrollPane;
    }

    /**
     * This method initializes jTextAreaHelp	
     * 	
     * @return javax.swing.JTextArea	
     */
    private JTextArea getJTextAreaHelp() {
        if (jTextAreaHelp == null) {
            jTextAreaHelp = new JTextArea();
            jTextAreaHelp.setLineWrap(true);
            jTextAreaHelp.setWrapStyleWord(true);
        }
        return jTextAreaHelp;
    }

    private JScrollPane getJScrollPaneModules() {
        if (jScrollPaneModules == null) {
            jScrollPaneModules = new JScrollPane();
            jScrollPaneModules.setBounds(new java.awt.Rectangle(longValueCol,rowFive,medWidth,60));
            jScrollPaneModules.setPreferredSize(new java.awt.Dimension(medWidth, 60));
            jScrollPaneModules.setViewportView(getICheckBoxListSupportedModules());
        }
        return jScrollPaneModules;
    }
    
    private ICheckBoxList getICheckBoxListSupportedModules() {
        if (iCheckBoxListModules == null) {
            iCheckBoxListModules = new ICheckBoxList();
            iCheckBoxListModules.setBounds(new java.awt.Rectangle(longValueCol,rowFour,medWidth,60));
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
            iCheckBoxListModules.setAllItems(v);
        }
        return iCheckBoxListModules;
    }
    
    private String vectorToString(Vector<String> v) {
        String s = " ";
        for (int i = 0; i < v.size(); ++i) {
            s += v.get(i);
            s += " ";
        }
        return s.trim();
    }
    
    private JScrollPane getJScrollPane1Arch() {
        if (jScrollPane1Arch == null) {
            jScrollPane1Arch = new JScrollPane();
            jScrollPane1Arch.setBounds(new java.awt.Rectangle(longValueCol,rowFour,medWidth,60));
            jScrollPane1Arch.setPreferredSize(new java.awt.Dimension(medWidth, 60));
            jScrollPane1Arch.setViewportView(getICheckBoxListArch());
        }
        return jScrollPane1Arch;
    }
    /**
     * This method initializes iCheckBoxList	
     * 	
     * @return org.tianocore.frameworkwizard.common.ui.iCheckBoxList.ICheckBoxList	
     */
    private ICheckBoxList getICheckBoxListArch() {
        if (iCheckBoxListArch == null) {
            iCheckBoxListArch = new ICheckBoxList();
            iCheckBoxListArch.setBounds(new java.awt.Rectangle(longValueCol,rowFour,medWidth,60));
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

//    private void getLibInstances(String libClass){
//        libNameGuidMap.clear();
//        try {
//                Iterator ismi = GlobalData.vModuleList.iterator();
//                while (ismi.hasNext()) {
//                    ModuleIdentification mi = (ModuleIdentification) ismi.next();
//                    
//                    Vector<String> classProduced = SurfaceAreaQuery.getLibraryClasses("ALWAYS_PRODUCED", mi);
//                    for (int i = 0; i < classProduced.size(); ++i) {
//                        if (classProduced.get(i).equals(libClass)) {
//                            libNameGuidMap.put(mi.getName(), mi.getGuid());
//                        }
//                    }
//                }
//           
//        }
//        catch(Exception e){
//            JOptionPane.showMessageDialog(frame, "Search Instances Failed.");
//        }
//        
//    }

//    private String nameToGuid(String name) {
//        String s = null;
//        if (!libNameGuidMap.containsKey(name)) {
//            return s;
//        }
//        
//        s = libNameGuidMap.get(name);
//        return s;
//    }
    
//    private String guidToName(String guid){
//        String s = "";
//        if (!libNameGuidMap.containsValue(guid)) {
//            return s;
//        }
//        Set<String> key = libNameGuidMap.keySet();
//        Iterator<String> is = key.iterator();
//        while(is.hasNext()) {
//            s = is.next();
//            if (libNameGuidMap.get(s).equals(guid)) {
//                break;
//            }
//        }
//        return s;
//    }

}


