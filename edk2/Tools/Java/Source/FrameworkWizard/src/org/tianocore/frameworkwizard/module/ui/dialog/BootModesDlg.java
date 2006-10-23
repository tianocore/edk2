/** @file
 <<The file is used to create, update BootModes of MSA file>>

 <<The BootModesDlg is called to add or edit a Module's Boot Modes definitions.>>
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 Package Name: Tools
 Module Name:  FrameworkWizard
 
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
import org.tianocore.frameworkwizard.module.Identifications.ModuleIdentification;
import org.tianocore.frameworkwizard.module.Identifications.BootModes.BootModesIdentification;
import org.tianocore.frameworkwizard.workspace.WorkspaceTools;

/**
 * The class is used to create, update BootModes of MSA/MBD file
 *  
 * It extends IDialog
 * 
 */
public class BootModesDlg extends IDialog {

    // /
    // / Define class Serial Version UID
    // /
    private static final long serialVersionUID = -3888558623432442561L;

    //
    // Define class members
    //
    private JPanel jContentPane = null;

    private JLabel jLabelBootModeName = null;

    private JComboBox jComboBoxBootModeName = null;

    private JLabel jLabelUsage = null;

    private JComboBox jComboBoxUsage = null;

    private StarLabel jStarLabel1 = null;

    private StarLabel jStarLabel2 = null;

    private JLabel jLabelFeatureFlag = null;

    private JTextField jTextFieldFeatureFlag = null;

    private JLabel jLabelArch = null;

    private JScrollPane jScrollPane = null;

    private JLabel jLabelHelpText = null;

    private JTextArea jTextAreaHelpText = null;

    private JScrollPane jScrollPaneHelpText = null;

    private ArchCheckBox jArchCheckBox = null;

    private JButton jButtonOk = null;

    private JButton jButtonCancel = null;

    //
    // Not used by UI
    //
    private BootModesIdentification id = null;

    private EnumerationData ed = new EnumerationData();
    
    private WorkspaceTools wt = new WorkspaceTools();

    /**
     * This method initializes jComboBoxBootModeName
     * 
     * @return javax.swing.JComboBox jComboBoxBootModeName
     * 
     */
    private JComboBox getJComboBoxBootModeName() {
        if (jComboBoxBootModeName == null) {
            jComboBoxBootModeName = new JComboBox();
            jComboBoxBootModeName.setBounds(new java.awt.Rectangle(168, 12, 320, 20));
            jComboBoxBootModeName.setPreferredSize(new java.awt.Dimension(320, 20));
            jComboBoxBootModeName
                                 .setToolTipText("<html><table>"
                                                 + "<tr><td>FULL</td><td>Boot with full configuration</td></tr>"
                                                 + "<tr><td>MINIMAL</td><td>Boot with minimal configuration</td></tr>"
                                                 + "<tr><td>NO_CHANGE</td><td>Boot assuming no configuration changes</td></tr>"
                                                 + "<tr><td>DIAGNOSTICS</td><td>Boot with full configuration plus diagnostics</td></tr>"
                                                 + "<tr><td>DEFAULT</td><td>Boot with default settings</td></tr>"
                                                 + "<tr><td>BOOT_ON_S#_RESUME</td><td>where # is 2, 3, 4 or 5</td></tr>"
                                                 + "<tr><td>FLASH_UPDATE</td><td>Boot on flash update</td></tr>"
                                                 + "<tr><td>RECOVERY</td><td>Boot in recovery mode</td></tr>"
                                                 + "</table></html>");
        }
        return jComboBoxBootModeName;
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
            jComboBoxUsage.setBounds(new java.awt.Rectangle(168, 37, 320, 20));
            jComboBoxUsage.setPreferredSize(new java.awt.Dimension(320, 20));
            jComboBoxUsage
                          .setToolTipText("<html><table>"
                                          + "<tr><td>ALWAYS_CONSUMED</td><td>Indicates Supports the specified boot mode</td></tr>"
                                          + "<tr><td>SOMETIMES_CONSUMED</td><td>Indicates Supports the specified boot mode on some execution paths</td></tr>"
                                          + "<tr><td>ALWAYS_PRODUCED</td><td>Always changes the boot mode</td></tr>"
                                          + "<tr><td>SOMETIMES_PRODUCED</td><td>Change the boot mode sometimes</td></tr>"
                                          + "</table></html>");
        }
        return jComboBoxUsage;
    }

    /**
     * This method initializes jTextFieldFeatureFlag
     * 
     * @return javax.swing.JTextField
     */
    private JTextField getJTextFieldFeatureFlag() {
        if (jTextFieldFeatureFlag == null) {
            jTextFieldFeatureFlag = new JTextField();
            jTextFieldFeatureFlag.setBounds(new java.awt.Rectangle(168, 132, 320, 20));
            jTextFieldFeatureFlag.setPreferredSize(new java.awt.Dimension(320, 20));
            jTextFieldFeatureFlag.setToolTipText("Postfix expression that must evaluate to TRUE or FALSE");
            jTextFieldFeatureFlag.setEnabled(false);
        }
        return jTextFieldFeatureFlag;
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
            jScrollPaneHelpText.setSize(new java.awt.Dimension(320, 40));
            jScrollPaneHelpText.setPreferredSize(new java.awt.Dimension(320, 40));
            jScrollPaneHelpText.setLocation(new java.awt.Point(168, 62));
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
            jButtonOk.setBounds(new java.awt.Rectangle(290, 162, 90, 20));
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
            jButtonCancel.setBounds(new java.awt.Rectangle(390, 162, 90, 20));
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
        this.setSize(505, 235);
        this.setContentPane(getJScrollPane());
        this.setTitle("Boot Modes");
        initFrame();
        this.setViewMode(false);
        this.centerWindow();
    }

    /**
     * This method initializes this Fill values to all fields if these values are
     * not empty
     * 
     * @param inBootModesId
     * 
     */
    private void init(BootModesIdentification inBootModesId, ModuleIdentification mid) {
        init();
        this.id = inBootModesId;
        
        //
        // Init arch with module's arch
        //
        this.jArchCheckBox.setEnabledItems(wt.getModuleArch(mid));
        
        if (this.id != null) {
            this.jComboBoxBootModeName.setSelectedItem(id.getName());
            this.jComboBoxUsage.setSelectedItem(id.getUsage());
            this.jTextAreaHelpText.setText(id.getHelp());
            this.jTextFieldFeatureFlag.setText(id.getFeatureFlag());
            this.jArchCheckBox.setSelectedItems(id.getSupArchList());
        }
    }

    /**
     * This is the override edit constructor
     * 
     * @param inBootModesIdentification
     * @param iFrame
     * 
     */
    public BootModesDlg(BootModesIdentification inBootModesIdentification, IFrame iFrame, ModuleIdentification mid) {
        super(iFrame, true);
        init(inBootModesIdentification, mid);
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
            this.jComboBoxBootModeName.setEnabled(!isView);
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
            jStarLabel1.setLocation(new java.awt.Point(2, 12));
            jLabelBootModeName = new JLabel();
            jLabelBootModeName.setText("Boot Mode Name");
            jLabelBootModeName.setBounds(new java.awt.Rectangle(12, 12, 155, 20));
            jStarLabel2 = new StarLabel();
            jStarLabel2.setLocation(new java.awt.Point(2, 37));
            jLabelUsage = new JLabel();
            jLabelUsage.setText("Usage");
            jLabelUsage.setBounds(new java.awt.Rectangle(12, 37, 155, 20));
            jLabelHelpText = new JLabel();
            jLabelHelpText.setBounds(new java.awt.Rectangle(12, 62, 155, 20));
            jLabelHelpText.setText("Help Text");
            jLabelFeatureFlag = new JLabel();
            jLabelFeatureFlag.setBounds(new java.awt.Rectangle(12, 132, 155, 20));
            jLabelFeatureFlag.setText("Feature Flag Expression");
            jLabelFeatureFlag.setEnabled(false);
            jLabelArch = new JLabel();
            jLabelArch.setBounds(new java.awt.Rectangle(12, 107, 155, 20));
            jLabelArch.setText("Supported Archectures");
            jArchCheckBox = new ArchCheckBox();
            jArchCheckBox.setBounds(new java.awt.Rectangle(168, 107, 320, 20));
            jArchCheckBox.setPreferredSize(new java.awt.Dimension(320, 20));

            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.setPreferredSize(new java.awt.Dimension(480, 180));

            jContentPane.add(jLabelBootModeName, null);
            jContentPane.add(getJComboBoxBootModeName(), null);
            jContentPane.add(jLabelUsage, null);
            jContentPane.add(getJComboBoxUsage(), null);

            jContentPane.add(jStarLabel1, null);
            jContentPane.add(jStarLabel2, null);
            jContentPane.add(jLabelFeatureFlag, null);
            jContentPane.add(getJTextFieldFeatureFlag(), null);
            jContentPane.add(jLabelArch, null);

            jContentPane.add(jLabelHelpText, null);
            jContentPane.add(getJScrollPaneHelpText(), null);
            jContentPane.add(jArchCheckBox, null);
            jContentPane.add(getJButtonOk(), null);
            jContentPane.add(getJButtonCancel(), null);
        }
        return jContentPane;
    }

    /**
     * This method initializes BootModeName groups and Usage type
     * 
     */
    private void initFrame() {
        Tools.generateComboBoxByVector(jComboBoxBootModeName, ed.getVBootModeNames());
        Tools.generateComboBoxByVector(jComboBoxUsage, ed.getVBootModeUsage());
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
                getCurrentBootModes();
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
        // Check FeatureFlag
        //
        if (!isEmpty(this.jTextFieldFeatureFlag.getText())) {
            if (!DataValidation.isFeatureFlag(this.jTextFieldFeatureFlag.getText())) {
                Log.wrn("Update Boot Modes", "Incorrect data type for Feature Flag");
                return false;
            }
        }

        return true;
    }

    private BootModesIdentification getCurrentBootModes() {
        String arg0 = this.jComboBoxBootModeName.getSelectedItem().toString();
        String arg1 = this.jComboBoxUsage.getSelectedItem().toString();

        String arg2 = this.jTextFieldFeatureFlag.getText();
        Vector<String> arg3 = this.jArchCheckBox.getSelectedItemsVector();
        String arg4 = this.jTextAreaHelpText.getText();
        id = new BootModesIdentification(arg0, arg1, arg2, arg3, arg4);
        return id;
    }

    public BootModesIdentification getId() {
        return id;
    }

    public void setId(BootModesIdentification id) {
        this.id = id;
    }
}
