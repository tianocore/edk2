/** @file
  Java class UpdatePkgHeader is GUI for update Package Header in spd file.
 
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
package org.tianocore.packaging;

import javax.swing.DefaultCellEditor;
import javax.swing.JComboBox;
import javax.swing.JPanel;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JTextField;
import javax.swing.JButton;

import javax.swing.JScrollPane;
import javax.swing.JTable;
import javax.swing.table.*;

import org.tianocore.common.Tools;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.*;

/**
 GUI for update Package Header in spd file
  
 @since PackageEditor 1.0
**/
public class UpdatePkgHeader extends JFrame implements ActionListener {

    private JPanel jContentPane = null;

    private JScrollPane jScrollPane = null;

    private JTable jTable = null;

    private SpdFileContents sfc = null;

    private JButton jButtonOk = null;

    private JButton jButtonCancel = null;

    private DefaultTableModel model = null;

    private JButton jButton = null;

    /**
     This is the default constructor
     **/
    public UpdatePkgHeader(SpdFileContents sfc) {
        super();
        this.sfc = sfc;
        initialize();

    }

    public void actionPerformed(ActionEvent arg0) {
        if (arg0.getSource() == jButtonOk) {
            this.save();
            this.dispose();

        }
        if (arg0.getSource() == jButtonCancel) {
            this.dispose();

        }

        if (arg0.getSource() == jButton) {
            String[] o = { "BASE", "" };
            model.addRow(o);
        }
    }

    /**
     This method initializes this
     
     @return void
     **/
    private void initialize() {
        this.setSize(604, 553);
        this.setDefaultCloseOperation(javax.swing.WindowConstants.DISPOSE_ON_CLOSE);
        this.setTitle("Update Package Headers");
        this.setContentPane(getJContentPane());
    }

    /**
     This method initializes jContentPane
     
     @return javax.swing.JPanel
     **/
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.add(getJScrollPane(), null);
            jContentPane.add(getJButtonOk(), null);
            jContentPane.add(getJButtonCancel(), null);
            jContentPane.add(getJButton(), null);
        }
        return jContentPane;
    }

    /**
     This method initializes jScrollPane	
     	
     @return javax.swing.JScrollPane	
     **/
    private JScrollPane getJScrollPane() {
        if (jScrollPane == null) {
            jScrollPane = new JScrollPane();
            jScrollPane.setBounds(new java.awt.Rectangle(38, 45, 453, 419));
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
            model.addColumn("ModuleType");
            model.addColumn("IncludeHeader");
            //
            // Using combobox to display ModuleType in table
            //
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
            jComboBoxSelect.addItem("TOOLS");
            jComboBoxSelect.addItem("UEFI_DRIVER");
            jComboBoxSelect.addItem("UEFI_APPLICATION");
            jComboBoxSelect.addItem("USER_DEFINED");
            typeColumn.setCellEditor(new DefaultCellEditor(jComboBoxSelect));

            if (sfc.getSpdPackageHeaderCount() == 0) {
                return jTable;
            }
            String[][] saa = new String[sfc.getSpdPackageHeaderCount()][2];
            sfc.getSpdPackageHeaders(saa);
            int i = 0;
            while (i < saa.length) {
                model.addRow(saa[i]);
                i++;
            }

        }
        return jTable;
    }

    /**
    Remove original package headers before saving updated ones
    **/
    protected void save() {
        sfc.removeSpdPkgHeader();
        int rowCount = model.getRowCount();
        int i = 0;
        while (i < rowCount) {
            String headFile = null;
            if (model.getValueAt(i, 1) != null) {
                headFile = model.getValueAt(i, 1).toString();
            }
            sfc.genSpdModuleHeaders(model.getValueAt(i, 0).toString(), headFile, null, null, null, null, null, null);
            i++;
        }
    }

    /**
     This method initializes jButtonOk	
     	
     @return javax.swing.JButton	
     **/
    private JButton getJButtonOk() {
        if (jButtonOk == null) {
            jButtonOk = new JButton();
            jButtonOk.setText("Ok");
            jButtonOk.setSize(new java.awt.Dimension(84, 20));
            jButtonOk.setLocation(new java.awt.Point(316, 486));
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
            jButtonCancel.setSize(new java.awt.Dimension(82, 20));
            jButtonCancel.setLocation(new java.awt.Point(411, 486));
            jButtonCancel.addActionListener(this);
        }
        return jButtonCancel;
    }

    /**
     This method initializes jButton	
     	
     @return javax.swing.JButton	
     **/
    private JButton getJButton() {
        if (jButton == null) {
            jButton = new JButton();
            jButton.setBounds(new java.awt.Rectangle(220, 486, 85, 20));
            jButton.setText("Insert");
            jButton.addActionListener(this);
        }
        return jButton;
    }
} //  @jve:decl-index=0:visual-constraint="11,7"
