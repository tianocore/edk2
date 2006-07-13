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
import java.awt.event.ComponentListener;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.awt.event.WindowEvent;
import java.io.IOException;
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
import javax.swing.event.MenuEvent;
import javax.swing.event.MenuListener;
import javax.swing.event.TreeSelectionEvent;
import javax.swing.event.TreeSelectionListener;

import org.apache.xmlbeans.XmlException;
import org.tianocore.ModuleSurfaceAreaDocument;
import org.tianocore.PackageSurfaceAreaDocument;
import org.tianocore.PlatformSurfaceAreaDocument;
import org.tianocore.frameworkwizard.common.DataType;
import org.tianocore.frameworkwizard.common.IFileFilter;
import org.tianocore.frameworkwizard.common.Log;
import org.tianocore.frameworkwizard.common.SaveFile;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.Identifications.Identification;
import org.tianocore.frameworkwizard.common.Identifications.OpenFile;
import org.tianocore.frameworkwizard.common.Identifications.OpeningModuleList;
import org.tianocore.frameworkwizard.common.Identifications.OpeningModuleType;
import org.tianocore.frameworkwizard.common.Identifications.OpeningPackageList;
import org.tianocore.frameworkwizard.common.Identifications.OpeningPackageType;
import org.tianocore.frameworkwizard.common.Identifications.OpeningPlatformList;
import org.tianocore.frameworkwizard.common.Identifications.OpeningPlatformType;
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
import org.tianocore.frameworkwizard.module.ui.ModuleDataHubs;
import org.tianocore.frameworkwizard.module.ui.ModuleDefinitions;
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
import org.tianocore.frameworkwizard.packaging.ui.SpdPackageDefinitions;
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

/**
 The class is used to show main GUI of ModuleEditor
 It extends IFrame implements MouseListener, TreeSelectionListener

 **/
public class FrameworkWizardUI extends IFrame implements MouseListener, TreeSelectionListener, ComponentListener,
                                             MenuListener {
    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = -7103240960573031772L;

    //
    // To save information of all files
    //
    private Vector<ModuleIdentification> vModuleList = new Vector<ModuleIdentification>();

    private Vector<PackageIdentification> vPackageList = new Vector<PackageIdentification>();

    private Vector<PlatformIdentification> vPlatformList = new Vector<PlatformIdentification>();

    private OpeningModuleList openingModuleList = new OpeningModuleList();

    private OpeningPackageList openingPackageList = new OpeningPackageList();

    private OpeningPlatformList openingPlatformList = new OpeningPlatformList();

    private int currentOpeningModuleIndex = -1;

    private int currentOpeningPackageIndex = -1;

    private int currentOpeningPlatformIndex = -1;

    private IDefaultMutableTreeNode dmtnRoot = null;

    private IDefaultMutableTreeNode dmtnModuleDescription = null;

    private IDefaultMutableTreeNode dmtnPackageDescription = null;

    private IDefaultMutableTreeNode dmtnPlatformDescription = null;

    private JPanel jContentPane = null;

    private JMenuBar jMenuBar = null;

    private JMenu jMenuFile = null;

    private JMenuItem jMenuItemFileNew = null;

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

    private JMenuItem jMenuItemEditFind = null;

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

    private JMenu jMenuProjectBuildTargets = null;

    private JCheckBoxMenuItem jCheckBoxMenuItemProjectBuildTargetsDebug = null;

    private JCheckBoxMenuItem jCheckBoxMenuItemProjectBuildTargetsRelease = null;

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

    //private JToolBar jToolBarFile = null;

    //private JToolBar jToolBarEdit = null;

    //private JToolBar jToolBarWindow = null;

    private static FrameworkWizardUI fwui = null;

    private JMenuItem jMenuItemProjectCreateFar = null;

    public static FrameworkWizardUI getInstance() {
        if (fwui == null) {
            fwui = new FrameworkWizardUI();
        }
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
     
     */
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
            //jTabbedPaneEditor.addChangeListener(this);
            jTabbedPaneEditor.addTab("Module", null, getJDesktopPaneModule(), null);
            jTabbedPaneEditor.addTab("Package", null, getJDesktopPanePackage(), null);
            jTabbedPaneEditor.addTab("Platform", null, getJDesktopPanePlatform(), null);
        }
        return jTabbedPaneEditor;
    }

    /**
     This method initializes jTabbedPaneTree
     
     @return javax.swing.JTabbedPane	
     
     */
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
            jMenuFile = new JMenu();
            jMenuFile.setText("File");
            jMenuFile.setMnemonic('F');
            jMenuFile.add(getJMenuItemFileNew());
            jMenuFile.add(getJMenuItemFileOpen());
            jMenuFile.add(getJMenuItemFileClose());
            jMenuFile.add(getJMenuItemFileCloseAll());
            jMenuFile.addSeparator();
            jMenuFile.add(getJMenuFileRecentFiles());
            //jMenuFile.addSeparator();
            jMenuFile.add(getJMenuItemFileSave());
            jMenuFile.add(getJMenuItemFileSaveAs());
            jMenuFile.add(getJMenuItemFileSaveAll());
            jMenuFile.addSeparator();
            jMenuFile.add(getJMenuItemFilePageSetup());
            jMenuFile.add(getJMenuItemFilePrint());
            //jMenuFile.addSeparator();
            jMenuFile.add(getJMenuItemFileImport());
            //jMenuFile.addSeparator();
            jMenuFile.add(getJMenuItemFileProperties());
            //jMenuFile.addSeparator();
            jMenuFile.add(getJMenuItemFileExit());
            jMenuFile.addMenuListener(this);
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
            jMenuEdit = new JMenu();
            jMenuEdit.setText("Edit");
            jMenuEdit.setMnemonic('E');
            jMenuEdit.add(getJMenuItemEditUndo());
            jMenuEdit.add(getJMenuItemEditRedo());
            jMenuEdit.addSeparator();
            jMenuEdit.add(getJMenuItemEditCut());
            jMenuEdit.add(getJMenuItemEditCopy());
            jMenuEdit.add(getJMenuItemEditPaste());
            jMenuEdit.add(getJMenuItemEditDelete());
            jMenuEdit.addSeparator();
            jMenuEdit.add(getJMenuItemEditSelectAll());
            jMenuEdit.add(getJMenuItemEditFind());
            jMenuEdit.add(getJMenuItemEditFindNext());
            jMenuEdit.addSeparator();
            jMenuEdit.setVisible(false);
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
            //jScrollPaneTree.setBounds(new java.awt.Rectangle(0, 1, 290, 545));
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
            jMenuHelp = new JMenu();
            jMenuHelp.setText("Help");
            jMenuHelp.add(getJMenuItemHelpContents());
            jMenuHelp.add(getJMenuItemHelpIndex());
            jMenuHelp.add(getJMenuItemHelpSearch());
            //jMenuHelp.addSeparator();
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
            jMenuItemHelpAbout.setText("About...");
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
            jMenuItemEditDelete.addActionListener(this);
            //
            //Disabled when no module is open
            //
            jMenuItemEditDelete.setEnabled(false);
        }
        return jMenuItemEditDelete;
    }

    //    /**
    //     This method initializes jPopupMenu 
    //     
    //     @return javax.swing.JPopupMenu jPopupMenu
    //     
    //     **/
    //    private JPopupMenu getJPopupMenu() {
    //        if (jPopupMenu == null) {
    //            jPopupMenu = new JPopupMenu();
    //            //
    //            //Add menu items of popup menu
    //            //
    //            jPopupMenu.add(getJMenuItemPopupAdd());
    //            jPopupMenu.add(getJMenuItemPopupUpdate());
    //            jPopupMenu.add(getJMenuItemPopupDelete());
    //            jPopupMenu.setBorder(new BevelBorder(BevelBorder.RAISED));
    //            jPopupMenu.addMouseListener(this);
    //        }
    //        return jPopupMenu;
    //    }
    //
    //    /**
    //     This method initializes jMenuItemPopupAdd 
    //     
    //     @return javax.swing.JMenuItem jMenuItemPopupAdd
    //     
    //     **/
    //    private JMenuItem getJMenuItemPopupAdd() {
    //        if (jMenuItemPopupAdd == null) {
    //            jMenuItemPopupAdd = new JMenuItem();
    //            jMenuItemPopupAdd.setText("Add");
    //            jMenuItemPopupAdd.addActionListener(this);
    //            jMenuItemPopupAdd.setEnabled(false);
    //        }
    //        return jMenuItemPopupAdd;
    //    }
    //
    //    /**
    //     This method initializes jMenuItemPopupUpdate 
    //     
    //     @return javax.swing.JMenuItem jMenuItemPopupUpdate
    //     
    //     **/
    //    private JMenuItem getJMenuItemPopupUpdate() {
    //        if (jMenuItemPopupUpdate == null) {
    //            jMenuItemPopupUpdate = new JMenuItem();
    //            jMenuItemPopupUpdate.setText("Update");
    //            jMenuItemPopupUpdate.addActionListener(this);
    //            jMenuItemPopupUpdate.setEnabled(false);
    //        }
    //        return jMenuItemPopupUpdate;
    //    }
    //
    //    /**
    //     This method initializes jMenuItemPopupDelete 
    //     
    //     @return javax.swing.JMenuItem jMenuItemPopupDelete
    //     
    //     **/
    //    private JMenuItem getJMenuItemPopupDelete() {
    //        if (jMenuItemPopupDelete == null) {
    //            jMenuItemPopupDelete = new JMenuItem();
    //            jMenuItemPopupDelete.setText("Delete");
    //            jMenuItemPopupDelete.addActionListener(this);
    //            jMenuItemPopupDelete.setEnabled(false);
    //        }
    //        return jMenuItemPopupDelete;
    //    }

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
            jMenuTools = new JMenu();
            jMenuTools.setText("Tools");
            jMenuTools.setMnemonic('T');
            jMenuTools.add(getJMenuItemToolsToolChainConfiguration());
            jMenuTools.addSeparator();
            jMenuTools.add(getJMenuItemToolsClone());
            //jMenuTools.addSeparator();
            jMenuTools.add(getJMenuItemToolsCodeScan());
            jMenuTools.addMenuListener(this);
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
            jMenuWindow = new JMenu();
            jMenuWindow.setText("Window");
            jMenuWindow.setMnemonic('W');
            jMenuWindow.add(getJMenuItemWindowDisplaySide());
            jMenuWindow.add(getJMenuItemWindowDisplayTopBottom());
            jMenuWindow.addSeparator();
            jMenuWindow.add(getJMenuItemWindowTabView());
            jMenuWindow.addSeparator();
            jMenuWindow.add(getJMenuItemWindowSource());
            jMenuWindow.add(getJMenuItemWindowXML());
            jMenuWindow.addSeparator();
            jMenuWindow.add(getJMenuItemWindowPreferences());
            jMenuWindow.setVisible(false);
        }
        return jMenuWindow;
    }

    /**
     This method initializes jPanelOperation 
     
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
     */
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
     */
    private JMenuItem getJMenuItemFileCloseAll() {
        if (jMenuItemFileCloseAll == null) {
            jMenuItemFileCloseAll = new JMenuItem();
            jMenuItemFileCloseAll.setText("Close All");
            jMenuItemFileCloseAll.setEnabled(true);
            jMenuItemFileCloseAll.addActionListener(this);
        }
        return jMenuItemFileCloseAll;
    }

    /**
     This method initializes jMenuItemFileSaveAll	
     
     @return javax.swing.JMenuItem jMenuItemFileSaveAll
     */
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
     */
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
     */
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
     */
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
     * This method initializes jMenuItemFileProperties	
     * 	
     * @return javax.swing.JMenuItem	
     */
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
     * This method initializes jMenuFileRecentFiles	
     * 	
     * @return javax.swing.JMenu	
     */
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
     */
    private JMenuItem getJMenuItemEditUndo() {
        if (jMenuItemEditUndo == null) {
            jMenuItemEditUndo = new JMenuItem();
            jMenuItemEditUndo.setText("Undo");
            jMenuItemEditUndo.setMnemonic('U');
            jMenuItemEditUndo.setEnabled(false);
            jMenuItemEditUndo.addActionListener(this);
        }
        return jMenuItemEditUndo;
    }

    /**
     This method initializes jMenuItemEditRedo	
     
     @return javax.swing.JMenuItem	
     */
    private JMenuItem getJMenuItemEditRedo() {
        if (jMenuItemEditRedo == null) {
            jMenuItemEditRedo = new JMenuItem();
            jMenuItemEditRedo.setText("Redo");
            jMenuItemEditRedo.setMnemonic('R');
            jMenuItemEditRedo.setEnabled(false);
            jMenuItemEditRedo.addActionListener(this);
        }
        return jMenuItemEditRedo;
    }

    /**
     This method initializes jMenuItemEditCut	
     
     @return javax.swing.JMenuItem	
     */
    private JMenuItem getJMenuItemEditCut() {
        if (jMenuItemEditCut == null) {
            jMenuItemEditCut = new JMenuItem();
            jMenuItemEditCut.setText("Cut");
            jMenuItemEditCut.setMnemonic('t');
            jMenuItemEditCut.setEnabled(false);
            jMenuItemEditCut.addActionListener(this);
        }
        return jMenuItemEditCut;
    }

    /**
     This method initializes jMenuItemEditCopy	
     
     @return javax.swing.JMenuItem	
     */
    private JMenuItem getJMenuItemEditCopy() {
        if (jMenuItemEditCopy == null) {
            jMenuItemEditCopy = new JMenuItem();
            jMenuItemEditCopy.setText("Copy");
            jMenuItemEditCopy.setMnemonic('C');
            jMenuItemEditCopy.setEnabled(false);
            jMenuItemEditCopy.addActionListener(this);
        }
        return jMenuItemEditCopy;
    }

    /**
     This method initializes jMenuItemEditPaste	
     
     return javax.swing.JMenuItem	
     */
    private JMenuItem getJMenuItemEditPaste() {
        if (jMenuItemEditPaste == null) {
            jMenuItemEditPaste = new JMenuItem();
            jMenuItemEditPaste.setText("Paste");
            jMenuItemEditPaste.setMnemonic('P');
            jMenuItemEditPaste.setEnabled(false);
            jMenuItemEditPaste.addActionListener(this);
        }
        return jMenuItemEditPaste;
    }

    /**
     This method initializes jMenuItem	
     
     @return javax.swing.JMenuItem	
     */
    private JMenuItem getJMenuItemEditSelectAll() {
        if (jMenuItemEditSelectAll == null) {
            jMenuItemEditSelectAll = new JMenuItem();
            jMenuItemEditSelectAll.setText("Select All");
            jMenuItemEditSelectAll.setMnemonic('A');
            jMenuItemEditSelectAll.setEnabled(false);
            jMenuItemEditSelectAll.addActionListener(this);
        }
        return jMenuItemEditSelectAll;
    }

    /**
     This method initializes jMenuItemEditFind	
     
     @return javax.swing.JMenuItem	
     */
    private JMenuItem getJMenuItemEditFind() {
        if (jMenuItemEditFind == null) {
            jMenuItemEditFind = new JMenuItem();
            jMenuItemEditFind.setText("Find");
            jMenuItemEditFind.setMnemonic('F');
            jMenuItemEditFind.setEnabled(false);
            jMenuItemEditFind.addActionListener(this);
        }
        return jMenuItemEditFind;
    }

    /**
     This method initializes jMenuItemEditFindNext	
     
     @return javax.swing.JMenuItem	
     
     */
    private JMenuItem getJMenuItemEditFindNext() {
        if (jMenuItemEditFindNext == null) {
            jMenuItemEditFindNext = new JMenuItem();
            jMenuItemEditFindNext.setText("Find Next");
            jMenuItemEditFindNext.setMnemonic('n');
            jMenuItemEditFindNext.setEnabled(false);
            jMenuItemEditFindNext.addActionListener(this);
        }
        return jMenuItemEditFindNext;
    }

    /**
     This method initializes jMenuView	
     
     @return javax.swing.JMenu	
     
     */
    private JMenu getJMenuView() {
        if (jMenuView == null) {
            jMenuView = new JMenu();
            jMenuView.setText("View");
            jMenuView.setMnemonic('V');
            jMenuView.add(getJMenuViewToolbars());
            jMenuView.add(getJMenuItemViewAdvanced());
            jMenuView.add(getJMenuItemViewStandard());
            jMenuView.add(getJMenuItemViewXML());
            jMenuView.setVisible(false);
        }
        return jMenuView;
    }

    /**
     This method initializes jMenuViewToolbars	
     
     @return javax.swing.JMenu	
     
     */
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
     
     */
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
     
     */
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
     
     */
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
     
     */
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
     
     */
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
     
     */
    private JMenu getJMenuProject() {
        if (jMenuProject == null) {
            jMenuProject = new JMenu();
            jMenuProject.setText("Project");
            jMenuProject.setMnemonic('P');
            jMenuProject.add(getJMenuItemProjectAdmin());
            //jMenuProject.addSeparator();
            jMenuProject.add(getJMenuItemProjectChangeWorkspace());
            jMenuProject.addSeparator();
            jMenuProject.add(getJMenuItemProjectCreateFar());
            jMenuProject.add(getJMenuItemProjectInstallFar());
            jMenuProject.add(getJMenuItemProjectUpdateFar());
            jMenuProject.add(getJMenuItemProjectRemoveFar());
            //jMenuProject.addSeparator();
            jMenuProject.add(getJMenuProjectBuildTargets());
        }
        return jMenuProject;
    }

    /**
     This method initializes jMenuItemProjectAdmin	
     
     @return javax.swing.JMenuItem	
     
     */
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
     
     */
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
     This method initializes jMenuProjectBuildTargets	
     
     @return javax.swing.JMenu	
     
     */
    private JMenu getJMenuProjectBuildTargets() {
        if (jMenuProjectBuildTargets == null) {
            jMenuProjectBuildTargets = new JMenu();
            jMenuProjectBuildTargets.setText("Build Targets");
            jMenuProjectBuildTargets.setMnemonic('T');
            jMenuProjectBuildTargets.add(getJCheckBoxMenuItemProjectBuildTargetsDebug());
            jMenuProjectBuildTargets.add(getJCheckBoxMenuItemProjectBuildTargetsRelease());
            jMenuProjectBuildTargets.setVisible(false);
        }
        return jMenuProjectBuildTargets;
    }

    /**
     This method initializes jCheckBoxMenuItemProjectBuildTargetsDebug	
     
     @return javax.swing.JCheckBoxMenuItem	
     
     */
    private JCheckBoxMenuItem getJCheckBoxMenuItemProjectBuildTargetsDebug() {
        if (jCheckBoxMenuItemProjectBuildTargetsDebug == null) {
            jCheckBoxMenuItemProjectBuildTargetsDebug = new JCheckBoxMenuItem();
            jCheckBoxMenuItemProjectBuildTargetsDebug.setText("Debug");
            jCheckBoxMenuItemProjectBuildTargetsDebug.setEnabled(false);
        }
        return jCheckBoxMenuItemProjectBuildTargetsDebug;
    }

    /**
     This method initializes jCheckBoxMenuItemProjectBuildTargetsRelease	
     
     @return javax.swing.JCheckBoxMenuItem	
     
     */
    private JCheckBoxMenuItem getJCheckBoxMenuItemProjectBuildTargetsRelease() {
        if (jCheckBoxMenuItemProjectBuildTargetsRelease == null) {
            jCheckBoxMenuItemProjectBuildTargetsRelease = new JCheckBoxMenuItem();
            jCheckBoxMenuItemProjectBuildTargetsRelease.setText("Release");
            jCheckBoxMenuItemProjectBuildTargetsRelease.setEnabled(false);
        }
        return jCheckBoxMenuItemProjectBuildTargetsRelease;
    }

    /**
     This method initializes jMenuItemToolsToolChainConfiguration	
     
     @return javax.swing.JMenuItem	
     
     */
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
     
     */
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
     
     */
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
     
     */
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
     
     */
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
     
     */
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
     
     */
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
     
     */
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
     
     */
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
     
     */
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
     
     */
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
     
     */
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
     * This method initializes jToolBar	
     * 	
     * @return javax.swing.JToolBar	
     */
    //	private JToolBar getJToolBarFile() {
    //		if (jToolBarFile == null) {
    //			jToolBarFile = new JToolBar();
    //			jToolBarFile.setFloatable(false);
    //		}
    //		return jToolBarFile;
    //	}
    /**
     * This method initializes jToolBarEdit	
     * 	
     * @return javax.swing.JToolBar	
     */
    //	private JToolBar getJToolBarEdit() {
    //		if (jToolBarEdit == null) {
    //			jToolBarEdit = new JToolBar();
    //			jToolBarEdit.setFloatable(false);
    //		}
    //		return jToolBarEdit;
    //	}
    /**
     * This method initializes jMenuItemToolsInstallPackage	
     * 	
     * @return javax.swing.JMenuItem	
     */
    private JMenuItem getJMenuItemProjectInstallFar() {
        if (jMenuItemProjectInstallFar == null) {
            jMenuItemProjectInstallFar = new JMenuItem();
            jMenuItemProjectInstallFar.setText("Install FAR");
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
            jMenuItemProjectUpdateFar.setText("Update FAR");
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
            jMenuItemProjectRemoveFar.setText("Remove FAR");
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
            jMenuItemProjectCreateFar.setText("Create FAR");
            jMenuItemProjectCreateFar.setMnemonic('C');
            jMenuItemProjectCreateFar.addActionListener(this);
        }
        return jMenuItemProjectCreateFar;
    }

    /* (non-Javadoc)
     * @see org.tianocore.packaging.common.ui.IFrame#main(java.lang.String[])
     *
     * Main class, start the GUI
     * 
     */
    public static void main(String[] args) {
        FrameworkWizardUI module = FrameworkWizardUI.getInstance();
        module.setVisible(true);
    }

    /**
     This is the default constructor
     
     **/
    public FrameworkWizardUI() {
        super();
        init();
    }

    /**
     This method initializes this
     
     
     **/
    private void init() {
        //
        // Set current workspace and check
        // Check if exists WORKSPACE
        // 
        //
        Workspace.setCurrentWorkspace(System.getenv("WORKSPACE"));
        if (!Workspace.checkCurrentWorkspace()) {
            JOptionPane.showConfirmDialog(null, "Workspace is not setup correctly. Please setup first.", "Warning",
                                          JOptionPane.DEFAULT_OPTION, JOptionPane.WARNING_MESSAGE);
            this.dispose();
            System.exit(0);
        }

        this.setSize(DataType.MAIN_FRAME_PREFERRED_SIZE_WIDTH, DataType.MAIN_FRAME_PREFERRED_SIZE_HEIGHT);
        this.setResizable(true);
        this.setJMenuBar(getjJMenuBar());
        this.addComponentListener(this);
        this.getCompontentsFromFrameworkDatabase();
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
        if (arg0.getSource() == jMenuItemFileNew) {
            this.fileNew();
        }

        if (arg0.getSource() == jMenuItemFileOpen) {
            this.open();
        }

        if (arg0.getSource() == jMenuItemFileClose) {
            this.close();
        }

        if (arg0.getSource() == jMenuItemFileCloseAll) {
            this.closeAll();
        }

        if (arg0.getSource() == jMenuItemFileSave) {
            this.save();
        }

        if (arg0.getSource() == jMenuItemFileSaveAs) {
            this.saveAs();
        }

        if (arg0.getSource() == jMenuItemFileSaveAll) {
            this.saveAll();
        }

        if (arg0.getSource() == jMenuItemFileExit) {
            this.exit();
        }

        if (arg0.getSource() == jMenuItemProjectChangeWorkspace) {
            changeWorkspace();
        }

        if (arg0.getSource() == jMenuItemProjectCreateFar) {
            CreateStepOne cso = new CreateStepOne(this, true);
            int result = cso.showDialog();
            if (result == DataType.RETURN_TYPE_OK) {
                String strReturn = "Create Far Done!";
                JOptionPane.showConfirmDialog(null, strReturn, "Done", JOptionPane.DEFAULT_OPTION, JOptionPane.INFORMATION_MESSAGE);
            }
            cso.dispose();
        }

        if (arg0.getSource() == jMenuItemProjectInstallFar) {
            InstallStepOne iso = new InstallStepOne(this, true);
            int result = iso.showDialog();
            if (result == DataType.RETURN_TYPE_OK) {
                String strReturn = "<html>Install Far Done! <br>The WORKSPACE will be refreshed!</html>";
                JOptionPane.showConfirmDialog(null, strReturn, "Done", JOptionPane.DEFAULT_OPTION, JOptionPane.INFORMATION_MESSAGE);
                this.closeAll();
            }
            iso.dispose();
        }

        if (arg0.getSource() == jMenuItemProjectRemoveFar) {
            DeleteStepOne dso = new DeleteStepOne(this, true);
            int result = dso.showDialog();
            if (result == DataType.RETURN_TYPE_OK) {
                String strReturn = "<html>Delete Far Done! <br>The WORKSPACE will be refreshed!</html>";
                JOptionPane.showConfirmDialog(null, strReturn, "Done", JOptionPane.DEFAULT_OPTION, JOptionPane.INFORMATION_MESSAGE);
                this.closeAll();
            }
            dso.dispose();
        }

        if (arg0.getSource() == jMenuItemProjectUpdateFar) {
            UpdateStepOne uso = new UpdateStepOne(this, true);
            int result = uso.showDialog();
            if (result == DataType.RETURN_TYPE_OK) {
                String strReturn = "<html>Update Far Done! <br>The WORKSPACE will be refreshed!</html>";
                JOptionPane.showConfirmDialog(null, strReturn, "Done", JOptionPane.DEFAULT_OPTION, JOptionPane.INFORMATION_MESSAGE);
                this.closeAll();
            }
            uso.dispose();
        }

        if (arg0.getSource() == jMenuItemToolsClone) {
            cloneItem();
        }

        if (arg0.getSource() == jMenuItemToolsToolChainConfiguration) {
            setupToolChainConfiguration();
        }

        if (arg0.getSource() == jMenuItemHelpAbout) {
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
        // Make root
        dmtnRoot = new IDefaultMutableTreeNode("WORKSPACE", IDefaultMutableTreeNode.WORKSPACE, -1);

        // Make Module Description
        dmtnModuleDescription = new IDefaultMutableTreeNode("ModuleDescription", IDefaultMutableTreeNode.MODULE, -1);

        //
        // First add package
        //
        if (this.vPackageList.size() > 0) {
            for (int index = 0; index < this.vPackageList.size(); index++) {
                IDefaultMutableTreeNode dmtnModulePackage = null;
                IDefaultMutableTreeNode dmtnModulePackageLibrary = null;
                IDefaultMutableTreeNode dmtnModulePackageModule = null;

                dmtnModulePackage = new IDefaultMutableTreeNode(this.vPackageList.elementAt(index).getName(),
                                                                IDefaultMutableTreeNode.MODULE_PACKAGE, false,
                                                                this.vPackageList.elementAt(index));
                dmtnModulePackageLibrary = new IDefaultMutableTreeNode("Library",
                                                                       IDefaultMutableTreeNode.MODULE_PACKAGE_LIBRARY,
                                                                       false, this.vPackageList.elementAt(index));
                dmtnModulePackageModule = new IDefaultMutableTreeNode("Module",
                                                                      IDefaultMutableTreeNode.MODULE_PACKAGE_MODULE,
                                                                      false, this.vPackageList.elementAt(index));

                Vector<ModuleIdentification> vModule = wt.getAllModules(this.vPackageList.elementAt(index));
                for (int indexJ = 0; indexJ < vModule.size(); indexJ++) {
                    if (vModule.get(indexJ).isLibrary()) {
                        dmtnModulePackageLibrary.add(new IDefaultMutableTreeNode(vModule.get(indexJ).getName(),
                                                                                 IDefaultMutableTreeNode.MSA_HEADER,
                                                                                 false, vModule.get(indexJ)));
                    } else {
                        dmtnModulePackageModule.add(new IDefaultMutableTreeNode(vModule.get(indexJ).getName(),
                                                                                IDefaultMutableTreeNode.MSA_HEADER,
                                                                                false, vModule.get(indexJ)));
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

        //                if (this.vModuleList.size() > 0) {
        //                    for (int index = 0; index < this.vModuleList.size(); index++) {
        //                        dmtnModuleDescription.add(new IDefaultMutableTreeNode(this.vModuleList.elementAt(index).getName(),
        //                                                                              IDefaultMutableTreeNode.MSA_HEADER, false,
        //                                                                              this.vModuleList.elementAt(index)));
        //                    }
        //                }

        // Make Package Description
        dmtnPackageDescription = new IDefaultMutableTreeNode("PackageDescription", IDefaultMutableTreeNode.PACKAGE, -1);
        if (this.vPackageList.size() > 0) {
            for (int index = 0; index < this.vPackageList.size(); index++) {
                dmtnPackageDescription.add(new IDefaultMutableTreeNode(this.vPackageList.elementAt(index).getName(),
                                                                       IDefaultMutableTreeNode.SPD_HEADER, false,
                                                                       this.vPackageList.elementAt(index)));
            }
        }

        // Make Platform Description
        dmtnPlatformDescription = new IDefaultMutableTreeNode("PlatformDescription", IDefaultMutableTreeNode.PLATFORM,
                                                              -1);
        if (this.vPlatformList.size() > 0) {
            for (int index = 0; index < this.vPlatformList.size(); index++) {
                dmtnPlatformDescription.add(new IDefaultMutableTreeNode(this.vPlatformList.elementAt(index).getName(),
                                                                        IDefaultMutableTreeNode.FPD_PLATFORMHEADER,
                                                                        false, this.vPlatformList.elementAt(index)));
            }
        }

        dmtnRoot.add(dmtnModuleDescription);
        dmtnRoot.add(dmtnPackageDescription);
        dmtnRoot.add(dmtnPlatformDescription);
        iTree = new ITree(dmtnRoot);
        iTree.addMouseListener(this);
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
        if (arg0.getButton() == MouseEvent.BUTTON1) {
        }
        if (arg0.getButton() == MouseEvent.BUTTON3) {
        }
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
        IDefaultMutableTreeNode node = new IDefaultMutableTreeNode(mid.getName(), IDefaultMutableTreeNode.MSA_HEADER,
                                                                   true, mid);
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
                                                               false, mid.getPackageId());
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
                                                                mid.getPackageId());
                iTree.addNode(packageNode, parentLibraryNode);
            }

            iTree.addNode(parentLibraryNode, node);
        }
    }

    /**
     Open Module

     @param path input file path
     
     **/
    private void openModule(String path, ModuleIdentification moduleId) {
        ModuleSurfaceAreaDocument.ModuleSurfaceArea msa = null;
        try {
            msa = OpenFile.openMsaFile(path);
        } catch (IOException e) {
            Log.err("Open Module Surface Area " + path, e.getMessage());
            return;
        } catch (XmlException e) {
            Log.err("Open Module Surface Area " + path, e.getMessage());
            return;
        } catch (Exception e) {
            Log.err("Open Module Surface Area " + path, "Invalid file type");
            return;
        }
        Identification id = new Identification(msa.getMsaHeader().getModuleName(), msa.getMsaHeader().getGuidValue(),
                                               msa.getMsaHeader().getVersion(), path);
        //
        // Generate module id
        //
        PackageIdentification pid = wt.getPackageIdByModuleId(id);
        if (pid != null) {
            //
            // To judge if the module existed in vModuleList
            // If not, add it to vModuleList
            //
            boolean isFind = false;
            for (int index = 0; index < vModuleList.size(); index++) {
                if (vModuleList.elementAt(index).equals(id)) {
                    isFind = true;
                    break;
                }
            }
            if (!isFind) {
                ModuleIdentification mid = new ModuleIdentification(id, pid, moduleId.isLibrary());
                vModuleList.addElement(mid);
                addModuleToTree(mid);
            }
        } else {
            //
            // The module is not in existing packages
            //
            Log.err("The module hasn't been added to any package of current workspace!");
            return;
        }

        //
        // Make the node selected
        //
        iTree.setSelectionPath(iTree.getPathOfNode(iTree.getNodeById(this.dmtnModuleDescription, id,
                                                                     IDefaultMutableTreeNode.MSA_HEADER)));
        //
        // Update opening Module list information
        //
        if (!openingModuleList.existsModule(id)) {
            //
            // Insert sub node of module
            //
            insertModuleTreeNode(id);
            iTree.getSelectNode().setOpening(true);

            //
            // Update opening module list
            //
            openingModuleList.insertToOpeningModuleList(id, msa);
            openingModuleList.setTreePathById(id, iTree.getSelectionPath());
        }
        //
        // Show msa header in editor panel
        //
        showModuleElement(IDefaultMutableTreeNode.MSA_HEADER, openingModuleList.getOpeningModuleById(id));
        this.currentOpeningModuleIndex = openingModuleList.findIndexOfListById(id);
    }

    /**
     Open Module

     @param path input file path
     
     **/
    private void openModule(String path) {
        ModuleSurfaceAreaDocument.ModuleSurfaceArea msa = null;
        try {
            msa = OpenFile.openMsaFile(path);
        } catch (IOException e) {
            Log.err("Open Module Surface Area " + path, e.getMessage());
            return;
        } catch (XmlException e) {
            Log.err("Open Module Surface Area " + path, e.getMessage());
            return;
        } catch (Exception e) {
            Log.err("Open Module Surface Area " + path, "Invalid file type");
            return;
        }
        Identification id = new Identification(msa.getMsaHeader().getModuleName(), msa.getMsaHeader().getGuidValue(),
                                               msa.getMsaHeader().getVersion(), path);
        //
        // Generate module id
        //
        PackageIdentification pid = wt.getPackageIdByModuleId(id);
        if (pid != null) {
            //
            // To judge if the module existed in vModuleList
            // If not, add it to vModuleList
            //
            boolean isFind = false;
            for (int index = 0; index < vModuleList.size(); index++) {
                if (vModuleList.elementAt(index).equals(id)) {
                    isFind = true;
                    break;
                }
            }
            if (!isFind) {
                ModuleIdentification mid = new ModuleIdentification(id, pid);
                vModuleList.addElement(mid);
                addModuleToTree(mid);
            }
        } else {
            //
            // The module is not in existing packages
            //
            Log.err("The module hasn't been added to any package of current workspace!");
            return;
        }

        //
        // Make the node selected
        //
        iTree.setSelectionPath(iTree.getPathOfNode(iTree.getNodeById(this.dmtnModuleDescription, id,
                                                                     IDefaultMutableTreeNode.MSA_HEADER)));
        //
        // Update opening Module list information
        //
        if (!openingModuleList.existsModule(id)) {
            //
            // Insert sub node of module
            //
            insertModuleTreeNode(id);
            iTree.getSelectNode().setOpening(true);

            //
            // Update opening module list
            //
            openingModuleList.insertToOpeningModuleList(id, msa);
            openingModuleList.setTreePathById(id, iTree.getSelectionPath());
        }
        //
        // Show msa header in editor panel
        //
        showModuleElement(IDefaultMutableTreeNode.MSA_HEADER, openingModuleList.getOpeningModuleById(id));
        this.currentOpeningModuleIndex = openingModuleList.findIndexOfListById(id);
    }

    /**
     Open Package

     @param path input file path
     
     **/
    private void openPackage(String path) {
        PackageSurfaceAreaDocument.PackageSurfaceArea spd = null;
        try {
            spd = OpenFile.openSpdFile(path);
        } catch (IOException e) {
            Log.err("Open Package Surface Area " + path, e.getMessage());
            return;
        } catch (XmlException e) {
            Log.err("Open Package Surface Area " + path, e.getMessage());
            return;
        } catch (Exception e) {
            Log.err("Open Package Surface Area " + path, "Invalid file type");
            return;
        }
        Identification id = new Identification(spd.getSpdHeader().getPackageName(), spd.getSpdHeader().getGuidValue(),
                                               spd.getSpdHeader().getVersion(), path);
        //
        // To judge if the package existed in vPackageList
        // If not, add it to vPackageList
        //
        boolean isFind = false;
        for (int index = 0; index < vPackageList.size(); index++) {
            if (vPackageList.elementAt(index).equals(id)) {
                isFind = true;
                break;
            }
        }
        if (!isFind) {
            //
            // The module is not in existing packages
            //
            Log.err("The package hasn't been added to current workspace!");
            return;
        }

        //
        // Make the node selected
        //
        iTree.setSelectionPath(iTree.getPathOfNode(iTree.getNodeById(this.dmtnPackageDescription, id,
                                                                     IDefaultMutableTreeNode.SPD_HEADER)));
        //
        // Update opening package list information
        //
        if (!openingPackageList.existsPackage(id)) {
            //
            // Insert sub node of module
            //
            insertPackageTreeNode(id);
            iTree.getSelectNode().setOpening(true);

            //
            // Update opening module list
            //
            openingPackageList.insertToOpeningPackageList(id, spd);
            openingPackageList.setTreePathById(id, iTree.getSelectionPath());
        }
        //
        // Show spd header in editor panel
        //
        showPackageElement(IDefaultMutableTreeNode.SPD_HEADER, openingPackageList.getOpeningPackageById(id));
        this.currentOpeningPackageIndex = openingPackageList.findIndexOfListById(id);
    }

    /**
     Open Package

     @param path input file path
     
     **/
    private void openPlatform(String path) {
        PlatformSurfaceAreaDocument.PlatformSurfaceArea fpd = null;
        try {
            fpd = OpenFile.openFpdFile(path);
        } catch (IOException e) {
            Log.err("Open Platform Surface Area " + path, e.getMessage());
            return;
        } catch (XmlException e) {
            Log.err("Open Platform Surface Area " + path, e.getMessage());
            return;
        } catch (Exception e) {
            Log.err("Open Platform Surface Area " + path, "Invalid file type");
            return;
        }
        Identification id = new Identification(fpd.getPlatformHeader().getPlatformName(), fpd.getPlatformHeader()
                                                                                             .getGuidValue(),
                                               fpd.getPlatformHeader().getVersion(), path);
        //
        // To judge if the platform existed in vPlatformList
        // If not, add it to vPlatformList
        //
        boolean isFind = false;
        for (int index = 0; index < vPlatformList.size(); index++) {
            if (vPlatformList.elementAt(index).equals(id)) {
                isFind = true;
                break;
            }
        }
        if (!isFind) {
            //
            // The module is not in existing packages
            //
            Log.err("The platform hasn't been added to current workspace!");
            return;
        }

        //
        // Make the node selected
        //
        iTree.setSelectionPath(iTree.getPathOfNode(iTree.getNodeById(this.dmtnPlatformDescription, id,
                                                                     IDefaultMutableTreeNode.FPD_PLATFORMHEADER)));
        //
        // Update opening package list information
        //
        if (!openingPlatformList.existsPlatform(id)) {
            //
            // Insert sub node of module
            //
            insertPlatformTreeNode(id);
            iTree.getSelectNode().setOpening(true);

            //
            // Update opening module list
            //
            openingPlatformList.insertToOpeningPlatformList(id, fpd);
            openingPlatformList.setTreePathById(id, iTree.getSelectionPath());
        }
        //
        // Show fpd header in editor panel
        //
        showPlatformElement(IDefaultMutableTreeNode.FPD_PLATFORMHEADER, openingPlatformList.getOpeningPlatformById(id));
        this.currentOpeningPlatformIndex = openingPlatformList.findIndexOfListById(id);
    }

    /**
     Save module 
     
     **/
    private void saveModule(int index) {
        OpeningModuleType omt = openingModuleList.getOpeningModuleByIndex(index);
        if (omt.isNew()) {
            if (getNewFilePath(DataType.MODULE_SURFACE_AREA_EXT) != JFileChooser.APPROVE_OPTION) {
                return;
            }
        }
        try {
            SaveFile.saveMsaFile(omt.getId().getPath(), omt.getXmlMsa());
            openingModuleList.setNew(omt.getId(), false);
            openingModuleList.setModuleSaved(omt.getId(), true);
        } catch (Exception e) {
            Log.err("Save Module", e.getMessage());
        }
    }

    /**
     Save package 
     
     **/
    private void savePackage(int index) {
        OpeningPackageType opt = openingPackageList.getOpeningPackageByIndex(index);
        if (opt.isNew()) {
            if (getNewFilePath(DataType.PACKAGE_SURFACE_AREA_EXT) != JFileChooser.APPROVE_OPTION) {
                return;
            }
        }
        try {
            SaveFile.saveSpdFile(opt.getId().getPath(), opt.getXmlSpd());
            openingPackageList.setNew(opt.getId(), false);
            openingPackageList.setPackageSaved(opt.getId(), true);
        } catch (Exception e) {
            Log.err("Save Package", e.getMessage());
        }
    }

    /**
     Save platform 
     
     **/
    private void savePlatform(int index) {
        OpeningPlatformType opt = openingPlatformList.getOpeningPlatformByIndex(index);
        if (opt.isNew()) {
            if (getNewFilePath(DataType.PACKAGE_SURFACE_AREA_EXT) != JFileChooser.APPROVE_OPTION) {
                return;
            }
        }
        try {
            SaveFile.saveFpdFile(opt.getId().getPath(), opt.getXmlFpd());
            openingPlatformList.setNew(opt.getId(), false);
            openingPlatformList.setPlatformSaved(opt.getId(), true);
        } catch (Exception e) {
            Log.err("Save Package", e.getMessage());
        }
    }

    public void componentHidden(ComponentEvent arg0) {
        // TODO Auto-generated method stub

    }

    public void componentMoved(ComponentEvent arg0) {
        // TODO Auto-generated method stub

    }

    public void componentResized(ComponentEvent arg0) {
        this.jSplitPane.setSize(this.getWidth() - DataType.MAIN_FRAME_WIDTH_SPACING,
                                this.getHeight() - DataType.MAIN_FRAME_HEIGHT_SPACING);
        this.jSplitPane.validate();
        resizeDesktopPanel();
    }

    public void componentShown(ComponentEvent arg0) {
        // TODO Auto-generated method stub

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

    private void getCompontentsFromFrameworkDatabase() {
        this.vModuleList = wt.getAllModules();
        this.vPackageList = wt.getAllPackages();
        this.vPlatformList = wt.getAllPlatforms();
    }

    private void insertModuleTreeNode(Identification id) {
        iTree.addNode(new IDefaultMutableTreeNode("Module Definitions", IDefaultMutableTreeNode.MSA_MODULEDEFINITIONS,
                                                  true, id));
        iTree.addNode(new IDefaultMutableTreeNode("Library Class Definitions",
                                                  IDefaultMutableTreeNode.MSA_LIBRARYCLASSDEFINITIONS, true, id));
        iTree.addNode(new IDefaultMutableTreeNode("Source Files", IDefaultMutableTreeNode.MSA_SOURCEFILES, true, id));
        iTree.addNode(new IDefaultMutableTreeNode("Package Dependencies",
                                                  IDefaultMutableTreeNode.MSA_PACKAGEDEPENDENCIES, true, id));
        iTree.addNode(new IDefaultMutableTreeNode("Protocols", IDefaultMutableTreeNode.MSA_PROTOCOLS, true, id));
        iTree.addNode(new IDefaultMutableTreeNode("Events", IDefaultMutableTreeNode.MSA_EVENTS, true, id));
        iTree.addNode(new IDefaultMutableTreeNode("Hobs", IDefaultMutableTreeNode.MSA_HOBS, true, id));
        iTree.addNode(new IDefaultMutableTreeNode("Ppis", IDefaultMutableTreeNode.MSA_PPIS, true, id));
        iTree.addNode(new IDefaultMutableTreeNode("Variables", IDefaultMutableTreeNode.MSA_VARIABLES, true, id));
        iTree.addNode(new IDefaultMutableTreeNode("Boot Modes", IDefaultMutableTreeNode.MSA_BOOTMODES, true, id));
        iTree.addNode(new IDefaultMutableTreeNode("System Tables", IDefaultMutableTreeNode.MSA_SYSTEMTABLES, true, id));
        iTree.addNode(new IDefaultMutableTreeNode("Data Hubs", IDefaultMutableTreeNode.MSA_DATAHUBS, true, id));
        iTree.addNode(new IDefaultMutableTreeNode("Hii Packages", IDefaultMutableTreeNode.MSA_HIIPACKAGES, true, id));
        iTree.addNode(new IDefaultMutableTreeNode("Guids", IDefaultMutableTreeNode.MSA_GUIDS, true, id));
        iTree.addNode(new IDefaultMutableTreeNode("Externs", IDefaultMutableTreeNode.MSA_EXTERNS, true, id));
        iTree.addNode(new IDefaultMutableTreeNode("PcdCoded", IDefaultMutableTreeNode.MSA_PCDS, true, id));
    }

    private void insertPackageTreeNode(Identification id) {
        iTree.addNode(new IDefaultMutableTreeNode("Package Definitions",
                                                  IDefaultMutableTreeNode.SPD_PACKAGEDEFINITIONS, true, id));
        iTree.addNode(new IDefaultMutableTreeNode("Library Class Declarations",
                                                  IDefaultMutableTreeNode.SPD_LIBRARYCLASSDECLARATIONS, true, id));
        iTree.addNode(new IDefaultMutableTreeNode("Msa Files", IDefaultMutableTreeNode.SPD_MSAFILES, false, id));
        iTree.addNode(new IDefaultMutableTreeNode("Package Headers", IDefaultMutableTreeNode.SPD_PACKAGEHEADERS, true,
                                                  id));
        iTree.addNode(new IDefaultMutableTreeNode("Guid Declarations", IDefaultMutableTreeNode.SPD_GUIDDECLARATIONS,
                                                  true, id));
        iTree.addNode(new IDefaultMutableTreeNode("Protocol Declarations",
                                                  IDefaultMutableTreeNode.SPD_PROTOCOLDECLARATIONS, true, id));
        iTree.addNode(new IDefaultMutableTreeNode("Ppi Declarations", IDefaultMutableTreeNode.SPD_PPIDECLARATIONS,
                                                  true, id));
        iTree.addNode(new IDefaultMutableTreeNode("Pcd Declarations", IDefaultMutableTreeNode.SPD_PCDDECLARATIONS,
                                                  true, id));
    }

    private void insertPlatformTreeNode(Identification id) {
        iTree.addNode(new IDefaultMutableTreeNode("Platform Definitions",
                                                  IDefaultMutableTreeNode.FPD_PLATFORMDEFINITIONS, true, id));
        iTree.addNode(new IDefaultMutableTreeNode("Flash", IDefaultMutableTreeNode.FPD_FLASH, true, id));
        iTree.addNode(new IDefaultMutableTreeNode("Framework Modules", IDefaultMutableTreeNode.FPD_FRAMEWORKMODULES,
                                                  true, id));
        iTree.addNode(new IDefaultMutableTreeNode("Pcd Dynamic Build Declarations",
                                                  IDefaultMutableTreeNode.FPD_PCDDYNAMICBUILDDECLARATIONS, true, id));
        iTree.addNode(new IDefaultMutableTreeNode("Build Options", IDefaultMutableTreeNode.FPD_BUILDOPTIONS, true, id));
    }

    /**
     Operate when double click a tree node
     
     **/
    private void doubleClickModuleTreeNode() {
        Identification id = iTree.getSelectNode().getId();
        int intCategory = iTree.getSelectCategory();
        String path = null;
        //          
        // If the node is not opened yet
        // Insert top level elements first
        //
        //if (intCategory == IDefaultMutableTreeNode.MSA_HEADER || intCategory == IDefaultMutableTreeNode.SPD_MSAFILES) {
        if (intCategory == IDefaultMutableTreeNode.MSA_HEADER) {

            if (intCategory == IDefaultMutableTreeNode.MSA_HEADER) {
                path = iTree.getSelectNode().getId().getPath();
            }
            if (intCategory == IDefaultMutableTreeNode.SPD_MSAFILES) {
                path = iTree.getSelectNode().getId().getPath();
            }
            openModule(path);
            return;
        }
        if (intCategory == IDefaultMutableTreeNode.SPD_HEADER) {
            path = iTree.getSelectNode().getId().getPath();
            openPackage(path);
            return;
        }
        if (intCategory == IDefaultMutableTreeNode.FPD_PLATFORMHEADER) {
            path = iTree.getSelectNode().getId().getPath();
            openPlatform(path);
            return;
        }
        //
        // Show editor panel
        //
        if (intCategory >= IDefaultMutableTreeNode.MSA_HEADER && intCategory < IDefaultMutableTreeNode.SPD_HEADER) {
            showModuleElement(intCategory, openingModuleList.getOpeningModuleById(id));
            this.currentOpeningModuleIndex = openingModuleList.findIndexOfListById(id);
        }
        if (intCategory >= IDefaultMutableTreeNode.SPD_HEADER
            && intCategory < IDefaultMutableTreeNode.FPD_PLATFORMHEADER) {
            showPackageElement(intCategory, openingPackageList.getOpeningPackageById(id));
            this.currentOpeningPackageIndex = openingPackageList.findIndexOfListById(id);
        }
        if (intCategory >= IDefaultMutableTreeNode.FPD_PLATFORMHEADER) {
            showPlatformElement(intCategory, openingPlatformList.getOpeningPlatformById(id));
            this.currentOpeningPlatformIndex = openingPlatformList.findIndexOfListById(id);
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
        case IDefaultMutableTreeNode.SPD_PACKAGEDEFINITIONS:
            SpdPackageDefinitions frmSpdPackageDefinitions = new SpdPackageDefinitions(spd);
            getJDesktopPanePackage().add(frmSpdPackageDefinitions, 1);
            break;
        case IDefaultMutableTreeNode.SPD_LIBRARYCLASSDECLARATIONS:
            SpdLibClassDecls frmSlcd = new SpdLibClassDecls(spd);
            getJDesktopPanePackage().add(frmSlcd, 1);
            break;
        case IDefaultMutableTreeNode.SPD_MSAFILES:
            SpdMsaFiles frmSmf = new SpdMsaFiles(spd);
            getJDesktopPanePackage().add(frmSmf, 1);
            break;
        case IDefaultMutableTreeNode.SPD_PACKAGEHEADERS:
            SpdPackageHeaders frmSph = new SpdPackageHeaders(spd);
            getJDesktopPanePackage().add(frmSph, 1);
            break;
        case IDefaultMutableTreeNode.SPD_GUIDDECLARATIONS:
            SpdGuidDecls frmSgd = new SpdGuidDecls(spd);
            getJDesktopPanePackage().add(frmSgd, 1);
            break;
        case IDefaultMutableTreeNode.SPD_PROTOCOLDECLARATIONS:
            SpdProtocolDecls frmSprod = new SpdProtocolDecls(spd);
            getJDesktopPanePackage().add(frmSprod, 1);
            break;
        case IDefaultMutableTreeNode.SPD_PPIDECLARATIONS:
            SpdPpiDecls frmSppid = new SpdPpiDecls(spd);
            getJDesktopPanePackage().add(frmSppid, 1);
            break;
        case IDefaultMutableTreeNode.SPD_PCDDECLARATIONS:
            SpdPcdDefs frmSpcdd = new SpdPcdDefs(spd);
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
        case IDefaultMutableTreeNode.MSA_MODULEDEFINITIONS:
            ModuleDefinitions frmMd = new ModuleDefinitions(msa);
            getJDesktopPaneModule().add(frmMd, 1);
            break;
        case IDefaultMutableTreeNode.MSA_LIBRARYCLASSDEFINITIONS:
            ModuleLibraryClassDefinitions frmMlcd = new ModuleLibraryClassDefinitions(msa);
            getJDesktopPaneModule().add(frmMlcd, 1);
            break;
        case IDefaultMutableTreeNode.MSA_PACKAGEDEPENDENCIES:
            ModulePackageDependencies frmMpd = new ModulePackageDependencies(msa);
            getJDesktopPaneModule().add(frmMpd, 1);
            break;
        case IDefaultMutableTreeNode.MSA_SOURCEFILES:
            ModuleSourceFiles frmMsf = new ModuleSourceFiles(msa);
            getJDesktopPaneModule().add(frmMsf, 1);
            break;
        case IDefaultMutableTreeNode.MSA_PROTOCOLS:
            ModuleProtocols frmMp = new ModuleProtocols(msa);
            getJDesktopPaneModule().add(frmMp, 1);
            break;
        case IDefaultMutableTreeNode.MSA_EVENTS:
            ModuleEvents frmMe = new ModuleEvents(msa);
            getJDesktopPaneModule().add(frmMe, 1);
            break;
        case IDefaultMutableTreeNode.MSA_HOBS:
            ModuleHobs frmMh = new ModuleHobs(msa);
            getJDesktopPaneModule().add(frmMh, 1);
            break;
        case IDefaultMutableTreeNode.MSA_PPIS:
            ModulePpis frmMpp = new ModulePpis(msa);
            getJDesktopPaneModule().add(frmMpp, 1);
            break;
        case IDefaultMutableTreeNode.MSA_VARIABLES:
            ModuleVariables frmMv = new ModuleVariables(msa);
            getJDesktopPaneModule().add(frmMv, 1);
            break;
        case IDefaultMutableTreeNode.MSA_BOOTMODES:
            ModuleBootModes frmMbm = new ModuleBootModes(msa);
            getJDesktopPaneModule().add(frmMbm, 1);
            break;
        case IDefaultMutableTreeNode.MSA_SYSTEMTABLES:
            ModuleSystemTables frmMst = new ModuleSystemTables(msa);
            getJDesktopPaneModule().add(frmMst, 1);
            break;
        case IDefaultMutableTreeNode.MSA_DATAHUBS:
            ModuleDataHubs frmMdh = new ModuleDataHubs(msa);
            getJDesktopPaneModule().add(frmMdh, 1);
            break;
        case IDefaultMutableTreeNode.MSA_HIIPACKAGES:
            ModuleHiiPackages frmMf = new ModuleHiiPackages(msa);
            getJDesktopPaneModule().add(frmMf, 1);
            break;
        case IDefaultMutableTreeNode.MSA_GUIDS:
            ModuleGuids frmGuid = new ModuleGuids(msa);
            getJDesktopPaneModule().add(frmGuid, 1);
            break;
        case IDefaultMutableTreeNode.MSA_EXTERNS:
            ModuleExterns frmMex = new ModuleExterns(msa);
            getJDesktopPaneModule().add(frmMex, 1);
            break;
        case IDefaultMutableTreeNode.MSA_PCDS:
            ModulePCDs frmPcd = new ModulePCDs(msa);
            getJDesktopPaneModule().add(frmPcd, 1);
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
                PackageSurfaceAreaDocument.PackageSurfaceArea psa = null;
                if (this.openingPackageList.existsPackage(smb.getMid().getPackageId())) {
                    psa = openingPackageList.getPackageSurfaceAreaFromId(smb.getMid().getPackageId());
                }
                try {
                    wt.addModuleToPackage(smb.getMid(), psa);
                } catch (IOException e) {
                    Log.err("Upddate MsaFiles of Package", e.getMessage());
                    e.printStackTrace();
                    return;
                } catch (XmlException e) {
                    Log.err("Upddate MsaFiles of Package", e.getMessage());
                    e.printStackTrace();
                    return;
                } catch (Exception e) {
                    Log.err("Upddate MsaFiles of Package", e.getMessage());
                    e.printStackTrace();
                    return;
                }
                this.openModule(smb.getMid().getPath(), smb.getMid());
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
                try {
                    wt.addPackageToDatabase(smb.getPid());
                } catch (Exception e) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
                vPackageList.addElement(smb.getPid());
                //
                // Add new SpdHeader node to the tree
                //
                IDefaultMutableTreeNode node = new IDefaultMutableTreeNode(vPackageList.lastElement().getName(),
                                                                           IDefaultMutableTreeNode.SPD_HEADER, true,
                                                                           vPackageList.lastElement());
                iTree.addNode(dmtnPackageDescription, node);
                this.openPackage(smb.getPid().getPath());
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
                try {
                    wt.addPlatformToDatabase(smb.getFid());
                } catch (Exception e) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
                vPlatformList.addElement(smb.getFid());
                //
                // Add new SpdHeader node to the tree
                //
                IDefaultMutableTreeNode node = new IDefaultMutableTreeNode(vPlatformList.lastElement().getName(),
                                                                           IDefaultMutableTreeNode.FPD_PLATFORMHEADER,
                                                                           true, vPlatformList.lastElement());
                iTree.addNode(dmtnPlatformDescription, node);
                this.openPlatform(smb.getFid().getPath());
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
                openModule(path);
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
        switch (this.jTabbedPaneEditor.getSelectedIndex()) {
        //
        // Current is module
        //
        case 0:
            if (this.currentOpeningModuleIndex > -1) {
                if (!openingModuleList.getModuleSaved(currentOpeningModuleIndex)) {
                    int result = showSaveDialog();
                    if (result == JOptionPane.YES_OPTION) {
                        this.saveAll();
                    }
                    if (result == JOptionPane.NO_OPTION) {
                        // Do nothing
                    }
                    if (result == JOptionPane.CANCEL_OPTION) {
                        return;
                    }
                }
                iTree.removeNodeChildrenByPath(openingModuleList.getTreePathByIndex(currentOpeningModuleIndex));
                this.openingModuleList.removeFromOpeningModuleListByIndex(this.currentOpeningModuleIndex);
                this.cleanDesktopPaneModule();
                this.currentOpeningModuleIndex = -1;
            }
            break;
        //
        // Current is package
        //    
        case 1:
            if (this.currentOpeningPackageIndex > -1) {
                if (!openingPackageList.getPackageSaved(currentOpeningPackageIndex)) {
                    int result = showSaveDialog();
                    if (result == JOptionPane.YES_OPTION) {
                        this.saveAll();
                    }
                    if (result == JOptionPane.NO_OPTION) {
                        // Do nothing
                    }
                    if (result == JOptionPane.CANCEL_OPTION) {
                        return;
                    }
                }
                iTree.removeNodeChildrenByPath(openingPackageList.getTreePathByIndex(currentOpeningPackageIndex));
                this.openingPackageList.removeFromOpeningPackageListByIndex(this.currentOpeningPackageIndex);
                this.cleanDesktopPanePackage();
                this.currentOpeningPackageIndex = -1;
            }
            break;
        //
        // Current is platform
        //
        case 2:
            if (this.currentOpeningPlatformIndex > -1) {
                if (!openingPlatformList.getPlatformSaved(currentOpeningPlatformIndex)) {
                    int result = showSaveDialog();
                    if (result == JOptionPane.YES_OPTION) {
                        this.saveAll();
                    }
                    if (result == JOptionPane.NO_OPTION) {
                        // Do nothing
                    }
                    if (result == JOptionPane.CANCEL_OPTION) {
                        return;
                    }
                }
                iTree.removeNodeChildrenByPath(openingPlatformList.getTreePathByIndex(currentOpeningPlatformIndex));
                this.openingPlatformList.removeFromOpeningPlatformListByIndex(this.currentOpeningPlatformIndex);
                this.cleanDesktopPanePlatform();
                this.currentOpeningPlatformIndex = -1;
            }
            break;
        }
    }

    /**
     Close all opening files and clean all showing internal frame
     
     **/
    private void closeAll() {
        int result = -1;
        if (!openingModuleList.isSaved() || !openingPackageList.isSaved() || !openingPlatformList.isSaved()) {
            result = showSaveDialog();
        }
        if (result == JOptionPane.YES_OPTION) {
            this.saveAll();
        }
        if (result == JOptionPane.NO_OPTION) {
            // Do nothing
        }
        if (result == JOptionPane.CANCEL_OPTION) {
            return;
        }
        this.cleanDesktopPane();
        this.getCompontentsFromFrameworkDatabase();
        openingModuleList.removeAllFromOpeningModuleList();
        openingPackageList.removeAllFromOpeningPackageList();
        openingPlatformList.removeAllFromOpeningPlatformList();
        this.makeEmptyTree();
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
        for (int index = 0; index < openingModuleList.size(); index++) {
            if (!openingModuleList.getModuleSaved(index)) {
                saveModule(index);
            }
        }

        //
        // Save all packages
        //
        for (int index = 0; index < openingPackageList.size(); index++) {
            if (!openingPackageList.getPackageSaved(index)) {
                savePackage(index);
            }
        }

        //
        // Save all platforms
        //
        for (int index = 0; index < openingPlatformList.size(); index++) {
            if (!openingPlatformList.getPlatformSaved(index)) {
                savePlatform(index);
            }
        }
    }

    /**
     To save changed items before exit.
     
     **/
    private void exit() {
        int result = -1;
        if (!openingModuleList.isSaved() || !openingPackageList.isSaved() || !openingPlatformList.isSaved()) {
            result = showSaveDialog();
        }
        if (result == JOptionPane.YES_OPTION) {
            this.saveAll();
        }
        if (result == JOptionPane.NO_OPTION) {
            // Do nothing
        }
        if (result == JOptionPane.CANCEL_OPTION) {
            return;
        }
        this.dispose();
        System.exit(0);
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
            closeAll();
            this.setTitle(DataType.PROJECT_NAME + " " + DataType.PROJECT_VERSION + " " + "- ["
                          + Workspace.getCurrentWorkspace() + "]");
        }
        sw.dispose();
    }

    /**
     Show Tool Chain Configuration Dialog to setup Tool Chain
     
     **/
    private void setupToolChainConfiguration() {
        ToolChainConfig tcc = ToolChainConfig.getInstance();
        tcc.showDialog();
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
            Log.err("Please select a target to clone!");
            return;
        }
        int category = iTree.getSelectCategory();
        Identification id = iTree.getSelectNode().getId();
        if (category == IDefaultMutableTreeNode.MODULE || category == IDefaultMutableTreeNode.PACKAGE
            || category == IDefaultMutableTreeNode.PLATFORM) {
            Log.err("Please select a target to clone!");
            return;
        }

        if (category == IDefaultMutableTreeNode.WORKSPACE) {
            mode = DataType.RETURN_TYPE_WORKSPACE;
            id = null;
        }
        if (category >= IDefaultMutableTreeNode.MSA_HEADER && category < IDefaultMutableTreeNode.SPD_HEADER) {
            mode = DataType.RETURN_TYPE_MODULE_SURFACE_AREA;
        }
        if (category >= IDefaultMutableTreeNode.SPD_HEADER && category < IDefaultMutableTreeNode.FPD_PLATFORMHEADER) {
            mode = DataType.RETURN_TYPE_PACKAGE_SURFACE_AREA;
        }
        if (category >= IDefaultMutableTreeNode.FPD_PLATFORMHEADER) {
            mode = DataType.RETURN_TYPE_PLATFORM_SURFACE_AREA;
        }

        Clone c = new Clone(this, true, mode, id);
        int result = c.showDialog();

        if (result == DataType.RETURN_TYPE_CANCEL) {
            c.dispose();
        }
        if (result == DataType.RETURN_TYPE_WORKSPACE) {
            Tools.showInformationMessage("Workspace Clone Finished");
        }
        if (result == DataType.RETURN_TYPE_MODULE_SURFACE_AREA) {
            Tools.showInformationMessage("Module Surface Area Clone Finished");
            vModuleList.addElement(c.getMid());
            addModuleToTree(c.getMid());
        }
        if (result == DataType.RETURN_TYPE_PACKAGE_SURFACE_AREA) {
            Tools.showInformationMessage("Package Surface Area Clone Finished");
            vPackageList.addElement(c.getPid());
            //
            // Add new SpdHeader node to the tree
            //
            IDefaultMutableTreeNode node = new IDefaultMutableTreeNode(vPackageList.lastElement().getName(),
                                                                       IDefaultMutableTreeNode.SPD_HEADER, true,
                                                                       vPackageList.lastElement());
            iTree.addNode(dmtnPackageDescription, node);
        }
        if (result == DataType.RETURN_TYPE_PLATFORM_SURFACE_AREA) {
            Tools.showInformationMessage("Platform Surface Area Clone Finished");
            vPlatformList.addElement(c.getFid());
            //
            // Add new SpdHeader node to the tree
            //
            IDefaultMutableTreeNode node = new IDefaultMutableTreeNode(vPlatformList.lastElement().getName(),
                                                                       IDefaultMutableTreeNode.FPD_PLATFORMHEADER,
                                                                       true, vPlatformList.lastElement());
            iTree.addNode(dmtnPlatformDescription, node);
            //this.openPlatform(c.getFid().getPath());
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
            jMenuItemFileClose.setEnabled(openingModuleList.isOpend() || openingPackageList.isOpend()
                                          || openingPlatformList.isOpend());
            jMenuItemFileCloseAll.setEnabled(openingModuleList.isOpend() || openingPackageList.isOpend()
                                             || openingPlatformList.isOpend());

            //
            // Enable save/save all if some files are changed
            //
            jMenuItemFileSave.setEnabled(!openingModuleList.isSaved() || !openingPackageList.isSaved()
                                         || !openingPlatformList.isSaved());
            jMenuItemFileSaveAll.setEnabled(!openingModuleList.isSaved() || !openingPackageList.isSaved()
                                            || !openingPlatformList.isSaved());
        }

        if (arg0.getSource() == jMenuTools) {
            //
            // Enable clone when select some items
            //
            if (iTree.getSelectionPath() == null) {
                jMenuItemToolsClone.setEnabled(false);
            } else {
                int category = iTree.getSelectCategory();
                if (category == IDefaultMutableTreeNode.MODULE || category == IDefaultMutableTreeNode.PACKAGE
                    || category == IDefaultMutableTreeNode.PLATFORM
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
}
