/** @file
 
 The file is used to create, update PCD of MSA/MBD file
 
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

import org.tianocore.PcdCodedDocument;
import org.tianocore.PcdItemTypes;
import org.tianocore.ModuleSurfaceAreaDocument.ModuleSurfaceArea;
import org.tianocore.PcdCodedDocument.PcdCoded;
import org.tianocore.PcdCodedDocument.PcdCoded.PcdEntry;
import org.tianocore.frameworkwizard.common.DataType;
import org.tianocore.frameworkwizard.common.DataValidation;
import org.tianocore.frameworkwizard.common.EnumerationData;
import org.tianocore.frameworkwizard.common.Log;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.Identifications.OpeningModuleType;
import org.tianocore.frameworkwizard.common.ui.IInternalFrame;
import org.tianocore.frameworkwizard.common.ui.StarLabel;
import org.tianocore.frameworkwizard.common.ui.iCheckBoxList.ICheckBoxList;
import org.tianocore.frameworkwizard.module.Identifications.PcdCoded.PcdCodedIdentification;
import org.tianocore.frameworkwizard.module.Identifications.PcdCoded.PcdCodedVector;
import org.tianocore.frameworkwizard.workspace.WorkspaceTools;

/**
 The class is used to create, update PCD of MSA/MBD file
 It extends IInternalFrame
 


 **/
public class ModulePCDs extends IInternalFrame {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = 2227717658188438696L;

    //
    //Define class members
    //
    private JPanel jContentPane = null;

    private JLabel jLabelItemType = null;

    private JLabel jLabelC_Name = null;

    private JComboBox jComboBoxItemType = null;

    private JComboBox jComboBoxCName = null;

    private JLabel jLabelDefaultValue = null;

    private JTextField jTextFieldDefaultValue = null;

    private StarLabel jStarLabel1 = null;

    private StarLabel jStarLabel2 = null;

    private StarLabel jStarLabel3 = null;

    private JLabel jLabelHelpText = null;

    private JTextField jTextFieldHelpText = null;

    private JTextArea jTextAreaList = null;

    private JComboBox jComboBoxList = null;

    private JButton jButtonAdd = null;

    private JButton jButtonRemove = null;

    private JButton jButtonUpdate = null;

    private JScrollPane jScrollPane = null;

    private JScrollPane jScrollPaneList = null;

    private JLabel jLabelTokenSpaceGuid = null;

    private JTextField jTextFieldTokenSpaceGuid = null;

    private JLabel jLabelFeatureFlag = null;

    private JTextField jTextFieldFeatureFlag = null;

    private JLabel jLabelArch = null;

    private ICheckBoxList iCheckBoxListArch = null;

    private JScrollPane jScrollPaneArch = null;

    //
    // Not used by UI
    //
    private int intSelectedItemId = 0;

    private OpeningModuleType omt = null;

    private ModuleSurfaceArea msa = null;

    private PcdCodedDocument.PcdCoded pcds = null;

    private PcdCodedIdentification id = null;

    private PcdCodedVector vid = new PcdCodedVector();

    private EnumerationData ed = new EnumerationData();
    
    private WorkspaceTools wt = new WorkspaceTools();

    /**
     This method initializes jComboBoxItemType 
     
     @return javax.swing.JComboBox jComboBoxItemType
     
     **/
    private JComboBox getJComboBoxItemType() {
        if (jComboBoxItemType == null) {
            jComboBoxItemType = new JComboBox();
            jComboBoxItemType.setBounds(new java.awt.Rectangle(160, 35, 320, 20));
            jComboBoxItemType.setPreferredSize(new java.awt.Dimension(320, 20));
        }
        return jComboBoxItemType;
    }

    /**
     This method initializes jTextFieldC_Name 
     
     @return javax.swing.JTextField jTextFieldC_Name
     
     **/
    private JComboBox getJComboBoxCName() {
        if (jComboBoxCName == null) {
            jComboBoxCName = new JComboBox();
            jComboBoxCName.setBounds(new java.awt.Rectangle(160, 10, 320, 20));
            jComboBoxCName.setPreferredSize(new java.awt.Dimension(320, 20));
        }
        return jComboBoxCName;
    }

    /**
     This method initializes jTextFieldDefaultValue 
     
     @return javax.swing.JTextField jTextFieldDefaultValue
     
     **/
    private JTextField getJTextFieldDefaultValue() {
        if (jTextFieldDefaultValue == null) {
            jTextFieldDefaultValue = new JTextField();
            jTextFieldDefaultValue.setBounds(new java.awt.Rectangle(160, 85, 320, 20));
            jTextFieldDefaultValue.setPreferredSize(new java.awt.Dimension(320, 20));
        }
        return jTextFieldDefaultValue;
    }

    /**
     * This method initializes jTextFieldHelpText	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldHelpText() {
        if (jTextFieldHelpText == null) {
            jTextFieldHelpText = new JTextField();
            jTextFieldHelpText.setBounds(new java.awt.Rectangle(160, 110, 320, 20));
            jTextFieldHelpText.setPreferredSize(new java.awt.Dimension(320, 20));
        }
        return jTextFieldHelpText;
    }

    /**
     This method initializes jComboBoxFileList 
     
     @return javax.swing.JComboBox jComboBoxFileList
     
     **/
    private JComboBox getJComboBoxList() {
        if (jComboBoxList == null) {
            jComboBoxList = new JComboBox();
            jComboBoxList.setBounds(new java.awt.Rectangle(15, 245, 210, 20));
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
            jButtonAdd.setBounds(new java.awt.Rectangle(230, 245, 80, 20));
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
            jButtonRemove.setBounds(new java.awt.Rectangle(400, 245, 80, 20));
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
            jButtonUpdate.setBounds(new java.awt.Rectangle(315, 245, 80, 20));
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
            jScrollPaneList.setBounds(new java.awt.Rectangle(15, 270, 465, 240));
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
     * This method initializes jTextFieldTokenSpaceGuid	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldTokenSpaceGuid() {
        if (jTextFieldTokenSpaceGuid == null) {
            jTextFieldTokenSpaceGuid = new JTextField();
            jTextFieldTokenSpaceGuid.setBounds(new java.awt.Rectangle(160, 60, 320, 20));
            jTextFieldTokenSpaceGuid.setPreferredSize(new java.awt.Dimension(320, 20));
        }
        return jTextFieldTokenSpaceGuid;
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
            jScrollPaneArch.setBounds(new java.awt.Rectangle(160, 160, 320, 80));
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
       this.setTitle("Pcd Coded");
       initFrame();
       this.setViewMode(false);
   }

   /**
    This method initializes this
    Fill values to all fields if these values are not empty
    
    @param inPackageDependencies

    **/
   private void init(PcdCoded inPcdCodeds) {
       init();
       this.pcds = inPcdCodeds;

       if (this.pcds != null) {
           if (this.pcds.getPcdEntryList().size() > 0) {
               for (int index = 0; index < this.pcds.getPcdEntryList().size(); index++) {
                   String arg0 = pcds.getPcdEntryList().get(index).getCName();
                   String arg1 = pcds.getPcdEntryList().get(index).getTokenSpaceGuidCName();
                   

                   String arg2 = pcds.getPcdEntryList().get(index).getFeatureFlag();
                   Vector<String> arg3 = Tools.convertListToVector(pcds.getPcdEntryList().get(index).getSupArchList());
                   
                   String arg4 = pcds.getPcdEntryList().get(index).getDefaultValue();
                   String arg5 = pcds.getPcdEntryList().get(index).getHelpText();
                   String arg6 = null;
                   if (pcds.getPcdEntryList().get(index).getPcdItemType() != null) {
                       arg6 = pcds.getPcdEntryList().get(index).getPcdItemType().toString();
                   }
                   
                   id = new PcdCodedIdentification(arg0, arg1, arg2, arg3, arg4, arg5, arg6);
                   vid.addPcdCoded(id);
               }
           }
       }
       //
       // Update the list
       //
       Tools.generateComboBoxByVector(jComboBoxList, vid.getPcdCodedName());
       reloadListArea();
   }
    
    /**
     This is the default constructor
     
     **/
    public ModulePCDs() {
        super();
        init();
        this.setVisible(true);
    }

    /**
     This is the override edit constructor
     
     @param inPcds The input data of PCDsDocument.PCDs
     
     **/
    public ModulePCDs(OpeningModuleType inOmt) {
        super();
        this.omt = inOmt;
        this.msa = omt.getXmlMsa();
        init(msa.getPcdCoded());
        this.setVisible(true);
    }

    /**
     Disable all components when the mode is view
     
     @param isView true - The view mode; false - The non-view mode
     
     **/
    public void setViewMode(boolean isView) {
        if (isView) {
            this.jTextFieldDefaultValue.setEnabled(!isView);
            this.jComboBoxItemType.setEnabled(!isView);
        }
    }

    /**
     This method initializes jContentPane
     
     @return javax.swing.JPanel jContentPane
     
     **/
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jLabelArch = new JLabel();
            jLabelArch.setBounds(new java.awt.Rectangle(15, 160, 140, 20));
            jLabelArch.setText("Sup Arch List");
            jLabelFeatureFlag = new JLabel();
            jLabelFeatureFlag.setBounds(new java.awt.Rectangle(15, 135, 140, 20));
            jLabelFeatureFlag.setText("Feature Flag");
            jLabelTokenSpaceGuid = new JLabel();
            jLabelTokenSpaceGuid.setBounds(new java.awt.Rectangle(15, 60, 140, 20));
            jLabelTokenSpaceGuid.setText("Token Space C_Name");
            jLabelHelpText = new JLabel();
            jLabelHelpText.setBounds(new java.awt.Rectangle(15, 110, 137, 19));
            jLabelHelpText.setText("Help Text");
            jLabelC_Name = new JLabel();
            jLabelC_Name.setText("C_Name");
            jLabelC_Name.setBounds(new java.awt.Rectangle(15, 10, 140, 20));
            jLabelDefaultValue = new JLabel();
            jLabelDefaultValue.setText("Default Value");
            jLabelDefaultValue.setBounds(new java.awt.Rectangle(15, 85, 140, 20));
            jLabelItemType = new JLabel();
            jLabelItemType.setText("Item Type");
            jLabelItemType.setBounds(new java.awt.Rectangle(15, 35, 140, 20));

            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.setPreferredSize(new java.awt.Dimension(490, 520));

            jContentPane.add(jLabelItemType, null);
            jContentPane.add(jLabelC_Name, null);
            jContentPane.add(getJComboBoxCName(), null);
            jContentPane.add(jLabelDefaultValue, null);
            jContentPane.add(getJTextFieldDefaultValue(), null);
            jContentPane.add(getJComboBoxItemType(), null);
            jStarLabel1 = new StarLabel();
            jStarLabel1.setLocation(new java.awt.Point(0, 10));
            jStarLabel2 = new StarLabel();
            jStarLabel2.setLocation(new java.awt.Point(0, 35));
            jStarLabel3 = new StarLabel();
            jStarLabel3.setLocation(new java.awt.Point(0, 110));
            jContentPane.add(jStarLabel1, null);
            jContentPane.add(jStarLabel2, null);
            jContentPane.add(jStarLabel3, null);
            jContentPane.add(jLabelHelpText, null);
            jContentPane.add(getJTextFieldHelpText(), null);

            jContentPane.add(getJComboBoxList(), null);
            jContentPane.add(getJButtonAdd(), null);
            jContentPane.add(getJButtonRemove(), null);
            jContentPane.add(getJButtonUpdate(), null);
            jContentPane.add(getJScrollPaneList(), null);
            jContentPane.add(jLabelTokenSpaceGuid, null);
            jContentPane.add(getJTextFieldTokenSpaceGuid(), null);
            jContentPane.add(jLabelFeatureFlag, null);
            jContentPane.add(getJTextFieldFeatureFlag(), null);
            jContentPane.add(jLabelArch, null);
            jContentPane.add(getJScrollPaneArch(), null);
        }
        return jContentPane;
    }

    /**
     This method initializes Usage type, Item type and Datum type
     
     **/
    private void initFrame() {
        Tools.generateComboBoxByVector(jComboBoxCName, wt.getAllPcdDeclarationsFromWorkspace());
        Tools.generateComboBoxByVector(jComboBoxItemType, ed.getVPcdItemTypes());
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
        // Check C_Name 
        //
        if (!isEmpty(this.jComboBoxCName.getSelectedItem().toString())) {
            if (!DataValidation.isC_NameType(this.jComboBoxCName.getSelectedItem().toString())) {
                Log.err("Incorrect data type for C_Name");
                return false;
            }
        }
        
        //
        // Check TokenSpaceGuid
        //
        if (!isEmpty(this.jTextFieldTokenSpaceGuid.getText())) {
            if (!DataValidation.isGuid(this.jTextFieldTokenSpaceGuid.getText())) {
                Log.err("Incorrect data type for Token Space C_Name");
                return false;
            }
        }
        
        //
        // Check DefaultValue
        //
        if (!isEmpty(this.jTextFieldDefaultValue.getText())) {
            if (!DataValidation.isDefaultValueType(this.jTextFieldDefaultValue.getText())) {
                Log.err("Incorrect data type for Default Value");
                return false;
            }
        }
        
        //
        // Check HelpText
        //
        if (isEmpty(this.jTextFieldHelpText.getText())) {
            Log.err("Help Text couldn't be empty");
            return false;
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
     Save all components of PCDs
     if exists pcds, set the value directly
     if not exists pcds, new an instance first
     
     **/
    public void save() {
        try {
            int count = this.vid.size();

            this.pcds = PcdCoded.Factory.newInstance();
            if (count > 0) {
                for (int index = 0; index < count; index++) {
                    PcdEntry p = PcdEntry.Factory.newInstance();
                    if (!isEmpty(vid.getPcdCoded(index).getName())) {
                        p.setCName(vid.getPcdCoded(index).getName());
                    }
                    if (!isEmpty(vid.getPcdCoded(index).getGuid())) {
                        p.setTokenSpaceGuidCName(vid.getPcdCoded(index).getGuid());
                    }
                    if (!isEmpty(vid.getPcdCoded(index).getFeatureFlag())) {
                        p.setFeatureFlag(vid.getPcdCoded(index).getFeatureFlag());
                    }
                    if (vid.getPcdCoded(index).getSupArchList() != null && vid.getPcdCoded(index).getSupArchList().size() > 0) {
                        p.setSupArchList(vid.getPcdCoded(index).getSupArchList());
                    }
                    if (!isEmpty(vid.getPcdCoded(index).getValue())) {
                        p.setDefaultValue(vid.getPcdCoded(index).getValue());
                    }
                    if (!isEmpty(vid.getPcdCoded(index).getHelp())) {
                        p.setHelpText(vid.getPcdCoded(index).getHelp());
                    }
                    if (!isEmpty(vid.getPcdCoded(index).getType())) {
                        p.setPcdItemType(PcdItemTypes.Enum.forString(vid.getPcdCoded(index).getType()));
                    }
                    this.pcds.addNewPcdEntry();
                    this.pcds.setPcdEntryArray(pcds.getPcdEntryList().size() - 1, p);
                }
            }

            this.msa.setPcdCoded(pcds);
            this.omt.setSaved(false);
        } catch (Exception e) {
            Log.err("Update Hobs", e.getMessage());
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

        resizeComponentWidth(jComboBoxCName, intCurrentWidth, intPreferredWidth);
        resizeComponentWidth(jTextFieldTokenSpaceGuid, intCurrentWidth, intPreferredWidth);
        resizeComponentWidth(jComboBoxItemType, intCurrentWidth, intPreferredWidth);
        resizeComponentWidth(jTextFieldDefaultValue, intCurrentWidth, intPreferredWidth);
        resizeComponentWidth(jTextFieldHelpText, intCurrentWidth, intPreferredWidth);
        resizeComponentWidth(jTextFieldFeatureFlag, intCurrentWidth, intPreferredWidth);
        resizeComponentWidth(jScrollPaneArch, intCurrentWidth, intPreferredWidth);

        resizeComponentWidth(jComboBoxList, intCurrentWidth, intPreferredWidth);
        resizeComponent(jScrollPaneList, intCurrentWidth, intCurrentHeight, intPreferredWidth, intPreferredHeight);

        relocateComponentX(jButtonAdd, intCurrentWidth, intPreferredWidth, DataType.SPACE_TO_RIGHT_FOR_ADD_BUTTON);
        relocateComponentX(jButtonRemove, intCurrentWidth, intPreferredWidth, DataType.SPACE_TO_RIGHT_FOR_REMOVE_BUTTON);
        relocateComponentX(jButtonUpdate, intCurrentWidth, intPreferredWidth, DataType.SPACE_TO_RIGHT_FOR_UPDATE_BUTTON);
    }
    
    private PcdCodedIdentification getCurrentPcdCoded() {
        String arg0 = this.jComboBoxCName.getSelectedItem().toString();
        String arg1 = this.jTextFieldTokenSpaceGuid.getText();
        

        String arg2 = this.jTextFieldFeatureFlag.getText();
        Vector<String> arg3 = this.iCheckBoxListArch.getAllCheckedItemsString();
        
        String arg4 = this.jTextFieldDefaultValue.getText();
        String arg5 = this.jTextFieldHelpText.getText();
        String arg6 = this.jComboBoxItemType.getSelectedItem().toString();
        id = new PcdCodedIdentification(arg0, arg1, arg2, arg3, arg4, arg5, arg6);
        return id;
    }
    
    /**
    Add current item to Vector
    
    **/
   private void addToList() {
       intSelectedItemId = vid.size();

       vid.addPcdCoded(getCurrentPcdCoded());

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

       vid.removePcdCoded(intTempIndex);

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

       vid.updatePcdCoded(getCurrentPcdCoded(), intTempIndex);

       jComboBoxList.removeAllItems();
       for (int index = 0; index < vid.size(); index++) {
           jComboBoxList.addItem(vid.getPcdCoded(index).getName());
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

           this.jComboBoxCName.setSelectedItem(vid.getPcdCoded(intSelectedItemId).getName());
           this.jTextFieldTokenSpaceGuid.setText(vid.getPcdCoded(intSelectedItemId).getGuid());
           
           this.jTextFieldDefaultValue.setText(vid.getPcdCoded(intSelectedItemId).getValue());
           this.jTextFieldHelpText.setText(vid.getPcdCoded(intSelectedItemId).getHelp());
           this.jComboBoxItemType.setSelectedItem(vid.getPcdCoded(intSelectedItemId).getType());

           jTextFieldFeatureFlag.setText(vid.getPcdCoded(intSelectedItemId).getFeatureFlag());
           iCheckBoxListArch.setAllItemsUnchecked();
           iCheckBoxListArch.initCheckedItem(true, vid.getPcdCoded(intSelectedItemId).getSupArchList());

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
           strListItem = strListItem + vid.getPcdCoded(index).getName() + DataType.UNIX_LINE_SEPARATOR;
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
