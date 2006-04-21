/** @file
  Java class UpdatePCD is GUI for update PCD definitions in spd file.
 
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
 GUI for update PCD definitions in spd file
  
 @since PackageEditor 1.0
**/
public class UpdatePCD extends JFrame implements ActionListener {

    private JPanel jContentPane = null;

    private SpdFileContents sfc = null;

    private JButton jButtonOk = null;

    private JButton jButtonCancel = null;

    private DefaultTableModel model = null;

    private String[][] saa = null;

    private JScrollPane jScrollPane = null;

    private JTable jTable = null;

    private JButton jButton = null;

    /**
     This is the default constructor
     **/
    public UpdatePCD(SpdFileContents sfc) {
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
            String[] o = { "FEATURE_FLAG", "", "", "UINT8", "0" };
            model.addRow(o);
        }

    }

    /**
     This method initializes this
     
     @return void
     **/
    private void initialize() {
        this.setSize(916, 486);
        this.setDefaultCloseOperation(javax.swing.WindowConstants.DISPOSE_ON_CLOSE);
        this.setTitle("Update PCD Definitions");
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
            jContentPane.add(getJButtonOk(), null);
            jContentPane.add(getJButtonCancel(), null);
            jContentPane.add(getJScrollPane(), null);
            jContentPane.add(getJButton(), null);
        }
        return jContentPane;
    }

    /**
    Remove original Pcd definitions before saving updated ones
    **/
    protected void save() {
        sfc.removeSpdPcdDefinition();
        int rowCount = model.getRowCount();
        int i = 0;
        while (i < rowCount) {
            String cName = null;
            if (model.getValueAt(i, 1) != null) {
                cName = model.getValueAt(i, 1).toString();
            }
            String token = null;
            if (model.getValueAt(i, 2) != null) {
                token = model.getValueAt(i, 2).toString();
            }
            String defaultVal = null;
            if (model.getValueAt(i, 4) != null) {
                defaultVal = model.getValueAt(i, 4).toString();
            }
            sfc.genSpdPcdDefinitions(model.getValueAt(i, 0).toString(), cName, token,
                                     model.getValueAt(i, 3).toString(), null, null, null, null, null, null, defaultVal);
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
            jButtonOk.setLocation(new java.awt.Point(605, 404));
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
            jButtonCancel.setLocation(new java.awt.Point(712, 404));
            jButtonCancel.addActionListener(this);
        }
        return jButtonCancel;
    }

    /**
     This method initializes jScrollPane	
     	
     @return javax.swing.JScrollPane	
     **/
    private JScrollPane getJScrollPane() {
        if (jScrollPane == null) {
            jScrollPane = new JScrollPane();
            jScrollPane.setHorizontalScrollBarPolicy(javax.swing.JScrollPane.HORIZONTAL_SCROLLBAR_AS_NEEDED);
            jScrollPane.setBounds(new java.awt.Rectangle(51, 62, 782, 304));
            jScrollPane.setViewportView(getJTable2());
        }
        return jScrollPane;
    }

    /**
     This method initializes jTable	
     	
     @return javax.swing.JTable	
     **/
    private JTable getJTable2() {
        if (jTable == null) {
            model = new DefaultTableModel();
            jTable = new JTable(model);
            jTable.setRowHeight(20);
            model.addColumn("ItemType");
            model.addColumn("C_Name");
            model.addColumn("Token");
            model.addColumn("DataType");
            model.addColumn("DefaultValue");
            //
            // Using combobox to display ItemType in table
            //
            JComboBox jComboBoxItemType = new JComboBox();
            jComboBoxItemType.addItem("FEATURE_FLAG");
            jComboBoxItemType.addItem("FIXED_AT_BUILD");
            jComboBoxItemType.addItem("PATCHABLE_IN_MODULE");
            jComboBoxItemType.addItem("DYNAMIC");
            jComboBoxItemType.addItem("DYNAMIC_EX");
            TableColumn itemTypeColumn = jTable.getColumnModel().getColumn(0);
            itemTypeColumn.setCellEditor(new DefaultCellEditor(jComboBoxItemType));
            //
            // Using combobox to display data type in table
            //
            JComboBox jComboBoxDataType = new JComboBox();
            jComboBoxDataType.addItem("UINT8");
            jComboBoxDataType.addItem("UINT16");
            jComboBoxDataType.addItem("UINT32");
            jComboBoxDataType.addItem("UINT64");
            jComboBoxDataType.addItem("VOID*");
            jComboBoxDataType.addItem("BOOLEAN");
            TableColumn dataTypeColumn = jTable.getColumnModel().getColumn(3);
            dataTypeColumn.setCellEditor(new DefaultCellEditor(jComboBoxDataType));

            if (sfc.getSpdPcdDefinitionCount() == 0) {

                return jTable;
            }
            saa = new String[sfc.getSpdPcdDefinitionCount()][5];
            sfc.getSpdPcdDefinitions(saa);
            int i = 0;
            while (i < saa.length) {
                model.addRow(saa[i]);
                i++;
            }

        }
        return jTable;
    }

    /**
     This method initializes jButton	
     	
     @return javax.swing.JButton	
     **/
    private JButton getJButton() {
        if (jButton == null) {
            jButton = new JButton();
            jButton.setBounds(new java.awt.Rectangle(499, 404, 77, 20));
            jButton.setText("Insert");
            jButton.addActionListener(this);
        }
        return jButton;
    }
} //  @jve:decl-index=0:visual-constraint="11,7"
