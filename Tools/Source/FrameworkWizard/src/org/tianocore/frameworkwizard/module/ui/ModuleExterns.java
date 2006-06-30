/** @file
 
 The file is used to create, update DataHub of MSA/MBD file
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/
package org.tianocore.frameworkwizard.module.ui;

import java.awt.event.ActionEvent;
import java.awt.event.ComponentEvent;
import java.awt.event.ItemEvent;
import java.awt.event.ItemListener;
import java.util.Vector;

import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;
import javax.swing.JTextField;

import org.tianocore.ExternsDocument;
import org.tianocore.PcdDriverTypes;
import org.tianocore.ExternsDocument.Externs;
import org.tianocore.ExternsDocument.Externs.Extern;
import org.tianocore.ModuleSurfaceAreaDocument.ModuleSurfaceArea;
import org.tianocore.frameworkwizard.common.DataType;
import org.tianocore.frameworkwizard.common.DataValidation;
import org.tianocore.frameworkwizard.common.EnumerationData;
import org.tianocore.frameworkwizard.common.Log;
import org.tianocore.frameworkwizard.common.OpeningModuleType;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.ui.IInternalFrame;
import org.tianocore.frameworkwizard.common.ui.iCheckBoxList.ICheckBoxList;
import org.tianocore.frameworkwizard.module.Identification.Externs.ExternsIdentification;
import org.tianocore.frameworkwizard.module.Identification.Externs.ExternsVector;

/**
 The class is used to create, update DataHub of MSA/MBD file 
 It extends IInternalFrame
 


 **/
public class ModuleExterns extends IInternalFrame implements ItemListener {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = -7382008402932047191L;

    //
    //Define class members
    //
    private JPanel jContentPane = null;

    private JLabel jLabelName = null;

    private JComboBox jComboBoxType = null;

    private JTextArea jTextAreaList = null;

    private JComboBox jComboBoxList = null;

    private JButton jButtonAdd = null;

    private JButton jButtonRemove = null;

    private JButton jButtonUpdate = null;

    private JScrollPane jScrollPane = null;

    private JScrollPane jScrollPaneList = null;

    private JLabel jLabelPcdIsDriver = null;

    private JComboBox jComboBoxPcdIsDriver = null;

    private JLabel jLabelC_Name = null;

    private JTextField jTextFieldC_Name = null;

    private JLabel jLabelFeatureFlag = null;

    private JLabel jLabelArch = null;

    private JTextField jTextFieldFeatureFlag = null;

    private ICheckBoxList iCheckBoxListArch = null;

    private JScrollPane jScrollPaneArch = null;

    //
    // Not used by UI
    //
    private int intSelectedItemId = 0;

    private OpeningModuleType omt = null;

    private ModuleSurfaceArea msa = null;

    private ExternsDocument.Externs externs = null;

    private ExternsIdentification id = null;

    private ExternsVector vid = new ExternsVector();

    private EnumerationData ed = new EnumerationData();

    /**
     This method initializes jComboBoxType 
     
     @return javax.swing.JComboBox jComboBoxType
     
     **/
    private JComboBox getJComboBoxType() {
        if (jComboBoxType == null) {
            jComboBoxType = new JComboBox();
            jComboBoxType.setBounds(new java.awt.Rectangle(160, 35, 320, 20));
            jComboBoxType.setPreferredSize(new java.awt.Dimension(320, 20));
        }
        return jComboBoxType;
    }

    /**
     This method initializes jComboBoxFileList 
     
     @return javax.swing.JComboBox jComboBoxFileList
     
     **/
    private JComboBox getJComboBoxList() {
        if (jComboBoxList == null) {
            jComboBoxList = new JComboBox();
            jComboBoxList.setBounds(new java.awt.Rectangle(15, 195, 210, 20));
            jComboBoxList.addItemListener(this);
            jComboBoxList.addActionListener(this);
            jComboBoxList.setPreferredSize(new java.awt.Dimension(210, 20));
        }
        return jComboBoxList;
    }

    /**
     This method initializes jButtonAdd 
     
     @return javax.swing.JButton jButtonAdd
     
     **/
    private JButton getJButtonAdd() {
        if (jButtonAdd == null) {
            jButtonAdd = new JButton();
            jButtonAdd.setBounds(new java.awt.Rectangle(230, 195, 80, 20));
            jButtonAdd.setText("Add");
            jButtonAdd.addActionListener(this);
            jButtonAdd.setPreferredSize(new java.awt.Dimension(80, 20));
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
            jButtonRemove.setBounds(new java.awt.Rectangle(400, 195, 80, 20));
            jButtonRemove.setText("Remove");
            jButtonRemove.addActionListener(this);
            jButtonRemove.setPreferredSize(new java.awt.Dimension(80, 20));
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
            jButtonUpdate.setBounds(new java.awt.Rectangle(315, 195, 80, 20));
            jButtonUpdate.setPreferredSize(new java.awt.Dimension(80, 20));
            jButtonUpdate.setText("Update");
            jButtonUpdate.addActionListener(this);
        }
        return jButtonUpdate;
    }

    /**
     * This method initializes jScrollPaneFileList   
     *   
     * @return javax.swing.JScrollPane   
     */
    private JScrollPane getJScrollPaneList() {
        if (jScrollPaneList == null) {
            jScrollPaneList = new JScrollPane();
            jScrollPaneList.setBounds(new java.awt.Rectangle(15, 220, 465, 240));
            jScrollPaneList.setViewportView(getJTextAreaList());
            jScrollPaneList.setPreferredSize(new java.awt.Dimension(465, 240));
        }
        return jScrollPaneList;
    }

    /**
     This method initializes jScrollPane  
     
     @return javax.swing.JScrollPane  
     */
    private JScrollPane getJScrollPane() {
        if (jScrollPane == null) {
            jScrollPane = new JScrollPane();
            jScrollPane.setViewportView(getJContentPane());
        }
        return jScrollPane;
    }

    /**
     * This method initializes jTextAreaFileList 
     *   
     * @return javax.swing.JTextArea 
     */
    private JTextArea getJTextAreaList() {
        if (jTextAreaList == null) {
            jTextAreaList = new JTextArea();
            jTextAreaList.setEditable(false);
        }
        return jTextAreaList;
    }

    /**
     * This method initializes jComboBoxPcdIsDriver	
     * 	
     * @return javax.swing.JComboBox	
     */
    private JComboBox getJComboBoxPcdIsDriver() {
        if (jComboBoxPcdIsDriver == null) {
            jComboBoxPcdIsDriver = new JComboBox();
            jComboBoxPcdIsDriver.setLocation(new java.awt.Point(160, 10));
            jComboBoxPcdIsDriver.setPreferredSize(new java.awt.Dimension(320, 20));
            jComboBoxPcdIsDriver.setSize(new java.awt.Dimension(320, 20));
            jComboBoxPcdIsDriver.addItemListener(this);
        }
        return jComboBoxPcdIsDriver;
    }

    /**
     This method initializes jTextFieldC_Name	
     
     @return javax.swing.JTextField	
     
     **/
    private JTextField getJTextFieldC_Name() {
        if (jTextFieldC_Name == null) {
            jTextFieldC_Name = new JTextField();
            jTextFieldC_Name.setBounds(new java.awt.Rectangle(160, 60, 320, 20));
            jTextFieldC_Name.setPreferredSize(new java.awt.Dimension(320,20));
        }
        return jTextFieldC_Name;
    }

    /**
     This method initializes jTextFieldFeatureFlag    
     
     @return javax.swing.JTextField   
     
     **/
    private JTextField getJTextFieldFeatureFlag() {
        if (jTextFieldFeatureFlag == null) {
            jTextFieldFeatureFlag = new JTextField();
            jTextFieldFeatureFlag.setBounds(new java.awt.Rectangle(160, 85, 320, 20));
            jTextFieldFeatureFlag.setPreferredSize(new java.awt.Dimension(320, 20));
        }
        return jTextFieldFeatureFlag;
    }

    /**
     This method initializes iCheckBoxListArch   

     @return ICheckBoxList   
     **/
    private ICheckBoxList getICheckBoxListSupportedArchitectures() {
        if (iCheckBoxListArch == null) {
            iCheckBoxListArch = new ICheckBoxList();
            iCheckBoxListArch.addFocusListener(this);
            iCheckBoxListArch.setToolTipText(DataType.SUP_ARCH_LIST_HELP_TEXT);
        }
        return iCheckBoxListArch;
    }

    /**
     This method initializes jScrollPaneArch 

     @return javax.swing.JScrollPane 

     **/
    private JScrollPane getJScrollPaneArch() {
        if (jScrollPaneArch == null) {
            jScrollPaneArch = new JScrollPane();
            jScrollPaneArch.setBounds(new java.awt.Rectangle(160, 110, 320, 80));
            jScrollPaneArch.setPreferredSize(new java.awt.Dimension(320, 80));
            jScrollPaneArch.setViewportView(getICheckBoxListSupportedArchitectures());
        }
        return jScrollPaneArch;
    }

    public static void main(String[] args) {

    }

    /**
     This method initializes this
     
     **/
    private void init() {
        this.setSize(500, 515);
        this.setContentPane(getJScrollPane());
        this.setTitle("Externs");
        initFrame();
    }

    /**
     This method initializes this
     Fill values to all fields if these values are not empty
     
     @param inPackageDependencies

     **/
    private void init(Externs inExterns) {
        init();
        this.externs = inExterns;

        if (this.externs != null) {
            //
            // Get PcdIsDriver
            //
            if (this.externs.getPcdIsDriver() != null) {
                this.jComboBoxPcdIsDriver.setSelectedItem(this.externs.getPcdIsDriver().toString());
            }
            
            //
            // Get specification
            //
            if (this.externs.getSpecificationList().size() > 0) {
                for (int index = 0; index < this.externs.getSpecificationList().size(); index++) {
                    String arg0 = externs.getSpecificationList().get(index);
                    String arg1 = EnumerationData.EXTERNS_SPECIFICATION;

                    id = new ExternsIdentification(arg0, arg1, null, null);
                    vid.addExterns(id);
                }
            }

            //
            // Get Externs list
            //
            if (this.externs.getExternList().size() > 0) {
                for (int index = 0; index < this.externs.getExternList().size(); index++) {
                    String arg0 = null;
                    String arg1 = null;
                    if (this.externs.getExternList().get(index).getModuleEntryPoint() != null) {
                        arg0 = this.externs.getExternList().get(index).getModuleEntryPoint();
                        arg1 = EnumerationData.EXTERNS_MODULE_ENTRY_POINT;
                    }
                    if (this.externs.getExternList().get(index).getModuleUnloadImage() != null) {
                        arg0 = this.externs.getExternList().get(index).getModuleUnloadImage();
                        arg1 = EnumerationData.EXTERNS_MODULE_UNLOAD_IMAGE;
                    }

                    if (this.externs.getExternList().get(index).getConstructor() != null) {
                        arg0 = this.externs.getExternList().get(index).getConstructor();
                        arg1 = EnumerationData.EXTERNS_CONSTRUCTOR;
                    }
                    if (this.externs.getExternList().get(index).getDestructor() != null) {
                        arg0 = this.externs.getExternList().get(index).getDestructor();
                        arg1 = EnumerationData.EXTERNS_DESTRUCTOR;
                    }

                    if (this.externs.getExternList().get(index).getDriverBinding() != null) {
                        arg0 = this.externs.getExternList().get(index).getDriverBinding();
                        arg1 = EnumerationData.EXTERNS_DRIVER_BINDING;
                    }
                    if (this.externs.getExternList().get(index).getComponentName() != null) {
                        arg0 = this.externs.getExternList().get(index).getComponentName();
                        arg1 = EnumerationData.EXTERNS_COMPONENT_NAME;
                    }
                    if (this.externs.getExternList().get(index).getDriverConfig() != null) {
                        arg0 = this.externs.getExternList().get(index).getDriverConfig();
                        arg1 = EnumerationData.EXTERNS_DRIVER_CONFIG;
                    }
                    if (this.externs.getExternList().get(index).getDriverDiag() != null) {
                        arg0 = this.externs.getExternList().get(index).getDriverDiag();
                        arg1 = EnumerationData.EXTERNS_DRIVER_DIAG;
                    }

                    if (this.externs.getExternList().get(index).getSetVirtualAddressMapCallBack() != null) {
                        arg0 = this.externs.getExternList().get(index).getSetVirtualAddressMapCallBack();
                        arg1 = EnumerationData.EXTERNS_SET_VIRTUAL_ADDRESS_MAP_CALL_BACK;
                    }
                    if (this.externs.getExternList().get(index).getExitBootServicesCallBack() != null) {
                        arg0 = this.externs.getExternList().get(index).getExitBootServicesCallBack();
                        arg1 = EnumerationData.EXTERNS_EXIT_BOOT_SERVICES_CALL_BACK;
                    }

                    String arg2 = externs.getExternList().get(index).getFeatureFlag();
                    Vector<String> arg3 = Tools
                                               .convertListToVector(externs.getExternList().get(index).getSupArchList());

                    id = new ExternsIdentification(arg0, arg1, arg2, arg3);
                    vid.addExterns(id);
                }
            }
        }
        //
        // Update the list
        //
        Tools.generateComboBoxByVector(jComboBoxList, vid.getExternsName());
        reloadListArea();
    }

    /**
     This is the default constructor
     
     **/
    public ModuleExterns() {
        super();
        init();
        this.setVisible(true);
    }

    /**
     This is the override edit constructor
     
     @param inExterns The input data of ExternsDocument.Externs
     
     **/
    public ModuleExterns(OpeningModuleType inOmt) {
        super();
        this.omt = inOmt;
        this.msa = omt.getXmlMsa();
        init(msa.getExterns());
        this.setVisible(true);
    }

    /**
     This method initializes jContentPane
     
     @return javax.swing.JPanel jContentPane
     
     **/
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jLabelC_Name = new JLabel();
            jLabelC_Name.setBounds(new java.awt.Rectangle(15, 60, 140, 20));
            jLabelC_Name.setText("Value");
            jLabelPcdIsDriver = new JLabel();
            jLabelPcdIsDriver.setBounds(new java.awt.Rectangle(15, 10, 140, 20));
            jLabelPcdIsDriver.setText("Pcd Is Driver");
            jLabelName = new JLabel();
            jLabelName.setText("Choose Type");
            jLabelName.setBounds(new java.awt.Rectangle(15, 35, 140, 20));
            jLabelArch = new JLabel();
            jLabelArch.setBounds(new java.awt.Rectangle(15, 110, 140, 20));
            jLabelArch.setText("Arch");
            jLabelFeatureFlag = new JLabel();
            jLabelFeatureFlag.setBounds(new java.awt.Rectangle(15, 85, 140, 20));
            jLabelFeatureFlag.setText("Feature Flag");

            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.setPreferredSize(new java.awt.Dimension(490, 475));

            jContentPane.add(jLabelName, null);
            jContentPane.add(getJComboBoxType(), null);
            jContentPane.add(jLabelFeatureFlag, null);
            jContentPane.add(jLabelArch, null);
            jContentPane.add(getJTextFieldFeatureFlag(), null);
            jContentPane.add(getJScrollPaneArch(), null);
            
            jContentPane.add(getJComboBoxList(), null);
            jContentPane.add(getJButtonAdd(), null);
            jContentPane.add(getJButtonRemove(), null);
            jContentPane.add(getJButtonUpdate(), null);
            jContentPane.add(getJScrollPaneList(), null);
            jContentPane.add(jLabelPcdIsDriver, null);
            jContentPane.add(getJComboBoxPcdIsDriver(), null);
            jContentPane.add(jLabelC_Name, null);
            jContentPane.add(getJTextFieldC_Name(), null);
        }
        return jContentPane;
    }

    /**
     This method initializes Usage type and Externs type
     
     **/
    private void initFrame() {
        Tools.generateComboBoxByVector(this.jComboBoxType, ed.getVExternTypes());
        Tools.generateComboBoxByVector(this.jComboBoxPcdIsDriver, ed.getVPcdDriverTypes());
        this.iCheckBoxListArch.setAllItems(ed.getVSupportedArchitectures());
    }

    /* (non-Javadoc)
     * @see java.awt.event.ActionListener#actionPerformed(java.awt.event.ActionEvent)
     *
     * Override actionPerformed to listen all actions
     * 
     */
    public void actionPerformed(ActionEvent arg0) {
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
    }

    /**
     Data validation for all fields
     
     @retval true - All datas are valid
     @retval false - At least one data is invalid
     
     **/
    public boolean checkAdd() {
        //
        // Check if all fields have correct data types 
        //

        //
        // Check CName 
        //
        if (isEmpty(this.jTextFieldC_Name.getText())) {
            Log.err("Value couldn't be empty");
            return false;
        }

        if (!isEmpty(this.jTextFieldC_Name.getText())) {
            if (this.jComboBoxType.getSelectedItem().toString().equals(EnumerationData.EXTERNS_SPECIFICATION)) {
                if (!DataValidation.isSentence(this.jTextFieldC_Name.getText())) {
                    Log.err("Incorrect data type for Specification");
                    return false;
                }    
            } else {
                if (!DataValidation.isC_NameType(this.jTextFieldC_Name.getText())) {
                    Log.err("Incorrect data type for C_Name");
                    return false;
                }    
            }
        }

        //
        // Check FeatureFlag
        //
        if (!isEmpty(this.jTextFieldFeatureFlag.getText())) {
            if (!DataValidation.isFeatureFlag(this.jTextFieldFeatureFlag.getText())) {
                Log.err("Incorrect data type for Feature Flag");
                return false;
            }
        }

        return true;
    }

    /**
     Save all components of Externs
     if exists externs, set the value directly
     if not exists externs, new an instance first
     
     **/
    public void save() {
        try {
            int count = this.vid.size();

            this.externs = Externs.Factory.newInstance();
            //
            // Save PcdIsDriver first
            //
            if (!this.jComboBoxPcdIsDriver.getSelectedItem().toString().equals(DataType.EMPTY_SELECT_ITEM)) {
                externs.setPcdIsDriver(PcdDriverTypes.Enum.forString(this.jComboBoxPcdIsDriver.getSelectedItem()
                                                                                              .toString()));
            }

            if (count > 0) {
                for (int index = 0; index < count; index++) {
                    //
                    // Save specfication
                    //
                    if (vid.getExterns(index).getType().equals(EnumerationData.EXTERNS_SPECIFICATION)) {
                        if (!isEmpty(vid.getExterns(index).getName())) {
                            this.externs.addNewSpecification();
                            this.externs.setSpecificationArray(externs.getSpecificationList().size() - 1,
                                                               vid.getExterns(index).getName());
                        }
                    } else {
                        //
                        // Save extern
                        //
                        Extern e = Extern.Factory.newInstance();

                        if (vid.getExterns(index).getType().equals(EnumerationData.EXTERNS_MODULE_ENTRY_POINT)) {
                            if (!isEmpty(vid.getExterns(index).getName())) {
                                e.setModuleEntryPoint(vid.getExterns(index).getName());
                            }
                        }
                        if (vid.getExterns(index).getType().equals(EnumerationData.EXTERNS_MODULE_UNLOAD_IMAGE)) {
                            if (!isEmpty(vid.getExterns(index).getName())) {
                                e.setModuleUnloadImage(vid.getExterns(index).getName());
                            }
                        }

                        if (vid.getExterns(index).getType().equals(EnumerationData.EXTERNS_CONSTRUCTOR)) {
                            if (!isEmpty(vid.getExterns(index).getName())) {
                                e.setConstructor(vid.getExterns(index).getName());
                            }
                        }
                        if (vid.getExterns(index).getType().equals(EnumerationData.EXTERNS_DESTRUCTOR)) {
                            if (!isEmpty(vid.getExterns(index).getName())) {
                                e.setDestructor(vid.getExterns(index).getName());
                            }
                        }

                        if (vid.getExterns(index).getType().equals(EnumerationData.EXTERNS_DRIVER_BINDING)) {
                            if (!isEmpty(vid.getExterns(index).getName())) {
                                e.setDriverBinding(vid.getExterns(index).getName());
                            }
                        }
                        if (vid.getExterns(index).getType().equals(EnumerationData.EXTERNS_COMPONENT_NAME)) {
                            if (!isEmpty(vid.getExterns(index).getName())) {
                                e.setComponentName(vid.getExterns(index).getName());
                            }
                        }
                        if (vid.getExterns(index).getType().equals(EnumerationData.EXTERNS_DRIVER_CONFIG)) {
                            if (!isEmpty(vid.getExterns(index).getName())) {
                                e.setDriverConfig(vid.getExterns(index).getName());
                            }
                        }
                        if (vid.getExterns(index).getType().equals(EnumerationData.EXTERNS_DRIVER_DIAG)) {
                            if (!isEmpty(vid.getExterns(index).getName())) {
                                e.setDriverDiag(vid.getExterns(index).getName());
                            }
                        }

                        if (vid.getExterns(index).getType()
                               .equals(EnumerationData.EXTERNS_SET_VIRTUAL_ADDRESS_MAP_CALL_BACK)) {
                            if (!isEmpty(vid.getExterns(index).getName())) {
                                e.setSetVirtualAddressMapCallBack(vid.getExterns(index).getName());
                            }
                        }
                        if (vid.getExterns(index).getType()
                               .equals(EnumerationData.EXTERNS_EXIT_BOOT_SERVICES_CALL_BACK)) {
                            if (!isEmpty(vid.getExterns(index).getName())) {
                                e.setExitBootServicesCallBack(vid.getExterns(index).getName());
                            }
                        }

                        if (!isEmpty(vid.getExterns(index).getFeatureFlag())) {
                            e.setFeatureFlag(vid.getExterns(index).getFeatureFlag());
                        }
                        if (vid.getExterns(index).getSupArchList() != null
                            && vid.getExterns(index).getSupArchList().size() > 0) {
                            e.setSupArchList(vid.getExterns(index).getSupArchList());
                        }

                        this.externs.addNewExtern();
                        this.externs.setExternArray(this.externs.getExternList().size() - 1, e);
                    }
                }
            }

            this.msa.setExterns(externs);
            this.omt.setSaved(false);
        } catch (Exception e) {
            Log.err("Update Externs", e.getMessage());
        }
    }

    /* (non-Javadoc)
     * @see java.awt.event.ComponentListener#componentResized(java.awt.event.ComponentEvent)
     * 
     * Override componentResized to resize all components when frame's size is changed
     */
    public void componentResized(ComponentEvent arg0) {
        int intCurrentWidth = this.getJContentPane().getWidth();
        int intCurrentHeight = this.getJContentPane().getHeight();
        int intPreferredWidth = this.getJContentPane().getPreferredSize().width;
        int intPreferredHeight = this.getJContentPane().getPreferredSize().height;

        resizeComponentWidth(this.jComboBoxPcdIsDriver, intCurrentWidth, intPreferredWidth);
        resizeComponentWidth(this.jComboBoxType, intCurrentWidth, intPreferredWidth);
        resizeComponentWidth(this.jTextFieldC_Name, intCurrentWidth, intPreferredWidth);
        resizeComponentWidth(this.jTextFieldFeatureFlag, intCurrentWidth, intPreferredWidth);
        resizeComponentWidth(this.jScrollPaneArch, intCurrentWidth, intPreferredWidth);

        resizeComponentWidth(jComboBoxList, intCurrentWidth, intPreferredWidth);
        resizeComponent(jScrollPaneList, intCurrentWidth, intCurrentHeight, intPreferredWidth, intPreferredHeight);

        relocateComponentX(jButtonAdd, intCurrentWidth, intPreferredWidth, DataType.SPACE_TO_RIGHT_FOR_ADD_BUTTON);
        relocateComponentX(jButtonRemove, intCurrentWidth, intPreferredWidth, DataType.SPACE_TO_RIGHT_FOR_REMOVE_BUTTON);
        relocateComponentX(jButtonUpdate, intCurrentWidth, intPreferredWidth, DataType.SPACE_TO_RIGHT_FOR_UPDATE_BUTTON);
    }

    private ExternsIdentification getCurrentExterns() {
        String arg0 = this.jTextFieldC_Name.getText();
        String arg1 = this.jComboBoxType.getSelectedItem().toString();

        String arg2 = this.jTextFieldFeatureFlag.getText();
        Vector<String> arg3 = this.iCheckBoxListArch.getAllCheckedItemsString();

        id = new ExternsIdentification(arg0, arg1, arg2, arg3);
        return id;
    }

    /**
     Add current item to Vector
     
     **/
    private void addToList() {
        intSelectedItemId = vid.size();

        vid.addExterns(getCurrentExterns());

        jComboBoxList.addItem(id.getName());
        jComboBoxList.setSelectedItem(id.getName());

        //
        // Reset select item index
        //
        intSelectedItemId = vid.size();

        //
        // Reload all fields of selected item
        //
        reloadFromList();

        // 
        // Save to memory
        //
        save();
    }

    /**
     Remove current item from Vector
     
     **/
    private void removeFromList() {
        //
        // Check if exist items
        //
        if (this.vid.size() < 1) {
            return;
        }

        int intTempIndex = intSelectedItemId;

        jComboBoxList.removeItemAt(intSelectedItemId);

        vid.removeExterns(intTempIndex);

        //
        // Reload all fields of selected item
        //
        reloadFromList();

        // 
        // Save to memory
        //
        save();
    }

    /**
     Update current item of Vector
     
     **/
    private void updateForList() {
        //
        // Check if exist items
        //
        if (this.vid.size() < 1) {
            return;
        }

        //
        // Backup selected item index
        //
        int intTempIndex = intSelectedItemId;

        vid.updateExterns(getCurrentExterns(), intTempIndex);

        jComboBoxList.removeAllItems();
        for (int index = 0; index < vid.size(); index++) {
            jComboBoxList.addItem(vid.getExterns(index).getName());
        }

        //
        // Restore selected item index
        //
        intSelectedItemId = intTempIndex;

        //
        // Reset select item index
        //
        jComboBoxList.setSelectedIndex(intSelectedItemId);

        //
        // Reload all fields of selected item
        //
        reloadFromList();

        // 
        // Save to memory
        //
        save();
    }

    /**
     Refresh all fields' values of selected item of Vector
     
     **/
    private void reloadFromList() {
        if (vid.size() > 0) {
            //
            // Get selected item index
            //
            intSelectedItemId = jComboBoxList.getSelectedIndex();

            this.jTextFieldC_Name.setText(vid.getExterns(intSelectedItemId).getName());
            this.jComboBoxType.setSelectedItem(vid.getExterns(intSelectedItemId).getType());

            jTextFieldFeatureFlag.setText(vid.getExterns(intSelectedItemId).getFeatureFlag());
            iCheckBoxListArch.setAllItemsUnchecked();
            iCheckBoxListArch.initCheckedItem(true, vid.getExterns(intSelectedItemId).getSupArchList());

        } else {
        }

        reloadListArea();
    }

    /**
     Update list area pane via the elements of Vector
     
     **/
    private void reloadListArea() {
        String strListItem = "";
        for (int index = 0; index < vid.size(); index++) {
            strListItem = strListItem + vid.getExterns(index).getName() + DataType.UNIX_LINE_SEPARATOR;
        }
        this.jTextAreaList.setText(strListItem);
    }

    /* (non-Javadoc)
     * @see java.awt.event.ItemListener#itemStateChanged(java.awt.event.ItemEvent)
     *
     * Reflesh the frame when selected item changed
     * 
     */
    public void itemStateChanged(ItemEvent arg0) {
        if (arg0.getSource() == this.jComboBoxList && arg0.getStateChange() == ItemEvent.SELECTED) {
            reloadFromList();
        }
        if (arg0.getSource() == this.jComboBoxPcdIsDriver && arg0.getStateChange() == ItemEvent.SELECTED && externs != null) {
            String s = this.jComboBoxPcdIsDriver.getSelectedItem().toString();
            if (s.equals(DataType.EMPTY_SELECT_ITEM)) {
                this.externs.setPcdIsDriver(null);
            } else {
                this.externs.setPcdIsDriver(PcdDriverTypes.Enum.forString(s));
            }
            this.msa.setExterns(externs);
            this.omt.setSaved(false);
        }
    }
}
