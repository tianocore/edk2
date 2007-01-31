/** @file
 
 The main GUI for module editor. 
 
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
import java.awt.event.ComponentEvent;
import java.awt.event.KeyEvent;
import java.awt.event.KeyListener;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.awt.event.WindowEvent;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.util.Iterator;
import java.util.Set;
import java.util.Vector;

import javax.swing.JButton;
import javax.swing.JCheckBoxMenuItem;
import javax.swing.JDesktopPane;
import javax.swing.JFileChooser;
import javax.swing.JFrame;
import javax.swing.JInternalFrame;
import javax.swing.JMenu;
import javax.swing.JMenuBar;
import javax.swing.JMenuItem;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JSplitPane;
import javax.swing.JTabbedPane;
import javax.swing.ToolTipManager;
import javax.swing.event.MenuEvent;
import javax.swing.event.MenuListener;
import javax.swing.event.TreeSelectionEvent;
import javax.swing.event.TreeSelectionListener;
import javax.swing.tree.TreePath;

import org.apache.xmlbeans.XmlException;
import org.tianocore.PackageSurfaceAreaDocument;
import org.tianocore.frameworkwizard.common.DataType;
import org.tianocore.frameworkwizard.common.GlobalData;
import org.tianocore.frameworkwizard.common.IFileFilter;
import org.tianocore.frameworkwizard.common.Log;
import org.tianocore.frameworkwizard.common.SaveFile;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.Identifications.Identification;
import org.tianocore.frameworkwizard.common.Identifications.OpeningModuleType;
import org.tianocore.frameworkwizard.common.Identifications.OpeningPackageType;
import org.tianocore.frameworkwizard.common.Identifications.OpeningPlatformType;
import org.tianocore.frameworkwizard.common.find.FindResult;
import org.tianocore.frameworkwizard.common.ui.IDefaultMutableTreeNode;
import org.tianocore.frameworkwizard.common.ui.IDesktopManager;
import org.tianocore.frameworkwizard.common.ui.IFrame;
import org.tianocore.frameworkwizard.common.ui.ITree;
import org.tianocore.frameworkwizard.far.createui.CreateStepOne;
import org.tianocore.frameworkwizard.far.deleteui.DeleteStepOne;
import org.tianocore.frameworkwizard.far.installui.InstallStepOne;
import org.tianocore.frameworkwizard.far.updateui.UpdateStepOne;
import org.tianocore.frameworkwizard.module.Identifications.ModuleIdentification;
import org.tianocore.frameworkwizard.module.ui.ModuleBootModes;
import org.tianocore.frameworkwizard.module.ui.ModuleBuildOptions;
import org.tianocore.frameworkwizard.module.ui.ModuleDataHubs;
import org.tianocore.frameworkwizard.module.ui.ModuleEvents;
import org.tianocore.frameworkwizard.module.ui.ModuleExterns;
import org.tianocore.frameworkwizard.module.ui.ModuleGuids;
import org.tianocore.frameworkwizard.module.ui.ModuleHiiPackages;
import org.tianocore.frameworkwizard.module.ui.ModuleHobs;
import org.tianocore.frameworkwizard.module.ui.ModuleLibraryClassDefinitions;
import org.tianocore.frameworkwizard.module.ui.ModulePCDs;
import org.tianocore.frameworkwizard.module.ui.ModulePackageDependencies;
import org.tianocore.frameworkwizard.module.ui.ModulePpis;
import org.tianocore.frameworkwizard.module.ui.ModuleProtocols;
import org.tianocore.frameworkwizard.module.ui.ModuleSourceFiles;
import org.tianocore.frameworkwizard.module.ui.ModuleSystemTables;
import org.tianocore.frameworkwizard.module.ui.ModuleVariables;
import org.tianocore.frameworkwizard.module.ui.MsaHeader;
import org.tianocore.frameworkwizard.packaging.PackageIdentification;
import org.tianocore.frameworkwizard.packaging.ui.SpdGuidDecls;
import org.tianocore.frameworkwizard.packaging.ui.SpdHeader;
import org.tianocore.frameworkwizard.packaging.ui.SpdLibClassDecls;
import org.tianocore.frameworkwizard.packaging.ui.SpdMsaFiles;
import org.tianocore.frameworkwizard.packaging.ui.SpdPackageHeaders;
import org.tianocore.frameworkwizard.packaging.ui.SpdPcdDefs;
import org.tianocore.frameworkwizard.packaging.ui.SpdPpiDecls;
import org.tianocore.frameworkwizard.packaging.ui.SpdProtocolDecls;
import org.tianocore.frameworkwizard.platform.PlatformIdentification;
import org.tianocore.frameworkwizard.platform.ui.FpdBuildOptions;
import org.tianocore.frameworkwizard.platform.ui.FpdDynamicPcdBuildDefinitions;
import org.tianocore.frameworkwizard.platform.ui.FpdFlash;
import org.tianocore.frameworkwizard.platform.ui.FpdFrameworkModules;
import org.tianocore.frameworkwizard.platform.ui.FpdHeader;
import org.tianocore.frameworkwizard.platform.ui.FpdPlatformDefs;
import org.tianocore.frameworkwizard.workspace.Workspace;
import org.tianocore.frameworkwizard.workspace.WorkspaceTools;
import org.tianocore.frameworkwizard.workspace.ui.SwitchWorkspace;
import org.tianocore.frameworkwizard.toolchain.Preferences;

/**
 The class is used to show main GUI of FrameworkWizard
 It extends IFrame implements MouseListener, TreeSelectionListener, ComponentListener and MenuListener

 **/
public class FrameworkWizardUI extends IFrame implements KeyListener, MouseListener, TreeSelectionListener,
                                             MenuListener {
    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = -7103240960573031772L;
    
    //
    // Set ToolTipText Show Time
    //
    static { ToolTipManager.sharedInstance().setDismissDelay(18000); }

    ///
    /// Used to record current operation target
    ///
    private int currentOpeningModuleIndex = -1;

    private int currentOpeningPackageIndex = -1;

    private int currentOpeningPlatformIndex = -1;

    ///
    /// Used to generate tree structure
    ///
    private IDefaultMutableTreeNode dmtnRoot = null;

    private IDefaultMutableTreeNode dmtnModuleDescription = null;

    private IDefaultMutableTreeNode dmtnPackageDescription = null;

    private IDefaultMutableTreeNode dmtnPlatformDescription = null;

    ///
    /// Used for UI
    ///
    private JPanel jContentPane = null;

    private JMenuBar jMenuBar = null;

    private JMenu jMenuFile = null;

    private JMenuItem jMenuItemFileNew = null;

    private JMenuItem jMenuItemFileRefresh = null;

    private JMenuItem jMenuItemFileSaveAs = null;

    private JMenuItem jMenuItemFileExit = null;

    private JMenu jMenuEdit = null;

    private JDesktopPane jDesktopPaneModule = null;

    private JDesktopPane jDesktopPanePackage = null;

    private JDesktopPane jDesktopPanePlatform = null;

    private JTabbedPane jTabbedPaneTree = null;

    private JTabbedPane jTabbedPaneEditor = null;

    private IDesktopManager iDesktopManager = new IDesktopManager();

    private JScrollPane jScrollPaneTree = null;

    private ITree iTree = null;

    private JMenu jMenuHelp = null;

    private JMenuItem jMenuItemHelpAbout = null;

    private JMenuItem jMenuItemEditDelete = null;

    private WorkspaceTools wt = new WorkspaceTools();

    private JMenuItem jMenuItemFileSave = null;

    private JMenuItem jMenuItemFileClose = null;

    private JMenu jMenuTools = null;

    private JMenu jMenuWindow = null;

    private JPanel jPanelOperation = null;

    private JButton jButtonOk = null;

    private JButton jButtonCancel = null;

    private JMenuItem jMenuItemFileOpen = null;

    private JMenuItem jMenuItemFileCloseAll = null;

    private JMenuItem jMenuItemFileSaveAll = null;

    private JMenuItem jMenuItemFilePageSetup = null;

    private JMenuItem jMenuItemFilePrint = null;

    private JMenuItem jMenuItemFileImport = null;

    private JMenuItem jMenuItemFileProperties = null;

    private JMenu jMenuFileRecentFiles = null;

    private JSplitPane jSplitPane = null;

    private JMenuItem jMenuItemEditUndo = null;

    private JMenuItem jMenuItemEditRedo = null;

    private JMenuItem jMenuItemEditCut = null;

    private JMenuItem jMenuItemEditCopy = null;

    private JMenuItem jMenuItemEditPaste = null;

    private JMenuItem jMenuItemEditSelectAll = null;

    private JMenuItem jMenuItemEditFindNext = null;

    private JMenu jMenuView = null;

    private JMenu jMenuViewToolbars = null;

    private JCheckBoxMenuItem jCheckBoxMenuItemViewToolbarsFile = null;

    private JCheckBoxMenuItem jCheckBoxMenuItemViewToolbarsEdit = null;

    private JCheckBoxMenuItem jCheckBoxMenuItemViewToolbarsWindow = null;

    private JMenuItem jMenuItemViewStandard = null;

    private JMenuItem jMenuItemViewAdvanced = null;

    private JMenu jMenuProject = null;

    private JMenuItem jMenuItemProjectAdmin = null;

    private JMenuItem jMenuItemProjectChangeWorkspace = null;

    private JMenuItem jMenuItemToolsBuildPreferences = null;

    //    private JCheckBoxMenuItem jCheckBoxMenuItemProjectBuildTargetsDebug = null;

    //    private JCheckBoxMenuItem jCheckBoxMenuItemProjectBuildTargetsRelease = null;

    private JMenuItem jMenuItemToolsToolChainConfiguration = null;

    private JMenuItem jMenuItemToolsClone = null;

    private JMenuItem jMenuItemToolsCodeScan = null;

    private JMenuItem jMenuItemWindowDisplaySide = null;

    private JMenuItem jMenuItemWindowDisplayTopBottom = null;

    private JMenuItem jMenuItemViewXML = null;

    private JMenuItem jMenuItemWindowTabView = null;

    private JMenuItem jMenuItemWindowSource = null;

    private JMenuItem jMenuItemWindowXML = null;

    private JMenuItem jMenuItemWindowPreferences = null;

    private JMenuItem jMenuItemHelpContents = null;

    private JMenuItem jMenuItemHelpIndex = null;

    private JMenuItem jMenuItemHelpSearch = null;

    private JMenuItem jMenuItemProjectInstallFar = null;

    private JMenuItem jMenuItemProjectUpdateFar = null;

    private JMenuItem jMenuItemProjectRemoveFar = null;

    private JMenuItem jMenuItemProjectCreateFar = null;

    private JMenu jMenuEditFind = null;

    private JMenuItem jMenuItemEditFindPcd = null;

    private JMenuItem jMenuItemEditFindLibraryClass = null;

    private JMenuItem jMenuItemEditFindPpi = null;

    private JMenuItem jMenuItemEditFindProtocol = null;

    private JMenuItem jMenuItemEditFindGuid = null;

    private JMenuItem jMenuItemEditFindLibraryInstance = null;

    ///
    /// A static definition for this class itself
    ///
    private static FrameworkWizardUI fwui = null;

    private JMenuItem jMenuItemToolsGenerateGuidsXref = null;

    /**
     If the class hasn't an instnace, new one.
     
     @return FrameworkWizardUI The instance of this class
     
     **/
    public static FrameworkWizardUI getInstance(String[] args) {
        if (fwui == null) {
            fwui = new FrameworkWizardUI(args);
        }
        return fwui;
    }
    
    /**
    If the class hasn't an instnace, new one.
    
    @return FrameworkWizardUI The instance of this class
    
    **/
   public static FrameworkWizardUI getInstance() {
       return fwui;
   }

    /**
     This method initializes jMenuBar 
     
     @return javax.swing.JMenuBar Main menu bar for the entire GUI
     
     **/
    private JMenuBar getjJMenuBar() {
        if (jMenuBar == null) {
            jMenuBar = new JMenuBar();
            jMenuBar.setPreferredSize(new java.awt.Dimension(0, 18));
            jMenuBar.add(getJMenuFile());
            jMenuBar.add(getJMenuEdit());
            jMenuBar.add(getJMenuView());
            jMenuBar.add(getJMenuProject());
            jMenuBar.add(getJMenuTools());
            jMenuBar.add(getJMenuWindow());
            jMenuBar.add(getJMenuHelp());
        }
        return jMenuBar;
    }

    /**
     This method initializes jSplitPane
     
     @return javax.swing.JSplitPane
     
     **/
    private JSplitPane getJSplitPane() {
        if (jSplitPane == null) {
            jSplitPane = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT, getJTabbedPaneTree(), getJTabbedPaneEditor());
            jSplitPane.setBounds(new java.awt.Rectangle(0, 1, DataType.MAIN_FRAME_SPLIT_PANEL_PREFERRED_SIZE_WIDTH,
                                                        DataType.MAIN_FRAME_SPLIT_PANEL_PREFERRED_SIZE_HEIGHT));
            jSplitPane.addComponentListener(this);
        }
        return jSplitPane;
    }

    /**
     This method initializes jTabbedPaneEditor	
     
     @return javax.swing.JTabbedPane	
     
     **/
    private JTabbedPane getJTabbedPaneEditor() {
        if (jTabbedPaneEditor == null) {
            jTabbedPaneEditor = new JTabbedPane();
            jTabbedPaneEditor.setBounds(new java.awt.Rectangle(DataType.MAIN_FRAME_EDITOR_PANEL_LOCATION_X,
                                                               DataType.MAIN_FRAME_EDITOR_PANEL_LOCATION_Y,
                                                               DataType.MAIN_FRAME_EDITOR_PANEL_PREFERRED_SIZE_WIDTH,
                                                               DataType.MAIN_FRAME_EDITOR_PANEL_PREFERRED_SIZE_HEIGHT));
            jTabbedPaneEditor
                             .setMinimumSize(new java.awt.Dimension(
                                                                    DataType.MAIN_FRAME_EDITOR_PANEL_PREFERRED_SIZE_WIDTH,
                                                                    DataType.MAIN_FRAME_EDITOR_PANEL_PREFERRED_SIZE_HEIGHT));
            jTabbedPaneEditor.addTab("Module", null, getJDesktopPaneModule(), null);
            jTabbedPaneEditor.addTab("Package", null, getJDesktopPanePackage(), null);
            jTabbedPaneEditor.addTab("Platform", null, getJDesktopPanePlatform(), null);
        }
        return jTabbedPaneEditor;
    }

    /**
     This method initializes jTabbedPaneTree
     
     @return javax.swing.JTabbedPane	
     
     **/
    private JTabbedPane getJTabbedPaneTree() {
        if (jTabbedPaneTree == null) {
            jTabbedPaneTree = new JTabbedPane();
            jTabbedPaneTree
                           .setPreferredSize(new java.awt.Dimension(
                                                                    DataType.MAIN_FRAME_TREE_PANEL_PREFERRED_SIZE_WIDTH,
                                                                    DataType.MAIN_FRAME_TREE_PANEL_PREFERRED_SIZE_HEIGHT));
            jTabbedPaneTree
                           .setMinimumSize(new java.awt.Dimension(DataType.MAIN_FRAME_TREE_PANEL_PREFERRED_SIZE_WIDTH,
                                                                  DataType.MAIN_FRAME_TREE_PANEL_PREFERRED_SIZE_HEIGHT));
            jTabbedPaneTree.addTab("Workspace Explorer", null, getJScrollPaneTree(), null);
        }
        return jTabbedPaneTree;
    }

    /**
     This method initializes jMenuFile 
     
     @return javax.swing.JMenu jMenuModule
     
     **/
    private JMenu getJMenuFile() {
        if (jMenuFile == null) {
            //
            // Set jMenuFile's attributes
            //
            jMenuFile = new JMenu();
            jMenuFile.setText("File");
            jMenuFile.setMnemonic('F');
            jMenuFile.addMenuListener(this);

            //
            // Add sub menu items
            //
            jMenuFile.add(getJMenuItemFileNew());
            jMenuFile.add(getJMenuItemFileOpen());
            jMenuFile.add(getJMenuItemFileClose());
            jMenuFile.add(getJMenuItemFileCloseAll());
            jMenuFile.addSeparator();

            jMenuFile.add(getJMenuFileRecentFiles());
            jMenuFile.add(getJMenuItemFileSave());
            jMenuFile.add(getJMenuItemFileSaveAs());
            jMenuFile.add(getJMenuItemFileSaveAll());
            jMenuFile.addSeparator();

            jMenuFile.add(getJMenuItemFileRefresh());
            jMenuFile.addSeparator();

            jMenuFile.add(getJMenuItemFilePageSetup());
            jMenuFile.add(getJMenuItemFilePrint());
            jMenuFile.add(getJMenuItemFileImport());
            jMenuFile.add(getJMenuItemFileProperties());

            jMenuFile.add(getJMenuItemFileExit());
        }
        return jMenuFile;
    }

    /**
     This method initializes jMenuItemFileSaveAs 
     
     @return javax.swing.JMenuItem jMenuItemFileSaveAs
     
     **/
    private JMenuItem getJMenuItemFileSaveAs() {
        if (jMenuItemFileSaveAs == null) {
            jMenuItemFileSaveAs = new JMenuItem();
            jMenuItemFileSaveAs.setText("Save As...");
            jMenuItemFileSaveAs.setMnemonic('a');
            jMenuItemFileSaveAs.addActionListener(this);
            jMenuItemFileSaveAs.setEnabled(false);
            jMenuItemFileSaveAs.setVisible(false);
        }
        return jMenuItemFileSaveAs;
    }

    /**
     This method initializes jMenuItemFileRefresh 
     
     @return javax.swing.JMenuItem jMenuItemFileRefresh
     
     **/
    private JMenuItem getJMenuItemFileRefresh() {
        if (jMenuItemFileRefresh == null) {
            jMenuItemFileRefresh = new JMenuItem();
            jMenuItemFileRefresh.setText("Refresh");
            jMenuItemFileRefresh.setMnemonic('R');
            jMenuItemFileRefresh.addActionListener(this);
            jMenuItemFileRefresh.setVisible(true);
        }
        return jMenuItemFileRefresh;
    }

    /**
     This method initializes jMenuItemModuleExit 
     
     @return javax.swing.JMenuItem jMenuItemModuleExit
     
     **/
    private JMenuItem getJMenuItemFileExit() {
        if (jMenuItemFileExit == null) {
            jMenuItemFileExit = new JMenuItem();
            jMenuItemFileExit.setText("Exit");
            jMenuItemFileExit.setMnemonic('x');
            jMenuItemFileExit.addActionListener(this);
        }
        return jMenuItemFileExit;
    }

    /**
     This method initializes jMenuEdit 
     
     @return javax.swing.JMenu jMenuEdit
     
     **/
    private JMenu getJMenuEdit() {
        if (jMenuEdit == null) {
            //
            // Set jMenuEdit's attributes
            //
            jMenuEdit = new JMenu();
            jMenuEdit.setText("Edit");
            jMenuEdit.setMnemonic('E');
            jMenuEdit.setVisible(true);

            //
            // Add sub menu items
            //
            jMenuEdit.add(getJMenuItemEditUndo());
            jMenuEdit.add(getJMenuItemEditRedo());
            //jMenuEdit.addSeparator();

            jMenuEdit.add(getJMenuItemEditCut());
            jMenuEdit.add(getJMenuItemEditCopy());
            jMenuEdit.add(getJMenuItemEditPaste());
            jMenuEdit.add(getJMenuItemEditDelete());
            //jMenuEdit.addSeparator();

            jMenuEdit.add(getJMenuItemEditSelectAll());
            jMenuEdit.add(getJMenuEditFind());
            jMenuEdit.add(getJMenuItemEditFindNext());
            //jMenuEdit.addSeparator();
        }
        return jMenuEdit;
    }

    /**
     This method initializes jDesktopPane 
     
     @return javax.swing.JDesktopPane jDesktopPane
     
     **/
    private JDesktopPane getJDesktopPaneModule() {
        if (jDesktopPaneModule == null) {
            jDesktopPaneModule = new JDesktopPane();
            jDesktopPaneModule
                              .setBounds(new java.awt.Rectangle(DataType.MAIN_FRAME_EDITOR_PANEL_LOCATION_X,
                                                                DataType.MAIN_FRAME_EDITOR_PANEL_LOCATION_Y,
                                                                DataType.MAIN_FRAME_EDITOR_PANEL_PREFERRED_SIZE_WIDTH,
                                                                DataType.MAIN_FRAME_EDITOR_PANEL_PREFERRED_SIZE_HEIGHT));
            jDesktopPaneModule
                              .setMinimumSize(new java.awt.Dimension(
                                                                     DataType.MAIN_FRAME_EDITOR_PANEL_PREFERRED_SIZE_WIDTH,
                                                                     DataType.MAIN_FRAME_EDITOR_PANEL_PREFERRED_SIZE_HEIGHT));
            jDesktopPaneModule.setDesktopManager(iDesktopManager);
            jDesktopPaneModule.addComponentListener(this);
        }
        return jDesktopPaneModule;
    }

    /**
     This method initializes jDesktopPane 
     
     @return javax.swing.JDesktopPane jDesktopPane
     
     **/
    private JDesktopPane getJDesktopPanePackage() {
        if (jDesktopPanePackage == null) {
            jDesktopPanePackage = new JDesktopPane();
            jDesktopPanePackage
                               .setBounds(new java.awt.Rectangle(DataType.MAIN_FRAME_EDITOR_PANEL_LOCATION_X,
                                                                 DataType.MAIN_FRAME_EDITOR_PANEL_LOCATION_Y,
                                                                 DataType.MAIN_FRAME_EDITOR_PANEL_PREFERRED_SIZE_WIDTH,
                                                                 DataType.MAIN_FRAME_EDITOR_PANEL_PREFERRED_SIZE_HEIGHT));
            jDesktopPanePackage
                               .setMinimumSize(new java.awt.Dimension(
                                                                      DataType.MAIN_FRAME_EDITOR_PANEL_PREFERRED_SIZE_WIDTH,
                                                                      DataType.MAIN_FRAME_EDITOR_PANEL_PREFERRED_SIZE_HEIGHT));
            jDesktopPanePackage.setDesktopManager(iDesktopManager);
            jDesktopPanePackage.addComponentListener(this);
        }
        return jDesktopPanePackage;
    }

    /**
     This method initializes jDesktopPane 
     
     @return javax.swing.JDesktopPane jDesktopPane
     
     **/
    private JDesktopPane getJDesktopPanePlatform() {
        if (jDesktopPanePlatform == null) {
            jDesktopPanePlatform = new JDesktopPane();
            jDesktopPanePlatform
                                .setBounds(new java.awt.Rectangle(
                                                                  DataType.MAIN_FRAME_EDITOR_PANEL_LOCATION_X,
                                                                  DataType.MAIN_FRAME_EDITOR_PANEL_LOCATION_Y,
                                                                  DataType.MAIN_FRAME_EDITOR_PANEL_PREFERRED_SIZE_WIDTH,
                                                                  DataType.MAIN_FRAME_EDITOR_PANEL_PREFERRED_SIZE_HEIGHT));
            jDesktopPanePlatform
                                .setMinimumSize(new java.awt.Dimension(
                                                                       DataType.MAIN_FRAME_EDITOR_PANEL_PREFERRED_SIZE_WIDTH,
                                                                       DataType.MAIN_FRAME_EDITOR_PANEL_PREFERRED_SIZE_HEIGHT));
            jDesktopPanePlatform.setDesktopManager(iDesktopManager);
            jDesktopPanePlatform.addComponentListener(this);
        }
        return jDesktopPanePlatform;
    }

    /**
     This method initializes jScrollPaneTree 
     
     @return javax.swing.JScrollPane jScrollPaneTree
     
     **/
    private JScrollPane getJScrollPaneTree() {
        if (jScrollPaneTree == null) {
            jScrollPaneTree = new JScrollPane();
            jScrollPaneTree
                           .setPreferredSize(new java.awt.Dimension(
                                                                    DataType.MAIN_FRAME_TREE_PANEL_PREFERRED_SIZE_WIDTH,
                                                                    DataType.MAIN_FRAME_TREE_PANEL_PREFERRED_SIZE_HEIGHT));
            jScrollPaneTree
                           .setMinimumSize(new java.awt.Dimension(
                                                                  DataType.MAIN_FRAME_TREE_PANEL_PREFERRED_SIZE_WIDTH / 2,
                                                                  DataType.MAIN_FRAME_TREE_PANEL_PREFERRED_SIZE_HEIGHT));
            jScrollPaneTree.setViewportView(getITree());
        }
        return jScrollPaneTree;
    }

    /**
     This method initializes iTree 
     
     @return org.tianocore.packaging.common.ui.ITree iTree
     
     **/
    private ITree getITree() {
        //
        //Before open a real module, use an empty root node for the tree
        //
        makeEmptyTree();
        return iTree;
    }

    /**
     This method initializes jMenuHelp 
     
     @return javax.swing.JMenu jMenuHelp
     
     **/
    private JMenu getJMenuHelp() {
        if (jMenuHelp == null) {
            //
            // Set jMenuHelp's attributes
            //
            jMenuHelp = new JMenu();
            jMenuHelp.setText("Help");
            jMenuHelp.setMnemonic('H');

            //
            // Add sub menu items
            //
            jMenuHelp.add(getJMenuItemHelpContents());
            jMenuHelp.add(getJMenuItemHelpIndex());
            jMenuHelp.add(getJMenuItemHelpSearch());

            jMenuHelp.add(getJMenuItemHelpAbout());
        }
        return jMenuHelp;
    }

    /**
     This method initializes jMenuItemHelpAbout 
     
     @return javax.swing.JMenuItem jMenuItemHelpAbout
     
     **/
    private JMenuItem getJMenuItemHelpAbout() {
        if (jMenuItemHelpAbout == null) {
            jMenuItemHelpAbout = new JMenuItem();
            jMenuItemHelpAbout.setText("About");
            jMenuItemHelpAbout.setMnemonic('A');
            jMenuItemHelpAbout.addActionListener(this);
        }
        return jMenuItemHelpAbout;
    }

    /**
     This method initializes jMenuItemEditDelete 
     
     @return javax.swing.JMenuItem jMenuItemEditDelete
     
     **/
    private JMenuItem getJMenuItemEditDelete() {
        if (jMenuItemEditDelete == null) {
            jMenuItemEditDelete = new JMenuItem();
            jMenuItemEditDelete.setText("Delete");
            jMenuItemEditDelete.setMnemonic('D');
            jMenuItemEditDelete.setVisible(false);
            jMenuItemEditDelete.setEnabled(false);
            jMenuItemEditDelete.addActionListener(this);
            //
            //Disabled first when no module is open
            //
            jMenuItemEditDelete.setEnabled(false);
        }
        return jMenuItemEditDelete;
    }

    /**
     This method initializes jMenuFileNew 
     
     @return javax.swing.JMenuItem jMenuFileNew
     
     **/
    private JMenuItem getJMenuItemFileNew() {
        if (jMenuItemFileNew == null) {
            jMenuItemFileNew = new JMenuItem();
            jMenuItemFileNew.setText("New...");
            jMenuItemFileNew.setMnemonic('N');
            jMenuItemFileNew.addActionListener(this);
        }
        return jMenuItemFileNew;
    }

    /**
     This method initializes jMenuItemFileSave 
     
     @return javax.swing.JMenuItem jMenuItemModuleSave
     
     **/
    private JMenuItem getJMenuItemFileSave() {
        if (jMenuItemFileSave == null) {
            jMenuItemFileSave = new JMenuItem();
            jMenuItemFileSave.setText("Save");
            jMenuItemFileSave.setMnemonic('S');
            jMenuItemFileSave.addActionListener(this);
            jMenuItemFileSave.setEnabled(true);
        }
        return jMenuItemFileSave;
    }

    /**
     This method initializes jMenuItemModuleClose 
     
     @return javax.swing.JMenuItem jMenuItemModuleClose
     
     **/
    private JMenuItem getJMenuItemFileClose() {
        if (jMenuItemFileClose == null) {
            jMenuItemFileClose = new JMenuItem();
            jMenuItemFileClose.setText("Close");
            jMenuItemFileClose.setMnemonic('C');
            jMenuItemFileClose.setEnabled(true);
            jMenuItemFileClose.addActionListener(this);
        }
        return jMenuItemFileClose;
    }

    /**
     This method initializes jMenuTools
     Reserved 
     
     @return javax.swing.JMenu jMenuTools
     
     **/
    private JMenu getJMenuTools() {
        if (jMenuTools == null) {
            //
            // Set jMenuTools's attributes
            //
            jMenuTools = new JMenu();
            jMenuTools.setText("Tools");
            jMenuTools.setMnemonic('T');
            jMenuTools.addMenuListener(this);

            //
            // Add sub menu items
            //

            jMenuTools.add(getJMenuItemToolsClone());
            jMenuTools.add(getJMenuItemToolsCodeScan());
            jMenuTools.addSeparator();

            jMenuTools.add(getJMenuItemToolsToolChainConfiguration());
            jMenuTools.add(getJMenuItemToolsBuildPreferences());
            jMenuTools.addSeparator();

            jMenuTools.add(getJMenuItemToolsGenerateGuidsXref());
        }
        return jMenuTools;
    }

    /**
     This method initializes jMenuWindow 
     Reserved
     
     @return javax.swing.JMenu jMenuWindow
     
     **/
    private JMenu getJMenuWindow() {
        if (jMenuWindow == null) {
            //
            // Set jMenuWindow's attribute
            //
            jMenuWindow = new JMenu();
            jMenuWindow.setText("Window");
            jMenuWindow.setMnemonic('W');
            jMenuWindow.setVisible(false);

            //
            // Add sub menu items
            //
            jMenuWindow.add(getJMenuItemWindowDisplaySide());
            jMenuWindow.add(getJMenuItemWindowDisplayTopBottom());
            jMenuWindow.addSeparator();

            jMenuWindow.add(getJMenuItemWindowTabView());
            jMenuWindow.addSeparator();

            jMenuWindow.add(getJMenuItemWindowSource());
            jMenuWindow.add(getJMenuItemWindowXML());
            jMenuWindow.addSeparator();

            jMenuWindow.add(getJMenuItemWindowPreferences());
        }
        return jMenuWindow;
    }

    /**
     This method initializes jPanelOperation
     Reserved
     
     @return javax.swing.JPanel jPanelOperation
     
     **/
    private JPanel getJPanelOperation() {
        if (jPanelOperation == null) {
            jPanelOperation = new JPanel();
            jPanelOperation.setLayout(null);
            jPanelOperation.setBounds(new java.awt.Rectangle(295, 520, 500, 25));
            jPanelOperation.add(getJButtonOk(), null);
            jPanelOperation.add(getJButtonCancel(), null);
            jPanelOperation.setVisible(false);
        }
        return jPanelOperation;
    }

    /**
     This method initializes jButtonOk
     Reserved
     
     @return javax.swing.JButton jButtonOk
     
     **/
    private JButton getJButtonOk() {
        if (jButtonOk == null) {
            jButtonOk = new JButton();
            jButtonOk.setBounds(new java.awt.Rectangle(395, 2, 90, 20));
            jButtonOk.setText("Ok");
            jButtonOk.setEnabled(false);
            jButtonOk.addActionListener(this);
        }
        return jButtonOk;
    }

    /**
     This method initializes jButtonCancel
     Reserved
     
     @return javax.swing.JButton jButtonCancel
     
     **/
    private JButton getJButtonCancel() {
        if (jButtonCancel == null) {
            jButtonCancel = new JButton();
            jButtonCancel.setBounds(new java.awt.Rectangle(395, 2, 90, 20));
            jButtonCancel.setText("Cancel");
            jButtonCancel.setEnabled(false);
            jButtonCancel.addActionListener(this);
            jButtonCancel.setVisible(false);
        }
        return jButtonCancel;
    }

    /**
     This method initializes jMenuItemFileOpen	
     
     @return javax.swing.JMenuItem jMenuItemFileOpen
     
     **/
    private JMenuItem getJMenuItemFileOpen() {
        if (jMenuItemFileOpen == null) {
            jMenuItemFileOpen = new JMenuItem();
            jMenuItemFileOpen.setText("Open...");
            jMenuItemFileOpen.setMnemonic('O');
            jMenuItemFileOpen.addActionListener(this);
        }
        return jMenuItemFileOpen;
    }

    /**
     This method initializes jMenuItemFileCloseAll	
     
     @return javax.swing.JMenuItem jMenuItemFileOpen
     
     **/
    private JMenuItem getJMenuItemFileCloseAll() {
        if (jMenuItemFileCloseAll == null) {
            jMenuItemFileCloseAll = new JMenuItem();
            jMenuItemFileCloseAll.setText("Close All");
            jMenuItemFileCloseAll.setMnemonic('A');
            jMenuItemFileCloseAll.setEnabled(true);
            jMenuItemFileCloseAll.addActionListener(this);
        }
        return jMenuItemFileCloseAll;
    }

    /**
     This method initializes jMenuItemFileSaveAll	
     
     @return javax.swing.JMenuItem jMenuItemFileSaveAll
     
     **/
    private JMenuItem getJMenuItemFileSaveAll() {
        if (jMenuItemFileSaveAll == null) {
            jMenuItemFileSaveAll = new JMenuItem();
            jMenuItemFileSaveAll.setText("Save All");
            jMenuItemFileSaveAll.setMnemonic('v');
            jMenuItemFileSaveAll.setEnabled(true);
            jMenuItemFileSaveAll.addActionListener(this);
        }
        return jMenuItemFileSaveAll;
    }

    /**
     This method initializes jMenuItemFilePageSetup	
     
     @return javax.swing.JMenuItem	
     
     **/
    private JMenuItem getJMenuItemFilePageSetup() {
        if (jMenuItemFilePageSetup == null) {
            jMenuItemFilePageSetup = new JMenuItem();
            jMenuItemFilePageSetup.setText("Page Setup");
            jMenuItemFilePageSetup.setMnemonic('u');
            jMenuItemFilePageSetup.setEnabled(false);
            jMenuItemFilePageSetup.addActionListener(this);
            jMenuItemFilePageSetup.setVisible(false);
        }
        return jMenuItemFilePageSetup;
    }

    /**
     This method initializes jMenuItemFilePrint	
     
     @return javax.swing.JMenuItem	
     
     **/
    private JMenuItem getJMenuItemFilePrint() {
        if (jMenuItemFilePrint == null) {
            jMenuItemFilePrint = new JMenuItem();
            jMenuItemFilePrint.setText("Print");
            jMenuItemFilePrint.setMnemonic('P');
            jMenuItemFilePrint.setEnabled(false);
            jMenuItemFilePrint.addActionListener(this);
            jMenuItemFilePrint.setVisible(false);
        }
        return jMenuItemFilePrint;
    }

    /**
     This method initializes jMenuItemFileImport	
     
     @return javax.swing.JMenuItem	
     
     **/
    private JMenuItem getJMenuItemFileImport() {
        if (jMenuItemFileImport == null) {
            jMenuItemFileImport = new JMenuItem();
            jMenuItemFileImport.setText("Import");
            jMenuItemFileImport.setMnemonic('I');
            jMenuItemFileImport.setEnabled(false);
            jMenuItemFileImport.addActionListener(this);
            jMenuItemFileImport.setVisible(false);
        }
        return jMenuItemFileImport;
    }

    /**
     This method initializes jMenuItemFileProperties	
     
     @return javax.swing.JMenuItem	
     
     **/
    private JMenuItem getJMenuItemFileProperties() {
        if (jMenuItemFileProperties == null) {
            jMenuItemFileProperties = new JMenuItem();
            jMenuItemFileProperties.setText("Properties");
            jMenuItemFileProperties.setMnemonic('t');
            jMenuItemFileProperties.setEnabled(false);
            jMenuItemFileProperties.addActionListener(this);
            jMenuItemFileProperties.setVisible(false);
        }
        return jMenuItemFileProperties;
    }

    /**
     This method initializes jMenuFileRecentFiles	
     
     @return javax.swing.JMenu	
     
     **/
    private JMenu getJMenuFileRecentFiles() {
        if (jMenuFileRecentFiles == null) {
            jMenuFileRecentFiles = new JMenu();
            jMenuFileRecentFiles.setText("Recent Files");
            jMenuFileRecentFiles.setMnemonic('F');
            jMenuFileRecentFiles.setEnabled(false);
            jMenuFileRecentFiles.addActionListener(this);
            jMenuFileRecentFiles.setVisible(false);
        }
        return jMenuFileRecentFiles;
    }

    /**
     This method initializes jMenuItemEditUndo	
     
     @return javax.swing.JMenuItem
     
     **/
    private JMenuItem getJMenuItemEditUndo() {
        if (jMenuItemEditUndo == null) {
            jMenuItemEditUndo = new JMenuItem();
            jMenuItemEditUndo.setText("Undo");
            jMenuItemEditUndo.setMnemonic('U');
            jMenuItemEditUndo.setEnabled(false);
            jMenuItemEditUndo.setVisible(false);
            jMenuItemEditUndo.addActionListener(this);
        }
        return jMenuItemEditUndo;
    }

    /**
     This method initializes jMenuItemEditRedo	
     
     @return javax.swing.JMenuItem	
     
     **/
    private JMenuItem getJMenuItemEditRedo() {
        if (jMenuItemEditRedo == null) {
            jMenuItemEditRedo = new JMenuItem();
            jMenuItemEditRedo.setText("Redo");
            jMenuItemEditRedo.setMnemonic('R');
            jMenuItemEditRedo.setEnabled(false);
            jMenuItemEditRedo.setVisible(false);
            jMenuItemEditRedo.addActionListener(this);
        }
        return jMenuItemEditRedo;
    }

    /**
     This method initializes jMenuItemEditCut	
     
     @return javax.swing.JMenuItem	
     
     **/
    private JMenuItem getJMenuItemEditCut() {
        if (jMenuItemEditCut == null) {
            jMenuItemEditCut = new JMenuItem();
            jMenuItemEditCut.setText("Cut");
            jMenuItemEditCut.setMnemonic('t');
            jMenuItemEditCut.setEnabled(false);
            jMenuItemEditCut.setVisible(false);
            jMenuItemEditCut.addActionListener(this);
        }
        return jMenuItemEditCut;
    }

    /**
     This method initializes jMenuItemEditCopy	
     
     @return javax.swing.JMenuItem	
     
     **/
    private JMenuItem getJMenuItemEditCopy() {
        if (jMenuItemEditCopy == null) {
            jMenuItemEditCopy = new JMenuItem();
            jMenuItemEditCopy.setText("Copy");
            jMenuItemEditCopy.setMnemonic('C');
            jMenuItemEditCopy.setEnabled(false);
            jMenuItemEditCopy.setVisible(false);
            jMenuItemEditCopy.addActionListener(this);
        }
        return jMenuItemEditCopy;
    }

    /**
     This method initializes jMenuItemEditPaste	
     
     return javax.swing.JMenuItem	
     
     **/
    private JMenuItem getJMenuItemEditPaste() {
        if (jMenuItemEditPaste == null) {
            jMenuItemEditPaste = new JMenuItem();
            jMenuItemEditPaste.setText("Paste");
            jMenuItemEditPaste.setMnemonic('P');
            jMenuItemEditPaste.setEnabled(false);
            jMenuItemEditPaste.setVisible(false);
            jMenuItemEditPaste.addActionListener(this);
        }
        return jMenuItemEditPaste;
    }

    /**
     This method initializes jMenuItem	
     
     @return javax.swing.JMenuItem	
     
     **/
    private JMenuItem getJMenuItemEditSelectAll() {
        if (jMenuItemEditSelectAll == null) {
            jMenuItemEditSelectAll = new JMenuItem();
            jMenuItemEditSelectAll.setText("Select All");
            jMenuItemEditSelectAll.setMnemonic('A');
            jMenuItemEditSelectAll.setEnabled(false);
            jMenuItemEditSelectAll.setVisible(false);
            jMenuItemEditSelectAll.addActionListener(this);
        }
        return jMenuItemEditSelectAll;
    }

    /**
     This method initializes jMenuItemEditFindNext	
     
     @return javax.swing.JMenuItem	
     
     **/
    private JMenuItem getJMenuItemEditFindNext() {
        if (jMenuItemEditFindNext == null) {
            jMenuItemEditFindNext = new JMenuItem();
            jMenuItemEditFindNext.setText("Find Next");
            jMenuItemEditFindNext.setMnemonic('n');
            jMenuItemEditFindNext.setEnabled(false);
            jMenuItemEditFindNext.setVisible(false);
            jMenuItemEditFindNext.addActionListener(this);
        }
        return jMenuItemEditFindNext;
    }

    /**
     This method initializes jMenuView	
     
     @return javax.swing.JMenu	
     
     **/
    private JMenu getJMenuView() {
        if (jMenuView == null) {
            //
            // Set jMenuView's attributes
            //
            jMenuView = new JMenu();
            jMenuView.setText("View");
            jMenuView.setMnemonic('V');
            jMenuView.setVisible(false);

            //
            // Add sub menu items
            //
            jMenuView.add(getJMenuViewToolbars());
            jMenuView.add(getJMenuItemViewAdvanced());
            jMenuView.add(getJMenuItemViewStandard());
            jMenuView.add(getJMenuItemViewXML());
        }
        return jMenuView;
    }

    /**
     This method initializes jMenuViewToolbars	
     
     @return javax.swing.JMenu	
     
     **/
    private JMenu getJMenuViewToolbars() {
        if (jMenuViewToolbars == null) {
            jMenuViewToolbars = new JMenu();
            jMenuViewToolbars.setText("Toolbars");
            jMenuViewToolbars.setMnemonic('T');

            jMenuViewToolbars.add(getJCheckBoxMenuItemViewToolbarsFile());
            jMenuViewToolbars.add(getJCheckBoxMenuItemViewToolbarsEdit());
            jMenuViewToolbars.add(getJCheckBoxMenuItemViewToolbarsWindow());
        }
        return jMenuViewToolbars;
    }

    /**
     This method initializes jCheckBoxMenuItemViewToolbarsFile	
     
     @return javax.swing.JCheckBoxMenuItem	
     
     **/
    private JCheckBoxMenuItem getJCheckBoxMenuItemViewToolbarsFile() {
        if (jCheckBoxMenuItemViewToolbarsFile == null) {
            jCheckBoxMenuItemViewToolbarsFile = new JCheckBoxMenuItem();
            jCheckBoxMenuItemViewToolbarsFile.setText("File");
            jCheckBoxMenuItemViewToolbarsFile.setEnabled(false);
            jCheckBoxMenuItemViewToolbarsFile.addActionListener(this);
        }
        return jCheckBoxMenuItemViewToolbarsFile;
    }

    /**
     This method initializes jCheckBoxMenuItemViewToolbarsEdit	
     
     @return javax.swing.JCheckBoxMenuItem	
     
     **/
    private JCheckBoxMenuItem getJCheckBoxMenuItemViewToolbarsEdit() {
        if (jCheckBoxMenuItemViewToolbarsEdit == null) {
            jCheckBoxMenuItemViewToolbarsEdit = new JCheckBoxMenuItem();
            jCheckBoxMenuItemViewToolbarsEdit.setText("Edit");
            jCheckBoxMenuItemViewToolbarsEdit.setEnabled(false);
            jCheckBoxMenuItemViewToolbarsEdit.addActionListener(this);
        }
        return jCheckBoxMenuItemViewToolbarsEdit;
    }

    /**
     This method initializes jCheckBoxMenuItemViewToolbarsWindow	
     
     @return javax.swing.JCheckBoxMenuItem	
     
     **/
    private JCheckBoxMenuItem getJCheckBoxMenuItemViewToolbarsWindow() {
        if (jCheckBoxMenuItemViewToolbarsWindow == null) {
            jCheckBoxMenuItemViewToolbarsWindow = new JCheckBoxMenuItem();
            jCheckBoxMenuItemViewToolbarsWindow.setText("Window");
            jCheckBoxMenuItemViewToolbarsWindow.setEnabled(false);
            jCheckBoxMenuItemViewToolbarsWindow.addActionListener(this);
        }
        return jCheckBoxMenuItemViewToolbarsWindow;
    }

    /**
     This method initializes jMenuItemStandard	
     
     @return javax.swing.JMenuItem	
     
     **/
    private JMenuItem getJMenuItemViewStandard() {
        if (jMenuItemViewStandard == null) {
            jMenuItemViewStandard = new JMenuItem();
            jMenuItemViewStandard.setText("Standard");
            jMenuItemViewStandard.setMnemonic('S');
            jMenuItemViewStandard.setEnabled(false);
            jMenuItemViewStandard.addActionListener(this);
        }
        return jMenuItemViewStandard;
    }

    /**
     This method initializes jMenuItemAdvanced	
     
     @return javax.swing.JMenuItem	
     
     **/
    private JMenuItem getJMenuItemViewAdvanced() {
        if (jMenuItemViewAdvanced == null) {
            jMenuItemViewAdvanced = new JMenuItem();
            jMenuItemViewAdvanced.setText("Advanced");
            jMenuItemViewAdvanced.setMnemonic('A');
            jMenuItemViewAdvanced.setEnabled(false);
            jMenuItemViewAdvanced.addActionListener(this);
        }
        return jMenuItemViewAdvanced;
    }

    /**
     This method initializes jMenuProject	
     
     @return javax.swing.JMenu	
     
     **/
    private JMenu getJMenuProject() {
        if (jMenuProject == null) {
            //
            // Set jMenuProject's attributes
            //
            jMenuProject = new JMenu();
            jMenuProject.setText("Project");
            jMenuProject.setMnemonic('P');

            //
            // Add sub menu items
            //
            jMenuProject.add(getJMenuItemProjectAdmin());

            jMenuProject.add(getJMenuItemProjectChangeWorkspace());
            jMenuProject.addSeparator();

            jMenuProject.add(getJMenuItemProjectCreateFar());
            jMenuProject.add(getJMenuItemProjectInstallFar());
            jMenuProject.add(getJMenuItemProjectUpdateFar());
            jMenuProject.add(getJMenuItemProjectRemoveFar());

        }
        return jMenuProject;
    }

    /**
     This method initializes jMenuItemProjectAdmin	
     
     @return javax.swing.JMenuItem	
     
     **/
    private JMenuItem getJMenuItemProjectAdmin() {
        if (jMenuItemProjectAdmin == null) {
            jMenuItemProjectAdmin = new JMenuItem();
            jMenuItemProjectAdmin.setText("Admin...");
            jMenuItemProjectAdmin.setMnemonic('A');
            jMenuItemProjectAdmin.setEnabled(false);
            jMenuItemProjectAdmin.addActionListener(this);
            jMenuItemProjectAdmin.setVisible(false);
        }
        return jMenuItemProjectAdmin;
    }

    /**
     This method initializes jMenuItemProjectChangeWorkspace	
     
     @return javax.swing.JMenuItem	
     
     **/
    private JMenuItem getJMenuItemProjectChangeWorkspace() {
        if (jMenuItemProjectChangeWorkspace == null) {
            jMenuItemProjectChangeWorkspace = new JMenuItem();
            jMenuItemProjectChangeWorkspace.setText("Change WORKSPACE...");
            jMenuItemProjectChangeWorkspace.setMnemonic('W');
            jMenuItemProjectChangeWorkspace.setEnabled(true);
            jMenuItemProjectChangeWorkspace.addActionListener(this);
        }
        return jMenuItemProjectChangeWorkspace;
    }

    /**
     This method initializes jMenuProjectBuildPreferences	
     
     @return javax.swing.JMenu	
     
     **/
    private JMenuItem getJMenuItemToolsBuildPreferences() {
        if (jMenuItemToolsBuildPreferences == null) {
            jMenuItemToolsBuildPreferences = new JMenuItem();
            jMenuItemToolsBuildPreferences.setText("Build Preferences...");
            jMenuItemToolsBuildPreferences.setMnemonic('P');
            jMenuItemToolsBuildPreferences.setEnabled(true);
            jMenuItemToolsBuildPreferences.addActionListener(this);
        }
        return jMenuItemToolsBuildPreferences;
    }

    /**
     This method initializes jMenuItemToolsToolChainConfiguration	
     
     @return javax.swing.JMenuItem	
     
     **/
    private JMenuItem getJMenuItemToolsToolChainConfiguration() {
        if (jMenuItemToolsToolChainConfiguration == null) {
            jMenuItemToolsToolChainConfiguration = new JMenuItem();
            jMenuItemToolsToolChainConfiguration.setText("Tool Chain Configuration...");
            jMenuItemToolsToolChainConfiguration.setMnemonic('C');
            jMenuItemToolsToolChainConfiguration.addActionListener(this);
        }
        return jMenuItemToolsToolChainConfiguration;
    }

    /**
     This method initializes jMenuItemToolsClone	
     
     @return javax.swing.JMenuItem	
     
     **/
    private JMenuItem getJMenuItemToolsClone() {
        if (jMenuItemToolsClone == null) {
            jMenuItemToolsClone = new JMenuItem();
            jMenuItemToolsClone.setText("Clone...");
            jMenuItemToolsClone.setMnemonic('l');
            jMenuItemToolsClone.setEnabled(true);
            jMenuItemToolsClone.addActionListener(this);
        }
        return jMenuItemToolsClone;
    }

    /**
     This method initializes jMenuItemToolsCodeScan	
     
     @return javax.swing.JMenuItem	
     
     **/
    private JMenuItem getJMenuItemToolsCodeScan() {
        if (jMenuItemToolsCodeScan == null) {
            jMenuItemToolsCodeScan = new JMenuItem();
            jMenuItemToolsCodeScan.setText("Code Scan...");
            jMenuItemToolsCodeScan.setMnemonic('S');
            jMenuItemToolsCodeScan.setEnabled(false);
            jMenuItemToolsCodeScan.addActionListener(this);
            jMenuItemToolsCodeScan.setVisible(false);
        }
        return jMenuItemToolsCodeScan;
    }

    /**
     This method initializes jMenuItemWindowSplitVertical	
     
     @return javax.swing.JMenuItem	
     
     **/
    private JMenuItem getJMenuItemWindowDisplaySide() {
        if (jMenuItemWindowDisplaySide == null) {
            jMenuItemWindowDisplaySide = new JMenuItem();
            jMenuItemWindowDisplaySide.setText("Display Side by Side");
            jMenuItemWindowDisplaySide.setMnemonic('S');
            jMenuItemWindowDisplaySide.setEnabled(false);
            jMenuItemWindowDisplaySide.addActionListener(this);
        }
        return jMenuItemWindowDisplaySide;
    }

    /**
     This method initializes jMenuItemWindowSplitHorizontal	
     
     @return javax.swing.JMenuItem	
     
     **/
    private JMenuItem getJMenuItemWindowDisplayTopBottom() {
        if (jMenuItemWindowDisplayTopBottom == null) {
            jMenuItemWindowDisplayTopBottom = new JMenuItem();
            jMenuItemWindowDisplayTopBottom.setText("Display Top and Bottom");
            jMenuItemWindowDisplayTopBottom.setMnemonic('B');
            jMenuItemWindowDisplayTopBottom.setEnabled(false);
            jMenuItemWindowDisplayTopBottom.addActionListener(this);
        }
        return jMenuItemWindowDisplayTopBottom;
    }

    /**
     This method initializes jMenuItemViewXML	
     
     @return javax.swing.JMenuItem	
     
     **/
    private JMenuItem getJMenuItemViewXML() {
        if (jMenuItemViewXML == null) {
            jMenuItemViewXML = new JMenuItem();
            jMenuItemViewXML.setText("XML");
            jMenuItemViewXML.setMnemonic('X');
            jMenuItemViewXML.setEnabled(false);
            jMenuItemViewXML.addActionListener(this);
        }
        return jMenuItemViewXML;
    }

    /**
     This method initializes jMenuItemWindowTabView	
     
     @return javax.swing.JMenuItem	
     
     **/
    private JMenuItem getJMenuItemWindowTabView() {
        if (jMenuItemWindowTabView == null) {
            jMenuItemWindowTabView = new JMenuItem();
            jMenuItemWindowTabView.setText("Tab View");
            jMenuItemWindowTabView.setMnemonic('T');
            jMenuItemWindowTabView.setEnabled(false);
            jMenuItemWindowTabView.addActionListener(this);
        }
        return jMenuItemWindowTabView;
    }

    /**
     This method initializes jMenuItemWindowSource	
     
     @return javax.swing.JMenuItem	
     
     **/
    private JMenuItem getJMenuItemWindowSource() {
        if (jMenuItemWindowSource == null) {
            jMenuItemWindowSource = new JMenuItem();
            jMenuItemWindowSource.setText("Source");
            jMenuItemWindowSource.setMnemonic('S');
            jMenuItemWindowSource.setEnabled(false);
            jMenuItemWindowSource.addActionListener(this);
        }
        return jMenuItemWindowSource;
    }

    /**
     This method initializes jMenuItemWindowXML	
     
     @return javax.swing.JMenuItem	
     
     **/
    private JMenuItem getJMenuItemWindowXML() {
        if (jMenuItemWindowXML == null) {
            jMenuItemWindowXML = new JMenuItem();
            jMenuItemWindowXML.setText("XML");
            jMenuItemWindowXML.setMnemonic('X');
            jMenuItemWindowXML.setEnabled(false);
            jMenuItemWindowXML.addActionListener(this);
        }
        return jMenuItemWindowXML;
    }

    /**
     This method initializes jMenuItemWindowPreferences	
     
     @return javax.swing.JMenuItem	
     
     **/
    private JMenuItem getJMenuItemWindowPreferences() {
        if (jMenuItemWindowPreferences == null) {
            jMenuItemWindowPreferences = new JMenuItem();
            jMenuItemWindowPreferences.setText("Preferences");
            jMenuItemWindowPreferences.setMnemonic('P');
            jMenuItemWindowPreferences.setEnabled(false);
            jMenuItemWindowPreferences.addActionListener(this);
        }
        return jMenuItemWindowPreferences;
    }

    /**
     This method initializes jMenuItemHelpContents	
     
     @return javax.swing.JMenuItem	
     
     **/
    private JMenuItem getJMenuItemHelpContents() {
        if (jMenuItemHelpContents == null) {
            jMenuItemHelpContents = new JMenuItem();
            jMenuItemHelpContents.setText("Contents");
            jMenuItemHelpContents.setMnemonic('C');
            jMenuItemHelpContents.setEnabled(false);
            jMenuItemHelpContents.addActionListener(this);
            jMenuItemHelpContents.setVisible(false);
        }
        return jMenuItemHelpContents;
    }

    /**
     This method initializes jMenuItemHelpIndex	
     
     @return javax.swing.JMenuItem	
     
     **/
    private JMenuItem getJMenuItemHelpIndex() {
        if (jMenuItemHelpIndex == null) {
            jMenuItemHelpIndex = new JMenuItem();
            jMenuItemHelpIndex.setText("Index");
            jMenuItemHelpIndex.setMnemonic('I');
            jMenuItemHelpIndex.setEnabled(false);
            jMenuItemHelpIndex.addActionListener(this);
            jMenuItemHelpIndex.setVisible(false);
        }
        return jMenuItemHelpIndex;
    }

    /**
     This method initializes jMenuItemHelpSearch	
     
     @return javax.swing.JMenuItem	
     
     */
    private JMenuItem getJMenuItemHelpSearch() {
        if (jMenuItemHelpSearch == null) {
            jMenuItemHelpSearch = new JMenuItem();
            jMenuItemHelpSearch.setText("Search");
            jMenuItemHelpSearch.setMnemonic('S');
            jMenuItemHelpSearch.setEnabled(false);
            jMenuItemHelpSearch.addActionListener(this);
            jMenuItemHelpSearch.setVisible(false);
        }
        return jMenuItemHelpSearch;
    }

    /**
     * This method initializes jMenuItemToolsInstallPackage	
     * 	
     * @return javax.swing.JMenuItem	
     */
    private JMenuItem getJMenuItemProjectInstallFar() {
        if (jMenuItemProjectInstallFar == null) {
            jMenuItemProjectInstallFar = new JMenuItem();
            jMenuItemProjectInstallFar.setText("Install FAR...");
            jMenuItemProjectInstallFar.setMnemonic('I');
            jMenuItemProjectInstallFar.setEnabled(true);
            jMenuItemProjectInstallFar.addActionListener(this);
        }
        return jMenuItemProjectInstallFar;
    }

    /**
     * This method initializes jMenuItemToolsUpdatePackage	
     * 	
     * @return javax.swing.JMenuItem	
     */
    private JMenuItem getJMenuItemProjectUpdateFar() {
        if (jMenuItemProjectUpdateFar == null) {
            jMenuItemProjectUpdateFar = new JMenuItem();
            jMenuItemProjectUpdateFar.setText("Update FAR...");
            jMenuItemProjectUpdateFar.setMnemonic('U');
            jMenuItemProjectUpdateFar.setEnabled(true);
            jMenuItemProjectUpdateFar.addActionListener(this);
            jMenuItemProjectUpdateFar.setVisible(true);
        }
        return jMenuItemProjectUpdateFar;
    }

    /**
     * This method initializes jMenuItemRemovePackage	
     * 	
     * @return javax.swing.JMenuItem	
     */
    private JMenuItem getJMenuItemProjectRemoveFar() {
        if (jMenuItemProjectRemoveFar == null) {
            jMenuItemProjectRemoveFar = new JMenuItem();
            jMenuItemProjectRemoveFar.setText("Remove FAR...");
            jMenuItemProjectRemoveFar.setMnemonic('R');
            jMenuItemProjectRemoveFar.setEnabled(true);
            jMenuItemProjectRemoveFar.addActionListener(this);
        }
        return jMenuItemProjectRemoveFar;
    }

    /**
     * This method initializes jMenuItemProjectCreateFar	
     * 	
     * @return javax.swing.JMenuItem	
     */
    private JMenuItem getJMenuItemProjectCreateFar() {
        if (jMenuItemProjectCreateFar == null) {
            jMenuItemProjectCreateFar = new JMenuItem();
            jMenuItemProjectCreateFar.setText("Create FAR...");
            jMenuItemProjectCreateFar.setMnemonic('C');
            jMenuItemProjectCreateFar.addActionListener(this);
        }
        return jMenuItemProjectCreateFar;
    }

    /**
     * This method initializes jMenuEditFind	
     * 	
     * @return javax.swing.JMenu	
     */
    private JMenu getJMenuEditFind() {
        if (jMenuEditFind == null) {
            jMenuEditFind = new JMenu();
            jMenuEditFind.setText("Find");
            jMenuEditFind.setMnemonic('F');

            jMenuEditFind.add(getJMenuItemEditFindPpi());
            jMenuEditFind.add(getJMenuItemEditFindProtocol());
            jMenuEditFind.add(getJMenuItemEditFindGuid());
            jMenuEditFind.add(getJMenuItemEditFindPcd());
            jMenuEditFind.add(getJMenuItemEditFindLibraryClass());
            jMenuEditFind.add(getJMenuItemEditFindLibraryInstance());
        }
        return jMenuEditFind;
    }

    /**
     * This method initializes jMenuItemEditFindPcd	
     * 	
     * @return javax.swing.JMenuItem	
     */
    private JMenuItem getJMenuItemEditFindPcd() {
        if (jMenuItemEditFindPcd == null) {
            jMenuItemEditFindPcd = new JMenuItem();
            jMenuItemEditFindPcd.setText("All PCD entries");
            jMenuItemEditFindPcd.setMnemonic('P');
            jMenuItemEditFindPcd.addActionListener(this);
        }
        return jMenuItemEditFindPcd;
    }

    /**
     * This method initializes jMenuItemEditFindLibraryClass	
     * 	
     * @return javax.swing.JMenuItem	
     */
    private JMenuItem getJMenuItemEditFindLibraryClass() {
        if (jMenuItemEditFindLibraryClass == null) {
            jMenuItemEditFindLibraryClass = new JMenuItem();
            jMenuItemEditFindLibraryClass.setText("All Library Classes");
            jMenuItemEditFindLibraryClass.setMnemonic('C');
            jMenuItemEditFindLibraryClass.addActionListener(this);
        }
        return jMenuItemEditFindLibraryClass;
    }

    /**
     * This method initializes jMenuItemEditFindPpi	
     * 	
     * @return javax.swing.JMenuItem	
     */
    private JMenuItem getJMenuItemEditFindPpi() {
        if (jMenuItemEditFindPpi == null) {
            jMenuItemEditFindPpi = new JMenuItem();
            jMenuItemEditFindPpi.setText("All PPIs");
            jMenuItemEditFindPpi.setMnemonic('I');
            jMenuItemEditFindPpi.addActionListener(this);
        }
        return jMenuItemEditFindPpi;
    }

    /**
     * This method initializes jMenuItemEditFindProtocol	
     * 	
     * @return javax.swing.JMenuItem	
     */
    private JMenuItem getJMenuItemEditFindProtocol() {
        if (jMenuItemEditFindProtocol == null) {
            jMenuItemEditFindProtocol = new JMenuItem();
            jMenuItemEditFindProtocol.setText("All Protocols");
            jMenuItemEditFindProtocol.setMnemonic('r');
            jMenuItemEditFindProtocol.addActionListener(this);
        }
        return jMenuItemEditFindProtocol;
    }

    /**
     * This method initializes jMenuItemEditFindGuid	
     * 	
     * @return javax.swing.JMenuItem	
     */
    private JMenuItem getJMenuItemEditFindGuid() {
        if (jMenuItemEditFindGuid == null) {
            jMenuItemEditFindGuid = new JMenuItem();
            jMenuItemEditFindGuid.setText("All GUIDs");
            jMenuItemEditFindGuid.setMnemonic('G');
            jMenuItemEditFindGuid.addActionListener(this);
        }
        return jMenuItemEditFindGuid;
    }

    /**
     * This method initializes jMenuItemEditFindLibraryInstance	
     * 	
     * @return javax.swing.JMenuItem	
     */
    private JMenuItem getJMenuItemEditFindLibraryInstance() {
        if (jMenuItemEditFindLibraryInstance == null) {
            jMenuItemEditFindLibraryInstance = new JMenuItem();
            jMenuItemEditFindLibraryInstance.setText("All Library Instances");
            jMenuItemEditFindLibraryInstance.setMnemonic('n');
            jMenuItemEditFindLibraryInstance.addActionListener(this);
            jMenuItemEditFindLibraryInstance.setVisible(false);
        }
        return jMenuItemEditFindLibraryInstance;
    }

    /**
     * This method initializes jMenuItemProjectGenerateGuidsXref	
     * 	
     * @return javax.swing.JMenuItem	
     */
    private JMenuItem getJMenuItemToolsGenerateGuidsXref() {
        if (jMenuItemToolsGenerateGuidsXref == null) {
            jMenuItemToolsGenerateGuidsXref = new JMenuItem();
            jMenuItemToolsGenerateGuidsXref.setText("Generate guids.xref...");
            jMenuItemToolsGenerateGuidsXref.setMnemonic('G');
            jMenuItemToolsGenerateGuidsXref.addActionListener(this);
        }
        return jMenuItemToolsGenerateGuidsXref;
    }

    /* (non-Javadoc)
     * @see org.tianocore.packaging.common.ui.IFrame#main(java.lang.String[])
     *
     * Main class, start the GUI
     * 
     */
    public static void main(String[] args) {      
        //
        // Start Main UI
        //
        FrameworkWizardUI module = FrameworkWizardUI.getInstance(args);
        module.setVisible(true);
    }

    /**
     This is the default constructor
     
     **/
    public FrameworkWizardUI(String[] args) {
        super();
        init(args);
    }

    /**
     This method initializes this
     
     
     **/
    private void init(String[] args) {
        //
        // Set current workspace and check
        // Check if exists WORKSPACE
        //
        Workspace.setCurrentWorkspace(System.getenv("WORKSPACE"));
        this.checkWorkspace();

        //
        // Show splash screen
        //
        SplashScreen ss = new SplashScreen();
        ss.setVisible(true);
        
        //
        // Go through args to check if enable log
        //
        for (int index = 0; index < args.length; index++) {
            if (args[index].equals("--log") || args[index].equals("-l")) {
                Log.setSaveLog(true);
            }
        }

        //
        // Init Global Data
        //
        GlobalData.init();

        //
        // Close splash screen
        //
        ss.dispose();
                
        //
        // Init the frame
        //
        this.setSize(DataType.MAIN_FRAME_PREFERRED_SIZE_WIDTH, DataType.MAIN_FRAME_PREFERRED_SIZE_HEIGHT);
        this.setResizable(true);
        this.setJMenuBar(getjJMenuBar());
        this.setContentPane(getJContentPane());
        this.setTitle(DataType.PROJECT_NAME + " " + DataType.PROJECT_VERSION + " " + "- ["
                      + Workspace.getCurrentWorkspace() + "]");
        this.setExitType(1);

        //
        // max the window
        //
        this.setExtendedState(JFrame.MAXIMIZED_BOTH);
    }

    /**
     This method initializes jContentPane
     
     @return javax.swing.JPanel jContentPane
     
     **/
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.add(getJPanelOperation(), null);
            jContentPane.add(getJSplitPane(), null);
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
        //
        // Operations of Menu
        //
        if (arg0.getSource() == this.jMenuItemFileNew) {
            this.fileNew();
        }

        if (arg0.getSource() == this.jMenuItemFileOpen) {
            this.open();
        }

        if (arg0.getSource() == this.jMenuItemFileClose) {
            this.close();
        }

        if (arg0.getSource() == this.jMenuItemFileCloseAll) {
            this.closeAll();
            this.refresh();
            this.makeEmptyTree();
        }

        if (arg0.getSource() == this.jMenuItemFileSave) {
            this.save();
        }

        if (arg0.getSource() == this.jMenuItemFileSaveAs) {
            this.saveAs();
        }

        if (arg0.getSource() == this.jMenuItemFileSaveAll) {
            this.saveAll();
        }

        if (arg0.getSource() == this.jMenuItemFileRefresh) {
            if (this.closeAll() == 0) {
                this.refresh();
                this.makeEmptyTree();
            }
        }

        if (arg0.getSource() == this.jMenuItemFileExit) {
            this.exit();
        }

        if (arg0.getSource() == this.jMenuItemEditFindPpi) {
            this.findPpi();
        }

        if (arg0.getSource() == this.jMenuItemEditFindProtocol) {
            this.findProtocol();
        }

        if (arg0.getSource() == this.jMenuItemEditFindGuid) {
            this.findGuid();
        }

        if (arg0.getSource() == this.jMenuItemEditFindPcd) {
            this.findPcd();
        }

        if (arg0.getSource() == this.jMenuItemEditFindLibraryClass) {
            this.findLibraryClass();
        }

        if (arg0.getSource() == this.jMenuItemEditFindLibraryInstance) {
            this.findLibraryInstance();
        }

        if (arg0.getSource() == jMenuItemToolsBuildPreferences) {
            configBuildPreferences();
        }

        if (arg0.getSource() == this.jMenuItemProjectChangeWorkspace) {
            this.changeWorkspace();
        }

        if (arg0.getSource() == this.jMenuItemProjectCreateFar) {
            this.createFar();
        }

        if (arg0.getSource() == this.jMenuItemProjectInstallFar) {
            this.installFar();
        }

        if (arg0.getSource() == this.jMenuItemProjectRemoveFar) {
            this.removeFar();
        }

        if (arg0.getSource() == this.jMenuItemProjectUpdateFar) {
            this.updateFar();
        }

        if (arg0.getSource() == this.jMenuItemToolsClone) {
            this.cloneItem();
        }

        if (arg0.getSource() == this.jMenuItemToolsToolChainConfiguration) {
            this.setupToolChainConfiguration();
        }

        if (arg0.getSource() == this.jMenuItemToolsGenerateGuidsXref) {
            this.generateGuidsXref();
        }

        if (arg0.getSource() == this.jMenuItemHelpAbout) {
            About a = new About(this, true);
            int result = a.showDialog();
            if (result == DataType.RETURN_TYPE_OK) {
                a.dispose();
            }
        }
    }

    /**
     Create an empty tree if no file is open
     
     **/
    private void makeEmptyTree() {
        //
        // Make root
        //
        dmtnRoot = new IDefaultMutableTreeNode("WORKSPACE", IDefaultMutableTreeNode.WORKSPACE, false, null, null);

        //
        // Make Module Description
        //
        dmtnModuleDescription = new IDefaultMutableTreeNode("Modules", IDefaultMutableTreeNode.MODULE_DESCRIPTION,
                                                            false, null, dmtnRoot);

        //
        // First add package
        //
        if (GlobalData.vPackageList.size() > 0) {
            for (int index = 0; index < GlobalData.vPackageList.size(); index++) {
                IDefaultMutableTreeNode dmtnModulePackage = null;
                IDefaultMutableTreeNode dmtnModulePackageLibrary = null;
                IDefaultMutableTreeNode dmtnModulePackageModule = null;

                dmtnModulePackage = new IDefaultMutableTreeNode(GlobalData.vPackageList.elementAt(index).getName(),
                                                                IDefaultMutableTreeNode.MODULE_PACKAGE, false,
                                                                GlobalData.vPackageList.elementAt(index),
                                                                this.dmtnModuleDescription);
                dmtnModulePackageLibrary = new IDefaultMutableTreeNode("Library",
                                                                       IDefaultMutableTreeNode.MODULE_PACKAGE_LIBRARY,
                                                                       false, GlobalData.vPackageList.elementAt(index),
                                                                       this.dmtnModuleDescription);
                dmtnModulePackageModule = new IDefaultMutableTreeNode("Module",
                                                                      IDefaultMutableTreeNode.MODULE_PACKAGE_MODULE,
                                                                      false, GlobalData.vPackageList.elementAt(index),
                                                                      this.dmtnModuleDescription);
                //
                // And then add each module in its package
                //
                Vector<ModuleIdentification> vModule = wt.getAllModules(GlobalData.vPackageList.elementAt(index));
                for (int indexJ = 0; indexJ < vModule.size(); indexJ++) {
                    if (vModule.get(indexJ).isLibrary()) {
                        dmtnModulePackageLibrary.add(new IDefaultMutableTreeNode(vModule.get(indexJ).getName(),
                                                                                 IDefaultMutableTreeNode.MODULE, false,
                                                                                 vModule.get(indexJ),
                                                                                 this.dmtnModuleDescription));
                    } else {
                        dmtnModulePackageModule.add(new IDefaultMutableTreeNode(vModule.get(indexJ).getName(),
                                                                                IDefaultMutableTreeNode.MODULE, false,
                                                                                vModule.get(indexJ),
                                                                                this.dmtnModuleDescription));
                    }
                }
                if (dmtnModulePackageModule.getChildCount() > 0) {
                    dmtnModulePackage.add(dmtnModulePackageModule);
                }
                if (dmtnModulePackageLibrary.getChildCount() > 0) {
                    dmtnModulePackage.add(dmtnModulePackageLibrary);
                }

                dmtnModuleDescription.add(dmtnModulePackage);
            }
        }

        //
        // Make Package Description
        //
        dmtnPackageDescription = new IDefaultMutableTreeNode("Packages", IDefaultMutableTreeNode.PACKAGE_DESCRIPTION,
                                                             false, null, this.dmtnRoot);
        if (GlobalData.vPackageList.size() > 0) {
            for (int index = 0; index < GlobalData.vPackageList.size(); index++) {
                dmtnPackageDescription.add(new IDefaultMutableTreeNode(GlobalData.vPackageList.elementAt(index)
                                                                                              .getName(),
                                                                       IDefaultMutableTreeNode.PACKAGE, false,
                                                                       GlobalData.vPackageList.elementAt(index),
                                                                       this.dmtnPackageDescription));
            }
        }

        //
        // Make Platform Description
        //
        dmtnPlatformDescription = new IDefaultMutableTreeNode("Platforms",
                                                              IDefaultMutableTreeNode.PLATFORM_DESCRIPTION, false,
                                                              null, this.dmtnRoot);
        if (GlobalData.vPlatformList.size() > 0) {
            for (int index = 0; index < GlobalData.vPlatformList.size(); index++) {
                dmtnPlatformDescription.add(new IDefaultMutableTreeNode(GlobalData.vPlatformList.elementAt(index)
                                                                                                .getName(),
                                                                        IDefaultMutableTreeNode.PLATFORM, false,
                                                                        GlobalData.vPlatformList.elementAt(index),
                                                                        this.dmtnPlatformDescription));
            }
        }

        //
        // Add sub nodes to root node
        //
        dmtnRoot.add(dmtnModuleDescription);
        dmtnRoot.add(dmtnPackageDescription);
        dmtnRoot.add(dmtnPlatformDescription);
        iTree = new ITree(dmtnRoot);
        iTree.addMouseListener(this);
        iTree.addKeyListener(this);
        jScrollPaneTree.setViewportView(iTree);
    }

    /* (non-Javadoc)
     * @see java.awt.event.WindowListener#windowClosing(java.awt.event.WindowEvent)
     *
     * Override windowClosing to popup warning message to confirm quit
     * 
     */
    public void windowClosing(WindowEvent arg0) {
        this.exit();
    }

    /* (non-Javadoc)
     * @see java.awt.event.MouseListener#mouseClicked(java.awt.event.MouseEvent)
     * 
     * Override mouseClicked to check if need display popup menu
     * 
     */
    public void mouseClicked(MouseEvent arg0) {
        //
        // When double click
        //
        if (arg0.getClickCount() == 2) {
            doubleClickModuleTreeNode();
        }
    }

    public void mouseEntered(MouseEvent arg0) {
        // TODO Auto-generated method stub
    }

    public void mouseExited(MouseEvent arg0) {
        // TODO Auto-generated method stub
    }

    public void mousePressed(MouseEvent arg0) {
        // TODO Auto-generated method stub
    }

    public void mouseReleased(MouseEvent arg0) {
        // TODO Auto-generated method stub
    }

    /**
     Remove all Internal Frame of Module Desktop Pane
     
     **/
    private void cleanDesktopPaneModule() {
        if (jDesktopPaneModule != null) {
            JInternalFrame[] iif = this.jDesktopPaneModule.getAllFrames();
            for (int index = 0; index < iif.length; index++) {
                iif[index].dispose();
            }
        }
    }

    /**
     Remove all Internal Frame of package Desktop Pane
     
     **/
    private void cleanDesktopPanePackage() {
        if (jDesktopPanePlatform != null) {
            JInternalFrame[] iif = this.jDesktopPanePackage.getAllFrames();
            for (int index = 0; index < iif.length; index++) {
                iif[index].dispose();
            }
        }
    }

    /**
     Remove all Internal Frame of platform Desktop Pane
     
     **/
    private void cleanDesktopPanePlatform() {
        if (jDesktopPanePlatform != null) {
            JInternalFrame[] iif = this.jDesktopPanePlatform.getAllFrames();
            for (int index = 0; index < iif.length; index++) {
                iif[index].dispose();
            }
        }
    }

    /**
     Remove all Internal Frame of all Desktop Panes
     
     **/
    private void cleanDesktopPane() {
        cleanDesktopPaneModule();
        cleanDesktopPanePackage();
        cleanDesktopPanePlatform();
    }

    /**
     Set file filter as input ext
     
     @param ext
     @return
     
     **/
    private int getNewFilePath(String ext) {
        JFileChooser fc = new JFileChooser(Workspace.getCurrentWorkspace());
        fc.setAcceptAllFileFilterUsed(false);
        fc.addChoosableFileFilter(new IFileFilter(ext));
        return fc.showSaveDialog(new JPanel());
    }

    /**
     Add a module to tree
     
     @param mid The module node to be added
     
     **/
    private void addModuleToTree(ModuleIdentification mid) {
        //
        // Add new MsaHeader node to the tree
        //
        IDefaultMutableTreeNode node = new IDefaultMutableTreeNode(mid.getName(), IDefaultMutableTreeNode.MODULE,
                                                                   false, mid, this.dmtnModuleDescription);
        //
        // First find the module belongs to which package
        //
        IDefaultMutableTreeNode packageNode = iTree.getNodeById(dmtnModuleDescription, mid.getPackageId(),
                                                                IDefaultMutableTreeNode.MODULE_PACKAGE);
        //
        // To check if has module node or library node
        //
        IDefaultMutableTreeNode parentModuleNode = null;
        IDefaultMutableTreeNode parentLibraryNode = null;
        boolean hasModule = false;
        boolean hasLibrary = false;
        for (int index = 0; index < packageNode.getChildCount(); index++) {
            IDefaultMutableTreeNode iNode = (IDefaultMutableTreeNode) packageNode.getChildAt(index);
            if (iNode.getCategory() == IDefaultMutableTreeNode.MODULE_PACKAGE_LIBRARY) {
                hasLibrary = true;
                parentLibraryNode = iNode;
            }
            if (iNode.getCategory() == IDefaultMutableTreeNode.MODULE_PACKAGE_MODULE) {
                hasModule = true;
                parentModuleNode = iNode;
            }
        }

        //
        // If is a module
        //
        if (!mid.isLibrary()) {
            //
            // Create parent node first if has no parent node
            //
            if (!hasModule) {
                parentModuleNode = new IDefaultMutableTreeNode("Module", IDefaultMutableTreeNode.MODULE_PACKAGE_MODULE,
                                                               false, mid.getPackageId(), this.dmtnModuleDescription);
                iTree.addNode(packageNode, parentModuleNode);
            }

            iTree.addNode(parentModuleNode, node);
        }

        //
        // If is a Library
        //
        if (mid.isLibrary()) {
            //
            // Create parent node first if has no parent node
            //
            if (!hasLibrary) {
                parentLibraryNode = new IDefaultMutableTreeNode("Library",
                                                                IDefaultMutableTreeNode.MODULE_PACKAGE_LIBRARY, false,
                                                                mid.getPackageId(), this.dmtnModuleDescription);
                iTree.addNode(packageNode, parentLibraryNode);
            }

            iTree.addNode(parentLibraryNode, node);
        }
    }

    /**
     Open Module

     @param path input file path
     
     **/
    private void openModule(String path, IDefaultMutableTreeNode belongNode) {
        ModuleIdentification id = GlobalData.openingModuleList.getIdByPath(path);
        if (id == null) {
            //
            // The module is not in existing packages
            //
            Log.wrn("Open Module", "The module does not belong to any package in the current workspace!");
            return;
        }

        //
        // Make the node selected
        //
        iTree.setSelectionPath(iTree.getPathOfNode(iTree.getNodeById(belongNode, id, IDefaultMutableTreeNode.MODULE)));

        //
        // Update opening Module list information
        //
        if (!iTree.getSelectNode().isOpening()) {
            //
            // Insert sub node of module
            //
            insertModuleTreeNode(id, belongNode);
            iTree.getSelectNode().setOpening(true);

            //
            // Update opening module list
            //
            GlobalData.openingModuleList.setModuleOpen(id, true);
            Set<TreePath> temp = GlobalData.openingModuleList.getTreePathById(id);
            temp.add(iTree.getSelectionPath());
            GlobalData.openingModuleList.setTreePathById(id, temp);
        }
        //
        // Select msa header node and show it in editor panel
        //
        iTree
             .setSelectionPath(iTree
                                    .getPathOfNode(iTree
                                                        .getNodeById(belongNode, id, IDefaultMutableTreeNode.MSA_HEADER)));
        showModuleElement(IDefaultMutableTreeNode.MSA_HEADER, GlobalData.openingModuleList.getOpeningModuleById(id));
        this.currentOpeningModuleIndex = GlobalData.openingModuleList.findIndexOfListById(id);
    }

    /**
     Open Package

     @param path input file path
     
     **/
    private void openPackage(String path) {
        PackageIdentification id = GlobalData.openingPackageList.getIdByPath(path);
        if (id == null) {
            //
            // The package is not in current workspace
            //
            Log.wrn("Open Package", "The package has not been installed in the current workspace!");
            return;
        }

        //
        // Make the node selected
        //
        iTree.setSelectionPath(iTree.getPathOfNode(iTree.getNodeById(this.dmtnPackageDescription, id,
                                                                     IDefaultMutableTreeNode.PACKAGE)));
        //
        // Update opening package list information
        //
        if (!GlobalData.openingPackageList.getPackageOpen(id)) {
            //
            // Insert sub node of module
            //
            insertPackageTreeNode(id);
            iTree.getSelectNode().setOpening(true);

            //
            // Update opening module list
            //
            GlobalData.openingPackageList.setPackageOpen(id, true);
            Set<TreePath> temp = GlobalData.openingPackageList.getTreePathById(id);
            temp.add(iTree.getSelectionPath());
            GlobalData.openingPackageList.setTreePathById(id, temp);
        }
        //
        // Show spd header in editor panel
        //
        iTree.setSelectionPath(iTree.getPathOfNode(iTree.getNodeById(this.dmtnPackageDescription, id,
                                                                     IDefaultMutableTreeNode.SPD_HEADER)));
        showPackageElement(IDefaultMutableTreeNode.SPD_HEADER, GlobalData.openingPackageList.getOpeningPackageById(id));
        this.currentOpeningPackageIndex = GlobalData.openingPackageList.findIndexOfListById(id);
    }

    /**
     Open Package

     @param path input file path
     
     **/
    private void openPlatform(String path) {
        PlatformIdentification id = GlobalData.openingPlatformList.getIdByPath(path);
        if (id == null) {
            //
            // The platform is not in current workspace
            //
            Log.wrn("Open Platform", "The platform has not been installed in the current workspace!");
            return;
        }

        //
        // Make the node selected
        //
        iTree.setSelectionPath(iTree.getPathOfNode(iTree.getNodeById(this.dmtnPlatformDescription, id,
                                                                     IDefaultMutableTreeNode.PLATFORM)));
        //
        // Update opening package list information
        //
        if (!GlobalData.openingPlatformList.getPlatformOpen(id)) {
            //
            // Insert sub node of module
            //
            insertPlatformTreeNode(id);
            iTree.getSelectNode().setOpening(true);

            //
            // Update opening module list
            //
            GlobalData.openingPlatformList.setPlatformOpen(id, true);
            Set<TreePath> temp = GlobalData.openingPlatformList.getTreePathById(id);
            temp.add(iTree.getSelectionPath());
            GlobalData.openingPlatformList.setTreePathById(id, temp);
        }
        //
        // Show fpd header in editor panel
        //
        iTree.setSelectionPath(iTree.getPathOfNode(iTree.getNodeById(this.dmtnPlatformDescription, id,
                                                                     IDefaultMutableTreeNode.FPD_PLATFORMHEADER)));
        showPlatformElement(IDefaultMutableTreeNode.FPD_PLATFORMHEADER,
                            GlobalData.openingPlatformList.getOpeningPlatformById(id));
        this.currentOpeningPlatformIndex = GlobalData.openingPlatformList.findIndexOfListById(id);
    }

    /**
     Save module 
     
     **/
    private void saveModule(int index) {
        OpeningModuleType omt = GlobalData.openingModuleList.getOpeningModuleByIndex(index);
        if (omt.isNew()) {
            if (getNewFilePath(DataType.MODULE_SURFACE_AREA_EXT) != JFileChooser.APPROVE_OPTION) {
                return;
            }
        }
        try {
            SaveFile.saveMsaFile(omt.getId().getPath(), omt.getXmlMsa());
            GlobalData.openingModuleList.setNew(omt.getId(), false);
            GlobalData.openingModuleList.setModuleSaved(omt.getId(), true);
        } catch (Exception e) {
            Log.wrn("Save Module", e.getMessage());
            Log.err("Save Module", e.getMessage());
        }
    }

    /**
     Save package 
     
     **/
    private void savePackage(int index) {
        OpeningPackageType opt = GlobalData.openingPackageList.getOpeningPackageByIndex(index);
        if (opt.isNew()) {
            if (getNewFilePath(DataType.PACKAGE_SURFACE_AREA_EXT) != JFileChooser.APPROVE_OPTION) {
                return;
            }
        }
        try {
            SaveFile.saveSpdFile(opt.getId().getPath(), opt.getXmlSpd());
            GlobalData.openingPackageList.setNew(opt.getId(), false);
            GlobalData.openingPackageList.setPackageSaved(opt.getId(), true);
        } catch (Exception e) {
            Log.wrn("Save Package", e.getMessage());
            Log.err("Save Package", e.getMessage());
        }
    }

    /**
     Save platform 
     
     **/
    private void savePlatform(int index) {
        OpeningPlatformType opt = GlobalData.openingPlatformList.getOpeningPlatformByIndex(index);
        if (opt.isNew()) {
            if (getNewFilePath(DataType.PACKAGE_SURFACE_AREA_EXT) != JFileChooser.APPROVE_OPTION) {
                return;
            }
        }
        try {
            SaveFile.saveFpdFile(opt.getId().getPath(), opt.getXmlFpd());
            GlobalData.openingPlatformList.setNew(opt.getId(), false);
            GlobalData.openingPlatformList.setPlatformSaved(opt.getId(), true);
        } catch (Exception e) {
            Log.wrn("Save Package", e.getMessage());
            Log.err("Save Package", e.getMessage());
        }
    }

    public void componentResized(ComponentEvent arg0) {
        if (this.jSplitPane != null) {
            this.jSplitPane.setSize(this.getWidth() - DataType.MAIN_FRAME_WIDTH_SPACING,
                                    this.getHeight() - DataType.MAIN_FRAME_HEIGHT_SPACING);
            this.jSplitPane.validate();
            resizeDesktopPanel();
        }
    }

    /**
     Resize JDesktopPanel
     
     */
    private void resizeDesktopPanel() {
        resizeDesktopPanel(this.jDesktopPaneModule);
        resizeDesktopPanel(this.jDesktopPanePackage);
        resizeDesktopPanel(this.jDesktopPanePlatform);
    }

    /**
     Resize JDesktopPanel
     
     */
    private void resizeDesktopPanel(JDesktopPane jdk) {
        JInternalFrame[] iif = jdk.getAllFrames();
        for (int index = 0; index < iif.length; index++) {
            iif[index].setSize(jdk.getWidth(), jdk.getHeight());
        }
    }

    private void insertModuleTreeNode(Identification id, IDefaultMutableTreeNode belongNode) {
        iTree.addNode(new IDefaultMutableTreeNode("Module Header", IDefaultMutableTreeNode.MSA_HEADER, true, id,
                                                  belongNode));
        iTree.addNode(new IDefaultMutableTreeNode("Source Files", IDefaultMutableTreeNode.MSA_SOURCEFILES, true, id,
                                                  belongNode));
        iTree.addNode(new IDefaultMutableTreeNode("Library Class Definitions",
                                                  IDefaultMutableTreeNode.MSA_LIBRARYCLASSDEFINITIONS, true, id,
                                                  belongNode));
        iTree
             .addNode(new IDefaultMutableTreeNode("Package Dependencies",
                                                  IDefaultMutableTreeNode.MSA_PACKAGEDEPENDENCIES, true, id, belongNode));
        iTree.addNode(new IDefaultMutableTreeNode("Protocols", IDefaultMutableTreeNode.MSA_PROTOCOLS, true, id,
                                                  belongNode));
        iTree.addNode(new IDefaultMutableTreeNode("Events", IDefaultMutableTreeNode.MSA_EVENTS, true, id, belongNode));
        iTree.addNode(new IDefaultMutableTreeNode("Hobs", IDefaultMutableTreeNode.MSA_HOBS, true, id, belongNode));
        iTree.addNode(new IDefaultMutableTreeNode("Ppis", IDefaultMutableTreeNode.MSA_PPIS, true, id, belongNode));
        iTree.addNode(new IDefaultMutableTreeNode("Variables", IDefaultMutableTreeNode.MSA_VARIABLES, true, id,
                                                  belongNode));
        iTree.addNode(new IDefaultMutableTreeNode("Boot Modes", IDefaultMutableTreeNode.MSA_BOOTMODES, true, id,
                                                  belongNode));
        iTree.addNode(new IDefaultMutableTreeNode("System Tables", IDefaultMutableTreeNode.MSA_SYSTEMTABLES, true, id,
                                                  belongNode));
        iTree.addNode(new IDefaultMutableTreeNode("Data Hubs", IDefaultMutableTreeNode.MSA_DATAHUBS, true, id,
                                                  belongNode));
        iTree.addNode(new IDefaultMutableTreeNode("Hii Packages", IDefaultMutableTreeNode.MSA_HIIPACKAGES, true, id,
                                                  belongNode));
        iTree.addNode(new IDefaultMutableTreeNode("Guids", IDefaultMutableTreeNode.MSA_GUIDS, true, id, belongNode));
        iTree.addNode(new IDefaultMutableTreeNode("External Defintions", IDefaultMutableTreeNode.MSA_EXTERNS, true, id,
                                                  belongNode));
        iTree.addNode(new IDefaultMutableTreeNode("Pcd Coded", IDefaultMutableTreeNode.MSA_PCDS, true, id, belongNode));
        iTree.addNode(new IDefaultMutableTreeNode("Build Options", IDefaultMutableTreeNode.MSA_BUILDOPTIONS, true, id, belongNode));
    }

    private void insertPackageTreeNode(Identification id) {
        IDefaultMutableTreeNode idmtTemp = this.dmtnPackageDescription;
        iTree.addNode(new IDefaultMutableTreeNode("Package Header", IDefaultMutableTreeNode.SPD_HEADER, true, id,
                                                  idmtTemp));
        iTree.addNode(new IDefaultMutableTreeNode("Library Class Declarations",
                                                  IDefaultMutableTreeNode.SPD_LIBRARYCLASSDECLARATIONS, true, id,
                                                  idmtTemp));
        iTree.addNode(new IDefaultMutableTreeNode("Msa Files", IDefaultMutableTreeNode.SPD_MSAFILES, false, id,
                                                  idmtTemp));
        iTree.addNode(new IDefaultMutableTreeNode("Package Includes", IDefaultMutableTreeNode.SPD_PACKAGEHEADERS, true,
                                                  id, idmtTemp));
        iTree.addNode(new IDefaultMutableTreeNode("Guid Declarations", IDefaultMutableTreeNode.SPD_GUIDDECLARATIONS,
                                                  true, id, idmtTemp));
        iTree
             .addNode(new IDefaultMutableTreeNode("Protocol Declarations",
                                                  IDefaultMutableTreeNode.SPD_PROTOCOLDECLARATIONS, true, id, idmtTemp));
        iTree.addNode(new IDefaultMutableTreeNode("Ppi Declarations", IDefaultMutableTreeNode.SPD_PPIDECLARATIONS,
                                                  true, id, idmtTemp));
        iTree.addNode(new IDefaultMutableTreeNode("Pcd Declarations", IDefaultMutableTreeNode.SPD_PCDDECLARATIONS,
                                                  true, id, idmtTemp));
        //
        // Add modules in this package
        //
        IDefaultMutableTreeNode dmtnModulePackageLibrary = null;
        IDefaultMutableTreeNode dmtnModulePackageModule = null;

        dmtnModulePackageLibrary = new IDefaultMutableTreeNode("Library",
                                                               IDefaultMutableTreeNode.MODULE_PACKAGE_LIBRARY, false,
                                                               id, idmtTemp);
        dmtnModulePackageModule = new IDefaultMutableTreeNode("Module", IDefaultMutableTreeNode.MODULE_PACKAGE_MODULE,
                                                              false, id, idmtTemp);

        Vector<ModuleIdentification> vModule = wt.getAllModules(new PackageIdentification(id));
        for (int indexJ = 0; indexJ < vModule.size(); indexJ++) {
            if (vModule.get(indexJ).isLibrary()) {
                dmtnModulePackageLibrary.add(new IDefaultMutableTreeNode(vModule.get(indexJ).getName(),
                                                                         IDefaultMutableTreeNode.MODULE, false,
                                                                         vModule.get(indexJ), idmtTemp));
            } else {
                dmtnModulePackageModule.add(new IDefaultMutableTreeNode(vModule.get(indexJ).getName(),
                                                                        IDefaultMutableTreeNode.MODULE, false,
                                                                        vModule.get(indexJ), idmtTemp));
            }
        }
        if (dmtnModulePackageModule.getChildCount() > 0) {
            iTree.addNode(dmtnModulePackageModule);
        }
        if (dmtnModulePackageLibrary.getChildCount() > 0) {
            iTree.addNode(dmtnModulePackageLibrary);
        }
    }

    private void insertPlatformTreeNode(Identification id) {
        IDefaultMutableTreeNode idmtTemp = this.dmtnPlatformDescription;
        iTree.addNode(new IDefaultMutableTreeNode("Platform Header", IDefaultMutableTreeNode.FPD_PLATFORMHEADER, true,
                                                  id, idmtTemp));
        iTree.addNode(new IDefaultMutableTreeNode("Platform Definitions",
                                                  IDefaultMutableTreeNode.FPD_PLATFORMDEFINITIONS, true, id, idmtTemp));
        iTree.addNode(new IDefaultMutableTreeNode("Flash Information", IDefaultMutableTreeNode.FPD_FLASH, true, id,
                                                  idmtTemp));
        iTree.addNode(new IDefaultMutableTreeNode("Framework Modules", IDefaultMutableTreeNode.FPD_FRAMEWORKMODULES,
                                                  true, id, idmtTemp));
        iTree.addNode(new IDefaultMutableTreeNode("Dynamic PCD Build Declarations",
                                                  IDefaultMutableTreeNode.FPD_PCDDYNAMICBUILDDECLARATIONS, true, id,
                                                  idmtTemp));
        iTree.addNode(new IDefaultMutableTreeNode("Build Options", IDefaultMutableTreeNode.FPD_BUILDOPTIONS, true, id,
                                                  idmtTemp));

        //
        // Add modules in this platform
        //
        IDefaultMutableTreeNode dmtnModulePackageLibrary = null;
        IDefaultMutableTreeNode dmtnModulePackageModule = null;

        dmtnModulePackageLibrary = new IDefaultMutableTreeNode("Library",
                                                               IDefaultMutableTreeNode.MODULE_PACKAGE_LIBRARY, false,
                                                               id, idmtTemp);
        dmtnModulePackageModule = new IDefaultMutableTreeNode("Module", IDefaultMutableTreeNode.MODULE_PACKAGE_MODULE,
                                                              false, id, idmtTemp);

        Vector<ModuleIdentification> vModule = wt.getAllModules(new PlatformIdentification(id));
        for (int indexJ = 0; indexJ < vModule.size(); indexJ++) {
            if (vModule.get(indexJ).isLibrary()) {
                dmtnModulePackageLibrary.add(new IDefaultMutableTreeNode(vModule.get(indexJ).getName(),
                                                                         IDefaultMutableTreeNode.MODULE, false,
                                                                         vModule.get(indexJ), idmtTemp));
            } else {
                dmtnModulePackageModule.add(new IDefaultMutableTreeNode(vModule.get(indexJ).getName(),
                                                                        IDefaultMutableTreeNode.MODULE, false,
                                                                        vModule.get(indexJ), idmtTemp));
            }
        }
        if (dmtnModulePackageModule.getChildCount() > 0) {
            iTree.addNode(dmtnModulePackageModule);
        }
        if (dmtnModulePackageLibrary.getChildCount() > 0) {
            iTree.addNode(dmtnModulePackageLibrary);
        }
    }

    /**
     Operate when double click a tree node
     
     **/
    private void doubleClickModuleTreeNode() {
        Identification id = null;
        int intCategory = -1;
        String path = null;
        IDefaultMutableTreeNode belongNode = null;

        try {
            //
            // Get selected tree node
            //
            if (iTree.getSelectNode() != null) {
                id = iTree.getSelectNode().getId();
            }

            //
            // If id is null, return directly
            //
            if (id == null) {
                return;
            }

            intCategory = iTree.getSelectCategory();
            belongNode = iTree.getSelectNode().getBelongNode();

            //              
            // If the node is not opened yet
            // Insert top level elements first
            //
            if (intCategory == IDefaultMutableTreeNode.MODULE) {
                path = iTree.getSelectNode().getId().getPath();
                openModule(path, belongNode);
                return;
            }
            if (intCategory == IDefaultMutableTreeNode.PACKAGE) {
                path = iTree.getSelectNode().getId().getPath();
                openPackage(path);
                return;
            }
            if (intCategory == IDefaultMutableTreeNode.PLATFORM) {
                path = iTree.getSelectNode().getId().getPath();
                openPlatform(path);
                return;
            }

            //
            // Show editor panel
            //
            if (intCategory >= IDefaultMutableTreeNode.MSA_HEADER && intCategory < IDefaultMutableTreeNode.SPD_HEADER) {
                showModuleElement(intCategory,
                                  GlobalData.openingModuleList.getOpeningModuleById(new ModuleIdentification(id)));
                this.currentOpeningModuleIndex = GlobalData.openingModuleList
                                                                             .findIndexOfListById(new ModuleIdentification(
                                                                                                                           id));
            }
            if (intCategory >= IDefaultMutableTreeNode.SPD_HEADER
                && intCategory < IDefaultMutableTreeNode.FPD_PLATFORMHEADER) {
                showPackageElement(intCategory,
                                   GlobalData.openingPackageList.getOpeningPackageById(new PackageIdentification(id)));
                this.currentOpeningPackageIndex = GlobalData.openingPackageList
                                                                               .findIndexOfListById(new PackageIdentification(
                                                                                                                              id));
            }
            if (intCategory >= IDefaultMutableTreeNode.FPD_PLATFORMHEADER) {
                showPlatformElement(
                                    intCategory,
                                    GlobalData.openingPlatformList
                                                                  .getOpeningPlatformById(new PlatformIdentification(id)));
                this.currentOpeningPlatformIndex = GlobalData.openingPlatformList
                                                                                 .findIndexOfListById(new PlatformIdentification(
                                                                                                                                 id));
            }
        } catch (Exception e) {
            Log.err("double click category: " + intCategory);
            Log.err("double click belong node: " + belongNode.toString());
            Log.err("double click id path: " + id);
            Log.err("double click exception: " + e.getMessage());
        }
    }

    /**
     Show content of editor panel via selected element
     
     @param elementType
     @param fpd
     
     **/
    private void showPlatformElement(int elementType, OpeningPlatformType fpd) {
        this.cleanDesktopPanePlatform();

        switch (elementType) {
        case IDefaultMutableTreeNode.FPD_PLATFORMDEFINITIONS:
            FpdPlatformDefs frmFpdPlatformDefs = new FpdPlatformDefs(fpd);
            getJDesktopPanePlatform().add(frmFpdPlatformDefs, 1);
            break;
        case IDefaultMutableTreeNode.FPD_PLATFORMHEADER:
            FpdHeader frmFpdHeader = new FpdHeader(fpd);
            getJDesktopPanePlatform().add(frmFpdHeader, 1);
            break;
        case IDefaultMutableTreeNode.FPD_FLASH:
            FpdFlash frmFpdFlash = new FpdFlash(fpd);
            getJDesktopPanePlatform().add(frmFpdFlash, 1);
            break;
        case IDefaultMutableTreeNode.FPD_FRAMEWORKMODULES:
            FpdFrameworkModules frmFpdFrameworkModules = new FpdFrameworkModules(fpd);
            getJDesktopPanePlatform().add(frmFpdFrameworkModules, 1);
            break;
        case IDefaultMutableTreeNode.FPD_PCDDYNAMICBUILDDECLARATIONS:
            FpdDynamicPcdBuildDefinitions frmFpdDynamicPcdBuildDefinitions = new FpdDynamicPcdBuildDefinitions(fpd);
            getJDesktopPanePlatform().add(frmFpdDynamicPcdBuildDefinitions, 1);
            break;
        case IDefaultMutableTreeNode.FPD_BUILDOPTIONS:
            FpdBuildOptions frmFpdBuildOptions = new FpdBuildOptions(fpd);
            getJDesktopPanePlatform().add(frmFpdBuildOptions, 1);
            break;
        }
        this.jTabbedPaneEditor.setSelectedIndex(2);
        resizeDesktopPanel();
    }

    /**
     Show content of editor panel via selected element
     
     @param elementType
     @param spd
     
     */
    private void showPackageElement(int elementType, OpeningPackageType spd) {
        this.cleanDesktopPanePackage();
        Tools.dirForNewSpd = spd.getId().getPath();
        switch (elementType) {
        case IDefaultMutableTreeNode.SPD_HEADER:
            SpdHeader frmSpdHeader = new SpdHeader(spd);
            getJDesktopPanePackage().add(frmSpdHeader, 1);
            break;
        case IDefaultMutableTreeNode.SPD_LIBRARYCLASSDECLARATIONS:
            SpdLibClassDecls frmSlcd = new SpdLibClassDecls(spd, this);
            getJDesktopPanePackage().add(frmSlcd, 1);
            break;
        case IDefaultMutableTreeNode.SPD_MSAFILES:
            SpdMsaFiles frmSmf = new SpdMsaFiles(spd, this);
            getJDesktopPanePackage().add(frmSmf, 1);
            break;
        case IDefaultMutableTreeNode.SPD_PACKAGEHEADERS:
            SpdPackageHeaders frmSph = new SpdPackageHeaders(spd, this);
            getJDesktopPanePackage().add(frmSph, 1);
            break;
        case IDefaultMutableTreeNode.SPD_GUIDDECLARATIONS:
            SpdGuidDecls frmSgd = new SpdGuidDecls(spd, this);
            getJDesktopPanePackage().add(frmSgd, 1);
            break;
        case IDefaultMutableTreeNode.SPD_PROTOCOLDECLARATIONS:
            SpdProtocolDecls frmSprod = new SpdProtocolDecls(spd, this);
            getJDesktopPanePackage().add(frmSprod, 1);
            break;
        case IDefaultMutableTreeNode.SPD_PPIDECLARATIONS:
            SpdPpiDecls frmSppid = new SpdPpiDecls(spd, this);
            getJDesktopPanePackage().add(frmSppid, 1);
            break;
        case IDefaultMutableTreeNode.SPD_PCDDECLARATIONS:
            SpdPcdDefs frmSpcdd = new SpdPcdDefs(spd, this);
            getJDesktopPanePackage().add(frmSpcdd, 1);
            break;
        }
        this.jTabbedPaneEditor.setSelectedIndex(1);
        resizeDesktopPanel();
    }

    /**
     Show content of editor panel via selected element
     
     @param elementType
     @param msa
     
     */
    private void showModuleElement(int elementType, OpeningModuleType msa) {
        this.cleanDesktopPaneModule();
        switch (elementType) {
        case IDefaultMutableTreeNode.MSA_HEADER:
            MsaHeader frmMsaHeader = new MsaHeader(msa);
            getJDesktopPaneModule().add(frmMsaHeader, 1);
            break;
        case IDefaultMutableTreeNode.MSA_LIBRARYCLASSDEFINITIONS:
            ModuleLibraryClassDefinitions frmMlcd = new ModuleLibraryClassDefinitions(msa, this);
            getJDesktopPaneModule().add(frmMlcd, 1);
            break;
        case IDefaultMutableTreeNode.MSA_PACKAGEDEPENDENCIES:
            ModulePackageDependencies frmMpd = new ModulePackageDependencies(msa, this);
            getJDesktopPaneModule().add(frmMpd, 1);
            break;
        case IDefaultMutableTreeNode.MSA_SOURCEFILES:
            ModuleSourceFiles frmMsf = new ModuleSourceFiles(msa, this);
            getJDesktopPaneModule().add(frmMsf, 1);
            break;
        case IDefaultMutableTreeNode.MSA_PROTOCOLS:
            ModuleProtocols frmMp = new ModuleProtocols(msa, this);
            getJDesktopPaneModule().add(frmMp, 1);
            break;
        case IDefaultMutableTreeNode.MSA_EVENTS:
            ModuleEvents frmMe = new ModuleEvents(msa, this);
            getJDesktopPaneModule().add(frmMe, 1);
            break;
        case IDefaultMutableTreeNode.MSA_HOBS:
            ModuleHobs frmMh = new ModuleHobs(msa, this);
            getJDesktopPaneModule().add(frmMh, 1);
            break;
        case IDefaultMutableTreeNode.MSA_PPIS:
            ModulePpis frmMpp = new ModulePpis(msa, this);
            getJDesktopPaneModule().add(frmMpp, 1);
            break;
        case IDefaultMutableTreeNode.MSA_VARIABLES:
            ModuleVariables frmMv = new ModuleVariables(msa, this);
            getJDesktopPaneModule().add(frmMv, 1);
            break;
        case IDefaultMutableTreeNode.MSA_BOOTMODES:
            ModuleBootModes frmMbm = new ModuleBootModes(msa, this);
            getJDesktopPaneModule().add(frmMbm, 1);
            break;
        case IDefaultMutableTreeNode.MSA_SYSTEMTABLES:
            ModuleSystemTables frmMst = new ModuleSystemTables(msa, this);
            getJDesktopPaneModule().add(frmMst, 1);
            break;
        case IDefaultMutableTreeNode.MSA_DATAHUBS:
            ModuleDataHubs frmMdh = new ModuleDataHubs(msa, this);
            getJDesktopPaneModule().add(frmMdh, 1);
            break;
        case IDefaultMutableTreeNode.MSA_HIIPACKAGES:
            ModuleHiiPackages frmMf = new ModuleHiiPackages(msa, this);
            getJDesktopPaneModule().add(frmMf, 1);
            break;
        case IDefaultMutableTreeNode.MSA_GUIDS:
            ModuleGuids frmGuid = new ModuleGuids(msa, this);
            getJDesktopPaneModule().add(frmGuid, 1);
            break;
        case IDefaultMutableTreeNode.MSA_EXTERNS:
            ModuleExterns frmMex = new ModuleExterns(msa, this);
            getJDesktopPaneModule().add(frmMex, 1);
            break;
        case IDefaultMutableTreeNode.MSA_PCDS:
            ModulePCDs frmPcd = new ModulePCDs(msa, this);
            getJDesktopPaneModule().add(frmPcd, 1);
            break;
        case IDefaultMutableTreeNode.MSA_BUILDOPTIONS:
            ModuleBuildOptions frmMbo = new ModuleBuildOptions(msa, this);
            getJDesktopPaneModule().add(frmMbo, 1);
            break;
        }
        this.jTabbedPaneEditor.setSelectedIndex(0);
        resizeDesktopPanel();
    }

    //
    // Define operations of menu
    //
    /**
     New a file
     
     **/
    private void fileNew() {
        int result = -1;
        //
        // Selece new file type
        //
        NewFileChooser nfc = new NewFileChooser(this, true);
        result = nfc.showDialog();

        if (result == DataType.RETURN_TYPE_CANCEL) {
            nfc.dispose();
            return;
        } else if (result == DataType.RETURN_TYPE_MODULE_SURFACE_AREA) {
            //
            // To new a module
            //
            SelectModuleBelong smb = new SelectModuleBelong(this, true, result);
            result = smb.showDialog();

            if (result == DataType.RETURN_TYPE_CANCEL) {
                nfc.dispose();
                smb.dispose();
                return;
            } else if (result == DataType.RETURN_TYPE_OK) {
                ModuleIdentification mid = smb.getMid();
                if (mid != null) {
                    //
                    // Update package of workspace first
                    //
                    PackageSurfaceAreaDocument.PackageSurfaceArea psa = null;
                    if (GlobalData.openingPackageList.existsPackage(mid.getPackageId())) {
                        psa = GlobalData.openingPackageList.getPackageSurfaceAreaFromId(mid.getPackageId());
                    }
                    try {
                        wt.addModuleToPackage(mid, psa);
                    } catch (IOException e) {
                        Log.wrn("Update MsaFiles in Package", e.getMessage());
                        Log.err("Update MsaFiles in Package", e.getMessage());
                        return;
                    } catch (XmlException e) {
                        Log.wrn("Update MsaFiles in Package", e.getMessage());
                        Log.err("Update MsaFiles in Package", e.getMessage());
                        return;
                    } catch (Exception e) {
                        Log.wrn("Update MsaFiles in Package", e.getMessage());
                        Log.err("Update MsaFiles in Package", e.getMessage());
                        return;
                    }

                    //
                    // Update Global Data
                    //
                    GlobalData.openingModuleList.insertToOpeningModuleList(mid, smb.getMsa());
                    GlobalData.vModuleList.addElement(mid);

                    //
                    // Create new node on the tree
                    //
                    addModuleToTree(mid);

                    //
                    // Open the node
                    //
                    this.openModule(mid.getPath(), this.dmtnModuleDescription);
                }
            }
        } else if (result == DataType.RETURN_TYPE_PACKAGE_SURFACE_AREA) {
            //
            // To new a package
            //
            SelectModuleBelong smb = new SelectModuleBelong(this, true, result);
            result = smb.showDialog();

            if (result == DataType.RETURN_TYPE_CANCEL) {
                nfc.dispose();
                smb.dispose();
                return;
            } else if (result == DataType.RETURN_TYPE_OK) {
                PackageIdentification pid = smb.getPid();
                if (pid != null) {
                    try {
                        wt.addPackageToDatabase(smb.getPid());
                    } catch (Exception e) {
                        Log.err("addPackageToDatabase", e.getMessage());
                    }

                    //
                    // Update Global Data
                    //
                    GlobalData.openingPackageList.insertToOpeningPackageList(pid, smb.getSpd());
                    GlobalData.vPackageList.addElement(pid);

                    //
                    // Add to Module Description node
                    //
                    IDefaultMutableTreeNode node = new IDefaultMutableTreeNode(pid.getName(),
                                                                               IDefaultMutableTreeNode.MODULE_PACKAGE,
                                                                               false, pid, this.dmtnModuleDescription);

                    iTree.addNode(dmtnModuleDescription, node);

                    //
                    // Add new SpdHeader node to the tree
                    //
                    node = new IDefaultMutableTreeNode(pid.getName(), IDefaultMutableTreeNode.PACKAGE, true, pid,
                                                       this.dmtnPackageDescription);
                    iTree.addNode(dmtnPackageDescription, node);

                    this.openPackage(pid.getPath());
                }
            }
        } else if (result == DataType.RETURN_TYPE_PLATFORM_SURFACE_AREA) {
            //
            // To new a platform
            //
            SelectModuleBelong smb = new SelectModuleBelong(this, true, result);
            result = smb.showDialog();

            if (result == DataType.RETURN_TYPE_CANCEL) {
                nfc.dispose();
                smb.dispose();
                return;
            } else if (result == DataType.RETURN_TYPE_OK) {
                PlatformIdentification fid = smb.getFid();
                if (fid != null) {
                    try {
                        wt.addPlatformToDatabase(fid);
                    } catch (Exception e) {
                        Log.err("addPlatformToDatabase", e.getMessage());
                    }

                    //
                    // Update global data
                    //
                    GlobalData.openingPlatformList.insertToOpeningPlatformList(fid, smb.getFpd());
                    GlobalData.vPlatformList.addElement(fid);
                    //
                    // Add new SpdHeader node to the tree
                    //
                    IDefaultMutableTreeNode node = new IDefaultMutableTreeNode(fid.getName(),
                                                                               IDefaultMutableTreeNode.PLATFORM, true,
                                                                               fid, this.dmtnPlatformDescription);
                    iTree.addNode(dmtnPlatformDescription, node);
                    this.openPlatform(fid.getPath());
                }
            }
        }
    }

    /**
     Open a file
     
     **/
    private void open() {
        JFileChooser fc = new JFileChooser(Workspace.getCurrentWorkspace());
        fc.setAcceptAllFileFilterUsed(false);
        IFileFilter iffM = new IFileFilter(DataType.MODULE_SURFACE_AREA_EXT);
        IFileFilter iffP = new IFileFilter(DataType.PACKAGE_SURFACE_AREA_EXT);
        IFileFilter iffF = new IFileFilter(DataType.PLATFORM_SURFACE_AREA_EXT);
        fc.addChoosableFileFilter(iffM);
        fc.addChoosableFileFilter(iffP);
        fc.addChoosableFileFilter(iffF);
        fc.setFileFilter(iffM);

        int result = fc.showOpenDialog(new JPanel());

        if (result == JFileChooser.APPROVE_OPTION) {
            String path = fc.getSelectedFile().getPath();
            String match = path.substring(path.length() - 4);
            if (match.equals(DataType.FILE_EXT_SEPARATOR + DataType.MODULE_SURFACE_AREA_EXT)) {
                openModule(path, this.dmtnModuleDescription);
            } else if (match.equals(DataType.FILE_EXT_SEPARATOR + DataType.PACKAGE_SURFACE_AREA_EXT)) {
                openPackage(path);
            } else if (match.equals(DataType.FILE_EXT_SEPARATOR + DataType.PLATFORM_SURFACE_AREA_EXT)) {
                openPlatform(path);
            }
        }
    }

    /**
     Close files
     
     **/
    private void close() {
        TreePath item = null;
        switch (this.jTabbedPaneEditor.getSelectedIndex()) {
        //
        // Current is module
        //
        case 0:
            if (this.currentOpeningModuleIndex > -1) {
                if (!GlobalData.openingModuleList.getModuleSaved(currentOpeningModuleIndex)) {
                    int result = showSaveDialog();
                    if (result == JOptionPane.YES_OPTION) {
                        this.save();
                    }
                    if (result == JOptionPane.NO_OPTION) {
                        // Do nothing
                    }
                    if (result == JOptionPane.CANCEL_OPTION) {
                        return;
                    }
                }

                //
                // Remove all tree paths for the module
                //
                Set<TreePath> openingTreePaths = GlobalData.openingModuleList
                                                                             .getTreePathByIndex(currentOpeningModuleIndex);
                Iterator<TreePath> openingTreePathsIter = openingTreePaths.iterator();
                while (openingTreePathsIter.hasNext()) {
                    item = openingTreePathsIter.next();
                    iTree.getNodeByPath(item).setOpening(false);
                    iTree.removeNodeChildrenByPath(item);
                }

                GlobalData.openingModuleList.reload(this.currentOpeningModuleIndex);
                GlobalData.openingModuleList.setModuleOpen(this.currentOpeningModuleIndex, false);
                GlobalData.openingModuleList.setModuleSaved(this.currentOpeningModuleIndex, true);
                
                this.cleanDesktopPaneModule();
                this.currentOpeningModuleIndex = -1;
            }
            break;
        //
        // Current is package
        //
        case 1:
            if (this.currentOpeningPackageIndex > -1) {
                if (!GlobalData.openingPackageList.getPackageSaved(currentOpeningPackageIndex)) {
                    int result = showSaveDialog();
                    if (result == JOptionPane.YES_OPTION) {
                        this.save();
                    }
                    if (result == JOptionPane.NO_OPTION) {
                        // Do nothing
                    }
                    if (result == JOptionPane.CANCEL_OPTION) {
                        return;
                    }
                }

                //
                // Remove all tree paths for the module
                //
                Set<TreePath> openingTreePaths = GlobalData.openingPackageList
                                                                              .getTreePathByIndex(currentOpeningPackageIndex);
                Iterator<TreePath> openingTreePathsIter = openingTreePaths.iterator();
                while (openingTreePathsIter.hasNext()) {
                    item = openingTreePathsIter.next();
                    iTree.getNodeByPath(item).setOpening(false);
                    iTree.removeNodeChildrenByPath(item);
                }

                GlobalData.openingPackageList.reload(this.currentOpeningPackageIndex);
                GlobalData.openingPackageList.setPackageOpen(this.currentOpeningPackageIndex, false);
                GlobalData.openingPackageList.setPackageSaved(this.currentOpeningPackageIndex, true);
                this.cleanDesktopPanePackage();
                this.currentOpeningPackageIndex = -1;
            }
            break;
        //
        // Current is platform
        //
        case 2:
            if (this.currentOpeningPlatformIndex > -1) {
                if (!GlobalData.openingPlatformList.getPlatformSaved(currentOpeningPlatformIndex)) {
                    int result = showSaveDialog();
                    if (result == JOptionPane.YES_OPTION) {
                        this.save();
                    }
                    if (result == JOptionPane.NO_OPTION) {
                        // Do nothing
                    }
                    if (result == JOptionPane.CANCEL_OPTION) {
                        return;
                    }
                }

                //
                // Remove all tree paths for the module
                //
                Set<TreePath> openingTreePaths = GlobalData.openingPlatformList
                                                                               .getTreePathByIndex(currentOpeningPlatformIndex);
                Iterator<TreePath> openingTreePathsIter = openingTreePaths.iterator();
                while (openingTreePathsIter.hasNext()) {
                    item = openingTreePathsIter.next();
                    iTree.getNodeByPath(item).setOpening(false);
                    iTree.removeNodeChildrenByPath(item);
                }

                GlobalData.openingPlatformList.reload(this.currentOpeningPlatformIndex);
                GlobalData.openingPlatformList.setPlatformOpen(this.currentOpeningPlatformIndex, false);
                GlobalData.openingPlatformList.setPlatformSaved(this.currentOpeningPlatformIndex, true);
                this.cleanDesktopPanePlatform();
                this.currentOpeningPlatformIndex = -1;
            }
            break;
        }
    }

    /**
     Close all opening files and clean all showing internal frame
     
     **/
    private int closeAll() {
        int result = JOptionPane.NO_OPTION;
        if (!GlobalData.openingModuleList.isSaved() || !GlobalData.openingPackageList.isSaved()
            || !GlobalData.openingPlatformList.isSaved()) {
            result = showSaveDialog();
        }
        if (result == JOptionPane.YES_OPTION) {
            this.saveAll();
        }
        if (result == JOptionPane.NO_OPTION) {
            //
            // Do nothing
            //
        }
        if (result == JOptionPane.CANCEL_OPTION || result == JOptionPane.CLOSED_OPTION) {
            return -1;
        }
        this.cleanDesktopPane();
        GlobalData.openingModuleList.closeAll();
        GlobalData.openingPackageList.closeAll();
        GlobalData.openingPlatformList.closeAll();

        return 0;
    }

    /**
     Refresh all global data from disk to memory
     
     **/
    private void refresh() {
        GlobalData.init();
    }

    /**
     Save currentModule when press button OK
     
     **/
    private void save() {
        switch (this.jTabbedPaneEditor.getSelectedIndex()) {
        case 0:
            if (this.currentOpeningModuleIndex > -1) {
                saveModule(this.currentOpeningModuleIndex);
            }
            break;
        case 1:
            if (this.currentOpeningPackageIndex > -1) {
                savePackage(this.currentOpeningPackageIndex);
            }
            break;
        case 2:
            if (this.currentOpeningPlatformIndex > -1) {
                savePlatform(this.currentOpeningPlatformIndex);
            }
            break;
        }
    }

    private void saveAs() {

    }

    private void saveAll() {
        //
        // Save all modules
        //
        for (int index = 0; index < GlobalData.openingModuleList.size(); index++) {
            if (!GlobalData.openingModuleList.getModuleSaved(index)) {
                saveModule(index);
            }
        }

        //
        // Save all packages
        //
        for (int index = 0; index < GlobalData.openingPackageList.size(); index++) {
            if (!GlobalData.openingPackageList.getPackageSaved(index)) {
                savePackage(index);
            }
        }

        //
        // Save all platforms
        //
        for (int index = 0; index < GlobalData.openingPlatformList.size(); index++) {
            if (!GlobalData.openingPlatformList.getPlatformSaved(index)) {
                savePlatform(index);
            }
        }
    }

    /**
     To save changed items before exit.
     
     **/
    private void exit() {
        int result = JOptionPane.NO_OPTION;
        if (!GlobalData.openingModuleList.isSaved() || !GlobalData.openingPackageList.isSaved()
            || !GlobalData.openingPlatformList.isSaved()) {
            result = showSaveDialog();
        }
        if (result == JOptionPane.YES_OPTION) {
            this.saveAll();
        } else if (result == JOptionPane.NO_OPTION) {
            // Do nothing
        } else if (result == JOptionPane.CANCEL_OPTION || result == JOptionPane.CLOSED_OPTION) {
            return;
        }
        this.dispose();
        System.exit(0);
    }

    /**
     To find all defined PPIs in workspace
     
     **/
    private void findPpi() {
        FindResult fr = FindResult.getInstance("PPI");
        fr.setVisible(true);
    }

    /**
     To find all defined PROTOCOLs in workspace
     
     **/
    private void findProtocol() {
        FindResult fr = FindResult.getInstance("PROTOCOL");
        fr.setVisible(true);
    }

    /**
     To find all defined PROTOCOLs in workspace
     
     **/
    private void findGuid() {
        FindResult fr = FindResult.getInstance("GUID");
        fr.setVisible(true);
    }

    /**
     To find all defined PROTOCOLs in workspace
     
     **/
    private void findPcd() {
        FindResult fr = FindResult.getInstance("PCD");
        fr.setVisible(true);
    }

    /**
     To find all defined Library Classes in workspace
     
     **/
    private void findLibraryClass() {
        FindResult fr = FindResult.getInstance("LIBRARY_CLASS");
        fr.setVisible(true);
    }

    /**
     To find all defined Library Instances in workspace
     
     **/
    private void findLibraryInstance() {
        FindResult fr = FindResult.getInstance("LIBRARY_INSTANCE");
        fr.setVisible(true);
    }

    /**
     Switch current workspace to others
     
     **/
    private void changeWorkspace() {
        SwitchWorkspace sw = new SwitchWorkspace(this, true);
        int result = sw.showDialog();
        if (result == DataType.RETURN_TYPE_CANCEL) {
            return;
        } else if (result == DataType.RETURN_TYPE_OK) {
            //
            // Reinit whole window
            //
            this.closeAll();
            this.setTitle(DataType.PROJECT_NAME + " " + DataType.PROJECT_VERSION + " " + "- ["
                          + Workspace.getCurrentWorkspace() + "]");
            //
            // Refrash the tree
            //
            this.refresh();
            this.makeEmptyTree();
        }
        sw.dispose();
    }

    /**
     To create a Far file from current workspace
     
     **/
    private void createFar() {
        CreateStepOne cso = new CreateStepOne(this, true);
        int result = cso.showDialog();
        if (result == DataType.RETURN_TYPE_OK) {
            String strReturn = "Far Creation Completed!";
            JOptionPane.showConfirmDialog(this, strReturn, "Done", JOptionPane.DEFAULT_OPTION,
                                          JOptionPane.INFORMATION_MESSAGE);
        }
        cso.dispose();
    }

    /**
     To install a Far file to current workspace
     
     **/
    private void installFar() {
        InstallStepOne iso = new InstallStepOne(this, true);
        int result = iso.showDialog();
        if (result == DataType.RETURN_TYPE_OK) {
            String strReturn = "<html>Far Installalation completed!<br>Refreshing the WORKSPACE!</html>";
            JOptionPane.showConfirmDialog(this, strReturn, "Done", JOptionPane.DEFAULT_OPTION,
                                          JOptionPane.INFORMATION_MESSAGE);
            this.closeAll();
            this.refresh();
            this.makeEmptyTree();
        }
        iso.dispose();
    }

    /**
     To remove a Far's items from current workspace
     
     **/
    private void removeFar() {
        DeleteStepOne dso = new DeleteStepOne(this, true);
        int result = dso.showDialog();
        if (result == DataType.RETURN_TYPE_OK) {
            String strReturn = "<html>Far Deletion completed!<br>Refreshing the WORKSPACE!</html>";
            JOptionPane.showConfirmDialog(this, strReturn, "Done", JOptionPane.DEFAULT_OPTION,
                                          JOptionPane.INFORMATION_MESSAGE);
            this.closeAll();
            this.refresh();
            this.makeEmptyTree();
        }
        dso.dispose();
    }

    /**
     To update an existing Far file
     
     **/
    private void updateFar() {
        UpdateStepOne uso = new UpdateStepOne(this, true);
        int result = uso.showDialog();
        if (result == DataType.RETURN_TYPE_OK) {
            String strReturn = "<html>Far Update completed!<br>Refreshing the WORKSPACE!</html>";
            JOptionPane.showConfirmDialog(this, strReturn, "Done", JOptionPane.DEFAULT_OPTION,
                                          JOptionPane.INFORMATION_MESSAGE);
            this.closeAll();
            this.refresh();
            this.makeEmptyTree();
        }
        uso.dispose();
    }

    /**
     Show Tool Chain Configuration Dialog to setup Tool Chain
     
     **/
    private void setupToolChainConfiguration() {
        ToolChainConfig tcc = ToolChainConfig.getInstance();
        tcc.showDialog();
    }

    private void configBuildPreferences() {
        Preferences bt = Preferences.getInstance();
        bt.showDialog();
    }

    /**
     Clone selected item
     
     **/
    private void cloneItem() {
        int mode = -1;

        //
        // Check if there is any item can be cloned
        //
        if (iTree.getSelectionPath() == null) {
            Log.wrn("Clone", "Please select a target to clone!");
            return;
        }
        int category = iTree.getSelectCategory();
        Identification id = iTree.getSelectNode().getId();

        if (category == IDefaultMutableTreeNode.WORKSPACE) {
            mode = DataType.RETURN_TYPE_WORKSPACE;
            id = null;
        }
        if ((category == IDefaultMutableTreeNode.MODULE)
            || (category >= IDefaultMutableTreeNode.MSA_HEADER && category < IDefaultMutableTreeNode.SPD_HEADER)) {
            mode = DataType.RETURN_TYPE_MODULE_SURFACE_AREA;
        }
        if ((category == IDefaultMutableTreeNode.PACKAGE)
            || (category >= IDefaultMutableTreeNode.SPD_HEADER && category < IDefaultMutableTreeNode.FPD_PLATFORMHEADER)) {
            mode = DataType.RETURN_TYPE_PACKAGE_SURFACE_AREA;
        }
        if ((category == IDefaultMutableTreeNode.PLATFORM) || (category >= IDefaultMutableTreeNode.FPD_PLATFORMHEADER)) {
            mode = DataType.RETURN_TYPE_PLATFORM_SURFACE_AREA;
        }

        Clone c = new Clone(this, true, mode, id);
        int result = c.showDialog();

        if (result == DataType.RETURN_TYPE_CANCEL) {
            c.dispose();
        }
        if (result == DataType.RETURN_TYPE_WORKSPACE) {
            Tools.showInformationMessage("Workspace Clone Completed!");
        }
        if (result == DataType.RETURN_TYPE_MODULE_SURFACE_AREA) {
            Tools.showInformationMessage("Module Clone Completed!");
            addModuleToTree(c.getMid());
        }
        if (result == DataType.RETURN_TYPE_PACKAGE_SURFACE_AREA) {
            Tools.showInformationMessage("Package Clone Completed!");
            //
            // Add new SpdHeader node to the tree
            //
            IDefaultMutableTreeNode node = new IDefaultMutableTreeNode(GlobalData.vPackageList.lastElement().getName(),
                                                                       IDefaultMutableTreeNode.PACKAGE, false,
                                                                       GlobalData.vPackageList.lastElement(),
                                                                       this.dmtnPackageDescription);
            iTree.addNode(this.dmtnPackageDescription, node);
        }
        if (result == DataType.RETURN_TYPE_PLATFORM_SURFACE_AREA) {
            Tools.showInformationMessage("Platform Surface Area Clone Finished");
            //
            // Add new SpdHeader node to the tree
            //
            IDefaultMutableTreeNode node = new IDefaultMutableTreeNode(
                                                                       GlobalData.vPlatformList.lastElement().getName(),
                                                                       IDefaultMutableTreeNode.PLATFORM, false,
                                                                       GlobalData.vPlatformList.lastElement(),
                                                                       this.dmtnPlatformDescription);
            iTree.addNode(this.dmtnPlatformDescription, node);
        }
        if (result == DataType.RETURN_TYPE_OK) {

        }
    }

    public void valueChanged(TreeSelectionEvent arg0) {
        // TODO Auto-generated method stub

    }

    public void menuCanceled(MenuEvent arg0) {
        // TODO Auto-generated method stub

    }

    public void menuDeselected(MenuEvent arg0) {
        // TODO Auto-generated method stub

    }

    public void menuSelected(MenuEvent arg0) {
        if (arg0.getSource() == jMenuFile) {
            //
            // Enable close/close all if some files are opened
            //
            switch (this.jTabbedPaneEditor.getSelectedIndex()) {
            case 0:
                jMenuItemFileClose
                                  .setEnabled(GlobalData.openingModuleList
                                                                          .getModuleOpen(this.currentOpeningModuleIndex));
                jMenuItemFileSave
                                 .setEnabled(!GlobalData.openingModuleList
                                                                          .getModuleSaved(this.currentOpeningModuleIndex));
                break;
            case 1:
                jMenuItemFileClose
                                  .setEnabled(GlobalData.openingPackageList
                                                                           .getPackageOpen(this.currentOpeningPackageIndex));
                jMenuItemFileSave
                                 .setEnabled(!GlobalData.openingPackageList
                                                                           .getPackageSaved(this.currentOpeningPackageIndex));
                break;
            case 2:
                jMenuItemFileClose
                                  .setEnabled(GlobalData.openingPlatformList
                                                                            .getPlatformOpen(this.currentOpeningPlatformIndex));
                jMenuItemFileSave
                                 .setEnabled(!GlobalData.openingPlatformList
                                                                            .getPlatformSaved(this.currentOpeningPlatformIndex));
                break;
            }
            jMenuItemFileCloseAll.setEnabled(GlobalData.openingModuleList.isOpen()
                                             || GlobalData.openingPackageList.isOpen()
                                             || GlobalData.openingPlatformList.isOpen());

            //
            // Enable save/save all if some files are changed
            //
            jMenuItemFileSaveAll.setEnabled(!GlobalData.openingModuleList.isSaved()
                                            || !GlobalData.openingPackageList.isSaved()
                                            || !GlobalData.openingPlatformList.isSaved());
        }

        if (arg0.getSource() == jMenuTools) {
            //
            // Enable clone when select some items
            //
            if (iTree.getSelectionPath() == null) {
                jMenuItemToolsClone.setEnabled(false);
            } else {
                int category = iTree.getSelectCategory();
                if (category == IDefaultMutableTreeNode.MODULE_DESCRIPTION
                    || category == IDefaultMutableTreeNode.PACKAGE_DESCRIPTION
                    || category == IDefaultMutableTreeNode.PLATFORM_DESCRIPTION
                    || category == IDefaultMutableTreeNode.MODULE_PACKAGE
                    || category == IDefaultMutableTreeNode.MODULE_PACKAGE_LIBRARY
                    || category == IDefaultMutableTreeNode.MODULE_PACKAGE_MODULE) {
                    jMenuItemToolsClone.setEnabled(false);
                } else {
                    jMenuItemToolsClone.setEnabled(true);
                }
            }
        }
    }

    public void keyTyped(KeyEvent arg0) {
        // TODO Auto-generated method stub

    }

    public void keyPressed(KeyEvent arg0) {
        // TODO Auto-generated method stub

    }

    public void keyReleased(KeyEvent arg0) {
        if (arg0.getSource() == this.iTree) {
            if (arg0.getKeyCode() == KeyEvent.VK_ENTER) {
                this.doubleClickModuleTreeNode();
            }
        }
    }

    /**
     Search whole workspace and find all module's name and guid, and save them to a file
     
     **/
    private void generateGuidsXref() {
        //
        // Init File Chooser
        //
        JFileChooser fc = new JFileChooser();
        fc.setCurrentDirectory(new File(Workspace.getCurrentWorkspace()));
        fc.setSelectedFile(new File(Workspace.getCurrentWorkspace() + DataType.FILE_SEPARATOR
                                    + DataType.GUIDS_XREF_FILE_NAME));
        fc.setMultiSelectionEnabled(false);

        //
        // Get guids xref and save to file
        //
        int result = fc.showSaveDialog(new JPanel());
        if (result == JFileChooser.APPROVE_OPTION) {
            Vector<String> v = wt.getAllModuleGuidXref();
            if (v.size() < 1) {
                Log.wrn("No guids found!!!");
                return;
            }
            File f = fc.getSelectedFile();
            if (!f.exists()) {
                try {
                    f.createNewFile();
                } catch (IOException e) {
                    Log.wrn("Fail to create file", e.getMessage());
                    Log.err("Fail to create file when generating guids.xref", e.getMessage());
                }
            }
            FileWriter fw = null;
            BufferedWriter bw = null;
            try {
                fw = new FileWriter(f);
                bw = new BufferedWriter(fw);
                for (int index = 0; index < v.size(); index++) {
                    bw.write(v.get(index));
                    bw.newLine();
                }
                bw.flush();
                fw.flush();
                bw.close();
                fw.close();
            } catch (IOException e) {
                Log.wrn("Fail to write file", e.getMessage());
                Log.err("Fail to write file when generating guids.xref", e.getMessage());
                return;
            }

            JOptionPane.showConfirmDialog(this, "File is created", "Generate guids.xref", JOptionPane.DEFAULT_OPTION,
                                          JOptionPane.INFORMATION_MESSAGE);
        }
    }

    /**
     Check if WORKSPACE Environment is valid
     
     **/
    private void checkWorkspace() {
        switch (Workspace.checkCurrentWorkspace()) {
        case Workspace.WORKSPACE_VALID:
            break;
        case Workspace.WORKSPACE_NOT_DEFINED:
            JOptionPane
                       .showConfirmDialog(
                                          this,
                                          "WORKSPACE Environment Variable Is Not Defined, Please select a valid WORKSPACE directory. "
                                                          + DataType.LINE_SEPARATOR
                                                          + DataType.LINE_SEPARATOR
                                                          + "NOTICE:"
                                                          + DataType.LINE_SEPARATOR
                                                          + "This does not change the System Environment Variable."
                                                          + DataType.LINE_SEPARATOR
                                                          + "It only applies to where the Wizard will manage modification and file creations.",
                                          "Error", JOptionPane.DEFAULT_OPTION, JOptionPane.ERROR_MESSAGE);
            SwitchWorkspace sw = new SwitchWorkspace(this, true);
            int result = sw.showDialog();
            if (result == DataType.RETURN_TYPE_CANCEL) {
                this.dispose();
                System.exit(0);
            } else if (result == DataType.RETURN_TYPE_OK) {
                sw.dispose();
                break;
            }
        case Workspace.WORKSPACE_NOT_EXIST:
            JOptionPane.showConfirmDialog(this, "Defined WORKSPACE Is Not Existed", "Error",
                                          JOptionPane.DEFAULT_OPTION, JOptionPane.ERROR_MESSAGE);
            this.dispose();
            System.exit(0);
        case Workspace.WORKSPACE_NOT_DIRECTORY:
            JOptionPane.showConfirmDialog(this, "Defined WORKSPACE Is Not A Directory", "Error",
                                          JOptionPane.DEFAULT_OPTION, JOptionPane.ERROR_MESSAGE);
            this.dispose();
            System.exit(0);
        case Workspace.WORKSPACE_NOT_VALID:
            JOptionPane.showConfirmDialog(this, "WORKSPACE Environment Variable Is Not Valid", "Error",
                                          JOptionPane.DEFAULT_OPTION, JOptionPane.ERROR_MESSAGE);
            this.dispose();
            System.exit(0);
        case Workspace.WORKSPACE_NO_TARGET_FILE:
            JOptionPane.showConfirmDialog(this, "Target.txt File Is Not Existed", "Error", JOptionPane.DEFAULT_OPTION,
                                          JOptionPane.ERROR_MESSAGE);
            this.dispose();
            System.exit(0);
        }
    }
}
