/** @file
 
 The file is used to create, update Include of MSA/MBD file
 
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

import org.tianocore.IncludesDocument;
import org.tianocore.PackageNameDocument;
import org.tianocore.PackageType;
import org.tianocore.PackageUsage;
import org.tianocore.SupportedArchitectures;
import org.tianocore.common.DataValidation;
import org.tianocore.common.Log;
import org.tianocore.packaging.common.ui.IDefaultMutableTreeNode;
import org.tianocore.packaging.common.ui.IInternalFrame;
import org.tianocore.packaging.common.ui.StarLabel;

/**
 The class is used to create, update Include of MSA/MBD file
 It extends IInternalFrame
 
 @since ModuleEditor 1.0

 **/
public class ModuleIncludes extends IInternalFrame implements ItemListener {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = 3465193035145152131L;

    //
    //Define class members
    //
    private IncludesDocument.Includes includes = null;

    private int location = -1;
    
    private int intSelectedItemId = 0;

    //
    //  1 - Add; 2 - Update
    //
    private int operation = -1;

    private Vector<String> vPackageName = new Vector<String>();

    private Vector<String> vUsage = new Vector<String>();

    private Vector<String> vPackageType = new Vector<String>();

    private Vector<String> vUpdatedDate = new Vector<String>();

    private JPanel jContentPane = null;

    private JButton jButtonOk = null;

    private JButton jButtonCancel = null;

    private JLabel jLabelPackageName = null;

    private JLabel jLabelPackageType = null;

    private JComboBox jComboBoxPackageType = null;

    private JLabel jLabelUsage = null;

    private JComboBox jComboBoxUsage = null;

    private StarLabel jStarLabel1 = null;

    private JTextField jTextFieldPackageName = null;

    private JComboBox jComboBoxFileList = null;

    private JButton jButtonAdd = null;

    private JButton jButtonUpdate = null;

    private JButton jButtonRemove = null;

    private JLabel jLabelUpdatedDate = null;

    private JTextField jTextFieldUpdatedDate = null;

    private JCheckBox jCheckBoxArch = null;

    private JComboBox jComboBoxArch = null;

    /**
     This method initializes jButtonOk 
     
     @return javax.swing.JButton jButtonOk
     
     **/
    private JButton getJButton() {
        if (jButtonOk == null) {
            jButtonOk = new JButton();
            jButtonOk.setText("OK");
            jButtonOk.setBounds(new java.awt.Rectangle(290, 165, 90, 20));
            jButtonOk.addActionListener(this);

        }
        return jButtonOk;
    }

    /**
     This method initializes jButtonCancel 
     
     @return javax.swing.JButton jButtonCancel
     
     **/
    private JButton getJButton1() {
        if (jButtonCancel == null) {
            jButtonCancel = new JButton();
            jButtonCancel.setText("Cancel");
            jButtonCancel.setBounds(new java.awt.Rectangle(390, 165, 90, 20));
            jButtonCancel.addActionListener(this);
        }
        return jButtonCancel;
    }

    /**
     This method initializes jComboBoxPackageType 
     
     @return javax.swing.JComboBox jComboBoxPackageType
     
     **/
    private JComboBox getJComboBoxPackageType() {
        if (jComboBoxPackageType == null) {
            jComboBoxPackageType = new JComboBox();
            jComboBoxPackageType.setBounds(new java.awt.Rectangle(160, 35, 320, 20));
        }
        return jComboBoxPackageType;
    }

    /**
     This method initializes jComboBoxUsage 
     
     @return javax.swing.JComboBox jComboBoxUsage
     
     **/
    private JComboBox getJComboBoxUsage() {
        if (jComboBoxUsage == null) {
            jComboBoxUsage = new JComboBox();
            jComboBoxUsage.setBounds(new java.awt.Rectangle(160, 60, 320, 20));
        }
        return jComboBoxUsage;
    }

    /**
     This method initializes jTextFieldPackageName 
     
     @return javax.swing.JTextField jTextFieldPackageName
     
     **/
    private JTextField getJTextFieldPackageName() {
        if (jTextFieldPackageName == null) {
            jTextFieldPackageName = new JTextField();
            jTextFieldPackageName.setBounds(new java.awt.Rectangle(160, 10, 320, 20));
        }
        return jTextFieldPackageName;
    }

    /**
     This method initializes jComboBoxFileList 
     
     @return javax.swing.JComboBox jComboBoxFileList
     
     **/
    private JComboBox getJComboBoxFileList() {
        if (jComboBoxFileList == null) {
            jComboBoxFileList = new JComboBox();
            jComboBoxFileList.setBounds(new java.awt.Rectangle(15, 110, 210, 20));
            jComboBoxFileList.addActionListener(this);
            jComboBoxFileList.addItemListener(this);
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
            jButtonAdd.setBounds(new java.awt.Rectangle(230, 110, 80, 20));
            jButtonAdd.setText("Add");
            jButtonAdd.addActionListener(this);
        }
        return jButtonAdd;
    }

    /**
     This method initializes jButtonUpdate 
     
     @return javax.swing.JButton jButtonUpdate
     
     **/
    private JButton getJButtonUpdate() {
        if (jButtonUpdate == null) {
            jButtonUpdate = new JButton();
            jButtonUpdate.setBounds(new java.awt.Rectangle(315, 110, 80, 20));
            jButtonUpdate.setText("Update");
            jButtonUpdate.addActionListener(this);
        }
        return jButtonUpdate;
    }

    /**
     This method initializes jButtonRemove 
     
     @return javax.swing.JButton jButtonRemove
     
     **/
    private JButton getJButtonRemove() {
        if (jButtonRemove == null) {
            jButtonRemove = new JButton();
            jButtonRemove.setBounds(new java.awt.Rectangle(400, 110, 80, 20));
            jButtonRemove.setText("Remove");
            jButtonRemove.addActionListener(this);
        }
        return jButtonRemove;
    }

    /**
     This method initializes jTextFieldUpdatedDate 
     
     @return javax.swing.JTextField jTextFieldUpdatedDate
     
     **/
    private JTextField getJTextFieldUpdatedDate() {
        if (jTextFieldUpdatedDate == null) {
            jTextFieldUpdatedDate = new JTextField();
            jTextFieldUpdatedDate.setBounds(new java.awt.Rectangle(160, 85, 320, 20));
        }
        return jTextFieldUpdatedDate;
    }

    /**
     This method initializes jCheckBoxArch 
     
     @return javax.swing.JCheckBox jCheckBoxArch
     
     **/
    private JCheckBox getJCheckBoxArch() {
        if (jCheckBoxArch == null) {
            jCheckBoxArch = new JCheckBox();
            jCheckBoxArch.setBounds(new java.awt.Rectangle(10, 135, 120, 20));
            jCheckBoxArch.setText("Specific Arch");
            jCheckBoxArch.addActionListener(this);
        }
        return jCheckBoxArch;
    }

    /**
     This method initializes jComboBoxArch 
     
     @return javax.swing.JComboBox jComboBoxArch
     
     **/
    private JComboBox getJComboBoxArch() {
        if (jComboBoxArch == null) {
            jComboBoxArch = new JComboBox();
            jComboBoxArch.setBounds(new java.awt.Rectangle(140, 135, 340, 20));
            jComboBoxArch.setEnabled(false);
        }
        return jComboBoxArch;
    }

    public static void main(String[] args) {

    }

    /**
     This is the default constructor
     
     **/
    public ModuleIncludes() {
        super();
        init();
        this.setVisible(true);
    }

    /**
     This is the override edit constructor
     
     @param inIncludes The input data of IncludesDocument.Includes
     
     **/
    public ModuleIncludes(IncludesDocument.Includes inIncludes) {
        super();
        init(inIncludes);
        this.setVisible(true);
    }

    /**
     This is the override edit constructor
     
     @param inIncludes The input data of IncludesDocument.Includes
     @param type The input data of node type
     @param index The input data of node index
     
     **/
    public ModuleIncludes(IncludesDocument.Includes inIncludes, int type, int index) {
        super();
        this.setVisible(true);
    }

    /**
     This is the override edit constructor
     
     @param inIncludes The input data of IncludesDocument.Includes
     @param type The input data of node type
     @param index The input data of node index
     @param inOperation The input data of current operation type
     
     **/
    public ModuleIncludes(IncludesDocument.Includes inIncludes, int type, int index, int inOperation) {
        super();
        init(inIncludes, type, index, inOperation);
        this.operation = inOperation;
        this.setVisible(true);
    }

    /**
     This method initializes this
     
     @param inIncludes
     
     **/
    private void init(IncludesDocument.Includes inIncludes) {
        init();
        this.setIncludes(inIncludes);
    }

    /**
     This method initializes this
     Fill values to all fields if these values are not empty
     
     @param inIncludes The input data of IncludesDocument.Includes
     @param type The input data of node type
     @param index The input data of node index
     @param inOperation The input data of current operation type
     
     **/
    private void init(IncludesDocument.Includes inIncludes, int type, int index, int inOperation) {
        init(inIncludes);
        this.location = index;
        this.operation = inOperation;

        if (operation == 2) {
            this.jCheckBoxArch.setEnabled(false);
            this.jComboBoxArch.setEnabled(false);

            if (type == IDefaultMutableTreeNode.INCLUDES_PACKAGENAME) {
                if (this.includes.getPackageNameList().size() > 0) {
                    for (int indexI = 0; indexI < this.includes.getPackageNameList().size(); indexI++) {
                        if (this.includes.getPackageNameArray(indexI).getStringValue() != null) {
                            vPackageName.addElement(this.includes.getPackageNameArray(indexI).getStringValue());
                        } else {
                            vPackageName.addElement("");
                        }
                        if (this.includes.getPackageNameArray(indexI).getUsage() != null) {
                            vUsage.addElement(this.includes.getPackageNameArray(indexI).getUsage().toString());
                        } else {
                            vUsage.addElement("ALWAYS_CONSUMED");
                        }
                        if (this.includes.getPackageNameArray(indexI).getPackageType() != null) {
                            vPackageType.addElement(this.includes.getPackageNameArray(indexI).getPackageType()
                                                                 .toString());
                        } else {
                            vPackageType.addElement("SOURCE");
                        }
                        if (this.includes.getPackageNameArray(indexI).getUpdatedDate() != null) {
                            vUpdatedDate.addElement(this.includes.getPackageNameArray(indexI).getUpdatedDate());
                        } else {
                            vUpdatedDate.addElement("");
                        }
                        jComboBoxFileList.addItem(this.includes.getPackageNameArray(indexI).getStringValue());
                    }
                }
            }
            if (type == IDefaultMutableTreeNode.INCLUDES_ARCH_ITEM) {
                this.jCheckBoxArch.setSelected(true);
                this.jComboBoxArch.setSelectedItem(this.includes.getArchArray(index).getArchType().toString());
                for (int indexI = 0; indexI < this.includes.getArchArray(index).getPackageNameList().size(); indexI++) {
                    if (this.includes.getArchArray(index).getPackageNameArray(indexI).getStringValue() != null) {
                        vPackageName.addElement(this.includes.getArchArray(index).getPackageNameArray(indexI)
                                                             .getStringValue());
                    } else {
                        vPackageName.addElement("");
                    }
                    if (this.includes.getArchArray(index).getPackageNameArray(indexI).getUsage() != null) {
                        vUsage.addElement(this.includes.getArchArray(index).getPackageNameArray(indexI).getUsage()
                                                       .toString());
                    } else {
                        vUsage.addElement("");
                    }
                    if (this.includes.getArchArray(index).getPackageNameArray(indexI).getPackageType() != null) {
                        vPackageType.addElement(this.includes.getArchArray(index).getPackageNameArray(indexI)
                                                             .getPackageType().toString());
                    } else {
                        vPackageType.addElement("");
                    }
                    if (this.includes.getArchArray(index).getPackageNameArray(indexI).getUpdatedDate() != null) {
                        vUpdatedDate.addElement(this.includes.getArchArray(index).getPackageNameArray(indexI)
                                                             .getUpdatedDate());
                    } else {
                        vUpdatedDate.addElement("");
                    }
                    jComboBoxFileList.addItem(this.includes.getArchArray(index).getPackageNameArray(indexI)
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
        this.setContentPane(getJContentPane());
        this.setTitle("Includes");
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
            this.jComboBoxPackageType.setEnabled(!isView);
            this.jComboBoxUsage.setEnabled(!isView);
            this.jTextFieldPackageName.setEnabled(!isView);
            this.jButtonAdd.setEnabled(!isView);
            this.jButtonUpdate.setEnabled(!isView);
            this.jButtonRemove.setEnabled(!isView);
            this.jTextFieldUpdatedDate.setEnabled(!isView);
            this.jCheckBoxArch.setEnabled(!isView);
            this.jComboBoxArch.setEnabled(!isView);

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
            jLabelUpdatedDate = new JLabel();
            jLabelUpdatedDate.setBounds(new java.awt.Rectangle(15, 85, 140, 20));
            jLabelUpdatedDate.setText("Updated Date");
            jLabelUsage = new JLabel();
            jLabelUsage.setBounds(new java.awt.Rectangle(15, 60, 140, 20));
            jLabelUsage.setText("Usage");
            jLabelPackageType = new JLabel();
            jLabelPackageType.setBounds(new java.awt.Rectangle(15, 35, 140, 20));
            jLabelPackageType.setText("Package Type");
            jLabelPackageName = new JLabel();
            jLabelPackageName.setBounds(new java.awt.Rectangle(15, 10, 140, 20));
            jLabelPackageName.setText("Package Name");
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.add(getJButton(), null);
            jContentPane.add(getJButton1(), null);
            jContentPane.add(jLabelPackageName, null);
            jContentPane.add(jLabelPackageType, null);
            jContentPane.add(getJComboBoxPackageType(), null);
            jContentPane.add(jLabelUsage, null);
            jContentPane.add(getJComboBoxUsage(), null);

            jStarLabel1 = new StarLabel();
            jStarLabel1.setLocation(new java.awt.Point(0, 10));

            jContentPane.add(jStarLabel1, null);
            jContentPane.add(getJTextFieldPackageName(), null);
            jContentPane.add(getJComboBoxFileList(), null);
            jContentPane.add(getJButtonAdd(), null);
            jContentPane.add(getJButtonUpdate(), null);
            jContentPane.add(getJButtonRemove(), null);
            jContentPane.add(jLabelUpdatedDate, null);
            jContentPane.add(getJTextFieldUpdatedDate(), null);
            jContentPane.add(getJCheckBoxArch(), null);
            jContentPane.add(getJComboBoxArch(), null);
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

        //
        //When and only when checked Arch box then can choose different arch types
        //
        if (arg0.getSource() == jCheckBoxArch) {
            if (this.jCheckBoxArch.isSelected()) {
                this.jComboBoxArch.setEnabled(true);
            } else {
                this.jComboBoxArch.setEnabled(false);
            }
        }
    }

    /**
     This method initializes Usage type, Package type and Arch type
     
     **/
    private void initFrame() {
        jComboBoxUsage.addItem("ALWAYS_CONSUMED");
        jComboBoxUsage.addItem("ALWAYS_PRODUCED");
        jComboBoxUsage.addItem("DEFAULT");

        jComboBoxPackageType.addItem("SOURCE");
        jComboBoxPackageType.addItem("BINARY");
        jComboBoxPackageType.addItem("MIXED");

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
        intSelectedItemId = vPackageName.size();
        vPackageName.addElement(this.jTextFieldPackageName.getText());
        vUsage.addElement(this.jComboBoxUsage.getSelectedItem().toString());
        vPackageType.addElement(this.jComboBoxPackageType.getSelectedItem().toString());
        vUpdatedDate.addElement(this.jTextFieldUpdatedDate.getText());
        jComboBoxFileList.addItem(this.jTextFieldPackageName.getText());
        jComboBoxFileList.setSelectedItem(this.jTextFieldPackageName.getText());
        
        //
        // Reset select item index
        //
        intSelectedItemId = vPackageName.size();
        
        //
        // Reload all fields of selected item
        //
        reloadFromList();
    }

    /**
     Remove item from Vector
     
     **/
    private void removeFromList() {
        int intTempIndex = intSelectedItemId;
        if (vPackageName.size() < 1) {
            return;
        }
        
        jComboBoxFileList.removeItemAt(intSelectedItemId);
        
        vPackageName.removeElementAt(intTempIndex);
        vUsage.removeElementAt(intTempIndex);
        vPackageType.removeElementAt(intTempIndex);
        vUpdatedDate.removeElementAt(intTempIndex);
        
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
        
        vPackageName.setElementAt(this.jTextFieldPackageName.getText(), intSelectedItemId);
        vUsage.setElementAt(this.jComboBoxUsage.getSelectedItem().toString(), intSelectedItemId);
        vPackageType.setElementAt(this.jComboBoxPackageType.getSelectedItem().toString(), intSelectedItemId);
        vUpdatedDate.setElementAt(this.jTextFieldUpdatedDate.getText(), intSelectedItemId);
        
        jComboBoxFileList.removeAllItems();
        for (int index = 0; index < vPackageName.size(); index++) {
            jComboBoxFileList.addItem(vPackageName.elementAt(index));
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
        if (vPackageName.size() > 0) {
            //
            // Get selected item index
            //
            intSelectedItemId = jComboBoxFileList.getSelectedIndex();
            
            this.jTextFieldPackageName.setText(vPackageName.elementAt(intSelectedItemId).toString());
            this.jComboBoxUsage.setSelectedItem(vUsage.elementAt(intSelectedItemId).toString());
            this.jComboBoxPackageType.setSelectedItem(vPackageType.elementAt(intSelectedItemId).toString());
            this.jTextFieldUpdatedDate.setText(vUpdatedDate.elementAt(intSelectedItemId).toString());
        } else {
            this.jTextFieldPackageName.setText("");
            this.jComboBoxUsage.setSelectedItem("");
            this.jComboBoxPackageType.setSelectedItem("");
            this.jTextFieldUpdatedDate.setText("");
        }
    }

    /**
     Get IncludesDocument.Includes
     
     @return IncludesDocument.Includes
     
     **/
    public IncludesDocument.Includes getIncludes() {
        return includes;
    }

    /**
     Set IncludesDocument.Includes
     
     @param includes IncludesDocument.Includes
     
     **/
    public void setIncludes(IncludesDocument.Includes includes) {
        this.includes = includes;
    }

    /* (non-Javadoc)
     * @see java.awt.event.ItemListener#itemStateChanged(java.awt.event.ItemEvent)
     *
     * Reflesh the frame when selected item changed
     * 
     */
    public void itemStateChanged(ItemEvent arg0) {
        if (arg0.getStateChange() == ItemEvent.SELECTED) {
            reloadFromList();
        }
    }

    /**
     Data validation for all fields
     
     @retval true - All datas are valid
     @retval false - At least one data is invalid
     
     **/
    public boolean check() {
        if (this.jComboBoxFileList.getItemCount() < 1) {
            Log.err("Must have one include at least!");
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
        if (isEmpty(this.jTextFieldPackageName.getText())) {
            Log.err("File Name couldn't be empty");
            return false;
        }

        //
        // Check if all fields have correct data types 
        //
        if (!DataValidation.isPackageName(this.jTextFieldPackageName.getText())) {
            Log.err("Incorrect data type for Package Name");
            return false;
        }
        if (!isEmpty(this.jTextFieldUpdatedDate.getText())
            && !DataValidation.isDateType(this.jTextFieldUpdatedDate.getText())) {
            Log.err("Incorrect data type for Update Date");
            return false;
        }

        return true;
    }

    /**
     Save all components of Includes
     if exists includes, set the value directly
     if not exists includes, new an instance first
     
     **/
    public void save() {
        try {
            if (this.includes == null) {
                includes = IncludesDocument.Includes.Factory.newInstance();
            }
            //
            //Save as file name
            //
            if (!this.jCheckBoxArch.isSelected()) {
                if (this.operation == 2) { //Add new packageName
                    //
                    //First remove all existed packageName
                    //
                    if (includes.getPackageNameList().size() > 0) {
                        for (int index = includes.getPackageNameList().size() - 1; index >= 0; index--) {
                            includes.removePackageName(index);
                        }
                    }
                }
                for (int index = 0; index < vPackageName.size(); index++) {
                    PackageNameDocument.PackageName packageName = PackageNameDocument.PackageName.Factory.newInstance();
                    if (!isEmpty(vPackageName.elementAt(index).toString())) {
                        packageName.setStringValue(vPackageName.elementAt(index).toString());
                    }
                    if (!isEmpty(vUsage.elementAt(index).toString())) {
                        packageName.setUsage(PackageUsage.Enum.forString(vUsage.elementAt(index).toString()));
                    }
                    if (!isEmpty(vPackageType.elementAt(index).toString())) {
                        packageName
                                   .setPackageType(PackageType.Enum.forString(vPackageType.elementAt(index).toString()));
                    }
                    if (!isEmpty(vUpdatedDate.elementAt(index).toString())) {
                        packageName.setUpdatedDate(vUpdatedDate.elementAt(index).toString());
                    }
                    includes.addNewPackageName();
                    includes.setPackageNameArray(includes.getPackageNameList().size() - 1, packageName);
                }
            }
            //
            //Save as Arch
            //
            if (this.jCheckBoxArch.isSelected()) {
                IncludesDocument.Includes.Arch arch = IncludesDocument.Includes.Arch.Factory.newInstance();
                if (this.operation == 2) {
                    //
                    //First remove all existed filename
                    //
                    for (int index = includes.getArchArray(location).getPackageNameList().size() - 1; index >= 0; index--) {
                        includes.getArchArray(location).removePackageName(index);
                    }
                }

                for (int index = 0; index < vPackageName.size(); index++) {
                    PackageNameDocument.PackageName packageName = PackageNameDocument.PackageName.Factory.newInstance();
                    if (!isEmpty(vPackageName.elementAt(index).toString())) {
                        packageName.setStringValue(vPackageName.elementAt(index).toString());
                    }
                    if (!isEmpty(vUsage.elementAt(index).toString())) {
                        packageName.setUsage(PackageUsage.Enum.forString(vUsage.elementAt(index).toString()));
                    }
                    if (!isEmpty(vPackageType.elementAt(index).toString())) {
                        packageName
                                   .setPackageType(PackageType.Enum.forString(vPackageType.elementAt(index).toString()));
                    }
                    if (!isEmpty(vUpdatedDate.elementAt(index).toString())) {
                        packageName.setUpdatedDate(vUpdatedDate.elementAt(index).toString());
                    }
                    arch.addNewPackageName();
                    arch.setPackageNameArray(arch.getPackageNameList().size() - 1, packageName);
                }
                arch
                    .setArchType(SupportedArchitectures.Enum.forString(this.jComboBoxArch.getSelectedItem().toString()));
                if (location > -1) {
                    includes.setArchArray(location, arch);
                } else {
                    includes.addNewArch();
                    includes.setArchArray(includes.getArchList().size() - 1, arch);
                }
            }
        } catch (Exception e) {
            Log.err("Update Source Files", e.getMessage());
        }
    }
}
