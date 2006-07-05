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
package org.tianocore.frameworkwizard.module.ui;

import java.awt.Dimension;
import java.awt.event.ActionEvent;
import java.awt.event.ComponentEvent;
import java.awt.event.ItemEvent;
import java.util.Vector;

import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;
import javax.swing.JTextField;

import org.tianocore.LibraryClassDefinitionsDocument;
import org.tianocore.LibraryUsage;
import org.tianocore.ModuleSurfaceAreaDocument;
import org.tianocore.LibraryClassDefinitionsDocument.LibraryClassDefinitions;
import org.tianocore.LibraryClassDocument.LibraryClass;
import org.tianocore.frameworkwizard.common.DataType;
import org.tianocore.frameworkwizard.common.DataValidation;
import org.tianocore.frameworkwizard.common.EnumerationData;
import org.tianocore.frameworkwizard.common.Log;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.Identifications.OpeningModuleType;
import org.tianocore.frameworkwizard.common.ui.IInternalFrame;
import org.tianocore.frameworkwizard.common.ui.StarLabel;
import org.tianocore.frameworkwizard.common.ui.iCheckBoxList.ICheckBoxList;
import org.tianocore.frameworkwizard.module.Identifications.LibraryClass.LibraryClassIdentification;
import org.tianocore.frameworkwizard.module.Identifications.LibraryClass.LibraryClassVector;
import org.tianocore.frameworkwizard.workspace.WorkspaceTools;

/**
 The class is used to create, update Library Class Definition of MSA/MBD file
 It extends IInternalFrame
 
 **/
public class ModuleLibraryClassDefinitions extends IInternalFrame {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = -1743248695411382857L;

    //
    //Define class members
    //
    private JPanel jContentPane = null;

    private JComboBox jComboBoxLibraryClassName = null;

    private JLabel jLabelUsage = null;

    private JComboBox jComboBoxUsage = null;

    private JComboBox jComboBoxList = null;

    private JButton jButtonAdd = null;

    private JButton jButtonRemove = null;

    private JButton jButtonUpdate = null;

    private JLabel jLabelLibraryClassName = null;

    private JScrollPane jScrollPaneList = null;

    private JScrollPane jScrollPane = null;

    private JTextArea jTextAreaList = null;

    private StarLabel jStarLabel1 = null;

    private StarLabel jStarLabel2 = null;

    private ICheckBoxList iCheckBoxListArch = null;

    private JScrollPane jScrollPaneArch = null;

    private JLabel jLabelRecommendedInstanceVersion = null;

    private JTextField jTextFieldRecommendedInstanceVersion = null;

    private JLabel jLabelRecommendedInstanceGuid = null;

    private JTextField jTextFieldRecommendedInstanceGuid = null;

    private JButton jButtonGenerateGuid = null;

    private JLabel jLabelFeatureFlag = null;

    private JTextField jTextFieldFeatureFlag = null;

    private JLabel jLabelArch = null;

    private JLabel jLabelModuleList = null;

    private JScrollPane jScrollPaneModuleList = null;

    private ICheckBoxList iCheckBoxListModule = null;

    private JLabel jLabelHelpText = null;

    private JTextField jTextFieldHelpText = null;

    //
    // Not for UI
    //
    private ModuleSurfaceAreaDocument.ModuleSurfaceArea msa = null;

    private LibraryClassDefinitions lcd = null;

    private LibraryClassVector vLibraryClass = new LibraryClassVector();

    private EnumerationData ed = new EnumerationData();

    private Vector<String> vLib = new Vector<String>();

    private int intSelectedItemId = 0;

    private WorkspaceTools wt = new WorkspaceTools();

    private LibraryClassIdentification lcid = null;

    private OpeningModuleType omt = null;

    /**
     This method initializes jComboBoxSelect 
     
     @return javax.swing.JComboBox jComboBoxSelect
     
     **/
    private JComboBox getJComboBoxLibraryClassName() {
        if (jComboBoxLibraryClassName == null) {
            jComboBoxLibraryClassName = new JComboBox();
            jComboBoxLibraryClassName.setBounds(new java.awt.Rectangle(160, 10, 320, 20));
            jComboBoxLibraryClassName.setPreferredSize(new Dimension(320, 20));
            jComboBoxLibraryClassName.setEnabled(true);
        }
        return jComboBoxLibraryClassName;
    }

    /**
     This method initializes jComboBoxUsage 
     
     @return javax.swing.JComboBox jComboBoxUsage
     
     **/
    private JComboBox getJComboBoxUsage() {
        if (jComboBoxUsage == null) {
            jComboBoxUsage = new JComboBox();
            jComboBoxUsage.setBounds(new java.awt.Rectangle(160, 35, 320, 20));
            jComboBoxUsage.setPreferredSize(new Dimension(320, 20));
        }
        return jComboBoxUsage;
    }

    /**
     This method initializes jComboBoxFileList 
     
     @return javax.swing.JComboBox jComboBoxFileList
     
     **/
    private JComboBox getJComboBoxList() {
        if (jComboBoxList == null) {
            jComboBoxList = new JComboBox();
            jComboBoxList.setBounds(new java.awt.Rectangle(15, 330, 210, 20));
            jComboBoxList.setPreferredSize(new Dimension(210, 20));
            jComboBoxList.addItemListener(this);
            jComboBoxList.addActionListener(this);
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
            jButtonAdd.setBounds(new java.awt.Rectangle(230, 330, 80, 20));
            jButtonAdd.setPreferredSize(new Dimension(80, 20));
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
            jButtonRemove.setBounds(new java.awt.Rectangle(400, 330, 80, 20));
            jButtonRemove.setText("Remove");
            jButtonRemove.setPreferredSize(new Dimension(80, 20));
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
            jButtonUpdate.setBounds(new java.awt.Rectangle(315, 330, 80, 20));
            jButtonUpdate.setText("Update");
            jButtonUpdate.setPreferredSize(new Dimension(80, 20));
            jButtonUpdate.addActionListener(this);
        }
        return jButtonUpdate;
    }

    /**
     This method initializes jScrollPane	
     
     @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPaneList() {
        if (jScrollPaneList == null) {
            jScrollPaneList = new JScrollPane();
            jScrollPaneList.setBounds(new java.awt.Rectangle(15, 355, 465, 100));
            jScrollPaneList.setPreferredSize(new Dimension(465, 260));
            jScrollPaneList.setViewportView(getJTextAreaList());
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
     * This method initializes jTextAreaList	
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
     * This method initializes jTextFieldRecommendedInstanceVersion	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldRecommendedInstanceVersion() {
        if (jTextFieldRecommendedInstanceVersion == null) {
            jTextFieldRecommendedInstanceVersion = new JTextField();
            jTextFieldRecommendedInstanceVersion.setPreferredSize(new java.awt.Dimension(260, 20));
            jTextFieldRecommendedInstanceVersion.setSize(new java.awt.Dimension(260, 20));
            jTextFieldRecommendedInstanceVersion.setLocation(new java.awt.Point(220, 85));
        }
        return jTextFieldRecommendedInstanceVersion;
    }

    /**
     * This method initializes jTextFieldRecommendedInstanceGuid	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldRecommendedInstanceGuid() {
        if (jTextFieldRecommendedInstanceGuid == null) {
            jTextFieldRecommendedInstanceGuid = new JTextField();
            jTextFieldRecommendedInstanceGuid.setBounds(new java.awt.Rectangle(220, 110, 190, 20));
            jTextFieldRecommendedInstanceGuid.setPreferredSize(new java.awt.Dimension(190, 20));
        }
        return jTextFieldRecommendedInstanceGuid;
    }

    /**
     * This method initializes jButtonGenerateGuid	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonGenerateGuid() {
        if (jButtonGenerateGuid == null) {
            jButtonGenerateGuid = new JButton();
            jButtonGenerateGuid.setBounds(new java.awt.Rectangle(415, 110, 65, 20));
            jButtonGenerateGuid.setPreferredSize(new java.awt.Dimension(65, 20));
            jButtonGenerateGuid.setText("GEN");
            jButtonGenerateGuid.addActionListener(this);
        }
        return jButtonGenerateGuid;
    }

    /**
     * This method initializes jTextFieldFeatureFlag	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldFeatureFlag() {
        if (jTextFieldFeatureFlag == null) {
            jTextFieldFeatureFlag = new JTextField();
            jTextFieldFeatureFlag.setBounds(new java.awt.Rectangle(160, 135, 320, 20));
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
        }
        return iCheckBoxListArch;
    }

    /**
     This method initializes iCheckBoxListArch   
     
     @return ICheckBoxList   
     **/
    private ICheckBoxList getICheckBoxListSupModuleList() {
        if (iCheckBoxListModule == null) {
            iCheckBoxListModule = new ICheckBoxList();
        }
        return iCheckBoxListModule;
    }

    /**
     This method initializes jScrollPaneArch 
     
     @return javax.swing.JScrollPane 
     
     **/
    private JScrollPane getJScrollPaneArch() {
        if (jScrollPaneArch == null) {
            jScrollPaneArch = new JScrollPane();
            jScrollPaneArch.setBounds(new java.awt.Rectangle(160, 160, 320, 80));
            jScrollPaneArch.setPreferredSize(new java.awt.Dimension(320, 80));
            jScrollPaneArch.setViewportView(getICheckBoxListSupportedArchitectures());
        }
        return jScrollPaneArch;
    }

    /**
     This method initializes jScrollPaneModuleList	
     
     @return javax.swing.JScrollPane	
     
     **/
    private JScrollPane getJScrollPaneModuleList() {
        if (jScrollPaneModuleList == null) {
            jScrollPaneModuleList = new JScrollPane();
            jScrollPaneModuleList.setBounds(new java.awt.Rectangle(160, 245, 320, 80));
            jScrollPaneModuleList.setPreferredSize(new java.awt.Dimension(320, 80));
            jScrollPaneModuleList.setViewportView(getICheckBoxListSupModuleList());
        }
        return jScrollPaneModuleList;
    }

    /**
     This method initializes jTextFieldHelpText	
     
     @return javax.swing.JTextField	
     
     **/
    private JTextField getJTextFieldHelpText() {
        if (jTextFieldHelpText == null) {
            jTextFieldHelpText = new JTextField();
            jTextFieldHelpText.setBounds(new java.awt.Rectangle(160, 60, 320, 20));
            jTextFieldHelpText.setPreferredSize(new java.awt.Dimension(320, 20));
        }
        return jTextFieldHelpText;
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
     
     @param 
     
     **/
    public ModuleLibraryClassDefinitions(OpeningModuleType inOmt) {
        super();
        this.omt = inOmt;
        this.msa = omt.getXmlMsa();
        initLibraryClass();
        init(msa.getLibraryClassDefinitions());
        this.setVisible(true);
    }

    //    private void initLibraryClass(MsaHeaderDocument.MsaHeader msaHeader) {
    //        Enum e = msaHeader.getModuleType();
    //        if (e == ModuleTypeDef.BASE) {
    //            vLib = ed.getVLibClassDefBase();
    //        } else if (e == ModuleTypeDef.PEI_CORE) {
    //            vLib = ed.getVLibClassDefPei();
    //        } else if (e == ModuleTypeDef.PEIM) {
    //            vLib = ed.getVLibClassDefPeim();
    //        } else if (e == ModuleTypeDef.DXE_CORE) {
    //            vLib = ed.getVLibClassDefDxeCore();
    //        } else if (e == ModuleTypeDef.DXE_DRIVER) {
    //            vLib = ed.getVLibClassDefDxeDriver();
    //        } else if (e == ModuleTypeDef.DXE_SMM_DRIVER) {
    //            vLib = ed.getVLibClassDefDxeSmmDriver();
    //        } else if (e == ModuleTypeDef.UEFI_DRIVER) {
    //            vLib = ed.getVLibClassDefUefiDriver();
    //        } else {
    //            //vLib = ed.getVLibClassDef();
    //        }
    //    }

    /**
     
     **/
    private void initLibraryClass() {
        vLib = wt.getAllLibraryClassDefinitionsFromWorkspace();
    }

    /**
     This method initializes this
     Fill values to all fields if these values are not empty
     
     @param inLibraryClassDefinitions The input data of LibraryClassDefinitionsDocument.LibraryClassDefinitions
     
     **/
    private void init(LibraryClassDefinitionsDocument.LibraryClassDefinitions inLibraryClassDefinitions) {
        init();
        this.lcd = inLibraryClassDefinitions;
        if (this.lcd != null) {
            if (this.lcd.getLibraryClassList().size() > 0) {
                for (int index = 0; index < this.lcd.getLibraryClassList().size(); index++) {
                    String name = lcd.getLibraryClassList().get(index).getKeyword();
                    String usage = null;
                    if (lcd.getLibraryClassList().get(index).getUsage() != null) {
                        usage = lcd.getLibraryClassList().get(index).getUsage().toString();    
                    }
                    String version = lcd.getLibraryClassList().get(index).getRecommendedInstanceVersion();
                    String guid = lcd.getLibraryClassList().get(index).getRecommendedInstanceGuid();
                    String featureFlag = lcd.getLibraryClassList().get(index).getFeatureFlag();
                    Vector<String> arch = Tools.convertListToVector(lcd.getLibraryClassList().get(index)
                                                                       .getSupArchList());
                    Vector<String> module = Tools.convertListToVector(lcd.getLibraryClassList().get(index)
                                                                         .getSupModuleList());
                    String help = lcd.getLibraryClassList().get(index).getHelpText();
                    LibraryClassIdentification lcid = new LibraryClassIdentification(name, usage, version, guid, arch,
                                                                                     featureFlag, module, help);
                    vLibraryClass.addLibraryClass(lcid);
                }
            }
        }
        //
        // Update the list
        //
        Tools.generateComboBoxByVector(jComboBoxList, vLibraryClass.getLibraryClassName());
        reloadListArea();
    }

    /**
     This method initializes this
     
     **/
    private void init() {
        this.setContentPane(getJScrollPane());
        this.setTitle("Library Class Definitions");
        this.setBounds(new java.awt.Rectangle(0, 0, 500, 515));
        initFrame();
        this.setViewMode(false);
    }

    /**
     Disable all components when the mode is view
     
     @param isView true - The view mode; false - The non-view mode
     
     **/
    public void setViewMode(boolean isView) {
        if (isView) {
            this.jComboBoxLibraryClassName.setEnabled(!isView);
            this.jComboBoxUsage.setEnabled(!isView);
        }
    }

    /**
     This method initializes jContentPane
     
     @return javax.swing.JPanel jContentPane
     
     **/
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jLabelHelpText = new JLabel();
            jLabelHelpText.setBounds(new java.awt.Rectangle(14, 60, 140, 20));
            jLabelHelpText.setText("Help Text");
            jLabelModuleList = new JLabel();
            jLabelModuleList.setBounds(new java.awt.Rectangle(15, 245, 140, 20));
            jLabelModuleList.setText("Sup Module List");
            jLabelArch = new JLabel();
            jLabelArch.setBounds(new java.awt.Rectangle(15, 160, 140, 20));
            jLabelArch.setText("Sup Arch List");
            jLabelFeatureFlag = new JLabel();
            jLabelFeatureFlag.setBounds(new java.awt.Rectangle(15, 135, 140, 20));
            jLabelFeatureFlag.setText("Feature Flag");
            jLabelRecommendedInstanceGuid = new JLabel();
            jLabelRecommendedInstanceGuid.setBounds(new java.awt.Rectangle(15, 110, 200, 20));
            jLabelRecommendedInstanceGuid.setText("Recommended Instance Guid");
            jLabelRecommendedInstanceVersion = new JLabel();
            jLabelRecommendedInstanceVersion.setBounds(new java.awt.Rectangle(15, 85, 200, 20));
            jLabelRecommendedInstanceVersion.setText("Recommended Instance Version");
            jLabelLibraryClassName = new JLabel();
            jLabelLibraryClassName.setBounds(new java.awt.Rectangle(15, 10, 140, 20));
            jLabelLibraryClassName.setText("Library Class Name");
            jLabelUsage = new JLabel();
            jLabelUsage.setBounds(new java.awt.Rectangle(15, 35, 140, 20));
            jLabelUsage.setText("Usage");
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.setPreferredSize(new java.awt.Dimension(490, 465));

            jContentPane.add(getJComboBoxLibraryClassName(), null);
            jContentPane.add(jLabelUsage, null);
            jContentPane.add(getJComboBoxUsage(), null);
            jContentPane.add(jLabelLibraryClassName, null);
            jContentPane.add(getJScrollPaneList(), null);
            jContentPane.add(getJComboBoxList(), null);
            jContentPane.add(getJButtonAdd(), null);
            jContentPane.add(getJButtonRemove(), null);
            jContentPane.add(getJButtonUpdate(), null);
            jContentPane.add(jLabelRecommendedInstanceVersion, null);
            jContentPane.add(getJTextFieldRecommendedInstanceVersion(), null);
            jContentPane.add(jLabelRecommendedInstanceGuid, null);
            jContentPane.add(getJTextFieldRecommendedInstanceGuid(), null);
            jContentPane.add(getJButtonGenerateGuid(), null);
            jContentPane.add(jLabelFeatureFlag, null);
            jContentPane.add(getJTextFieldFeatureFlag(), null);
            jContentPane.add(jLabelArch, null);
            jContentPane.add(getJScrollPaneArch(), null);
            jStarLabel1 = new StarLabel();
            jStarLabel1.setLocation(new java.awt.Point(0, 10));
            jStarLabel2 = new StarLabel();
            jStarLabel2.setLocation(new java.awt.Point(0, 35));

            jContentPane.add(jStarLabel1, null);
            jContentPane.add(jStarLabel2, null);
            jContentPane.add(jLabelModuleList, null);
            jContentPane.add(getJScrollPaneModuleList(), null);
            jContentPane.add(jLabelHelpText, null);
            jContentPane.add(getJTextFieldHelpText(), null);
        }
        return jContentPane;
    }

    /**
     This method initializes all existing libraries and usage types
     
     **/
    private void initFrame() {
        Tools.generateComboBoxByVector(jComboBoxLibraryClassName, vLib);
        Tools.generateComboBoxByVector(jComboBoxUsage, ed.getVLibraryUsage());
        this.iCheckBoxListArch.setAllItems(ed.getVSupportedArchitectures());
        this.iCheckBoxListModule.setAllItems(ed.getVFrameworkModuleTypes());
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
            updateForList();
        }
        if (arg0.getSource() == jButtonGenerateGuid) {
            this.jTextFieldRecommendedInstanceGuid.setText(Tools.generateUuidString());
        }
    }

    /**
     Data validation for all fields
     
     @retval true - All datas are valid
     @retval false - At least one data is invalid
     
     **/
    public boolean checkAdd() {
        //
        // Check LibraryClass
        //
        if (this.jComboBoxLibraryClassName.getSelectedItem() == null) {
            Log.err("No Library Class can be added");
            return false;
        }
        if (!DataValidation.isLibraryClass(this.jComboBoxLibraryClassName.getSelectedItem().toString())) {
            Log.err("Incorrect data type for Library Class");
            return false;
        }

        //
        // Check RecommendedInstanceVersion
        //
        if (!isEmpty(this.jTextFieldRecommendedInstanceVersion.getText())) {
            if (!DataValidation.isRecommendedInstanceVersion(this.jTextFieldRecommendedInstanceVersion.getText())) {
                Log.err("Incorrect data type for Recommended Instance Version");
                return false;
            }
        }

        //
        // Check RecommendedInstanceGuid
        //
        if (!isEmpty(this.jTextFieldRecommendedInstanceGuid.getText())) {
            if (!DataValidation.isGuid(this.jTextFieldRecommendedInstanceGuid.getText())) {
                Log.err("Incorrect data type for Recommended Instance Guid");
                return false;
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

        if (this.vLibraryClass.findLibraryClass(this.jComboBoxLibraryClassName.getSelectedItem().toString()) > -1) {
            Log.err("The Library Class has been added already!");
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
            int intLibraryCount = this.vLibraryClass.size();

            lcd = LibraryClassDefinitions.Factory.newInstance();
            if (intLibraryCount > 0) {
                for (int index = 0; index < intLibraryCount; index++) {
                    LibraryClass mLibraryClass = LibraryClass.Factory.newInstance();

                    mLibraryClass.setKeyword(vLibraryClass.getLibraryClass(index).getLibraryClassName());
                    mLibraryClass
                                 .setUsage(LibraryUsage.Enum.forString(vLibraryClass.getLibraryClass(index).getUsage()));
                    if (!isEmpty(vLibraryClass.getLibraryClass(index).getRecommendedInstanceVersion())) {
                        mLibraryClass.setRecommendedInstanceVersion(vLibraryClass.getLibraryClass(index)
                                                                                 .getRecommendedInstanceVersion());
                    }
                    if (!isEmpty(vLibraryClass.getLibraryClass(index).getRecommendedInstanceGuid())) {
                        mLibraryClass.setRecommendedInstanceGuid(vLibraryClass.getLibraryClass(index)
                                                                              .getRecommendedInstanceGuid());
                    }
                    if (!isEmpty(vLibraryClass.getLibraryClass(index).getFeatureFlag())) {
                        mLibraryClass.setFeatureFlag(vLibraryClass.getLibraryClass(index).getFeatureFlag());
                    }
                    if (vLibraryClass.getLibraryClass(index).getSupArchList() != null
                        && vLibraryClass.getLibraryClass(index).getSupArchList().size() > 0) {
                        mLibraryClass.setSupArchList(vLibraryClass.getLibraryClass(index).getSupArchList());
                    }
                    if (!isEmpty(vLibraryClass.getLibraryClass(index).getHelp())) {
                        mLibraryClass.setHelpText(vLibraryClass.getLibraryClass(index).getHelp());
                    }

                    this.lcd.addNewLibraryClass();
                    this.lcd.setLibraryClassArray(index, mLibraryClass);
                }
            }

            if (msa.getLibraryClassDefinitions() == null) {
                this.msa.addNewLibraryClassDefinitions();
            }
            this.msa.setLibraryClassDefinitions(this.lcd);

            this.omt.setSaved(false);
        } catch (Exception e) {
            Log.err("Update Library Class Definitions", e.getMessage());
        }
    }

    private LibraryClassIdentification getCurrentLibraryClass() {
        String name = this.jComboBoxLibraryClassName.getSelectedItem().toString();
        String usage = this.jComboBoxUsage.getSelectedItem().toString();
        String version = this.jTextFieldRecommendedInstanceVersion.getText();
        String guid = this.jTextFieldRecommendedInstanceGuid.getText();
        String featureFlag = this.jTextFieldFeatureFlag.getText();
        Vector<String> arch = this.iCheckBoxListArch.getAllCheckedItemsString();
        Vector<String> module = this.iCheckBoxListModule.getAllCheckedItemsString();
        String help = this.jTextFieldHelpText.getText();
        lcid = new LibraryClassIdentification(name, usage, version, guid, arch, featureFlag, module, help);
        return lcid;
    }

    /**
     Add current item to Vector
     
     **/
    private void addToList() {
        intSelectedItemId = vLibraryClass.size();

        vLibraryClass.addLibraryClass(getCurrentLibraryClass());

        jComboBoxList.addItem(lcid.getLibraryClassName());
        jComboBoxList.setSelectedItem(lcid.getLibraryClassName());

        //
        // Reset select item index
        //
        intSelectedItemId = vLibraryClass.size();

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
        if (this.vLibraryClass.size() < 1) {
            return;
        }

        int intTempIndex = intSelectedItemId;

        jComboBoxList.removeItemAt(intSelectedItemId);

        vLibraryClass.removeLibraryClass(intTempIndex);

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
        if (this.vLibraryClass.size() < 1) {
            return;
        }

        //
        // Backup selected item index
        //
        int intTempIndex = intSelectedItemId;

        vLibraryClass.updateLibraryClass(getCurrentLibraryClass(), intTempIndex);

        jComboBoxList.removeAllItems();
        for (int index = 0; index < vLibraryClass.size(); index++) {
            jComboBoxList.addItem(vLibraryClass.getLibraryClass(index).getLibraryClassName());
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
        if (vLibraryClass.size() > 0) {
            //
            // Get selected item index
            //
            intSelectedItemId = jComboBoxList.getSelectedIndex();

            this.jComboBoxLibraryClassName.setSelectedItem(vLibraryClass.getLibraryClass(intSelectedItemId)
                                                                        .getLibraryClassName());
            this.jComboBoxUsage.setSelectedItem(vLibraryClass.getLibraryClass(intSelectedItemId).getUsage());
            this.jTextFieldRecommendedInstanceVersion.setText(vLibraryClass.getLibraryClass(intSelectedItemId)
                                                                           .getRecommendedInstanceVersion());
            this.jTextFieldRecommendedInstanceGuid.setText(vLibraryClass.getLibraryClass(intSelectedItemId)
                                                                        .getRecommendedInstanceGuid());
            this.jTextFieldFeatureFlag.setText(vLibraryClass.getLibraryClass(intSelectedItemId).getFeatureFlag());
            this.iCheckBoxListArch.setAllItemsUnchecked();
            this.iCheckBoxListArch.initCheckedItem(true, vLibraryClass.getLibraryClass(intSelectedItemId)
                                                                      .getSupArchList());
            this.iCheckBoxListModule.setAllItemsUnchecked();
            this.iCheckBoxListModule.initCheckedItem(true, vLibraryClass.getLibraryClass(intSelectedItemId)
                                                                        .getSupModuleList());
            this.jTextFieldHelpText.setText(vLibraryClass.getLibraryClass(intSelectedItemId).getHelp());
        } else {
        }

        reloadListArea();
    }

    /**
     Update list area pane via the elements of Vector
     
     **/
    private void reloadListArea() {
        String strListItem = "";
        for (int index = 0; index < vLibraryClass.size(); index++) {
            strListItem = strListItem + vLibraryClass.getLibraryClass(index).getLibraryClassName()
                          + DataType.UNIX_LINE_SEPARATOR;
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
        if (arg0.getStateChange() == ItemEvent.SELECTED) {
            reloadFromList();
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

        resizeComponentWidth(this.jComboBoxLibraryClassName, intCurrentWidth, intPreferredWidth);
        resizeComponentWidth(this.jComboBoxUsage, intCurrentWidth, intPreferredWidth);
        resizeComponentWidth(this.jTextFieldHelpText, intCurrentWidth, intPreferredWidth);
        resizeComponentWidth(this.jTextFieldRecommendedInstanceVersion, intCurrentWidth, intPreferredWidth);
        resizeComponentWidth(this.jTextFieldRecommendedInstanceGuid, intCurrentWidth, intPreferredWidth);
        resizeComponentWidth(this.jTextFieldFeatureFlag, intCurrentWidth, intPreferredWidth);
        resizeComponentWidth(this.jScrollPaneArch, intCurrentWidth, intPreferredWidth);
        resizeComponentWidth(this.jScrollPaneModuleList, intCurrentWidth, intPreferredWidth);

        relocateComponentX(this.jButtonGenerateGuid, intCurrentWidth, intPreferredWidth,
                           DataType.SPACE_TO_RIGHT_FOR_GENERATE_BUTTON);

        resizeComponentWidth(this.jComboBoxList, intCurrentWidth, intPreferredWidth);
        resizeComponent(this.jScrollPaneList, intCurrentWidth, intCurrentHeight, intPreferredWidth, intPreferredHeight);
        relocateComponentX(this.jButtonAdd, intCurrentWidth, intPreferredWidth, DataType.SPACE_TO_RIGHT_FOR_ADD_BUTTON);
        relocateComponentX(this.jButtonRemove, intCurrentWidth, intPreferredWidth,
                           DataType.SPACE_TO_RIGHT_FOR_REMOVE_BUTTON);
        relocateComponentX(this.jButtonUpdate, intCurrentWidth, intPreferredWidth,
                           DataType.SPACE_TO_RIGHT_FOR_UPDATE_BUTTON);
    }
}
