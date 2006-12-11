/** @file
 
 The file is used to clone workspace, module, package and platform
 
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
import java.io.IOException;
import java.math.BigInteger;
import java.util.Vector;

import javax.swing.JFileChooser;
import javax.swing.JPanel;
import javax.swing.JLabel;
import javax.swing.JTextField;
import javax.swing.JButton;

import org.apache.xmlbeans.XmlException;
import org.tianocore.ModuleDefinitionsDocument.ModuleDefinitions;
import org.tianocore.ModuleSurfaceAreaDocument.ModuleSurfaceArea;
import org.tianocore.PackageDefinitionsDocument.PackageDefinitions;
import org.tianocore.PackageSurfaceAreaDocument.PackageSurfaceArea;
import org.tianocore.PlatformDefinitionsDocument.PlatformDefinitions;
import org.tianocore.PlatformSurfaceAreaDocument.PlatformSurfaceArea;
import org.tianocore.frameworkwizard.common.DataType;
import org.tianocore.frameworkwizard.common.DataValidation;
import org.tianocore.frameworkwizard.common.FileOperation;
import org.tianocore.frameworkwizard.common.GlobalData;
import org.tianocore.frameworkwizard.common.IFileFilter;
import org.tianocore.frameworkwizard.common.Log;
import org.tianocore.frameworkwizard.common.OpenFile;
import org.tianocore.frameworkwizard.common.SaveFile;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.Identifications.Identification;
import org.tianocore.frameworkwizard.common.ui.IDialog;
import org.tianocore.frameworkwizard.common.ui.IFrame;
import org.tianocore.frameworkwizard.module.Identifications.ModuleIdentification;
import org.tianocore.frameworkwizard.packaging.PackageIdentification;
import org.tianocore.frameworkwizard.platform.PlatformIdentification;
import org.tianocore.frameworkwizard.workspace.Workspace;
import org.tianocore.frameworkwizard.workspace.WorkspaceTools;
import javax.swing.JComboBox;

/**
 The class is used to provide functions to clone workspace, module, package and platform
 It extends IDialog

 **/
public class Clone extends IDialog {

    ///
    /// Define Class Serial Version UID
    ///
    private static final long serialVersionUID = -5469299324965727137L;

    ///
    /// Define Class Members
    ///
    private JPanel jContentPane = null;

    private JLabel jLabelType = null;

    private JTextField jTextFieldType = null;

    private JLabel jLabelSource = null;

    private JTextField jTextFieldSource = null;

    private JButton jButtonBrowse = null;

    private JLabel jLabelDestinationFile = null;

    private JTextField jTextFieldFilePath = null;

    private JLabel jLabelBaseName = null;

    private JTextField jTextFieldBaseName = null;

    private JLabel jLabelGuid = null;

    private JTextField jTextFieldGuid = null;

    private JLabel jLabelVersion = null;

    private JTextField jTextFieldVersion = null;

    private JButton jButtonOk = null;

    private JButton jButtonCancel = null;

    private JButton jButtonGenerateGuid = null;

    private JLabel jLabelBelong = null;

    private JComboBox jComboBoxExistingPackage = null;

    ///
    /// Define members not for UI
    ///

    private int mode = -1;

    private Vector<PackageIdentification> packages = null;

    private WorkspaceTools wt = new WorkspaceTools();

    private Identification oldId = null;

    private Identification newId = null;

    private ModuleIdentification mid = null;

    private PackageIdentification pid = null;

    private PlatformIdentification fid = null;

    /**
     This method initializes jTextFieldType	
     
     @return javax.swing.JTextField
     
     **/
    private JTextField getJTextFieldType() {
        if (jTextFieldType == null) {
            jTextFieldType = new JTextField();
            jTextFieldType.setBounds(new java.awt.Rectangle(210, 10, 320, 20));
            jTextFieldType.setEditable(false);
        }
        return jTextFieldType;
    }

    /**
     This method initializes jTextFieldSource	
     
     @return javax.swing.JTextField	
     
     **/
    private JTextField getJTextFieldSource() {
        if (jTextFieldSource == null) {
            jTextFieldSource = new JTextField();
            jTextFieldSource.setBounds(new java.awt.Rectangle(210, 35, 320, 20));
            jTextFieldSource.setEditable(false);
        }
        return jTextFieldSource;
    }

    /**
     This method initializes jButtonBrowse	
     
     @return javax.swing.JButton	
     
     **/
    private JButton getJButtonBrowse() {
        if (jButtonBrowse == null) {
            jButtonBrowse = new JButton();
            jButtonBrowse.setBounds(new java.awt.Rectangle(445, 85, 85, 20));
            jButtonBrowse.setText("Browse");
            jButtonBrowse.addActionListener(this);
        }
        return jButtonBrowse;
    }

    /**
     This method initializes jTextFieldDestinationFile	
     
     @return javax.swing.JTextField	
     
     **/
    private JTextField getJTextFieldFilePath() {
        if (jTextFieldFilePath == null) {
            jTextFieldFilePath = new JTextField();
            jTextFieldFilePath.setBounds(new java.awt.Rectangle(210, 85, 230, 20));
        }
        return jTextFieldFilePath;
    }

    /**
     This method initializes jTextFieldBaseName	
     
     @return javax.swing.JTextField	
     
     **/
    private JTextField getJTextFieldBaseName() {
        if (jTextFieldBaseName == null) {
            jTextFieldBaseName = new JTextField();
            jTextFieldBaseName.setBounds(new java.awt.Rectangle(210, 110, 320, 20));
        }
        return jTextFieldBaseName;
    }

    /**
     This method initializes jTextFieldGuid	
     
     @return javax.swing.JTextField	
     
     **/
    private JTextField getJTextFieldGuid() {
        if (jTextFieldGuid == null) {
            jTextFieldGuid = new JTextField();
            jTextFieldGuid.setBounds(new java.awt.Rectangle(210, 135, 230, 20));
        }
        return jTextFieldGuid;
    }

    /**
     This method initializes jTextFieldVersion	
     
     @return javax.swing.JTextField	
     
     **/
    private JTextField getJTextFieldVersion() {
        if (jTextFieldVersion == null) {
            jTextFieldVersion = new JTextField();
            jTextFieldVersion.setBounds(new java.awt.Rectangle(210, 160, 320, 20));
        }
        return jTextFieldVersion;
    }

    /**
     This method initializes jButtonOk	
     
     @return javax.swing.JButton	
     
     **/
    private JButton getJButtonOk() {
        if (jButtonOk == null) {
            jButtonOk = new JButton();
            jButtonOk.setBounds(new java.awt.Rectangle(285, 200, 90, 20));
            jButtonOk.setText("Ok");
            jButtonOk.addActionListener(this);
        }
        return jButtonOk;
    }

    /**
     This method initializes jButtonCancel	
     
     @return javax.swing.JButton	
     
     **/
    private JButton getJButtonCancel() {
        if (jButtonCancel == null) {
            jButtonCancel = new JButton();
            jButtonCancel.setBounds(new java.awt.Rectangle(405, 200, 90, 20));
            jButtonCancel.setText("Cancel");
            jButtonCancel.addActionListener(this);
        }
        return jButtonCancel;
    }

    /**
     This method initializes jButtonGenerateGuid	
     
     @return javax.swing.JButton	
     
     **/
    private JButton getJButtonGenerateGuid() {
        if (jButtonGenerateGuid == null) {
            jButtonGenerateGuid = new JButton();
            jButtonGenerateGuid.setBounds(new java.awt.Rectangle(445, 135, 85, 20));
            jButtonGenerateGuid.setText("Gen");
            jButtonGenerateGuid.addActionListener(this);
        }
        return jButtonGenerateGuid;
    }

    /**
     This method initializes jComboBoxExistingPackage	
     
     @return javax.swing.JComboBox	
     
     **/
    private JComboBox getJComboBoxExistingPackage() {
        if (jComboBoxExistingPackage == null) {
            jComboBoxExistingPackage = new JComboBox();
            jComboBoxExistingPackage.setBounds(new java.awt.Rectangle(210, 60, 320, 20));
        }
        return jComboBoxExistingPackage;
    }

    /**
     This is the default constructor
     
     **/
    public Clone() {
        super();
        init();
    }

    /**
     This is the override constructor
     
     @param parentFrame       The parent frame which starts this frame
     @param modal             To identify the frame's modal
     @param fileType          To identify the clone target type
     @param identification    The clone target's identification
     
     **/
    public Clone(IFrame parentFrame, boolean modal, int fileType, Identification identification) {
        super(parentFrame, modal);
        this.mode = fileType;
        if (identification != null) {
            this.oldId = new Identification(identification.getName(), identification.getGuid(),
                                            identification.getVersion(), identification.getPath());
            this.newId = new Identification(identification.getName(), identification.getGuid(),
                                            identification.getVersion(), identification.getPath());
        }
        init(mode);
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

    /**
     This method initializes this
     
     **/
    private void init() {
        this.setSize(550, 260);
        this.setContentPane(getJContentPane());
        this.setTitle("Clone");
        this.centerWindow();
    }

    /**
     This method initializes this with given clone target type.
     Customize the frame interface via different clone target type.
     
     @param mode To identify the clone target type
     
     **/
    private void init(int mode) {
        init();
        //
        // For MODULE_SURFACE_AREA
        //
        if (mode == DataType.RETURN_TYPE_MODULE_SURFACE_AREA) {
            this.jTextFieldType.setText(DataType.MODULE_SURFACE_AREA);
            String s = oldId.getPath();
            s = Tools.getRelativePath(s, Tools.getFilePathOnly(wt.getPackageIdByModuleId(oldId).getPath()));
            this.jTextFieldSource.setText(Tools.convertPathToCurrentOsType(s));
            initExistingPackage();
            this.jButtonBrowse.setVisible(false);
            this.jTextFieldFilePath
                                   .setToolTipText("<html>Input the module's relative path and filename, for example:<br>Application\\HelloWorld\\HelloWorld.msa</html>");
            this.jTextFieldFilePath.setSize(320, this.jTextFieldFilePath.getSize().height);
            this.jLabelDestinationFile.setText("New Module Path and Filename");
        }
        //
        // For PACKAGE_SURFACE_AREA
        //
        if (mode == DataType.RETURN_TYPE_PACKAGE_SURFACE_AREA) {
            this.jTextFieldType.setText(DataType.PACKAGE_SURFACE_AREA);
            String s = oldId.getPath();
            s = Tools.getRelativePath(oldId.getPath(), Workspace.getCurrentWorkspace());
            this.jTextFieldSource.setText(Tools.convertPathToCurrentOsType(s));
            this.jLabelBelong.setEnabled(false);
            this.jComboBoxExistingPackage.setEnabled(false);
            this.jButtonBrowse.setVisible(false);
            this.jTextFieldFilePath
                                   .setToolTipText("<html>Input the package's relative path and file name, for example:<br>MdePkg\\MdePkg.spd</html>");
            this.jTextFieldFilePath.setSize(320, this.jTextFieldFilePath.getSize().height);
            this.jLabelDestinationFile.setText("New Package Path and Filename");

            //
            // Check if the package can be cloned
            //
            PackageSurfaceArea spd = GlobalData.openingPackageList
                                                                  .getPackageSurfaceAreaFromId(GlobalData.openingPackageList
                                                                                                                            .getIdByPath(this.oldId
                                                                                                                                                   .getPath()));
            if (spd != null) {
                if (spd.getPackageDefinitions() != null) {
                    if (!spd.getPackageDefinitions().getRePackage()) {
                        Log.wrn("Clone Package", "This package can't repackaged and cloned");
                        this.jTextFieldBaseName.setEnabled(false);
                        this.jTextFieldFilePath.setEnabled(false);
                        this.jTextFieldGuid.setEnabled(false);
                        this.jTextFieldVersion.setEnabled(false);
                        this.jButtonGenerateGuid.setEnabled(false);
                        this.jButtonOk.setEnabled(false);
                    }
                }
            }
        }
        //
        // For PLATFORM_SURFACE_AREA
        //
        if (mode == DataType.RETURN_TYPE_PLATFORM_SURFACE_AREA) {
            this.jTextFieldType.setText(DataType.PLATFORM_SURFACE_AREA);
            this.jTextFieldSource.setText(oldId.getPath());
            this.jLabelBelong.setEnabled(false);
            this.jComboBoxExistingPackage.setEnabled(false);
            this.jTextFieldFilePath
                                   .setToolTipText("<html>Select the platform's relative path and filename. For example:<br>C:\\MyWorkspace\\EdkNt32Pkg\\Nt32.fpd</html>");
            this.jLabelDestinationFile.setText("New Platform Path and Filename");
        }
        //
        // For WORKSPACE
        //
        if (mode == DataType.RETURN_TYPE_WORKSPACE) {
            this.jTextFieldType.setText(DataType.WORKSPACE);
            this.jTextFieldSource.setText(Workspace.getCurrentWorkspace());
            this.jLabelBelong.setEnabled(false);
            this.jComboBoxExistingPackage.setEnabled(false);
            this.jLabelBaseName.setEnabled(false);
            this.jTextFieldBaseName.setEditable(false);
            this.jLabelGuid.setEnabled(false);
            this.jTextFieldGuid.setEnabled(false);
            this.jButtonGenerateGuid.setEnabled(false);
            this.jLabelVersion.setEnabled(false);
            this.jTextFieldVersion.setEnabled(false);
            this.jTextFieldFilePath
                                   .setToolTipText("<html>Input the workspace path, for example:<br>C:\\MyWorkspace</html>");
            this.jLabelDestinationFile.setText("New Workspace Path");
        }
    }

    /**
     This method initializes jContentPane
     
     @return javax.swing.JPanel
     
     **/
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jLabelBelong = new JLabel();
            jLabelBelong.setBounds(new java.awt.Rectangle(15, 60, 190, 20));
            jLabelBelong.setText("Clone Package");
            jLabelVersion = new JLabel();
            jLabelVersion.setBounds(new java.awt.Rectangle(15, 160, 190, 20));
            jLabelVersion.setText("Version");
            jLabelGuid = new JLabel();
            jLabelGuid.setBounds(new java.awt.Rectangle(15, 135, 190, 20));
            jLabelGuid.setText("Guid");
            jLabelBaseName = new JLabel();
            jLabelBaseName.setBounds(new java.awt.Rectangle(15, 110, 190, 20));
            jLabelBaseName.setText("Base Name");
            jLabelDestinationFile = new JLabel();
            jLabelDestinationFile.setBounds(new java.awt.Rectangle(15, 85, 190, 20));
            jLabelDestinationFile.setText("Destination File Name");
            jLabelSource = new JLabel();
            jLabelSource.setBounds(new java.awt.Rectangle(15, 35, 190, 20));
            jLabelSource.setText("Source");
            jLabelType = new JLabel();
            jLabelType.setBounds(new java.awt.Rectangle(15, 10, 190, 20));
            jLabelType.setText("Type");
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.setSize(new java.awt.Dimension(540, 227));
            jContentPane.add(jLabelType, null);
            jContentPane.add(getJTextFieldType(), null);
            jContentPane.add(jLabelSource, null);
            jContentPane.add(getJTextFieldSource(), null);
            jContentPane.add(jLabelDestinationFile, null);
            jContentPane.add(getJTextFieldFilePath(), null);
            jContentPane.add(jLabelBaseName, null);
            jContentPane.add(getJTextFieldBaseName(), null);
            jContentPane.add(jLabelGuid, null);
            jContentPane.add(getJTextFieldGuid(), null);
            jContentPane.add(jLabelVersion, null);
            jContentPane.add(getJTextFieldVersion(), null);
            jContentPane.add(getJButtonOk(), null);
            jContentPane.add(getJButtonCancel(), null);
            jContentPane.add(getJButtonBrowse(), null);
            jContentPane.add(getJButtonGenerateGuid(), null);
            jContentPane.add(jLabelBelong, null);
            jContentPane.add(getJComboBoxExistingPackage(), null);
        }
        return jContentPane;
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
                try {
                    //
                    // Save to file
                    //
                    this.save();
                } catch (IOException e) {
                    Log.wrn("Clone", e.getMessage());
                    Log.err("Clone", e.getMessage());
                    return;
                } catch (XmlException e) {
                    Log.wrn("Clone", e.getMessage());
                    Log.err("Clone", e.getMessage());
                    return;
                } catch (Exception e) {
                    Log.wrn("Clone", e.getMessage());
                    Log.err("Clone", e.getMessage());
                    return;
                }
            } else {
                return;
            }
            this.setVisible(false);
        }

        if (arg0.getSource() == this.jButtonGenerateGuid) {
            this.jTextFieldGuid.setText(Tools.generateUuidString());
        }

        //
        // Use different file ext for different clone target type
        //
        if (arg0.getSource() == this.jButtonBrowse) {
            JFileChooser fc = new JFileChooser();
            fc.setAcceptAllFileFilterUsed(false);

            if (mode == DataType.RETURN_TYPE_WORKSPACE) {
                fc.setCurrentDirectory(new File(Workspace.getCurrentWorkspace()));
                fc.setFileSelectionMode(JFileChooser.DIRECTORIES_ONLY);
            }
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
     Check name, guid and version.
     If all of them are valid, save information to new id
     
     @retval true   All name, guid and version are valid
     @retval false  Any one of name, guid and version is invalid
     
     **/
    private boolean checkId(int mode) {
        String name = this.jTextFieldBaseName.getText();
        String guid = this.jTextFieldGuid.getText();
        String version = this.jTextFieldVersion.getText();
        
        //
        // Check Basename
        //
        if (isEmpty(name)) {
            Log.wrn("Clone", "The Name is required!");
            return false;
        }
        if (!DataValidation.isBaseName(name)) {
            Log
               .wrn("Clone",
                    "<html>Incorrect data type for the Name, it must<br>be a single word, starting with an alpha character.</html>");
            return false;
        }

        //
        // Check Guid
        //
        if (isEmpty(guid)) {
            Log.wrn("Clone", "A Guid is required!!");
            return false;
        }
        if (!DataValidation.isGuid(guid)) {
            Log
               .wrn(
                    "Clone",
                    "<html>Incorrect data type for Guid, which must<br>be in registry format (8-4-4-4-12) for example:<br>d3adb123-eef1-466d-39ac-02febcaf5997</html>");
            return false;
        }

        //
        // Check Version
        //
        if (isEmpty(version)) {
            Log.wrn("Clone", "A Version must be entered!");
            return false;
        }
        if (!DataValidation.isVersion(version)) {
            Log
               .wrn(
                    "Clone",
                    "<html>Incorrect data type for Version, which must<br>be one or more digits, optionally followed by sequence<br>of one or more dot,  one or more digits; examples:<br>1.0 1.0.1 12.25.256</html>");
            return false;
        }
        
        if (mode == DataType.RETURN_TYPE_MODULE_SURFACE_AREA) {
            String packageGuid = packages.elementAt(this.jComboBoxExistingPackage.getSelectedIndex()).getGuid();
            String packageVersion = packages.elementAt(this.jComboBoxExistingPackage.getSelectedIndex()).getVersion();
            if (GlobalData.findModuleId(guid, version, packageGuid, packageVersion) != null) {
                Log.wrn("Clone", "A module with same Guid and same Version already exists, please selece a new Guid or Version!");
                return false;
            }
        }

        if (mode == DataType.RETURN_TYPE_PACKAGE_SURFACE_AREA) {
            if (GlobalData.findPackageId(guid, version) != null) {
                Log.wrn("Clone", "A package with same Guid and same Version already exists, please selece a new Guid or Version!");
                return false;
            }
        }

        if (mode == DataType.RETURN_TYPE_PLATFORM_SURFACE_AREA) {
            if (GlobalData.findPlatformId(guid, version) != null) {
                Log.wrn("Clone", "A platform with same Guid and same Version already exists, please selece a new Guid or Version!");
                return false;
            }
        }

        //
        // Save information to id
        //
        newId.setName(this.jTextFieldBaseName.getText());
        newId.setGuid(this.jTextFieldGuid.getText());
        newId.setVersion(this.jTextFieldVersion.getText());
        newId.setPath(this.jTextFieldFilePath.getText());

        return true;
    }

    /**
     Check before save
     
     @retval true   All check points are passed
     @retval false  Any one of check points is failed
     
     **/
    private boolean check() {
        String src = this.oldId.getPath();
        String trg = this.jTextFieldFilePath.getText();
        File srcFile = new File(src);
        File trgFile = new File(trg);

        //
        // Common Check
        //
        if (!srcFile.exists()) {
            Log.wrn("Clone", "The source file does not exist!");
            return false;
        }
        if (isEmpty(trg)) {
            Log.wrn("Clone", "The destination file path must be entered!");
            return false;
        }
        if (src.equals(trg)) {
            Log.wrn("Clone", "The source and destination can not be same!");
            return false;
        }
        if (trgFile.exists()) {
            Log.wrn("Clone", "The destination already exists!");
            return false;
        }

        //
        // Check for workspace
        //
        if (mode == DataType.RETURN_TYPE_WORKSPACE) {
            if (trg.indexOf(src + DataType.FILE_SEPARATOR) == 0) {
                Log.wrn("Clone", "The new workspace can not be located within the current workspace!");
                return false;
            }
        }

        //
        // Check for Module
        //
        if (mode == DataType.RETURN_TYPE_MODULE_SURFACE_AREA) {
            trg = this.getModulePath();
            if (Tools.getFilePathOnly(src).equals(Tools.getFilePathOnly(trg))) {
                Log.wrn("Clone", "The source and destination paths for cloning a module must be different!");
                return false;
            }
            trgFile = new File(trg);
            if (trgFile.exists()) {
                Log.wrn("Clone", "The target module already exists!");
                return false;
            }
            
            //
            // Check if path already exists
            // Currently we allow user to add multiple msa files in one same directory
            // Remove this checkpoint
            //
//            if (GlobalData.isDuplicateRelativePath(Tools.getFilePathOnly(trg), mode)) {
//                Log.wrn("Clone", "There already exists a same directory with a module");
//                return false;
//            }
            
            return checkId(mode);
        }

        //
        // Check for Package
        //
        if (mode == DataType.RETURN_TYPE_PACKAGE_SURFACE_AREA) {
            if (trg.indexOf(DataType.DOS_FILE_SEPARATOR) == -1 && trg.indexOf(DataType.UNIX_FILE_SEPARATOR) == -1) {
                Log.wrn("Clone", "The package name must include a path!");
                return false;
            }
            trg = this.getPackagePath();
            if (Tools.getFilePathOnly(src).equals(Tools.getFilePathOnly(trg))) {
                Log.wrn("Clone", "The source and destination paths for cloning a package must be different!");
                return false;
            }
            trgFile = new File(trg);
            if (trgFile.exists()) {
                Log.wrn("Clone", "The target package already exists!");
                return false;
            }
            if (GlobalData.isDuplicateRelativePath(Tools.getFilePathOnly(trg), mode)) {
                Log.wrn("Clone", "There already exists a same directory with a package");
                return false;
            }
            
            return checkId(mode);
        }

        //
        // Check for Platform
        //
        if (mode == DataType.RETURN_TYPE_PLATFORM_SURFACE_AREA) {
            if (trg.indexOf(Workspace.getCurrentWorkspace()) != 0) {
                Log.wrn("Clone", "The platform clone must be located in the current workspace!");
                return false;
            }
            if (Tools.getFilePathOnly(src).equals(Tools.getFilePathOnly(trg))) {
                Log.wrn("Clone", "The source and destination paths for cloning a platform must be different!");
                return false;
            }
            trgFile = new File(trg);
            if (trgFile.exists()) {
                Log.wrn("Clone", "The target platform already exists.");
                return false;
            }
            if (GlobalData.isDuplicateRelativePath(Tools.getFilePathOnly(trg), mode)) {
                Log.wrn("Clone", "There already exists a same directory with a platform");
                return false;
            }
            
            return checkId(mode);
        }

        return true;
    }

    /**
     Save clone target to new location
     
     @throws IOException
     @throws XmlException
     @throws Exception
     
     **/
    private void save() throws IOException, XmlException, Exception {
        String src = this.oldId.getPath();
        String trg = this.jTextFieldFilePath.getText();
        Vector<String> vFiles = new Vector<String>();

        //
        // Clone Workspace
        //
        if (mode == DataType.RETURN_TYPE_WORKSPACE) {
            FileOperation.copyFolder(src, trg);
            this.returnType = DataType.RETURN_TYPE_WORKSPACE;
        }

        //
        // Clone Module Surface Area
        //
        if (mode == DataType.RETURN_TYPE_MODULE_SURFACE_AREA) {
            //
            // Get target path from source path
            //
            trg = getModulePath();
            newId.setPath(trg);
            vFiles = wt.getAllFilesPathOfModule(src);

            String oldPackagePath = GlobalData.openingModuleList.getIdByPath(src).getPackageId().getPath();
            String newPackagePath = packages.elementAt(this.jComboBoxExistingPackage.getSelectedIndex()).getPath();

            //
            // First copy all files to new directory
            //
            FileOperation.copyFile(src, trg);
            for (int index = 1; index < vFiles.size(); index++) {
                String oldFile = vFiles.get(index);
                String newFile = "";
                if (oldFile.indexOf(Tools.getFilePathOnly(src)) > -1) {
                    //
                    // The file is not include header
                    //
                    newFile = oldFile.replace(Tools.getFilePathOnly(src), Tools.getFilePathOnly(trg));
                } else if (oldFile.indexOf(Tools.getFilePathOnly(oldPackagePath)) > -1) {
                    //
                    // The file is include header
                    //
                    newFile = oldFile.replace(Tools.getFilePathOnly(oldPackagePath),
                                              Tools.getFilePathOnly(newPackagePath));
                }

                FileOperation.copyFile(oldFile, newFile);
            }

            //
            // Create new msa file
            //
            ModuleSurfaceArea msa = null;
            msa = OpenFile.openMsaFile(src);

            //
            // Update to memory
            //
            msa.getMsaHeader().setModuleName(newId.getName());
            msa.getMsaHeader().setGuidValue(newId.getGuid());
            msa.getMsaHeader().setVersion(newId.getVersion());

            //
            // Update <Cloned> Section
            //
            updateModuleClonedId(msa, oldId);

            //
            // Save to file
            //
            SaveFile.saveMsaFile(trg, msa);

            //
            // Update to platformId
            //
            this.setMid(new ModuleIdentification(newId,
                                                 packages.elementAt(this.jComboBoxExistingPackage.getSelectedIndex())));

            //
            // Open belonging package
            //
            PackageSurfaceArea psa = PackageSurfaceArea.Factory.newInstance();
            psa = OpenFile.openSpdFile(mid.getPackageId().getPath());

            //
            // Update the db file
            //
            wt.addModuleToPackage(mid, psa);

            //
            // Update GlobalData
            //
            GlobalData.vModuleList.addElement(mid);
            GlobalData.openingModuleList.insertToOpeningModuleList(mid, msa);

            this.returnType = DataType.RETURN_TYPE_MODULE_SURFACE_AREA;
        }

        //
        // Clone Package Surface Area
        //
        if (mode == DataType.RETURN_TYPE_PACKAGE_SURFACE_AREA) {
            //
            // Get target path from source path
            //
            trg = this.getPackagePath();
            newId.setPath(trg);
            vFiles = wt.getAllFilesPathOfPakcage(src);

            //
            // First copy all files to new directory
            //
            FileOperation.copyFile(src, trg);
            for (int index = 1; index < vFiles.size(); index++) {
                String oldFile = vFiles.get(index);
                String newFile = vFiles.get(index).replace(Tools.getFilePathOnly(src), Tools.getFilePathOnly(trg));
                FileOperation.copyFile(oldFile, newFile);
            }

            //
            // Create new spd file
            //
            PackageSurfaceArea spd = null;
            spd = OpenFile.openSpdFile(src);

            //
            // Update to memory
            //
            spd.getSpdHeader().setPackageName(newId.getName());
            spd.getSpdHeader().setGuidValue(newId.getGuid());
            spd.getSpdHeader().setVersion(newId.getVersion());

            //
            // Update <Cloned> Section
            //
            updatePackageClonedId(spd, oldId);

            //
            // Save to file
            //
            SaveFile.saveSpdFile(trg, spd);

            //
            // Update to platformId
            //
            this.setPid(new PackageIdentification(newId));

            //
            // Update the db file
            //
            wt.addPackageToDatabase(pid);

            //
            // Update GlobalData
            //
            GlobalData.vPackageList.addElement(pid);
            GlobalData.openingPackageList.insertToOpeningPackageList(pid, spd);

            //
            // Add all cloned modules
            //
            Vector<String> modulePaths = GlobalData.getAllModulesOfPackage(pid.getPath());
            String modulePath = null;
            ModuleSurfaceArea msa = null;

            for (int indexJ = 0; indexJ < modulePaths.size(); indexJ++) {
                try {
                    modulePath = modulePaths.get(indexJ);
                    msa = OpenFile.openMsaFile(modulePath);
                } catch (IOException e) {
                    Log.err("Open Module Surface Area " + modulePath, e.getMessage());
                    continue;
                } catch (XmlException e) {
                    Log.err("Open Module Surface Area " + modulePath, e.getMessage());
                    continue;
                } catch (Exception e) {
                    Log.err("Open Module Surface Area " + modulePath, "Invalid file type");
                    continue;
                }
                Identification id = Tools.getId(modulePath, msa);
                mid = new ModuleIdentification(id, pid);
                GlobalData.vModuleList.addElement(mid);
                GlobalData.openingModuleList.insertToOpeningModuleList(mid, msa);
            }

            this.returnType = DataType.RETURN_TYPE_PACKAGE_SURFACE_AREA;
        }

        //
        // Clone Platform Surface Area
        //
        if (mode == DataType.RETURN_TYPE_PLATFORM_SURFACE_AREA) {
            PlatformSurfaceArea fpd = null;
            fpd = OpenFile.openFpdFile(src);

            //
            // Update to memory
            //
            fpd.getPlatformHeader().setPlatformName(newId.getName());
            fpd.getPlatformHeader().setGuidValue(newId.getGuid());
            fpd.getPlatformHeader().setVersion(newId.getVersion());

            //
            // Update Cloned From element
            //
            updatePlatformClonedId(fpd, oldId);

            //
            // Save to file
            //
            SaveFile.saveFpdFile(trg, fpd);

            //
            // Update to platformId
            //
            this.setFid(new PlatformIdentification(newId));

            //
            // Update the db file
            //
            wt.addPlatformToDatabase(fid);

            //
            // Update GlobalData
            //
            GlobalData.vPlatformList.addElement(fid);
            GlobalData.openingPlatformList.insertToOpeningPlatformList(fid, fpd);

            this.returnType = DataType.RETURN_TYPE_PLATFORM_SURFACE_AREA;
        }
        vFiles = null;
    }

    /**
     Get the path of selected package
     
     @return String The path of selected package
     
     **/
    private String getSelectPackagePath() {
        return Tools.getFilePathOnly(packages.elementAt(this.jComboBoxExistingPackage.getSelectedIndex()).getPath());
    }

    /**
     Get the path of source module
     Since the path of source module is relative, make it up to full path.
     
     @return String The full path of source module
     
     **/
    private String getModulePath() {
        String trg = this.jTextFieldFilePath.getText();
        trg = Tools.addPathExt(trg, mode);
        trg = Tools.addFileSeparator(getSelectPackagePath()) + trg;
        Tools.convertPathToCurrentOsType(trg);
        return trg;
    }

    /**
     Get the path of source package
     Since the path of source package is relative, make it up to full path.
     
     @return String The full path of source package
     
     **/
    private String getPackagePath() {
        String trg = this.jTextFieldFilePath.getText();
        trg = Tools.addPathExt(trg, mode);
        trg = Tools.addFileSeparator(Workspace.getCurrentWorkspace()) + trg;
        trg = Tools.convertPathToCurrentOsType(trg);
        return trg;
    }

    /**
     Set msa file's <Cloned> section via given identification
     
     @param msa ModuleSurfaceArea for clone target
     @param id Identification of clone source
     
     **/
    private void updateModuleClonedId(ModuleSurfaceArea msa, Identification id) {
        //
        // Get PlatformDefinitions First
        //
        ModuleDefinitions pd = null;
        if (msa.getModuleDefinitions() == null) {
            pd = ModuleDefinitions.Factory.newInstance();
            msa.addNewModuleDefinitions();
        } else {
            pd = msa.getModuleDefinitions();
        }

        //
        // Get ClonedFrom then
        //
        ModuleDefinitions.ClonedFrom cf = null;
        BigInteger count = new BigInteger("-1");
        if (pd.getClonedFrom() == null) {
            cf = ModuleDefinitions.ClonedFrom.Factory.newInstance();
        } else {
            cf = pd.getClonedFrom();
            if (cf != null) {
                for (int index = 0; index < cf.getClonedList().size(); index++) {
                    if (cf.getClonedList().get(index).getId() != null) {
                        count = count.max(cf.getClonedList().get(index).getId());
                    }
                }
            }
        }

        //
        // Set new Cloned item
        //
        ModuleDefinitions.ClonedFrom.Cloned c = ModuleDefinitions.ClonedFrom.Cloned.Factory.newInstance();
        c.setModuleGuid(id.getGuid());
        c.setModuleVersion(id.getVersion());
        c.setPackageGuid(wt.getPackageIdByModuleId(oldId).getGuid());
        c.setPackageVersion(wt.getPackageIdByModuleId(oldId).getVersion());
        c.setId(count.add(new BigInteger("1")));
        String guid = wt.getModuleFarGuid(oldId);
        if (guid != null && !guid.equals("")) {
            c.setFarGuid(guid);
        }

        cf.addNewCloned();
        cf.setClonedArray(cf.getClonedList().size() - 1, c);
        pd.addNewClonedFrom();
        pd.setClonedFrom(cf);
        msa.setModuleDefinitions(pd);
    }

    /**
     Set spd file's <Cloned> section via given identification
     
     @param spd PackageSurfaceArea for clone target
     @param id Identification of clone source
     
     **/
    private void updatePackageClonedId(PackageSurfaceArea spd, Identification id) {
        //
        // Get PlatformDefinitions First
        //
        PackageDefinitions pd = null;
        if (spd.getPackageDefinitions() == null) {
            pd = PackageDefinitions.Factory.newInstance();
            spd.addNewPackageDefinitions();
        } else {
            pd = spd.getPackageDefinitions();
        }

        //
        // Get ClonedFrom then
        //
        PackageDefinitions.ClonedFrom cf = null;
        BigInteger count = new BigInteger("-1");
        if (pd.getClonedFrom() == null) {
            cf = PackageDefinitions.ClonedFrom.Factory.newInstance();
        } else {
            cf = pd.getClonedFrom();
            if (cf != null) {
                for (int index = 0; index < cf.getClonedList().size(); index++) {
                    if (cf.getClonedList().get(index).getId() != null) {
                        count = count.max(cf.getClonedList().get(index).getId());
                    }
                }
            }
        }

        //
        // Set new Cloned item
        //
        PackageDefinitions.ClonedFrom.Cloned c = PackageDefinitions.ClonedFrom.Cloned.Factory.newInstance();
        c.setPackageGuid(id.getGuid());
        c.setPackageVersion(id.getVersion());
        c.setId(count.add(new BigInteger("1")));
        String guid = wt.getPackageFarGuid(oldId);
        if (guid != null && !guid.equals("")) {
            c.setFarGuid(guid);
        }

        cf.addNewCloned();
        cf.setClonedArray(cf.getClonedList().size() - 1, c);
        pd.addNewClonedFrom();
        pd.setClonedFrom(cf);
        spd.setPackageDefinitions(pd);
    }

    /**
     Set fpd file's <Cloned> section via given identification
     
     @param fpd PlatformSurfaceArea for clone target
     @param id Identification of clone source
     
     **/
    private void updatePlatformClonedId(PlatformSurfaceArea fpd, Identification id) {
        //
        // Get PlatformDefinitions First
        //
        PlatformDefinitions pd = null;
        if (fpd.getPlatformDefinitions() == null) {
            pd = PlatformDefinitions.Factory.newInstance();
            fpd.addNewPlatformDefinitions();
        } else {
            pd = fpd.getPlatformDefinitions();
        }

        //
        // Get ClonedFrom then
        //
        PlatformDefinitions.ClonedFrom cf = null;
        BigInteger count = new BigInteger("-1");
        if (pd.getClonedFrom() == null) {
            cf = PlatformDefinitions.ClonedFrom.Factory.newInstance();
        } else {
            cf = pd.getClonedFrom();
            if (cf != null) {
                for (int index = 0; index < cf.getClonedList().size(); index++) {
                    if (cf.getClonedList().get(index).getId() != null) {
                        count = count.max(cf.getClonedList().get(index).getId());
                    }
                }
            }
        }

        //
        // Set new Cloned item
        //
        PlatformDefinitions.ClonedFrom.Cloned c = PlatformDefinitions.ClonedFrom.Cloned.Factory.newInstance();
        c.setPlatformGuid(id.getGuid());
        c.setPlatformVersion(id.getVersion());
        c.setId(count.add(new BigInteger("1")));
        String guid = wt.getPlatformFarGuid(oldId);
        if (guid != null && !guid.equals("")) {
            c.setFarGuid(guid);
        }

        cf.addNewCloned();
        cf.setClonedArray(cf.getClonedList().size() - 1, c);
        pd.addNewClonedFrom();
        pd.setClonedFrom(cf);
        fpd.setPlatformDefinitions(pd);
    }

    /**
     Get PlatformIdentification
     
     @return PlatformIdentification
     
     **/
    public PlatformIdentification getFid() {
        return fid;
    }

    /**
     Set PlatformIdentification
     
     @param fid PlatformIdentification
     
     **/
    public void setFid(PlatformIdentification fid) {
        this.fid = fid;
    }

    /**
     Get ModuleIdentification
     
     @return ModuleIdentification
     
     **/
    public ModuleIdentification getMid() {
        return mid;
    }

    /**
     Set ModuleIdentification
     
     @param mid ModuleIdentification
     
     **/
    public void setMid(ModuleIdentification mid) {
        this.mid = mid;
    }

    /**
     Get PackageIdentification
     
     @return PackageIdentification
     
     **/
    public PackageIdentification getPid() {
        return pid;
    }

    /**
     Set PackageIdentification
     
     @param pid PackageIdentification
     
     **/
    public void setPid(PackageIdentification pid) {
        this.pid = pid;
    }
}
