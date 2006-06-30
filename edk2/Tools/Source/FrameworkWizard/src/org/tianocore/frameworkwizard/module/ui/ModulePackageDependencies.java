/** @file
 
 The file is used to create, update Include of MSA/MBD file
 
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
import java.awt.event.ItemListener;
import java.util.Vector;

import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;
import javax.swing.JTextField;

import org.tianocore.PackageDependenciesDocument;
import org.tianocore.ModuleSurfaceAreaDocument.ModuleSurfaceArea;
import org.tianocore.PackageDependenciesDocument.PackageDependencies;
import org.tianocore.PackageDependenciesDocument.PackageDependencies.Package;
import org.tianocore.frameworkwizard.common.DataType;
import org.tianocore.frameworkwizard.common.DataValidation;
import org.tianocore.frameworkwizard.common.EnumerationData;
import org.tianocore.frameworkwizard.common.Log;
import org.tianocore.frameworkwizard.common.OpeningModuleType;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.ui.IInternalFrame;
import org.tianocore.frameworkwizard.common.ui.StarLabel;
import org.tianocore.frameworkwizard.common.ui.iCheckBoxList.ICheckBoxList;
import org.tianocore.frameworkwizard.module.Identification.PackageDependencies.PackageDependenciesIdentification;
import org.tianocore.frameworkwizard.module.Identification.PackageDependencies.PackageDependenciesVector;
import org.tianocore.frameworkwizard.packaging.PackageIdentification;
import org.tianocore.frameworkwizard.workspace.WorkspaceTools;

/**
 The class is used to create, update Include of MSA/MBD file
 It extends IInternalFrame
 


 **/
public class ModulePackageDependencies extends IInternalFrame implements ItemListener {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = 3465193035145152131L;

    //
    //Define class members
    //
    private JPanel jContentPane = null;

    private JLabel jLabelPackageName = null;

    private StarLabel jStarLabel1 = null;

    private JComboBox jComboBoxPackageName = null;

    private JLabel jLabelPackageGuid = null;

    private JTextField jTextFieldPackageGuid = null;

    private JButton jButtonGenerateGuid = null;

    private JLabel jLabelPackageVersion = null;

    private JTextField jTextFieldPackageVersion = null;

    private JLabel jLabelFeatureFlag = null;

    private JTextField jTextFieldFeatureFlag = null;

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

    //
    // Not used by UI
    //
    private int intSelectedItemId = 0;

    private OpeningModuleType omt = null;

    private ModuleSurfaceArea msa = null;

    private PackageDependenciesIdentification id = null;

    private PackageDependenciesVector vid = new PackageDependenciesVector();

    private PackageDependenciesDocument.PackageDependencies packageDependencies = null;

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
            jComboBoxPackageName.setBounds(new java.awt.Rectangle(160, 10, 320, 20));
            jComboBoxPackageName.setPreferredSize(new java.awt.Dimension(320, 20));
            jComboBoxPackageName.setToolTipText("If your Module requires a package list that here.");
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
            jTextFieldPackageGuid.setBounds(new java.awt.Rectangle(160, 35, 320, 20));
            jTextFieldPackageGuid.setPreferredSize(new java.awt.Dimension(320, 20));
            jTextFieldPackageGuid.setEditable(false);
        }
        return jTextFieldPackageGuid;
    }

    /**
     * This method initializes jButtonGenerateGuid	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonGenerateGuid() {
        if (jButtonGenerateGuid == null) {
            jButtonGenerateGuid = new JButton();
            jButtonGenerateGuid.setBounds(new java.awt.Rectangle(415, 35, 65, 20));
            jButtonGenerateGuid.setPreferredSize(new java.awt.Dimension(65, 20));
            jButtonGenerateGuid.setText("GEN");
            jButtonGenerateGuid.addActionListener(this);
            jButtonGenerateGuid.setVisible(false);
        }
        return jButtonGenerateGuid;
    }

    /**
     * This method initializes jTextFieldPackageVersion	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldPackageVersion() {
        if (jTextFieldPackageVersion == null) {
            jTextFieldPackageVersion = new JTextField();
            jTextFieldPackageVersion.setBounds(new java.awt.Rectangle(160, 60, 320, 20));
            jTextFieldPackageVersion.setPreferredSize(new java.awt.Dimension(320, 20));
            jTextFieldPackageVersion
                                    .setToolTipText("If this module depends on a specific version of a package, enter the package version here.  If the module can use the latest version that does not break backward compatibility, leave this field blank");
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
            jTextFieldFeatureFlag.setBounds(new java.awt.Rectangle(160, 85, 320, 20));
            jTextFieldFeatureFlag.setPreferredSize(new java.awt.Dimension(320, 20));
        }
        return jTextFieldFeatureFlag;
    }

    /**
     This method initializes jComboBoxFileList 
     
     @return javax.swing.JComboBox jComboBoxFileList
     
     **/
    private JComboBox getJComboBoxList() {
        if (jComboBoxList == null) {
            jComboBoxList = new JComboBox();
            jComboBoxList.setBounds(new java.awt.Rectangle(15, 195, 210, 20));
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
            jButtonAdd.setBounds(new java.awt.Rectangle(230, 195, 80, 20));
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
            jButtonRemove.setBounds(new java.awt.Rectangle(400, 195, 80, 20));
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
            jButtonUpdate.setBounds(new java.awt.Rectangle(315, 195, 80, 20));
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
            jScrollPaneList.setBounds(new java.awt.Rectangle(15, 220, 465, 240));
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
            jScrollPaneArch.setBounds(new java.awt.Rectangle(160, 110, 320, 80));
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
        this.setTitle("Includes");
        initFrame();
        this.setViewMode(false);
    }

    /**
     This method initializes this
     Fill values to all fields if these values are not empty
     
     @param inPackageDependencies

     **/
    private void init(PackageDependencies inPackageDependencies) {
        init();
        this.packageDependencies = inPackageDependencies;

        if (this.packageDependencies != null) {
            if (this.packageDependencies.getPackageList().size() > 0) {
                for (int index = 0; index < this.packageDependencies.getPackageList().size(); index++) {
                    String arg0 = "";
                    String arg1 = packageDependencies.getPackageList().get(index).getPackageVersion();
                    String arg2 = packageDependencies.getPackageList().get(index).getPackageGuid();
                    //
                    // If no guid, skip current item
                    //
                    if (arg2 == null) {
                        continue;
                    }
                    for (int indexJ = 0; indexJ < this.vPackage.size(); indexJ++) {
                        if (vPackage.get(indexJ).getGuid().equals(arg2)) {
                            arg0 = vPackage.get(indexJ).getName();
                        }
                    }
                    String arg3 = packageDependencies.getPackageList().get(index).getFeatureFlag();
                    Vector<String> arg4 = Tools.convertListToVector(packageDependencies.getPackageList().get(index)
                                                                                       .getSupArchList());
                    id = new PackageDependenciesIdentification(arg0, arg1, arg2, arg3, arg4);
                    vid.addPackageDependencies(id);
                }
            }
        }
        //
        // Update the list
        //
        Tools.generateComboBoxByVector(jComboBoxList, vid.getPackageDependenciesName());
        reloadListArea();
    }

    /**
     This is the default constructor
     
     **/
    public ModulePackageDependencies() {
        super();
        init();
        this.setVisible(true);
    }

    /**
     This is the override constructor
     
     **/
    public ModulePackageDependencies(OpeningModuleType inOmt) {
        super();
        this.omt = inOmt;
        this.msa = omt.getXmlMsa();
        init(msa.getPackageDependencies());
        this.setVisible(true);
    }

    /**
     Disable all components when the mode is view
     
     @param isView true - The view mode; false - The non-view mode
     
     **/
    public void setViewMode(boolean isView) {
        if (isView) {
            this.jComboBoxPackageName.setEnabled(!isView);
            this.jButtonAdd.setEnabled(!isView);
            this.jButtonUpdate.setEnabled(!isView);
            this.jButtonRemove.setEnabled(!isView);
        }
    }

    /**
     This method initializes jContentPane
     
     @return javax.swing.JPanel jContentPane
     
     **/
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jLabelArch = new JLabel();
            jLabelArch.setBounds(new java.awt.Rectangle(15, 110, 140, 20));
            jLabelArch.setText("Sup Arch List");
            jLabelPackageVersion = new JLabel();
            jLabelPackageVersion.setBounds(new java.awt.Rectangle(15, 60, 140, 20));
            jLabelPackageVersion.setText("Package Version");
            jLabelPackageGuid = new JLabel();
            jLabelPackageGuid.setBounds(new java.awt.Rectangle(15, 35, 140, 20));
            jLabelPackageGuid.setText("Package Guid");
            jLabelPackageName = new JLabel();
            jLabelPackageName.setBounds(new java.awt.Rectangle(15, 10, 140, 20));
            jLabelPackageName.setText("Package Name");

            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.setPreferredSize(new java.awt.Dimension(490, 470));

            jContentPane.add(jLabelPackageName, null);
            jStarLabel1 = new StarLabel();
            jStarLabel1.setLocation(new java.awt.Point(0, 35));

            jContentPane.add(jStarLabel1, null);
            jContentPane.add(getJComboBoxPackageName(), null);
            jContentPane.add(jLabelPackageGuid, null);
            jContentPane.add(getJTextFieldPackageGuid(), null);
            jContentPane.add(getJButtonGenerateGuid(), null);
            jContentPane.add(jLabelPackageVersion, null);
            jContentPane.add(getJTextFieldPackageVersion(), null);

            jLabelFeatureFlag = new JLabel();
            jLabelFeatureFlag.setBounds(new java.awt.Rectangle(15, 85, 140, 20));
            jLabelFeatureFlag.setText("Feature Flag");
            jContentPane.add(jLabelFeatureFlag, null);
            jContentPane.add(getJTextFieldFeatureFlag(), null);

            jContentPane.add(getJComboBoxList(), null);
            jContentPane.add(getJButtonAdd(), null);
            jContentPane.add(getJButtonRemove(), null);
            jContentPane.add(getJButtonUpdate(), null);
            jContentPane.add(getJScrollPaneList(), null);
            jContentPane.add(jLabelArch, null);
            jContentPane.add(getJScrollPaneArch(), null);
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
     This method initializes Usage type, Package type and Arch type
     
     **/
    private void initFrame() {
        EnumerationData ed = new EnumerationData();

        this.iCheckBoxListArch.setAllItems(ed.getVSupportedArchitectures());

        for (int index = 0; index < vPackage.size(); index++) {
            jComboBoxPackageName.addItem(vPackage.elementAt(index).getName());
        }
        //jComboBoxPackageName.addItemListener(this);
    }

    /**
     Data validation for all fields
     
     @retval true - All datas are valid
     @retval false - At least one data is invalid
     
     **/
    public boolean check() {
        return true;
    }

    /**
     Data validation for all fields before add current item to Vector
     
     @retval true - All datas are valid
     @retval false - At least one data is invalid
     
     **/
    public boolean checkAdd() {
        //
        // Check if all fields have correct data types 
        //
        
        //
        // Check PackageGuid 
        //
        if (!isEmpty(this.jTextFieldPackageGuid.getText())) {
            if (!DataValidation.isGuid(this.jTextFieldPackageGuid.getText())) {
                Log.err("Incorrect data type for Package Guid");
                return false;
            }
        }
        
        //
        // Check PackageVersion 
        //
        if (!isEmpty(this.jTextFieldPackageVersion.getText())) {
            if (!DataValidation.isVersion(this.jTextFieldPackageVersion.getText())) {
                Log.err("Incorrect data type for Package Version");
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
     Save all components of Includes
     if exists includes, set the value directly
     if not exists includes, new an instance first
     
     **/
    public void save() {
        try {
            //
            //Save as file name
            //
            int count = this.vid.size();

            this.packageDependencies = PackageDependencies.Factory.newInstance();
            if (count > 0) {
                for (int index = 0; index < count; index++) {
                    Package p = Package.Factory.newInstance();

                    if (!isEmpty(vid.getPackageDependencies(index).getVersion())) {
                        p.setPackageVersion(vid.getPackageDependencies(index).getVersion());
                    }
                    if (!isEmpty(vid.getPackageDependencies(index).getGuid())) {
                        p.setPackageGuid(vid.getPackageDependencies(index).getGuid());
                    }
                    if (!isEmpty(vid.getPackageDependencies(index).getFeatureFlag())) {
                        p.setFeatureFlag(vid.getPackageDependencies(index).getFeatureFlag());
                    }
                    if (vid.getPackageDependencies(index).getSupArchList() != null
                        && vid.getPackageDependencies(index).getSupArchList().size() > 0) {
                        p.setSupArchList(vid.getPackageDependencies(index).getSupArchList());
                    }

                    this.packageDependencies.addNewPackage();
                    this.packageDependencies.setPackageArray(index, p);
                }
            }
            this.msa.setPackageDependencies(packageDependencies);
            this.omt.setSaved(false);
        } catch (Exception e) {
            e.printStackTrace();
            Log.err("Update Package Dependencies", e.getMessage());
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

        resizeComponentWidth(this.jComboBoxPackageName, intCurrentWidth, intPreferredWidth);
        resizeComponentWidth(this.jTextFieldPackageGuid, intCurrentWidth, intPreferredWidth);
        resizeComponentWidth(this.jTextFieldPackageVersion, intCurrentWidth, intPreferredWidth);
        resizeComponentWidth(this.jTextFieldFeatureFlag, intCurrentWidth, intPreferredWidth);
        resizeComponentWidth(this.jScrollPaneArch, intCurrentWidth, intPreferredWidth);

        resizeComponentWidth(jComboBoxList, intCurrentWidth, intPreferredWidth);
        resizeComponent(jScrollPaneList, intCurrentWidth, intCurrentHeight, intPreferredWidth, intPreferredHeight);
        relocateComponentX(jButtonAdd, intCurrentWidth, intPreferredWidth, DataType.SPACE_TO_RIGHT_FOR_ADD_BUTTON);
        relocateComponentX(jButtonRemove, intCurrentWidth, intPreferredWidth, DataType.SPACE_TO_RIGHT_FOR_REMOVE_BUTTON);
        relocateComponentX(jButtonUpdate, intCurrentWidth, intPreferredWidth, DataType.SPACE_TO_RIGHT_FOR_UPDATE_BUTTON);
        relocateComponentX(this.jButtonGenerateGuid, intCurrentWidth, intPreferredWidth,
                           DataType.SPACE_TO_RIGHT_FOR_GENERATE_BUTTON);
    }

    private PackageDependenciesIdentification getCurrentPackageDependencies() {
        String arg0 = this.jComboBoxPackageName.getSelectedItem().toString();
        String arg1 = this.jTextFieldPackageVersion.getText();
        String arg2 = this.jTextFieldPackageGuid.getText();

        String arg3 = this.jTextFieldFeatureFlag.getText();
        Vector<String> arg4 = this.iCheckBoxListArch.getAllCheckedItemsString();
        id = new PackageDependenciesIdentification(arg0, arg1, arg2, arg3, arg4);
        return id;
    }

    /**
     Add current item to Vector
     
     **/
    private void addToList() {
        intSelectedItemId = vid.size();

        vid.addPackageDependencies(getCurrentPackageDependencies());

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

        vid.removePackageDependencies(intTempIndex);

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

        vid.updatePackageDependencies(getCurrentPackageDependencies(), intTempIndex);

        jComboBoxList.removeAllItems();
        for (int index = 0; index < vid.size(); index++) {
            jComboBoxList.addItem(vid.getPackageDependencies(index).getName());
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

            this.jComboBoxPackageName.setSelectedItem(vid.getPackageDependencies(intSelectedItemId).getName());
            this.jTextFieldPackageVersion.setText(vid.getPackageDependencies(intSelectedItemId).getVersion());
            this.jTextFieldPackageGuid.setText(vid.getPackageDependencies(intSelectedItemId).getGuid());

            jTextFieldFeatureFlag.setText(vid.getPackageDependencies(intSelectedItemId).getFeatureFlag());
            iCheckBoxListArch.setAllItemsUnchecked();
            iCheckBoxListArch.initCheckedItem(true, vid.getPackageDependencies(intSelectedItemId).getSupArchList());

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
            strListItem = strListItem + vid.getPackageDependencies(index).getName() + DataType.UNIX_LINE_SEPARATOR;
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
        if (arg0.getSource() == this.jComboBoxPackageName && arg0.getStateChange() == ItemEvent.SELECTED) {
            for (int index = 0; index < vPackage.size(); index++) {
                if (this.jComboBoxPackageName.getSelectedItem().toString().equals(vPackage.get(index).getName())) {
                    this.jTextFieldPackageGuid.setText(vPackage.get(index).getGuid());
                }
            }
        }
    }

}
