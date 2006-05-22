/** @file
  Java class PackagePkgHeader is GUI for create Package header elements of spd file.
 
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
package org.tianocore.packaging;

import java.awt.BorderLayout;
import java.awt.Dimension;
import java.awt.Toolkit;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.Vector;

import javax.swing.DefaultListModel;
import javax.swing.JPanel;
import javax.swing.JRadioButton;
import javax.swing.JTextField;
import javax.swing.JComboBox;
import javax.swing.JLabel;
import javax.swing.JList;
import javax.swing.JScrollPane;
import javax.swing.JButton;
import javax.swing.JFrame;

import org.tianocore.packaging.common.ui.StarLabel;

/**
 GUI for create Package header elements of spd file
  
 @since PackageEditor 1.0
**/
public class PackagePkgHeader extends JFrame implements ActionListener {
    private static String Separator = "::";

    private DefaultListModel listItem = new DefaultListModel();

    private SpdFileContents sfc = null;

    private JPanel jContentPane = null;

    private JRadioButton jRadioButtonAdd = null;

    private JRadioButton jRadioButtonSelect = null;

    private JTextField jTextFieldAdd = null;

    private JComboBox jComboBoxSelect = null;

    private JScrollPane jScrollPane = null;

    private JList jListLibraryClassDefinitions = null;

    private JButton jButtonAdd = null;

    private JButton jButtonRemove = null;

    private JButton jButtonClearAll = null;

    private JButton jButtonCancel = null;

    private JButton jButtonOk = null;

    private JLabel jLabel = null;

    private JTextField jTextField = null;

    private StarLabel starLabel = null;

    /**
     This method initializes this
     
     **/
    private void initialize() {
        this.setTitle("Package Headers");
        this.setDefaultCloseOperation(javax.swing.WindowConstants.DISPOSE_ON_CLOSE);

    }

    /**
     This method initializes jRadioButtonAdd	
     	
     @return javax.swing.JRadioButton	
     **/
    private JRadioButton getJRadioButtonAdd() {
        if (jRadioButtonAdd == null) {
            jRadioButtonAdd = new JRadioButton();
            jRadioButtonAdd.setBounds(new java.awt.Rectangle(10, 35, 205, 20));
            jRadioButtonAdd.setText("Add a new Module Type");
            jRadioButtonAdd.setEnabled(false);
            jRadioButtonAdd.addActionListener(this);
            jRadioButtonAdd.setSelected(false);
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
            jRadioButtonSelect.setBounds(new java.awt.Rectangle(10, 10, 205, 20));
            jRadioButtonSelect.setText("Select an existed Module Type");
            jRadioButtonSelect.setActionCommand("Select an existed Module Type");
            jRadioButtonSelect.addActionListener(this);
            jRadioButtonSelect.setSelected(true);
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
            jTextFieldAdd.setBounds(new java.awt.Rectangle(220, 35, 260, 20));
            jTextFieldAdd.setEditable(false);
            jTextFieldAdd.setEnabled(false);
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
            jComboBoxSelect.setEnabled(true);
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
            jScrollPane.setBounds(new java.awt.Rectangle(10, 121, 350, 174));
            jScrollPane.setViewportView(getJListLibraryClassDefinitions());
        }
        return jScrollPane;
    }

    /**
     This method initializes jListLibraryClassDefinitions	
     	
     @return javax.swing.JList	
     **/
    private JList getJListLibraryClassDefinitions() {
        if (jListLibraryClassDefinitions == null) {
            jListLibraryClassDefinitions = new JList(listItem);
        }
        return jListLibraryClassDefinitions;
    }

    /**
     This method initializes jButtonAdd	
     	
     @return javax.swing.JButton	
     **/
    private JButton getJButtonAdd() {
        if (jButtonAdd == null) {
            jButtonAdd = new JButton();
            jButtonAdd.setBounds(new java.awt.Rectangle(375, 132, 90, 20));
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
            jButtonRemove.setBounds(new java.awt.Rectangle(375, 230, 90, 20));
            jButtonRemove.setText("Remove");
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
            jButtonClearAll.setBounds(new java.awt.Rectangle(375, 260, 90, 20));
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
            jButtonOk.addActionListener(this);
        }
        return jButtonOk;
    }

    /**
     This is the default constructor
     **/
    public PackagePkgHeader(SpdFileContents sfc) {
        super();
        initialize();
        init();
        this.sfc = sfc;
    }

    /**
     Start the window at the center of screen
     *
     **/
    protected void centerWindow(int intWidth, int intHeight) {
        Dimension d = Toolkit.getDefaultToolkit().getScreenSize();
        this.setLocation((d.width - intWidth) / 2, (d.height - intHeight) / 2);
    }

    /**
     Start the window at the center of screen
     *
     **/
    protected void centerWindow() {
        centerWindow(this.getSize().width, this.getSize().height);
    }

    //	private void init(LibraryClassDefinitionsDocument.LibraryClassDefinitions inLibraryClassDefinitions) {
    //		init();
    //		this.setLibraryClassDefinitions(inLibraryClassDefinitions);
    //		int intLibraryCount = this.libraryClassDefinitions.getLibraryClassArray().length;
    //		if (intLibraryCount > 0) {
    //			for (int index = 0; index < intLibraryCount; index++) {
    //				listItem.addElement(this.libraryClassDefinitions.getLibraryClassArray(index).getUsage().toString() + 
    //									this.Separator + 
    //									this.libraryClassDefinitions.getLibraryClassArray(index).getStringValue());
    //				this.libraryClassDefinitions.getLibraryClassArray();
    //			}
    //		}
    //	}

    /**
     This method initializes this
     
     @return void
     **/
    private void init() {
        this.setContentPane(getJContentPane());
        this.setTitle("Library Class Declarations");
        this.setBounds(new java.awt.Rectangle(0, 0, 500, 370));
        this.centerWindow();
        initFrame();
    }

    /**
     This method initializes jContentPane
     
     @return javax.swing.JPanel
     **/
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            starLabel = new StarLabel();
            starLabel.setBounds(new java.awt.Rectangle(5, 85, 10, 20));
            jLabel = new JLabel();
            jLabel.setBounds(new java.awt.Rectangle(14, 85, 201, 22));
            jLabel.setText("Include Header for Selected Type");
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.add(getJRadioButtonAdd(), null);
            jContentPane.add(getJRadioButtonSelect(), null);
            jContentPane.add(getJTextFieldAdd(), null);
            jContentPane.add(getJComboBoxSelect(), null);
            jContentPane.add(getJScrollPane(), null);
            jContentPane.add(getJButtonAdd(), null);
            jContentPane.add(getJButtonRemove(), null);
            jContentPane.add(getJButtonClearAll(), null);
            jContentPane.add(getJButtonCancel(), null);
            jContentPane.add(getJButtonOk(), null);
            jContentPane.add(jLabel, null);
            jContentPane.add(getJTextField(), null);
            jContentPane.add(starLabel, null);
        }
        return jContentPane;
    }

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
        jComboBoxSelect.addItem("TOOLS");
        jComboBoxSelect.addItem("UEFI_DRIVER");
        jComboBoxSelect.addItem("UEFI_APPLICATION");
        jComboBoxSelect.addItem("USER_DEFINED");

    }

    public void actionPerformed(ActionEvent arg0) {
        if (arg0.getSource() == jButtonOk) {
            this.save();
            this.dispose();

        }
        if (arg0.getSource() == jButtonCancel) {
            this.dispose();
        }

        if (arg0.getSource() == jButtonAdd) {
            String strLibClass = "";

            if (jRadioButtonAdd.isSelected()) {
                strLibClass = jTextFieldAdd.getText();
            }
            if (jRadioButtonSelect.isSelected()) {
                strLibClass = jComboBoxSelect.getSelectedItem().toString();
            }

            listItem.addElement(jTextField.getText().replace('\\', '/') + Separator + strLibClass);
        }

        if (arg0.getSource() == jButtonRemove) {
            int intSelected[] = jListLibraryClassDefinitions.getSelectedIndices();
            if (intSelected.length > 0) {
                for (int index = intSelected.length - 1; index > -1; index--) {
                    listItem.removeElementAt(intSelected[index]);
                }
            }
            jListLibraryClassDefinitions.getSelectionModel().clearSelection();
        }

        if (arg0.getSource() == jButtonClearAll) {
            listItem.removeAllElements();
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

    private void save() {
        try {
            int intLibraryCount = listItem.getSize();

            if (intLibraryCount > 0) {

                for (int index = 0; index < intLibraryCount; index++) {
                    String strAll = listItem.get(index).toString();
                    String strInclude = strAll.substring(0, strAll.indexOf(Separator));
                    String strType = strAll.substring(strAll.indexOf(Separator) + Separator.length());
                    sfc.genSpdModuleHeaders(strType, strInclude, null, null, null, null, null, null);
                }
            } else {

            }

        } catch (Exception e) {
            System.out.println(e.toString());
        }
    }

    /**
     This method initializes jTextField	
     	
     @return javax.swing.JTextField	
     **/
    private JTextField getJTextField() {
        if (jTextField == null) {
            jTextField = new JTextField();
            jTextField.setBounds(new java.awt.Rectangle(221, 86, 257, 21));
        }
        return jTextField;
    }

}
