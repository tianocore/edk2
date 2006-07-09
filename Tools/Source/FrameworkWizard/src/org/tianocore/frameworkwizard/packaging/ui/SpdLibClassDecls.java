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
import java.util.Vector;

import javax.swing.AbstractAction;
import javax.swing.JFileChooser;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JRadioButton;
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

import org.tianocore.PackageSurfaceAreaDocument;
import org.tianocore.frameworkwizard.common.DataValidation;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.Identifications.OpeningPackageType;
import org.tianocore.frameworkwizard.common.ui.IInternalFrame;
import org.tianocore.frameworkwizard.common.ui.StarLabel;
import org.tianocore.frameworkwizard.common.ui.iCheckBoxList.ICheckBoxList;
import org.tianocore.frameworkwizard.platform.ui.ListEditor;

import javax.swing.JCheckBox;

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

    private JRadioButton jRadioButtonAdd = null;

    private JRadioButton jRadioButtonSelect = null;

    private JTextField jTextFieldAdd = null;

    private JComboBox jComboBoxSelect = null;

    private JScrollPane jScrollPane = null;

    private JButton jButtonAdd = null;

    private JButton jButtonRemove = null;

    private JButton jButtonClearAll = null;

    private JLabel jLabel = null;

    private JTextField jTextField = null;

    private JButton jButtonBrowse = null;
    
    private StarLabel jStarLabel1 = null;
    
    private StarLabel jStarLabel2 = null;
    
    private SpdFileContents sfc = null;
    
    private OpeningPackageType docConsole = null;

    private JLabel jLabel1 = null;
    
    private JScrollPane topScrollPane = null;  //  @jve:decl-index=0:visual-constraint="10,53"
    
    private int selectedRow = -1;

    private StarLabel starLabel = null;

    private JLabel jLabel2 = null;

    private JTextField jTextFieldHelp = null;

    private JLabel jLabel3 = null;

    private JTextField jTextField1 = null;

    private JLabel jLabel4 = null;

    private JTextField jTextField2 = null;

    private JLabel jLabel5 = null;

    private JCheckBox jCheckBox = null;

    private JCheckBox jCheckBox1 = null;

    private JCheckBox jCheckBox2 = null;

    private JCheckBox jCheckBox3 = null;

    private JLabel jLabel6 = null;
    
    private JScrollPane jScrollPaneArch = null;
    
    private ICheckBoxList iCheckBoxListArch = null;

    private JCheckBox jCheckBox4 = null;

    private JCheckBox jCheckBox5 = null;

    /**
      This method initializes this
     
     **/
    private void initialize() {
        
        this.setDefaultCloseOperation(javax.swing.WindowConstants.DISPOSE_ON_CLOSE);

    }

    /**
      This method initializes jRadioButtonAdd	
      	
      @return javax.swing.JRadioButton	
     **/
    private JRadioButton getJRadioButtonAdd() {
        if (jRadioButtonAdd == null) {
            jRadioButtonAdd = new JRadioButton();
            jRadioButtonAdd.setBounds(new java.awt.Rectangle(9,63,197,20));
            jRadioButtonAdd.setText("Library Class Name");
            jRadioButtonAdd.addActionListener(this);
            jRadioButtonAdd.setSelected(true);
            jRadioButtonAdd.setVisible(false);
        }
        return jRadioButtonAdd;
    }

    /**
      This method initializes jRadioButtonSelect	
      	
      @return javax.swing.JRadioButton	
     **/
    private JRadioButton getJRadioButtonSelect() {
        if (jRadioButtonSelect == null) {
            jRadioButtonSelect = new JRadioButton();
            jRadioButtonSelect.setBounds(new java.awt.Rectangle(9,10,198,20));
            jRadioButtonSelect.setText("Select Existing Library Class");
            jRadioButtonSelect.addActionListener(this);
            jRadioButtonSelect.setSelected(true);
            jRadioButtonSelect.setVisible(false);
        }
        return jRadioButtonSelect;
    }

    /**
      This method initializes jTextFieldAdd	
      	
      @return javax.swing.JTextField	
     **/
    private JTextField getJTextFieldAdd() {
        if (jTextFieldAdd == null) {
            jTextFieldAdd = new JTextField();
            jTextFieldAdd.setBounds(new java.awt.Rectangle(122,6,390,20));
            jTextFieldAdd.setPreferredSize(new java.awt.Dimension(260,20));
            jTextFieldAdd.setEnabled(true);
        }
        return jTextFieldAdd;
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
     **/
    private JScrollPane getJScrollPane() {
        if (jScrollPane == null) {
            jScrollPane = new JScrollPane();
            jScrollPane.setBounds(new java.awt.Rectangle(12,351,608,139));
            jScrollPane.setPreferredSize(new java.awt.Dimension(330,150));
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
           model.addColumn("LibraryClass");
           model.addColumn("IncludeHeader");
           model.addColumn("HelpText");
           model.addColumn("RecommendedInstance");
           model.addColumn("InstanceVersion");
           model.addColumn("SupportedArch");
           model.addColumn("SupportedModule");
           
           Vector<String> vArch = new Vector<String>();
           vArch.add("IA32");
           vArch.add("X64");
           vArch.add("IPF");
           vArch.add("EBC");
           vArch.add("ARM");
           vArch.add("PPC");
           jTable.getColumnModel().getColumn(5).setCellEditor(new ListEditor(vArch));
           
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
           jTable.getColumnModel().getColumn(6).setCellEditor(new ListEditor(vModule));
          
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
            String lib = m.getValueAt(row, 0) + "";
            String hdr = m.getValueAt(row, 1) + "";
            String hlp = m.getValueAt(row, 2) + "";
            String guid = m.getValueAt(row, 3) + "";
            String ver = m.getValueAt(row, 4) + "";
            String arch = null;
            if (m.getValueAt(row, 5) != null) {
               arch = m.getValueAt(row, 5).toString();
            }
            String module = null;
            if (m.getValueAt(row, 6) != null) {
                module = m.getValueAt(row, 6).toString();
            }
            String[] rowData = {lib, hdr, hlp};
            if (!dataValidation(rowData)) {
                return;
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
            jButtonAdd.setSize(new java.awt.Dimension(80,20));
            jButtonAdd.setLocation(new java.awt.Point(365,315));
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
            jButtonRemove.setSize(new java.awt.Dimension(80,20));
            jButtonRemove.setLocation(new java.awt.Point(449,315));
            jButtonRemove.addActionListener(this);
        }
        return jButtonRemove;
    }

    /**
      This method initializes jButtonRemoveAll	
      	
      @return javax.swing.JButton	
     **/
    private JButton getJButtonClearAll() {
        if (jButtonClearAll == null) {
            jButtonClearAll = new JButton();
            jButtonClearAll.setText("Clear All");
            jButtonClearAll.setSize(new java.awt.Dimension(86,20));
            jButtonClearAll.setLocation(new java.awt.Point(536,315));
            jButtonClearAll.addActionListener(this);
        }
        return jButtonClearAll;
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
            model.addRow(saa[i]);
            i++;
        }
    }
    private JScrollPane getJContentPane(){
        if (topScrollPane == null){
          topScrollPane = new JScrollPane();
          topScrollPane.setSize(new java.awt.Dimension(634,500));
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
            jLabel6 = new JLabel();
            jLabel6.setBounds(new java.awt.Rectangle(16,252,108,16));
            jLabel6.setText("Supported Module");
            jLabel6.setEnabled(true);
            jLabel5 = new JLabel();
            jLabel5.setBounds(new java.awt.Rectangle(16,215,93,16));
            jLabel5.setText("Supported Arch");
            jLabel5.setEnabled(true);
            jLabel4 = new JLabel();
            jLabel4.setBounds(new java.awt.Rectangle(16,138,196,16));
            jLabel4.setEnabled(true);
            jLabel4.setText("Recommended Instance Version");
            jLabel3 = new JLabel();
            jLabel3.setBounds(new java.awt.Rectangle(17,112,195,16));
            jLabel3.setEnabled(true);
            jLabel3.setText("Recommended Instance GUID");
            jLabel2 = new JLabel();
            jLabel2.setBounds(new java.awt.Rectangle(16,33,82,20));
            jLabel2.setText("Help Text");
            starLabel = new StarLabel();
            starLabel.setBounds(new java.awt.Rectangle(1,33,10,20));
            jLabel1 = new JLabel();
            jLabel1.setBounds(new java.awt.Rectangle(16,6,82,20));
            jLabel1.setText("Library Class");
            jStarLabel1 = new StarLabel();
            jStarLabel1.setLocation(new java.awt.Point(1,7));
            jStarLabel2 = new StarLabel();
            jStarLabel2.setLocation(new java.awt.Point(-1,74));
            jLabel = new JLabel();
            jLabel.setBounds(new java.awt.Rectangle(14,74,199,22));
            jLabel.setText("Include Header for Specified Class");
            
            jContentPane = new JPanel();
            jContentPane.setPreferredSize(new Dimension(480, 400));
            jContentPane.setLayout(null);
            jContentPane.add(jLabel, null);
            jContentPane.add(jStarLabel1, null);
            jContentPane.add(jStarLabel2, null);
            jContentPane.add(getJRadioButtonAdd(), null);
            jContentPane.add(getJRadioButtonSelect(), null);
            jContentPane.add(getJTextFieldAdd(), null);
            jContentPane.add(getJComboBoxSelect(), null);
            jContentPane.add(getJScrollPane(), null);
            jContentPane.add(getJButtonAdd(), null);
            jContentPane.add(getJButtonRemove(), null);
            jContentPane.add(getJButtonClearAll(), null);
            
            jContentPane.add(getJTextField(), null);
            jContentPane.add(getJButtonBrowse(), null);
            jContentPane.add(jLabel1, null);
            jContentPane.add(starLabel, null);
            jContentPane.add(jLabel2, null);
            jContentPane.add(getJTextFieldHelp(), null);
            jContentPane.add(jLabel3, null);
            jContentPane.add(getJTextField1(), null);
            jContentPane.add(jLabel4, null);
            jContentPane.add(getJTextField2(), null);
            jContentPane.add(jLabel5, null);
            jContentPane.add(getJCheckBox(), null);
            jContentPane.add(getJCheckBox1(), null);
            jContentPane.add(getJCheckBox2(), null);
            jContentPane.add(getJCheckBox3(), null);
            jContentPane.add(jLabel6, null);
            
            jContentPane.add(getJScrollPaneArch(), null);
            jContentPane.add(getJCheckBox4(), null);
            jContentPane.add(getJCheckBox5(), null);
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
            String[] row = {null, null, null, jTextField1.getText(), jTextField2.getText(), null, null};
            row[0] = jTextFieldAdd.getText();
            row[1] = jTextField.getText().replace('\\', '/');
            row[2] = jTextFieldHelp.getText();
            row[5] = booleanToString(jCheckBox.isSelected(), jCheckBox1.isSelected(), jCheckBox2.isSelected(), jCheckBox3.isSelected(), jCheckBox4.isSelected(), jCheckBox5.isSelected());
            if (row[5].length() == 0){
                row[5] = null;
            }
            row[6] = vectorToString(iCheckBoxListArch.getAllCheckedItemsString());
            if (row[6].length() == 0){
                row[6] = null;
            }
            if (!dataValidation(row)) {
                return;
            }
            model.addRow(row);
            docConsole.setSaved(false);
            sfc.genSpdLibClassDeclarations(row[0], row[3], row[1], row[2], row[5], null, null, row[4], null, row[6]);
            
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
                sfc.removeSpdLibClass(rowSelected);
            }
        }

        if (arg0.getSource() == jButtonClearAll) {
            if (model.getRowCount() == 0) {
                return;
            }
            
            model.setRowCount(0);
            sfc.removeSpdLibClass();
        }

        if (arg0.getSource() == jRadioButtonAdd) {
            if (jRadioButtonAdd.isSelected()) {
                jRadioButtonSelect.setSelected(false);
                jTextFieldAdd.setEnabled(true);
                jComboBoxSelect.setEnabled(false);
            }
            if (!jRadioButtonSelect.isSelected() && !jRadioButtonAdd.isSelected()) {
                jRadioButtonAdd.setSelected(true);
                jTextFieldAdd.setEnabled(true);
                jComboBoxSelect.setEnabled(false);
            }
        }

        if (arg0.getSource() == jRadioButtonSelect) {
            if (jRadioButtonSelect.isSelected()) {
                jRadioButtonAdd.setSelected(false);
                jTextFieldAdd.setEnabled(false);
                jComboBoxSelect.setEnabled(true);
            }
            if (!jRadioButtonSelect.isSelected() && !jRadioButtonAdd.isSelected()) {
                jRadioButtonSelect.setSelected(true);
                jTextFieldAdd.setEnabled(false);
                jComboBoxSelect.setEnabled(true);
            }
        }
    }

    private boolean dataValidation(String[] row) {
        if (!DataValidation.isKeywordType(row[0])) {
            JOptionPane.showMessageDialog(frame, "Library Class is NOT KeyWord Type.");
            return false;
        }
        if (!DataValidation.isPathAndFilename(row[1])) {
            JOptionPane.showMessageDialog(frame, "Include Header is NOT PathAndFilename Type.");
        }
        if (row[2].length() == 0) {
            JOptionPane.showMessageDialog(frame, "HelpText could NOT be empty.");
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
    private JTextField getJTextField() {
        if (jTextField == null) {
            jTextField = new JTextField();
            jTextField.setBounds(new java.awt.Rectangle(221,75,290,21));
            jTextField.setPreferredSize(new java.awt.Dimension(260,20));
        }
        return jTextField;
    }

    /**
      This method initializes jButtonBrowse	
      	
      @return javax.swing.JButton	
     **/
    private JButton getJButtonBrowse() {
        if (jButtonBrowse == null) {
            jButtonBrowse = new JButton();
            jButtonBrowse.setBounds(new java.awt.Rectangle(528,75,90,20));
            jButtonBrowse.setText("Browse");
            jButtonBrowse.setPreferredSize(new java.awt.Dimension(80,20));
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
                    jTextField.setText(headerDest.substring(fileIndex + 1).replace('\\', '/'));
               
                }

            });
        }
        return jButtonBrowse;
    }
    
    public void componentResized(ComponentEvent arg0) {
        int intPreferredWidth = 500;
        
        resizeComponentWidth(this.jTextFieldAdd, this.getWidth(), intPreferredWidth);
        resizeComponentWidth(this.jTextFieldHelp, this.getWidth(), intPreferredWidth);
        resizeComponentWidth(this.jScrollPane, this.getWidth(), intPreferredWidth);
        
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
     * This method initializes jTextField1	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextField1() {
        if (jTextField1 == null) {
            jTextField1 = new JTextField();
            jTextField1.setBounds(new java.awt.Rectangle(220,110,291,20));
            jTextField1.setEnabled(true);
        }
        return jTextField1;
    }

    /**
     * This method initializes jTextField2	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextField2() {
        if (jTextField2 == null) {
            jTextField2 = new JTextField();
            jTextField2.setBounds(new java.awt.Rectangle(218,135,292,20));
            jTextField2.setEnabled(true);
        }
        return jTextField2;
    }

    /**
     * This method initializes jCheckBox	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBox() {
        if (jCheckBox == null) {
            jCheckBox = new JCheckBox();
            jCheckBox.setBounds(new java.awt.Rectangle(123,213,57,21));
            jCheckBox.setText("IA32");
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
            jCheckBox1.setBounds(new java.awt.Rectangle(182,213,49,20));
            jCheckBox1.setText("X64");
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
            jCheckBox2.setText("IPF");
            jCheckBox2.setSize(new java.awt.Dimension(50,20));
            jCheckBox2.setLocation(new java.awt.Point(237,213));
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
            jCheckBox3.setBounds(new java.awt.Rectangle(286,213,50,20));
            jCheckBox3.setText("EBC");
            jCheckBox3.setPreferredSize(new java.awt.Dimension(21,20));
        }
        return jCheckBox3;
    }

    private JScrollPane getJScrollPaneArch() {
        if (jScrollPaneArch == null) {
            jScrollPaneArch = new JScrollPane();
            jScrollPaneArch.setBounds(new java.awt.Rectangle(130,252,230,88));
            jScrollPaneArch.setPreferredSize(new java.awt.Dimension(320, 80));
            jScrollPaneArch.setViewportView(getICheckBoxListSupportedArchitectures());
        }
        return jScrollPaneArch;
    }
    
    private ICheckBoxList getICheckBoxListSupportedArchitectures() {
        if (iCheckBoxListArch == null) {
            iCheckBoxListArch = new ICheckBoxList();
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
            iCheckBoxListArch.setAllItems(v);
        }
        return iCheckBoxListArch;
    }
    
    private String booleanToString(boolean b1, boolean b2, boolean b3, boolean b4, boolean b5, boolean b6){
        String s = " ";
        if (b1){
            s += "IA32 ";
        }
        if (b2){
            s += "X64 ";
        }
        if (b3){
            s += "IPF ";
        }
        if (b4){
            s += "EBC ";
        }
        if (b5){
            s += "ARM ";
        }
        if (b6){
            s += "PPC ";
        }
        return s.trim();
    }
    
    private String vectorToString(Vector<String> v) {
        String s = " ";
        for (int i = 0; i < v.size(); ++i) {
            s += v.get(i);
            s += " ";
        }
        return s.trim();
    }
    /**
     * This method initializes jCheckBox4	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBox4() {
        if (jCheckBox4 == null) {
            jCheckBox4 = new JCheckBox();
            jCheckBox4.setBounds(new java.awt.Rectangle(343,213,60,20));
            jCheckBox4.setText("ARM");
            jCheckBox4.setPreferredSize(new Dimension(60, 20));
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
            jCheckBox5.setBounds(new java.awt.Rectangle(400,213,60,20));
            jCheckBox5.setText("PPC");
            jCheckBox5.setPreferredSize(new Dimension(60, 20));
        }
        return jCheckBox5;
    }

    public static void main(String[] args){
        new SpdLibClassDecls().setVisible(true);
    }
}


