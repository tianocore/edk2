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
package org.tianocore.frameworkwizard.module.ui.dialog;

import java.awt.event.ActionEvent;
import java.util.Vector;

import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextField;
import javax.swing.JTextArea;

import org.tianocore.frameworkwizard.common.DataType;
import org.tianocore.frameworkwizard.common.DataValidation;
import org.tianocore.frameworkwizard.common.EnumerationData;
import org.tianocore.frameworkwizard.common.Log;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.ui.ArchCheckBox;
import org.tianocore.frameworkwizard.common.ui.IDialog;
import org.tianocore.frameworkwizard.common.ui.IFrame;
import org.tianocore.frameworkwizard.common.ui.StarLabel;
import org.tianocore.frameworkwizard.module.Identifications.Events.EventsIdentification;
import org.tianocore.frameworkwizard.workspace.WorkspaceTools;

/**
 * The class is used to create, update Event of MSA/MBD file It extends
 * IInternalFrame
 * 
 * @since ModuleEditor 1.0
 * 
 */
public class EventsDlg extends IDialog {

  // /
  // / Define class Serial Version UID
  // /
  private static final long serialVersionUID = -4396143706422842331L;

  //
  // Define class members
  //
  private JPanel jContentPane = null;

  private JLabel jLabelEventType = null;

  private JLabel jLabelC_Name = null;

  private JComboBox jComboBoxGuidC_Name = null;

  private JLabel jLabelUsage = null;

  private JLabel jLabelGroup = null;

  private JComboBox jComboBoxUsage = null;

  private JComboBox jComboBoxEventGroup = null;

  private StarLabel jStarLabel1 = null;

  private StarLabel jStarLabel2 = null;

  private StarLabel jStarLabel3 = null;

  private StarLabel jStarLabel4 = null;

  private JComboBox jComboBoxEventsType = null;

  private JScrollPane jScrollPane = null;

  private JLabel jLabelArch = null;

  private JLabel jLabelHelpText = null;

  private JTextArea jTextAreaHelpText = null;

  private JScrollPane jScrollPaneHelpText = null;

  private JLabel jLabelFeatureFlag = null;

  private JTextField jTextFieldFeatureFlag = null;

  private ArchCheckBox jArchCheckBox = null;

  private JButton jButtonOk = null;

  private JButton jButtonCancel = null;

  //
  // Not used by UI
  //
  private EventsIdentification id = null;

  private EnumerationData ed = new EnumerationData();

  private WorkspaceTools wt = new WorkspaceTools();

  /**
   * This method initializes jComboBoxType
   * 
   * @return javax.swing.JComboBox jComboBoxType
   * 
   */
  private JComboBox getJComboBoxEventsType() {
    if (jComboBoxEventsType == null) {
      jComboBoxEventsType = new JComboBox();
      jComboBoxEventsType.setBounds(new java.awt.Rectangle(160, 10, 320, 20));
      jComboBoxEventsType.setPreferredSize(new java.awt.Dimension(320, 20));
      jComboBoxEventsType
          .setToolTipText("<html>Select CreateEvents if the Module has an event that is waiting to be signaled.<br>Select SignalEvents if the Module will signal all events in an event group.<br>NOTE: Signal events are named by GUID.</html>");
    }
    return jComboBoxEventsType;
  }

  /**
   * This method initializes jTextFieldC_Name
   * 
   * @return javax.swing.JTextField jTextFieldC_Name
   * 
   */
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
   * This method initializes jComboBoxEventsType
   * 
   * @return javax.swing.JComboBox
   */
  private JComboBox getJComboBoxEventGroup() {
    if (jComboBoxEventGroup == null) {
      jComboBoxEventGroup = new JComboBox();
      jComboBoxEventGroup.setBounds(new java.awt.Rectangle(160, 60, 320, 20));
      jComboBoxEventGroup.setPreferredSize(new java.awt.Dimension(320, 20));
      jComboBoxEventGroup
          .setToolTipText("Select Type of Event: Guid or Timer.");

    }
    return jComboBoxEventGroup;
  }

  /**
   * This method initializes jComboBoxUsage
   * 
   * @return javax.swing.JComboBox jComboBoxUsage
   * 
   */
  private JComboBox getJComboBoxUsage() {
    if (jComboBoxUsage == null) {
      jComboBoxUsage = new JComboBox();
      jComboBoxUsage.setBounds(new java.awt.Rectangle(160, 85, 320, 20));
      jComboBoxUsage.setPreferredSize(new java.awt.Dimension(320, 20));
      jComboBoxUsage
          .setToolTipText("<html><table><tr><td colspan=2 align=center><b>Create Events</b></td></tr><tr><td>ALWAYS_CONSUMED</td><td>Module registers a notification function and REQUIRES that it be<br>executed for the module to fully function.</td></tr><tr><td>SOMETIMES_CONSUMED</td><td>Module registers a notification function and calls the function<br>when it is signaled</td></tr><tr><td colspan=2 align=center><b>Signal Events</b></td></tr><tr><td>ALWAYS_PRODUCED</td><td>Module will Always signal the event</td></tr><tr><td>SOMETIMES_PRODUCED</td><td>Module will sometimes signal the event</td></tr></table></html>");
    }
    return jComboBoxUsage;
  }

  /**
   * This method initializes jScrollPane
   * 
   * @return javax.swing.JScrollPane
   */
  private JScrollPane getJScrollPane() {
    if (jScrollPane == null) {
      jScrollPane = new JScrollPane();
      jScrollPane.setViewportView(getJContentPane());
    }
    return jScrollPane;
  }

  /**
   * This method initializes jTextFieldFeatureFlag
   * 
   * @return javax.swing.JTextField
   */
  private JTextField getJTextFieldFeatureFlag() {
    if (jTextFieldFeatureFlag == null) {
      jTextFieldFeatureFlag = new JTextField();
      jTextFieldFeatureFlag
          .setBounds(new java.awt.Rectangle(160, 155, 320, 20));
      jTextFieldFeatureFlag.setPreferredSize(new java.awt.Dimension(320, 20));
      jTextFieldFeatureFlag
          .setToolTipText("Postfix expression that must evaluate to TRUE or FALSE");
    }
    return jTextFieldFeatureFlag;
  }

  /**
   * This method initializes jTextAreaHelpText
   * 
   * @return javax.swing.JTextArea
   * 
   */
  private JTextArea getJTextAreaHelpText() {
    if (jTextAreaHelpText == null) {
      jTextAreaHelpText = new JTextArea();
      jTextAreaHelpText.setLineWrap(true);
      jTextAreaHelpText.setWrapStyleWord(true);
    }
    return jTextAreaHelpText;
  }

  /**
   * This method initializes jScrollPaneHelpText
   * 
   * @return javax.swing.JScrollPane
   * 
   */
  private JScrollPane getJScrollPaneHelpText() {
    if (jScrollPaneHelpText == null) {
      jScrollPaneHelpText = new JScrollPane();
      jScrollPaneHelpText.setHorizontalScrollBarPolicy(javax.swing.JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);
      jScrollPaneHelpText.setSize(new java.awt.Dimension(320,40));
      jScrollPaneHelpText.setPreferredSize(new java.awt.Dimension(320,40));
      jScrollPaneHelpText.setLocation(new java.awt.Point(160,110));
      jScrollPaneHelpText.setViewportView(getJTextAreaHelpText());
    }
    return jScrollPaneHelpText;
  }
  
  /**
   * This method initializes jButtonOk
   * 
   * @return javax.swing.JButton
   * 
   */
  private JButton getJButtonOk() {
    if (jButtonOk == null) {
      jButtonOk = new JButton();
      jButtonOk.setBounds(new java.awt.Rectangle(290, 202, 90, 20));
      jButtonOk.setText("Ok");
      jButtonOk.addActionListener(this);
    }
    return jButtonOk;
  }

  /**
   * This method initializes jButtonCancel
   * 
   * @return javax.swing.JButton
   * 
   */
  private JButton getJButtonCancel() {
    if (jButtonCancel == null) {
      jButtonCancel = new JButton();
      jButtonCancel.setBounds(new java.awt.Rectangle(390, 202, 90, 20));
      jButtonCancel.setText("Cancel");
      jButtonCancel.addActionListener(this);
    }
    return jButtonCancel;
  }

  public static void main(String[] args) {

  }

  /**
   * This method initializes this
   * 
   */
  private void init() {
    this.setSize(500, 275);
    this.setContentPane(getJScrollPane());
    this.setTitle("Events");
    initFrame();
    this.setViewMode(false);
    this.centerWindow();
  }

  /**
   * This method initializes this Fill values to all fields if these values are
   * not empty
   * 
   * @param inEventsId
   * 
   */
  private void init(EventsIdentification inEventsId) {
    init();
    this.id = inEventsId;

    if (this.id != null) {
      this.jComboBoxGuidC_Name.setSelectedItem(id.getName());
      this.jComboBoxEventsType.setSelectedItem(id.getType());
      this.jComboBoxUsage.setSelectedItem(id.getUsage());
      this.jTextAreaHelpText.setText(id.getHelp());

      jTextFieldFeatureFlag.setText(id.getFeatureFlag());
      this.jArchCheckBox.setSelectedItems(id.getSupArchList());
      this.jComboBoxEventGroup.setSelectedItem(id.getGroup());
    }
  }

  /**
   * This is the override edit constructor
   * 
   * @param inEventsIdentification
   * @param iFrame
   * 
   */
  public EventsDlg(EventsIdentification inEventsIdentification, IFrame iFrame) {
    super(iFrame, true);
    init(inEventsIdentification);
  }

  /**
   * Disable all components when the mode is view
   * 
   * @param isView
   *          true - The view mode; false - The non-view mode
   * 
   */
  public void setViewMode(boolean isView) {
    if (isView) {
      this.jComboBoxGuidC_Name.setEnabled(!isView);
      this.jComboBoxUsage.setEnabled(!isView);
    }
  }

  /**
   * This method initializes jContentPane
   * 
   * @return javax.swing.JPanel jContentPane
   * 
   */
  private JPanel getJContentPane() {
    if (jContentPane == null) {
      jStarLabel1 = new StarLabel();
      jStarLabel1.setLocation(new java.awt.Point(2, 10));
      jLabelEventType = new JLabel();
      jLabelEventType.setText("Type");
      jLabelEventType.setBounds(new java.awt.Rectangle(15, 10, 145, 20));

      jStarLabel2 = new StarLabel();
      jStarLabel2.setLocation(new java.awt.Point(2, 35));
      jLabelC_Name = new JLabel();
      jLabelC_Name.setText("Guid C Name");
      jLabelC_Name.setBounds(new java.awt.Rectangle(15, 35, 145, 20));

      jStarLabel3 = new StarLabel();
      jStarLabel3.setLocation(new java.awt.Point(2, 60));
      jLabelGroup = new JLabel();
      jLabelGroup.setText("Event Type");
      jLabelGroup.setBounds(new java.awt.Rectangle(15, 60, 145, 20));

      jStarLabel4 = new StarLabel();
      jStarLabel4.setLocation(new java.awt.Point(2, 85));
      jLabelUsage = new JLabel();
      jLabelUsage.setText("Usage");
      jLabelUsage.setBounds(new java.awt.Rectangle(15, 85, 140, 20));

      jLabelHelpText = new JLabel();
      jLabelHelpText.setBounds(new java.awt.Rectangle(15, 110, 145, 20));
      jLabelHelpText.setText("Help Text");

      jLabelFeatureFlag = new JLabel();
      jLabelFeatureFlag.setBounds(new java.awt.Rectangle(15, 155, 145, 20));
      jLabelFeatureFlag.setText("Feature Flag");

      jLabelArch = new JLabel();
      jLabelArch.setBounds(new java.awt.Rectangle(15, 180, 145, 20));
      jLabelArch.setText("Arch");
      jArchCheckBox = new ArchCheckBox();
      jArchCheckBox.setBounds(new java.awt.Rectangle(160, 180, 320, 20));
      jArchCheckBox.setPreferredSize(new java.awt.Dimension(320, 20));

      jContentPane = new JPanel();
      jContentPane.setLayout(null);
      jContentPane.setPreferredSize(new java.awt.Dimension(485, 230));

      jContentPane.add(jStarLabel1, null);
      jContentPane.add(jLabelEventType, null);
      jContentPane.add(getJComboBoxEventsType(), null);
      jContentPane.add(jStarLabel2, null);
      jContentPane.add(jLabelC_Name, null);
      jContentPane.add(getJComboBoxGuidC_Name(), null);
      jContentPane.add(jStarLabel3, null);
      jContentPane.add(jLabelGroup, null);
      jContentPane.add(getJComboBoxEventGroup(), null);
      jContentPane.add(jStarLabel4, null);
      jContentPane.add(jLabelUsage, null);
      jContentPane.add(getJComboBoxUsage(), null);
      jContentPane.add(jLabelHelpText, null);
      jContentPane.add(getJScrollPaneHelpText(), null);
      jContentPane.add(jLabelFeatureFlag, null);
      jContentPane.add(getJTextFieldFeatureFlag(), null);
      jContentPane.add(jLabelArch, null);
      jContentPane.add(jArchCheckBox, null);
      jContentPane.add(getJButtonOk(), null);
      jContentPane.add(getJButtonCancel(), null);

    }
    return jContentPane;
  }

  /**
   * This method initializes events groups and usage type
   * 
   */
  private void initFrame() {
    Tools.generateComboBoxByVector(jComboBoxEventsType, ed.getVEventType());
    Tools.generateComboBoxByVector(jComboBoxEventGroup, ed.getVEventGroup());
    Tools.generateComboBoxByVector(jComboBoxUsage, ed.getVEventUsage());
    Tools.generateComboBoxByVector(jComboBoxGuidC_Name, wt
        .getAllGuidDeclarationsFromWorkspace());
  }

  /*
   * (non-Javadoc)
   * 
   * @see java.awt.event.ActionListener#actionPerformed(java.awt.event.ActionEvent)
   * 
   * Override actionPerformed to listen all actions
   * 
   */
  public void actionPerformed(ActionEvent arg0) {
    if (arg0.getSource() == jButtonOk) {
      if (checkAdd()) {
        getCurrentEvents();
        this.returnType = DataType.RETURN_TYPE_OK;
        this.setVisible(false);
      }
    }

    if (arg0.getSource() == jButtonCancel) {
      this.returnType = DataType.RETURN_TYPE_CANCEL;
      this.setVisible(false);
    }
  }

  /**
   * Data validation for all fields
   * 
   * @retval true - All datas are valid
   * @retval false - At least one data is invalid
   * 
   */
  public boolean checkAdd() {
    //
    // Check if all fields have correct data types
    //

    //
    // Check Name
    //
    if (isEmpty(this.jComboBoxGuidC_Name.getSelectedItem().toString())) {
      Log.wrn("Update Events", "Event Name couldn't be empty");
      return false;
    }

    if (!isEmpty(this.jComboBoxGuidC_Name.getSelectedItem().toString())) {
      if (!DataValidation.isC_NameType(this.jComboBoxGuidC_Name
          .getSelectedItem().toString())) {
        Log.wrn("Update Events", "Incorrect data type for Event Name");
        return false;
      }
    }

    //
    // Check FeatureFlag
    //
    if (!isEmpty(this.jTextFieldFeatureFlag.getText())) {
      if (!DataValidation.isFeatureFlag(this.jTextFieldFeatureFlag.getText())) {
        Log.wrn("Update Events", "Incorrect data type for Feature Flag");
        return false;
      }
    }

    return true;
  }

  private EventsIdentification getCurrentEvents() {
    String arg0 = this.jComboBoxGuidC_Name.getSelectedItem().toString();
    String arg1 = this.jComboBoxEventsType.getSelectedItem().toString();
    String arg2 = this.jComboBoxUsage.getSelectedItem().toString();

    String arg3 = this.jTextFieldFeatureFlag.getText();
    Vector<String> arg4 = this.jArchCheckBox.getSelectedItemsVector();
    String arg5 = this.jTextAreaHelpText.getText();
    String arg6 = this.jComboBoxEventGroup.getSelectedItem().toString();
    id = new EventsIdentification(arg0, arg1, arg2, arg3, arg4, arg5, arg6);
    return id;
  }
  
  public EventsIdentification getId() {
    return id;
  }

  public void setId(EventsIdentification id) {
    this.id = id;
  }
}
