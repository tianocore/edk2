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
import java.awt.event.ActionEvent;
import java.awt.event.ComponentEvent;
import java.io.File;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;
import java.util.Vector;

import javax.swing.AbstractAction;
import javax.swing.JFileChooser;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JTable;
import javax.swing.JTextField;
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
import javax.swing.table.TableModel;

import org.apache.xmlbeans.XmlObject;
import org.tianocore.PackageSurfaceAreaDocument;
import org.tianocore.frameworkwizard.common.DataValidation;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.Identifications.OpeningPackageType;
import org.tianocore.frameworkwizard.common.ui.IInternalFrame;
import org.tianocore.frameworkwizard.common.ui.StarLabel;
import org.tianocore.frameworkwizard.common.ui.iCheckBoxList.ICheckBoxList;
import org.tianocore.frameworkwizard.platform.ui.ListEditor;
import org.tianocore.frameworkwizard.platform.ui.global.GlobalData;
import org.tianocore.frameworkwizard.platform.ui.global.SurfaceAreaQuery;
import org.tianocore.frameworkwizard.platform.ui.id.ModuleIdentification;
import org.tianocore.frameworkwizard.platform.ui.id.PackageIdentification;


/**
 GUI for create library definition elements of spd file.
  
 @since PackageEditor 1.0
**/
public class SpdLibClassDecls extends IInternalFrame implements TableModelListener{
    /**
     * 
     */
    private static final long serialVersionUID = 1L;

    static JFrame frame;
    
    private JTable jTable = null;

    private DefaultTableModel model = null;

    private JPanel jContentPane = null;

    private JTextField jTextFieldClass = null;

    private JComboBox jComboBoxSelect = null;

    private JScrollPane jScrollPane = null;

    private JButton jButtonAdd = null;

    private JButton jButtonRemove = null;

    private JButton jButtonRemoveAll = null;

    private JLabel jLabelHdr = null;

    private JTextField jTextFieldHdr = null;

    private JButton jButtonBrowse = null;
    
    private StarLabel jStarLabel1 = null;
    
    private StarLabel jStarLabel2 = null;
    
    private SpdFileContents sfc = null;
    
    private OpeningPackageType docConsole = null;

    private JLabel jLabel1ClassName = null;
    
    private JScrollPane topScrollPane = null;  //  @jve:decl-index=0:visual-constraint="10,53"
    
    private int selectedRow = -1;

    private StarLabel starLabel = null;

    private JLabel jLabel2HelpText = null;

    private JTextField jTextFieldHelp = null;

    private JLabel jLabel3RecInstName = null;

    private JTextField jTextField1RecInstName = null;

    private JLabel jLabel4RecInstVer = null;

    private JTextField jTextField2RecInstVer = null;

    private JLabel jLabel5SupArchList = null;

    private JLabel jLabel6SupModList = null;
    
    private JScrollPane jScrollPaneModules = null;
    
    private JScrollPane jScrollPane1Arch = null;
    
    private ICheckBoxList iCheckBoxListModules = null;

    private ICheckBoxList iCheckBoxListArch = null;

    private JComboBox jComboBox = null;

    private int cnClassName = 0;
    private int cnHdrFile = 1;
    private int cnHelpText = 2;
    private int cnRecInstName = 3;
    private int cnRecInstVer = 4;
    private int cnSupArch = 5;
    private int cnSupMod = 6;
    
    HashMap<String, String> libNameGuidMap = new HashMap<String, String>();


    /**
      This method initializes this
     
     **/
    private void initialize() {
        
        this.setDefaultCloseOperation(javax.swing.WindowConstants.DISPOSE_ON_CLOSE);

    }

    /**
      This method initializes jTextFieldAdd	
      	
      @return javax.swing.JTextField	
     **/
    private JTextField getJTextFieldClass() {
        if (jTextFieldClass == null) {
            jTextFieldClass = new JTextField();
            jTextFieldClass.setBounds(new java.awt.Rectangle(122,6,390,20));
            jTextFieldClass.setPreferredSize(new java.awt.Dimension(260,20));
            jTextFieldClass.setEnabled(true);
        }
        return jTextFieldClass;
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
      This method initializes jScrollPane	
      	
      @return javax.swing.JScrollPane	

      Used for the Table of Library Classes that are provided by this package

     **/
    private JScrollPane getJScrollPane() {
        if (jScrollPane == null) {
            jScrollPane = new JScrollPane();
            jScrollPane.setBounds(new java.awt.Rectangle(12,351,608,253));
            jScrollPane.setPreferredSize(new java.awt.Dimension(500,419));
            jScrollPane.setViewportView(getJTable());
        }
        return jScrollPane;
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
           model.addColumn("Sup. Arch");
           model.addColumn("Mod. Types");
           
           Vector<String> vArch = new Vector<String>();
           vArch.add("IA32");
           vArch.add("X64");
           vArch.add("IPF");
           vArch.add("EBC");
           vArch.add("ARM");
           vArch.add("PPC");
           jTable.getColumnModel().getColumn(cnSupArch).setCellEditor(new ListEditor(vArch));
           
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

           jTable.getColumnModel().getColumn(cnSupMod).setCellEditor(new ListEditor(vModule));
          
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
            if (m.getValueAt(row, cnRecInstName) != null) {
                name = m.getValueAt(row, cnRecInstName).toString();
            } 
            String ver = null;
            if (m.getValueAt(row, cnRecInstVer) != null){
                ver = m.getValueAt(row, cnRecInstVer).toString();
            }
            String arch = null;
            if (m.getValueAt(row, cnSupArch) != null) {
               arch = m.getValueAt(row, cnSupArch).toString();
            }
            String module = null;
            if (m.getValueAt(row, cnSupMod) != null) {
                module = m.getValueAt(row, cnSupMod).toString();
            }
            String[] rowData = {lib, hdr, hlp, name, ver};
            if (!dataValidation(rowData)) {
                return;
            }
            
            String guid = null;
            if (name != null && name.length() > 0) {
                getLibInstances(lib);
                guid = nameToGuid(name);
                if (guid == null){
                  JOptionPane.showMessageDialog(frame, "Recommended Instance NOT exists.");
                  return;
                }
            }
            
            String[] sa = new String[7];
            sfc.getSpdLibClassDeclaration(sa, row);
            Object cellData = m.getValueAt(row, column);
            if (cellData == null) {
                cellData = "";
            }
            if (column == cnRecInstName) {
                if (guid == null) {
                    if (sa[cnRecInstName] == null) {
                        return;
                    }
                }
                else {
                    if (guid.equals(sa[cnRecInstName])) {
                        return;
                    }
                }
            }
            else {
                if (cellData.equals(sa[column])) {
                    return;
                }
                if (cellData.toString().length() == 0 && sa[column] == null) {
                    return;
                }
            }
            docConsole.setSaved(false);
            sfc.updateSpdLibClass(row, lib, hdr, hlp, guid, ver, arch, module);
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
            jButtonAdd.setSize(new java.awt.Dimension(99,20));
            jButtonAdd.setBounds(new java.awt.Rectangle(321,326,99,20));
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
            jButtonRemove.setText("Remove");
            jButtonRemove.setSize(new java.awt.Dimension(99,20));
            jButtonRemove.setBounds(new java.awt.Rectangle(424,326,99,20));
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
            jButtonRemoveAll.setText("Remove All");
            jButtonRemoveAll.setSize(new java.awt.Dimension(99,20));
            jButtonRemoveAll.setBounds(new java.awt.Rectangle(527,326,99,20));
            jButtonRemoveAll.addActionListener(this);
        }
        return jButtonRemoveAll;
    }

    /**
      This is the default constructor
     **/
    public SpdLibClassDecls() {
        super();
        initialize();
        init();
        
    }

    public SpdLibClassDecls(PackageSurfaceAreaDocument.PackageSurfaceArea inPsa){
        this();
        sfc = new SpdFileContents(inPsa);
        init(sfc);
    }
    
    public SpdLibClassDecls(OpeningPackageType opt) {
        this(opt.getXmlSpd());
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
        initFrame();
    }

    private void init(SpdFileContents sfc) {
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
            if (saa[i][3] != null && saa[i][3].length() > 0) {
                getLibInstances(saa[i][0]);
                saa[i][3] = guidToName(saa[i][3]);
            }
            
            model.addRow(saa[i]);
            i++;
        }
    }
    private JScrollPane getJContentPane(){
        if (topScrollPane == null){
          topScrollPane = new JScrollPane();
//          topScrollPane.setSize(new java.awt.Dimension(634,590));
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
            jStarLabel1 = new StarLabel();
            jStarLabel1.setLocation(new java.awt.Point(1,7));
            jLabel1ClassName = new JLabel();
            jLabel1ClassName.setBounds(new java.awt.Rectangle(16,6,82,20));
            jLabel1ClassName.setText("Library Class");

            // Help Text
            starLabel = new StarLabel();
            starLabel.setBounds(new java.awt.Rectangle(1,33,10,20));
            jLabel2HelpText = new JLabel();
            jLabel2HelpText.setBounds(new java.awt.Rectangle(16,33,82,20));
            jLabel2HelpText.setText("Help Text");

            // Header File
            jStarLabel2 = new StarLabel();
            jStarLabel2.setLocation(new java.awt.Point(1,74));
            jLabelHdr = new JLabel();
            jLabelHdr.setBounds(new java.awt.Rectangle(14,74,199,22));
            jLabelHdr.setText("Include Header for Specified Class");

            jLabel6SupModList = new JLabel();
            jLabel6SupModList.setBounds(new java.awt.Rectangle(16,252,108,16));
            jLabel6SupModList.setText("Supported Module");
            jLabel6SupModList.setEnabled(true);

            jLabel5SupArchList = new JLabel();
            jLabel5SupArchList.setBounds(new java.awt.Rectangle(15,169,93,16));
            jLabel5SupArchList.setText("Supported Arch");
            jLabel5SupArchList.setEnabled(true);
            jLabel4RecInstVer = new JLabel();
            jLabel4RecInstVer.setBounds(new java.awt.Rectangle(16,138,196,16));
            jLabel4RecInstVer.setEnabled(true);
            jLabel4RecInstVer.setText("Recommended Instance Version");
            jLabel3RecInstName = new JLabel();
            jLabel3RecInstName.setBounds(new java.awt.Rectangle(17,112,195,16));
            jLabel3RecInstName.setEnabled(true);
            jLabel3RecInstName.setText("Recommended Instance Name");
            
            jContentPane = new JPanel();
            jContentPane.setPreferredSize(new Dimension(680, 600));
            jContentPane.setLayout(null);
            jContentPane.add(jLabelHdr, null);
            jContentPane.add(jStarLabel1, null);
            jContentPane.add(jStarLabel2, null);
            jContentPane.add(getJTextFieldClass(), null);
            jContentPane.add(getJComboBoxSelect(), null);
            jContentPane.add(getJScrollPane(), null);
            jContentPane.add(getJButtonAdd(), null);
            jContentPane.add(getJButtonRemove(), null);
            jContentPane.add(getJButtonRemoveAll(), null);
            
            jContentPane.add(getJTextFieldHdr(), null);
            jContentPane.add(getJButtonBrowse(), null);
            jContentPane.add(jLabel1ClassName, null);
            jContentPane.add(starLabel, null);
            jContentPane.add(jLabel2HelpText, null);
            jContentPane.add(getJTextFieldHelp(), null);

            jContentPane.add(jLabel3RecInstName, null);
            jContentPane.add(getJTextField1RecInstName(), null);
            jContentPane.add(jLabel4RecInstVer, null);
            jContentPane.add(getJTextField2RecInstVer(), null);
            jContentPane.add(jLabel5SupArchList, null);
            jContentPane.add(jLabel6SupModList, null);
            
            jContentPane.add(getJScrollPaneModules(), null);
            jContentPane.add(getJScrollPane1Arch(), null);
            jContentPane.add(getJComboBox(), null);
            
        }
        
        return jContentPane;
    }

    /**
     fill ComboBoxes with pre-defined contents
    **/
    private void initFrame() {
        jComboBoxSelect.addItem("BaseCpuICacheFlush");
        jComboBoxSelect.addItem("BaseDebugLibNull");
        jComboBoxSelect.addItem("BaseDebugLibReportStatusCode");
        jComboBoxSelect.addItem("BaseIoLibIntrinsic");
        jComboBoxSelect.addItem("BaseLib");
        jComboBoxSelect.addItem("BaseMemoryLib");
        jComboBoxSelect.addItem("BaseMemoryLibMmx");
        jComboBoxSelect.addItem("BaseMemoryLibSse2");
        jComboBoxSelect.addItem("BasePeCoffGetEntryPointLib");
        jComboBoxSelect.addItem("BasePeCoffLib");
        jComboBoxSelect.addItem("BasePrintLib");
        jComboBoxSelect.addItem("BaseReportStatusCodeLibNull");
        jComboBoxSelect.addItem("CommonPciCf8Lib");
        jComboBoxSelect.addItem("CommonPciExpressLib");
        jComboBoxSelect.addItem("CommonPciLibCf8");
        jComboBoxSelect.addItem("CommonPciLibPciExpress");
        jComboBoxSelect.addItem("DxeCoreEntryPoint");
        jComboBoxSelect.addItem("DxeHobLib");
        jComboBoxSelect.addItem("DxeIoLibCpuIo");
        jComboBoxSelect.addItem("DxeLib");
        jComboBoxSelect.addItem("DxePcdLib");
        jComboBoxSelect.addItem("DxeReportStatusCodeLib");
        jComboBoxSelect.addItem("DxeServicesTableLib");
        jComboBoxSelect.addItem("PeiCoreEntryPoint");
        jComboBoxSelect.addItem("PeiMemoryLib");
        jComboBoxSelect.addItem("PeimEntryPoint");
        jComboBoxSelect.addItem("PeiReportStatusCodeLib");
        jComboBoxSelect.addItem("PeiServicesTablePointerLib");
        jComboBoxSelect.addItem("PeiServicesTablePointerLibMm7");
        jComboBoxSelect.addItem("UefiDebugLibConOut");
        jComboBoxSelect.addItem("UefiDebugLibStdErr");
        jComboBoxSelect.addItem("UefiDriverEntryPointMultiple");
        jComboBoxSelect.addItem("UefiDriverEntryPointSingle");
        jComboBoxSelect.addItem("UefiDriverEntryPointSingleUnload");
        jComboBoxSelect.addItem("UefiDriverModelLib");
        jComboBoxSelect.addItem("UefiDriverModelLibNoConfigNoDiag");
        jComboBoxSelect.addItem("UefiLib");
        jComboBoxSelect.addItem("UefiMemoryLib");

    }

    /* (non-Javadoc)
     * @see java.awt.event.ActionListener#actionPerformed(java.awt.event.ActionEvent)
     */
    public void actionPerformed(ActionEvent arg0) {
        
        if (arg0.getSource() == jButtonAdd) {
            
            //ToDo: check before add
            // LAH WAS String[] row = {null, null, null, jComboBox.getSelectedItem()+"", jTextField2RecInstVer.getText(), null, null};
            String[] row = {null, null, null, null, null, null, null};
            row[cnClassName] = jTextFieldClass.getText();
            row[cnHdrFile] = jTextFieldHdr.getText().replace('\\', '/');
            row[cnHelpText] = jTextFieldHelp.getText();
            row[cnRecInstName] = jComboBox.getSelectedItem()+"";
            row[cnRecInstVer] = jTextField2RecInstVer.getText();
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
            getLibInstances(row[cnClassName]);
            String recommendGuid = nameToGuid(row[cnRecInstName]);
            if (row[cnRecInstName].equals("null")) {
                row[cnRecInstName] = null;
            }
            else{
                if (recommendGuid == null) {
                  JOptionPane.showMessageDialog(frame, "Recommended Instance NOT exists.");
                  return;
                }
            }

            sfc.genSpdLibClassDeclarations(row[cnClassName], recommendGuid, row[cnHdrFile], row[cnHelpText], row[cnSupArch], null, null, row[cnRecInstVer], null, row[cnSupMod]);
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
            JOptionPane.showMessageDialog(frame, "Library Class is NOT KeyWord Type.");
            return false;
        }
        if (!DataValidation.isPathAndFilename(row[cnHdrFile])) {
            JOptionPane.showMessageDialog(frame, "Include Header is NOT PathAndFilename Type.");
            return false;
        }
        if (row[cnHelpText].length() == 0) {
            JOptionPane.showMessageDialog(frame, "Help Text Must NOT be empty.");
            return false;
        }
        if (row[cnRecInstVer] != null && row[cnRecInstVer].length() > 0) {
            if (row[cnRecInstName] == null || row[cnRecInstName].length() == 0) {
                JOptionPane.showMessageDialog(frame, "Recommended Instance Version must associate with Instance Name.");
                return false;
            }
            
            if (!DataValidation.isVersionDataType(row[cnRecInstVer])) {
                JOptionPane.showMessageDialog(frame, "Recommended Instance Version is NOT VersionDataType.");
                return false;
            }
        }
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
            jTextFieldHdr.setBounds(new java.awt.Rectangle(218,75,305,21));
            jTextFieldHdr.setPreferredSize(new java.awt.Dimension(260,20));
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
            jButtonBrowse.setBounds(new java.awt.Rectangle(527,75,90,20));
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
                    int retval = chooser.showOpenDialog(frame);
                    if (retval == JFileChooser.APPROVE_OPTION) {

                        theFile = chooser.getSelectedFile();
                        String file = theFile.getPath();
                        if (!file.startsWith(dirPrefix)) {
                            JOptionPane.showMessageDialog(frame, "You can only select files in current package!");
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
        
        resizeComponentWidth(this.jTextFieldClass, this.getWidth(), intPreferredWidth);
        resizeComponentWidth(this.jTextFieldHelp, this.getWidth(), intPreferredWidth);
        resizeComponentWidth(this.jScrollPane, this.getWidth(), intPreferredWidth-10);
        
    }
    /**
     * This method initializes jTextFieldHelp	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldHelp() {
        if (jTextFieldHelp == null) {
            jTextFieldHelp = new JTextField();
            jTextFieldHelp.setBounds(new java.awt.Rectangle(122,33,390,20));
            jTextFieldHelp.setPreferredSize(new java.awt.Dimension(260,20));
        }
        return jTextFieldHelp;
    }

    /**
     * This method initializes jTextField1RecInstName	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextField1RecInstName() {
        if (jTextField1RecInstName == null) {
            jTextField1RecInstName = new JTextField();
            jTextField1RecInstName.setBounds(new java.awt.Rectangle(218,110,291,20));
            jTextField1RecInstName.setEnabled(true);
            jTextField1RecInstName.setVisible(false);
        }
        return jTextField1RecInstName;
    }

    /**
     * This method initializes jTextField2RecInstVer	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextField2RecInstVer() {
        if (jTextField2RecInstVer == null) {
            jTextField2RecInstVer = new JTextField();
            jTextField2RecInstVer.setBounds(new java.awt.Rectangle(218,135,292,20));
            jTextField2RecInstVer.setEnabled(true);
        }
        return jTextField2RecInstVer;
    }

    private JScrollPane getJScrollPaneModules() {
        if (jScrollPaneModules == null) {
            jScrollPaneModules = new JScrollPane();
            jScrollPaneModules.setBounds(new java.awt.Rectangle(218,245,293,73));
            jScrollPaneModules.setPreferredSize(new java.awt.Dimension(320, 80));
            jScrollPaneModules.setViewportView(getICheckBoxListSupportedModules());
        }
        return jScrollPaneModules;
    }
    
    private ICheckBoxList getICheckBoxListSupportedModules() {
        if (iCheckBoxListModules == null) {
            iCheckBoxListModules = new ICheckBoxList();
            iCheckBoxListModules.setBounds(new java.awt.Rectangle(218,246,292,73));
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
            jScrollPane1Arch.setBounds(new java.awt.Rectangle(218,170,293,73));
            jScrollPane1Arch.setPreferredSize(new java.awt.Dimension(320, 80));
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
            iCheckBoxListArch.setBounds(new java.awt.Rectangle(218,171,292,66));
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
     * This method initializes jComboBox	
     * 	
     * @return javax.swing.JComboBox	
     */
    private JComboBox getJComboBox() {
        if (jComboBox == null) {
            jComboBox = new JComboBox();
            jComboBox.setPreferredSize(new java.awt.Dimension(31,20));
            jComboBox.setSize(new java.awt.Dimension(290,20));
            jComboBox.setLocation(new java.awt.Point(218,111));
            jComboBox.addFocusListener(new java.awt.event.FocusAdapter() {
                public void focusGained(java.awt.event.FocusEvent e) {
                    if (jTextFieldClass.getText().length() == 0) {
                        return;
                    }
                    jComboBox.removeAllItems();
                    getLibInstances(jTextFieldClass.getText());
                    Set<String> libNames = libNameGuidMap.keySet();
                    Iterator<String> si = libNames.iterator();
                    while(si.hasNext()) {
                        jComboBox.addItem(si.next());
                    }
                }
            });
         
        }
        return jComboBox;
    }

    private void getLibInstances(String libClass){
        libNameGuidMap.clear();
        try {
            GlobalData.initInfo("Tools" + File.separator + "Conf" + File.separator + "FrameworkDatabase.db", System.getenv("WORKSPACE"));
        
            Set<PackageIdentification> spi = GlobalData.getPackageList();
            Iterator ispi = spi.iterator();
            
            while (ispi.hasNext()) {
                PackageIdentification pi = (PackageIdentification) ispi.next();

                Set<ModuleIdentification> smi = GlobalData.getModules(pi);
                Iterator ismi = smi.iterator();
                while (ismi.hasNext()) {
                    ModuleIdentification mi = (ModuleIdentification) ismi.next();
                    Map<String, XmlObject> m = GlobalData.getNativeMsa(mi);
                    SurfaceAreaQuery.setDoc(m);
                    String[] classProduced = SurfaceAreaQuery.getLibraryClasses("ALWAYS_PRODUCED");
                    for (int i = 0; i < classProduced.length; ++i) {
                        if (classProduced[i].equals(libClass)) {
                            libNameGuidMap.put(mi.getName(), mi.getGuid());
                        }
                    }
                }
            }
        }
        catch(Exception e){
            JOptionPane.showMessageDialog(frame, "Search Instances Fail.");
        }
        
    }

    private String nameToGuid(String name) {
        String s = null;
        if (!libNameGuidMap.containsKey(name)) {
            return s;
        }
        
        s = libNameGuidMap.get(name);
        return s;
    }
    
    private String guidToName(String guid){
        String s = "";
        if (!libNameGuidMap.containsValue(guid)) {
            return s;
        }
        Set<String> key = libNameGuidMap.keySet();
        Iterator<String> is = key.iterator();
        while(is.hasNext()) {
            s = is.next();
            if (libNameGuidMap.get(s).equals(guid)) {
                break;
            }
        }
        return s;
    }

}


