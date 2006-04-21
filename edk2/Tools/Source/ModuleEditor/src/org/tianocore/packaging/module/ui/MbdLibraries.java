/** @file
 
 The file is used to create, update MbdLibraries of a MBD file
 
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
import java.awt.event.ItemEvent;
import java.awt.event.ItemListener;
import java.util.Vector;

import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JComboBox;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JTextField;

import org.tianocore.LibrariesDocument;
import org.tianocore.LibraryUsage;
import org.tianocore.SupportedArchitectures;
import org.tianocore.common.DataValidation;
import org.tianocore.common.Log;
import org.tianocore.common.Tools;
import org.tianocore.packaging.common.ui.IDefaultMutableTreeNode;
import org.tianocore.packaging.common.ui.IInternalFrame;
import org.tianocore.packaging.common.ui.StarLabel;

/**
 The class is used to create, update MbdLibraries of a MBD file
 It extends IInternalFrame
 
 @since ModuleEditor 1.0

 **/
public class MbdLibraries extends IInternalFrame implements ItemListener {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = 8042998899875417568L;

    //
    //Define class members
    //
    private LibrariesDocument.Libraries libraries = null;

    private int location = -1;
    
    private int intSelectedItemId = 0;

    //
    //1 - Add; 2 - Update
    //
    private int operation = -1;

    private Vector<String> vName = new Vector<String>();

    private Vector<String> vGuid = new Vector<String>();

    private Vector<String> vLibraryClass = new Vector<String>();

    private Vector<String> vClassGuid = new Vector<String>();

    private Vector<String> vVersion = new Vector<String>();

    private Vector<String> vUsage = new Vector<String>();

    private Vector<String> vOverrideID = new Vector<String>();

    private JPanel jContentPane = null;

    private JButton jButtonOk = null;

    private JButton jButtonCancel = null;

    private JLabel jLabelName = null;

    private JTextField jTextFieldFileName = null;

    private JLabel jLabelGuid = null;

    private JTextField jTextFieldGuid = null;

    private JButton jButtonGenerateGuid = null;

    private JComboBox jComboBoxArch = null;

    private JLabel jLabelLibraryClass = null;

    private JTextField jTextFieldLibraryClass = null;

    private JLabel jLabelUsage = null;

    private JLabel jLabelClassGuid = null;

    private JTextField jTextFieldClassGuid = null;

    private JLabel jLabelOverrideID = null;

    private JTextField jTextFieldOverrideID = null;

    private JComboBox jComboBoxUsage = null;

    private StarLabel jStarLabel1 = null;

    private JComboBox jComboBoxFileList = null;

    private JButton jButtonAdd = null;

    private JButton jButtonRemove = null;

    private JButton jButtonUpdate = null;

    private JCheckBox jCheckBoxArch = null;

    private JButton jButtonGenerateGuid2 = null;

    private JLabel jLabelVersion = null;

    private JTextField jTextFieldVersion = null;

    /**
     This method initializes jButtonOk 
     
     @return javax.swing.JButton jButtonOk 
     
     **/
    private JButton getJButton() {
        if (jButtonOk == null) {
            jButtonOk = new JButton();
            jButtonOk.setText("OK");
            jButtonOk.setBounds(new java.awt.Rectangle(290, 240, 90, 20));
            jButtonOk.addActionListener(this);
        }
        return jButtonOk;
    }

    /**
     *This method initializes jButtonCancel 
     
     @return javax.swing.JButton jButtonCancel
     
     **/
    private JButton getJButtonCancel() {
        if (jButtonCancel == null) {
            jButtonCancel = new JButton();
            jButtonCancel.setText("Cancel");
            jButtonCancel.setBounds(new java.awt.Rectangle(390, 240, 90, 20));
            jButtonCancel.setPreferredSize(new java.awt.Dimension(90, 20));
            jButtonCancel.addActionListener(this);
        }
        return jButtonCancel;
    }

    /**
     This method initializes jTextFieldFileName 
     
     @return javax.swing.JTextField jTextFieldFileName
     
     **/
    private JTextField getJTextFieldSourceFilesDirectory() {
        if (jTextFieldFileName == null) {
            jTextFieldFileName = new JTextField();
            jTextFieldFileName.setBounds(new java.awt.Rectangle(160, 10, 320, 20));
        }
        return jTextFieldFileName;
    }

    /**
     This method initializes jTextFieldGuid 
     
     @return javax.swing.JTextField jTextFieldGuid
     
     **/
    private JTextField getJTextFieldGuid() {
        if (jTextFieldGuid == null) {
            jTextFieldGuid = new JTextField();
            jTextFieldGuid.setBounds(new java.awt.Rectangle(160, 35, 250, 20));
        }
        return jTextFieldGuid;
    }

    /**
     This method initializes jButtonGenerateGuid 
     
     @return javax.swing.JButton jButtonGenerateGuid
     
     **/
    private JButton getJButtonGenerateGuid() {
        if (jButtonGenerateGuid == null) {
            jButtonGenerateGuid = new JButton();
            jButtonGenerateGuid.setBounds(new java.awt.Rectangle(415, 35, 65, 20));
            jButtonGenerateGuid.setText("GEN");
            jButtonGenerateGuid.addActionListener(this);
        }
        return jButtonGenerateGuid;
    }

    /**
     This method initializes jComboBoxArch 
     
     @return javax.swing.JComboBox jComboBoxArch
     
     **/
    private JComboBox getJComboBoxArch() {
        if (jComboBoxArch == null) {
            jComboBoxArch = new JComboBox();
            jComboBoxArch.setBounds(new java.awt.Rectangle(140, 210, 340, 20));
            jComboBoxArch.setEnabled(false);
        }
        return jComboBoxArch;
    }

    /**
     This method initializes jTextFieldLibraryClass 
     
     @return javax.swing.JTextField jTextFieldLibraryClass
     
     **/
    private JTextField getJTextFieldLibraryClass() {
        if (jTextFieldLibraryClass == null) {
            jTextFieldLibraryClass = new JTextField();
            jTextFieldLibraryClass.setBounds(new java.awt.Rectangle(160, 60, 320, 20));
        }
        return jTextFieldLibraryClass;
    }

    /**
     This method initializes jTextFieldClassGuid 
     
     @return javax.swing.JTextField jTextFieldClassGuid
     
     **/
    private JTextField getJTextFieldClassGuid() {
        if (jTextFieldClassGuid == null) {
            jTextFieldClassGuid = new JTextField();
            jTextFieldClassGuid.setBounds(new java.awt.Rectangle(160, 85, 250, 20));
        }
        return jTextFieldClassGuid;
    }

    /**
     This method initializes jTextFieldOverrideID 
     
     @return javax.swing.JTextField jTextFieldOverrideID
     
     **/
    private JTextField getJTextFieldOverrideID() {
        if (jTextFieldOverrideID == null) {
            jTextFieldOverrideID = new JTextField();
            jTextFieldOverrideID.setBounds(new java.awt.Rectangle(160, 160, 50, 20));
        }
        return jTextFieldOverrideID;
    }

    /**
     This method initializes jComboBoxUsage 
     
     @return javax.swing.JComboBox jComboBoxUsage
     
     **/
    private JComboBox getJComboBoxUsage() {
        if (jComboBoxUsage == null) {
            jComboBoxUsage = new JComboBox();
            jComboBoxUsage.setBounds(new java.awt.Rectangle(160, 135, 320, 20));
        }
        return jComboBoxUsage;
    }

    /**
     This method initializes jComboBoxFileList 
     
     @return javax.swing.JComboBox jComboBoxFileList
     
     **/
    private JComboBox getJComboBoxFileList() {
        if (jComboBoxFileList == null) {
            jComboBoxFileList = new JComboBox();
            jComboBoxFileList.setBounds(new java.awt.Rectangle(15, 185, 210, 20));
            jComboBoxFileList.addItemListener(this);
            jComboBoxFileList.addActionListener(this);
        }
        return jComboBoxFileList;
    }

    /**
     This method initializes jButtonAdd 
     
     @return javax.swing.JButton jButtonAdd 
     
     **/
    private JButton getJButtonAdd() {
        if (jButtonAdd == null) {
            jButtonAdd = new JButton();
            jButtonAdd.setBounds(new java.awt.Rectangle(230, 185, 80, 20));
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
            jButtonRemove.setBounds(new java.awt.Rectangle(400, 185, 80, 20));
            jButtonRemove.setText("Remove");
            jButtonRemove.addActionListener(this);
        }
        return jButtonRemove;
    }

    /**
     This method initializes jButtonUpdate 
     
     @return javax.swing.JButton jButtonUpdate
     
     **/
    private JButton getJButtonUpdate() {
        if (jButtonUpdate == null) {
            jButtonUpdate = new JButton();
            jButtonUpdate.setBounds(new java.awt.Rectangle(315, 185, 80, 20));
            jButtonUpdate.setText("Update");
            jButtonUpdate.addActionListener(this);
        }
        return jButtonUpdate;
    }

    /**
     This method initializes jCheckBoxArch 
     
     @return javax.swing.JCheckBox jCheckBoxArch
     
     **/
    private JCheckBox getJCheckBoxArch() {
        if (jCheckBoxArch == null) {
            jCheckBoxArch = new JCheckBox();
            jCheckBoxArch.setBounds(new java.awt.Rectangle(10, 210, 119, 20));
            jCheckBoxArch.setText("Specific Arch");
            jCheckBoxArch.addActionListener(this);
        }
        return jCheckBoxArch;
    }

    /**
     This method initializes jButtonGenerateGuid2 
     
     @return javax.swing.JButton jButtonGenerateGuid2
     
     **/
    private JButton getJButtonGenerateGuid2() {
        if (jButtonGenerateGuid2 == null) {
            jButtonGenerateGuid2 = new JButton();
            jButtonGenerateGuid2.setBounds(new java.awt.Rectangle(415, 85, 65, 20));
            jButtonGenerateGuid2.setText("GEN");
            jButtonGenerateGuid2.addActionListener(this);
        }
        return jButtonGenerateGuid2;
    }

    /**
     This method initializes jTextFieldVersion 
     
     @return javax.swing.JTextField jTextFieldVersion
     
     **/
    private JTextField getJTextFieldVersion() {
        if (jTextFieldVersion == null) {
            jTextFieldVersion = new JTextField();
            jTextFieldVersion.setBounds(new java.awt.Rectangle(160, 110, 320, 20));
        }
        return jTextFieldVersion;
    }

    public static void main(String[] args) {

    }

    /**
     This is the default constructor
     
     **/
    public MbdLibraries() {
        super();
        init();
        this.setVisible(true);
    }

    /**
     This is the override edit constructor
     
     @param inLibraries The input LibrariesDocument.Libraries
     
     **/
    public MbdLibraries(LibrariesDocument.Libraries inLibraries) {
        super();
        init(inLibraries);
        this.setVisible(true);
    }

    /**
     This is the override edit constructor
     
     @param inLibraries The input LibrariesDocument.Libraries
     @param type The input data of node type
     @param index The input data of node index
     
     **/
    public MbdLibraries(LibrariesDocument.Libraries inLibraries, int type, int index) {
        super();
        init(inLibraries);
        this.setVisible(true);
    }

    /**
     This is the override edit constructor
     
     @param inLibraries The input LibrariesDocument.Libraries
     @param type The input data of node type
     @param index The input data of node index
     @param inOperation The input data of operation type
     
     **/
    public MbdLibraries(LibrariesDocument.Libraries inLibraries, int type, int index, int inOperation) {
        super();
        init(inLibraries, type, index, inOperation);
        this.operation = inOperation;
        this.setVisible(true);
    }

    /**
     This method initializes this
     
     @param inLibraries LibrariesDocument.Libraries
     
     **/
    private void init(LibrariesDocument.Libraries inLibraries) {
        init();
        this.setLibraries(inLibraries);
    }

    /**
     This method initializes this
     Fill values to all fields if these values are not empty
     
     @param inLibraries LibrariesDocument.Libraries
     @param type The input data of node type
     @param index The input data of node index
     @param inOperation The input data of operation type
     
     **/
    private void init(LibrariesDocument.Libraries inLibraries, int type, int index, int inOperation) {
        init(inLibraries);
        this.location = index;
        this.operation = inOperation;

        if (operation == 2) {
            this.jCheckBoxArch.setEnabled(false);
            this.jComboBoxArch.setEnabled(false);

            if (type == IDefaultMutableTreeNode.LIBRARIES_LIBRARY) {
                if (this.libraries.getLibraryList().size() > 0) {
                    for (int indexI = 0; indexI < this.libraries.getLibraryList().size(); indexI++) {
                        if (this.libraries.getLibraryArray(indexI).getStringValue() != null) {
                            vName.addElement(this.libraries.getLibraryArray(indexI).getStringValue());
                        } else {
                            vName.addElement("");
                        }
                        if (this.libraries.getLibraryArray(indexI).getGuid() != null) {
                            vGuid.addElement(this.libraries.getLibraryArray(indexI).getGuid());
                        } else {
                            vGuid.addElement("");
                        }
                        if (this.libraries.getLibraryArray(indexI).getLibraryClass() != null) {
                            vLibraryClass.addElement(this.libraries.getLibraryArray(indexI).getLibraryClass());
                        } else {
                            vLibraryClass.addElement("");
                        }
                        if (this.libraries.getLibraryArray(indexI).getClassGuid() != null) {
                            vClassGuid.addElement(this.libraries.getLibraryArray(indexI).getClassGuid());
                        } else {
                            vClassGuid.addElement("");
                        }
                        if (this.libraries.getLibraryArray(indexI).getVersion() != null) {
                            vVersion.addElement(this.libraries.getLibraryArray(indexI).getVersion());
                        } else {
                            vVersion.addElement("");
                        }
                        if (this.libraries.getLibraryArray(indexI).getUsage() != null) {
                            vUsage.addElement(this.libraries.getLibraryArray(indexI).getUsage().toString());
                        } else {
                            vUsage.addElement("ALWAYS_CONSUMED");
                        }
                        vOverrideID.addElement(String.valueOf(this.libraries.getLibraryArray(indexI).getOverrideID()));
                        jComboBoxFileList.addItem(this.libraries.getLibraryArray(indexI).getStringValue());
                    }
                }
            }
            if (type == IDefaultMutableTreeNode.LIBRARIES_ARCH_ITEM) {
                this.jCheckBoxArch.setSelected(true);
                this.jComboBoxArch.setSelectedItem(this.libraries.getArchArray(index).getArchType().toString());
                for (int indexI = 0; indexI < this.libraries.getArchArray(index).getLibraryList().size(); indexI++) {
                    if (this.libraries.getArchArray(index).getLibraryArray(indexI).getStringValue() != null) {
                        vName.addElement(this.libraries.getArchArray(index).getLibraryArray(indexI).getStringValue());
                    } else {
                        vName.addElement("");
                    }
                    if (this.libraries.getArchArray(index).getLibraryArray(indexI).getGuid() != null) {
                        vGuid.addElement(this.libraries.getArchArray(index).getLibraryArray(indexI).getGuid());
                    } else {
                        vGuid.addElement("");
                    }
                    if (this.libraries.getArchArray(index).getLibraryArray(indexI).getLibraryClass() != null) {
                        vLibraryClass.addElement(this.libraries.getArchArray(index).getLibraryArray(indexI)
                                                               .getLibraryClass());
                    } else {
                        vLibraryClass.addElement("");
                    }
                    if (this.libraries.getArchArray(index).getLibraryArray(indexI).getClassGuid() != null) {
                        vClassGuid
                                  .addElement(this.libraries.getArchArray(index).getLibraryArray(indexI).getClassGuid());
                    } else {
                        vClassGuid.addElement("");
                    }
                    if (this.libraries.getArchArray(index).getLibraryArray(indexI).getVersion() != null) {
                        vVersion.addElement(this.libraries.getArchArray(index).getLibraryArray(indexI).getVersion());
                    } else {
                        vVersion.addElement("");
                    }
                    if (this.libraries.getArchArray(index).getLibraryArray(indexI).getUsage() != null) {
                        vUsage.addElement(this.libraries.getArchArray(index).getLibraryArray(indexI).getUsage()
                                                        .toString());
                    } else {
                        vUsage.addElement("");
                    }
                    vOverrideID.addElement(String.valueOf(this.libraries.getArchArray(index).getLibraryArray(indexI).getOverrideID()));
                    jComboBoxFileList.addItem(this.libraries.getArchArray(index).getLibraryArray(indexI)
                                                            .getStringValue());
                }
            }
        }
    }

    /**
     This method initializes this
     
     **/
    private void init() {
        this.setSize(500, 515);
        this.setName("JFrame");
        this.setContentPane(getJContentPane());
        this.setTitle("Source Files");
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
            this.jTextFieldFileName.setEnabled(!isView);
            this.jTextFieldGuid.setEnabled(!isView);
            this.jButtonGenerateGuid.setEnabled(!isView);
            this.jComboBoxArch.setEnabled(!isView);
            this.jTextFieldLibraryClass.setEnabled(!isView);
            this.jTextFieldClassGuid.setEnabled(!isView);
            this.jTextFieldOverrideID.setEnabled(!isView);
            this.jComboBoxUsage.setEnabled(!isView);
            this.jButtonAdd.setEnabled(!isView);
            this.jButtonRemove.setEnabled(!isView);
            this.jButtonUpdate.setEnabled(!isView);
            this.jCheckBoxArch.setEnabled(!isView);
            this.jTextFieldVersion.setEnabled(!isView);

            this.jButtonOk.setEnabled(!isView);
            this.jButtonCancel.setEnabled(!isView);
            this.jButtonGenerateGuid.setEnabled(!isView);
            this.jButtonGenerateGuid2.setEnabled(!isView);
        }
    }

    /**
     This method initializes jContentPane
     
     @return javax.swing.JPanel jContentPane
     
     **/
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jLabelVersion = new JLabel();
            jLabelVersion.setBounds(new java.awt.Rectangle(15, 110, 140, 20));
            jLabelVersion.setText("Version");
            jLabelOverrideID = new JLabel();
            jLabelOverrideID.setBounds(new java.awt.Rectangle(15, 160, 140, 20));
            jLabelOverrideID.setText("Override ID");
            jLabelClassGuid = new JLabel();
            jLabelClassGuid.setBounds(new java.awt.Rectangle(15, 85, 140, 20));
            jLabelClassGuid.setText("Class Guid");
            jLabelUsage = new JLabel();
            jLabelUsage.setBounds(new java.awt.Rectangle(15, 135, 140, 20));
            jLabelUsage.setText("Usage");
            jLabelLibraryClass = new JLabel();
            jLabelLibraryClass.setBounds(new java.awt.Rectangle(15, 60, 140, 20));
            jLabelLibraryClass.setText("Library Class");
            jLabelGuid = new JLabel();
            jLabelGuid.setBounds(new java.awt.Rectangle(15, 35, 140, 20));
            jLabelGuid.setText("Guid");
            jLabelName = new JLabel();
            jLabelName.setText("Name");
            jLabelName.setBounds(new java.awt.Rectangle(15, 10, 140, 20));
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.add(getJButton(), null);
            jContentPane.add(getJButtonCancel(), null);
            jContentPane.add(jLabelName, null);
            jContentPane.add(getJTextFieldSourceFilesDirectory(), null);
            jContentPane.add(jLabelGuid, null);
            jContentPane.add(getJTextFieldGuid(), null);
            jContentPane.add(getJButtonGenerateGuid(), null);
            jContentPane.add(getJComboBoxArch(), null);
            jContentPane.add(jLabelLibraryClass, null);
            jContentPane.add(getJTextFieldLibraryClass(), null);
            jContentPane.add(jLabelUsage, null);
            jContentPane.add(jLabelClassGuid, null);
            jContentPane.add(getJTextFieldClassGuid(), null);
            jContentPane.add(getJButtonGenerateGuid2(), null);
            jContentPane.add(jLabelVersion, null);
            jContentPane.add(getJTextFieldVersion(), null);
            jContentPane.add(jLabelOverrideID, null);
            jContentPane.add(getJTextFieldOverrideID(), null);
            jContentPane.add(getJComboBoxUsage(), null);

            jStarLabel1 = new StarLabel();
            jStarLabel1.setLocation(new java.awt.Point(0, 10));

            jContentPane.add(jStarLabel1, null);
            jContentPane.add(getJComboBoxFileList(), null);
            jContentPane.add(getJButtonAdd(), null);
            jContentPane.add(getJButtonRemove(), null);
            jContentPane.add(getJButtonUpdate(), null);
            jContentPane.add(getJCheckBoxArch(), null);

        }
        return jContentPane;
    }

    /* (non-Javadoc)
     * @see java.awt.event.ActionListener#actionPerformed(java.awt.event.ActionEvent)
     *
     * Override actionPerformed to listen all actions
     * 
     */
    public void actionPerformed(ActionEvent arg0) {
        if (arg0.getSource() == jButtonOk) {
            this.setEdited(true);
            this.save();
            this.dispose();
        }
        if (arg0.getSource() == jButtonCancel) {
            this.dispose();
        }
        if (arg0.getSource() == jButtonGenerateGuid) {
            jTextFieldGuid.setText(Tools.generateUuidString());
        }
        if (arg0.getSource() == jButtonGenerateGuid2) {
            jTextFieldClassGuid.setText(Tools.generateUuidString());
        }
        if (arg0.getSource() == jButtonAdd) {
            if (!checkAdd()) {
                return;
            }
            addToList();
        }
        if (arg0.getSource() == jButtonRemove) {
            removeFromList();
        }
        if (arg0.getSource() == jButtonUpdate) {
            if (!checkAdd()) {
                return;
            }
            updateForList();
        }
        if (arg0.getSource() == jCheckBoxArch) {
            if (this.jCheckBoxArch.isSelected()) {
                this.jComboBoxArch.setEnabled(true);
            } else {
                this.jComboBoxArch.setEnabled(false);
            }
        }
    }

    /**
     Init the items of Usage and Arch
     
     **/
    private void initFrame() {
        jComboBoxUsage.addItem("ALWAYS_CONSUMED");
        jComboBoxUsage.addItem("SOMETIMES_CONSUMED");
        jComboBoxUsage.addItem("ALWAYS_PRODUCED");
        jComboBoxUsage.addItem("SOMETIMES_PRODUCED");
        jComboBoxUsage.addItem("DEFAULT");
        jComboBoxUsage.addItem("PRIVATE");

        jComboBoxArch.addItem("ALL");
        jComboBoxArch.addItem("EBC");
        jComboBoxArch.addItem("ARM");
        jComboBoxArch.addItem("IA32");
        jComboBoxArch.addItem("X64");
        jComboBoxArch.addItem("IPF");
        jComboBoxArch.addItem("PPC");
    }

    /**
     Add current item to Vector
     
     **/
    private void addToList() {
        intSelectedItemId = vName.size();
        vName.addElement(this.jTextFieldFileName.getText());
        vGuid.addElement(this.jTextFieldGuid.getText());
        vLibraryClass.addElement(this.jTextFieldLibraryClass.getText());
        vClassGuid.addElement(this.jTextFieldClassGuid.getText());
        vVersion.addElement(this.jTextFieldVersion.getText());
        vUsage.addElement(this.jComboBoxUsage.getSelectedItem().toString());
        vOverrideID.addElement(this.jTextFieldOverrideID.getText());
        jComboBoxFileList.addItem(this.jTextFieldFileName.getText());
        jComboBoxFileList.setSelectedItem(this.jTextFieldFileName.getText());
        
        //
        // Reset select item index
        //
        intSelectedItemId = vName.size();
        
        //
        // Reload all fields of selected item
        //
        reloadFromList();
    }

    /**
     Remove current item from Vector
     
     **/
    private void removeFromList() {
        int intTempIndex = intSelectedItemId;
        if (vName.size() < 1) {
            return;
        }
        
        jComboBoxFileList.removeItemAt(intSelectedItemId);
        
        vName.removeElementAt(intTempIndex);
        vGuid.removeElementAt(intTempIndex);
        vLibraryClass.removeElementAt(intTempIndex);
        vClassGuid.removeElementAt(intTempIndex);
        vVersion.removeElementAt(intTempIndex);
        vUsage.removeElementAt(intTempIndex);
        vOverrideID.removeElementAt(intTempIndex);
        
        //
        // Reload all fields of selected item
        //
        reloadFromList();
    }

    /**
     Update current item of Vector
     
     **/
    private void updateForList() {
        //
        // Backup selected item index
        //
        int intTempIndex = intSelectedItemId;
        vName.setElementAt(this.jTextFieldFileName.getText(), intSelectedItemId);
        vGuid.setElementAt(this.jTextFieldGuid.getText(), intSelectedItemId);
        vLibraryClass.setElementAt(this.jTextFieldLibraryClass.getText(), intSelectedItemId);
        vClassGuid.setElementAt(this.jTextFieldClassGuid.getText(), intSelectedItemId);
        vVersion.setElementAt(this.jTextFieldVersion.getText(), intSelectedItemId);
        vUsage.setElementAt(this.jComboBoxUsage.getSelectedItem().toString(), intSelectedItemId);
        vOverrideID.setElementAt(this.jTextFieldOverrideID.getText(), intSelectedItemId);
        
        jComboBoxFileList.removeAllItems();
        for (int index = 0; index < vName.size(); index++) {
            jComboBoxFileList.addItem(vName.elementAt(index));
        }
        
        //
        // Restore selected item index
        //
        intSelectedItemId = intTempIndex;
        
        //
        // Reset select item index
        //
        jComboBoxFileList.setSelectedIndex(intSelectedItemId);
        
        //
        // Reload all fields of selected item
        //
        reloadFromList();
    }

    /**
     Refresh all fields' values of selected item of Vector
     
     **/
    private void reloadFromList() {
        if (vName.size() > 0) {
            //
            // Get selected item index
            //
            intSelectedItemId = jComboBoxFileList.getSelectedIndex();
            
            this.jTextFieldFileName.setText(vName.elementAt(intSelectedItemId).toString());
            this.jComboBoxUsage.setSelectedItem(vUsage.elementAt(intSelectedItemId).toString());
            this.jTextFieldGuid.setText(vGuid.elementAt(intSelectedItemId).toString());
            this.jTextFieldLibraryClass.setText(vLibraryClass.elementAt(intSelectedItemId).toString());
            this.jTextFieldClassGuid.setText(vClassGuid.elementAt(intSelectedItemId).toString());
            this.jTextFieldVersion.setText(vVersion.elementAt(intSelectedItemId).toString());
            this.jTextFieldOverrideID.setText(vOverrideID.elementAt(intSelectedItemId).toString());
        } else {
            this.jTextFieldFileName.setText("");
            this.jComboBoxUsage.setSelectedIndex(0);
            this.jTextFieldGuid.setText("");
            this.jTextFieldLibraryClass.setText("");
            this.jTextFieldClassGuid.setText("");
            this.jTextFieldVersion.setText("");
            this.jTextFieldOverrideID.setText("");
        }
    }

    /* (non-Javadoc)
     * @see java.awt.event.ItemListener#itemStateChanged(java.awt.event.ItemEvent)
     *
     * Reflesh the frame when selected item changed 
     */
    public void itemStateChanged(ItemEvent arg0) {
        if (arg0.getStateChange() == ItemEvent.SELECTED) {
            reloadFromList();
        }
    }

    /**
     Data validation for all fields before save
     
     @retval true - All datas are valid
     @retval false - At least one data is invalid
     
     **/
    public boolean check() {
        if (this.jComboBoxFileList.getItemCount() < 1) {
            Log.err("Must have one Library at least!");
            return false;
        }
        return true;
    }

    /**
     Data validation for all fields before add current item to Vector
     
     @retval true - All datas are valid
     @retval false - At least one data is invalid
     
     **/
    public boolean checkAdd() {
        //
        // Check if all required fields are not empty
        //
        if (isEmpty(this.jTextFieldFileName.getText())) {
            Log.err("File Name couldn't be empty");
            return false;
        }

        //
        // Check if all fields have correct data types 
        //
        if (!DataValidation.isBaseName(this.jTextFieldFileName.getText())) {
            Log.err("Incorrect data type for Name");
            return false;
        }
        if (!isEmpty(this.jTextFieldGuid.getText()) && !DataValidation.isGuid(this.jTextFieldGuid.getText())) {
            Log.err("Incorrect data type for Guid");
            return false;
        }
        if (!isEmpty(this.jTextFieldLibraryClass.getText())
            && !DataValidation.isPath(this.jTextFieldLibraryClass.getText())) {
            Log.err("Incorrect data type for Path");
            return false;
        }
        if (!isEmpty(this.jTextFieldClassGuid.getText()) && !DataValidation.isGuid(this.jTextFieldClassGuid.getText())) {
            Log.err("Incorrect data type for Guid");
            return false;
        }
        if (!isEmpty(this.jTextFieldOverrideID.getText())
            && !DataValidation.isOverrideID(this.jTextFieldOverrideID.getText())) {
            Log.err("Incorrect data type for Override ID");
            return false;
        }

        return true;
    }

    /**
     Save all components of Mbd Libraries
     if exists libraries, set the value directly
     if not exists libraries, new an instance first
     
     **/
    public void save() {
        try {
            if (this.libraries == null) {
                libraries = LibrariesDocument.Libraries.Factory.newInstance();
            }
            //
            //Save as file name
            //
            if (!this.jCheckBoxArch.isSelected()) {
                if (this.operation == 2) { //Add new filename
                    //
                    //First remove all existed filename
                    //
                    if (libraries.getLibraryList().size() > 0) {
                        for (int index = libraries.getLibraryList().size() - 1; index >= 0; index--) {
                            libraries.removeLibrary(index);
                        }
                    }
                }
                LibrariesDocument.Libraries.Library library = LibrariesDocument.Libraries.Library.Factory.newInstance();
                for (int index = 0; index < vName.size(); index++) {
                    if (!isEmpty(vName.elementAt(index).toString())) {
                        library.setStringValue(vName.elementAt(index).toString());
                    }
                    if (!isEmpty(vGuid.elementAt(index).toString())) {
                        library.setGuid(vGuid.elementAt(index).toString());
                    }
                    if (!isEmpty(vLibraryClass.elementAt(index).toString())) {
                        library.setLibraryClass(vLibraryClass.elementAt(index).toString());
                    }
                    if (!isEmpty(vClassGuid.elementAt(index).toString())) {
                        library.setClassGuid(vClassGuid.elementAt(index).toString());
                    }
                    if (!isEmpty(vVersion.elementAt(index).toString())) {
                        library.setVersion(vVersion.elementAt(index).toString());
                    }
                    if (!isEmpty(vUsage.elementAt(index).toString())) {
                        library.setUsage(LibraryUsage.Enum.forString(vUsage.elementAt(index).toString()));
                    }
                    if (!isEmpty(vOverrideID.elementAt(index).toString())) {
                        library.setOverrideID(Integer.parseInt(vOverrideID.elementAt(index).toString()));
                    }
                    libraries.addNewLibrary();
                    libraries.setLibraryArray(libraries.getLibraryList().size() - 1, library);
                }
            }
            //
            //Save as Arch
            //
            if (this.jCheckBoxArch.isSelected()) {
                LibrariesDocument.Libraries.Arch arch = LibrariesDocument.Libraries.Arch.Factory.newInstance();
                if (this.operation == 2) {
                    //First remove all existed filename
                    for (int index = libraries.getArchArray(location).getLibraryList().size() - 1; index >= 0; index--) {
                        libraries.getArchArray(location).removeLibrary(index);
                    }
                }
                for (int index = 0; index < vName.size(); index++) {
                    LibrariesDocument.Libraries.Arch.Library library = LibrariesDocument.Libraries.Arch.Library.Factory.newInstance();
                    if (!isEmpty(vName.elementAt(index).toString())) {
                        library.setStringValue(vName.elementAt(index).toString());
                    }
                    if (!isEmpty(vGuid.elementAt(index).toString())) {
                        library.setGuid(vGuid.elementAt(index).toString());
                    }
                    if (!isEmpty(vLibraryClass.elementAt(index).toString())) {
                        library.setLibraryClass(vLibraryClass.elementAt(index).toString());
                    }
                    if (!isEmpty(vClassGuid.elementAt(index).toString())) {
                        library.setClassGuid(vClassGuid.elementAt(index).toString());
                    }
                    if (!isEmpty(vVersion.elementAt(index).toString())) {
                        library.setVersion(vVersion.elementAt(index).toString());
                    }
                    if (!isEmpty(vUsage.elementAt(index).toString())) {
                        library.setUsage(LibraryUsage.Enum.forString(vUsage.elementAt(index).toString()));
                    }
                    if (!isEmpty(vOverrideID.elementAt(index).toString())) {
                        library.setOverrideID(Integer.parseInt(vOverrideID.elementAt(index).toString()));
                    }
                    arch.addNewLibrary();
                    arch.setLibraryArray(arch.getLibraryList().size() - 1, library);
                }
                arch
                    .setArchType(SupportedArchitectures.Enum.forString(this.jComboBoxArch.getSelectedItem().toString()));
                if (location > -1) {
                    libraries.setArchArray(location, arch);
                } else {
                    libraries.addNewArch();
                    libraries.setArchArray(libraries.getArchList().size() - 1, arch);
                }
            }
        } catch (Exception e) {
            Log.err("Update Source Files", e.getMessage());
        }
    }

    /**
     Get LibrariesDocument.Libraries
     
     @return LibrariesDocument.Libraries
     
     **/
    public LibrariesDocument.Libraries getLibraries() {
        return libraries;
    }

    /**
     Set LibrariesDocument.Libraries
     
     @param libraries The input LibrariesDocument.Libraries
     
     **/
    public void setLibraries(LibrariesDocument.Libraries libraries) {
        this.libraries = libraries;
    }
}
