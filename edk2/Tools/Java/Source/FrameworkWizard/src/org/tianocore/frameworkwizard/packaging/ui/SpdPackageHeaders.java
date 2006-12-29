/** @file
  Java class SpdPackageHeaders is GUI for create library definition elements of spd file.
 
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

import javax.swing.DefaultCellEditor;
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
import javax.swing.table.TableColumn;
import javax.swing.table.TableModel;

import org.tianocore.PackageSurfaceAreaDocument;
import org.tianocore.frameworkwizard.common.DataValidation;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.Identifications.OpeningPackageType;
import org.tianocore.frameworkwizard.common.ui.IInternalFrame;
import org.tianocore.frameworkwizard.common.ui.StarLabel;

/**
 GUI for create library definition elements of spd file.
  
 @since PackageEditor 1.0
**/
public class SpdPackageHeaders extends IInternalFrame implements TableModelListener{
    /**
     * 
     */
    private static final long serialVersionUID = 1L;

    private JFrame topFrame;
    
    private SpdFileContents sfc = null;
    
    private OpeningPackageType docConsole = null;

    private DefaultTableModel model = null;

    private JPanel jContentPane = null;

    private JLabel jLabelSelect = null;

    private JComboBox jComboBoxSelect = null;

    private JButton jButtonAdd = null;

    private JButton jButtonRemove = null;

    private JButton jButtonClearAll = null;

    private JLabel jLabel = null;

    private JTextField jTextField = null;

    private JButton jButtonBrowse = null;
    
    private StarLabel jStarLabel1 = null;
    
    private StarLabel jStarLabel2 = null;

    private JScrollPane jScrollPane = null;

    private JScrollPane jScrollPanePkgHdr = null;

    private int selectedRow = -1;

    private JTable jTable = null;

    private final int buttonWidth = 100;
    private final int buttonSep = 6;
    private final int addButtonCol = 148;
    private final int removeButtonCol = addButtonCol + buttonWidth + buttonSep;
    private final int removeAllButtonCol = removeButtonCol + buttonWidth + buttonSep;
    private final int labelCol = 12;
    private final int valueCol = 168;
    private final int rowOne = 12;
    private final int rowTwo = rowOne + 25;
    private final int rowThree = rowTwo + 25;
    private final int rowFour = rowThree + 30;
    private final int rowFive = rowFour + 30;


    /**
      This method initializes this
     
     **/
    private void initialize() {
        
        this.setDefaultCloseOperation(javax.swing.WindowConstants.DISPOSE_ON_CLOSE);

    }

    /**
      This method initializes jLabelSelect
      	
      @return javax.swing.JLabel jLabelSelect
     **/
    private JLabel getJLabelSelect() {
        if (jLabelSelect == null) {
            jLabelSelect = new JLabel();
            jLabelSelect.setBounds(new java.awt.Rectangle(labelCol,rowOne,155,20));
            jLabelSelect.setText("Select ModuleType");

        }
        return jLabelSelect;
    }

    /**
      This method initializes jComboBoxSelect	
      	
      @return javax.swing.JComboBox	
     **/
    private JComboBox getJComboBoxSelect() {
        if (jComboBoxSelect == null) {
            jComboBoxSelect = new JComboBox();
            jComboBoxSelect.setBounds(new java.awt.Rectangle(valueCol, rowOne, 260, 20));
            jComboBoxSelect.setPreferredSize(new java.awt.Dimension(260,20));
            
            jComboBoxSelect.setEnabled(true);
        }
        return jComboBoxSelect;
    }

    /**
    This method initializes jTable  
        
    @return javax.swing.JTable  
    *
   private JTable getJTable() {
       if (jTable == null) {
           model = new PackageHeaderTableModel();
           model.addColumn("ModuleType");
           model.addColumn("IncludeHeader");
           

       }
       return jTable;
   }*/
    /**
      This method initializes jButtonAdd	
      	
      @return javax.swing.JButton	
     **/
    private JButton getJButtonAdd() {
        if (jButtonAdd == null) {
            jButtonAdd = new JButton();
            jButtonAdd.setBounds(new java.awt.Rectangle(addButtonCol,rowFour,buttonWidth,20));
            jButtonAdd.setText("Add");
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
            jButtonRemove.setBounds(new java.awt.Rectangle(removeButtonCol,rowFour,buttonWidth,20));
            jButtonRemove.setText("Delete");
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
            jButtonClearAll.setBounds(new java.awt.Rectangle(removeAllButtonCol,rowFour,buttonWidth,20));
            jButtonClearAll.setText("Clear All");
            jButtonClearAll.addActionListener(this);
        }
        return jButtonClearAll;
    }

    /**
      This is the default constructor
     **/
    public SpdPackageHeaders(JFrame frame) {
        super();
        initialize();
        init();
        topFrame = frame;
    }

    public SpdPackageHeaders(PackageSurfaceAreaDocument.PackageSurfaceArea inPsa, JFrame frame){
        this(frame);
        sfc = new SpdFileContents(inPsa);
        init(sfc);
    }
    
    public SpdPackageHeaders(OpeningPackageType opt, JFrame frame) {
        this(opt.getXmlSpd(), frame);
        docConsole = opt;
    }
    /**
      This method initializes this
      
      @return void
     **/
    private void init() {
        
        this.setContentPane(getJScrollPane());
        this.setTitle("Package Headers");
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

    private void init(SpdFileContents sfc){

        if (sfc.getSpdPkgDefsRdOnly().equals("true")) {
            JOptionPane.showMessageDialog(topFrame, "This is a read-only package. You will not be able to edit contents in table.");
        }
        initFrame();
        
        if (sfc.getSpdPackageHeaderCount() == 0) {
            return ;
        }
        String[][] saa = new String[sfc.getSpdPackageHeaderCount()][2];
        sfc.getSpdPackageHeaders(saa);
        int i = 0;
        while (i < saa.length) {
            model.addRow(saa[i]);
            i++;
        }
        
    }
    
    private JScrollPane getJScrollPane(){
        if (jScrollPane == null){
          jScrollPane = new JScrollPane();
          jScrollPane.setViewportView(getJContentPane());
        }
        return jScrollPane;
    }
    /**
      This method initializes jContentPane
      
      @return javax.swing.JPanel
     **/
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jStarLabel1 = new StarLabel();
            jStarLabel1.setLocation(new java.awt.Point(0, rowOne));
            jStarLabel2 = new StarLabel();
            jStarLabel2.setLocation(new java.awt.Point(0,rowTwo));
            jLabel = new JLabel();
            jLabel.setBounds(new java.awt.Rectangle(labelCol,rowTwo,320,22));
            jLabel.setText("Include Header for Module Type");
            
            jContentPane = new JPanel();
            jContentPane.setPreferredSize(new Dimension(480, 325));
            jContentPane.setLayout(null);
            jContentPane.add(jLabel, null);
            jContentPane.add(jStarLabel1, null);
            jContentPane.add(jStarLabel2, null);
            jContentPane.add(getJLabelSelect(), null);
            jContentPane.add(getJComboBoxSelect(), null);
            jContentPane.add(getJButtonAdd(), null);
            jContentPane.add(getJButtonRemove(), null);
            jContentPane.add(getJButtonClearAll(), null);
            
            jContentPane.add(getJTextField(), null);
            jContentPane.add(getJButtonBrowse(), null);
            jContentPane.add(getJScrollPanePkgHdr(), null);
        }
        return jContentPane;
    }

    /**
     fill ComboBoxes with pre-defined contents
    **/
    private void initFrame() {
        jComboBoxSelect.addItem("BASE");
        jComboBoxSelect.addItem("SEC");
        jComboBoxSelect.addItem("PEI_CORE");
        jComboBoxSelect.addItem("PEIM");
        jComboBoxSelect.addItem("DXE_CORE");
        jComboBoxSelect.addItem("DXE_DRIVER");
        jComboBoxSelect.addItem("DXE_RUNTIME_DRIVER");
        jComboBoxSelect.addItem("DXE_SAL_DRIVER");
        jComboBoxSelect.addItem("DXE_SMM_DRIVER");
        jComboBoxSelect.addItem("TOOL");
        jComboBoxSelect.addItem("UEFI_DRIVER");
        jComboBoxSelect.addItem("UEFI_APPLICATION");
        jComboBoxSelect.addItem("USER_DEFINED");
        jComboBoxSelect.setSelectedIndex(0);
        
        boolean editable = true;
        if (sfc.getSpdPkgDefsRdOnly().equals("true")) {
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
        
        docConsole.setSaved(false);

        if (arg0.getSource() == jButtonAdd) {
            String strLibClass = "";
            
            strLibClass = jComboBoxSelect.getSelectedItem().toString();
            //ToDo: check before add
            String[] row = {"", ""};
            row[0] = strLibClass;
            row[1] = jTextField.getText().replace('\\', '/');
            if (!dataValidation(row)) {
                return;
            }
            model.addRow(row);
            jTable.changeSelection(model.getRowCount()-1, 0, false, false);
            sfc.genSpdModuleHeaders(row[0], row[1], null, null, null, null, null, null);
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
                sfc.removeSpdPkgHeader(rowSelected);
            }
        }

        if (arg0.getSource() == jButtonClearAll) {
            if (model.getRowCount() == 0) {
                return;
            }
            model.setRowCount(0);
            sfc.removeSpdPkgHeader();
        }

    }
    
    private boolean dataValidation(String[] row) {
        if (!DataValidation.isPathAndFilename(row[1])) {
            JOptionPane.showMessageDialog(this, "Include header path is NOT PathAndFilename type.");
            return false;
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
            jTextField.setBounds(new java.awt.Rectangle(labelCol,rowThree,346,20));
            jTextField.setPreferredSize(new java.awt.Dimension(345,20));
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
            jButtonBrowse.setBounds(new java.awt.Rectangle(374,rowThree,buttonWidth,20));
            jButtonBrowse.setText("Browse");
            jButtonBrowse.setPreferredSize(new java.awt.Dimension(buttonWidth,20));
            jButtonBrowse.addActionListener(new javax.swing.AbstractAction() {
                /**
                 * 
                 */
                private static final long serialVersionUID = 1L;

                public void actionPerformed(ActionEvent e) {
                    //
                    // Select files from current workspace
                    //
                    String dirPrefix = Tools.dirForNewSpd.substring(0, Tools.dirForNewSpd.lastIndexOf(File.separator));
                    JFileChooser chooser = new JFileChooser(dirPrefix);
                    File theFile = null;
                    String headerDest = null;
                    
                    chooser.setMultiSelectionEnabled(false);
                    chooser.setFileSelectionMode(JFileChooser.FILES_AND_DIRECTORIES);
                    int retval = chooser.showOpenDialog(SpdPackageHeaders.this);
                    if (retval == JFileChooser.APPROVE_OPTION) {

                        theFile = chooser.getSelectedFile();
                        String file = theFile.getPath();
                        if (!file.startsWith(dirPrefix)) {
                            JOptionPane.showMessageDialog(SpdPackageHeaders.this, "You can only select files in current package!");
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
    
    /**
     * This method initializes jScrollPane	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPanePkgHdr() {
        if (jScrollPanePkgHdr == null) {
            jScrollPanePkgHdr = new JScrollPane();
            jScrollPanePkgHdr.setBounds(new java.awt.Rectangle(labelCol,rowFive,453,258));
            jScrollPanePkgHdr.setViewportView(getJTable());
        }
        return jScrollPanePkgHdr;
    }

    /**
     * This method initializes jTable	
     * 	
     * @return javax.swing.JTable	
     */
    private JTable getJTable() {
        if (jTable == null) {
            model = new DefaultTableModel();
            jTable = new JTable(model);
            jTable.setRowHeight(20);
            model.addColumn("ModuleType");
            model.addColumn("IncludeHeader");
            TableColumn typeColumn = jTable.getColumnModel().getColumn(0);
            JComboBox jComboBoxSelect = new JComboBox();
            jComboBoxSelect.addItem("BASE");
            jComboBoxSelect.addItem("SEC");
            jComboBoxSelect.addItem("PEI_CORE");
            jComboBoxSelect.addItem("PEIM");
            jComboBoxSelect.addItem("DXE_CORE");
            jComboBoxSelect.addItem("DXE_DRIVER");
            jComboBoxSelect.addItem("DXE_RUNTIME_DRIVER");
            jComboBoxSelect.addItem("DXE_SAL_DRIVER");
            jComboBoxSelect.addItem("DXE_SMM_DRIVER");
            jComboBoxSelect.addItem("TOOL");
            jComboBoxSelect.addItem("UEFI_DRIVER");
            jComboBoxSelect.addItem("UEFI_APPLICATION");
            jComboBoxSelect.addItem("USER_DEFINED");
            typeColumn.setCellEditor(new DefaultCellEditor(jComboBoxSelect));
            
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
            String[] sa = new String[2];
            sfc.getSpdModuleHeader(sa, row);
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
            String pkg = m.getValueAt(row, 0) + "";
            String hdr = m.getValueAt(row, 1) + "";
            String[] rowData = {pkg, hdr};
            if (!dataValidation(rowData)) {
                return;
            }
            docConsole.setSaved(false);
            sfc.updateSpdPkgHdr(row, pkg, hdr);
        }
    }

    public void componentResized(ComponentEvent arg0) {
        int intPreferredWidth = 500;
        
        // Tools.resizeComponentWidth(this.jComboBoxSelect, this.getWidth(), intPreferredWidth);
        Tools.resizeComponentWidth(this.jTextField, this.getWidth(), intPreferredWidth);
        Tools.resizeComponentWidth(this.jScrollPanePkgHdr, this.getWidth(), intPreferredWidth);
        Tools.relocateComponentX(this.jButtonBrowse, this.getWidth(), this.getPreferredSize().width, 30);
    }

}


