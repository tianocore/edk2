/** @file
 
 The file is used to setup tool chain configuration
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/
package org.tianocore.frameworkwizard;

import java.awt.event.ActionEvent;
import java.awt.event.WindowEvent;
import java.io.File;
import java.io.IOException;

import javax.swing.JButton;
import javax.swing.JFileChooser;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTable;
import javax.swing.JTextField;
import javax.swing.ListSelectionModel;
import javax.swing.event.ListSelectionEvent;
import javax.swing.event.ListSelectionListener;
import javax.swing.event.TableModelEvent;
import javax.swing.event.TableModelListener;
import javax.swing.table.DefaultTableModel;
import javax.swing.table.TableModel;

import org.tianocore.frameworkwizard.common.DataType;
import org.tianocore.frameworkwizard.common.IFileFilter;
import org.tianocore.frameworkwizard.common.Log;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.Identifications.ToolChainConfigId;
import org.tianocore.frameworkwizard.common.Identifications.ToolChainConfigVector;
import org.tianocore.frameworkwizard.common.ui.IFrame;
import org.tianocore.frameworkwizard.workspace.Workspace;

public class ToolChainConfig extends IFrame implements ListSelectionListener, TableModelListener{

    ///
    /// Define Class Members
    ///
    private static final long serialVersionUID = 1486930966278269085L;

    private JPanel jContentPane = null;

    private JScrollPane jScrollPane = null;

    private DefaultTableModel model = null;

    private JTable jTable = null;

    private JButton jButtonOpen = null;

    private JButton jButtonSave = null;

    private JButton jButtonClose = null;

    private String toolsDir = Tools.addFileSeparator(Workspace.getCurrentWorkspace()) + "Tools"
                              + DataType.FILE_SEPARATOR + "Conf";

    private String currentFile = Tools.addFileSeparator(toolsDir) + "tools_def.template";
    
    private ToolChainConfigVector vtcc = new ToolChainConfigVector();

    private JLabel jLabelName = null;

    private JTextField jTextFieldName = null;

    private JLabel jLabelValue = null;

    private JTextField jTextFieldValue = null;

    private JButton jButtonAdd = null;

    private JButton jButtonRemove = null;

    private int selectedRow = -1;

    private JButton jButtonHelp = null;
    
    private static ToolChainConfig tcc = null;
    
    private ToolChainConfigHelp tcch = null;
    
    /**
     This method initializes jScrollPane	
     
     @return javax.swing.JScrollPane	
     
     **/
    private JScrollPane getJScrollPane() {
        if (jScrollPane == null) {
            jScrollPane = new JScrollPane();
            jScrollPane.setBounds(new java.awt.Rectangle(15, 15, 555, 345));
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

            model.addColumn("Property");
            model.addColumn("Value");

            jTable.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
            jTable.getSelectionModel().addListSelectionListener(this);
            jTable.getModel().addTableModelListener(this);
        }
        return jTable;
    }

    /**
     This method initializes jButtonOpen	
     
     @return javax.swing.JButton	
     
     **/
    private JButton getJButtonOpen() {
        if (jButtonOpen == null) {
            jButtonOpen = new JButton();
            jButtonOpen.setBounds(new java.awt.Rectangle(40, 405, 120, 20));
            jButtonOpen.setText("Open File");
            jButtonOpen.addActionListener(this);
        }
        return jButtonOpen;
    }

    /**
     This method initializes jButtonSave	
     
     @return javax.swing.JButton	
     
     **/
    private JButton getJButtonSave() {
        if (jButtonSave == null) {
            jButtonSave = new JButton();
            jButtonSave.setBounds(new java.awt.Rectangle(170, 405, 120, 20));
            jButtonSave.setText("Save File");
            jButtonSave.addActionListener(this);
        }
        return jButtonSave;
    }

    /**
     This method initializes jButtonClose	
     
     @return javax.swing.JButton	
     
     **/
    private JButton getJButtonClose() {
        if (jButtonClose == null) {
            jButtonClose = new JButton();
            jButtonClose.setBounds(new java.awt.Rectangle(490, 405, 80, 20));
            jButtonClose.setText("Close");
            jButtonClose.addActionListener(this);
        }
        return jButtonClose;
    }

    /**
     This method initializes jTextFieldName	
     
     @return javax.swing.JTextField	
     
     **/
    private JTextField getJTextFieldName() {
        if (jTextFieldName == null) {
            jTextFieldName = new JTextField();
            jTextFieldName.setBounds(new java.awt.Rectangle(60, 365, 140, 20));
        }
        return jTextFieldName;
    }

    /**
     This method initializes jTextFieldValue	
     
     @return javax.swing.JTextField	
     
     **/
    private JTextField getJTextFieldValue() {
        if (jTextFieldValue == null) {
            jTextFieldValue = new JTextField();
            jTextFieldValue.setBounds(new java.awt.Rectangle(250, 365, 140, 20));
        }
        return jTextFieldValue;
    }

    /**
     This method initializes jButtonAdd	
     
     @return javax.swing.JButton	
     
     **/
    private JButton getJButtonAdd() {
        if (jButtonAdd == null) {
            jButtonAdd = new JButton();
            jButtonAdd.setBounds(new java.awt.Rectangle(400, 365, 80, 20));
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
            jButtonRemove.setBounds(new java.awt.Rectangle(490, 365, 80, 20));
            jButtonRemove.setText("Remove");
            jButtonRemove.addActionListener(this);
        }
        return jButtonRemove;
    }

    /**
     * This method initializes jButtonHelp	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonHelp() {
        if (jButtonHelp == null) {
            jButtonHelp = new JButton();
            jButtonHelp.setBounds(new java.awt.Rectangle(300,405,120,20));
            jButtonHelp.setText("Help");
            jButtonHelp.addActionListener(this);
        }
        return jButtonHelp;
    }

    /**
     
     @param args
     
     **/
    public static void main(String[] args) {
        // TODO Auto-generated method stub

    }
    
    public static ToolChainConfig getInstance() {
        if (tcc == null) {
            tcc = new ToolChainConfig();
        }
        return tcc;
    }

    /**
     * This is the default constructor
     */
    public ToolChainConfig() {
        super();
        init();
    }

    /**
     This method initializes this
     
     **/
    private void init() {
        this.setSize(600, 480);
        this.setContentPane(getJContentPane());
        this.setTitle("Tool Chain Configuration");
        this.centerWindow();

        //
        // Read default file
        //
        File f = new File(currentFile);
        if (f.exists()) {
            try {
                vtcc.removeAll();
                vtcc.parseFile(this.currentFile);
                this.setTitle("Tool Chain Configuration" + " [" + currentFile + "]");
            } catch (IOException e) {
                Log.log(this.currentFile + "Read Error", e.getMessage());
                e.printStackTrace();
            }
        } else {
            Log.log("File Open Error: ", this.currentFile + " File Not Found");
        }

        showTable();
    }

    /**
     This method initializes jContentPane
     
     @return javax.swing.JPanel
     
     **/
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jLabelValue = new JLabel();
            jLabelValue.setBounds(new java.awt.Rectangle(205, 365, 40, 20));
            jLabelValue.setText("Value");
            jLabelName = new JLabel();
            jLabelName.setBounds(new java.awt.Rectangle(15, 365, 40, 20));
            jLabelName.setText("Name");
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.add(getJScrollPane(), null);
            jContentPane.add(getJButtonOpen(), null);
            jContentPane.add(getJButtonSave(), null);
            jContentPane.add(getJButtonClose(), null);
            jContentPane.add(jLabelName, null);
            jContentPane.add(getJTextFieldName(), null);
            jContentPane.add(jLabelValue, null);
            jContentPane.add(getJTextFieldValue(), null);
            jContentPane.add(getJButtonAdd(), null);
            jContentPane.add(getJButtonRemove(), null);
            jContentPane.add(getJButtonHelp(), null);
        }
        return jContentPane;
    }

    /* (non-Javadoc)
     * @see java.awt.event.ActionListener#actionPerformed(java.awt.event.ActionEvent)
     * 
     * Override actionPerformed to listen all actions
     */
    public void actionPerformed(ActionEvent arg0) {
        if (arg0.getSource() == jButtonClose) {
            this.exit();
        }

        if (arg0.getSource() == jButtonOpen) {
            JFileChooser fc = new JFileChooser();
            fc.setAcceptAllFileFilterUsed(false);
            IFileFilter iff = new IFileFilter(DataType.TEXT_FILE_EXT);
            fc.addChoosableFileFilter(iff);
            fc.setCurrentDirectory(new File(toolsDir));

            int result = fc.showOpenDialog(new JPanel());
            if (result == JFileChooser.APPROVE_OPTION) {
                try {
                    vtcc.removeAll();
                    vtcc.parseFile(fc.getSelectedFile().getPath());
                    currentFile = fc.getSelectedFile().getPath();
                    this.setTitle("Tool Chain Configuration" + " [" + currentFile + "]");
                } catch (IOException e) {
                    Log.wrn(this.currentFile + "Read Error", e.getMessage());
                    Log.err(this.currentFile + "Read Error", e.getMessage());
                    return;
                }
                this.showTable();
            }
        }

        if (arg0.getSource() == jButtonSave) {
            JFileChooser fc = new JFileChooser();
            fc.setAcceptAllFileFilterUsed(false);
            IFileFilter iff = new IFileFilter(DataType.TEXT_FILE_EXT);
            fc.addChoosableFileFilter(iff);
            fc.setCurrentDirectory(new File(toolsDir));

            int result = fc.showSaveDialog(new JPanel());
            if (result == JFileChooser.APPROVE_OPTION) {
                currentFile = fc.getSelectedFile().getPath();
                currentFile = Tools.addPathExt(currentFile, DataType.RETURN_TYPE_TEXT);
                try {
                    vtcc.saveFile(currentFile);
                } catch (IOException e) {
                    Log.wrn(this.currentFile + "Write Error", e.getMessage());
                    Log.err(this.currentFile + "Write Error", e.getMessage());
                    return;
                }
            }
        }

        if (arg0.getSource() == jButtonAdd) {
            if (check()) {
                String[] row = { jTextFieldName.getText(), jTextFieldValue.getText() };
                this.vtcc.addToolChainConfigs(new ToolChainConfigId(row[0], row[1]));
                this.model.addRow(row);
            }
        }

        if (arg0.getSource() == jButtonRemove) {
            if (jTable.isEditing()) {
                jTable.getCellEditor().stopCellEditing();
            }
            if (selectedRow > -1) {
                this.model.removeRow(selectedRow);
                this.vtcc.removeToolChainConfigs(selectedRow);
                selectedRow = -1;
            }
        }
        
        if (arg0.getSource() == jButtonHelp) {
            tcch = ToolChainConfigHelp.getInstance();
            tcch.setVisible(true);
        }
    }

    /**
     Read content of vector and put then into table
     
     **/
    private void showTable() {
        clearAll();
 
        if (vtcc.size() > 0) {
            for (int index = 0; index < vtcc.size(); index++) {
                model.addRow(vtcc.toStringVector(index));
            }
        }
        this.jTable.repaint();
        this.jTable.updateUI();
        //this.jScrollPane.setViewportView(this.jTable);
    }

    /**
     Clear all table rows
     
     **/
    private void clearAll() {
        if (model != null) {
            for (int index = model.getRowCount() - 1; index >= 0; index--) {
                model.removeRow(index);
            }
        }
    }

    /**
     Check if name or value is empty
     
     @return
     
     **/
    private boolean check() {
        if (isEmpty(this.jTextFieldName.getText())) {
            Log.wrn("Add Tool Chain", "The Property Name must be entered!");
            return false;
        }

        if (isEmpty(this.jTextFieldValue.getText())) {
            Log.wrn("Add Tool Chain", "The Property Value must be entered!");
            return false;
        }
        return true;
    }

    /* (non-Javadoc)
     * @see javax.swing.event.ListSelectionListener#valueChanged(javax.swing.event.ListSelectionEvent)
     *
     */
    public void valueChanged(ListSelectionEvent arg0) {
        if (arg0.getValueIsAdjusting()) {
            return;
        }
        ListSelectionModel lsm = (ListSelectionModel) arg0.getSource();
        if (lsm.isSelectionEmpty()) {
            return;
        } else {
            selectedRow = lsm.getMinSelectionIndex();
        }
    }

    public void tableChanged(TableModelEvent arg0) {
        int row = arg0.getFirstRow();
        TableModel m = (TableModel)arg0.getSource();
        if (arg0.getType() == TableModelEvent.UPDATE){
            this.vtcc.getToolChainConfigs(row).setName(m.getValueAt(row, 0).toString());
            this.vtcc.getToolChainConfigs(row).setValue(m.getValueAt(row, 1).toString());
        }
    }
    
    /* (non-Javadoc)
     * @see java.awt.event.WindowListener#windowClosing(java.awt.event.WindowEvent)
     *
     * Override windowClosing to popup warning message to confirm quit
     * 
     */
    public void windowClosing(WindowEvent arg0) {
        this.exit();
    }
    
    private void exit() {
        this.setVisible(false);
        if (tcch != null) {
            tcch.dispose();
        }
    }
}
