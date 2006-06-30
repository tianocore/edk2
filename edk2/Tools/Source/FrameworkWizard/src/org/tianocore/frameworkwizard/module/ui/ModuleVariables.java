/** @file
 
 The file is used to create, update Variable of MSA/MBD file
 
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
import java.util.Vector;

import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;
import javax.swing.JTextField;

import org.tianocore.VariableUsage;
import org.tianocore.VariablesDocument;
import org.tianocore.ModuleSurfaceAreaDocument.ModuleSurfaceArea;
import org.tianocore.VariablesDocument.Variables;
import org.tianocore.VariablesDocument.Variables.Variable;
import org.tianocore.frameworkwizard.common.DataType;
import org.tianocore.frameworkwizard.common.DataValidation;
import org.tianocore.frameworkwizard.common.EnumerationData;
import org.tianocore.frameworkwizard.common.Log;
import org.tianocore.frameworkwizard.common.OpeningModuleType;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.ui.IInternalFrame;
import org.tianocore.frameworkwizard.common.ui.StarLabel;
import org.tianocore.frameworkwizard.common.ui.iCheckBoxList.ICheckBoxList;
import org.tianocore.frameworkwizard.module.Identification.Variables.VariablesIdentification;
import org.tianocore.frameworkwizard.module.Identification.Variables.VariablesVector;

/**
 The class is used to create, update Variable of MSA/MBD file
 It extends IInternalFrame
 
 **/
public class ModuleVariables extends IInternalFrame {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = -6998982978030439446L;

    //
    //Define class members
    //
    private JPanel jContentPane = null;

    private JLabel jLabelVariableName = null;

    private JTextField jTextFieldVariableName = null;

    private JLabel jLabelUsage = null;

    private JComboBox jComboBoxUsage = null;

    private StarLabel jStarLabel1 = null;

    private StarLabel jStarLabel2 = null;

    private JTextArea jTextAreaList = null;

    private JComboBox jComboBoxList = null;

    private JButton jButtonAdd = null;

    private JButton jButtonRemove = null;

    private JButton jButtonUpdate = null;

    private JScrollPane jScrollPane = null;

    private JScrollPane jScrollPaneList = null;

    private JLabel jLabelGuidCName = null;

    private JTextField jTextFieldGuidC_Name = null;

    private JTextField jTextFieldFeatureFlag = null;

    private JLabel jLabelFeatureFlag = null;

    private JLabel jLabelArch = null;

    private ICheckBoxList iCheckBoxListArch = null;

    private JScrollPane jScrollPaneArch = null;
    
    private JLabel jLabelHelpText = null;

    private JTextField jTextFieldHelpText = null;

    //
    // Not used by UI
    //
    private int intSelectedItemId = 0;

    private OpeningModuleType omt = null;

    private ModuleSurfaceArea msa = null;

    private VariablesDocument.Variables variables = null;

    private VariablesIdentification id = null;

    private VariablesVector vid = new VariablesVector();

    private EnumerationData ed = new EnumerationData();

    /**
     This method initializes jTextFieldString 
     
     @return javax.swing.JTextField jTextFieldString
     
     **/
    private JTextField getJTextFieldString() {
        if (jTextFieldVariableName == null) {
            jTextFieldVariableName = new JTextField();
            jTextFieldVariableName.setSize(new java.awt.Dimension(320, 20));
            jTextFieldVariableName.setPreferredSize(new java.awt.Dimension(320, 20));
            jTextFieldVariableName.setLocation(new java.awt.Point(160, 10));
        }
        return jTextFieldVariableName;
    }

    /**
     This method initializes jComboBoxUsage 
     
     @return javax.swing.JComboBox jComboBoxUsage
     
     **/
    private JComboBox getJComboBoxUsage() {
        if (jComboBoxUsage == null) {
            jComboBoxUsage = new JComboBox();
            jComboBoxUsage.setBounds(new java.awt.Rectangle(160, 60, 320, 20));
            jComboBoxUsage.setPreferredSize(new java.awt.Dimension(320, 20));
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
            jComboBoxList.setBounds(new java.awt.Rectangle(15, 220, 210, 20));
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
            jButtonAdd.setBounds(new java.awt.Rectangle(230, 220, 80, 20));
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
            jButtonRemove.setBounds(new java.awt.Rectangle(400, 220, 80, 20));
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
            jButtonUpdate.setBounds(new java.awt.Rectangle(315, 220, 80, 20));
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
            jScrollPaneList.setBounds(new java.awt.Rectangle(15, 245, 465, 240));
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
     * This method initializes jTextFieldGuidCName	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldGuidC_Name() {
        if (jTextFieldGuidC_Name == null) {
            jTextFieldGuidC_Name = new JTextField();
            jTextFieldGuidC_Name.setBounds(new java.awt.Rectangle(160, 35, 320, 20));
            jTextFieldGuidC_Name.setPreferredSize(new java.awt.Dimension(320, 20));
        }
        return jTextFieldGuidC_Name;
    }

    /**
     This method initializes jTextFieldFeatureFlag 
     
     @return javax.swing.JTextField jTextFieldFeatureFlag
     
     **/
    private JTextField getJTextFieldFeatureFlag() {
        if (jTextFieldFeatureFlag == null) {
            jTextFieldFeatureFlag = new JTextField();
            jTextFieldFeatureFlag.setBounds(new java.awt.Rectangle(160, 110, 320, 20));
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
            jScrollPaneArch.setBounds(new java.awt.Rectangle(160, 135, 320, 80));
            jScrollPaneArch.setPreferredSize(new java.awt.Dimension(320, 80));
            jScrollPaneArch.setViewportView(getICheckBoxListSupportedArchitectures());
        }
        return jScrollPaneArch;
    }
    
    /**
    This method initializes jTextFieldHelpText  
    
    @return javax.swing.JTextField  
    
    **/
   private JTextField getJTextFieldHelpText() {
       if (jTextFieldHelpText == null) {
           jTextFieldHelpText = new JTextField();
           jTextFieldHelpText.setBounds(new java.awt.Rectangle(160, 85, 320, 20));
           jTextFieldHelpText.setPreferredSize(new java.awt.Dimension(320, 20));
       }
       return jTextFieldHelpText;
   }

    public static void main(String[] args) {

    }
    
    /**
    This method initializes this
    
    **/
   private void init() {
       this.setSize(500, 515);
       this.setContentPane(getJScrollPane());
       this.setTitle("Variables");
       initFrame();
       this.setViewMode(false);
   }

   /**
    This method initializes this
    Fill values to all fields if these values are not empty
    
    @param inPackageDependencies

    **/
   private void init(Variables inVariables) {
       init();
       this.variables = inVariables;

       if (this.variables != null) {
           if (this.variables.getVariableList().size() > 0) {
               for (int index = 0; index < this.variables.getVariableList().size(); index++) {
                   String arg0 = variables.getVariableList().get(index).getVariableName();
                   String arg1 = variables.getVariableList().get(index).getGuidCName();
                   String arg2 = null;
                   if (variables.getVariableList().get(index).getUsage() != null) {
                       arg2 = variables.getVariableList().get(index).getUsage().toString();    
                   }
                   
                   String arg3 = variables.getVariableList().get(index).getFeatureFlag();
                   Vector<String> arg4 = Tools.convertListToVector(variables.getVariableList().get(index).getSupArchList());
                   String arg5 = variables.getVariableList().get(index).getHelpText();
                   id = new VariablesIdentification(arg0, arg1, arg2, arg3, arg4, arg5);
                   vid.addVariables(id);
               }
           }
       }
       //
       // Update the list
       //
       Tools.generateComboBoxByVector(jComboBoxList, vid.getVariablesName());
       reloadListArea();
   }

    /**
     This is the default constructor
     
     **/
    public ModuleVariables() {
        super();
        init();
        this.setVisible(true);
    }

    /**
     This is the override edit constructor
     
     @param inVariables The input data of VariablesDocument.Variables
     
     **/
    public ModuleVariables(OpeningModuleType inOmt) {
        super();
        this.omt = inOmt;
        this.msa = omt.getXmlMsa();
        init(msa.getVariables());
        this.setVisible(true);
    }

    /**
     Disable all components when the mode is view
     
     @param isView true - The view mode; false - The non-view mode
     
     **/
    public void setViewMode(boolean isView) {
        if (isView) {
            this.jTextFieldVariableName.setEnabled(!isView);
            this.jComboBoxUsage.setEnabled(!isView);
        }
    }

    /**
     This method initializes jContentPane
     
     @return javax.swing.JPanel jContentPane
     
     **/
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jLabelGuidCName = new JLabel();
            jLabelGuidCName.setBounds(new java.awt.Rectangle(15, 35, 140, 20));
            jLabelGuidCName.setText("Guid C_Name");
            jLabelUsage = new JLabel();
            jLabelUsage.setText("Usage");
            jLabelUsage.setBounds(new java.awt.Rectangle(15, 60, 140, 20));
            jLabelVariableName = new JLabel();
            jLabelVariableName.setText("Variable Name");
            jLabelVariableName.setLocation(new java.awt.Point(15, 10));
            jLabelVariableName.setSize(new java.awt.Dimension(140, 20));
            jLabelArch = new JLabel();
            jLabelArch.setBounds(new java.awt.Rectangle(15, 135, 140, 20));
            jLabelArch.setText("Arch");
            jLabelFeatureFlag = new JLabel();
            jLabelFeatureFlag.setText("Feature Flag");
            jLabelFeatureFlag.setBounds(new java.awt.Rectangle(15, 110, 140, 20));

            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.setPreferredSize(new java.awt.Dimension(490, 495));

            jContentPane.add(jLabelVariableName, null);
            jContentPane.add(jLabelGuidCName, null);
            jContentPane.add(getJTextFieldGuidC_Name(), null);
            jContentPane.add(getJTextFieldString(), null);
            jContentPane.add(jLabelUsage, null);
            jContentPane.add(getJComboBoxUsage(), null);
            jStarLabel1 = new StarLabel();
            jStarLabel1.setLocation(new java.awt.Point(0, 10));
            jStarLabel2 = new StarLabel();
            jStarLabel2.setLocation(new java.awt.Point(0, 35));
            jLabelHelpText = new JLabel();
            jLabelHelpText.setBounds(new java.awt.Rectangle(14, 85, 140, 20));
            jLabelHelpText.setText("Help Text");

            jContentPane.add(jStarLabel1, null);
            jContentPane.add(jStarLabel2, null);

            jContentPane.add(getJComboBoxList(), null);
            jContentPane.add(getJButtonAdd(), null);
            jContentPane.add(getJButtonRemove(), null);
            jContentPane.add(getJButtonUpdate(), null);
            jContentPane.add(getJScrollPaneList(), null);

            jContentPane.add(jLabelArch, null);
            jContentPane.add(jLabelFeatureFlag, null);
            jContentPane.add(getJTextFieldFeatureFlag(), null);
            jContentPane.add(getJScrollPaneArch(), null);
            jContentPane.add(jLabelHelpText, null);
            jContentPane.add(getJTextFieldHelpText(), null);
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
     This method initializes Usage type
     
     **/
    private void initFrame() {
        Tools.generateComboBoxByVector(jComboBoxUsage, ed.getVPpiUsage());
        this.iCheckBoxListArch.setAllItems(ed.getVSupportedArchitectures());
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
        // Check VariableName 
        //
        if (isEmpty(this.jTextFieldVariableName.getText())) {
            Log.err("Variable Name couldn't be empty");
            return false;
        }
        
        if (!isEmpty(this.jTextFieldVariableName.getText())) {
            if (!DataValidation.isHexWordArrayType(this.jTextFieldVariableName.getText())) {
                Log.err("Incorrect data type for Variable Name");
                return false;
            }
        }
        
        //
        // Check GuidC_Name 
        //
        if (isEmpty(this.jTextFieldGuidC_Name.getText())) {
            Log.err("Guid C_Name couldn't be empty");
            return false;
        }
        
        if (!isEmpty(this.jTextFieldGuidC_Name.getText())) {
            if (!DataValidation.isC_NameType(this.jTextFieldGuidC_Name.getText())) {
                Log.err("Incorrect data type for Guid C_Name");
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

        return true;
    }

    /**
     Save all components of Variables
     if exists variables, set the value directly
     if not exists variables, new an instance first
     
     **/
    public void save() {
        try {
            int count = this.vid.size();

            this.variables = Variables.Factory.newInstance();
            if (count > 0) {
                for (int index = 0; index < count; index++) {
                    Variable p = Variable.Factory.newInstance();
                    if (!isEmpty(vid.getVariables(index).getName())) {
                        p.setVariableName(vid.getVariables(index).getName());
                    }
                    if (!isEmpty(vid.getVariables(index).getGuid())) {
                        p.setGuidCName(vid.getVariables(index).getGuid());
                    }
                    if (!isEmpty(vid.getVariables(index).getUsage())) {
                        p.setUsage(VariableUsage.Enum.forString(vid.getVariables(index).getUsage()));
                    }
                    if (!isEmpty(vid.getVariables(index).getFeatureFlag())) {
                        p.setFeatureFlag(vid.getVariables(index).getFeatureFlag());
                    }
                    if (vid.getVariables(index).getSupArchList() != null && vid.getVariables(index).getSupArchList().size() > 0) {
                        p.setSupArchList(vid.getVariables(index).getSupArchList());
                    }
                    if (!isEmpty(vid.getVariables(index).getHelp())) {
                        p.setHelpText(vid.getVariables(index).getHelp());
                    }
                    this.variables.addNewVariable();
                    this.variables.setVariableArray(variables.getVariableList().size() - 1, p);
                }
            }

            this.msa.setVariables(variables);
            this.omt.setSaved(false);
        } catch (Exception e) {
            Log.err("Update Variables", e.getMessage());
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

        resizeComponentWidth(jTextFieldVariableName, intCurrentWidth, intPreferredWidth);
        resizeComponentWidth(jTextFieldGuidC_Name, intCurrentWidth, intPreferredWidth);
        resizeComponentWidth(jComboBoxUsage, intCurrentWidth, intPreferredWidth);
        resizeComponentWidth(jTextFieldHelpText, intCurrentWidth, intPreferredWidth);
        resizeComponentWidth(jTextFieldFeatureFlag, intCurrentWidth, intPreferredWidth);
        resizeComponentWidth(jScrollPaneArch, intCurrentWidth, intPreferredWidth);

        resizeComponentWidth(jComboBoxList, intCurrentWidth, intPreferredWidth);
        resizeComponent(jScrollPaneList, intCurrentWidth, intCurrentHeight, intPreferredWidth, intPreferredHeight);
        relocateComponentX(jButtonAdd, intCurrentWidth, intPreferredWidth, DataType.SPACE_TO_RIGHT_FOR_ADD_BUTTON);
        relocateComponentX(jButtonRemove, intCurrentWidth, intPreferredWidth, DataType.SPACE_TO_RIGHT_FOR_REMOVE_BUTTON);
        relocateComponentX(jButtonUpdate, intCurrentWidth, intPreferredWidth, DataType.SPACE_TO_RIGHT_FOR_UPDATE_BUTTON);
    }
    
    private VariablesIdentification getCurrentVariables() {
        String arg0 = this.jTextFieldVariableName.getText();
        String arg1 = this.jTextFieldGuidC_Name.getText();
        String arg2 = this.jComboBoxUsage.getSelectedItem().toString();

        String arg3 = this.jTextFieldFeatureFlag.getText();
        Vector<String> arg4 = this.iCheckBoxListArch.getAllCheckedItemsString();
        String arg5 = this.jTextFieldHelpText.getText();
        
        id = new VariablesIdentification(arg0, arg1, arg2, arg3, arg4, arg5);
        return id;
    }
    
    /**
    Add current item to Vector
    
    **/
   private void addToList() {
       intSelectedItemId = vid.size();

       vid.addVariables(getCurrentVariables());

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

       vid.removeVariables(intTempIndex);

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

       vid.updateVariables(getCurrentVariables(), intTempIndex);

       jComboBoxList.removeAllItems();
       for (int index = 0; index < vid.size(); index++) {
           jComboBoxList.addItem(vid.getVariables(index).getName());
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

           this.jTextFieldVariableName.setText(vid.getVariables(intSelectedItemId).getName());
           this.jTextFieldGuidC_Name.setText(vid.getVariables(intSelectedItemId).getGuid());
           this.jComboBoxUsage.setSelectedItem(vid.getVariables(intSelectedItemId).getUsage());
           this.jTextFieldHelpText.setText(vid.getVariables(intSelectedItemId).getHelp());

           jTextFieldFeatureFlag.setText(vid.getVariables(intSelectedItemId).getFeatureFlag());
           iCheckBoxListArch.setAllItemsUnchecked();
           iCheckBoxListArch.initCheckedItem(true, vid.getVariables(intSelectedItemId).getSupArchList());

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
           strListItem = strListItem + vid.getVariables(index).getName() + DataType.UNIX_LINE_SEPARATOR;
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
   }
}
