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
import java.io.File;
import java.io.IOException;

import javax.swing.JButton;
import javax.swing.JFileChooser;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTable;
import javax.swing.table.DefaultTableModel;

import org.tianocore.frameworkwizard.common.DataType;
import org.tianocore.frameworkwizard.common.Log;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.Identifications.ToolChainConfigVector;
import org.tianocore.frameworkwizard.common.ui.IDialog;
import org.tianocore.frameworkwizard.common.ui.IFrame;
import org.tianocore.frameworkwizard.workspace.Workspace;

public class ToolChainConfig extends IDialog {

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

    /**
     * This method initializes jScrollPane	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPane() {
        if (jScrollPane == null) {
            jScrollPane = new JScrollPane();
            jScrollPane.setBounds(new java.awt.Rectangle(15, 15, 555, 345));
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
            jTable = new JTable();
            //            model = new DefaultTableModel();
            //            jTable = new JTable(model);
            //            jTable.setRowHeight(20);
        }
        return jTable;
    }

    /**
     * This method initializes jButtonOpen	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonOpen() {
        if (jButtonOpen == null) {
            jButtonOpen = new JButton();
            jButtonOpen.setBounds(new java.awt.Rectangle(140, 390, 120, 25));
            jButtonOpen.setText("Open a file");
            jButtonOpen.addActionListener(this);
        }
        return jButtonOpen;
    }

    /**
     * This method initializes jButtonSave	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonSave() {
        if (jButtonSave == null) {
            jButtonSave = new JButton();
            jButtonSave.setBounds(new java.awt.Rectangle(280, 390, 120, 25));
            jButtonSave.setText("Save to a file");
            jButtonSave.addActionListener(this);
        }
        return jButtonSave;
    }

    /**
     * This method initializes jButtonClose	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonClose() {
        if (jButtonClose == null) {
            jButtonClose = new JButton();
            jButtonClose.setBounds(new java.awt.Rectangle(465, 390, 100, 25));
            jButtonClose.setText("Close");
            jButtonClose.addActionListener(this);
        }
        return jButtonClose;
    }

    /**
     
     @param args
     
     **/
    public static void main(String[] args) {
        // TODO Auto-generated method stub

    }

    /**
     * This is the default constructor
     */
    public ToolChainConfig(IFrame parentFrame, boolean modal) {
        super(parentFrame, modal);
        init();
    }

    /**
     * This method initializes this
     * 
     * @return void
     */
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
            Log.log("Open file", this.currentFile + " Not Found");
        }

        showTable();
    }

    /**
     * This method initializes jContentPane
     * 
     * @return javax.swing.JPanel
     */
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.add(getJScrollPane(), null);
            jContentPane.add(getJButtonOpen(), null);
            jContentPane.add(getJButtonSave(), null);
            jContentPane.add(getJButtonClose(), null);
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
            this.setVisible(false);
            this.returnType = DataType.RETURN_TYPE_CANCEL;
        }

        if (arg0.getSource() == jButtonOpen) {
            JFileChooser fc = new JFileChooser();
            fc.setAcceptAllFileFilterUsed(false);
            fc.setCurrentDirectory(new File(toolsDir));

            int result = fc.showSaveDialog(new JPanel());
            if (result == JFileChooser.APPROVE_OPTION) {
                try {
                    vtcc.removeAll();
                    vtcc.parseFile(fc.getSelectedFile().getPath());
                    currentFile = fc.getSelectedFile().getPath();                    
                    this.setTitle("Tool Chain Configuration" + " [" + currentFile + "]");
                } catch (IOException e) {
                    Log.err(this.currentFile + "Read Error", e.getMessage());
                    e.printStackTrace();
                    return;
                }
                this.showTable();
            }
        }

        if (arg0.getSource() == jButtonSave) {
            JFileChooser fc = new JFileChooser();
            fc.setAcceptAllFileFilterUsed(false);
            fc.setCurrentDirectory(new File(toolsDir));

            int result = fc.showOpenDialog(new JPanel());
            if (result == JFileChooser.APPROVE_OPTION) {
            }
        }
    }

    /**
     Read content of vector and put then into table
     
     **/
    private void showTable() {
        model = new DefaultTableModel();
        jTable = new JTable(model);
        jTable.setRowHeight(20);

        model.addColumn("Name");
        model.addColumn("Value");
        if (vtcc.size() > 0) {
            for (int index = 0; index < vtcc.size(); index++) {
                model.addRow(vtcc.toStringVector(index));
            }
        }
        this.jScrollPane.setViewportView(this.jTable);
    }
}
