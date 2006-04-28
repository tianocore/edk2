/** @file
 
 The file is used to create, update Library Class Definition of MSA/MBD file
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/
package org.tianocore.packaging.module.ui;

import java.awt.event.ActionEvent;

import javax.swing.DefaultListModel;
import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.JLabel;
import javax.swing.JList;
import javax.swing.JPanel;
import javax.swing.JRadioButton;
import javax.swing.JScrollPane;
import javax.swing.JTextField;

import org.tianocore.LibraryClassDefinitionsDocument;
import org.tianocore.LibraryClassDocument;
import org.tianocore.LibraryUsage;
import org.tianocore.common.DataValidation;
import org.tianocore.common.Log;
import org.tianocore.packaging.common.ui.IInternalFrame;

/**
 The class is used to create, update Library Class Definition of MSA/MBD file
 It extends IInternalFrame
 
 @since ModuleEditor 1.0

 **/
public class ModuleLibraryClassDefinitions extends IInternalFrame {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = -1743248695411382857L;

    //
    //Define class members
    //
    private static String Separator = "::";

    private DefaultListModel listItem = new DefaultListModel();

    private LibraryClassDefinitionsDocument.LibraryClassDefinitions libraryClassDefinitions = null;

    private JPanel jContentPane = null;

    private JRadioButton jRadioButtonAdd = null;

    private JRadioButton jRadioButtonSelect = null;

    private JTextField jTextFieldAdd = null;

    private JComboBox jComboBoxSelect = null;

    private JLabel jLabelUsage = null;

    private JComboBox jComboBoxUsage = null;

    private JScrollPane jScrollPane = null;

    private JList jListLibraryClassDefinitions = null;

    private JButton jButtonAdd = null;

    private JButton jButtonRemove = null;

    private JButton jButtonClearAll = null;

    private JButton jButtonCancel = null;

    private JButton jButtonOk = null;

    /**
     This method initializes jRadioButtonAdd 
     
     @return javax.swing.JRadioButton jRadioButtonAdd
     
     **/
    private JRadioButton getJRadioButtonAdd() {
        if (jRadioButtonAdd == null) {
            jRadioButtonAdd = new JRadioButton();
            jRadioButtonAdd.setBounds(new java.awt.Rectangle(15, 35, 205, 20));
            jRadioButtonAdd.setText("Add a new Library Class");
            jRadioButtonAdd.addActionListener(this);
            jRadioButtonAdd.setSelected(false);
        }
        return jRadioButtonAdd;
    }

    /**
     This method initializes jRadioButtonSelect 
     
     @return javax.swing.JRadioButton jRadioButtonSelect
     
     **/
    private JRadioButton getJRadioButtonSelect() {
        if (jRadioButtonSelect == null) {
            jRadioButtonSelect = new JRadioButton();
            jRadioButtonSelect.setBounds(new java.awt.Rectangle(15, 10, 205, 20));
            jRadioButtonSelect.setText("Select an existing Library Class");
            jRadioButtonSelect.addActionListener(this);
            jRadioButtonSelect.setSelected(true);
        }
        return jRadioButtonSelect;
    }

    /**
     This method initializes jTextFieldAdd 
     
     @return javax.swing.JTextField jTextFieldAdd
     
     **/
    private JTextField getJTextFieldAdd() {
        if (jTextFieldAdd == null) {
            jTextFieldAdd = new JTextField();
            jTextFieldAdd.setBounds(new java.awt.Rectangle(220, 35, 260, 20));
            jTextFieldAdd.setEnabled(false);
        }
        return jTextFieldAdd;
    }

    /**
     This method initializes jComboBoxSelect 
     
     @return javax.swing.JComboBox jComboBoxSelect
     
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
     This method initializes jComboBoxUsage 
     
     @return javax.swing.JComboBox jComboBoxUsage
     
     **/
    private JComboBox getJComboBoxUsage() {
        if (jComboBoxUsage == null) {
            jComboBoxUsage = new JComboBox();
            jComboBoxUsage.setBounds(new java.awt.Rectangle(220, 60, 260, 20));
        }
        return jComboBoxUsage;
    }

    /**
     This method initializes jScrollPane 
     
     @return javax.swing.JScrollPane jScrollPane
     
     **/
    private JScrollPane getJScrollPane() {
        if (jScrollPane == null) {
            jScrollPane = new JScrollPane();
            jScrollPane.setBounds(new java.awt.Rectangle(15, 95, 350, 200));
            jScrollPane.setViewportView(getJListLibraryClassDefinitions());
        }
        return jScrollPane;
    }

    /**
     This method initializes jListLibraryClassDefinitions 
     
     @return javax.swing.JList jListLibraryClassDefinitions
     
     **/
    private JList getJListLibraryClassDefinitions() {
        if (jListLibraryClassDefinitions == null) {
            jListLibraryClassDefinitions = new JList(listItem);
        }
        return jListLibraryClassDefinitions;
    }

    /**
     This method initializes jButtonAdd 
     
     @return javax.swing.JButton jButtonAdd
     
     **/
    private JButton getJButtonAdd() {
        if (jButtonAdd == null) {
            jButtonAdd = new JButton();
            jButtonAdd.setBounds(new java.awt.Rectangle(380, 115, 90, 20));
            jButtonAdd.setText("Add");
            jButtonAdd.addActionListener(this);
        }
        return jButtonAdd;
    }

    /**
     This method initializes jButtonRemove
     
     @return javax.swing.JButton jButtonRemove 
     
     **/
    private JButton getJButtonRemove() {
        if (jButtonRemove == null) {
            jButtonRemove = new JButton();
            jButtonRemove.setBounds(new java.awt.Rectangle(380, 230, 90, 20));
            jButtonRemove.setText("Remove");
            jButtonRemove.addActionListener(this);
        }
        return jButtonRemove;
    }

    /**
     This method initializes jButtonRemoveAll 
     
     @return javax.swing.JButton jButtonClearAll
     
     **/
    private JButton getJButtonClearAll() {
        if (jButtonClearAll == null) {
            jButtonClearAll = new JButton();
            jButtonClearAll.setBounds(new java.awt.Rectangle(380, 260, 90, 20));
            jButtonClearAll.setText("Clear All");
            jButtonClearAll.addActionListener(this);
        }
        return jButtonClearAll;
    }

    /**
     This method initializes jButtonCancel 
     
     @return javax.swing.JButton jButtonCancel
     
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
     This method initializes jButtonOk 
     
     @return javax.swing.JButton jButtonOk
     
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

    public static void main(String[] args) {

    }

    /**
     This is the default constructor
     
     **/
    public ModuleLibraryClassDefinitions() {
        super();
        init();
        this.setVisible(true);
    }

    /**
     This is the override edit constructor
     
     @param inLibraryClassDefinitions The input data of LibraryClassDefinitionsDocument.LibraryClassDefinitions
     
     **/
    public ModuleLibraryClassDefinitions(
                                         LibraryClassDefinitionsDocument.LibraryClassDefinitions inLibraryClassDefinitions) {
        super();
        init(inLibraryClassDefinitions);
        this.setVisible(true);
    }

    /**
     This method initializes this
     Fill values to all fields if these values are not empty
     
     @param inLibraryClassDefinitions The input data of LibraryClassDefinitionsDocument.LibraryClassDefinitions
     
     **/
    private void init(LibraryClassDefinitionsDocument.LibraryClassDefinitions inLibraryClassDefinitions) {
        init();
        this.setLibraryClassDefinitions(inLibraryClassDefinitions);
        if (this.libraryClassDefinitions != null) {
            if (this.libraryClassDefinitions.getLibraryClassList().size() > 0) {
                for (int index = 0; index < this.libraryClassDefinitions.getLibraryClassList().size(); index++) {
                    listItem.addElement(this.libraryClassDefinitions.getLibraryClassArray(index).getUsage().toString()
                                        + ModuleLibraryClassDefinitions.Separator
                                        + this.libraryClassDefinitions.getLibraryClassArray(index).getStringValue());
                    this.libraryClassDefinitions.getLibraryClassList();
                }
            }
        }
    }

    /**
     This method initializes this
     
     **/
    private void init() {
        this.setContentPane(getJContentPane());
        this.setTitle("Library Class Definitions");
        this.setBounds(new java.awt.Rectangle(0, 0, 500, 515));
        //this.centerWindow();
        initFrame();
        this.setViewMode(false);
    }

    /**
     Disable all components when the mode is view
     
     @param isView true - The view mode; false - The non-view mode
     
     **/
    public void setViewMode(boolean isView) {
        this.jButtonOk.setVisible(false);
        this.jButtonCancel.setVisible(false);
        if (isView) {
            this.jRadioButtonAdd.setEnabled(!isView);
            this.jRadioButtonSelect.setEnabled(!isView);
            this.jTextFieldAdd.setEnabled(!isView);
            this.jComboBoxSelect.setEnabled(!isView);
            this.jComboBoxUsage.setEnabled(!isView);
            this.jButtonAdd.setEnabled(!isView);
            this.jButtonRemove.setEnabled(!isView);
            this.jButtonClearAll.setEnabled(!isView);
            this.jButtonCancel.setEnabled(!isView);
            this.jButtonOk.setEnabled(!isView);
        }
    }

    /**
     This method initializes jContentPane
     
     @return javax.swing.JPanel jContentPane
     
     **/
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jLabelUsage = new JLabel();
            jLabelUsage.setBounds(new java.awt.Rectangle(15, 60, 205, 20));
            jLabelUsage.setText("Usage");
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.add(getJRadioButtonAdd(), null);
            jContentPane.add(getJRadioButtonSelect(), null);
            jContentPane.add(getJTextFieldAdd(), null);
            jContentPane.add(getJComboBoxSelect(), null);
            jContentPane.add(jLabelUsage, null);
            jContentPane.add(getJComboBoxUsage(), null);
            jContentPane.add(getJScrollPane(), null);
            jContentPane.add(getJButtonAdd(), null);
            jContentPane.add(getJButtonRemove(), null);
            jContentPane.add(getJButtonClearAll(), null);
            jContentPane.add(getJButtonCancel(), null);
            jContentPane.add(getJButtonOk(), null);
        }
        return jContentPane;
    }

    /**
     This method initializes all existing libraries and usage types
     
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

        jComboBoxUsage.addItem("ALWAYS_CONSUMED");
        jComboBoxUsage.addItem("SOMETIMES_CONSUMED");
        jComboBoxUsage.addItem("ALWAYS_PRODUCED");
        jComboBoxUsage.addItem("SOMETIMES_PRODUCED");
        jComboBoxUsage.addItem("DEFAULT");
        jComboBoxUsage.addItem("PRIVATE");
    }

    /* (non-Javadoc)
     * @see java.awt.event.ActionListener#actionPerformed(java.awt.event.ActionEvent)
     *
     * Override actionPerformed to listen all actions
     * 
     */
    public void actionPerformed(ActionEvent arg0) {
        if (arg0.getSource() == jButtonOk) {
            this.dispose();
            this.setEdited(true);
            this.save();
        }
        if (arg0.getSource() == jButtonCancel) {
            this.dispose();
            this.setEdited(false);
        }

        //
        // Add current Library and its usage to the list
        //
        if (arg0.getSource() == jButtonAdd) {
            if (!checkAdd()) {
                return;
            }
            String strLibClass = "";
            String strUsage = "";
            if (jRadioButtonAdd.isSelected()) {
                strLibClass = jTextFieldAdd.getText();
            }
            if (jRadioButtonSelect.isSelected()) {
                strLibClass = jComboBoxSelect.getSelectedItem().toString();
            }
            strUsage = jComboBoxUsage.getSelectedItem().toString();
            listItem.addElement(strUsage + ModuleLibraryClassDefinitions.Separator + strLibClass);
        }

        //
        // Remove current Library and its usage of the list
        //
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

        //
        // Contorl the selected status when click RadionButton
        // Do not use Radio Button Group
        //
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

    /**
     Data validation for all fields
     
     @retval true - All datas are valid
     @retval false - At least one data is invalid
     
     **/
    public boolean check() {
        if (listItem.getSize() < 1) {
            Log.err("Must have one Library Class at least!");
            return false;
        }
        return true;
    }

    /**
     Data validation for all fields
     
     @retval true - All datas are valid
     @retval false - At least one data is invalid
     
     **/
    public boolean checkAdd() {
        //
        // Check if all required fields are not empty
        //
        if (this.jRadioButtonAdd.isSelected() && isEmpty(this.jTextFieldAdd.getText())) {
            Log.err("Library Class couldn't be empty");
            return false;
        }

        //
        // Check if all fields have correct data types 
        //
        if (this.jRadioButtonAdd.isSelected() && !DataValidation.isLibraryClass(this.jTextFieldAdd.getText())) {
            Log.err("Incorrect data type for Library Class");
            return false;
        }
        return true;
    }

    /**
     Save all components of Mbd Header
     if exists mbdHeader, set the value directly
     if not exists mbdHeader, new an instance first
     
     **/
    public void save() {
        try {
            int intLibraryCount = listItem.getSize();

            if (this.libraryClassDefinitions == null) {
                libraryClassDefinitions = LibraryClassDefinitionsDocument.LibraryClassDefinitions.Factory.newInstance();
            }

            if (intLibraryCount > 0) {
                LibraryClassDocument.LibraryClass mLibraryClass = LibraryClassDocument.LibraryClass.Factory
                                                                                                           .newInstance();
                for (int index = this.libraryClassDefinitions.getLibraryClassList().size() - 1; index > -1; index--) {
                    this.libraryClassDefinitions.removeLibraryClass(index);
                }
                for (int index = 0; index < intLibraryCount; index++) {
                    String strAll = listItem.get(index).toString();
                    String strUsage = strAll.substring(0, strAll.indexOf(ModuleLibraryClassDefinitions.Separator));
                    String strLibraryClass = strAll.substring(strAll.indexOf(ModuleLibraryClassDefinitions.Separator)
                                                              + ModuleLibraryClassDefinitions.Separator.length());
                    mLibraryClass.setStringValue(strLibraryClass);
                    mLibraryClass.setUsage(LibraryUsage.Enum.forString(strUsage));
                    this.libraryClassDefinitions.addNewLibraryClass();
                    this.libraryClassDefinitions.setLibraryClassArray(index, mLibraryClass);
                }
            } else {
                this.libraryClassDefinitions.setNil();
            }
        } catch (Exception e) {
            Log.err("Update Library Class Definitions", e.getMessage());
        }
    }

    /**
     Get LibraryClassDefinitionsDocument.LibraryClassDefinitions
     
     @return LibraryClassDefinitionsDocument.LibraryClassDefinitions
     
     **/
    public LibraryClassDefinitionsDocument.LibraryClassDefinitions getLibraryClassDefinitions() {
        return libraryClassDefinitions;
    }

    /**
     Set LibraryClassDefinitionsDocument.LibraryClassDefinitions
     
     @param libraryClassDefinitions The input data of LibraryClassDefinitionsDocument.LibraryClassDefinitions
     
     **/
    public void setLibraryClassDefinitions(
                                           LibraryClassDefinitionsDocument.LibraryClassDefinitions libraryClassDefinitions) {
        this.libraryClassDefinitions = libraryClassDefinitions;
    }
}
