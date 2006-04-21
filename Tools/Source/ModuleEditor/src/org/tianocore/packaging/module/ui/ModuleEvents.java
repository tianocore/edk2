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
package org.tianocore.packaging.module.ui;

import java.awt.Dimension;
import java.awt.event.ActionEvent;

import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JRadioButton;
import javax.swing.JTextField;

import org.tianocore.EventTypes;
import org.tianocore.EventUsage;
import org.tianocore.EventsDocument;
import org.tianocore.GuidDocument;
import org.tianocore.common.DataValidation;
import org.tianocore.common.Log;
import org.tianocore.common.Tools;
import org.tianocore.packaging.common.ui.IDefaultMutableTreeNode;
import org.tianocore.packaging.common.ui.IInternalFrame;
import org.tianocore.packaging.common.ui.StarLabel;

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
    private EventsDocument.Events events = null;

    private EventsDocument.Events.CreateEvents createEvent = null;

    private EventsDocument.Events.SignalEvents signalEvent = null;

    private int location = -1;

    private JPanel jContentPane = null;

    private JLabel jLabelEventType = null;

    private JRadioButton jRadioButtonEventCreate = null;

    private JRadioButton jRadioButtonEventSignal = null;

    private JLabel jLabelC_Name = null;

    private JTextField jTextFieldC_Name = null;

    private JLabel jLabelGuid = null;

    private JTextField jTextFieldGuid = null;

    private JLabel jLabelEventGroup = null;

    private JComboBox jComboBoxEventGroup = null;

    private JLabel jLabelUsage = null;

    private JComboBox jComboBoxUsage = null;

    private JButton jButtonOk = null;

    private JButton jButtonCancel = null;

    private JButton jButtonGenerateGuid = null;

    private JLabel jLabelOverrideID = null;

    private JTextField jTextFieldOverrideID = null;

    private StarLabel jStarLabel1 = null;

    private StarLabel jStarLabel2 = null;

    /**
     This method initializes jRadioButtonEnentType 
     
     @return javax.swing.JRadioButton jRadioButtonEventCreate
     
     **/
    private JRadioButton getJRadioButtonEventCreate() {
        if (jRadioButtonEventCreate == null) {
            jRadioButtonEventCreate = new JRadioButton();
            jRadioButtonEventCreate.setText("Create");
            jRadioButtonEventCreate.setBounds(new java.awt.Rectangle(160, 10, 90, 20));
            jRadioButtonEventCreate.addActionListener(this);
            jRadioButtonEventCreate.setSelected(true);
        }
        return jRadioButtonEventCreate;
    }

    /**
     This method initializes jRadioButtonEventSignal 
     
     @return javax.swing.JRadioButton jRadioButtonEventSignal
     
     **/
    private JRadioButton getJRadioButtonEventSignal() {
        if (jRadioButtonEventSignal == null) {
            jRadioButtonEventSignal = new JRadioButton();
            jRadioButtonEventSignal.setText("Signal");
            jRadioButtonEventSignal.setBounds(new java.awt.Rectangle(320, 10, 90, 20));
            jRadioButtonEventSignal.addActionListener(this);
        }
        return jRadioButtonEventSignal;
    }

    /**
     This method initializes jTextFieldC_Name 
     
     @return javax.swing.JTextField jTextFieldC_Name
     
     **/
    private JTextField getJTextFieldC_Name() {
        if (jTextFieldC_Name == null) {
            jTextFieldC_Name = new JTextField();
            jTextFieldC_Name.setBounds(new java.awt.Rectangle(160, 35, 320, 20));
        }
        return jTextFieldC_Name;
    }

    /**
     This method initializes jTextFieldGuid 
     
     @return javax.swing.JTextField jTextFieldGuid
     
     **/
    private JTextField getJTextFieldGuid() {
        if (jTextFieldGuid == null) {
            jTextFieldGuid = new JTextField();
            jTextFieldGuid.setBounds(new java.awt.Rectangle(160, 60, 240, 20));
        }
        return jTextFieldGuid;
    }

    /**
     This method initializes jComboBoxEventGroup 
     
     @return javax.swing.JComboBox jComboBoxEventGroup
     
     **/
    private JComboBox getJComboBoxEventGroup() {
        if (jComboBoxEventGroup == null) {
            jComboBoxEventGroup = new JComboBox();
            jComboBoxEventGroup.setBounds(new java.awt.Rectangle(160, 85, 320, 20));
        }
        return jComboBoxEventGroup;
    }

    /**
     This method initializes jComboBoxUsage 
     
     @return javax.swing.JComboBox jComboBoxUsage
     
     **/
    private JComboBox getJComboBoxUsage() {
        if (jComboBoxUsage == null) {
            jComboBoxUsage = new JComboBox();
            jComboBoxUsage.setBounds(new java.awt.Rectangle(160, 110, 320, 20));
        }
        return jComboBoxUsage;
    }

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
    private JButton getJButtonCancel() {
        if (jButtonCancel == null) {
            jButtonCancel = new JButton();
            jButtonCancel.setText("Cancel");
            jButtonCancel.setBounds(new java.awt.Rectangle(390, 165, 90, 20));
            jButtonCancel.setPreferredSize(new Dimension(90, 20));
            jButtonCancel.addActionListener(this);
        }
        return jButtonCancel;
    }

    /**
     This method initializes jButtonGenerateGuid 
     
     @return javax.swing.JButton jButtonGenerateGuid
     
     **/
    private JButton getJButtonGenerateGuid() {
        if (jButtonGenerateGuid == null) {
            jButtonGenerateGuid = new JButton();
            jButtonGenerateGuid.setBounds(new java.awt.Rectangle(405, 60, 75, 20));
            jButtonGenerateGuid.setText("GEN");
            jButtonGenerateGuid.addActionListener(this);
        }
        return jButtonGenerateGuid;
    }

    /**
     This method initializes jTextFieldOverrideID 
     
     @return javax.swing.JTextField jTextFieldOverrideID
     
     **/
    private JTextField getJTextFieldOverrideID() {
        if (jTextFieldOverrideID == null) {
            jTextFieldOverrideID = new JTextField();
            jTextFieldOverrideID.setBounds(new java.awt.Rectangle(160, 135, 320, 20));
        }
        return jTextFieldOverrideID;
    }

    public static void main(String[] args) {

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
    public ModuleEvents(EventsDocument.Events inEvents) {
        super();
        init(inEvents);
        this.setVisible(true);
    }

    /**
     This is the override edit constructor
     
     @param inEvents The input EventsDocument.Events
     @param type The input data of node type
     @param index The input data of node index
     
     **/
    public ModuleEvents(EventsDocument.Events inEvents, int type, int index) {
        super();
        init(inEvents, type, index);
        this.setVisible(true);
    }

    /**
     This method initializes this
     
     @param inEvents The input EventsDocument.Events
     
     **/
    private void init(EventsDocument.Events inEvents) {
        init();
        this.setEvents(inEvents);
    }

    /**
     This method initializes this
     Fill values to all fields if these values are not empty
     
     @param inEvents EventsDocument.Events
     @param type The input data of node type
     @param index The input data of node index
     
     **/
    private void init(EventsDocument.Events inEvents, int type, int index) {
        init(inEvents);
        this.location = index;
        if (type == IDefaultMutableTreeNode.EVENTS_CREATEEVENTS_ITEM) {
            this.jRadioButtonEventCreate.setSelected(true);
            this.jRadioButtonEventSignal.setSelected(false);
            if (this.events.getCreateEvents().getEventArray(index).getCName() != null) {
                this.jTextFieldC_Name.setText(this.events.getCreateEvents().getEventArray(index).getCName());
            }
            if (this.events.getCreateEvents().getEventArray(index).getGuid() != null) {
                this.jTextFieldGuid.setText(this.events.getCreateEvents().getEventArray(index).getGuid()
                                                       .getStringValue());
            }
            if (this.events.getCreateEvents().getEventArray(index).getEventGroup() != null) {
                this.jComboBoxEventGroup.setSelectedItem(this.events.getCreateEvents().getEventArray(index)
                                                                    .getEventGroup().toString());
            }
            if (this.events.getCreateEvents().getEventArray(index).getUsage() != null) {
                this.jComboBoxUsage.setSelectedItem(this.events.getCreateEvents().getEventArray(index).getUsage()
                                                               .toString());
            }
            this.jTextFieldOverrideID.setText(String.valueOf(this.events.getCreateEvents().getEventArray(index)
                                                                        .getOverrideID()));
        } else if (type == IDefaultMutableTreeNode.EVENTS_SIGNALEVENTS_ITEM) {
            this.jRadioButtonEventCreate.setSelected(false);
            this.jRadioButtonEventSignal.setSelected(true);
            this.jComboBoxUsage.setEnabled(false);
            if (this.events.getSignalEvents().getEventArray(index).getCName() != null) {
                this.jTextFieldC_Name.setText(this.events.getSignalEvents().getEventArray(index).getCName());
            }
            if (this.events.getSignalEvents().getEventArray(index).getGuid() != null) {
                this.jTextFieldGuid.setText(this.events.getSignalEvents().getEventArray(index).getGuid().toString());
            }
            if (this.events.getSignalEvents().getEventArray(index).getEventGroup() != null) {
                this.jComboBoxEventGroup.setSelectedItem(this.events.getSignalEvents().getEventArray(index)
                                                                    .getEventGroup().toString());
            }
            if (this.events.getSignalEvents().getEventArray(index).getUsage() != null) {
                this.jComboBoxUsage.setSelectedItem(this.events.getSignalEvents().getEventArray(index).getUsage()
                                                               .toString());
            }
            this.jTextFieldOverrideID.setText(String.valueOf(this.events.getSignalEvents().getEventArray(index)
                                                                        .getOverrideID()));
        }
        this.jRadioButtonEventCreate.setEnabled(false);
        this.jRadioButtonEventSignal.setEnabled(false);
    }

    /**
     This method initializes this
     
     **/
    private void init() {
        this.setSize(500, 515);
        this.setContentPane(getJContentPane());
        this.setTitle("Events");
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
            this.jRadioButtonEventCreate.setEnabled(!isView);
            this.jRadioButtonEventSignal.setEnabled(!isView);
            this.jTextFieldC_Name.setEnabled(!isView);
            this.jTextFieldGuid.setEnabled(!isView);
            this.jComboBoxEventGroup.setEnabled(!isView);
            this.jComboBoxUsage.setEnabled(!isView);
            this.jTextFieldOverrideID.setEnabled(!isView);
            this.jButtonCancel.setEnabled(!isView);
            this.jButtonGenerateGuid.setEnabled(!isView);
            this.jButtonOk.setEnabled(!isView);
        }
    }

    /**
     This method initializes jContentPane
     
     @return javax.swing.JPanel jContentPane
     
     **/
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jLabelOverrideID = new JLabel();
            jLabelOverrideID.setBounds(new java.awt.Rectangle(15, 135, 140, 20));
            jLabelOverrideID.setText("Override ID");
            jLabelUsage = new JLabel();
            jLabelUsage.setText("Usage");
            jLabelUsage.setBounds(new java.awt.Rectangle(15, 110, 140, 20));
            jLabelEventGroup = new JLabel();
            jLabelEventGroup.setText("Event Group");
            jLabelEventGroup.setBounds(new java.awt.Rectangle(15, 85, 140, 20));
            jLabelGuid = new JLabel();
            jLabelGuid.setText("Guid");
            jLabelGuid.setBounds(new java.awt.Rectangle(15, 60, 140, 20));
            jLabelC_Name = new JLabel();
            jLabelC_Name.setText("C_Name");
            jLabelC_Name.setBounds(new java.awt.Rectangle(15, 35, 140, 20));
            jLabelEventType = new JLabel();
            jLabelEventType.setText("Event Type");
            jLabelEventType.setBounds(new java.awt.Rectangle(15, 10, 140, 20));
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.add(jLabelEventType, null);
            jContentPane.add(getJRadioButtonEventCreate(), null);
            jContentPane.add(getJRadioButtonEventSignal(), null);
            jContentPane.add(jLabelC_Name, null);
            jContentPane.add(getJTextFieldC_Name(), null);
            jContentPane.add(jLabelGuid, null);
            jContentPane.add(getJTextFieldGuid(), null);
            jContentPane.add(jLabelEventGroup, null);
            jContentPane.add(getJComboBoxEventGroup(), null);
            jContentPane.add(jLabelUsage, null);
            jContentPane.add(getJComboBoxUsage(), null);
            jContentPane.add(getJButton(), null);
            jContentPane.add(getJButtonCancel(), null);
            jContentPane.add(getJButtonGenerateGuid(), null);
            jContentPane.add(jLabelOverrideID, null);
            jContentPane.add(getJTextFieldOverrideID(), null);

            jStarLabel1 = new StarLabel();
            jStarLabel1.setBounds(new java.awt.Rectangle(0, 10, 10, 20));
            jStarLabel2 = new StarLabel();
            jStarLabel2.setBounds(new java.awt.Rectangle(0, 35, 10, 20));

            jContentPane.add(jStarLabel1, null);
            jContentPane.add(jStarLabel2, null);
        }
        return jContentPane;
    }

    /**
     This method initializes events groups and usage type
     
     **/
    private void initFrame() {
        jComboBoxEventGroup.addItem("EVENT_GROUP_EXIT_BOOT_SERVICES");
        jComboBoxEventGroup.addItem("EVENT_GROUP_VIRTUAL_ADDRESS_CHANGE");
        jComboBoxEventGroup.addItem("EVENT_GROUP_MEMORY_MAP_CHANGE");
        jComboBoxEventGroup.addItem("EVENT_GROUP_READY_TO_BOOT");
        jComboBoxEventGroup.addItem("EVENT_GROUP_LEGACY_BOOT");

        jComboBoxUsage.addItem("ALWAYS_CONSUMED");
        jComboBoxUsage.addItem("SOMETIMES_CONSUMED");
        jComboBoxUsage.addItem("ALWAYS_PRODUCED");
        jComboBoxUsage.addItem("SOMETIMES_PRODUCED");
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
            this.setEdited(true);
            this.save();
            this.dispose();
        }
        if (arg0.getSource() == jButtonCancel) {
            this.dispose();
        }

        if (arg0.getSource() == jRadioButtonEventCreate) {
            if (jRadioButtonEventCreate.isSelected()) {
                jRadioButtonEventSignal.setSelected(false);
            }
            if (!jRadioButtonEventSignal.isSelected() && !jRadioButtonEventCreate.isSelected()) {
                jRadioButtonEventCreate.setSelected(true);
            }
        }

        if (arg0.getSource() == jRadioButtonEventSignal) {
            if (jRadioButtonEventSignal.isSelected()) {
                jRadioButtonEventCreate.setSelected(false);
            }
            if (!jRadioButtonEventSignal.isSelected() && !jRadioButtonEventCreate.isSelected()) {
                jRadioButtonEventSignal.setSelected(true);
            }
        }

        if (arg0.getSource() == jButtonGenerateGuid) {
            jTextFieldGuid.setText(Tools.generateUuidString());
        }
    }

    public EventsDocument.Events getEvents() {
        return events;
    }

    public void setEvents(EventsDocument.Events events) {
        this.events = events;
    }

    /**
     Data validation for all fields
     
     @retval true - All datas are valid
     @retval false - At least one data is invalid
     
     **/
    public boolean check() {
        //
        // Check if all required fields are not empty
        //
        if (isEmpty(this.jTextFieldC_Name.getText())) {
            Log.err("C_Name couldn't be empty");
            return false;
        }

        //
        // Check if all fields have correct data types 
        //
        if (!DataValidation.isCName(this.jTextFieldC_Name.getText())) {
            Log.err("Incorrect data type for C_Name");
            return false;
        }
        if (!isEmpty(this.jTextFieldGuid.getText()) && !DataValidation.isGuid(this.jTextFieldGuid.getText())) {
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
     Save all components of Events
     if exists events, set the value directly
     if not exists events, new an instance first
     
     **/
    public void save() {
        try {
            if (this.events == null) {
                events = EventsDocument.Events.Factory.newInstance();
                createEvent = EventsDocument.Events.CreateEvents.Factory.newInstance();
                signalEvent = EventsDocument.Events.SignalEvents.Factory.newInstance();
            } else {
                if (events.getCreateEvents() != null) {
                    createEvent = events.getCreateEvents();
                } else {
                    createEvent = EventsDocument.Events.CreateEvents.Factory.newInstance();
                }
                if (events.getSignalEvents() != null) {
                    signalEvent = events.getSignalEvents();
                } else {
                    signalEvent = EventsDocument.Events.SignalEvents.Factory.newInstance();
                }

            }
            if (this.jRadioButtonEventCreate.isSelected()) {
                EventsDocument.Events.CreateEvents.Event event = EventsDocument.Events.CreateEvents.Event.Factory
                                                                                                                 .newInstance();
                event.setCName(this.jTextFieldC_Name.getText());
                if (!isEmpty(this.jTextFieldGuid.getText())) {
                    GuidDocument.Guid guid = GuidDocument.Guid.Factory.newInstance();
                    guid.setStringValue(this.jTextFieldGuid.getText());
                    event.setGuid(guid);
                }
                event.setEventGroup(EventTypes.Enum.forString(jComboBoxEventGroup.getSelectedItem().toString()));
                event.setUsage(EventUsage.Enum.forString(jComboBoxUsage.getSelectedItem().toString()));
                if (!isEmpty(this.jTextFieldOverrideID.getText())) {
                    event.setOverrideID(Integer.parseInt(this.jTextFieldOverrideID.getText()));
                }
                if (location > -1) {
                    createEvent.setEventArray(location, event);
                } else {
                    createEvent.addNewEvent();
                    createEvent.setEventArray(createEvent.getEventList().size() - 1, event);
                }
                events.setCreateEvents(createEvent);
            }
            if (this.jRadioButtonEventSignal.isSelected()) {
                EventsDocument.Events.SignalEvents.Event event = EventsDocument.Events.SignalEvents.Event.Factory
                                                                                                                 .newInstance();
                event.setCName(this.jTextFieldC_Name.getText());
                if (!isEmpty(this.jTextFieldGuid.getText())) {
                    GuidDocument.Guid guid = GuidDocument.Guid.Factory.newInstance();
                    guid.setStringValue(this.jTextFieldGuid.getText());
                    event.setGuid(guid);
                }
                event.setEventGroup(EventTypes.Enum.forString(jComboBoxEventGroup.getSelectedItem().toString()));
                event.setUsage(EventUsage.Enum.forString(jComboBoxUsage.getSelectedItem().toString()));
                if (!isEmpty(this.jTextFieldOverrideID.getText())) {
                    event.setOverrideID(Integer.parseInt(this.jTextFieldOverrideID.getText()));
                }
                if (location > -1) {
                    signalEvent.setEventArray(location, event);
                } else {
                    signalEvent.addNewEvent();
                    signalEvent.setEventArray(signalEvent.getEventList().size() - 1, event);
                }
                events.setSignalEvents(signalEvent);
            }
        } catch (Exception e) {
            Log.err("Update Events", e.getMessage());
        }
    }
}
