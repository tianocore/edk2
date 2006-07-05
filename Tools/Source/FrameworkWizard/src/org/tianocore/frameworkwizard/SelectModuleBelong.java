/** @file
 
 The file is used to let user choose to create module in an existing package
 or to create a new package first.
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/
package org.tianocore.frameworkwizard;

import java.awt.event.ActionEvent;
import java.io.File;
import java.util.Vector;

import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.JFileChooser;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JTextField;

import org.tianocore.ModuleSurfaceAreaDocument;
import org.tianocore.MsaHeaderDocument;
import org.tianocore.SpdHeaderDocument;
import org.tianocore.PackageSurfaceAreaDocument.PackageSurfaceArea;
import org.tianocore.PlatformHeaderDocument.PlatformHeader;
import org.tianocore.PlatformSurfaceAreaDocument.PlatformSurfaceArea;
import org.tianocore.frameworkwizard.common.DataType;
import org.tianocore.frameworkwizard.common.DataValidation;
import org.tianocore.frameworkwizard.common.IFileFilter;
import org.tianocore.frameworkwizard.common.Log;
import org.tianocore.frameworkwizard.common.SaveFile;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.ui.IDialog;
import org.tianocore.frameworkwizard.common.ui.IFrame;
import org.tianocore.frameworkwizard.common.ui.StarLabel;
import org.tianocore.frameworkwizard.module.Identifications.ModuleIdentification;
import org.tianocore.frameworkwizard.packaging.PackageIdentification;
import org.tianocore.frameworkwizard.platform.PlatformIdentification;
import org.tianocore.frameworkwizard.workspace.Workspace;
import org.tianocore.frameworkwizard.workspace.WorkspaceTools;

public class SelectModuleBelong extends IDialog {

    /**
     Define class members
     
     **/
    private static final long serialVersionUID = 4171355136991448972L;

    private JPanel jContentPane = null;

    private JComboBox jComboBoxExistingPackage = null;

    private JButton jButtonOk = null;

    private JButton jButtonCancel = null;

    private Vector<PackageIdentification> packages = null;

    private JLabel jLabelPackage = null;

    private JLabel jLabelFilePath = null;

    private JTextField jTextFieldFilePath = null;

    private JButton jButtonBrowse = null;

    private JLabel jLabelName = null;

    private JTextField jTextFieldName = null;

    private JLabel jLabelGuid = null;

    private JTextField jTextFieldGuid = null;

    private JButton jButtonGen = null;

    private JLabel jLabelVersion = null;

    private JTextField jTextFieldVersion = null;

    private StarLabel jStarLabel1 = null;

    private StarLabel jStarLabel2 = null;

    private StarLabel jStarLabel3 = null;

    private StarLabel jStarLabel4 = null;

    private StarLabel jStarLabel5 = null;

    private WorkspaceTools wt = new WorkspaceTools();

    private ModuleIdentification mid = null;

    private PackageIdentification pid = null;

    private PlatformIdentification fid = null;

    private int mode = -1;

    /**
     * This method initializes jComboBoxExistingPackage	
     * 	
     * @return javax.swing.JComboBox	
     */
    private JComboBox getJComboBoxExistingPackage() {
        if (jComboBoxExistingPackage == null) {
            jComboBoxExistingPackage = new JComboBox();
            jComboBoxExistingPackage.setBounds(new java.awt.Rectangle(140, 10, 340, 20));
        }
        return jComboBoxExistingPackage;
    }

    /**
     * This method initializes jButtonNext	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonOk() {
        if (jButtonOk == null) {
            jButtonOk = new JButton();
            jButtonOk.setBounds(new java.awt.Rectangle(310, 140, 80, 20));
            jButtonOk.setText("Ok");
            jButtonOk.addActionListener(this);
        }
        return jButtonOk;
    }

    /**
     * This method initializes jButtonCancel	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonCancel() {
        if (jButtonCancel == null) {
            jButtonCancel = new JButton();
            jButtonCancel.setBounds(new java.awt.Rectangle(395, 140, 80, 20));
            jButtonCancel.setText("Cancel");
            jButtonCancel.addActionListener(this);
        }
        return jButtonCancel;
    }

    /**
     * This method initializes jTextFieldFilePath	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldFilePath() {
        if (jTextFieldFilePath == null) {
            jTextFieldFilePath = new JTextField();
            jTextFieldFilePath.setBounds(new java.awt.Rectangle(140, 35, 250, 20));
        }
        return jTextFieldFilePath;
    }

    /**
     * This method initializes jButtonBrowse	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonBrowse() {
        if (jButtonBrowse == null) {
            jButtonBrowse = new JButton();
            jButtonBrowse.setBounds(new java.awt.Rectangle(395, 35, 85, 20));
            jButtonBrowse.setText("Browse");
            jButtonBrowse.addActionListener(this);
        }
        return jButtonBrowse;
    }

    /**
     * This method initializes jTextFieldModuleName	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldName() {
        if (jTextFieldName == null) {
            jTextFieldName = new JTextField();
            jTextFieldName.setBounds(new java.awt.Rectangle(140, 60, 340, 20));
        }
        return jTextFieldName;
    }

    /**
     * This method initializes jTextFieldGuid	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldGuid() {
        if (jTextFieldGuid == null) {
            jTextFieldGuid = new JTextField();
            jTextFieldGuid.setBounds(new java.awt.Rectangle(140, 85, 250, 20));
        }
        return jTextFieldGuid;
    }

    /**
     * This method initializes jButtonGen	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonGen() {
        if (jButtonGen == null) {
            jButtonGen = new JButton();
            jButtonGen.setBounds(new java.awt.Rectangle(395, 85, 85, 20));
            jButtonGen.setText("GEN");
            jButtonGen.addActionListener(this);
        }
        return jButtonGen;
    }

    /**
     * This method initializes jTextFieldVersion	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldVersion() {
        if (jTextFieldVersion == null) {
            jTextFieldVersion = new JTextField();
            jTextFieldVersion.setBounds(new java.awt.Rectangle(140, 110, 340, 20));
        }
        return jTextFieldVersion;
    }

    /**
     * @param args
     */
    public static void main(String[] args) {
        SelectModuleBelong smb = new SelectModuleBelong();
        smb.setVisible(true);
    }

    /**
     * This is the default constructor
     */
    public SelectModuleBelong() {
        super();
        init();
    }

    /**
     * This is the default constructor
     */
    public SelectModuleBelong(IFrame parentFrame, boolean modal, int fileType) {
        super(parentFrame, modal);
        this.mode = fileType;
        init();
        initExistingPackage();
        if (mode != DataType.RETURN_TYPE_MODULE_SURFACE_AREA) {
            this.jStarLabel1.setVisible(false);
            this.jLabelPackage.setVisible(false);
            this.jComboBoxExistingPackage.setVisible(false);
        }
    }

    /**
     * This method initializes this
     * 
     * @return void
     */
    private void init() {
        this.setSize(500, 200);
        this.setContentPane(getJContentPane());
        this.setTitle("New Module");
        this.centerWindow();
    }

    /**
     * This method initializes jContentPane
     * 
     * @return javax.swing.JPanel
     */
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jLabelVersion = new JLabel();
            jLabelVersion.setBounds(new java.awt.Rectangle(15, 110, 120, 20));
            jLabelVersion.setText("Version");
            jLabelGuid = new JLabel();
            jLabelGuid.setBounds(new java.awt.Rectangle(15, 85, 120, 20));
            jLabelGuid.setText("Guid");
            jLabelName = new JLabel();
            jLabelName.setBounds(new java.awt.Rectangle(15, 60, 120, 20));
            jLabelName.setText("Name");
            jLabelFilePath = new JLabel();
            jLabelFilePath.setBounds(new java.awt.Rectangle(15, 35, 120, 20));
            jLabelFilePath.setText("File Path");
            jLabelPackage = new JLabel();
            jLabelPackage.setBounds(new java.awt.Rectangle(15, 10, 120, 20));
            jLabelPackage.setText("Choose Package");
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.setSize(new java.awt.Dimension(490, 175));
            jContentPane.add(getJComboBoxExistingPackage(), null);
            jContentPane.add(getJButtonOk(), null);
            jContentPane.add(getJButtonCancel(), null);
            jContentPane.add(jLabelPackage, null);
            jContentPane.add(jLabelFilePath, null);
            jContentPane.add(getJTextFieldFilePath(), null);
            jContentPane.add(getJButtonBrowse(), null);
            jContentPane.add(jLabelName, null);
            jContentPane.add(getJTextFieldName(), null);
            jContentPane.add(jLabelGuid, null);
            jContentPane.add(getJTextFieldGuid(), null);
            jContentPane.add(getJButtonGen(), null);
            jContentPane.add(jLabelVersion, null);
            jContentPane.add(getJTextFieldVersion(), null);

            jStarLabel1 = new StarLabel();
            jStarLabel1.setLocation(new java.awt.Point(0, 10));
            jStarLabel2 = new StarLabel();
            jStarLabel2.setLocation(new java.awt.Point(0, 35));
            jStarLabel3 = new StarLabel();
            jStarLabel3.setLocation(new java.awt.Point(0, 60));
            jStarLabel4 = new StarLabel();
            jStarLabel4.setLocation(new java.awt.Point(0, 85));
            jStarLabel5 = new StarLabel();
            jStarLabel5.setLocation(new java.awt.Point(0, 110));
            jContentPane.add(jStarLabel1, null);
            jContentPane.add(jStarLabel2, null);
            jContentPane.add(jStarLabel3, null);
            jContentPane.add(jStarLabel4, null);
            jContentPane.add(jStarLabel5, null);
        }
        return jContentPane;
    }

    /**
     Query all existing packages and fill them into combox
     
     **/
    private void initExistingPackage() {
        packages = wt.getAllPackages();
        for (int index = 0; index < packages.size(); index++) {
            this.jComboBoxExistingPackage.addItem(packages.elementAt(index).getName());
        }
    }

    /* (non-Javadoc)
     * @see java.awt.event.ActionListener#actionPerformed(java.awt.event.ActionEvent)
     * 
     * Override actionPerformed to listen all actions
     */
    public void actionPerformed(ActionEvent arg0) {
        if (arg0.getSource() == jButtonCancel) {
            this.setVisible(false);
            this.returnType = DataType.RETURN_TYPE_CANCEL;
        }

        if (arg0.getSource() == jButtonOk) {
            if (this.check()) {
                this.save();
            } else {
                return;
            }
            this.setVisible(false);
            this.returnType = DataType.RETURN_TYPE_OK;
        }

        if (arg0.getSource() == this.jButtonGen) {
            this.jTextFieldGuid.setText(Tools.generateUuidString());
        }

        if (arg0.getSource() == this.jButtonBrowse) {
            JFileChooser fc = new JFileChooser();
            fc.setAcceptAllFileFilterUsed(false);

            if (mode == DataType.RETURN_TYPE_MODULE_SURFACE_AREA) {
                fc.setCurrentDirectory(new File(packages.elementAt(this.jComboBoxExistingPackage.getSelectedIndex())
                                                        .getPath()));
                fc.addChoosableFileFilter(new IFileFilter(DataType.MODULE_SURFACE_AREA_EXT));
            }
            if (mode == DataType.RETURN_TYPE_PACKAGE_SURFACE_AREA) {
                fc.setCurrentDirectory(new File(Workspace.getCurrentWorkspace()));
                fc.addChoosableFileFilter(new IFileFilter(DataType.PACKAGE_SURFACE_AREA_EXT));
            }
            if (mode == DataType.RETURN_TYPE_PLATFORM_SURFACE_AREA) {
                fc.setCurrentDirectory(new File(Workspace.getCurrentWorkspace()));
                fc.addChoosableFileFilter(new IFileFilter(DataType.PLATFORM_SURFACE_AREA_EXT));
            }
            int result = fc.showSaveDialog(new JPanel());
            if (result == JFileChooser.APPROVE_OPTION) {
                this.jTextFieldFilePath.setText(Tools.addPathExt(fc.getSelectedFile().getPath(), mode));
            }
        }
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
        if (isEmpty(this.jTextFieldFilePath.getText())) {
            Log.err("File Path couldn't be empty");
            return false;
        }
        if (isEmpty(this.jTextFieldName.getText())) {
            Log.err("Name couldn't be empty");
            return false;
        }
        if (isEmpty(this.jTextFieldGuid.getText())) {
            Log.err("Guid Value couldn't be empty");
            return false;
        }
        if (isEmpty(this.jTextFieldVersion.getText())) {
            Log.err("Version couldn't be empty");
            return false;
        }

        //
        // Check if all fields have correct data types 
        //
        if (!DataValidation.isBaseName(this.jTextFieldName.getText())) {
            Log.err("Incorrect data type for Base Name");
            return false;
        }
        if (!DataValidation.isGuid((this.jTextFieldGuid).getText())) {
            Log.err("Incorrect data type for Guid");
            return false;
        }

        if (mode == DataType.RETURN_TYPE_MODULE_SURFACE_AREA) {
            //
            // Check if the module is already existed in current package
            //
            String packagePath = packages.elementAt(this.jComboBoxExistingPackage.getSelectedIndex()).getPath();
            String modulePath = Tools.convertPathToCurrentOsType(this.jTextFieldFilePath.getText());
            Vector<String> msaFile = wt.getAllModulesOfPackage(packagePath);

            for (int index = 0; index < msaFile.size(); index++) {
                if (msaFile.elementAt(index).equals(modulePath)) {
                    Log.err("This module is already existing in selected package");
                    return false;
                }
            }
        }

        if (mode == DataType.RETURN_TYPE_PACKAGE_SURFACE_AREA) {
            //
            // Check if the package is already existed in database
            //
            String path = Tools.convertPathToCurrentOsType(this.jTextFieldFilePath.getText());
            Vector<PackageIdentification> vPackageList = wt.getAllPackages();
            if (vPackageList != null && vPackageList.size() > 0) {
                for (int index = 0; index < vPackageList.size(); index++) {
                    if (vPackageList.get(index).getPath().equals(path)) {
                        Log.err("This package is already existing in database");
                        return false;
                    }
                }
            }
        }

        if (mode == DataType.RETURN_TYPE_PLATFORM_SURFACE_AREA) {
            //
            // Check if the platform is already existed in database
            //
            String path = Tools.convertPathToCurrentOsType(this.jTextFieldFilePath.getText());
            Vector<PlatformIdentification> vPlatfromList = wt.getAllPlatforms();
            if (vPlatfromList != null && vPlatfromList.size() > 0) {
                for (int index = 0; index < vPlatfromList.size(); index++) {
                    if (vPlatfromList.get(index).getPath().equals(path)) {
                        Log.err("This platform is already existing in database");
                        return false;
                    }
                }
            }
        }

        return true;
    }

    /**
     Save file
     
     **/
    public void save() {
        if (mode == DataType.RETURN_TYPE_MODULE_SURFACE_AREA) {
            this.saveModule();
        }
        if (mode == DataType.RETURN_TYPE_PACKAGE_SURFACE_AREA) {
            this.savePackage();
        }
        if (mode == DataType.RETURN_TYPE_PLATFORM_SURFACE_AREA) {
            this.savePlatform();
        }
    }

    /**
     Save all components of Msa Header
     
     **/
    private void saveModule() {
        ModuleSurfaceAreaDocument.ModuleSurfaceArea msa = null;
        String path = Tools.convertPathToCurrentOsType(this.jTextFieldFilePath.getText());

        //
        // Save to memory
        //
        try {
            MsaHeaderDocument.MsaHeader msaHeader = null;

            msa = ModuleSurfaceAreaDocument.ModuleSurfaceArea.Factory.newInstance();
            msaHeader = MsaHeaderDocument.MsaHeader.Factory.newInstance();

            msaHeader.setModuleName(this.jTextFieldName.getText());
            msaHeader.setGuidValue(this.jTextFieldGuid.getText());
            msaHeader.setVersion(this.jTextFieldVersion.getText());

            msa.setMsaHeader(msaHeader);
        } catch (Exception e) {
            Log.err("Save ModuleSurfaceArea Document", e.getMessage());
            return;
        }

        //
        // Save to real file
        //
        try {
            SaveFile.saveMsaFile(path, msa);

        } catch (Exception e) {
            Log.err("Save Module to file system", e.getMessage());
            return;
        }

        //
        // Save to identification
        //
        mid = new ModuleIdentification(this.jTextFieldName.getText(), this.jTextFieldGuid.getText(),
                                       this.jTextFieldVersion.getText(), path);
        mid.setPackageId(packages.elementAt(this.jComboBoxExistingPackage.getSelectedIndex()));
    }

    /**
     Save all components of Spd Header
     
     **/
    private void savePackage() {
        PackageSurfaceArea spd = null;
        String path = Tools.convertPathToCurrentOsType(this.jTextFieldFilePath.getText());

        //
        // Save to memory
        //
        try {
            SpdHeaderDocument.SpdHeader spdHeader = null;

            spd = PackageSurfaceArea.Factory.newInstance();
            spdHeader = SpdHeaderDocument.SpdHeader.Factory.newInstance();

            spdHeader.setPackageName(this.jTextFieldName.getText());
            spdHeader.setGuidValue(this.jTextFieldGuid.getText());
            spdHeader.setVersion(this.jTextFieldVersion.getText());

            spd.setSpdHeader(spdHeader);
        } catch (Exception e) {
            Log.err("Save PackageSurfaceArea Document", e.getMessage());
            return;
        }

        //
        // Save to real file
        //
        try {
            SaveFile.saveSpdFile(path, spd);

        } catch (Exception e) {
            Log.err("Save Package to file system", e.getMessage());
            return;
        }

        //
        // Save to identification
        //
        pid = new PackageIdentification(this.jTextFieldName.getText(), this.jTextFieldGuid.getText(),
                                        this.jTextFieldVersion.getText(), path);
    }

    /**
     Save all components of Fpd Header
     
     **/
    private void savePlatform() {
        PlatformSurfaceArea fpd = null;
        String path = Tools.convertPathToCurrentOsType(this.jTextFieldFilePath.getText());

        //
        // Save to memory
        //
        try {
            PlatformHeader fpdHeader = null;

            fpd = PlatformSurfaceArea.Factory.newInstance();
            fpdHeader = PlatformHeader.Factory.newInstance();

            fpdHeader.setPlatformName(this.jTextFieldName.getText());
            fpdHeader.setGuidValue(this.jTextFieldGuid.getText());
            fpdHeader.setVersion(this.jTextFieldVersion.getText());

            fpd.setPlatformHeader(fpdHeader);
        } catch (Exception e) {
            Log.err("Save FrameworkPlatformDescription Document", e.getMessage());
            return;
        }

        //
        // Save to real file
        //
        try {
            SaveFile.saveFpdFile(path, fpd);

        } catch (Exception e) {
            Log.err("Save Platform to file system", e.getMessage());
            return;
        }

        //
        // Save to identification
        //
        fid = new PlatformIdentification(this.jTextFieldName.getText(), this.jTextFieldGuid.getText(),
                                         this.jTextFieldVersion.getText(), path);
    }

    public ModuleIdentification getMid() {
        return mid;
    }

    public void setMid(ModuleIdentification mid) {
        this.mid = mid;
    }

    public PlatformIdentification getFid() {
        return fid;
    }

    public void setFid(PlatformIdentification fid) {
        this.fid = fid;
    }

    public PackageIdentification getPid() {
        return pid;
    }

    public void setPid(PackageIdentification pid) {
        this.pid = pid;
    }
}
