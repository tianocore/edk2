/** @file
 
 The file is used to create, update Package Dependencies section of the MSA file
 
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

import org.tianocore.frameworkwizard.common.DataType;
import org.tianocore.frameworkwizard.common.DataValidation;
import org.tianocore.frameworkwizard.common.Log;
import org.tianocore.frameworkwizard.common.ui.ArchCheckBox;
import org.tianocore.frameworkwizard.common.ui.IDialog;
import org.tianocore.frameworkwizard.common.ui.IFrame;
import org.tianocore.frameworkwizard.common.ui.StarLabel;
import org.tianocore.frameworkwizard.module.Identifications.ModuleIdentification;
import org.tianocore.frameworkwizard.module.Identifications.PackageDependencies.PackageDependenciesIdentification;
import org.tianocore.frameworkwizard.packaging.PackageIdentification;
import org.tianocore.frameworkwizard.workspace.WorkspaceTools;

/**
 * The class is used to create, update Package Dependencies section of the MSA file
 * 
 * It extends IDialog
 * 
 */
public class PackageDepDlg extends IDialog implements ItemListener {
    // /
    // / Define class Serial Version UID
    // /
    private static final long serialVersionUID = 3465193035145152131L;

    //
    // Define class members
    //
    private JPanel jContentPane = null;

    private JLabel jLabelPackageName = null;

    private StarLabel jStarLabel1 = null;

    private JComboBox jComboBoxPackageName = null;

    private JLabel jLabelPackageGuid = null;

    private JTextField jTextFieldPackageGuid = null;

    private JLabel jLabelPackageVersion = null;

    private JTextField jTextFieldPackageVersion = null;

    private JLabel jLabelFeatureFlag = null;

    private JTextField jTextFieldFeatureFlag = null;

    private JScrollPane jScrollPane = null;

    private JLabel jLabelArch = null;

    private ArchCheckBox jArchCheckBox = null;

    private JButton jButtonOk = null;

    private JButton jButtonCancel = null;

    //
    // Not used by UI
    //
    private PackageDependenciesIdentification pdid = null;

    private WorkspaceTools wt = new WorkspaceTools();

    private Vector<PackageIdentification> vPackage = wt.getAllPackages();

    /**
     * This method initializes jComboBoxPackageName
     * 
     * @return javax.swing.JComboBox
     */
    private JComboBox getJComboBoxPackageName() {
        if (jComboBoxPackageName == null) {
            jComboBoxPackageName = new JComboBox();
            jComboBoxPackageName.setBounds(new java.awt.Rectangle(168, 12, 320, 20));
            jComboBoxPackageName.setPreferredSize(new java.awt.Dimension(320, 20));
            jComboBoxPackageName.setToolTipText("If your Module requires a package, list that here.");
            jComboBoxPackageName.addItemListener(this);
        }
        return jComboBoxPackageName;
    }

    /**
     * This method initializes jTextFieldPackageGuid
     * 
     * @return javax.swing.JTextField
     */
    private JTextField getJTextFieldPackageGuid() {
        if (jTextFieldPackageGuid == null) {
            jTextFieldPackageGuid = new JTextField();
            jTextFieldPackageGuid.setBounds(new java.awt.Rectangle(168, 37, 320, 20));
            jTextFieldPackageGuid.setPreferredSize(new java.awt.Dimension(320, 20));
            jTextFieldPackageGuid.setEditable(false);
            jTextFieldPackageGuid.setVisible(false);
        }
        return jTextFieldPackageGuid;
    }

    /**
     * This method initializes jTextFieldPackageVersion
     * 
     * @return javax.swing.JTextField
     */
    private JTextField getJTextFieldPackageVersion() {
        if (jTextFieldPackageVersion == null) {
            jTextFieldPackageVersion = new JTextField();
            jTextFieldPackageVersion.setBounds(new java.awt.Rectangle(168, 37, 320, 20));
            jTextFieldPackageVersion.setPreferredSize(new java.awt.Dimension(320, 20));
            jTextFieldPackageVersion
                                    .setToolTipText("<html>If this module depends on a specific version of a package, <br>"
                                                    + "enter the package version here.  <br>"
                                                    + "If the module can use the latest version <br>"
                                                    + "that does not break backward compatibility, <br>"
                                                    + "leave this field blank</html>");
        }
        return jTextFieldPackageVersion;
    }

    /**
     * This method initializes jTextFieldFeatureFlag
     * 
     * @return javax.swing.JTextField
     */
    private JTextField getJTextFieldFeatureFlag() {
        if (jTextFieldFeatureFlag == null) {
            jTextFieldFeatureFlag = new JTextField();
            jTextFieldFeatureFlag.setBounds(new java.awt.Rectangle(168, 87, 320, 20));
            jTextFieldFeatureFlag.setPreferredSize(new java.awt.Dimension(320, 20));
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
     * This method initializes jButtonOk
     * 
     * @return javax.swing.JButton
     * 
     */
    private JButton getJButtonOk() {
        if (jButtonOk == null) {
            jButtonOk = new JButton();
            jButtonOk.setBounds(new java.awt.Rectangle(290, 122, 90, 20));
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
            jButtonCancel.setBounds(new java.awt.Rectangle(390, 122, 90, 20));
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
        this.setSize(505, 216);
        this.setContentPane(getJScrollPane());
        this.setTitle("Package Dependencies");
        initFrame();
        this.setViewMode(false);
        this.centerWindow();
    }

    /**
     * This method initializes this
     * 
     */
    private void init(PackageDependenciesIdentification inPackageDependenciesIdentification, ModuleIdentification mid) {
        init();
        this.pdid = inPackageDependenciesIdentification;
        
        //
        // Init arch with module's arch
        //
        this.jArchCheckBox.setEnabledItems(wt.getModuleArch(mid));
        
        if (pdid != null) {
            this.jComboBoxPackageName.setSelectedItem(pdid.getName());
            this.jTextFieldPackageVersion.setText(pdid.getVersion());
            this.jTextFieldPackageGuid.setText(pdid.getGuid());
            jTextFieldFeatureFlag.setText(pdid.getFeatureFlag());
            this.jArchCheckBox.setSelectedItems(pdid.getSupArchList());
        }
    }

    /**
     * This is the default constructor
     * 
     */
    public PackageDepDlg(PackageDependenciesIdentification inPackageDependenciesIdentification, IFrame iFrame, ModuleIdentification mid) {
        super(iFrame, true);
        init(inPackageDependenciesIdentification, mid);
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
            this.jComboBoxPackageName.setEnabled(!isView);
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
            jLabelPackageName = new JLabel();
            jLabelPackageName.setBounds(new java.awt.Rectangle(12, 12, 168, 20));
            jLabelPackageName.setText("Package Name");

            jLabelPackageVersion = new JLabel();
            jLabelPackageVersion.setBounds(new java.awt.Rectangle(12, 37, 168, 20));
            jLabelPackageVersion.setText("Package Version");

            jLabelPackageGuid = new JLabel();
            jLabelPackageGuid.setBounds(new java.awt.Rectangle(12, 37, 168, 20));
            jLabelPackageGuid.setText("Package Guid");
            jLabelPackageGuid.setVisible(false);

            jLabelFeatureFlag = new JLabel();
            jLabelFeatureFlag.setBounds(new java.awt.Rectangle(12, 87, 168, 20));
            jLabelFeatureFlag.setText("Feature Flag Expression");
            jLabelFeatureFlag.setEnabled(false);

            jLabelArch = new JLabel();
            jLabelArch.setBounds(new java.awt.Rectangle(12, 62, 168, 20));
            jLabelArch.setText("Supported Architectures");
            jArchCheckBox = new ArchCheckBox();
            jArchCheckBox.setBounds(new java.awt.Rectangle(168, 62, 320, 20));
            jArchCheckBox.setPreferredSize(new java.awt.Dimension(320, 20));

            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.setPreferredSize(new java.awt.Dimension(485, 170));

            jContentPane.add(jStarLabel1, null);
            jContentPane.add(jLabelPackageName, null);
            jContentPane.add(getJComboBoxPackageName(), null);
            jContentPane.add(jLabelPackageGuid, null);
            jContentPane.add(getJTextFieldPackageGuid(), null);
            jContentPane.add(jLabelPackageVersion, null);
            jContentPane.add(getJTextFieldPackageVersion(), null);
            // LAH Not Used here
            // jContentPane.add(getJButtonGenerateGuid(), null);
            jContentPane.add(jLabelFeatureFlag, null);
            jContentPane.add(getJTextFieldFeatureFlag(), null);
            jContentPane.add(jLabelArch, null);
            jContentPane.add(jArchCheckBox, null);
            jContentPane.add(getJButtonOk(), null);
            jContentPane.add(getJButtonCancel(), null);
        }
        return jContentPane;
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
                getCurrentPackageDependencies();
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
     * This method initializes Usage type, Package type and Arch type
     * 
     */
    private void initFrame() {
        for (int index = 0; index < vPackage.size(); index++) {
            jComboBoxPackageName.addItem(vPackage.elementAt(index).getName());
        }
    }

    /**
     * Data validation for all fields before add current item to Vector
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
        // Check PackageGuid
        //
        // if (!isEmpty(this.jTextFieldPackageGuid.getText())) {
        // if (!DataValidation.isGuid(this.jTextFieldPackageGuid.getText())) {
        // Log.err("Incorrect data type for Package Guid");
        // return false;
        // }
        // }

        //
        // Check PackageVersion
        //
        if (!isEmpty(this.jTextFieldPackageVersion.getText())) {
            if (!DataValidation.isVersion(this.jTextFieldPackageVersion.getText())) {
                Log.wrn("Update Package Dependencies", "Incorrect data type for Package Version");
                return false;
            }
        }

        //
        // Check FeatureFlag
        //
        if (!isEmpty(this.jTextFieldFeatureFlag.getText())) {
            if (!DataValidation.isFeatureFlag(this.jTextFieldFeatureFlag.getText())) {
                Log.wrn("Update Package Dependencies", "Incorrect data type for Feature Flag");
                return false;
            }
        }

        return true;
    }

    private PackageDependenciesIdentification getCurrentPackageDependencies() {
        String arg0 = this.jComboBoxPackageName.getSelectedItem().toString();
        String arg1 = this.jTextFieldPackageVersion.getText();
        String arg2 = this.jTextFieldPackageGuid.getText();

        String arg3 = this.jTextFieldFeatureFlag.getText();
        Vector<String> arg4 = this.jArchCheckBox.getSelectedItemsVector();
        pdid = new PackageDependenciesIdentification(arg0, arg1, arg2, arg3, arg4);
        return pdid;
    }

    /*
     * (non-Javadoc)
     * 
     * @see java.awt.event.ItemListener#itemStateChanged(java.awt.event.ItemEvent)
     * 
     * Reflesh the frame when selected item changed
     * 
     */
    public void itemStateChanged(ItemEvent arg0) {
        if (arg0.getSource() == this.jComboBoxPackageName && arg0.getStateChange() == ItemEvent.SELECTED) {
            for (int index = 0; index < vPackage.size(); index++) {
                if (this.jComboBoxPackageName.getSelectedItem().toString().equals(vPackage.get(index).getName())) {
                    this.jTextFieldPackageGuid.setText(vPackage.get(index).getGuid());
                }
            }
        }
    }

    public PackageDependenciesIdentification getPdid() {
        return pdid;
    }

    public void setPdid(PackageDependenciesIdentification pdid) {
        this.pdid = pdid;
    }

}
