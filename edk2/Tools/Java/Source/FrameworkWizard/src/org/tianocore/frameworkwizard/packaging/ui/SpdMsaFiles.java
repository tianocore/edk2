/** @file
  Java class SpdMsaFiles is GUI for create library definition elements of spd file.
 
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

import javax.swing.AbstractAction;
import javax.swing.JFileChooser;
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
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.Identifications.OpeningPackageType;
import org.tianocore.frameworkwizard.common.ui.IInternalFrame;
import org.tianocore.frameworkwizard.common.ui.StarLabel;

/**
 GUI for create library definition elements of spd file.
  
 @since PackageEditor 1.0
**/
public class SpdMsaFiles extends IInternalFrame implements TableModelListener{
    /**
     * 
     */
    private static final long serialVersionUID = 1L;

    private JFrame topFrame;
    
    private JScrollPane jScrollPane = null;  //  @jve:decl-index=0:visual-constraint="10,95"

    private JTable jTable = null;

    private DefaultTableModel model = null;

    private JPanel jContentPane = null;

    private JScrollPane jScrollPaneMsa = null;

    private JButton jButtonAdd = null;

    private JButton jButtonRemove = null;

    private JButton jButtonClearAll = null;

    private JButton jButtonCancel = null;

    private JButton jButtonOk = null;

    private JLabel jLabel = null;

    private JTextField jTextField = null;

    private JButton jButtonBrowse = null;
    
    private StarLabel jStarLabel2 = null;
    
    private SpdFileContents sfc = null;
    
    private OpeningPackageType docConsole = null;

    private int selectedRow = -1; 

    /**
      This method initializes this
     
     **/
    private void initialize() {
        
        this.setDefaultCloseOperation(javax.swing.WindowConstants.DISPOSE_ON_CLOSE);

    }

    /**
      This method initializes jScrollPane	
      	
      @return javax.swing.JScrollPane	
     **/
    private JScrollPane getJScrollPaneMsa() {
        if (jScrollPaneMsa == null) {
            jScrollPaneMsa = new JScrollPane();
            jScrollPaneMsa.setBounds(new java.awt.Rectangle(13,177,461,421));
            jScrollPaneMsa.setViewportView(getJTable());
        }
        return jScrollPaneMsa;
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
           model.addColumn("MsaFile");
           
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
    /**
      This method initializes jButtonAdd	
      	
      @return javax.swing.JButton	
     **/
    private JButton getJButtonAdd() {
        if (jButtonAdd == null) {
            jButtonAdd = new JButton();
            jButtonAdd.setBounds(new java.awt.Rectangle(163,148,90,20));
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
            jButtonRemove.setBounds(new java.awt.Rectangle(266,148,90,20));
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
            jButtonClearAll.setBounds(new java.awt.Rectangle(374,148,90,20));
            jButtonClearAll.setText("Clear All");
            jButtonClearAll.addActionListener(this);
        }
        return jButtonClearAll;
    }

    /**
      This method initializes jButtonCancel	
      	
      @return javax.swing.JButton	
     **/
    private JButton getJButtonCancel() {
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
    private JButton getJButtonOk() {
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
    public SpdMsaFiles(JFrame frame) {
        super();
        initialize();
        init();
        topFrame = frame;
    }

    public SpdMsaFiles(PackageSurfaceAreaDocument.PackageSurfaceArea inPsa, JFrame frame){
        this(frame);
        sfc = new SpdFileContents(inPsa);
        init(sfc);
    }
    
    public SpdMsaFiles(OpeningPackageType opt, JFrame frame){
        this(opt.getXmlSpd(), frame);
        docConsole = opt;
    }
    /**
      This method initializes this
      
      @return void
     **/
    private void init() {
        this.setBounds(new java.awt.Rectangle(0, 0, 500, 370));
        this.setPreferredSize(new Dimension(490, 400));
        this.setContentPane(getJScrollPane());
        this.setTitle("Msa Files");
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
        
        if (sfc.getSpdMsaFileCount() == 0) {
            return ;
        }
        //
        // initialize table using SpdFileContents object
        //
        String[][] saa = new String[sfc.getSpdMsaFileCount()][4];
        sfc.getSpdMsaFiles(saa);            
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
            jStarLabel2 = new StarLabel();
            jStarLabel2.setLocation(new java.awt.Point(2,24));
            jLabel = new JLabel();
            jLabel.setBounds(new java.awt.Rectangle(17,24,111,22));
            jLabel.setText("Msa File ");
            
            jContentPane = new JPanel();
            jContentPane.setPreferredSize(new Dimension(480, 325));
            jContentPane.setLayout(null);
            jContentPane.add(jLabel, null);
            jContentPane.add(jStarLabel2, null);
            jContentPane.add(getJScrollPaneMsa(), null);
            jContentPane.add(getJButtonAdd(), null);
            jContentPane.add(getJButtonRemove(), null);
            jContentPane.add(getJButtonClearAll(), null);
            jContentPane.add(getJButtonCancel(), null);
            jContentPane.add(getJButtonOk(), null);
            
            jContentPane.add(getJTextField(), null);
            jContentPane.add(getJButtonBrowse(), null);
        }
        return jContentPane;
    }

    /**
     fill ComboBoxes with pre-defined contents
    **/
    private void initFrame() {
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
        
        if (arg0.getSource() == jButtonOk) {
            this.save();
            this.dispose();

        }
        if (arg0.getSource() == jButtonCancel) {
            this.dispose();
        }

        if (arg0.getSource() == jButtonAdd) {
            //ToDo: check before add
            String[] row = {jTextField.getText().replace('\\', '/')};
            if (jTextField.getText().length() == 0) {
                JOptionPane.showMessageDialog(this, "Msa File is NOT PathAndFilename type.");
                return;
            }
            
            String dirPrefix = Tools.dirForNewSpd.substring(0, Tools.dirForNewSpd.lastIndexOf(File.separator));
            if (!new File(dirPrefix + File.separator + jTextField.getText()).exists()) {
                JOptionPane.showMessageDialog(this, "File NOT Exists in Current Package.");
                return;
            }
            model.addRow(row);
            jTable.changeSelection(model.getRowCount()-1, 0, false, false);
            sfc.genSpdMsaFiles(row[0], null, null, null);
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
                sfc.removeSpdMsaFile(rowSelected);
                docConsole.setSaved(false);
            }
        }

        if (arg0.getSource() == jButtonClearAll) {
            if (model.getRowCount() == 0) {
                return;
            }
            model.setRowCount(0);
            sfc.removeSpdMsaFile();
            docConsole.setSaved(false);
        }

    }
    
    public void tableChanged(TableModelEvent arg0) {
        // TODO Auto-generated method stub
        int row = arg0.getFirstRow();
        int column = arg0.getColumn();
        TableModel m = (TableModel)arg0.getSource();
        if (arg0.getType() == TableModelEvent.UPDATE){
            String[] sa = new String[1];
            sfc.getSpdMsaFile(sa, row);
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
            String file = m.getValueAt(row, 0) + "";
            if (file.length() == 0) {
                JOptionPane.showMessageDialog(this, "Msa File is NOT PathAndFilename type.");
                return;
            }
            docConsole.setSaved(false);
            sfc.updateSpdMsaFile(row, file, null, null, null);
        }
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
            jTextField.setBounds(new java.awt.Rectangle(14,51,346,21));
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
            jButtonBrowse.setBounds(new java.awt.Rectangle(376,50,92,21));
            jButtonBrowse.setText("Browse");
            jButtonBrowse.setPreferredSize(new java.awt.Dimension(90,20));
            jButtonBrowse.addActionListener(new AbstractAction() {
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
                    int retval = chooser.showOpenDialog(SpdMsaFiles.this);
                    if (retval == JFileChooser.APPROVE_OPTION) {

                        theFile = chooser.getSelectedFile();
                        String file = theFile.getPath();
                        if (!file.startsWith(dirPrefix)) {
                            JOptionPane.showMessageDialog(SpdMsaFiles.this, "You can only select files in current package!");
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
        
        Tools.resizeComponentWidth(this.jScrollPaneMsa, this.getWidth(), intPreferredWidth);
        Tools.resizeComponentWidth(this.jTextField, this.getWidth(), intPreferredWidth);
        Tools.relocateComponentX(this.jButtonBrowse, this.getWidth(), this.getPreferredSize().width, 25);
    }
    
}


