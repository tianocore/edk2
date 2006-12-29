/** @file
 
 The file is used to create, update Ppi section of the MSA file
 
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
import java.awt.event.ItemEvent;
import java.awt.event.ItemListener;
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
import org.tianocore.frameworkwizard.common.ui.IComboBox;
import org.tianocore.frameworkwizard.common.ui.IDialog;
import org.tianocore.frameworkwizard.common.ui.IFrame;
import org.tianocore.frameworkwizard.common.ui.StarLabel;
import org.tianocore.frameworkwizard.module.Identifications.ModuleIdentification;
import org.tianocore.frameworkwizard.module.Identifications.Ppis.PpisIdentification;
import org.tianocore.frameworkwizard.packaging.PackageIdentification;
import org.tianocore.frameworkwizard.workspace.WorkspaceTools;

/**
 * The class is used to create, update Ppi section of the MSA file
 * 
 * It extends IDialog
 * 
 */
public class PpisDlg extends IDialog implements ItemListener {

    // /
    // / Define class Serial Version UID
    // /
    private static final long serialVersionUID = -4284901202357037724L;

    //
    // Define class members
    //
    private JPanel jContentPane = null;

    private JLabel jLabelC_Name = null;

    private IComboBox iComboBoxCName = null;

    private JTextField jTextFieldFeatureFlag = null;

    private JLabel jLabelFeatureFlag = null;

    private JLabel jLabelUsage = null;

    private JComboBox jComboBoxUsage = null;

    private JLabel jLabelPpiType = null;

    private StarLabel jStarLabel1 = null;

    private StarLabel jStarLabel2 = null;

    private StarLabel jStarLabel3 = null;

    private JComboBox jComboBoxPpiType = null;

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
    private PpisIdentification id = null;

    private WorkspaceTools wt = new WorkspaceTools();

    private EnumerationData ed = new EnumerationData();

    /**
     * This method initializes jComboBoxPpiType
     * 
     * @return javax.swing.JComboBox
     */
    private JComboBox getJComboBoxPpiType() {
        if (jComboBoxPpiType == null) {
            jComboBoxPpiType = new JComboBox();
            jComboBoxPpiType.setBounds(new java.awt.Rectangle(168, 12, 320, 20));
            jComboBoxPpiType.setPreferredSize(new java.awt.Dimension(320, 20));
            jComboBoxPpiType.addItemListener(this);
            jComboBoxPpiType
                            .setToolTipText("<html>PPIs are named by GUID.<br>PPI Notify is consumed via a register PPI Notify mechanism</html>");
        }
        return jComboBoxPpiType;
    }

    /**
     * This method initializes jTextFieldC_Name
     * 
     * @return javax.swing.JTextField jTextFieldC_Name
     * 
     */
    private IComboBox getIComboBoxCName() {
        if (iComboBoxCName == null) {
            iComboBoxCName = new IComboBox();
            iComboBoxCName.setBounds(new java.awt.Rectangle(168, 37, 320, 20));
            iComboBoxCName.setPreferredSize(new java.awt.Dimension(320, 20));
            iComboBoxCName.setToolTipText("Select Guid C Name of PPI");
        }
        return iComboBoxCName;
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
            jComboBoxUsage.setBounds(new java.awt.Rectangle(168, 62, 320, 20));
            jComboBoxUsage.setPreferredSize(new java.awt.Dimension(320, 20));
            jComboBoxUsage
                          .setToolTipText("<html><table><tr><td colspan=2 align=center><b>PPI</b></td></tr><tr><td>ALWAYS_CONSUMED</td><td>Module always consumes the PPI</td></tr><tr><td>SOMETIMES_CONSUMED</td><td>Module sometimes consumes the PPI</td></tr><tr><td>ALWAYS_PRODUCED</td><td>Module always produces the PPI</td></tr><tr><td>SOMETIMES_PRODUCED</td><td>Module sometimes produces the PPI</td></tr><tr><td colspan=2 align=center><b>PPI Notify</b></td></tr><tr><td>SOMETIMES_CONSUMED</td><td>Module will consume the PPI if it is produced. Consumption<br>is defined by executing the PPI notify function</td></tr></table></html>");
        }
        return jComboBoxUsage;
    }

    /**
     * This method initializes jTextFieldFeatureFlag
     * 
     * @return javax.swing.JTextField jTextFieldFeatureFlag
     * 
     */
    private JTextField getJTextFieldFeatureFlag() {
        if (jTextFieldFeatureFlag == null) {
            jTextFieldFeatureFlag = new JTextField();
            jTextFieldFeatureFlag.setBounds(new java.awt.Rectangle(168, 157, 320, 20));
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
            jScrollPaneHelpText.setLocation(new java.awt.Point(168, 87));
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
            jButtonOk.setBounds(new java.awt.Rectangle(290, 187, 90, 20));
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
            jButtonCancel.setBounds(new java.awt.Rectangle(390, 187, 90, 20));
            jButtonCancel.setText("Cancel");
            jButtonCancel.addActionListener(this);
        }
        return jButtonCancel;
    }

    /**
     * This method initializes this
     * 
     */
    private void init() {
        this.setSize(505, 260);
        this.setContentPane(getJScrollPane());
        this.setTitle("PPI Definitions");
        initFrame();
        this.centerWindow();
    }

    /**
     This method initializes this Fill values to all fields if these values are
     not empty
     
     @param inPpisId
     @param mid
     
     **/
    private void init(PpisIdentification inPpisId, ModuleIdentification mid) {
        init();
        this.id = inPpisId;
        
        //
        // Init arch with module's arch
        //
        this.jArchCheckBox.setEnabledItems(wt.getModuleArch(mid));
        
        //
        // Get defined ppis from dependent packages
        //
        Vector<PackageIdentification> vpid = wt.getPackageDependenciesOfModule(mid);
        if (vpid.size() <= 0) {
            Log.wrn("Init Ppi", "This module hasn't defined any package dependency, so there is no ppi can be added");
        }

        Tools.generateComboBoxByVector(this.iComboBoxCName,
                                       wt.getAllPpiDeclarationsFromPackages(wt.getPackageDependenciesOfModule(mid)));

        if (this.id != null) {
            this.iComboBoxCName.setSelectedItem(id.getName());
            this.jComboBoxPpiType.setSelectedItem(id.getType());
            this.jComboBoxUsage.setSelectedItem(id.getUsage());
            this.jTextAreaHelpText.setText(id.getHelp());
            this.jTextFieldFeatureFlag.setText(id.getFeatureFlag());
            this.jArchCheckBox.setSelectedItems(id.getSupArchList());
        }
    }

    /**
     * This is the override edit constructor
     * 
     * @param inProtocolsIdentification
     * @param iFrame
     * 
     */
    public PpisDlg(PpisIdentification inPpisIdentification, IFrame iFrame, ModuleIdentification mid) {
        super(iFrame, true);
        init(inPpisIdentification, mid);
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
            jLabelPpiType = new JLabel();
            jLabelPpiType.setBounds(new java.awt.Rectangle(12, 12, 168, 20));
            jLabelPpiType.setText("Select Ppi Type");

            jStarLabel2 = new StarLabel();
            jStarLabel2.setLocation(new java.awt.Point(2, 37));
            jLabelC_Name = new JLabel();
            jLabelC_Name.setText("PPI GUID C Name");
            jLabelC_Name.setBounds(new java.awt.Rectangle(12, 37, 168, 20));

            jStarLabel3 = new StarLabel();
            jStarLabel3.setLocation(new java.awt.Point(2, 62));
            jLabelUsage = new JLabel();
            jLabelUsage.setText("Usage");
            jLabelUsage.setBounds(new java.awt.Rectangle(12, 62, 168, 20));

            jLabelHelpText = new JLabel();
            jLabelHelpText.setBounds(new java.awt.Rectangle(14, 87, 168, 20));
            jLabelHelpText.setText("Help Text");

            jLabelFeatureFlag = new JLabel();
            jLabelFeatureFlag.setText("Feature Flag Expression");
            jLabelFeatureFlag.setBounds(new java.awt.Rectangle(12, 157, 168, 20));
            jLabelFeatureFlag.setEnabled(false);

            jLabelArch = new JLabel();
            jLabelArch.setBounds(new java.awt.Rectangle(12, 132, 168, 20));
            jLabelArch.setText("Supported Architectures");
            jArchCheckBox = new ArchCheckBox();
            jArchCheckBox.setBounds(new java.awt.Rectangle(168, 132, 320, 20));
            jArchCheckBox.setPreferredSize(new java.awt.Dimension(320, 20));

            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.setPreferredSize(new java.awt.Dimension(485, 215));

            jContentPane.add(jStarLabel1, null);
            jContentPane.add(jLabelPpiType, null);
            jContentPane.add(getJComboBoxPpiType(), null);
            jContentPane.add(jStarLabel2, null);
            jContentPane.add(jLabelC_Name, null);
            jContentPane.add(getIComboBoxCName(), null);
            jContentPane.add(jStarLabel3, null);
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
     * This method initializes Usage type
     * 
     */
    private void initFrame() {
        Tools.generateComboBoxByVector(jComboBoxPpiType, ed.getVPpiType());
        Tools.generateComboBoxByVector(jComboBoxUsage, ed.getVPpiUsage());
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
                getCurrentPpis();
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
        if (this.iComboBoxCName.getSelectedItem() == null) {
            Log.wrn("Update Ppis", "Please select one Ppi/PpiNotify Name");
            return false;
        }

        if (!isEmpty(this.iComboBoxCName.getSelectedItem().toString())) {
            if (!DataValidation.isC_NameType(this.iComboBoxCName.getSelectedItem().toString())) {
                Log.wrn("Update Ppis", "Incorrect data type for Ppi/PpiNotify Name");
                return false;
            }
        }

        //
        // Check FeatureFlag
        //
        if (!isEmpty(this.jTextFieldFeatureFlag.getText())) {
            if (!DataValidation.isFeatureFlag(this.jTextFieldFeatureFlag.getText())) {
                Log.wrn("Update Ppis", "Incorrect data type for Feature Flag");
                return false;
            }
        }

        return true;
    }

    private PpisIdentification getCurrentPpis() {
        String arg0 = this.iComboBoxCName.getSelectedItem().toString();
        String arg1 = this.jComboBoxPpiType.getSelectedItem().toString();
        String arg2 = this.jComboBoxUsage.getSelectedItem().toString();

        String arg3 = this.jTextFieldFeatureFlag.getText();
        Vector<String> arg4 = this.jArchCheckBox.getSelectedItemsVector();
        String arg5 = this.jTextAreaHelpText.getText();

        id = new PpisIdentification(arg0, arg1, arg2, arg3, arg4, arg5);
        return id;
    } /*
     * (non-Javadoc)
     * 
     * @see java.awt.event.ItemListener#itemStateChanged(java.awt.event.ItemEvent)
     * 
     * Reflesh the frame when selected item changed
     * 
     */

    public void itemStateChanged(ItemEvent arg0) {
        if (arg0.getSource() == this.jComboBoxPpiType && arg0.getStateChange() == ItemEvent.SELECTED) {
            if (this.jComboBoxPpiType.getSelectedItem().toString().equals(ed.getVPpiType().get(0))) {
                Tools.generateComboBoxByVector(this.jComboBoxUsage, ed.getVPpiUsage());
            } else {
                Tools.generateComboBoxByVector(this.jComboBoxUsage, ed.getVPpiNotifyUsage());
            }
        }
    }

    public PpisIdentification getId() {
        return id;
    }

    public void setId(PpisIdentification id) {
        this.id = id;
    }
}
