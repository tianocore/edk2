/** @file
 
 The file is used to create, update Protocol of MSA/MBD file
 
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

import org.tianocore.ProtocolNotifyUsage;
import org.tianocore.ProtocolUsage;
import org.tianocore.ProtocolsDocument;
import org.tianocore.ModuleSurfaceAreaDocument.ModuleSurfaceArea;
import org.tianocore.ProtocolsDocument.Protocols;
import org.tianocore.ProtocolsDocument.Protocols.Protocol;
import org.tianocore.ProtocolsDocument.Protocols.ProtocolNotify;
import org.tianocore.frameworkwizard.common.DataType;
import org.tianocore.frameworkwizard.common.DataValidation;
import org.tianocore.frameworkwizard.common.EnumerationData;
import org.tianocore.frameworkwizard.common.Log;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.Identifications.OpeningModuleType;
import org.tianocore.frameworkwizard.common.ui.IInternalFrame;
import org.tianocore.frameworkwizard.common.ui.StarLabel;
import org.tianocore.frameworkwizard.common.ui.iCheckBoxList.ICheckBoxList;
import org.tianocore.frameworkwizard.module.Identifications.Protocols.ProtocolsIdentification;
import org.tianocore.frameworkwizard.module.Identifications.Protocols.ProtocolsVector;
import org.tianocore.frameworkwizard.workspace.WorkspaceTools;

/**
 The class is used to create, update Protocol of MSA/MBD file
 It extends IInternalFrame
 


 **/
public class ModuleProtocols extends IInternalFrame {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = -9084913640747858848L;

    //
    //Define class members
    //
    private JPanel jContentPane = null;

    private JLabel jLabelC_Name = null;

    private JLabel jLabelFeatureFlag = null;

    private JTextField jTextFieldFeatureFlag = null;

    private JLabel jLabelUsage = null;

    private JComboBox jComboBoxUsage = null;

    private StarLabel jStarLabel1 = null;

    private StarLabel jStarLabel2 = null;

    private JLabel jLabelProtocolType = null;

    private JLabel jLabelArch = null;

    private JTextArea jTextAreaList = null;

    private JComboBox jComboBoxList = null;

    private JButton jButtonAdd = null;

    private JButton jButtonRemove = null;

    private JButton jButtonUpdate = null;

    private JScrollPane jScrollPane = null;

    private JScrollPane jScrollPaneList = null;

    private JComboBox jComboBoxProtocolType = null;

    private JComboBox jComboBoxCName = null;

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

    private ProtocolsDocument.Protocols protocols = null;

    private ProtocolsIdentification id = null;

    private ProtocolsVector vid = new ProtocolsVector();

    private WorkspaceTools wt = new WorkspaceTools();

    private EnumerationData ed = new EnumerationData();

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
     This method initializes jComboBoxUsage 
     
     @return javax.swing.JComboBox jComboBoxUsage
     
     **/
    private JComboBox getJComboBoxProtocolUsage() {
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
     * This method initializes jComboBoxProtocolType	
     * 	
     * @return javax.swing.JComboBox	
     */
    private JComboBox getJComboBoxProtocolType() {
        if (jComboBoxProtocolType == null) {
            jComboBoxProtocolType = new JComboBox();
            jComboBoxProtocolType.setBounds(new java.awt.Rectangle(160, 10, 320, 20));
            jComboBoxProtocolType.setPreferredSize(new java.awt.Dimension(320, 20));
            jComboBoxProtocolType.addItemListener(this);
            jComboBoxProtocolType.setToolTipText("Select Protocol Type");
        }
        return jComboBoxProtocolType;
    }

    /**
     * This method initializes jComboBoxCName	
     * 	
     * @return javax.swing.JComboBox	
     */
    private JComboBox getJComboBoxCName() {
        if (jComboBoxCName == null) {
            jComboBoxCName = new JComboBox();
            jComboBoxCName.setBounds(new java.awt.Rectangle(160, 35, 320, 20));
            jComboBoxCName.setPreferredSize(new java.awt.Dimension(320, 20));

        }
        return jComboBoxCName;
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
        this.setTitle("Protocols");
        initFrame();
        this.setViewMode(false);
    }

    /**
     This method initializes this
     Fill values to all fields if these values are not empty
     
     @param inPackageDependencies

     **/
    private void init(Protocols inProtocols) {
        init();
        this.protocols = inProtocols;

        if (this.protocols != null) {
            if (this.protocols.getProtocolList().size() > 0) {
                for (int index = 0; index < this.protocols.getProtocolList().size(); index++) {
                    String arg0 = protocols.getProtocolList().get(index).getProtocolCName();
                    String arg1 = ed.getVProtocolType().get(0);
                    String arg2 = null;
                    if (protocols.getProtocolList().get(index).getUsage() != null) {
                        arg2 = protocols.getProtocolList().get(index).getUsage().toString();    
                    }
                    
                    String arg3 = protocols.getProtocolList().get(index).getFeatureFlag();
                    Vector<String> arg4 = Tools.convertListToVector(protocols.getProtocolList().get(index)
                                                                             .getSupArchList());
                    String arg5 = protocols.getProtocolList().get(index).getHelpText();
                    id = new ProtocolsIdentification(arg0, arg1, arg2, arg3, arg4, arg5);
                    vid.addProtocols(id);
                }
            }
            if (this.protocols.getProtocolNotifyList().size() > 0) {
                for (int index = 0; index < this.protocols.getProtocolNotifyList().size(); index++) {
                    String arg0 = protocols.getProtocolNotifyList().get(index).getProtocolNotifyCName();
                    String arg1 = ed.getVProtocolType().get(1);
                    String arg2 = null;
                    if (protocols.getProtocolNotifyList().get(index).getUsage() != null) {
                        arg2 = protocols.getProtocolNotifyList().get(index).getUsage().toString();    
                    }
                    
                    String arg3 = protocols.getProtocolNotifyList().get(index).getFeatureFlag();
                    Vector<String> arg4 = Tools.convertListToVector(protocols.getProtocolNotifyList().get(index)
                                                                             .getSupArchList());
                    String arg5 = protocols.getProtocolNotifyList().get(index).getHelpText();
                    id = new ProtocolsIdentification(arg0, arg1, arg2, arg3, arg4, arg5);
                    vid.addProtocols(id);
                }
            }
        }
        //
        // Update the list
        //
        Tools.generateComboBoxByVector(jComboBoxList, vid.getProtocolsName());
        reloadListArea();
    }

    /**
     This is the default constructor
     
     **/
    public ModuleProtocols() {
        super();
        init();
        this.setVisible(true);
    }

    /**
     This is the override edit constructor
     
     @param inProtocol The input data of ProtocolsDocument.Protocols
     
     **/
    public ModuleProtocols(OpeningModuleType inOmt) {
        super();
        this.omt = inOmt;
        this.msa = omt.getXmlMsa();
        init(msa.getProtocols());
        this.setVisible(true);
    }

    /**
     Disable all components when the mode is view
     
     @param isView true - The view mode; false - The non-view mode
     
     **/
    public void setViewMode(boolean isView) {
        if (isView) {
            this.jComboBoxUsage.setEnabled(!isView);
            this.jTextFieldFeatureFlag.setEnabled(!isView);
        }
    }

    /**
     This method initializes jContentPane
     
     @return javax.swing.JPanel jContentPane
     
     **/
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jLabelHelpText = new JLabel();
            jLabelHelpText.setBounds(new java.awt.Rectangle(14, 85, 140, 20));
            jLabelHelpText.setText("Help Text");
            jLabelArch = new JLabel();
            jLabelArch.setBounds(new java.awt.Rectangle(15, 135, 140, 20));
            jLabelArch.setText("Arch Type");
            jLabelProtocolType = new JLabel();
            jLabelProtocolType.setBounds(new java.awt.Rectangle(15, 10, 140, 20));
            jLabelProtocolType.setText("Protocol Type");
            jLabelUsage = new JLabel();
            jLabelUsage.setText("Usage");
            jLabelUsage.setBounds(new java.awt.Rectangle(15, 60, 140, 20));
            jLabelFeatureFlag = new JLabel();
            jLabelFeatureFlag.setText("Feature Flag");
            jLabelFeatureFlag.setBounds(new java.awt.Rectangle(15, 110, 140, 20));
            jLabelC_Name = new JLabel();
            jLabelC_Name.setText("C_Name Type");
            jLabelC_Name.setBounds(new java.awt.Rectangle(15, 35, 140, 20));

            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.setPreferredSize(new java.awt.Dimension(490, 495));

            jContentPane.add(jLabelC_Name, null);
            jContentPane.add(jLabelFeatureFlag, null);
            jContentPane.add(getJTextFieldFeatureFlag(), null);
            jContentPane.add(jLabelUsage, null);
            jContentPane.add(getJComboBoxProtocolUsage(), null);
            jContentPane.add(jLabelProtocolType, null);

            jStarLabel1 = new StarLabel();
            jStarLabel1.setBounds(new java.awt.Rectangle(0, 10, 10, 20));
            jStarLabel2 = new StarLabel();
            jStarLabel2.setBounds(new java.awt.Rectangle(0, 35, 10, 20));

            jContentPane.add(jStarLabel1, null);
            jContentPane.add(jStarLabel2, null);
            jContentPane.add(jLabelArch, null);
            jContentPane.add(getJComboBoxList(), null);
            jContentPane.add(getJButtonAdd(), null);
            jContentPane.add(getJButtonRemove(), null);
            jContentPane.add(getJButtonUpdate(), null);
            jContentPane.add(getJScrollPaneList(), null);

            jContentPane.add(getJComboBoxProtocolType(), null);
            jContentPane.add(getJComboBoxCName(), null);
            jContentPane.add(getJScrollPaneArch(), null);
            jContentPane.add(jLabelHelpText, null);
            jContentPane.add(getJTextFieldHelpText(), null);
        }
        return jContentPane;
    }

    /**
     This method initializes Usage type
     
     **/
    private void initFrame() {
        Tools.generateComboBoxByVector(jComboBoxProtocolType, ed.getVProtocolType());
        Tools.generateComboBoxByVector(jComboBoxCName, wt.getAllProtocolDeclarationsFromWorkspace());
        Tools.generateComboBoxByVector(jComboBoxUsage, ed.getVProtocolUsage());

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
        // Check Name 
        //
        if (!isEmpty(this.jComboBoxCName.getSelectedItem().toString())) {
            if (!DataValidation.isC_NameType(this.jComboBoxCName.getSelectedItem().toString())) {
                Log.err("Incorrect data type for Protocol/ProtocolNotify Name");
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
     Save all components of Protocols
     if exists protocols, set the value directly
     if not exists protocols, new an instance first
     
     **/
    public void save() {
        try {
            int count = this.vid.size();

            this.protocols = Protocols.Factory.newInstance();
            if (count > 0) {
                for (int index = 0; index < count; index++) {
                    if (vid.getProtocols(index).getType().equals(ed.getVProtocolType().get(0))) {
                        Protocol p = Protocol.Factory.newInstance();
                        if (!isEmpty(vid.getProtocols(index).getName())) {
                            p.setProtocolCName(vid.getProtocols(index).getName());
                        }
                        if (!isEmpty(vid.getProtocols(index).getUsage())) {
                            p.setUsage(ProtocolUsage.Enum.forString(vid.getProtocols(index).getUsage()));
                        }
                        if (!isEmpty(vid.getProtocols(index).getFeatureFlag())) {
                            p.setFeatureFlag(vid.getProtocols(index).getFeatureFlag());
                        }
                        if (vid.getProtocols(index).getSupArchList() != null
                            && vid.getProtocols(index).getSupArchList().size() > 0) {
                            p.setSupArchList(vid.getProtocols(index).getSupArchList());
                        }
                        if (!isEmpty(vid.getProtocols(index).getHelp())) {
                            p.setHelpText(vid.getProtocols(index).getHelp());
                        }
                        this.protocols.addNewProtocol();
                        this.protocols.setProtocolArray(protocols.getProtocolList().size() - 1, p);
                    }
                    if (vid.getProtocols(index).getType().equals(ed.getVProtocolType().get(1))) {
                        ProtocolNotify p = ProtocolNotify.Factory.newInstance();
                        if (!isEmpty(vid.getProtocols(index).getName())) {
                            p.setProtocolNotifyCName(vid.getProtocols(index).getName());
                        }
                        if (!isEmpty(vid.getProtocols(index).getUsage())) {
                            p.setUsage(ProtocolNotifyUsage.Enum.forString(vid.getProtocols(index).getUsage()));
                        }
                        if (!isEmpty(vid.getProtocols(index).getFeatureFlag())) {
                            p.setFeatureFlag(vid.getProtocols(index).getFeatureFlag());
                        }
                        if (vid.getProtocols(index).getSupArchList() != null
                            && vid.getProtocols(index).getSupArchList().size() > 0) {
                            p.setSupArchList(vid.getProtocols(index).getSupArchList());
                        }
                        if (!isEmpty(vid.getProtocols(index).getHelp())) {
                            p.setHelpText(vid.getProtocols(index).getHelp());
                        }
                        this.protocols.addNewProtocolNotify();
                        this.protocols.setProtocolNotifyArray(protocols.getProtocolNotifyList().size() - 1, p);
                    }
                }
            }

            this.msa.setProtocols(protocols);
            this.omt.setSaved(false);
        } catch (Exception e) {
            e.printStackTrace();
            Log.err("Update Protocols", e.getMessage());
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

        resizeComponentWidth(this.jComboBoxProtocolType, intCurrentWidth, intPreferredWidth);
        resizeComponentWidth(this.jComboBoxCName, intCurrentWidth, intPreferredWidth);
        resizeComponentWidth(this.jComboBoxUsage, intCurrentWidth, intPreferredWidth);
        resizeComponentWidth(this.jTextFieldHelpText, intCurrentWidth, intPreferredWidth);
        resizeComponentWidth(this.jTextFieldFeatureFlag, intCurrentWidth, intPreferredWidth);
        resizeComponentWidth(this.jScrollPaneArch, intCurrentWidth, intPreferredWidth);

        resizeComponentWidth(this.jComboBoxList, intCurrentWidth, intPreferredWidth);
        resizeComponent(this.jScrollPaneList, intCurrentWidth, intCurrentHeight, intPreferredWidth, intPreferredHeight);

        relocateComponentX(this.jButtonAdd, intCurrentWidth, intPreferredWidth, DataType.SPACE_TO_RIGHT_FOR_ADD_BUTTON);
        relocateComponentX(this.jButtonRemove, intCurrentWidth, intPreferredWidth,
                           DataType.SPACE_TO_RIGHT_FOR_REMOVE_BUTTON);
        relocateComponentX(this.jButtonUpdate, intCurrentWidth, intPreferredWidth,
                           DataType.SPACE_TO_RIGHT_FOR_UPDATE_BUTTON);
    }

    private ProtocolsIdentification getCurrentProtocols() {
        String arg0 = this.jComboBoxCName.getSelectedItem().toString();
        String arg1 = this.jComboBoxProtocolType.getSelectedItem().toString();
        String arg2 = this.jComboBoxUsage.getSelectedItem().toString();

        String arg3 = this.jTextFieldFeatureFlag.getText();
        Vector<String> arg4 = this.iCheckBoxListArch.getAllCheckedItemsString();
        String arg5 = this.jTextFieldHelpText.getText();
        id = new ProtocolsIdentification(arg0, arg1, arg2, arg3, arg4, arg5);
        return id;
    }

    /**
     Add current item to Vector
     
     **/
    private void addToList() {
        intSelectedItemId = vid.size();

        vid.addProtocols(getCurrentProtocols());

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

        vid.removeProtocols(intTempIndex);

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

        vid.updateProtocols(getCurrentProtocols(), intTempIndex);

        jComboBoxList.removeAllItems();
        for (int index = 0; index < vid.size(); index++) {
            jComboBoxList.addItem(vid.getProtocols(index).getName());
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

            this.jComboBoxCName.setSelectedItem(vid.getProtocols(intSelectedItemId).getName());
            this.jComboBoxProtocolType.setSelectedItem(vid.getProtocols(intSelectedItemId).getType());
            this.jComboBoxUsage.setSelectedItem(vid.getProtocols(intSelectedItemId).getUsage());
            this.jTextFieldHelpText.setText(vid.getProtocols(intSelectedItemId).getHelp());
            
            jTextFieldFeatureFlag.setText(vid.getProtocols(intSelectedItemId).getFeatureFlag());
            iCheckBoxListArch.setAllItemsUnchecked();
            iCheckBoxListArch.initCheckedItem(true, vid.getProtocols(intSelectedItemId).getSupArchList());
            

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
            strListItem = strListItem + vid.getProtocols(index).getName() + DataType.UNIX_LINE_SEPARATOR;
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
        if (arg0.getSource() == this.jComboBoxProtocolType && arg0.getStateChange() == ItemEvent.SELECTED) {
            if (this.jComboBoxProtocolType.getSelectedItem().toString().equals(ed.getVProtocolType().get(0))) {
                Tools.generateComboBoxByVector(this.jComboBoxUsage, ed.getVProtocolUsage());
            } else {
                Tools.generateComboBoxByVector(this.jComboBoxUsage, ed.getVProtocolNotifyUsage());
            }
        }
    }
}
