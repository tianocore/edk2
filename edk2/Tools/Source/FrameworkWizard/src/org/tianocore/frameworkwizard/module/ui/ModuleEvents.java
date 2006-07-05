/** @file
 
 The file is used to create, update Event of MSA/MBD file
 
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

import org.tianocore.EventsDocument;
import org.tianocore.ProtocolNotifyUsage;
import org.tianocore.ProtocolUsage;
import org.tianocore.EventsDocument.Events;
import org.tianocore.EventsDocument.Events.CreateEvents;
import org.tianocore.EventsDocument.Events.SignalEvents;
import org.tianocore.ModuleSurfaceAreaDocument.ModuleSurfaceArea;
import org.tianocore.frameworkwizard.common.DataType;
import org.tianocore.frameworkwizard.common.DataValidation;
import org.tianocore.frameworkwizard.common.EnumerationData;
import org.tianocore.frameworkwizard.common.Log;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.Identifications.OpeningModuleType;
import org.tianocore.frameworkwizard.common.ui.IInternalFrame;
import org.tianocore.frameworkwizard.common.ui.StarLabel;
import org.tianocore.frameworkwizard.common.ui.iCheckBoxList.ICheckBoxList;
import org.tianocore.frameworkwizard.module.Identifications.Events.EventsIdentification;
import org.tianocore.frameworkwizard.module.Identifications.Events.EventsVector;
import org.tianocore.frameworkwizard.workspace.WorkspaceTools;

/**
 The class is used to create, update Event of MSA/MBD file
 It extends IInternalFrame
 
 @since ModuleEditor 1.0

 **/
public class ModuleEvents extends IInternalFrame {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = -4396143706422842331L;

    //
    //Define class members
    //
    private JPanel jContentPane = null;

    private JLabel jLabelEventType = null;

    private JLabel jLabelC_Name = null;

    private JComboBox jComboBoxGuidC_Name = null;

    private JLabel jLabelUsage = null;

    private JComboBox jComboBoxUsage = null;

    private StarLabel jStarLabel1 = null;

    private StarLabel jStarLabel2 = null;

    private JComboBox jComboBoxEventsType = null;

    private JTextArea jTextAreaList = null;

    private JComboBox jComboBoxList = null;

    private JButton jButtonAdd = null;

    private JButton jButtonRemove = null;

    private JButton jButtonUpdate = null;

    private JScrollPane jScrollPane = null;

    private JScrollPane jScrollPaneList = null;

    private JLabel jLabelArch = null;

    private ICheckBoxList iCheckBoxListArch = null;

    private JScrollPane jScrollPaneArch = null;
    
    private JLabel jLabelHelpText = null;

    private JTextField jTextFieldHelpText = null;
    
    private JLabel jLabelFeatureFlag = null;

    private JTextField jTextFieldFeatureFlag = null;

    //
    // Not used by UI
    //
    private int intSelectedItemId = 0;

    private OpeningModuleType omt = null;

    private ModuleSurfaceArea msa = null;

    private EventsDocument.Events events = null;

    private EventsIdentification id = null;

    private EventsVector vid = new EventsVector();

    private EnumerationData ed = new EnumerationData();
    
    private WorkspaceTools wt = new WorkspaceTools();

    /**
     This method initializes jTextFieldC_Name 
     
     @return javax.swing.JTextField jTextFieldC_Name
     
     **/
    private JComboBox getJComboBoxGuidC_Name() {
        if (jComboBoxGuidC_Name == null) {
            jComboBoxGuidC_Name = new JComboBox();
            jComboBoxGuidC_Name.setBounds(new java.awt.Rectangle(160, 35, 320, 20));
            jComboBoxGuidC_Name.setPreferredSize(new java.awt.Dimension(320, 20));
            jComboBoxGuidC_Name.setToolTipText("Select the GUID C Name of the Event");
        }
        return jComboBoxGuidC_Name;
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
     * This method initializes jComboBoxEventsType	
     * 	
     * @return javax.swing.JComboBox	
     */
    private JComboBox getJComboBoxEventsType() {
        if (jComboBoxEventsType == null) {
            jComboBoxEventsType = new JComboBox();
            jComboBoxEventsType.setBounds(new java.awt.Rectangle(160, 10, 320, 20));
            jComboBoxEventsType.setPreferredSize(new java.awt.Dimension(320, 20));
            jComboBoxEventsType.setToolTipText("Select Create event if the Module has an event that is waiting to be signaled.  Select Signal if the Module will signal all events in an event group.  Signal Event The events are named by GUID.");
        }
        return jComboBoxEventsType;
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
     * This method initializes jTextFieldFeatureFlag	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldFeatureFlag() {
        if (jTextFieldFeatureFlag == null) {
            jTextFieldFeatureFlag = new JTextField();
            jTextFieldFeatureFlag.setBounds(new java.awt.Rectangle(160,110,320,20));
            jTextFieldFeatureFlag.setPreferredSize(new java.awt.Dimension(320,20));
        }
        return jTextFieldFeatureFlag;
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
       this.setTitle("Events");
       initFrame();
       this.setViewMode(false);
   }

   /**
    This method initializes this
    Fill values to all fields if these values are not empty
    
    @param inPackageDependencies

    **/
   private void init(Events inEvents) {
       init();
       this.events = inEvents;

       if (this.events != null) {
           if (this.events.getCreateEvents() != null) {
               if (this.events.getCreateEvents().getEventTypesList().size() > 0) {
                   for (int index = 0; index < this.events.getCreateEvents().getEventTypesList().size(); index++) {
                       String arg0 = events.getCreateEvents().getEventTypesList().get(index).getEventGuidCName();
                       String arg1 = ed.getVEventType().get(0);
                       String arg2 = null;
                       if (events.getCreateEvents().getEventTypesList().get(index).getUsage() != null) {
                           arg2 = events.getCreateEvents().getEventTypesList().get(index).getUsage().toString();    
                       }
                       
                       String arg3 = events.getCreateEvents().getEventTypesList().get(index).getFeatureFlag();
                       Vector<String> arg4 = Tools.convertListToVector(events.getCreateEvents().getEventTypesList().get(index)
                                                                                .getSupArchList());
                       String arg5 = events.getCreateEvents().getEventTypesList().get(index).getHelpText();
                       id = new EventsIdentification(arg0, arg1, arg2, arg3, arg4, arg5);
                       vid.addEvents(id);
                   }
               }
           }
           if (this.events.getSignalEvents() != null) {
               if (this.events.getSignalEvents().getEventTypesList().size() > 0) {
                   for (int index = 0; index < this.events.getSignalEvents().getEventTypesList().size(); index++) {
                       String arg0 = events.getSignalEvents().getEventTypesList().get(index).getEventGuidCName();
                       String arg1 = ed.getVEventType().get(1);
                       String arg2 = null;
                       if (events.getSignalEvents().getEventTypesList().get(index).getUsage() != null) {
                           arg2 = events.getSignalEvents().getEventTypesList().get(index).getUsage().toString();    
                       }

                       String arg3 = events.getSignalEvents().getEventTypesList().get(index).getFeatureFlag();
                       Vector<String> arg4 = Tools.convertListToVector(events.getSignalEvents().getEventTypesList().get(index)
                                                                                .getSupArchList());
                       String arg5 = events.getSignalEvents().getEventTypesList().get(index).getHelpText();
                       id = new EventsIdentification(arg0, arg1, arg2, arg3, arg4, arg5);
                       vid.addEvents(id);
                   }
               }
           }
       }
       //
       // Update the list
       //
       Tools.generateComboBoxByVector(jComboBoxList, vid.getEventsName());
       reloadListArea();
   }

    /**
     This is the default constructor
     
     **/
    public ModuleEvents() {
        super();
        init();
        this.setVisible(true);
    }

    /**
     This is the override edit constructor
     
     @param inEvents The input EventsDocument.Events
     
     **/
    public ModuleEvents(OpeningModuleType inOmt) {
        super();
        this.omt = inOmt;
        this.msa = omt.getXmlMsa();
        init(msa.getEvents());
        this.setVisible(true);
    }

    /**
     Disable all components when the mode is view
     
     @param isView true - The view mode; false - The non-view mode
     
     **/
    public void setViewMode(boolean isView) {
        if (isView) {
            this.jComboBoxGuidC_Name.setEnabled(!isView);
            this.jComboBoxUsage.setEnabled(!isView);
        }
    }

    /**
     This method initializes jContentPane
     
     @return javax.swing.JPanel jContentPane
     
     **/
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jLabelFeatureFlag = new JLabel();
            jLabelFeatureFlag.setBounds(new java.awt.Rectangle(15,110,140,20));
            jLabelFeatureFlag.setText("Feature Flag");
            jLabelArch = new JLabel();
            jLabelArch.setBounds(new java.awt.Rectangle(15, 135, 140, 20));
            jLabelArch.setText("Arch");
            jLabelUsage = new JLabel();
            jLabelUsage.setText("Usage");
            jLabelUsage.setBounds(new java.awt.Rectangle(15, 60, 140, 20));
            jLabelC_Name = new JLabel();
            jLabelC_Name.setText("Guid C_Name");
            jLabelC_Name.setBounds(new java.awt.Rectangle(15, 35, 140, 20));
            jLabelEventType = new JLabel();
            jLabelEventType.setText("Event Type");
            jLabelEventType.setBounds(new java.awt.Rectangle(15, 10, 140, 20));
            jLabelHelpText = new JLabel();
            jLabelHelpText.setBounds(new java.awt.Rectangle(14, 85, 140, 20));
            jLabelHelpText.setText("Help Text");

            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.setPreferredSize(new java.awt.Dimension(490, 490));

            jContentPane.add(jLabelEventType, null);
            jContentPane.add(jLabelC_Name, null);
            jContentPane.add(getJComboBoxGuidC_Name(), null);
            jContentPane.add(jLabelUsage, null);
            jContentPane.add(getJComboBoxUsage(), null);
            jStarLabel1 = new StarLabel();
            jStarLabel1.setBounds(new java.awt.Rectangle(0, 10, 10, 20));
            jStarLabel2 = new StarLabel();
            jStarLabel2.setBounds(new java.awt.Rectangle(0, 35, 10, 20));

            jContentPane.add(jStarLabel1, null);
            jContentPane.add(jStarLabel2, null);
            jContentPane.add(getJComboBoxEventsType(), null);

            jContentPane.add(getJComboBoxList(), null);
            jContentPane.add(getJButtonAdd(), null);
            jContentPane.add(getJButtonRemove(), null);
            jContentPane.add(getJButtonUpdate(), null);
            jContentPane.add(getJScrollPaneList(), null);
            jContentPane.add(jLabelArch, null);
            jContentPane.add(getJScrollPaneArch(), null);
            jContentPane.add(jLabelFeatureFlag, null);
            jContentPane.add(getJTextFieldFeatureFlag(), null);
            jContentPane.add(jLabelHelpText, null);
            jContentPane.add(getJTextFieldHelpText(), null);
        }
        return jContentPane;
    }

    /**
     This method initializes events groups and usage type
     
     **/
    private void initFrame() {
        Tools.generateComboBoxByVector(jComboBoxEventsType, ed.getVEventType());
        Tools.generateComboBoxByVector(jComboBoxUsage, ed.getVEventUsage());
        Tools.generateComboBoxByVector(jComboBoxGuidC_Name, wt.getAllGuidDeclarationsFromWorkspace());
        
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
        if (isEmpty(this.jComboBoxGuidC_Name.getSelectedItem().toString())) {
            Log.err("Event Name couldn't be empty");
            return false;
        }
        
        if (!isEmpty(this.jComboBoxGuidC_Name.getSelectedItem().toString())) {
            if (!DataValidation.isC_NameType(this.jComboBoxGuidC_Name.getSelectedItem().toString())) {
                Log.err("Incorrect data type for Event Name");
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
     Save all components of Events
     if exists events, set the value directly
     if not exists events, new an instance first
     
     **/
    public void save() {
        try {
            int count = this.vid.size();

            this.events = Events.Factory.newInstance();
            CreateEvents ce = CreateEvents.Factory.newInstance();
            SignalEvents se = SignalEvents.Factory.newInstance();
            
            if (count > 0) {
                for (int index = 0; index < count; index++) {
                    if (vid.getEvents(index).getType().equals(ed.getVEventType().get(0))) {
                        CreateEvents.EventTypes e = CreateEvents.EventTypes.Factory.newInstance();
                        if (!isEmpty(vid.getEvents(index).getName())) {
                            e.setEventGuidCName(vid.getEvents(index).getName());
                        }
                        if (!isEmpty(vid.getEvents(index).getUsage())) {
                            e.setUsage(ProtocolUsage.Enum.forString(vid.getEvents(index).getUsage()));
                        }
                        if (!isEmpty(vid.getEvents(index).getFeatureFlag())) {
                            e.setFeatureFlag(vid.getEvents(index).getFeatureFlag());
                        }
                        if (vid.getEvents(index).getSupArchList() != null
                            && vid.getEvents(index).getSupArchList().size() > 0) {
                            e.setSupArchList(vid.getEvents(index).getSupArchList());
                        }
                        if (!isEmpty(vid.getEvents(index).getHelp())) {
                            e.setHelpText(vid.getEvents(index).getHelp());
                        }
                        ce.addNewEventTypes();
                        ce.setEventTypesArray(ce.getEventTypesList().size() - 1, e);
                    }
                    if (vid.getEvents(index).getType().equals("Protocol Notify")) {
                        SignalEvents.EventTypes e = SignalEvents.EventTypes.Factory.newInstance();
                        if (!isEmpty(vid.getEvents(index).getName())) {
                            e.setEventGuidCName(vid.getEvents(index).getName());
                        }
                        if (!isEmpty(vid.getEvents(index).getUsage())) {
                            e.setUsage(ProtocolNotifyUsage.Enum.forString(vid.getEvents(index).getUsage()));
                        }
                        if (!isEmpty(vid.getEvents(index).getFeatureFlag())) {
                            e.setFeatureFlag(vid.getEvents(index).getFeatureFlag());
                        }
                        if (vid.getEvents(index).getSupArchList() != null
                            && vid.getEvents(index).getSupArchList().size() > 0) {
                            e.setSupArchList(vid.getEvents(index).getSupArchList());
                        }
                        if (!isEmpty(vid.getEvents(index).getHelp())) {
                            e.setHelpText(vid.getEvents(index).getHelp());
                        }
                        se.addNewEventTypes();
                        se.setEventTypesArray(ce.getEventTypesList().size() - 1, e);
                    }
                }
            }
            if (ce.getEventTypesList().size() > 0) {
                events.addNewCreateEvents();
                events.setCreateEvents(ce);
            }
            if (se.getEventTypesList().size() > 0) {
                events.addNewSignalEvents();
                events.setSignalEvents(se);
            }
            this.msa.setEvents(events);
            this.omt.setSaved(false);
        } catch (Exception e) {
            e.printStackTrace();
            Log.err("Update Events", e.getMessage());
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

        resizeComponentWidth(jComboBoxEventsType, intCurrentWidth, intPreferredWidth);
        resizeComponentWidth(jComboBoxGuidC_Name, intCurrentWidth, intPreferredWidth);
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
    
    private EventsIdentification getCurrentEvents() {
        String arg0 = this.jComboBoxGuidC_Name.getSelectedItem().toString();
        String arg1 = this.jComboBoxEventsType.getSelectedItem().toString();
        String arg2 = this.jComboBoxUsage.getSelectedItem().toString();

        String arg3 = this.jTextFieldFeatureFlag.getText();
        Vector<String> arg4 = this.iCheckBoxListArch.getAllCheckedItemsString();
        String arg5 = this.jTextFieldHelpText.getText();
        id = new EventsIdentification(arg0, arg1, arg2, arg3, arg4, arg5);
        return id;
    }

    /**
     Add current item to Vector
     
     **/
    private void addToList() {
        intSelectedItemId = vid.size();

        vid.addEvents(getCurrentEvents());

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

        vid.removeEvents(intTempIndex);

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

        vid.updateEvents(getCurrentEvents(), intTempIndex);

        jComboBoxList.removeAllItems();
        for (int index = 0; index < vid.size(); index++) {
            jComboBoxList.addItem(vid.getEvents(index).getName());
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

            this.jComboBoxGuidC_Name.setSelectedItem(vid.getEvents(intSelectedItemId).getName());
            this.jComboBoxEventsType.setSelectedItem(vid.getEvents(intSelectedItemId).getType());
            this.jComboBoxUsage.setSelectedItem(vid.getEvents(intSelectedItemId).getUsage());
            this.jTextFieldHelpText.setText(vid.getEvents(intSelectedItemId).getHelp());

            jTextFieldFeatureFlag.setText(vid.getEvents(intSelectedItemId).getFeatureFlag());
            iCheckBoxListArch.setAllItemsUnchecked();
            iCheckBoxListArch.initCheckedItem(true, vid.getEvents(intSelectedItemId).getSupArchList());

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
            strListItem = strListItem + vid.getEvents(index).getName() + DataType.UNIX_LINE_SEPARATOR;
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
