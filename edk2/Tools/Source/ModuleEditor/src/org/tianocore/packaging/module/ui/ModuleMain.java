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

package org.tianocore.packaging.module.ui;

import java.awt.event.ActionEvent;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.awt.event.WindowEvent;
import java.io.File;
import java.io.IOException;

import javax.swing.JButton;
import javax.swing.JDesktopPane;
import javax.swing.JFileChooser;
import javax.swing.JInternalFrame;
import javax.swing.JMenu;
import javax.swing.JMenuBar;
import javax.swing.JMenuItem;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JPopupMenu;
import javax.swing.JScrollPane;
import javax.swing.border.BevelBorder;
import javax.swing.event.TreeSelectionEvent;
import javax.swing.event.TreeSelectionListener;

import org.apache.xmlbeans.XmlCursor;
import org.apache.xmlbeans.XmlException;
import org.apache.xmlbeans.XmlObject;
import org.apache.xmlbeans.XmlOptions;
import org.tianocore.BootModesDocument;
import org.tianocore.BuildOptionsDocument;
import org.tianocore.DataHubsDocument;
import org.tianocore.EventsDocument;
import org.tianocore.ExternsDocument;
import org.tianocore.FormsetsDocument;
import org.tianocore.GuidsDocument;
import org.tianocore.HobsDocument;
import org.tianocore.IncludesDocument;
import org.tianocore.LibrariesDocument;
import org.tianocore.LibraryClassDefinitionsDocument;
import org.tianocore.LibraryModuleBuildDescriptionDocument;
import org.tianocore.LibraryModuleSurfaceAreaDocument;
import org.tianocore.MbdHeaderDocument;
import org.tianocore.MbdLibHeaderDocument;
import org.tianocore.ModuleBuildDescriptionDocument;
import org.tianocore.ModuleSurfaceAreaDocument;
import org.tianocore.MsaHeaderDocument;
import org.tianocore.MsaLibHeaderDocument;
import org.tianocore.PCDsDocument;
import org.tianocore.PPIsDocument;
import org.tianocore.ProtocolsDocument;
import org.tianocore.SourceFilesDocument;
import org.tianocore.SystemTablesDocument;
import org.tianocore.VariablesDocument;
import org.tianocore.common.IFileFilter;
import org.tianocore.common.Log;
import org.tianocore.packaging.common.ui.IDefaultMutableTreeNode;
import org.tianocore.packaging.common.ui.IDesktopManager;
import org.tianocore.packaging.common.ui.IFrame;
import org.tianocore.packaging.common.ui.ITree;
import org.tianocore.packaging.workspace.common.Workspace;

/**
 The class is used to show main GUI of ModuleEditor
 It extends IFrame implements MouseListener, TreeSelectionListener
 
 @since ModuleEditor 1.0

 **/
public class ModuleMain extends IFrame implements MouseListener, TreeSelectionListener {
    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = -7103240960573031772L;

    //
    //Define class members
    //
    private String currentModule = "";

    private String saveFileName = "";

    ///
    ///  0 - reserved; 1 - msa; 2 - mbd; 3 - lmsa; 4 - lmbd;
    ///
    private int currentModuleType = 0;

    private int currentNodeType = -1;

    private String windowTitle = "ModuleEditor 1.0 ";

    private ModuleSurfaceAreaDocument xmlMsaDoc = null;

    private ModuleBuildDescriptionDocument xmlMbdDoc = null;

    private LibraryModuleSurfaceAreaDocument xmlMlsaDoc = null;

    private LibraryModuleBuildDescriptionDocument xmlMlbdDoc = null;

    private MsaHeaderDocument.MsaHeader xmlmh = null;

    private MbdHeaderDocument.MbdHeader xmlmbdh = null;

    private MsaLibHeaderDocument.MsaLibHeader xmlmlh = null;

    private MbdLibHeaderDocument.MbdLibHeader xmlmlbdh = null;

    private LibraryClassDefinitionsDocument.LibraryClassDefinitions xmllcd = null;

    private LibrariesDocument.Libraries xmllib = null;

    private SourceFilesDocument.SourceFiles xmlsf = null;

    private IncludesDocument.Includes xmlic = null;

    private ProtocolsDocument.Protocols xmlpl = null;

    private EventsDocument.Events xmlen = null;

    private HobsDocument.Hobs xmlhob = null;

    private PPIsDocument.PPIs xmlppi = null;

    private VariablesDocument.Variables xmlvb = null;

    private BootModesDocument.BootModes xmlbm = null;

    private SystemTablesDocument.SystemTables xmlst = null;

    private DataHubsDocument.DataHubs xmldh = null;

    private FormsetsDocument.Formsets xmlfs = null;

    private GuidsDocument.Guids xmlgu = null;

    private ExternsDocument.Externs xmlet = null;

    private PCDsDocument.PCDs xmlpcd = null;

    private BuildOptionsDocument.BuildOptions xmlbo = null;

    IDefaultMutableTreeNode dmtnRoot = null;

    private JPanel jContentPane = null;

    private JMenuBar jMenuBar = null;

    private JMenu jMenuModule = null;

    private JMenu jMenuModuleNew = null;

    private JMenuItem jMenuItemModuleNewModule = null;

    private JMenuItem jMenuItemModuleSaveAs = null;

    private JMenuItem jMenuItemModuleExit = null;

    private JMenu jMenuEdit = null;

    private JMenuItem jMenuItemEditAddLibraryClassDefinitions = null;

    private JMenuItem jMenuItemEditAddSourceFiles = null;

    private JMenuItem jMenuItemEditAddIncludes = null;

    private JMenuItem jMenuItemEditAddProtocols = null;

    private JMenuItem jMenuItemEditAddEvents = null;

    private JMenuItem jMenuItemEditAddHobs = null;

    private JMenuItem jMenuItemEditAddPPIs = null;

    private JMenuItem jMenuItemEditAddVariables = null;

    private JMenuItem jMenuItemEditAddBootModes = null;

    private JMenuItem jMenuItemEditAddSystemTables = null;

    private JMenuItem jMenuItemEditAddDataHubs = null;

    private JMenuItem jMenuItemEditAddFormsets = null;

    private JMenuItem jMenuItemEditAddGuids = null;

    private JMenuItem jMenuItemEditAddExterns = null;

    private JMenuItem jMenuItemEditAddPCDs = null;

    private JDesktopPane jDesktopPane = null;

    private IDesktopManager iDesktopManager = new IDesktopManager();

    private JScrollPane jScrollPaneTree = null;

    private ITree iTree = null;

    private JMenu jMenuHelp = null;

    private JMenuItem jMenuItemHelpAbout = null;

    private JMenu jMenuEditAdd = null;

    private JMenuItem jMenuItemEditDelete = null;

    private JMenuItem jMenuItemEditUpdate = null;

    private JPopupMenu jPopupMenu = null;

    private JMenuItem jMenuItemPopupAdd = null;

    private JMenuItem jMenuItemPopupUpdate = null;

    private JMenuItem jMenuItemPopupDelete = null;

    private Workspace ws = new Workspace();

    private static final int OPENED = 0;

    private static final int CLOSED = 1;

    private static final int NEW_WITHOUT_CHANGE = 2;

    private static final int NEW_WITH_CHANGE = 3;

    private static final int UPDATE_WITHOUT_CHANGE = 4;

    private static final int UPDATE_WITH_CHANGE = 5;

    private static final int SAVE_WITHOUT_CHANGE = 6;

    private static final int SAVE_WITH_CHANGE = 7;

    private static final int ADD = 1;

    private static final int UPDATE = 2;

    //private static final int DELETE = 3;

    private static final int VIEW = 4;

    private MsaHeader msa = null;

    private MbdHeader mbd = null;

    private MsaLibHeader mlsa = null;

    private MbdLibHeader mlbd = null;

    private ModuleLibraryClassDefinitions mlcd = null;

    private MbdLibraries mlib = null;

    private ModuleSourceFiles msf = null;

    private ModuleIncludes mic = null;

    private ModuleProtocols mp = null;

    private ModuleEvents mev = null;

    private ModuleHobs mh = null;

    private ModulePpis mpp = null;

    private ModuleVariables mv = null;

    private ModuleBootModes mbm = null;

    private ModuleSystemTables mst = null;

    private ModuleDataHubs mdh = null;

    private ModuleFormsets mf = null;

    private ModuleGuids mg = null;

    private ModuleExterns met = null;

    private ModulePCDs mpcd = null;

    private JMenuItem jMenuItemModuleOpenModule = null;

    private JMenuItem jMenuItemModuleNewModuleBuildDescription = null;

    private JMenuItem jMenuItemModuleNewLibraryModule = null;

    private JMenuItem jMenuItemModuleNewLibraryModuleBuildDescription = null;

    private JMenu jMenuModuleOpen = null;

    private JMenuItem jMenuItemModuleOpenModuleBuildDescription = null;

    private JMenuItem jMenuItemModuleOpenLibraryModule = null;

    private JMenuItem jMenuItemModuleOpenLibraryModuleBuildDescription = null;

    private JMenuItem jMenuItemModuleSave = null;

    private JMenuItem jMenuItemModuleClose = null;

    private JMenu jMenuTools = null;

    private JMenu jMenuWindow = null;

    private JMenuItem jMenuItemEditAddLibraries = null;

    private JPanel jPanelOperation = null;

    private JButton jButtonOk = null;

    private JButton jButtonCancel = null;

    /**
     This method initializes jMenuBar 
     
     @return javax.swing.JMenuBar Main menu bar for the entire GUI
     
     **/
    private JMenuBar getjJMenuBar() {
        if (jMenuBar == null) {
            jMenuBar = new JMenuBar();
            jMenuBar.setPreferredSize(new java.awt.Dimension(0, 18));
            jMenuBar.add(getJMenuModule());
            jMenuBar.add(getJMenuEdit());
            jMenuBar.add(getJMenuTools());
            jMenuBar.add(getJMenuWindow());
            jMenuBar.add(getJMenuHelp());
        }
        return jMenuBar;
    }

    /**
     This method initializes jMenuFile 
     
     @return javax.swing.JMenu jMenuModule
     
     **/
    private JMenu getJMenuModule() {
        if (jMenuModule == null) {
            jMenuModule = new JMenu();
            jMenuModule.setText("Module");
            jMenuModule.add(getJMenuModuleNew());
            jMenuModule.addSeparator();
            jMenuModule.add(getJMenuModuleOpen());
            jMenuModule.addSeparator();
            jMenuModule.add(getJMenuItemModuleSave());
            jMenuModule.add(getJMenuItemModuleSaveAs());
            jMenuModule.addSeparator();
            jMenuModule.add(getJMenuItemModuleClose());
            jMenuModule.addSeparator();
            jMenuModule.add(getJMenuItemModuleExit());
        }
        return jMenuModule;
    }

    /**
     This method initializes jMenuItemModuleNewModule 
     
     @return javax.swing.JMenuItem jMenuItemModuleNewModule
     
     **/
    private JMenuItem getJMenuItemModuleNewModule() {
        if (jMenuItemModuleNewModule == null) {
            jMenuItemModuleNewModule = new JMenuItem();
            jMenuItemModuleNewModule.setText("Module (.msa)");
            jMenuItemModuleNewModule.addActionListener(this);
        }
        return jMenuItemModuleNewModule;
    }

    /**
     This method initializes jMenuItemModuleSaveAs 
     
     @return javax.swing.JMenuItem jMenuItemModuleSaveAs
     
     **/
    private JMenuItem getJMenuItemModuleSaveAs() {
        if (jMenuItemModuleSaveAs == null) {
            jMenuItemModuleSaveAs = new JMenuItem();
            jMenuItemModuleSaveAs.setText("Save As...");
            jMenuItemModuleSaveAs.addActionListener(this);
            jMenuItemModuleSaveAs.setEnabled(false);
        }
        return jMenuItemModuleSaveAs;
    }

    /**
     This method initializes jMenuItemModuleExit 
     
     @return javax.swing.JMenuItem jMenuItemModuleExit
     
     **/
    private JMenuItem getJMenuItemModuleExit() {
        if (jMenuItemModuleExit == null) {
            jMenuItemModuleExit = new JMenuItem();
            jMenuItemModuleExit.setText("Exit");
            jMenuItemModuleExit.addActionListener(this);
        }
        return jMenuItemModuleExit;
    }

    /**
     This method initializes jMenuEdit 
     
     @return javax.swing.JMenu jMenuEdit
     
     **/
    private JMenu getJMenuEdit() {
        if (jMenuEdit == null) {
            jMenuEdit = new JMenu();
            jMenuEdit.setText("Edit");
            jMenuEdit.add(getJMenu());
            jMenuEdit.add(getJMenuItemEditUpdate());
            jMenuEdit.add(getJMenuItemEditDelete());
        }
        return jMenuEdit;
    }

    /**
     This method initializes jMenuItemEditAddLibraryClassDefinitions 
     
     @return javax.swing.JMenuItem jMenuItemEditAddLibraryClassDefinitions
     
     **/
    private JMenuItem getJMenuItemEditAddLibraryClassDefinitions() {
        if (jMenuItemEditAddLibraryClassDefinitions == null) {
            jMenuItemEditAddLibraryClassDefinitions = new JMenuItem();
            jMenuItemEditAddLibraryClassDefinitions.setText("Library Class Definitions");
            jMenuItemEditAddLibraryClassDefinitions.addActionListener(this);
        }
        return jMenuItemEditAddLibraryClassDefinitions;
    }

    /**
     This method initializes jMenuItemEditAddSourceFiles 
     
     @return javax.swing.JMenuItem jMenuItemEditAddSourceFiles
     
     **/
    private JMenuItem getJMenuItemEditAddSourceFiles() {
        if (jMenuItemEditAddSourceFiles == null) {
            jMenuItemEditAddSourceFiles = new JMenuItem();
            jMenuItemEditAddSourceFiles.setText("Source Files");
            jMenuItemEditAddSourceFiles.addActionListener(this);
        }
        return jMenuItemEditAddSourceFiles;
    }

    /**
     This method initializes jMenuItemEditAddIncludes 
     
     @return javax.swing.JMenuItem jMenuItemEditAddIncludes
     
     **/
    private JMenuItem getJMenuItemEditAddIncludes() {
        if (jMenuItemEditAddIncludes == null) {
            jMenuItemEditAddIncludes = new JMenuItem();
            jMenuItemEditAddIncludes.setText("Includes");
            jMenuItemEditAddIncludes.addActionListener(this);
        }
        return jMenuItemEditAddIncludes;
    }

    /**
     This method initializes jMenuItemEditAddProtocols 
     
     @return javax.swing.JMenuItem jMenuItemEditAddProtocols
     
     **/
    private JMenuItem getJMenuItemEditAddProtocols() {
        if (jMenuItemEditAddProtocols == null) {
            jMenuItemEditAddProtocols = new JMenuItem();
            jMenuItemEditAddProtocols.setText("Protocols");
            jMenuItemEditAddProtocols.addActionListener(this);
        }
        return jMenuItemEditAddProtocols;
    }

    /**
     This method initializes jMenuItemEditAddEvents 
     
     @return javax.swing.JMenuItem jMenuItemEditAddEvents
     
     **/
    private JMenuItem getJMenuItemEditAddEvents() {
        if (jMenuItemEditAddEvents == null) {
            jMenuItemEditAddEvents = new JMenuItem();
            jMenuItemEditAddEvents.setText("Events");
            jMenuItemEditAddEvents.addActionListener(this);
        }
        return jMenuItemEditAddEvents;
    }

    /**
     This method initializes jMenuItemEditAddHobs 
     
     @return javax.swing.JMenuItem jMenuItemEditAddHobs
     
     **/
    private JMenuItem getJMenuItemEditAddHobs() {
        if (jMenuItemEditAddHobs == null) {
            jMenuItemEditAddHobs = new JMenuItem();
            jMenuItemEditAddHobs.setText("Hobs");
            jMenuItemEditAddHobs.addActionListener(this);
        }
        return jMenuItemEditAddHobs;
    }

    /**
     This method initializes jMenuItemEditAddPPIs 
     
     @return javax.swing.JMenuItem jMenuItemEditAddPPIs
     
     **/
    private JMenuItem getJMenuItemEditAddPPIs() {
        if (jMenuItemEditAddPPIs == null) {
            jMenuItemEditAddPPIs = new JMenuItem();
            jMenuItemEditAddPPIs.setText("PPIs");
            jMenuItemEditAddPPIs.addActionListener(this);
        }
        return jMenuItemEditAddPPIs;
    }

    /**
     This method initializes jMenuItemEditAddVariables 
     
     @return javax.swing.JMenuItem jMenuItemEditAddVariables
     
     **/
    private JMenuItem getJMenuItemEditAddVariables() {
        if (jMenuItemEditAddVariables == null) {
            jMenuItemEditAddVariables = new JMenuItem();
            jMenuItemEditAddVariables.setText("Variables");
            jMenuItemEditAddVariables.addActionListener(this);
        }
        return jMenuItemEditAddVariables;
    }

    /**
     This method initializes jMenuItemEditAddBootModes 
     
     @return javax.swing.JMenuItem jMenuItemEditAddBootModes
     
     **/
    private JMenuItem getJMenuItemAddBootModes() {
        if (jMenuItemEditAddBootModes == null) {
            jMenuItemEditAddBootModes = new JMenuItem();
            jMenuItemEditAddBootModes.setText("Boot Modes");
            jMenuItemEditAddBootModes.addActionListener(this);
        }
        return jMenuItemEditAddBootModes;
    }

    /**
     This method initializes jMenuItemEditAddSystemTables 
     
     @return javax.swing.JMenuItem jMenuItemEditAddSystemTables
     
     **/
    private JMenuItem getJMenuItemAddSystemTables() {
        if (jMenuItemEditAddSystemTables == null) {
            jMenuItemEditAddSystemTables = new JMenuItem();
            jMenuItemEditAddSystemTables.setText("System Tables");
            jMenuItemEditAddSystemTables.addActionListener(this);
        }
        return jMenuItemEditAddSystemTables;
    }

    /**
     This method initializes jMenuItemEditAddDataHubs 
     
     @return javax.swing.JMenuItem jMenuItemEditAddDataHubs
     
     **/
    private JMenuItem getJMenuItemEditAddDataHubs() {
        if (jMenuItemEditAddDataHubs == null) {
            jMenuItemEditAddDataHubs = new JMenuItem();
            jMenuItemEditAddDataHubs.setText("Data Hubs");
            jMenuItemEditAddDataHubs.addActionListener(this);
        }
        return jMenuItemEditAddDataHubs;
    }

    /**
     This method initializes jMenuItemEditAddFormsets 
     
     @return javax.swing.JMenuItem jMenuItemEditAddFormsets
     
     **/
    private JMenuItem getJMenuItemEditAddFormsets() {
        if (jMenuItemEditAddFormsets == null) {
            jMenuItemEditAddFormsets = new JMenuItem();
            jMenuItemEditAddFormsets.setText("Formsets");
            jMenuItemEditAddFormsets.addActionListener(this);
        }
        return jMenuItemEditAddFormsets;
    }

    /**
     This method initializes jMenuItemEditAddGuids 
     
     @return javax.swing.JMenuItem jMenuItemEditAddGuids
     
     **/
    private JMenuItem getJMenuItemEditAddGuids() {
        if (jMenuItemEditAddGuids == null) {
            jMenuItemEditAddGuids = new JMenuItem();
            jMenuItemEditAddGuids.setText("Guids");
            jMenuItemEditAddGuids.addActionListener(this);
        }
        return jMenuItemEditAddGuids;
    }

    /**
     This method initializes jMenuItemEditAddExterns 
     
     @return javax.swing.JMenuItem jMenuItemEditAddExterns
     
     **/
    private JMenuItem getJMenuItemEditAddExterns() {
        if (jMenuItemEditAddExterns == null) {
            jMenuItemEditAddExterns = new JMenuItem();
            jMenuItemEditAddExterns.setText("Externs");
            jMenuItemEditAddExterns.addActionListener(this);
        }
        return jMenuItemEditAddExterns;
    }

    /**
     This method initializes jMenuItemEditAddPCDs 
     
     @return javax.swing.JMenuItem jMenuItemEditAddPCDs
     
     **/
    private JMenuItem getJMenuItemEditAddPCDs() {
        if (jMenuItemEditAddPCDs == null) {
            jMenuItemEditAddPCDs = new JMenuItem();
            jMenuItemEditAddPCDs.setText("PCDs");
            jMenuItemEditAddPCDs.addActionListener(this);
        }
        return jMenuItemEditAddPCDs;
    }

    /**
     This method initializes jDesktopPane 
     
     @return javax.swing.JDesktopPane jDesktopPane
     
     **/
    private JDesktopPane getJDesktopPane() {
        if (jDesktopPane == null) {
            jDesktopPane = new JDesktopPane();
            jDesktopPane.setBounds(new java.awt.Rectangle(295, 1, 500, 515));
            jDesktopPane.setDesktopManager(iDesktopManager);
        }
        return jDesktopPane;
    }

    /**
     This method initializes jScrollPaneTree 
     
     @return javax.swing.JScrollPane jScrollPaneTree
     
     **/
    private JScrollPane getJScrollPaneTree() {
        if (jScrollPaneTree == null) {
            jScrollPaneTree = new JScrollPane();
            jScrollPaneTree.setBounds(new java.awt.Rectangle(0, 1, 290, 545));
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
        IDefaultMutableTreeNode root = new IDefaultMutableTreeNode("No Msa/Mbd file opened", -1, -1);
        iTree = new ITree(root);
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
     This method initializes jMenuEditAdd 
     
     @return javax.swing.JMenu jMenuEditAdd
     
     **/
    private JMenu getJMenu() {
        if (jMenuEditAdd == null) {
            jMenuEditAdd = new JMenu();
            jMenuEditAdd.setText("Add");
            //
            //Add all menu items of menu "Add"
            //
            jMenuEditAdd.add(getJMenuItemEditAddLibraries());
            jMenuEditAdd.add(getJMenuItemEditAddLibraryClassDefinitions());
            jMenuEditAdd.add(getJMenuItemEditAddSourceFiles());
            jMenuEditAdd.add(getJMenuItemEditAddIncludes());
            jMenuEditAdd.add(getJMenuItemEditAddProtocols());
            jMenuEditAdd.add(getJMenuItemEditAddEvents());
            jMenuEditAdd.add(getJMenuItemEditAddHobs());
            jMenuEditAdd.add(getJMenuItemEditAddPPIs());
            jMenuEditAdd.add(getJMenuItemEditAddVariables());
            jMenuEditAdd.add(getJMenuItemAddBootModes());
            jMenuEditAdd.add(getJMenuItemAddSystemTables());
            jMenuEditAdd.add(getJMenuItemEditAddDataHubs());
            jMenuEditAdd.add(getJMenuItemEditAddFormsets());
            jMenuEditAdd.add(getJMenuItemEditAddGuids());
            jMenuEditAdd.add(getJMenuItemEditAddExterns());
            jMenuEditAdd.add(getJMenuItemEditAddPCDs());
            jMenuEditAdd.setEnabled(false);
        }
        return jMenuEditAdd;
    }

    /**
     This method initializes jMenuItemEditDelete 
     
     @return javax.swing.JMenuItem jMenuItemEditDelete
     
     **/
    private JMenuItem getJMenuItemEditDelete() {
        if (jMenuItemEditDelete == null) {
            jMenuItemEditDelete = new JMenuItem();
            jMenuItemEditDelete.setText("Delete");
            jMenuItemEditDelete.addActionListener(this);
            //
            //Disabled when no module is open
            //
            jMenuItemEditDelete.setEnabled(false);
        }
        return jMenuItemEditDelete;
    }

    /**
     This method initializes jMenuItemEditUpdate 
     
     @return javax.swing.JMenuItem jMenuItemEditUpdate
     
     **/
    private JMenuItem getJMenuItemEditUpdate() {
        if (jMenuItemEditUpdate == null) {
            jMenuItemEditUpdate = new JMenuItem();
            jMenuItemEditUpdate.setText("Update");
            jMenuItemEditUpdate.addActionListener(this);
            //
            //Disabled when no module is open
            //
            jMenuItemEditUpdate.setEnabled(false);
        }
        return jMenuItemEditUpdate;
    }

    /**
     This method initializes jPopupMenu 
     
     @return javax.swing.JPopupMenu jPopupMenu
     
     **/
    private JPopupMenu getJPopupMenu() {
        if (jPopupMenu == null) {
            jPopupMenu = new JPopupMenu();
            //
            //Add menu items of popup menu
            //
            jPopupMenu.add(getJMenuItemPopupAdd());
            jPopupMenu.add(getJMenuItemPopupUpdate());
            jPopupMenu.add(getJMenuItemPopupDelete());
            jPopupMenu.setBorder(new BevelBorder(BevelBorder.RAISED));
            jPopupMenu.addMouseListener(this);
        }
        return jPopupMenu;
    }

    /**
     This method initializes jMenuItemPopupAdd 
     
     @return javax.swing.JMenuItem jMenuItemPopupAdd
     
     **/
    private JMenuItem getJMenuItemPopupAdd() {
        if (jMenuItemPopupAdd == null) {
            jMenuItemPopupAdd = new JMenuItem();
            jMenuItemPopupAdd.setText("Add");
            jMenuItemPopupAdd.addActionListener(this);
            jMenuItemPopupAdd.setEnabled(false);
        }
        return jMenuItemPopupAdd;
    }

    /**
     This method initializes jMenuItemPopupUpdate 
     
     @return javax.swing.JMenuItem jMenuItemPopupUpdate
     
     **/
    private JMenuItem getJMenuItemPopupUpdate() {
        if (jMenuItemPopupUpdate == null) {
            jMenuItemPopupUpdate = new JMenuItem();
            jMenuItemPopupUpdate.setText("Update");
            jMenuItemPopupUpdate.addActionListener(this);
            jMenuItemPopupUpdate.setEnabled(false);
        }
        return jMenuItemPopupUpdate;
    }

    /**
     This method initializes jMenuItemPopupDelete 
     
     @return javax.swing.JMenuItem jMenuItemPopupDelete
     
     **/
    private JMenuItem getJMenuItemPopupDelete() {
        if (jMenuItemPopupDelete == null) {
            jMenuItemPopupDelete = new JMenuItem();
            jMenuItemPopupDelete.setText("Delete");
            jMenuItemPopupDelete.addActionListener(this);
            jMenuItemPopupDelete.setEnabled(false);
        }
        return jMenuItemPopupDelete;
    }

    /**
     This method initializes jMenuModuleNew 
     
     @return javax.swing.JMenu jMenuModuleNew
     
     **/
    private JMenu getJMenuModuleNew() {
        if (jMenuModuleNew == null) {
            jMenuModuleNew = new JMenu();
            jMenuModuleNew.setText("New");
            jMenuModuleNew.add(getJMenuItemModuleNewModule());
            jMenuModuleNew.add(getJMenuItemModuleNewModuleBuildDescription());
            jMenuModuleNew.add(getJMenuItemModuleNewLibraryModule());
            jMenuModuleNew.add(getJMenuItemModuleNewLibraryModuleBuildDescription());
        }
        return jMenuModuleNew;
    }

    /**
     This method initializes jMenuItemModuleOpenModule 
     
     @return javax.swing.JMenuItem jMenuItemModuleOpenModule
     
     **/
    private JMenuItem getJMenuItemModuleOpenModule() {
        if (jMenuItemModuleOpenModule == null) {
            jMenuItemModuleOpenModule = new JMenuItem();
            jMenuItemModuleOpenModule.setText("Module (.msa)");
            jMenuItemModuleOpenModule.addActionListener(this);
        }
        return jMenuItemModuleOpenModule;
    }

    /**
     This method initializes jMenuItemFileNewModuleBuildDescription 
     
     @return javax.swing.JMenuItem jMenuItemModuleNewModuleBuildDescription
     
     **/
    private JMenuItem getJMenuItemModuleNewModuleBuildDescription() {
        if (jMenuItemModuleNewModuleBuildDescription == null) {
            jMenuItemModuleNewModuleBuildDescription = new JMenuItem();
            jMenuItemModuleNewModuleBuildDescription.setText("Module Build Description (.mbd)");
            jMenuItemModuleNewModuleBuildDescription.addActionListener(this);
        }
        return jMenuItemModuleNewModuleBuildDescription;
    }

    /**
     This method initializes jMenuItemFileNewLibraryModule 
     
     @return javax.swing.JMenuItem jMenuItemModuleNewLibraryModule
     
     **/
    private JMenuItem getJMenuItemModuleNewLibraryModule() {
        if (jMenuItemModuleNewLibraryModule == null) {
            jMenuItemModuleNewLibraryModule = new JMenuItem();
            jMenuItemModuleNewLibraryModule.setText("Library Module (*.msa)");
            jMenuItemModuleNewLibraryModule.addActionListener(this);
        }
        return jMenuItemModuleNewLibraryModule;
    }

    /**
     This method initializes jMenuItemFileNewLibraryModuleBuildDescription 
     
     @return javax.swing.JMenuItem jMenuItemModuleNewLibraryModuleBuildDescription
     
     **/
    private JMenuItem getJMenuItemModuleNewLibraryModuleBuildDescription() {
        if (jMenuItemModuleNewLibraryModuleBuildDescription == null) {
            jMenuItemModuleNewLibraryModuleBuildDescription = new JMenuItem();
            jMenuItemModuleNewLibraryModuleBuildDescription.setText("Library Module Build Description (.mbd)");
            jMenuItemModuleNewLibraryModuleBuildDescription.addActionListener(this);
        }
        return jMenuItemModuleNewLibraryModuleBuildDescription;
    }

    /**
     This method initializes jMenuOpen 
     
     @return javax.swing.JMenu jMenuModuleOpen
     
     **/
    private JMenu getJMenuModuleOpen() {
        if (jMenuModuleOpen == null) {
            jMenuModuleOpen = new JMenu();
            jMenuModuleOpen.setText("Open");
            jMenuModuleOpen.add(getJMenuItemModuleOpenModule());
            jMenuModuleOpen.add(getJMenuItemModuleOpenModuleBuildDescription());
            jMenuModuleOpen.add(getJMenuItemModuleOpenLibraryModule());
            jMenuModuleOpen.add(getJMenuItemModuleOpenLibraryModuleBuildDescription());
        }
        return jMenuModuleOpen;
    }

    /**
     This method initializes jMenuItemFileOpenModuleBuildDescription 
     
     @return javax.swing.JMenuItem jMenuItemModuleOpenModuleBuildDescription
     
     **/
    private JMenuItem getJMenuItemModuleOpenModuleBuildDescription() {
        if (jMenuItemModuleOpenModuleBuildDescription == null) {
            jMenuItemModuleOpenModuleBuildDescription = new JMenuItem();
            jMenuItemModuleOpenModuleBuildDescription.setText("Module Build Description (.mbd)");
            jMenuItemModuleOpenModuleBuildDescription.addActionListener(this);
        }
        return jMenuItemModuleOpenModuleBuildDescription;
    }

    /**
     This method initializes jMenuItemFileOpenLibraryModule 
     
     @return javax.swing.JMenuItem jMenuItemModuleOpenLibraryModule
     
     **/
    private JMenuItem getJMenuItemModuleOpenLibraryModule() {
        if (jMenuItemModuleOpenLibraryModule == null) {
            jMenuItemModuleOpenLibraryModule = new JMenuItem();
            jMenuItemModuleOpenLibraryModule.setText("Library Module (.msa)");
            jMenuItemModuleOpenLibraryModule.addActionListener(this);
        }
        return jMenuItemModuleOpenLibraryModule;
    }

    /**
     This method initializes jMenuItemFileOpenLibraryModuleBuildDescription 
     
     @return javax.swing.JMenuItem jMenuItemModuleOpenLibraryModuleBuildDescription
     
     **/
    private JMenuItem getJMenuItemModuleOpenLibraryModuleBuildDescription() {
        if (jMenuItemModuleOpenLibraryModuleBuildDescription == null) {
            jMenuItemModuleOpenLibraryModuleBuildDescription = new JMenuItem();
            jMenuItemModuleOpenLibraryModuleBuildDescription.setText("Library Module Build Description (.mbd)");
            jMenuItemModuleOpenLibraryModuleBuildDescription.addActionListener(this);
        }
        return jMenuItemModuleOpenLibraryModuleBuildDescription;
    }

    /**
     This method initializes jMenuItemFileSave 
     
     @return javax.swing.JMenuItem jMenuItemModuleSave
     
     **/
    private JMenuItem getJMenuItemModuleSave() {
        if (jMenuItemModuleSave == null) {
            jMenuItemModuleSave = new JMenuItem();
            jMenuItemModuleSave.setText("Save");
            jMenuItemModuleSave.addActionListener(this);
            jMenuItemModuleSave.setEnabled(false);
        }
        return jMenuItemModuleSave;
    }

    /**
     This method initializes jMenuItemModuleClose 
     
     @return javax.swing.JMenuItem jMenuItemModuleClose
     
     **/
    private JMenuItem getJMenuItemModuleClose() {
        if (jMenuItemModuleClose == null) {
            jMenuItemModuleClose = new JMenuItem();
            jMenuItemModuleClose.setText("Close");
            jMenuItemModuleClose.setEnabled(false);
            jMenuItemModuleClose.addActionListener(this);
        }
        return jMenuItemModuleClose;
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
            jMenuTools.addActionListener(this);
            jMenuTools.setEnabled(false);
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
            jMenuWindow.setEnabled(false);
            jMenuWindow.addActionListener(this);
        }
        return jMenuWindow;
    }

    /**
     This method initializes jMenuItemEditAddLibraries 
     
     @return javax.swing.JMenuItem jMenuItemEditAddLibraries
     
     **/
    private JMenuItem getJMenuItemEditAddLibraries() {
        if (jMenuItemEditAddLibraries == null) {
            jMenuItemEditAddLibraries = new JMenuItem();
            jMenuItemEditAddLibraries.setText("Libraries");
            jMenuItemEditAddLibraries.addActionListener(this);
        }
        return jMenuItemEditAddLibraries;
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

    /* (non-Javadoc)
     * @see org.tianocore.packaging.common.ui.IFrame#main(java.lang.String[])
     *
     * Main class, start the GUI
     * 
     */
    public static void main(String[] args) {
        ModuleMain module = new ModuleMain();
        module.setVisible(true);
    }

    /**
     This is the default constructor
     
     **/
    public ModuleMain() {
        super();
        init();
    }

    /**
     This method initializes this
     
     
     **/
    private void init() {
        //
        // Check if exists WORKSPACE
        // 
        //
        if (!ws.checkCurrentWorkspace()) {
            JOptionPane.showConfirmDialog(null, "You haven't a workspace yet. Please setup first.", "Warning",
                                          JOptionPane.DEFAULT_OPTION, JOptionPane.WARNING_MESSAGE);
            this.dispose();
            System.exit(0);
        }

        this.setSize(800, 600);
        this.setResizable(false);
        this.setJMenuBar(getjJMenuBar());
        this.setContentPane(getJContentPane());
        this.setTitle(windowTitle + "- [" + ws.getCurrentWorkspace() + "]");
        this.setExitType(1);
        this.centerWindow();
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
            jContentPane.add(getJDesktopPane(), null);
            jContentPane.add(getJScrollPaneTree(), null);
            jContentPane.add(getJPopupMenu(), null);
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
        // Open relevant frame via clicking different menu items
        //
        if (arg0.getSource() == jMenuItemHelpAbout) {
            ModuleAbout ma = new ModuleAbout();
            ma.setEdited(false);
        }

        if (arg0.getSource() == jMenuItemEditAddLibraries) {
            showLibraries(ModuleMain.ADD, IDefaultMutableTreeNode.LIBRARIES, -1);
        }

        if (arg0.getSource() == jMenuItemEditAddLibraryClassDefinitions) {
            showLibraryClassDefinitions(ModuleMain.ADD, IDefaultMutableTreeNode.LIBRARYCLASSDEFINITIONS);
        }

        if (arg0.getSource() == jMenuItemEditAddSourceFiles) {
            showSourceFiles(ModuleMain.ADD, IDefaultMutableTreeNode.SOURCEFILES, -1);
        }

        if (arg0.getSource() == jMenuItemEditAddIncludes) {
            showIncludes(ModuleMain.ADD, IDefaultMutableTreeNode.INCLUDES, -1);
        }

        if (arg0.getSource() == jMenuItemEditAddProtocols) {
            showProtocols(ModuleMain.ADD, IDefaultMutableTreeNode.PROTOCOLS, -1);
        }

        if (arg0.getSource() == jMenuItemEditAddEvents) {
            showEvents(ModuleMain.ADD, IDefaultMutableTreeNode.EVENTS, -1);
        }

        if (arg0.getSource() == jMenuItemEditAddHobs) {
            showHobs(ModuleMain.ADD, IDefaultMutableTreeNode.HOBS, -1);
        }

        if (arg0.getSource() == jMenuItemEditAddPPIs) {
            showPpis(ModuleMain.ADD, IDefaultMutableTreeNode.PPIS, -1);
        }

        if (arg0.getSource() == jMenuItemEditAddVariables) {
            showVariables(ModuleMain.ADD, IDefaultMutableTreeNode.VARIABLES, -1);
        }

        if (arg0.getSource() == jMenuItemEditAddBootModes) {
            showBootModes(ModuleMain.ADD, IDefaultMutableTreeNode.BOOTMODES, -1);
        }

        if (arg0.getSource() == jMenuItemEditAddSystemTables) {
            showSystemTables(ModuleMain.ADD, IDefaultMutableTreeNode.SYSTEMTABLES, -1);
        }

        if (arg0.getSource() == jMenuItemEditAddDataHubs) {
            showDataHubs(ModuleMain.ADD, IDefaultMutableTreeNode.DATAHUBS, -1);
        }

        if (arg0.getSource() == jMenuItemEditAddFormsets) {
            showFormsets(ModuleMain.ADD, IDefaultMutableTreeNode.FORMSETS, -1);
        }

        if (arg0.getSource() == jMenuItemEditAddGuids) {
            showGuids(ModuleMain.ADD, IDefaultMutableTreeNode.GUIDS, -1);
        }

        if (arg0.getSource() == jMenuItemEditAddExterns) {
            showExterns(ModuleMain.ADD, IDefaultMutableTreeNode.EXTERNS, -1);
        }

        if (arg0.getSource() == jMenuItemEditAddPCDs) {
            showPCDs(ModuleMain.ADD, IDefaultMutableTreeNode.PCDS, -1);
        }

        if (arg0.getSource() == jMenuItemModuleNewModule) {
            this.closeCurrentModule();
            showMsaHeader(ModuleMain.ADD);
        }

        if (arg0.getSource() == jMenuItemModuleNewModuleBuildDescription) {
            this.closeCurrentModule();
            showMbdHeader(ModuleMain.ADD);
        }

        if (arg0.getSource() == jMenuItemModuleNewLibraryModule) {
            this.closeCurrentModule();
            showMlsaHeader(ModuleMain.ADD);
        }

        if (arg0.getSource() == jMenuItemModuleNewLibraryModuleBuildDescription) {
            this.closeCurrentModule();
            showMlbdHeader(ModuleMain.ADD);
        }

        //
        // Open Msa, Mbd, Lmsa and Lmbd
        //
        if (arg0.getSource() == jMenuItemModuleOpenModule) {
            openFile(1, 1);
        }

        if (arg0.getSource() == jMenuItemModuleOpenModuleBuildDescription) {
            openFile(1, 2);
        }

        if (arg0.getSource() == jMenuItemModuleOpenLibraryModule) {
            openFile(1, 3);
        }

        if (arg0.getSource() == jMenuItemModuleOpenLibraryModuleBuildDescription) {
            openFile(1, 4);
        }

        //
        // Listen popup menu items
        //
        if (arg0.getSource() == jMenuItemPopupAdd) {
            int intCategory = iTree.getSelectCategory();
            int intLocation = iTree.getSelectLoaction();
            addCurrentModule(intCategory, intLocation);
        }

        if (arg0.getSource() == jMenuItemPopupUpdate || arg0.getSource() == jMenuItemEditUpdate) {
            int intCategory = iTree.getSelectCategory();
            int intLocation = iTree.getSelectLoaction();
            updateCurrentModule(intCategory, intLocation);
        }

        if (arg0.getSource() == jMenuItemPopupDelete || arg0.getSource() == jMenuItemEditDelete) {
            int intCategory = iTree.getSelectCategory();
            int intLocation = iTree.getSelectLoaction();
            deleteCurrentModule(intCategory, intLocation);
        }

        if (arg0.getSource() == jMenuItemModuleExit) {
            this.onExit();
        }

        if (arg0.getSource() == jMenuItemModuleClose) {
            closeCurrentModule();
        }

        if (arg0.getSource() == jMenuItemModuleSaveAs) {
            saveAsCurrentModule();
        }

        if (arg0.getSource() == jMenuItemModuleSave) {
            saveCurrentModule();
        }

        if (arg0.getSource() == jButtonOk) {
            save();
        }

        if (arg0.getSource() == jButtonCancel) {

        }
    }

    /**
     Open file
     
     @param intOperationType Open - 1 or Save - 2
     @param intFileType Msa - 1, Mbd - 2, Lmsa - 3, Lmbd - 4
     @return opened file path
     
     **/
    private void openFile(int intOperationType, int intFileType) {
        String strCurrentPath = "";
        if (this.currentModule == "") {
            strCurrentPath = ws.getCurrentWorkspace();
        } else {
            strCurrentPath = this.currentModule
                                               .substring(
                                                          0,
                                                          this.currentModule
                                                                            .lastIndexOf(System
                                                                                               .getProperty("file.separator")));
        }

        JFileChooser fc = new JFileChooser(strCurrentPath);
        fc.setAcceptAllFileFilterUsed(false);
        switch (intOperationType) {
        case 1:
            fc.setDialogTitle("Open");
            break;
        case 2:
            fc.setDialogTitle("Save As");
            break;
        }
        //
        // Config File Filter via different file types
        //
        switch (intFileType) {
        case 1:
            fc.addChoosableFileFilter(new IFileFilter("msa"));
            break;
        case 2:
            fc.addChoosableFileFilter(new IFileFilter("mbd"));
            break;
        case 3:
            fc.addChoosableFileFilter(new IFileFilter("msa"));
            break;
        case 4:
            fc.addChoosableFileFilter(new IFileFilter("mbd"));
            break;
        }

        int result = fc.showOpenDialog(new JPanel());
        //
        // Open relevanf file after click "OK"
        //
        if (result == JFileChooser.APPROVE_OPTION) {
            switch (intOperationType) {
            case 1:
                closeCurrentModule();
                switch (intFileType) {
                case 1:
                    openMsaFile(fc.getSelectedFile().getPath());
                    break;
                case 2:
                    openMbdFile(fc.getSelectedFile().getPath());
                    break;
                case 3:
                    openMlsaFile(fc.getSelectedFile().getPath());
                    break;
                case 4:
                    openMlbdFile(fc.getSelectedFile().getPath());
                    break;
                }
                break;
            case 2:
                switch (intFileType) {
                case 1:
                    this.saveFileName = fc.getSelectedFile().getPath();
                    break;
                case 2:
                    this.saveFileName = fc.getSelectedFile().getPath();
                    break;
                case 3:
                    this.saveFileName = fc.getSelectedFile().getPath();
                    break;
                case 4:
                    this.saveFileName = fc.getSelectedFile().getPath();
                    break;
                }
                break;
            }
        } else {
            if (intOperationType == 2) {
                this.saveFileName = "";
            }
        }
    }

    /**
     Open specificed Msa file and read its content
     
     @param strMsaFilePath The input data of Msa File Path
     
     **/
    private void openMsaFile(String strMsaFilePath) {
        Log.log("Open Msa", strMsaFilePath);
        try {
            File msaFile = new File(strMsaFilePath);
            xmlMsaDoc = (ModuleSurfaceAreaDocument) XmlObject.Factory.parse(msaFile);
            this.currentModule = strMsaFilePath;
            this.saveFileName = strMsaFilePath;
            this.currentModuleType = 1;
        } catch (IOException e) {
            Log.err("Open Msa " + strMsaFilePath, e.getMessage());
            return;
        } catch (XmlException e) {
            Log.err("Open Msa " + strMsaFilePath, e.getMessage());
            return;
        } catch (Exception e) {
            Log.err("Open Msa " + strMsaFilePath, "Invalid file type");
            return;
        }

        xmlmh = xmlMsaDoc.getModuleSurfaceArea().getMsaHeader();
        xmllcd = xmlMsaDoc.getModuleSurfaceArea().getLibraryClassDefinitions();
        xmlsf = xmlMsaDoc.getModuleSurfaceArea().getSourceFiles();
        xmlic = xmlMsaDoc.getModuleSurfaceArea().getIncludes();
        xmlpl = xmlMsaDoc.getModuleSurfaceArea().getProtocols();
        xmlen = xmlMsaDoc.getModuleSurfaceArea().getEvents();
        xmlhob = xmlMsaDoc.getModuleSurfaceArea().getHobs();
        xmlppi = xmlMsaDoc.getModuleSurfaceArea().getPPIs();
        xmlvb = xmlMsaDoc.getModuleSurfaceArea().getVariables();
        xmlbm = xmlMsaDoc.getModuleSurfaceArea().getBootModes();
        xmlst = xmlMsaDoc.getModuleSurfaceArea().getSystemTables();
        xmldh = xmlMsaDoc.getModuleSurfaceArea().getDataHubs();
        xmlfs = xmlMsaDoc.getModuleSurfaceArea().getFormsets();
        xmlgu = xmlMsaDoc.getModuleSurfaceArea().getGuids();
        xmlet = xmlMsaDoc.getModuleSurfaceArea().getExterns();
        xmlpcd = xmlMsaDoc.getModuleSurfaceArea().getPCDs();
        xmlbo = xmlMsaDoc.getModuleSurfaceArea().getBuildOptions();

        this.showMsaHeader(ModuleMain.VIEW);
        reloadTreeAndTable(ModuleMain.OPENED);
        jMenuEditAdd.setEnabled(true);
    }

    /**
     Open specificed Mbd file and read its content
     
     @param strMbdFilePath The input data of Mbd File Path
     
     **/
    private void openMbdFile(String strMbdFilePath) {
        Log.log("Open Mbd", strMbdFilePath);
        try {
            File mbdFile = new File(strMbdFilePath);
            xmlMbdDoc = (ModuleBuildDescriptionDocument) XmlObject.Factory.parse(mbdFile);
            this.currentModule = strMbdFilePath;
            this.saveFileName = strMbdFilePath;
            this.currentModuleType = 2;
        } catch (IOException e) {
            Log.err("Open Mbd " + strMbdFilePath, e.getMessage());
            return;
        } catch (XmlException e) {
            Log.err("Open Mbd " + strMbdFilePath, e.getMessage());
            return;
        } catch (Exception e) {
            Log.err("Open Mbd " + strMbdFilePath, "Invalid file type");
            return;
        }

        xmlmbdh = xmlMbdDoc.getModuleBuildDescription().getMbdHeader();
        xmllib = xmlMbdDoc.getModuleBuildDescription().getLibraries();
        xmlsf = xmlMbdDoc.getModuleBuildDescription().getSourceFiles();
        xmlic = xmlMbdDoc.getModuleBuildDescription().getIncludes();
        xmlpl = xmlMbdDoc.getModuleBuildDescription().getProtocols();
        xmlen = xmlMbdDoc.getModuleBuildDescription().getEvents();
        xmlhob = xmlMbdDoc.getModuleBuildDescription().getHobs();
        xmlppi = xmlMbdDoc.getModuleBuildDescription().getPPIs();
        xmlvb = xmlMbdDoc.getModuleBuildDescription().getVariables();
        xmlbm = xmlMbdDoc.getModuleBuildDescription().getBootModes();
        xmlst = xmlMbdDoc.getModuleBuildDescription().getSystemTables();
        xmldh = xmlMbdDoc.getModuleBuildDescription().getDataHubs();
        xmlfs = xmlMbdDoc.getModuleBuildDescription().getFormsets();
        xmlgu = xmlMbdDoc.getModuleBuildDescription().getGuids();
        xmlet = xmlMbdDoc.getModuleBuildDescription().getExterns();
        xmlpcd = xmlMbdDoc.getModuleBuildDescription().getPCDs();
        xmlbo = xmlMbdDoc.getModuleBuildDescription().getBuildOptions();

        this.showMbdHeader(ModuleMain.VIEW);
        reloadTreeAndTable(ModuleMain.OPENED);
        jMenuEditAdd.setEnabled(true);
    }

    /**
     Open specificed Mlsa file and read its content
     
     @param strMlsaFilePath The input data of Mlsa File Path
     
     **/
    private void openMlsaFile(String strMlsaFilePath) {
        Log.log("Open Mlsa", strMlsaFilePath);
        try {
            File mlsaFile = new File(strMlsaFilePath);
            xmlMlsaDoc = (LibraryModuleSurfaceAreaDocument) XmlObject.Factory.parse(mlsaFile);
            this.currentModule = strMlsaFilePath;
            this.saveFileName = strMlsaFilePath;
            this.currentModuleType = 3;
        } catch (IOException e) {
            Log.err("Open Mlsa " + strMlsaFilePath, e.getMessage());
            return;
        } catch (XmlException e) {
            Log.err("Open Mlsa " + strMlsaFilePath, e.getMessage());
            return;
        } catch (Exception e) {
            Log.err("Open Mlsa " + strMlsaFilePath, "Invalid file type");
            return;
        }

        xmlmlh = xmlMlsaDoc.getLibraryModuleSurfaceArea().getMsaLibHeader();
        xmllcd = xmlMlsaDoc.getLibraryModuleSurfaceArea().getLibraryClassDefinitions();
        xmlsf = xmlMlsaDoc.getLibraryModuleSurfaceArea().getSourceFiles();
        xmlic = xmlMlsaDoc.getLibraryModuleSurfaceArea().getIncludes();
        xmlpl = xmlMlsaDoc.getLibraryModuleSurfaceArea().getProtocols();
        xmlen = xmlMlsaDoc.getLibraryModuleSurfaceArea().getEvents();
        xmlhob = xmlMlsaDoc.getLibraryModuleSurfaceArea().getHobs();
        xmlppi = xmlMlsaDoc.getLibraryModuleSurfaceArea().getPPIs();
        xmlvb = xmlMlsaDoc.getLibraryModuleSurfaceArea().getVariables();
        xmlbm = xmlMlsaDoc.getLibraryModuleSurfaceArea().getBootModes();
        xmlst = xmlMlsaDoc.getLibraryModuleSurfaceArea().getSystemTables();
        xmldh = xmlMlsaDoc.getLibraryModuleSurfaceArea().getDataHubs();
        xmlfs = xmlMlsaDoc.getLibraryModuleSurfaceArea().getFormsets();
        xmlgu = xmlMlsaDoc.getLibraryModuleSurfaceArea().getGuids();
        xmlet = xmlMlsaDoc.getLibraryModuleSurfaceArea().getExterns();
        xmlpcd = xmlMlsaDoc.getLibraryModuleSurfaceArea().getPCDs();
        xmlbo = xmlMlsaDoc.getLibraryModuleSurfaceArea().getBuildOptions();

        this.showMlsaHeader(ModuleMain.VIEW);
        reloadTreeAndTable(ModuleMain.OPENED);
        jMenuEditAdd.setEnabled(true);
    }

    /**
     Open specificed Mlbd file and read its content
     
     @param strMlbdFilePath The input data of Mlbd File Path
     
     **/
    private void openMlbdFile(String strMlbdFilePath) {
        Log.log("Open Mlbd", strMlbdFilePath);
        try {
            File mlbdFile = new File(strMlbdFilePath);
            xmlMlbdDoc = (LibraryModuleBuildDescriptionDocument) XmlObject.Factory.parse(mlbdFile);
            this.currentModule = strMlbdFilePath;
            this.saveFileName = strMlbdFilePath;
            this.currentModuleType = 4;
        } catch (IOException e) {
            Log.err("Open Mlbd " + strMlbdFilePath, e.getMessage());
            return;
        } catch (XmlException e) {
            Log.err("Open Mlbd " + strMlbdFilePath, e.getMessage());
            return;
        } catch (Exception e) {
            Log.err("Open Mlbd " + strMlbdFilePath, "Invalid file type");
            return;
        }

        xmlmlbdh = xmlMlbdDoc.getLibraryModuleBuildDescription().getMbdLibHeader();
        xmllib = xmlMlbdDoc.getLibraryModuleBuildDescription().getLibraries();
        xmlsf = xmlMlbdDoc.getLibraryModuleBuildDescription().getSourceFiles();
        xmlic = xmlMlbdDoc.getLibraryModuleBuildDescription().getIncludes();
        xmlpl = xmlMlbdDoc.getLibraryModuleBuildDescription().getProtocols();
        xmlen = xmlMlbdDoc.getLibraryModuleBuildDescription().getEvents();
        xmlhob = xmlMlbdDoc.getLibraryModuleBuildDescription().getHobs();
        xmlppi = xmlMlbdDoc.getLibraryModuleBuildDescription().getPPIs();
        xmlvb = xmlMlbdDoc.getLibraryModuleBuildDescription().getVariables();
        xmlbm = xmlMlbdDoc.getLibraryModuleBuildDescription().getBootModes();
        xmlst = xmlMlbdDoc.getLibraryModuleBuildDescription().getSystemTables();
        xmldh = xmlMlbdDoc.getLibraryModuleBuildDescription().getDataHubs();
        xmlfs = xmlMlbdDoc.getLibraryModuleBuildDescription().getFormsets();
        xmlgu = xmlMlbdDoc.getLibraryModuleBuildDescription().getGuids();
        xmlet = xmlMlbdDoc.getLibraryModuleBuildDescription().getExterns();
        xmlpcd = xmlMlbdDoc.getLibraryModuleBuildDescription().getPCDs();
        xmlbo = xmlMlbdDoc.getLibraryModuleBuildDescription().getBuildOptions();

        this.showMlbdHeader(ModuleMain.VIEW);
        reloadTreeAndTable(ModuleMain.OPENED);
        jMenuEditAdd.setEnabled(true);
    }

    /**
     Create an empty tree if no file is open
     
     **/
    private void makeEmptyTree() {
        dmtnRoot = new IDefaultMutableTreeNode("No Msa/Mbd file opened", -1, -1);
        iTree = new ITree(dmtnRoot);
        jScrollPaneTree.setViewportView(iTree);
    }

    /**
     Create the tree to display all components of current open file.
     First to check if the component is null or not
     If not null, hangs it to the tree
     If null, skip it
     
     **/
    private void makeTree() {
        iTree.removeAll();

        //
        //Make an empty tree when closing
        //
        if (this.currentModuleType == 0) {
            makeEmptyTree();
            return;
        }

        //
        //Msa File
        //
        if (this.currentModuleType == 1) {
            //
            //Add MsaHeader Node
            //
            if (xmlmh != null) {
                dmtnRoot = new IDefaultMutableTreeNode(xmlmh.getBaseName().getStringValue(),
                                                       IDefaultMutableTreeNode.MSA_HEADER,
                                                       IDefaultMutableTreeNode.OPERATION_UPDATE_DELETE);
            } else {
                makeEmptyTree();
                return;
            }

            //
            //Add LibraryClassDefinitions Node
            //
            if (xmllcd != null && xmllcd.getLibraryClassList().size() > 0) {
                IDefaultMutableTreeNode libraryClassDefinitions = new IDefaultMutableTreeNode(
                                                                                              "Library Class Definitions",
                                                                                              IDefaultMutableTreeNode.LIBRARYCLASSDEFINITIONS,
                                                                                              IDefaultMutableTreeNode.OPERATION_UPDATE_DELETE);
                for (int indexI = 0; indexI < xmllcd.getLibraryClassList().size(); indexI++) {
                    libraryClassDefinitions
                                           .add(new IDefaultMutableTreeNode(
                                                                            xmllcd.getLibraryClassArray(indexI)
                                                                                  .getStringValue(),
                                                                            IDefaultMutableTreeNode.LIBRARY_CLASS_DEFINITION,
                                                                            IDefaultMutableTreeNode.OPERATION_NULL));
                }
                dmtnRoot.add(libraryClassDefinitions);
            }
        }

        //
        //Mbd File
        //
        if (this.currentModuleType == 2) {
            //
            //Add MsaHeader Node
            //
            if (xmlmbdh != null) {
                dmtnRoot = new IDefaultMutableTreeNode(xmlmbdh.getBaseName().getStringValue(),
                                                       IDefaultMutableTreeNode.MBD_HEADER,
                                                       IDefaultMutableTreeNode.OPERATION_UPDATE_DELETE);
            } else {
                makeEmptyTree();
                return;
            }

            //
            //Add Libraries Node
            //
            if (xmllib != null) {
                IDefaultMutableTreeNode libraries = new IDefaultMutableTreeNode(
                                                                                "Libraries",
                                                                                IDefaultMutableTreeNode.LIBRARIES,
                                                                                IDefaultMutableTreeNode.OPERATION_ADD_DELETE);
                if (xmllib.getArchList().size() > 0) {
                    IDefaultMutableTreeNode librariesArch = new IDefaultMutableTreeNode(
                                                                                        "Arch",
                                                                                        IDefaultMutableTreeNode.LIBRARIES_ARCH,
                                                                                        IDefaultMutableTreeNode.OPERATION_ADD_DELETE);
                    for (int indexI = 0; indexI < xmllib.getArchList().size(); indexI++) {
                        librariesArch.add(new IDefaultMutableTreeNode(xmllib.getArchArray(indexI).getArchType()
                                                                            .toString(),
                                                                      IDefaultMutableTreeNode.LIBRARIES_ARCH_ITEM,
                                                                      IDefaultMutableTreeNode.OPERATION_UPDATE_DELETE,
                                                                      indexI));
                    }
                    libraries.add(librariesArch);
                }
                if (xmllib.getLibraryList().size() > 0) {
                    IDefaultMutableTreeNode library = new IDefaultMutableTreeNode(
                                                                                  "Library",
                                                                                  IDefaultMutableTreeNode.LIBRARIES_LIBRARY,
                                                                                  IDefaultMutableTreeNode.OPERATION_ADD_UPDATE_DELETE);
                    for (int indexI = 0; indexI < xmllib.getLibraryList().size(); indexI++) {
                        library.add(new IDefaultMutableTreeNode(xmllib.getLibraryArray(indexI).getStringValue(),
                                                                IDefaultMutableTreeNode.LIBRARIES_LIBRARY_ITEM,
                                                                IDefaultMutableTreeNode.OPERATION_DELETE));
                    }
                    libraries.add(library);
                }
                dmtnRoot.add(libraries);
            }
        }

        //
        //MLsa File
        //
        if (this.currentModuleType == 3) {
            //
            //Add MsaHeader Node
            //
            if (xmlmlh != null) {
                dmtnRoot = new IDefaultMutableTreeNode(xmlmlh.getBaseName().getStringValue(),
                                                       IDefaultMutableTreeNode.MLSA_HEADER,
                                                       IDefaultMutableTreeNode.OPERATION_UPDATE_DELETE);
            } else {
                makeEmptyTree();
                return;
            }

            //
            //Add LibraryClassDefinitions Node
            //
            if (xmllcd != null && xmllcd.getLibraryClassList().size() > 0) {
                IDefaultMutableTreeNode libraryClassDefinitions = new IDefaultMutableTreeNode(
                                                                                              "Library Class Definitions",
                                                                                              IDefaultMutableTreeNode.LIBRARYCLASSDEFINITIONS,
                                                                                              IDefaultMutableTreeNode.OPERATION_UPDATE_DELETE);
                for (int indexI = 0; indexI < xmllcd.getLibraryClassList().size(); indexI++) {
                    libraryClassDefinitions
                                           .add(new IDefaultMutableTreeNode(
                                                                            xmllcd.getLibraryClassArray(indexI)
                                                                                  .getStringValue(),
                                                                            IDefaultMutableTreeNode.LIBRARY_CLASS_DEFINITION,
                                                                            IDefaultMutableTreeNode.OPERATION_NULL));
                }
                dmtnRoot.add(libraryClassDefinitions);
            }
        }

        //
        //Mlbd File
        //
        if (this.currentModuleType == 4) {
            //
            //Add MsaHeader Node
            //
            if (xmlmlbdh != null) {
                dmtnRoot = new IDefaultMutableTreeNode(xmlmlbdh.getBaseName().getStringValue(),
                                                       IDefaultMutableTreeNode.MLBD_HEADER,
                                                       IDefaultMutableTreeNode.OPERATION_UPDATE_DELETE);
            } else {
                makeEmptyTree();
                return;
            }

            //
            //Add Libraries Node
            //
            if (xmllib != null) {
                IDefaultMutableTreeNode libraries = new IDefaultMutableTreeNode(
                                                                                "Libraries",
                                                                                IDefaultMutableTreeNode.LIBRARIES,
                                                                                IDefaultMutableTreeNode.OPERATION_ADD_DELETE);
                if (xmllib.getArchList().size() > 0) {
                    IDefaultMutableTreeNode librariesArch = new IDefaultMutableTreeNode(
                                                                                        "Arch",
                                                                                        IDefaultMutableTreeNode.LIBRARIES_ARCH,
                                                                                        IDefaultMutableTreeNode.OPERATION_ADD_DELETE);
                    for (int indexI = 0; indexI < xmllib.getArchList().size(); indexI++) {
                        librariesArch.add(new IDefaultMutableTreeNode(xmllib.getArchArray(indexI).getArchType()
                                                                            .toString(),
                                                                      IDefaultMutableTreeNode.LIBRARIES_ARCH_ITEM,
                                                                      IDefaultMutableTreeNode.OPERATION_UPDATE_DELETE,
                                                                      indexI));
                    }
                    libraries.add(librariesArch);
                }
                if (xmllib.getLibraryList().size() > 0) {
                    IDefaultMutableTreeNode library = new IDefaultMutableTreeNode(
                                                                                  "Library",
                                                                                  IDefaultMutableTreeNode.LIBRARIES_LIBRARY,
                                                                                  IDefaultMutableTreeNode.OPERATION_ADD_UPDATE_DELETE);
                    for (int indexI = 0; indexI < xmllib.getLibraryList().size(); indexI++) {
                        library.add(new IDefaultMutableTreeNode(xmllib.getLibraryArray(indexI).getStringValue(),
                                                                IDefaultMutableTreeNode.LIBRARIES_LIBRARY_ITEM,
                                                                IDefaultMutableTreeNode.OPERATION_DELETE));
                    }
                    libraries.add(library);
                }
                dmtnRoot.add(libraries);
            }
        }

        //
        //Add SourceFiles Node
        //
        if (xmlsf != null) {
            IDefaultMutableTreeNode sourceFiles = new IDefaultMutableTreeNode(
                                                                              "Source Files",
                                                                              IDefaultMutableTreeNode.SOURCEFILES,
                                                                              IDefaultMutableTreeNode.OPERATION_ADD_DELETE);
            if (xmlsf.getArchList().size() > 0) {
                IDefaultMutableTreeNode sourceFilesArch = new IDefaultMutableTreeNode(
                                                                                      "Arch",
                                                                                      IDefaultMutableTreeNode.SOURCEFILES_ARCH,
                                                                                      IDefaultMutableTreeNode.OPERATION_ADD_DELETE);
                for (int indexI = 0; indexI < xmlsf.getArchList().size(); indexI++) {
                    sourceFilesArch
                                   .add(new IDefaultMutableTreeNode(
                                                                    xmlsf.getArchArray(indexI).getArchType().toString(),
                                                                    IDefaultMutableTreeNode.SOURCEFILES_ARCH_ITEM,
                                                                    IDefaultMutableTreeNode.OPERATION_UPDATE_DELETE,
                                                                    indexI));
                }
                sourceFiles.add(sourceFilesArch);
            }
            if (xmlsf.getFilenameList().size() > 0) {
                IDefaultMutableTreeNode sourceFilesFileName = new IDefaultMutableTreeNode(
                                                                                          "File Name",
                                                                                          IDefaultMutableTreeNode.SOURCEFILES_FILENAME,
                                                                                          IDefaultMutableTreeNode.OPERATION_ADD_UPDATE_DELETE);
                for (int indexI = 0; indexI < xmlsf.getFilenameList().size(); indexI++) {
                    sourceFilesFileName
                                       .add(new IDefaultMutableTreeNode(
                                                                        xmlsf.getFilenameArray(indexI).getStringValue(),
                                                                        IDefaultMutableTreeNode.SOURCEFILES_FILENAME_ITEM,
                                                                        IDefaultMutableTreeNode.OPERATION_DELETE));
                }
                sourceFiles.add(sourceFilesFileName);
            }
            dmtnRoot.add(sourceFiles);
        }

        //
        //Add includes
        //
        if (xmlic != null) {
            IDefaultMutableTreeNode includes = new IDefaultMutableTreeNode("Includes",
                                                                           IDefaultMutableTreeNode.INCLUDES,
                                                                           IDefaultMutableTreeNode.OPERATION_ADD_DELETE);
            if (xmlic.getArchList().size() > 0) {
                IDefaultMutableTreeNode includesArch = new IDefaultMutableTreeNode(
                                                                                   "Arch",
                                                                                   IDefaultMutableTreeNode.INCLUDES_ARCH,
                                                                                   IDefaultMutableTreeNode.OPERATION_ADD_DELETE);
                for (int indexI = 0; indexI < xmlic.getArchList().size(); indexI++) {
                    includesArch.add(new IDefaultMutableTreeNode(xmlic.getArchArray(indexI).getArchType().toString(),
                                                                 IDefaultMutableTreeNode.INCLUDES_ARCH_ITEM,
                                                                 IDefaultMutableTreeNode.OPERATION_UPDATE_DELETE,
                                                                 indexI));
                }
                includes.add(includesArch);
            }
            if (xmlic.getPackageNameList().size() > 0) {
                IDefaultMutableTreeNode includesPackageName = new IDefaultMutableTreeNode(
                                                                                          "Package Name",
                                                                                          IDefaultMutableTreeNode.INCLUDES_PACKAGENAME,
                                                                                          IDefaultMutableTreeNode.OPERATION_ADD_UPDATE_DELETE);
                for (int indexI = 0; indexI < xmlic.getPackageNameList().size(); indexI++) {
                    includesPackageName
                                       .add(new IDefaultMutableTreeNode(
                                                                        xmlic.getPackageNameArray(indexI)
                                                                             .getStringValue(),
                                                                        IDefaultMutableTreeNode.INCLUDES_PACKAGENAME_ITEM,
                                                                        IDefaultMutableTreeNode.OPERATION_DELETE));
                }
                includes.add(includesPackageName);
            }
            dmtnRoot.add(includes);
        }

        //
        //Add protocols
        //
        if (xmlpl != null) {
            IDefaultMutableTreeNode dmtnProtocols = new IDefaultMutableTreeNode(
                                                                                "Protocols",
                                                                                IDefaultMutableTreeNode.PROTOCOLS,
                                                                                IDefaultMutableTreeNode.OPERATION_ADD_DELETE);
            IDefaultMutableTreeNode dmtnProtocol = new IDefaultMutableTreeNode(
                                                                               "Protocol",
                                                                               IDefaultMutableTreeNode.PROTOCOLS_PROTOCOL,
                                                                               IDefaultMutableTreeNode.OPERATION_ADD_DELETE);
            IDefaultMutableTreeNode dmtnProtocolNotify = new IDefaultMutableTreeNode(
                                                                                     "Protocol Notify",
                                                                                     IDefaultMutableTreeNode.PROTOCOLS_PROTOCOLNOTIFY,
                                                                                     IDefaultMutableTreeNode.OPERATION_ADD_DELETE);
            if (xmlpl.getProtocolList().size() > 0) {
                for (int indexI = 0; indexI < xmlpl.getProtocolList().size(); indexI++) {
                    dmtnProtocol.add(new IDefaultMutableTreeNode(xmlpl.getProtocolArray(indexI).getStringValue(),
                                                                 IDefaultMutableTreeNode.PROTOCOLS_PROTOCOL_ITEM,
                                                                 IDefaultMutableTreeNode.OPERATION_UPDATE_DELETE,
                                                                 indexI));
                }
                dmtnProtocols.add(dmtnProtocol);
            }
            if (xmlpl.getProtocolNotifyList().size() > 0) {
                for (int indexI = 0; indexI < xmlpl.getProtocolNotifyList().size(); indexI++) {
                    dmtnProtocolNotify
                                      .add(new IDefaultMutableTreeNode(
                                                                       xmlpl.getProtocolNotifyArray(indexI)
                                                                            .getStringValue(),
                                                                       IDefaultMutableTreeNode.PROTOCOLS_PROTOCOLNOTIFY_ITEM,
                                                                       IDefaultMutableTreeNode.OPERATION_UPDATE_DELETE,
                                                                       indexI));
                }
                dmtnProtocols.add(dmtnProtocolNotify);
            }
            dmtnRoot.add(dmtnProtocols);
        }

        //
        //Add events
        //
        if (xmlen != null) {
            IDefaultMutableTreeNode dmtnEvents = new IDefaultMutableTreeNode(
                                                                             "Events",
                                                                             IDefaultMutableTreeNode.EVENTS,
                                                                             IDefaultMutableTreeNode.OPERATION_ADD_DELETE);
            IDefaultMutableTreeNode dmtnCreateEvents = new IDefaultMutableTreeNode(
                                                                                   "Create",
                                                                                   IDefaultMutableTreeNode.EVENTS_CREATEEVENTS,
                                                                                   IDefaultMutableTreeNode.OPERATION_ADD_DELETE);
            IDefaultMutableTreeNode dmtnSignalEvents = new IDefaultMutableTreeNode(
                                                                                   "Signal",
                                                                                   IDefaultMutableTreeNode.EVENTS_SIGNALEVENTS,
                                                                                   IDefaultMutableTreeNode.OPERATION_ADD_DELETE);
            if (xmlen.getCreateEvents() != null && xmlen.getCreateEvents().getEventList().size() > 0) {
                for (int indexI = 0; indexI < xmlen.getCreateEvents().getEventList().size(); indexI++) {
                    dmtnCreateEvents.add(new IDefaultMutableTreeNode(xmlen.getCreateEvents().getEventArray(indexI)
                                                                          .getCName(),
                                                                     IDefaultMutableTreeNode.EVENTS_CREATEEVENTS_ITEM,
                                                                     IDefaultMutableTreeNode.OPERATION_UPDATE_DELETE,
                                                                     indexI));
                }
                dmtnEvents.add(dmtnCreateEvents);
            }
            if (xmlen.getSignalEvents() != null && xmlen.getSignalEvents().getEventList().size() > 0) {
                for (int indexI = 0; indexI < xmlen.getSignalEvents().getEventList().size(); indexI++) {
                    dmtnSignalEvents.add(new IDefaultMutableTreeNode(xmlen.getSignalEvents().getEventArray(indexI)
                                                                          .getCName(),
                                                                     IDefaultMutableTreeNode.EVENTS_SIGNALEVENTS_ITEM,
                                                                     IDefaultMutableTreeNode.OPERATION_UPDATE_DELETE,
                                                                     indexI));
                }
                dmtnEvents.add(dmtnSignalEvents);
            }
            dmtnRoot.add(dmtnEvents);
        }

        //
        //Add hobs
        //
        if (xmlhob != null) {
            IDefaultMutableTreeNode dmtnHobs = new IDefaultMutableTreeNode("Hobs", IDefaultMutableTreeNode.HOBS,
                                                                           IDefaultMutableTreeNode.OPERATION_ADD_DELETE);
            if (xmlhob.getHobList().size() > 0) {
                for (int indexI = 0; indexI < xmlhob.getHobList().size(); indexI++) {
                    dmtnHobs.add(new IDefaultMutableTreeNode(xmlhob.getHobArray(indexI).getName(),
                                                             IDefaultMutableTreeNode.HOBS_HOB_ITEM,
                                                             IDefaultMutableTreeNode.OPERATION_UPDATE_DELETE, indexI));
                }
            }
            dmtnRoot.add(dmtnHobs);
        }

        //
        //Add ppis
        //
        if (xmlppi != null) {
            IDefaultMutableTreeNode dmtnPpis = new IDefaultMutableTreeNode("Ppis", IDefaultMutableTreeNode.PPIS,
                                                                           IDefaultMutableTreeNode.OPERATION_ADD_DELETE);
            IDefaultMutableTreeNode dmtnPpi = new IDefaultMutableTreeNode("Ppi", IDefaultMutableTreeNode.PPIS_PPI,
                                                                          IDefaultMutableTreeNode.OPERATION_ADD_DELETE);
            IDefaultMutableTreeNode dmtnPpiNotify = new IDefaultMutableTreeNode(
                                                                                "Ppi Notify",
                                                                                IDefaultMutableTreeNode.PPIS_PPINOTIFY,
                                                                                IDefaultMutableTreeNode.OPERATION_ADD_DELETE);
            if (xmlppi.getPpiList().size() > 0) {
                for (int indexI = 0; indexI < xmlppi.getPpiList().size(); indexI++) {
                    dmtnPpi.add(new IDefaultMutableTreeNode(xmlppi.getPpiArray(indexI).getStringValue(),
                                                            IDefaultMutableTreeNode.PPIS_PPI_ITEM,
                                                            IDefaultMutableTreeNode.OPERATION_UPDATE_DELETE, indexI));
                }
                dmtnPpis.add(dmtnPpi);
            }
            if (xmlppi.getPpiNotifyList().size() > 0) {
                for (int indexI = 0; indexI < xmlppi.getPpiNotifyList().size(); indexI++) {
                    dmtnPpiNotify.add(new IDefaultMutableTreeNode(xmlppi.getPpiNotifyArray(indexI).getStringValue(),
                                                                  IDefaultMutableTreeNode.PPIS_PPINOTIFY_ITEM,
                                                                  IDefaultMutableTreeNode.OPERATION_UPDATE_DELETE,
                                                                  indexI));
                }
                dmtnPpis.add(dmtnPpiNotify);
            }
            dmtnRoot.add(dmtnPpis);
        }

        //
        //Add variables
        //
        if (xmlvb != null) {
            IDefaultMutableTreeNode dmtnVariables = new IDefaultMutableTreeNode(
                                                                                "Variables",
                                                                                IDefaultMutableTreeNode.VARIABLES,
                                                                                IDefaultMutableTreeNode.OPERATION_ADD_DELETE);
            if (xmlvb.getVariableList().size() > 0) {
                for (int indexI = 0; indexI < xmlvb.getVariableList().size(); indexI++) {
                    dmtnVariables.add(new IDefaultMutableTreeNode(xmlvb.getVariableArray(indexI).getString(),
                                                                  IDefaultMutableTreeNode.VARIABLES_VARIABLE_ITEM,
                                                                  IDefaultMutableTreeNode.OPERATION_UPDATE_DELETE,
                                                                  indexI));
                }
            }
            dmtnRoot.add(dmtnVariables);
        }

        //
        //Add bootmodes
        //
        if (xmlbm != null) {
            IDefaultMutableTreeNode dmtnBootModes = new IDefaultMutableTreeNode(
                                                                                "BootModes",
                                                                                IDefaultMutableTreeNode.BOOTMODES,
                                                                                IDefaultMutableTreeNode.OPERATION_ADD_DELETE);
            if (xmlbm.getBootModeList().size() > 0) {
                for (int indexI = 0; indexI < xmlbm.getBootModeList().size(); indexI++) {
                    dmtnBootModes.add(new IDefaultMutableTreeNode(xmlbm.getBootModeArray(indexI).getBootModeName()
                                                                       .toString(),
                                                                  IDefaultMutableTreeNode.BOOTMODES_BOOTMODE_ITEM,
                                                                  IDefaultMutableTreeNode.OPERATION_UPDATE_DELETE,
                                                                  indexI));
                }
            }
            dmtnRoot.add(dmtnBootModes);
        }

        //
        //Add systemtables
        //
        if (xmlst != null) {
            IDefaultMutableTreeNode dmtnSystemTables = new IDefaultMutableTreeNode(
                                                                                   "SystemTables",
                                                                                   IDefaultMutableTreeNode.SYSTEMTABLES,
                                                                                   IDefaultMutableTreeNode.OPERATION_ADD_DELETE);
            if (xmlst.getSystemTableList().size() > 0) {
                for (int indexI = 0; indexI < xmlst.getSystemTableList().size(); indexI++) {
                    dmtnSystemTables
                                    .add(new IDefaultMutableTreeNode(
                                                                     xmlst.getSystemTableArray(indexI).getEntryList().get(0),
                                                                     IDefaultMutableTreeNode.SYSTEMTABLES_SYSTEMTABLE_ITEM,
                                                                     IDefaultMutableTreeNode.OPERATION_UPDATE_DELETE,
                                                                     indexI));
                }
            }
            dmtnRoot.add(dmtnSystemTables);
        }

        //
        //Add datahubs
        //
        if (xmldh != null) {
            IDefaultMutableTreeNode dmtnDataHubs = new IDefaultMutableTreeNode(
                                                                               "DataHubs",
                                                                               IDefaultMutableTreeNode.DATAHUBS,
                                                                               IDefaultMutableTreeNode.OPERATION_ADD_DELETE);
            if (xmldh.getDataHubRecordList().size() > 0) {
                for (int indexI = 0; indexI < xmldh.getDataHubRecordList().size(); indexI++) {
                    dmtnDataHubs.add(new IDefaultMutableTreeNode(xmldh.getDataHubRecordArray(indexI).getStringValue(),
                                                                 IDefaultMutableTreeNode.DATAHUBS_DATAHUB_ITEM,
                                                                 IDefaultMutableTreeNode.OPERATION_UPDATE_DELETE,
                                                                 indexI));
                }
            }
            dmtnRoot.add(dmtnDataHubs);
        }

        //
        //Add formsets
        //
        if (xmlfs != null) {
            IDefaultMutableTreeNode dmtnFormsets = new IDefaultMutableTreeNode(
                                                                               "Formsets",
                                                                               IDefaultMutableTreeNode.FORMSETS,
                                                                               IDefaultMutableTreeNode.OPERATION_ADD_DELETE);
            if (xmlfs.getFormsetList().size() > 0) {
                for (int indexI = 0; indexI < xmlfs.getFormsetList().size(); indexI++) {
                    dmtnFormsets.add(new IDefaultMutableTreeNode(xmlfs.getFormsetArray(indexI).getStringValue(),
                                                                 IDefaultMutableTreeNode.FORMSETS_FORMSET_ITEM,
                                                                 IDefaultMutableTreeNode.OPERATION_UPDATE_DELETE,
                                                                 indexI));
                }
            }
            dmtnRoot.add(dmtnFormsets);
        }

        //
        //Add guids
        //
        if (xmlgu != null) {
            IDefaultMutableTreeNode dmtnGuids = new IDefaultMutableTreeNode(
                                                                            "Guids",
                                                                            IDefaultMutableTreeNode.GUIDS,
                                                                            IDefaultMutableTreeNode.OPERATION_ADD_DELETE);
            if (xmlgu.getGuidEntryList().size() > 0) {
                for (int indexI = 0; indexI < xmlgu.getGuidEntryList().size(); indexI++) {
                    dmtnGuids.add(new IDefaultMutableTreeNode(xmlgu.getGuidEntryArray(indexI).getCName(),
                                                              IDefaultMutableTreeNode.GUIDS_GUIDENTRY_ITEM,
                                                              IDefaultMutableTreeNode.OPERATION_UPDATE_DELETE, indexI));
                }
            }
            dmtnRoot.add(dmtnGuids);
        }

        //
        //Add externs
        //
        if (xmlet != null) {
            IDefaultMutableTreeNode dmtnExterns = new IDefaultMutableTreeNode(
                                                                              "Externs",
                                                                              IDefaultMutableTreeNode.EXTERNS,
                                                                              IDefaultMutableTreeNode.OPERATION_ADD_DELETE);
            if (xmlet.getExternList().size() > 0) {
                for (int indexI = 0; indexI < xmlet.getExternList().size(); indexI++) {
                    dmtnExterns
                               .add(new IDefaultMutableTreeNode("Extern " + Integer.valueOf(indexI + 1),
                                                                IDefaultMutableTreeNode.EXTERNS_EXTERN_ITEM,
                                                                IDefaultMutableTreeNode.OPERATION_UPDATE_DELETE, indexI));
                }
            }
            dmtnRoot.add(dmtnExterns);
        }

        //
        //Add pcds
        //
        if (xmlpcd != null) {
            IDefaultMutableTreeNode dmtnPCDs = new IDefaultMutableTreeNode("PCDs", IDefaultMutableTreeNode.PCDS,
                                                                           IDefaultMutableTreeNode.OPERATION_ADD_DELETE);
            if (xmlpcd.getPcdDataList().size() > 0) {
                for (int indexI = 0; indexI < xmlpcd.getPcdDataList().size(); indexI++) {
                    dmtnPCDs.add(new IDefaultMutableTreeNode(xmlpcd.getPcdDataArray(indexI).getCName(),
                                                             IDefaultMutableTreeNode.PCDS_PCDDATA_ITEM,
                                                             IDefaultMutableTreeNode.OPERATION_UPDATE_DELETE, indexI));
                }
            }
            dmtnRoot.add(dmtnPCDs);
        }

        iTree = new ITree(dmtnRoot);
        iTree.addMouseListener(this);
        iTree.addTreeSelectionListener(this);
        jScrollPaneTree.setViewportView(iTree);
    }

    /* (non-Javadoc)
     * @see java.awt.event.WindowListener#windowClosing(java.awt.event.WindowEvent)
     *
     * Override windowClosing to popup warning message to confirm quit
     * 
     */
    public void windowClosing(WindowEvent arg0) {
        this.onExit();
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
            jPopupMenu.show(arg0.getComponent(), arg0.getX(), arg0.getY());
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
     Init popup menu
     
     **/
    public void valueChanged(TreeSelectionEvent arg0) {
        int intOperation = iTree.getSelectOperation();
        if (intOperation == IDefaultMutableTreeNode.OPERATION_NULL) {
            setMenuItemAddEnabled(false);
            setMenuItemUpdateEnabled(false);
            setMenuItemDeleteEnabled(false);
        }
        if (intOperation == IDefaultMutableTreeNode.OPERATION_ADD) {
            setMenuItemAddEnabled(true);
            setMenuItemUpdateEnabled(false);
            setMenuItemDeleteEnabled(false);
        }
        if (intOperation == IDefaultMutableTreeNode.OPERATION_UPDATE) {
            setMenuItemAddEnabled(false);
            setMenuItemUpdateEnabled(true);
            setMenuItemDeleteEnabled(false);
        }
        if (intOperation == IDefaultMutableTreeNode.OPERATION_DELETE) {
            setMenuItemAddEnabled(false);
            setMenuItemUpdateEnabled(false);
            setMenuItemDeleteEnabled(true);
        }
        if (intOperation == IDefaultMutableTreeNode.OPERATION_ADD_UPDATE) {
            setMenuItemAddEnabled(true);
            setMenuItemUpdateEnabled(true);
            setMenuItemDeleteEnabled(false);
        }
        if (intOperation == IDefaultMutableTreeNode.OPERATION_ADD_DELETE) {
            setMenuItemAddEnabled(true);
            setMenuItemUpdateEnabled(false);
            setMenuItemDeleteEnabled(true);
        }
        if (intOperation == IDefaultMutableTreeNode.OPERATION_UPDATE_DELETE) {
            setMenuItemAddEnabled(false);
            setMenuItemUpdateEnabled(true);
            setMenuItemDeleteEnabled(true);
        }
        if (intOperation == IDefaultMutableTreeNode.OPERATION_ADD_UPDATE_DELETE) {
            setMenuItemAddEnabled(true);
            setMenuItemUpdateEnabled(true);
            setMenuItemDeleteEnabled(true);
        }
        viewCurrentModule();
    }

    /**
     Enable/Disable add menu item
     
     **/
    private void setMenuItemAddEnabled(boolean isEnable) {
        jMenuItemPopupAdd.setEnabled(isEnable);
    }

    /**
     Enable/Disable update menu item
     
     **/
    private void setMenuItemUpdateEnabled(boolean isEnable) {
        jMenuItemPopupUpdate.setEnabled(isEnable);
        jMenuItemEditUpdate.setEnabled(isEnable);
    }

    /**
     Enable/Disable delete menu item
     
     **/
    private void setMenuItemDeleteEnabled(boolean isEnable) {
        jMenuItemPopupDelete.setEnabled(isEnable);
        jMenuItemEditDelete.setEnabled(isEnable);
    }

    /**
     Close current open module

     **/
    private void closeCurrentModule() {
        cleanAllXml();

        reloadTreeAndTable(ModuleMain.CLOSED);
        setMenuItemAddEnabled(false);
        setMenuItemUpdateEnabled(false);
        setMenuItemDeleteEnabled(false);
        cleanDesktopPane();
    }

    /**
     Remove all Internal Frame of Desktop Pane
     
     **/
    private void cleanDesktopPane() {
        JInternalFrame[] iif = this.jDesktopPane.getAllFrames();
        for (int index = 0; index < iif.length; index++) {
            iif[index].dispose();
        }
    }

    /**
     Set all xml document null
     
     **/
    private void cleanAllXml() {
        this.currentModule = "";
        this.saveFileName = "";
        this.currentModuleType = 0;
        this.currentNodeType = 0;

        xmlMsaDoc = null;
        xmlMbdDoc = null;
        xmlMlsaDoc = null;
        xmlMlbdDoc = null;
        xmlmh = null;
        xmllcd = null;
        xmllib = null;
        xmlsf = null;
        xmlic = null;
        xmlpl = null;
        xmlen = null;
        xmlhob = null;
        xmlppi = null;
        xmlvb = null;
        xmlbm = null;
        xmlst = null;
        xmldh = null;
        xmlfs = null;
        xmlgu = null;
        xmlet = null;
        xmlpcd = null;
        xmlbo = null;
    }

    /**
     Execute add operation for current node 
     
     @param intCategory The category of current node
     @param intLocation The location of current node
     
     **/
    private void addCurrentModule(int intCategory, int intLocation) {
        //
        //Add new libraries
        //
        if (intCategory == IDefaultMutableTreeNode.LIBRARIES
            || intCategory == IDefaultMutableTreeNode.LIBRARIES_LIBRARY
            || intCategory == IDefaultMutableTreeNode.LIBRARIES_ARCH) {
            showLibraries(ModuleMain.ADD, intCategory, -1);
        }

        //
        //Add new sourcefiles
        //
        if (intCategory == IDefaultMutableTreeNode.SOURCEFILES
            || intCategory == IDefaultMutableTreeNode.SOURCEFILES_FILENAME
            || intCategory == IDefaultMutableTreeNode.SOURCEFILES_ARCH) {
            showSourceFiles(ModuleMain.ADD, intCategory, -1);
        }

        //
        //Add new includes
        //
        if (intCategory == IDefaultMutableTreeNode.INCLUDES
            || intCategory == IDefaultMutableTreeNode.INCLUDES_PACKAGENAME
            || intCategory == IDefaultMutableTreeNode.INCLUDES_ARCH) {
            showIncludes(ModuleMain.ADD, intCategory, -1);
        }

        //
        //Add new protocols
        //
        if (intCategory == IDefaultMutableTreeNode.PROTOCOLS
            || intCategory == IDefaultMutableTreeNode.PROTOCOLS_PROTOCOL
            || intCategory == IDefaultMutableTreeNode.PROTOCOLS_PROTOCOLNOTIFY) {
            showProtocols(ModuleMain.ADD, intCategory, -1);
        }

        //
        //Add new events
        //
        if (intCategory == IDefaultMutableTreeNode.EVENTS || intCategory == IDefaultMutableTreeNode.EVENTS_CREATEEVENTS
            || intCategory == IDefaultMutableTreeNode.EVENTS_SIGNALEVENTS) {
            showEvents(ModuleMain.ADD, intCategory, -1);
        }

        //
        //Add new hobs
        //
        if (intCategory == IDefaultMutableTreeNode.HOBS || intCategory == IDefaultMutableTreeNode.HOBS_HOB_ITEM) {
            showHobs(ModuleMain.ADD, intCategory, -1);
        }

        //
        //Add new ppis
        //
        if (intCategory == IDefaultMutableTreeNode.PPIS || intCategory == IDefaultMutableTreeNode.PPIS_PPI
            || intCategory == IDefaultMutableTreeNode.PPIS_PPINOTIFY) {
            showPpis(ModuleMain.ADD, intCategory, -1);
        }

        //
        //Add new variables
        //
        if (intCategory == IDefaultMutableTreeNode.VARIABLES
            || intCategory == IDefaultMutableTreeNode.VARIABLES_VARIABLE_ITEM) {
            showVariables(ModuleMain.ADD, intCategory, -1);
        }

        //
        //Add new BootModes
        //
        if (intCategory == IDefaultMutableTreeNode.BOOTMODES
            || intCategory == IDefaultMutableTreeNode.BOOTMODES_BOOTMODE_ITEM) {
            showBootModes(ModuleMain.ADD, intCategory, -1);
        }

        //
        //Add new SystemTables
        //
        if (intCategory == IDefaultMutableTreeNode.SYSTEMTABLES
            || intCategory == IDefaultMutableTreeNode.SYSTEMTABLES_SYSTEMTABLE_ITEM) {
            showSystemTables(ModuleMain.ADD, intCategory, -1);
        }

        //
        //Add new DataHubs
        //
        if (intCategory == IDefaultMutableTreeNode.DATAHUBS
            || intCategory == IDefaultMutableTreeNode.DATAHUBS_DATAHUB_ITEM) {
            showDataHubs(ModuleMain.ADD, intCategory, -1);
        }

        //
        //Add new Formsets
        //
        if (intCategory == IDefaultMutableTreeNode.FORMSETS
            || intCategory == IDefaultMutableTreeNode.FORMSETS_FORMSET_ITEM) {
            showFormsets(ModuleMain.ADD, intCategory, -1);
        }

        //
        //Add new Guids
        //
        if (intCategory == IDefaultMutableTreeNode.GUIDS || intCategory == IDefaultMutableTreeNode.GUIDS_GUIDENTRY_ITEM) {
            showGuids(ModuleMain.ADD, intCategory, -1);
        }

        //
        //Add new Externs
        //
        if (intCategory == IDefaultMutableTreeNode.EXTERNS
            || intCategory == IDefaultMutableTreeNode.EXTERNS_EXTERN_ITEM) {
            showExterns(ModuleMain.ADD, intCategory, -1);
        }

        //
        //Add new PCDs
        //
        if (intCategory == IDefaultMutableTreeNode.PCDS || intCategory == IDefaultMutableTreeNode.PCDS_PCDDATA_ITEM) {
            showPCDs(ModuleMain.ADD, intCategory, -1);
        }
    }

    /**
     Execute delete operation of current node
     
     @param intCategory The category of current node
     @param intLocation The location of current node
     
     **/
    private void deleteCurrentModule(int intCategory, int intLocation) {
        //
        // Delete Msa Header
        //
        if (intCategory == IDefaultMutableTreeNode.MSA_HEADER || intCategory == IDefaultMutableTreeNode.MBD_HEADER
            || intCategory == IDefaultMutableTreeNode.MLSA_HEADER || intCategory == IDefaultMutableTreeNode.MLBD_HEADER) {
            if (JOptionPane.showConfirmDialog(null, "The module will be deleted permanently, do you want to continue?") == JOptionPane.YES_OPTION) {
                try {
                    File f = new File(currentModule);
                    f.delete();
                    closeCurrentModule();
                } catch (Exception e) {
                    Log.err("Delete " + currentModule, e.getMessage());
                }
            } else {
                return;
            }
        }

        //
        //Delete LIBRARY CLASS DEFINITIONS
        //
        if (intCategory == IDefaultMutableTreeNode.LIBRARYCLASSDEFINITIONS) {
            xmllcd = null;
        }

        //
        //Delete Libraries
        //
        if (intCategory == IDefaultMutableTreeNode.LIBRARIES) {
            xmllib = null;
        }
        if (intCategory == IDefaultMutableTreeNode.LIBRARIES_LIBRARY) {
            for (int indexI = xmllib.getLibraryList().size() - 1; indexI > -1; indexI--) {
                xmllib.removeLibrary(indexI);
            }
            if (xmllib.getArchList().size() < 1 && xmllib.getLibraryList().size() < 1) {
                xmllib = null;
            }
        }
        if (intCategory == IDefaultMutableTreeNode.LIBRARIES_ARCH) {
            for (int indexI = xmllib.getArchList().size() - 1; indexI > -1; indexI--) {
                xmllib.removeArch(indexI);
            }
            if (xmllib.getArchList().size() < 1 && xmllib.getLibraryList().size() < 1) {
                xmllib = null;
            }
        }
        if (intCategory == IDefaultMutableTreeNode.LIBRARIES_ARCH_ITEM) {
            xmllib.removeArch(intLocation);
        }
        if (intCategory == IDefaultMutableTreeNode.LIBRARIES_LIBRARY_ITEM) {
            xmllib.removeLibrary(intLocation);
        }

        //
        //Delete SourceFiles
        //
        if (intCategory == IDefaultMutableTreeNode.SOURCEFILES) {
            xmlsf = null;
        }
        if (intCategory == IDefaultMutableTreeNode.SOURCEFILES_FILENAME) {
            for (int indexI = xmlsf.getFilenameList().size() - 1; indexI > -1; indexI--) {
                xmlsf.removeFilename(indexI);
            }
            if (xmlsf.getArchList().size() < 1 && xmlsf.getFilenameList().size() < 1) {
                xmlsf = null;
            }
        }
        if (intCategory == IDefaultMutableTreeNode.SOURCEFILES_ARCH) {
            for (int indexI = xmlsf.getArchList().size() - 1; indexI > -1; indexI--) {
                xmlsf.removeArch(indexI);
            }
            if (xmlsf.getArchList().size() < 1 && xmlsf.getFilenameList().size() < 1) {
                xmlsf = null;
            }
        }
        if (intCategory == IDefaultMutableTreeNode.SOURCEFILES_ARCH_ITEM) {
            xmlsf.removeArch(intLocation);
        }
        if (intCategory == IDefaultMutableTreeNode.SOURCEFILES_FILENAME_ITEM) {
            xmlsf.removeFilename(intLocation);
        }

        //
        //Delete Includes
        //
        if (intCategory == IDefaultMutableTreeNode.INCLUDES) {
            xmlic = null;
        }
        if (intCategory == IDefaultMutableTreeNode.INCLUDES_PACKAGENAME) {
            for (int indexI = xmlic.getPackageNameList().size() - 1; indexI > -1; indexI--) {
                xmlic.removePackageName(indexI);
            }
            if (xmlic.getArchList().size() < 1 && xmlic.getPackageNameList().size() < 1) {
                xmlic = null;
            }
        }
        if (intCategory == IDefaultMutableTreeNode.INCLUDES_ARCH) {
            for (int indexI = xmlic.getArchList().size() - 1; indexI > -1; indexI--) {
                xmlic.removeArch(indexI);
            }
            if (xmlic.getArchList().size() < 1 && xmlic.getPackageNameList().size() < 1) {
                xmlic = null;
            }
        }
        if (intCategory == IDefaultMutableTreeNode.INCLUDES_ARCH_ITEM) {
            xmlic.removeArch(intLocation);
        }
        if (intCategory == IDefaultMutableTreeNode.INCLUDES_PACKAGENAME_ITEM) {
            xmlic.removePackageName(intLocation);
        }

        //
        //Delete Protocols
        //
        if (intCategory == IDefaultMutableTreeNode.PROTOCOLS) {
            xmlpl = null;
        }
        if (intCategory == IDefaultMutableTreeNode.PROTOCOLS_PROTOCOL) {
            for (int indexI = xmlpl.getProtocolList().size() - 1; indexI > -1; indexI--) {
                xmlpl.removeProtocol(indexI);
            }
            if (xmlpl.getProtocolList().size() < 1 && xmlpl.getProtocolNotifyList().size() < 1) {
                xmlpl = null;
            }
        }
        if (intCategory == IDefaultMutableTreeNode.PROTOCOLS_PROTOCOLNOTIFY) {
            for (int indexI = xmlpl.getProtocolList().size() - 1; indexI > -1; indexI--) {
                xmlpl.removeProtocolNotify(indexI);
            }
            if (xmlpl.getProtocolList().size() < 1 && xmlpl.getProtocolNotifyList().size() < 1) {
                xmlpl = null;
            }
        }
        if (intCategory == IDefaultMutableTreeNode.PROTOCOLS_PROTOCOL_ITEM) {
            xmlpl.removeProtocol(intLocation);
        }
        if (intCategory == IDefaultMutableTreeNode.PROTOCOLS_PROTOCOLNOTIFY_ITEM) {
            xmlpl.removeProtocolNotify(intLocation);
        }

        //
        //Delete Events
        //
        if (intCategory == IDefaultMutableTreeNode.EVENTS) {
            xmlen = null;
        }
        if (intCategory == IDefaultMutableTreeNode.EVENTS_CREATEEVENTS) {
            for (int indexI = xmlen.getCreateEvents().getEventList().size() - 1; indexI > -1; indexI--) {
                xmlen.getCreateEvents().removeEvent(indexI);
            }
            if (xmlen.getCreateEvents().getEventList().size() < 1 && xmlen.getSignalEvents().getEventList().size() < 1) {
                xmlen = null;
            }
        }
        if (intCategory == IDefaultMutableTreeNode.EVENTS_SIGNALEVENTS) {
            for (int indexI = xmlen.getSignalEvents().getEventList().size() - 1; indexI > -1; indexI--) {
                xmlen.getSignalEvents().removeEvent(indexI);
            }
            if (xmlen.getCreateEvents().getEventList().size() < 1 && xmlen.getSignalEvents().getEventList().size() < 1) {
                xmlen = null;
            }
        }
        if (intCategory == IDefaultMutableTreeNode.EVENTS_CREATEEVENTS_ITEM) {
            xmlen.getCreateEvents().removeEvent(intLocation);
        }
        if (intCategory == IDefaultMutableTreeNode.EVENTS_SIGNALEVENTS_ITEM) {
            xmlen.getSignalEvents().removeEvent(intLocation);
        }

        //
        //Delete Hobs
        //
        if (intCategory == IDefaultMutableTreeNode.HOBS) {
            xmlhob = null;
        }
        if (intCategory == IDefaultMutableTreeNode.HOBS_HOB_ITEM) {
            xmlhob.removeHob(intLocation);
            if (xmlhob.getHobList().size() < 1) {
                xmlhob = null;
            }
        }

        //
        //Delete Ppis
        //
        if (intCategory == IDefaultMutableTreeNode.PPIS) {
            xmlppi = null;
        }
        if (intCategory == IDefaultMutableTreeNode.PPIS_PPI) {
            for (int indexI = xmlppi.getPpiList().size() - 1; indexI > -1; indexI--) {
                xmlppi.removePpi(indexI);
            }
            if (xmlppi.getPpiList().size() < 1 && xmlppi.getPpiNotifyList().size() < 1) {
                xmlppi = null;
            }
        }
        if (intCategory == IDefaultMutableTreeNode.PPIS_PPINOTIFY) {
            for (int indexI = xmlppi.getPpiNotifyList().size() - 1; indexI > -1; indexI--) {
                xmlppi.removePpiNotify(indexI);
            }
            if (xmlppi.getPpiList().size() < 1 && xmlppi.getPpiNotifyList().size() < 1) {
                xmlppi = null;
            }
        }
        if (intCategory == IDefaultMutableTreeNode.PPIS_PPI_ITEM) {
            xmlppi.removePpi(intLocation);
        }
        if (intCategory == IDefaultMutableTreeNode.PPIS_PPINOTIFY_ITEM) {
            xmlppi.removePpiNotify(intLocation);
        }

        //
        //Delete Variables
        //
        if (intCategory == IDefaultMutableTreeNode.VARIABLES) {
            xmlvb = null;
        }
        if (intCategory == IDefaultMutableTreeNode.VARIABLES_VARIABLE_ITEM) {
            xmlvb.removeVariable(intLocation);
            if (xmlvb.getVariableList().size() < 1) {
                xmlvb = null;
            }
        }

        //
        //Delete BootModes
        //
        if (intCategory == IDefaultMutableTreeNode.BOOTMODES) {
            xmlbm = null;
        }
        if (intCategory == IDefaultMutableTreeNode.BOOTMODES_BOOTMODE_ITEM) {
            xmlbm.removeBootMode(intLocation);
            if (xmlbm.getBootModeList().size() < 1) {
                xmlbm = null;
            }
        }

        //
        //Delete SystemTables
        //
        if (intCategory == IDefaultMutableTreeNode.SYSTEMTABLES) {
            xmlst = null;
        }
        if (intCategory == IDefaultMutableTreeNode.SYSTEMTABLES_SYSTEMTABLE_ITEM) {
            xmlst.removeSystemTable(intLocation);
            if (xmlst.getSystemTableList().size() < 1) {
                xmlst = null;
            }
        }

        //
        //Delete DataHubs
        //
        if (intCategory == IDefaultMutableTreeNode.DATAHUBS) {
            xmldh = null;
        }
        if (intCategory == IDefaultMutableTreeNode.DATAHUBS_DATAHUB_ITEM) {
            xmldh.removeDataHubRecord(intLocation);
            if (xmldh.getDataHubRecordList().size() < 1) {
                xmldh = null;
            }
        }

        //
        //Delete Formsets
        //
        if (intCategory == IDefaultMutableTreeNode.FORMSETS) {
            xmlfs = null;
        }
        if (intCategory == IDefaultMutableTreeNode.FORMSETS_FORMSET_ITEM) {
            xmlfs.removeFormset(intLocation);
            if (xmlfs.getFormsetList().size() < 1) {
                xmlfs = null;
            }
        }

        //
        //Delete Guids
        //
        if (intCategory == IDefaultMutableTreeNode.GUIDS) {
            xmlgu = null;
        }
        if (intCategory == IDefaultMutableTreeNode.GUIDS_GUIDENTRY_ITEM) {
            xmlgu.removeGuidEntry(intLocation);
            if (xmlgu.getGuidEntryList().size() < 1) {
                xmlgu = null;
            }
        }

        //
        //Delete Externs
        //
        if (intCategory == IDefaultMutableTreeNode.EXTERNS) {
            xmlet = null;
        }
        if (intCategory == IDefaultMutableTreeNode.EXTERNS_EXTERN_ITEM) {
            xmlet.removeExtern(intLocation);
            if (xmlet.getExternList().size() < 1) {
                xmlet = null;
            }
        }

        //
        //Delete PCDs
        //
        if (intCategory == IDefaultMutableTreeNode.PCDS) {
            xmlpcd = null;
        }
        if (intCategory == IDefaultMutableTreeNode.PCDS_PCDDATA_ITEM) {
            xmlpcd.removePcdData(intLocation);
            if (xmlpcd.getPcdDataList().size() < 1) {
                xmlpcd = null;
            }
        }
        this.cleanDesktopPane();
        reloadTreeAndTable(UPDATE_WITH_CHANGE);
    }

    /**
     View current Module
     
     **/
    private void viewCurrentModule() {
        int intCategory = iTree.getSelectCategory();
        int intLocation = iTree.getSelectLoaction();
        //
        //View Msa Header
        //
        if (intCategory == IDefaultMutableTreeNode.MSA_HEADER) {
            showMsaHeader(ModuleMain.VIEW);
        }

        //
        //View Mbd Header
        //
        if (intCategory == IDefaultMutableTreeNode.MBD_HEADER) {
            showMbdHeader(ModuleMain.VIEW);
        }

        //
        //View Msa Lib Header
        //
        if (intCategory == IDefaultMutableTreeNode.MLSA_HEADER) {
            showMlsaHeader(ModuleMain.VIEW);
        }

        //
        //View Mbd Lib Header
        //
        if (intCategory == IDefaultMutableTreeNode.MLBD_HEADER) {
            showMlbdHeader(ModuleMain.VIEW);
        }

        //
        //View Libraries
        //
        if (intCategory == IDefaultMutableTreeNode.LIBRARIES_LIBRARY
            || intCategory == IDefaultMutableTreeNode.LIBRARIES_ARCH_ITEM) {
            showLibraries(ModuleMain.VIEW, intCategory, intLocation);
        }

        //
        //View LIBRARY CLASS DEFINITIONS
        //
        if (intCategory == IDefaultMutableTreeNode.LIBRARYCLASSDEFINITIONS) {
            showLibraryClassDefinitions(ModuleMain.VIEW, intCategory);
        }

        //
        //View Source Files
        //
        if (intCategory == IDefaultMutableTreeNode.SOURCEFILES_FILENAME
            || intCategory == IDefaultMutableTreeNode.SOURCEFILES_ARCH_ITEM) {
            showSourceFiles(ModuleMain.VIEW, intCategory, intLocation);
        }

        //
        //View Includes
        //
        if (intCategory == IDefaultMutableTreeNode.INCLUDES_PACKAGENAME
            || intCategory == IDefaultMutableTreeNode.INCLUDES_ARCH_ITEM) {
            showIncludes(ModuleMain.VIEW, intCategory, intLocation);
        }

        //
        //View Protocols
        //
        if (intCategory == IDefaultMutableTreeNode.PROTOCOLS_PROTOCOL_ITEM
            || intCategory == IDefaultMutableTreeNode.PROTOCOLS_PROTOCOLNOTIFY_ITEM) {
            showProtocols(ModuleMain.VIEW, intCategory, intLocation);
        }

        //
        //View Hobs
        //
        if (intCategory == IDefaultMutableTreeNode.HOBS_HOB_ITEM) {
            showHobs(ModuleMain.VIEW, intCategory, intLocation);
        }

        //
        //View Events
        //
        if (intCategory == IDefaultMutableTreeNode.EVENTS_CREATEEVENTS_ITEM
            || intCategory == IDefaultMutableTreeNode.EVENTS_SIGNALEVENTS_ITEM) {
            showEvents(ModuleMain.VIEW, intCategory, intLocation);
        }

        //
        //View Ppis
        //
        if (intCategory == IDefaultMutableTreeNode.PPIS_PPI_ITEM
            || intCategory == IDefaultMutableTreeNode.PPIS_PPINOTIFY_ITEM) {
            showPpis(ModuleMain.VIEW, intCategory, intLocation);
        }

        //
        //View Variables
        //
        if (intCategory == IDefaultMutableTreeNode.VARIABLES_VARIABLE_ITEM) {
            showVariables(ModuleMain.VIEW, intCategory, intLocation);
        }

        //
        //View BootModes
        //
        if (intCategory == IDefaultMutableTreeNode.BOOTMODES_BOOTMODE_ITEM) {
            showBootModes(ModuleMain.VIEW, intCategory, intLocation);
        }

        //
        //View SystemTables
        //
        if (intCategory == IDefaultMutableTreeNode.SYSTEMTABLES_SYSTEMTABLE_ITEM) {
            showSystemTables(ModuleMain.VIEW, intCategory, intLocation);
        }

        //
        //View DataHubs
        //
        if (intCategory == IDefaultMutableTreeNode.DATAHUBS_DATAHUB_ITEM) {
            showDataHubs(ModuleMain.VIEW, intCategory, intLocation);
        }

        //
        //View Formsets
        //
        if (intCategory == IDefaultMutableTreeNode.FORMSETS_FORMSET_ITEM) {
            showFormsets(ModuleMain.VIEW, intCategory, intLocation);
        }

        //
        //View Guids
        //
        if (intCategory == IDefaultMutableTreeNode.GUIDS_GUIDENTRY_ITEM) {
            showGuids(ModuleMain.VIEW, intCategory, intLocation);
        }

        //
        //View Externs
        //
        if (intCategory == IDefaultMutableTreeNode.EXTERNS_EXTERN_ITEM) {
            showExterns(ModuleMain.VIEW, intCategory, intLocation);
        }

        //
        //View PCDs
        //
        if (intCategory == IDefaultMutableTreeNode.PCDS_PCDDATA_ITEM) {
            showPCDs(ModuleMain.VIEW, intCategory, intLocation);
        }
    }

    /**
     Execute update operation of current module
     
     @param intCategory The category of current node
     @param intLocation The location of current node
     
     **/
    private void updateCurrentModule(int intCategory, int intLocation) {
        //
        //Update Msa Header
        //
        if (intCategory == IDefaultMutableTreeNode.MSA_HEADER) {
            showMsaHeader(ModuleMain.UPDATE);
        }

        //
        //Update Mbd Header
        //
        if (intCategory == IDefaultMutableTreeNode.MBD_HEADER) {
            showMbdHeader(ModuleMain.UPDATE);
        }

        //
        //Update Msa Lib Header
        //
        if (intCategory == IDefaultMutableTreeNode.MLSA_HEADER) {
            showMlsaHeader(ModuleMain.UPDATE);
        }

        //
        //Update Mbd Lib Header
        //
        if (intCategory == IDefaultMutableTreeNode.MLBD_HEADER) {
            showMlbdHeader(ModuleMain.UPDATE);
        }

        //
        //Update Libraries
        //
        if (intCategory == IDefaultMutableTreeNode.LIBRARIES_LIBRARY
            || intCategory == IDefaultMutableTreeNode.LIBRARIES_ARCH_ITEM) {
            showLibraries(ModuleMain.UPDATE, intCategory, intLocation);
        }

        //
        //Update LIBRARY CLASS DEFINITIONS
        //
        if (intCategory == IDefaultMutableTreeNode.LIBRARYCLASSDEFINITIONS) {
            showLibraryClassDefinitions(ModuleMain.UPDATE, intCategory);
        }

        //
        //Update Source Files
        //
        if (intCategory == IDefaultMutableTreeNode.SOURCEFILES_FILENAME
            || intCategory == IDefaultMutableTreeNode.SOURCEFILES_ARCH_ITEM) {
            showSourceFiles(ModuleMain.UPDATE, intCategory, intLocation);
        }

        //
        //Update Includes
        //
        if (intCategory == IDefaultMutableTreeNode.INCLUDES_PACKAGENAME
            || intCategory == IDefaultMutableTreeNode.INCLUDES_ARCH_ITEM) {
            showIncludes(ModuleMain.UPDATE, intCategory, intLocation);
        }

        //
        //Update Protocols
        //
        if (intCategory == IDefaultMutableTreeNode.PROTOCOLS_PROTOCOL_ITEM
            || intCategory == IDefaultMutableTreeNode.PROTOCOLS_PROTOCOLNOTIFY_ITEM) {
            showProtocols(ModuleMain.UPDATE, intCategory, intLocation);
        }

        //
        //Update Hobs
        //
        if (intCategory == IDefaultMutableTreeNode.HOBS_HOB_ITEM) {
            showHobs(ModuleMain.UPDATE, intCategory, intLocation);
        }

        //
        //Update Events
        //
        if (intCategory == IDefaultMutableTreeNode.EVENTS_CREATEEVENTS_ITEM
            || intCategory == IDefaultMutableTreeNode.EVENTS_SIGNALEVENTS_ITEM) {
            showEvents(ModuleMain.UPDATE, intCategory, intLocation);
        }

        //
        //Update Ppis
        //
        if (intCategory == IDefaultMutableTreeNode.PPIS_PPI_ITEM
            || intCategory == IDefaultMutableTreeNode.PPIS_PPINOTIFY_ITEM) {
            showPpis(ModuleMain.UPDATE, intCategory, intLocation);
        }

        //
        //Update Variables
        //
        if (intCategory == IDefaultMutableTreeNode.VARIABLES_VARIABLE_ITEM) {
            showVariables(ModuleMain.UPDATE, intCategory, intLocation);
        }

        //
        //Update BootModes
        //
        if (intCategory == IDefaultMutableTreeNode.BOOTMODES_BOOTMODE_ITEM) {
            showBootModes(ModuleMain.UPDATE, intCategory, intLocation);
        }

        //
        //Update SystemTables
        //
        if (intCategory == IDefaultMutableTreeNode.SYSTEMTABLES_SYSTEMTABLE_ITEM) {
            showSystemTables(ModuleMain.UPDATE, intCategory, intLocation);
        }

        //
        //Update DataHubs
        //
        if (intCategory == IDefaultMutableTreeNode.DATAHUBS_DATAHUB_ITEM) {
            showDataHubs(ModuleMain.UPDATE, intCategory, intLocation);
        }

        //
        //Update Formsets
        //
        if (intCategory == IDefaultMutableTreeNode.FORMSETS_FORMSET_ITEM) {
            showFormsets(ModuleMain.UPDATE, intCategory, intLocation);
        }

        //
        //Update Guids
        //
        if (intCategory == IDefaultMutableTreeNode.GUIDS_GUIDENTRY_ITEM) {
            showGuids(ModuleMain.UPDATE, intCategory, intLocation);
        }

        //
        //Update Externs
        //
        if (intCategory == IDefaultMutableTreeNode.EXTERNS_EXTERN_ITEM) {
            showExterns(ModuleMain.UPDATE, intCategory, intLocation);
        }

        //
        //Update PCDs
        //
        if (intCategory == IDefaultMutableTreeNode.PCDS_PCDDATA_ITEM) {
            showPCDs(ModuleMain.UPDATE, intCategory, intLocation);
        }
    }

    /**
     Save current module
     Call relevant function via different file types
     
     **/
    private void saveCurrentModule() {
        if (this.saveFileName == "") {
            openFile(2, this.currentModuleType);
        }
        if (this.saveFileName == "") {
            this.saveFileName = this.currentModule;
            return;
        } else {
            switch (this.currentModuleType) {
            case 1:
                saveMsa();
                break;
            case 2:
                saveMbd();
                break;
            case 3:
                saveMlsa();
                break;
            case 4:
                saveMlbd();
                break;
            }

        }

        reloadTreeAndTable(SAVE_WITH_CHANGE);
    }

    /**
     Save current module as
     
     **/
    private void saveAsCurrentModule() {
        this.saveFileName = "";
        saveCurrentModule();
    }

    /**
     Save file as msa
     
     **/
    private void saveMsa() {
        File f = new File(this.saveFileName);
        ModuleSurfaceAreaDocument msaDoc = ModuleSurfaceAreaDocument.Factory.newInstance();
        ModuleSurfaceAreaDocument.ModuleSurfaceArea msa = ModuleSurfaceAreaDocument.ModuleSurfaceArea.Factory
                                                                                                             .newInstance();

        //
        //Add all components into xml doc file
        //
        if (xmlmh != null) {
            msa.setMsaHeader(xmlmh);
        }
        if (xmllcd != null) {
            msa.setLibraryClassDefinitions(xmllcd);
        }
        if (xmlsf != null) {
            msa.setSourceFiles(xmlsf);
        }
        if (xmlic != null) {
            msa.setIncludes(xmlic);
        }
        if (xmlpl != null) {
            msa.setProtocols(xmlpl);
        }
        if (xmlen != null) {
            msa.setEvents(xmlen);
        }
        if (xmlhob != null) {
            msa.setHobs(xmlhob);
        }
        if (xmlppi != null) {
            msa.setPPIs(xmlppi);
        }
        if (xmlvb != null) {
            msa.setVariables(xmlvb);
        }
        if (xmlbm != null) {
            msa.setBootModes(xmlbm);
        }
        if (xmlst != null) {
            msa.setSystemTables(xmlst);
        }
        if (xmldh != null) {
            msa.setDataHubs(xmldh);
        }
        if (xmlfs != null) {
            msa.setFormsets(xmlfs);
        }
        if (xmlgu != null) {
            msa.setGuids(xmlgu);
        }
        if (xmlet != null) {
            msa.setExterns(xmlet);
        }
        if (xmlpcd != null) {
            msa.setPCDs(xmlpcd);
        }
        if (xmlbo != null) {
            msa.setBuildOptions(xmlbo);
        }
        //
        //Init namespace
        //
        XmlCursor cursor = msa.newCursor();
        String uri = "http://www.TianoCore.org/2006/Edk2.0";
        cursor.push();
        cursor.toNextToken();
        cursor.insertNamespace("", uri);
        cursor.insertNamespace("xsi", "http://www.w3.org/2001/XMLSchema-instance");
        cursor.pop();

        //
        //Config file format
        //
        XmlOptions options = new XmlOptions();
        options.setCharacterEncoding("UTF-8");
        options.setSavePrettyPrint();
        options.setSavePrettyPrintIndent(2);

        //
        //Create finial doc
        //
        msaDoc.addNewModuleSurfaceArea();
        msaDoc.setModuleSurfaceArea((ModuleSurfaceAreaDocument.ModuleSurfaceArea) cursor.getObject());
        try {
            //
            //Save the file
            //
            msaDoc.save(f, options);
            this.currentModule = this.saveFileName;
        } catch (Exception e) {
            Log.err("Save Msa", e.getMessage());
        }
    }

    /**
     Save file as mbd
     
     **/
    private void saveMbd() {
        File f = new File(this.saveFileName);
        ModuleBuildDescriptionDocument mbdDoc = ModuleBuildDescriptionDocument.Factory.newInstance();
        ModuleBuildDescriptionDocument.ModuleBuildDescription mbd = ModuleBuildDescriptionDocument.ModuleBuildDescription.Factory
                                                                                                                                 .newInstance();
        //
        //Add all components into xml doc file
        //
        if (xmlmbdh != null) {
            mbd.setMbdHeader(xmlmbdh);
        }
        if (xmllib != null) {
            mbd.setLibraries(xmllib);
        }
        if (xmlsf != null) {
            mbd.setSourceFiles(xmlsf);
        }
        if (xmlic != null) {
            mbd.setIncludes(xmlic);
        }
        if (xmlpl != null) {
            mbd.setProtocols(xmlpl);
        }
        if (xmlen != null) {
            mbd.setEvents(xmlen);
        }
        if (xmlhob != null) {
            mbd.setHobs(xmlhob);
        }
        if (xmlppi != null) {
            mbd.setPPIs(xmlppi);
        }
        if (xmlvb != null) {
            mbd.setVariables(xmlvb);
        }
        if (xmlbm != null) {
            mbd.setBootModes(xmlbm);
        }
        if (xmlst != null) {
            mbd.setSystemTables(xmlst);
        }
        if (xmldh != null) {
            mbd.setDataHubs(xmldh);
        }
        if (xmlfs != null) {
            mbd.setFormsets(xmlfs);
        }
        if (xmlgu != null) {
            mbd.setGuids(xmlgu);
        }
        if (xmlet != null) {
            mbd.setExterns(xmlet);
        }
        if (xmlpcd != null) {
            mbd.setPCDs(xmlpcd);
        }
        if (xmlbo != null) {
            mbd.setBuildOptions(xmlbo);
        }
        //
        //Init namespace
        //
        XmlCursor cursor = mbd.newCursor();
        String uri = "http://www.TianoCore.org/2006/Edk2.0";
        cursor.push();
        cursor.toNextToken();
        cursor.insertNamespace("", uri);
        cursor.insertNamespace("xsi", "http://www.w3.org/2001/XMLSchema-instance");
        cursor.pop();

        //
        //Config file format
        //
        XmlOptions options = new XmlOptions();
        options.setCharacterEncoding("UTF-8");
        options.setSavePrettyPrint();
        options.setSavePrettyPrintIndent(2);

        //
        //Create finial doc
        //
        mbdDoc.addNewModuleBuildDescription();
        mbdDoc.setModuleBuildDescription((ModuleBuildDescriptionDocument.ModuleBuildDescription) cursor.getObject());
        try {
            //
            //Save the file
            //
            mbdDoc.save(f, options);
            this.currentModule = this.saveFileName;
        } catch (Exception e) {
            Log.err("Save Mbd", e.getMessage());
        }
    }

    /**
     Save file as mlsa
     
     **/
    private void saveMlsa() {
        File f = new File(this.saveFileName);
        LibraryModuleSurfaceAreaDocument mlsaDoc = LibraryModuleSurfaceAreaDocument.Factory.newInstance();
        LibraryModuleSurfaceAreaDocument.LibraryModuleSurfaceArea mlsa = LibraryModuleSurfaceAreaDocument.LibraryModuleSurfaceArea.Factory
                                                                                                                                          .newInstance();
        //
        //Add all components into xml doc file
        //
        if (xmlmlh != null) {
            mlsa.setMsaLibHeader(xmlmlh);
        }
        if (xmllcd != null) {
            mlsa.setLibraryClassDefinitions(xmllcd);
        }
        if (xmlsf != null) {
            mlsa.setSourceFiles(xmlsf);
        }
        if (xmlic != null) {
            mlsa.setIncludes(xmlic);
        }
        if (xmlpl != null) {
            mlsa.setProtocols(xmlpl);
        }
        if (xmlen != null) {
            mlsa.setEvents(xmlen);
        }
        if (xmlhob != null) {
            mlsa.setHobs(xmlhob);
        }
        if (xmlppi != null) {
            mlsa.setPPIs(xmlppi);
        }
        if (xmlvb != null) {
            mlsa.setVariables(xmlvb);
        }
        if (xmlbm != null) {
            mlsa.setBootModes(xmlbm);
        }
        if (xmlst != null) {
            mlsa.setSystemTables(xmlst);
        }
        if (xmldh != null) {
            mlsa.setDataHubs(xmldh);
        }
        if (xmlfs != null) {
            mlsa.setFormsets(xmlfs);
        }
        if (xmlgu != null) {
            mlsa.setGuids(xmlgu);
        }
        if (xmlet != null) {
            mlsa.setExterns(xmlet);
        }
        if (xmlpcd != null) {
            mlsa.setPCDs(xmlpcd);
        }
        if (xmlbo != null) {
            mlsa.setBuildOptions(xmlbo);
        }
        //
        //Init namespace
        //
        XmlCursor cursor = mlsa.newCursor();
        String uri = "http://www.TianoCore.org/2006/Edk2.0";
        cursor.push();
        cursor.toNextToken();
        cursor.insertNamespace("", uri);
        cursor.insertNamespace("xsi", "http://www.w3.org/2001/XMLSchema-instance");
        cursor.pop();

        //
        //Config file format
        //
        XmlOptions options = new XmlOptions();
        options.setCharacterEncoding("UTF-8");
        options.setSavePrettyPrint();
        options.setSavePrettyPrintIndent(2);

        //
        //Create finial doc
        //
        mlsaDoc.addNewLibraryModuleSurfaceArea();
        mlsaDoc
               .setLibraryModuleSurfaceArea((LibraryModuleSurfaceAreaDocument.LibraryModuleSurfaceArea) cursor
                                                                                                              .getObject());
        try {
            //
            //Save the file
            //
            mlsaDoc.save(f, options);
            this.currentModule = this.saveFileName;
        } catch (Exception e) {
            Log.err("Save Mlsa", e.getMessage());
        }
    }

    /**
     Save file as mbd
     
     **/
    private void saveMlbd() {
        File f = new File(this.saveFileName);
        LibraryModuleBuildDescriptionDocument mlbdDoc = LibraryModuleBuildDescriptionDocument.Factory.newInstance();
        LibraryModuleBuildDescriptionDocument.LibraryModuleBuildDescription mlbd = LibraryModuleBuildDescriptionDocument.LibraryModuleBuildDescription.Factory
                                                                                                                                                              .newInstance();
        //
        //Add all components into xml doc file
        //
        if (xmlmlbdh != null) {
            mlbd.setMbdLibHeader(xmlmlbdh);
        }
        if (xmllib != null) {
            mlbd.setLibraries(xmllib);
        }
        if (xmlsf != null) {
            mlbd.setSourceFiles(xmlsf);
        }
        if (xmlic != null) {
            mlbd.setIncludes(xmlic);
        }
        if (xmlpl != null) {
            mlbd.setProtocols(xmlpl);
        }
        if (xmlen != null) {
            mlbd.setEvents(xmlen);
        }
        if (xmlhob != null) {
            mlbd.setHobs(xmlhob);
        }
        if (xmlppi != null) {
            mlbd.setPPIs(xmlppi);
        }
        if (xmlvb != null) {
            mlbd.setVariables(xmlvb);
        }
        if (xmlbm != null) {
            mlbd.setBootModes(xmlbm);
        }
        if (xmlst != null) {
            mlbd.setSystemTables(xmlst);
        }
        if (xmldh != null) {
            mlbd.setDataHubs(xmldh);
        }
        if (xmlfs != null) {
            mlbd.setFormsets(xmlfs);
        }
        if (xmlgu != null) {
            mlbd.setGuids(xmlgu);
        }
        if (xmlet != null) {
            mlbd.setExterns(xmlet);
        }
        if (xmlpcd != null) {
            mlbd.setPCDs(xmlpcd);
        }
        if (xmlbo != null) {
            mlbd.setBuildOptions(xmlbo);
        }
        //
        //Init namespace
        //
        XmlCursor cursor = mlbd.newCursor();
        String uri = "http://www.TianoCore.org/2006/Edk2.0";
        cursor.push();
        cursor.toNextToken();
        cursor.insertNamespace("", uri);
        cursor.insertNamespace("xsi", "http://www.w3.org/2001/XMLSchema-instance");
        cursor.pop();

        //
        //Config file format
        //
        XmlOptions options = new XmlOptions();
        options.setCharacterEncoding("UTF-8");
        options.setSavePrettyPrint();
        options.setSavePrettyPrintIndent(2);

        //
        //Create finial doc
        //
        mlbdDoc.addNewLibraryModuleBuildDescription();
        mlbdDoc
               .setLibraryModuleBuildDescription((LibraryModuleBuildDescriptionDocument.LibraryModuleBuildDescription) cursor
                                                                                                                             .getObject());
        try {
            //
            //Save the file
            //
            mlbdDoc.save(f, options);
            this.currentModule = this.saveFileName;
        } catch (Exception e) {
            Log.err("Save Mbd", e.getMessage());
        }
    }

    /**
     Reflash the tree via current value of xml documents.
     
     @param intMode The input data of current operation type
     
     **/
    private void reloadTreeAndTable(int intMode) {
        makeTree();
        if (intMode == ModuleMain.OPENED) {
            this.jMenuItemModuleClose.setEnabled(true);
            this.jMenuItemModuleSaveAs.setEnabled(true);
            this.jMenuEditAdd.setEnabled(true);
            this.setTitle(windowTitle + "- [" + this.currentModule + "]");
            this.jButtonOk.setEnabled(false);
            this.jButtonCancel.setEnabled(false);
        }
        if (intMode == ModuleMain.CLOSED) {
            this.jMenuItemModuleClose.setEnabled(false);
            this.jMenuItemModuleSave.setEnabled(false);
            this.jMenuItemModuleSaveAs.setEnabled(false);
            this.jMenuEditAdd.setEnabled(false);
            this.setTitle(windowTitle + "- [" + ws.getCurrentWorkspace() + "]");
            this.setButtonEnable(false);
        }
        if (intMode == ModuleMain.NEW_WITHOUT_CHANGE) {

        }

        if (intMode == ModuleMain.NEW_WITH_CHANGE) {
            this.jMenuItemModuleClose.setEnabled(true);
            this.jMenuItemModuleSave.setEnabled(true);
            this.jMenuItemModuleSaveAs.setEnabled(true);
            this.jMenuEditAdd.setEnabled(true);
            setButtonEnable(false);
        }
        if (intMode == ModuleMain.UPDATE_WITHOUT_CHANGE) {

        }
        if (intMode == ModuleMain.UPDATE_WITH_CHANGE) {
            this.jMenuItemModuleClose.setEnabled(true);
            this.jMenuItemModuleSave.setEnabled(true);
            this.jMenuItemModuleSaveAs.setEnabled(true);
        }
        if (intMode == ModuleMain.SAVE_WITHOUT_CHANGE) {
            this.jMenuItemModuleClose.setEnabled(true);
            this.jMenuItemModuleSave.setEnabled(true);
            this.jMenuItemModuleSaveAs.setEnabled(true);
            this.jButtonOk.setEnabled(false);
            this.jButtonCancel.setEnabled(false);
        }
        if (intMode == ModuleMain.SAVE_WITH_CHANGE) {
            this.jMenuItemModuleClose.setEnabled(true);
            this.jMenuItemModuleSave.setEnabled(false);
            this.jMenuItemModuleSaveAs.setEnabled(true);
            this.jMenuItemEditUpdate.setEnabled(false);
            this.jMenuItemEditDelete.setEnabled(false);
            this.setTitle(windowTitle + "- [" + this.currentModule + "]");
            this.jButtonOk.setEnabled(false);
            this.jButtonCancel.setEnabled(false);
        }

        if (this.currentModuleType == 1 || this.currentModuleType == 3) {
            this.jMenuItemEditAddLibraries.setEnabled(false);
            this.jMenuItemEditAddLibraryClassDefinitions.setEnabled(true);
        }
        if (this.currentModuleType == 2 || this.currentModuleType == 4) {
            this.jMenuItemEditAddLibraries.setEnabled(true);
            this.jMenuItemEditAddLibraryClassDefinitions.setEnabled(false);
        }
    }

    /**
     Enable/Disable button Ok and Cancel
     
     @param isEnabled The input data to indicate if button is enabled or not
     
     **/
    private void setButtonEnable(boolean isEnabled) {
        this.jButtonCancel.setEnabled(isEnabled);
        this.jButtonOk.setEnabled(isEnabled);
    }

    /**
     Show msa header
     When the operation is VIEW, disable all fields of internal frame
     
     @param type The input data of operation type
     
     **/
    private void showMsaHeader(int type) {
        msa = null;
        msa = new MsaHeader(this.xmlmh);
        this.jDesktopPane.removeAll();
        this.jDesktopPane.add(msa, 1);
        this.currentNodeType = IDefaultMutableTreeNode.MSA_HEADER;
        this.currentModuleType = 1;
        if (type == ModuleMain.VIEW) {
            setButtonEnable(false);
            msa.setViewMode(true);
        } else {
            setButtonEnable(true);
        }
    }

    /**
    Show MbdHeader
    When the operation is VIEW, disable all fields of internal frame
    
    @param type The input data of operation type
    
    **/
    private void showMbdHeader(int type) {
        mbd = null;
        mbd = new MbdHeader(this.xmlmbdh);
        this.jDesktopPane.removeAll();
        this.jDesktopPane.add(mbd, 1);
        this.currentNodeType = IDefaultMutableTreeNode.MBD_HEADER;
        this.currentModuleType = 2;
        if (type == ModuleMain.VIEW) {
            setButtonEnable(false);
            mbd.setViewMode(true);
        } else {
            setButtonEnable(true);
        }
    }

    /**
    Show MlsaHeader
    When the operation is VIEW, disable all fields of internal frame
    
    @param type The input data of operation type
    
    **/
    private void showMlsaHeader(int type) {
        mlsa = null;
        mlsa = new MsaLibHeader(this.xmlmlh);
        this.jDesktopPane.removeAll();
        this.jDesktopPane.add(mlsa, 1);
        this.currentNodeType = IDefaultMutableTreeNode.MLSA_HEADER;
        this.currentModuleType = 3;
        if (type == ModuleMain.VIEW) {
            setButtonEnable(false);
            mlsa.setViewMode(true);
        } else {
            setButtonEnable(true);
        }
    }

    /**
    Show MlbdHeader
    When the operation is VIEW, disable all fields of internal frame
    
    @param type The input data of operation type
    
    **/
    private void showMlbdHeader(int type) {
        mlbd = null;
        mlbd = new MbdLibHeader(this.xmlmlbdh);
        this.jDesktopPane.removeAll();
        this.jDesktopPane.add(mlbd, 1);
        this.currentNodeType = IDefaultMutableTreeNode.MLBD_HEADER;
        this.currentModuleType = 4;
        if (type == ModuleMain.VIEW) {
            setButtonEnable(false);
            mlbd.setViewMode(true);
        } else {
            setButtonEnable(true);
        }
    }

    /**
    Show Libraries
    When the operation is VIEW, disable all fields of internal frame
    
    @param type The input data of operation type
    
    **/
    private void showLibraries(int operationType, int nodeType, int location) {
        mlib = null;
        if (operationType == ModuleMain.ADD) {
            mlib = new MbdLibraries(this.xmllib, -1, -1, 1);
        }
        if (operationType == ModuleMain.UPDATE || operationType == ModuleMain.VIEW) {
            mlib = new MbdLibraries(this.xmllib, nodeType, location, 2);
        }
        this.jDesktopPane.removeAll();
        this.jDesktopPane.add(mlib, 1);
        this.currentNodeType = nodeType;
        if (operationType == ModuleMain.VIEW) {
            setButtonEnable(false);
            mlib.setViewMode(true);
        } else {
            setButtonEnable(true);
        }
    }

    /**
    Show LibraryClassDefinitions
    When the operation is VIEW, disable all fields of internal frame
    
    @param type The input data of operation type
    
    **/
    private void showLibraryClassDefinitions(int operationType, int nodeType) {
        mlcd = null;
        if (operationType == ModuleMain.ADD) {
            mlcd = new ModuleLibraryClassDefinitions(this.xmllcd);
        }
        if (operationType == ModuleMain.UPDATE || operationType == ModuleMain.VIEW) {
            mlcd = new ModuleLibraryClassDefinitions(this.xmllcd);
        }
        this.jDesktopPane.removeAll();
        this.jDesktopPane.add(mlcd, 1);
        this.currentNodeType = nodeType;
        if (operationType == ModuleMain.VIEW) {
            setButtonEnable(false);
            mlcd.setViewMode(true);
        } else {
            setButtonEnable(true);
        }
    }

    /**
    Show SourceFiles
    When the operation is VIEW, disable all fields of internal frame
    
    @param type The input data of operation type
    
    **/
    private void showSourceFiles(int operationType, int nodeType, int location) {
        msf = null;
        if (operationType == ModuleMain.ADD) {
            msf = new ModuleSourceFiles(this.xmlsf, -1, -1, 1);
        }
        if (operationType == ModuleMain.UPDATE || operationType == ModuleMain.VIEW) {
            msf = new ModuleSourceFiles(this.xmlsf, nodeType, location, 2);
        }
        this.jDesktopPane.removeAll();
        this.jDesktopPane.add(msf, 1);
        this.currentNodeType = nodeType;
        if (operationType == ModuleMain.VIEW) {
            setButtonEnable(false);
            msf.setViewMode(true);
        } else {
            setButtonEnable(true);
        }
    }

    /**
    Show Includes
    When the operation is VIEW, disable all fields of internal frame
    
    @param type The input data of operation type
    
    **/
    private void showIncludes(int operationType, int nodeType, int location) {
        mic = null;
        if (operationType == ModuleMain.ADD) {
            mic = new ModuleIncludes(this.xmlic, -1, -1, 1);
        }
        if (operationType == ModuleMain.UPDATE || operationType == ModuleMain.VIEW) {
            mic = new ModuleIncludes(this.xmlic, nodeType, location, 2);
        }
        this.jDesktopPane.removeAll();
        this.jDesktopPane.add(mic, 1);
        this.currentNodeType = nodeType;
        if (operationType == ModuleMain.VIEW) {
            setButtonEnable(false);
            mic.setViewMode(true);
        } else {
            setButtonEnable(true);
        }
    }

    /**
    Show Protocols
    When the operation is VIEW, disable all fields of internal frame
    
    @param type The input data of operation type
    
    **/
    private void showProtocols(int operationType, int nodeType, int location) {
        mp = null;
        if (operationType == ModuleMain.ADD) {
            mp = new ModuleProtocols(this.xmlpl);
        }
        if (operationType == ModuleMain.UPDATE || operationType == ModuleMain.VIEW) {
            mp = new ModuleProtocols(this.xmlpl, nodeType, location);
        }
        this.jDesktopPane.removeAll();
        this.jDesktopPane.add(mp, 1);
        this.currentNodeType = nodeType;
        if (operationType == ModuleMain.VIEW) {
            setButtonEnable(false);
            mp.setViewMode(true);
        } else {
            setButtonEnable(true);
        }
    }

    /**
    Show Events
    When the operation is VIEW, disable all fields of internal frame
    
    @param type The input data of operation type
    
    **/
    private void showEvents(int operationType, int nodeType, int location) {
        mev = null;
        if (operationType == ModuleMain.ADD) {
            mev = new ModuleEvents(this.xmlen);
        }
        if (operationType == ModuleMain.UPDATE || operationType == ModuleMain.VIEW) {
            mev = new ModuleEvents(this.xmlen, nodeType, location);
        }
        this.jDesktopPane.removeAll();
        this.jDesktopPane.add(mev, 1);
        this.currentNodeType = nodeType;
        if (operationType == ModuleMain.VIEW) {
            setButtonEnable(false);
            mev.setViewMode(true);
        } else {
            setButtonEnable(true);
        }
    }

    /**
    Show Hobs
    When the operation is VIEW, disable all fields of internal frame
    
    @param type The input data of operation type
    
    **/
    private void showHobs(int operationType, int nodeType, int location) {
        mh = null;
        if (operationType == ModuleMain.ADD) {
            mh = new ModuleHobs(this.xmlhob);
        }
        if (operationType == ModuleMain.UPDATE || operationType == ModuleMain.VIEW) {
            mh = new ModuleHobs(this.xmlhob, nodeType, location);
        }
        this.jDesktopPane.removeAll();
        this.jDesktopPane.add(mh, 1);
        this.currentNodeType = nodeType;
        if (operationType == ModuleMain.VIEW) {
            setButtonEnable(false);
            mh.setViewMode(true);
        } else {
            setButtonEnable(true);
        }
    }

    /**
    Show Ppis
    When the operation is VIEW, disable all fields of internal frame
    
    @param type The input data of operation type
    
    **/
    private void showPpis(int operationType, int nodeType, int location) {
        mpp = null;
        if (operationType == ModuleMain.ADD) {
            mpp = new ModulePpis(this.xmlppi);
        }
        if (operationType == ModuleMain.UPDATE || operationType == ModuleMain.VIEW) {
            mpp = new ModulePpis(this.xmlppi, nodeType, location);
        }
        this.jDesktopPane.removeAll();
        this.jDesktopPane.add(mpp, 1);
        this.currentNodeType = nodeType;
        if (operationType == ModuleMain.VIEW) {
            setButtonEnable(false);
            mpp.setViewMode(true);
        } else {
            setButtonEnable(true);
        }
    }

    /**
    Show Variables
    When the operation is VIEW, disable all fields of internal frame
    
    @param type The input data of operation type
    
    **/
    private void showVariables(int operationType, int nodeType, int location) {
        mv = null;
        if (operationType == ModuleMain.ADD) {
            mv = new ModuleVariables(this.xmlvb);
        }
        if (operationType == ModuleMain.UPDATE || operationType == ModuleMain.VIEW) {
            mv = new ModuleVariables(this.xmlvb, nodeType, location);
        }
        this.jDesktopPane.removeAll();
        this.jDesktopPane.add(mv, 1);
        this.currentNodeType = nodeType;
        if (operationType == ModuleMain.VIEW) {
            setButtonEnable(false);
            mv.setViewMode(true);
        } else {
            setButtonEnable(true);
        }
    }

    /**
    Show BootModes
    When the operation is VIEW, disable all fields of internal frame
    
    @param type The input data of operation type
    
    **/
    private void showBootModes(int operationType, int nodeType, int location) {
        mbm = null;
        if (operationType == ModuleMain.ADD) {
            mbm = new ModuleBootModes(this.xmlbm);
        }
        if (operationType == ModuleMain.UPDATE || operationType == ModuleMain.VIEW) {
            mbm = new ModuleBootModes(this.xmlbm, nodeType, location);
        }
        this.jDesktopPane.removeAll();
        this.jDesktopPane.add(mbm, 1);
        this.currentNodeType = nodeType;
        if (operationType == ModuleMain.VIEW) {
            setButtonEnable(false);
            mbm.setViewMode(true);
        } else {
            setButtonEnable(true);
        }
    }

    /**
    Show SystemTables
    When the operation is VIEW, disable all fields of internal frame
    
    @param type The input data of operation type
    
    **/
    private void showSystemTables(int operationType, int nodeType, int location) {
        mst = null;
        if (operationType == ModuleMain.ADD) {
            mst = new ModuleSystemTables(this.xmlst);
        }
        if (operationType == ModuleMain.UPDATE || operationType == ModuleMain.VIEW) {
            mst = new ModuleSystemTables(this.xmlst, nodeType, location);
        }
        this.jDesktopPane.removeAll();
        this.jDesktopPane.add(mst, 1);
        this.currentNodeType = nodeType;
        if (operationType == ModuleMain.VIEW) {
            setButtonEnable(false);
            mst.setViewMode(true);
        } else {
            setButtonEnable(true);
        }
    }

    /**
    Show DataHubs
    When the operation is VIEW, disable all fields of internal frame
    
    @param type The input data of operation type
    
    **/
    private void showDataHubs(int operationType, int nodeType, int location) {
        mdh = null;
        if (operationType == ModuleMain.ADD) {
            mdh = new ModuleDataHubs(this.xmldh);
        }
        if (operationType == ModuleMain.UPDATE || operationType == ModuleMain.VIEW) {
            mdh = new ModuleDataHubs(this.xmldh, nodeType, location);
        }
        this.jDesktopPane.removeAll();
        this.jDesktopPane.add(mdh, 1);
        this.currentNodeType = nodeType;
        if (operationType == ModuleMain.VIEW) {
            setButtonEnable(false);
            mdh.setViewMode(true);
        } else {
            setButtonEnable(true);
        }
    }

    /**
    Show Formsets
    When the operation is VIEW, disable all fields of internal frame
    
    @param type The input data of operation type
    
    **/
    private void showFormsets(int operationType, int nodeType, int location) {
        mf = null;
        if (operationType == ModuleMain.ADD) {
            mf = new ModuleFormsets(this.xmlfs);
        }
        if (operationType == ModuleMain.UPDATE || operationType == ModuleMain.VIEW) {
            mf = new ModuleFormsets(this.xmlfs, nodeType, location);
        }
        this.jDesktopPane.removeAll();
        this.jDesktopPane.add(mf, 1);
        this.currentNodeType = nodeType;
        if (operationType == ModuleMain.VIEW) {
            setButtonEnable(false);
            mf.setViewMode(true);
        } else {
            setButtonEnable(true);
        }
    }

    /**
    Show Show Guids
    When the operation is VIEW, disable all fields of internal frame
    
    @param type The input data of operation type
    
    **/
    private void showGuids(int operationType, int nodeType, int location) {
        mg = null;
        if (operationType == ModuleMain.ADD || operationType == ModuleMain.VIEW) {
            mg = new ModuleGuids(this.xmlgu);
        }
        if (operationType == ModuleMain.UPDATE || operationType == ModuleMain.VIEW) {
            mg = new ModuleGuids(this.xmlgu, nodeType, location);
        }
        this.jDesktopPane.removeAll();
        this.jDesktopPane.add(mg, 1);
        this.currentNodeType = nodeType;
        if (operationType == ModuleMain.VIEW) {
            setButtonEnable(false);
            mg.setViewMode(true);
        } else {
            setButtonEnable(true);
        }
    }

    /**
    Show Externs
    When the operation is VIEW, disable all fields of internal frame
    
    @param type The input data of operation type
    
    **/
    private void showExterns(int operationType, int nodeType, int location) {
        met = null;
        if (operationType == ModuleMain.ADD) {
            met = new ModuleExterns(this.xmlet);
        }
        if (operationType == ModuleMain.UPDATE || operationType == ModuleMain.VIEW) {
            met = new ModuleExterns(this.xmlet, nodeType, location);
        }
        this.jDesktopPane.removeAll();
        this.jDesktopPane.add(met, 1);
        this.currentNodeType = nodeType;
        if (operationType == ModuleMain.VIEW) {
            setButtonEnable(false);
            met.setViewMode(true);
        } else {
            setButtonEnable(true);
        }
    }

    /**
    Show PCDs
    When the operation is VIEW, disable all fields of internal frame
    
    @param type The input data of operation type
    
    **/
    private void showPCDs(int operationType, int nodeType, int location) {
        mpcd = null;
        if (operationType == ModuleMain.ADD) {
            mpcd = new ModulePCDs(this.xmlpcd);
        }
        if (operationType == ModuleMain.UPDATE || operationType == ModuleMain.VIEW) {
            mpcd = new ModulePCDs(this.xmlpcd, nodeType, location);
        }
        this.jDesktopPane.removeAll();
        this.jDesktopPane.add(mpcd, 1);
        this.currentNodeType = nodeType;
        if (operationType == ModuleMain.VIEW) {
            setButtonEnable(false);
            mpcd.setViewMode(true);
        } else {
            setButtonEnable(true);
        }
    }

    /**
    Save currentModule when press button OK
    
    **/
    private void save() {
        if (this.currentNodeType == IDefaultMutableTreeNode.MSA_HEADER) {
            if (!msa.check()) {
                return;
            }
            msa.save();
            msa.setViewMode(true);
            this.xmlmh = msa.getMsaHeader();
        }

        if (this.currentNodeType == IDefaultMutableTreeNode.MBD_HEADER) {
            if (!mbd.check()) {
                return;
            }
            mbd.save();
            mbd.setViewMode(true);
            this.xmlmbdh = mbd.getMbdHeader();
        }

        if (this.currentNodeType == IDefaultMutableTreeNode.MLSA_HEADER) {
            if (!mlsa.check()) {
                return;
            }
            mlsa.save();
            mlsa.setViewMode(true);
            this.xmlmlh = mlsa.getMsaLibHeader();
        }

        if (this.currentNodeType == IDefaultMutableTreeNode.MLBD_HEADER) {
            if (!mlbd.check()) {
                return;
            }
            mlbd.save();
            mlbd.setViewMode(true);
            this.xmlmlbdh = mlbd.getMbdLibHeader();
        }

        if (this.currentNodeType == IDefaultMutableTreeNode.LIBRARIES
            || this.currentNodeType == IDefaultMutableTreeNode.LIBRARIES_ARCH
            || this.currentNodeType == IDefaultMutableTreeNode.LIBRARIES_ARCH_ITEM
            || this.currentNodeType == IDefaultMutableTreeNode.LIBRARIES_LIBRARY
            || this.currentNodeType == IDefaultMutableTreeNode.LIBRARIES_LIBRARY_ITEM) {
            if (!mlib.check()) {
                return;
            }
            mlib.save();
            mlib.setViewMode(true);
            this.xmllib = mlib.getLibraries();
        }

        if (this.currentNodeType == IDefaultMutableTreeNode.LIBRARYCLASSDEFINITIONS
            || this.currentNodeType == IDefaultMutableTreeNode.LIBRARY_CLASS_DEFINITION) {
            if (!mlcd.check()) {
                return;
            }
            mlcd.save();
            mlcd.setViewMode(true);
            this.xmllcd = mlcd.getLibraryClassDefinitions();
        }

        if (this.currentNodeType == IDefaultMutableTreeNode.SOURCEFILES
            || this.currentNodeType == IDefaultMutableTreeNode.SOURCEFILES_ARCH
            || this.currentNodeType == IDefaultMutableTreeNode.SOURCEFILES_ARCH_ITEM
            || this.currentNodeType == IDefaultMutableTreeNode.SOURCEFILES_FILENAME
            || this.currentNodeType == IDefaultMutableTreeNode.SOURCEFILES_FILENAME_ITEM) {
            if (!msf.check()) {
                return;
            }
            msf.save();
            msf.setViewMode(true);
            this.xmlsf = msf.getSourceFiles();
        }

        if (this.currentNodeType == IDefaultMutableTreeNode.INCLUDES
            || this.currentNodeType == IDefaultMutableTreeNode.INCLUDES_ARCH
            || this.currentNodeType == IDefaultMutableTreeNode.INCLUDES_ARCH_ITEM
            || this.currentNodeType == IDefaultMutableTreeNode.INCLUDES_PACKAGENAME
            || this.currentNodeType == IDefaultMutableTreeNode.INCLUDES_PACKAGENAME_ITEM) {
            if (!mic.check()) {
                return;
            }
            mic.save();
            mic.setViewMode(true);
            this.xmlic = mic.getIncludes();
        }

        if (this.currentNodeType == IDefaultMutableTreeNode.PROTOCOLS
            || this.currentNodeType == IDefaultMutableTreeNode.PROTOCOLS_PROTOCOL
            || this.currentNodeType == IDefaultMutableTreeNode.PROTOCOLS_PROTOCOL_ITEM
            || this.currentNodeType == IDefaultMutableTreeNode.PROTOCOLS_PROTOCOLNOTIFY
            || this.currentNodeType == IDefaultMutableTreeNode.PROTOCOLS_PROTOCOLNOTIFY_ITEM) {
            if (!mp.check()) {
                return;
            }
            mp.save();
            mp.setViewMode(true);
            this.xmlpl = mp.getProtocols();
        }

        if (this.currentNodeType == IDefaultMutableTreeNode.EVENTS
            || this.currentNodeType == IDefaultMutableTreeNode.EVENTS_CREATEEVENTS
            || this.currentNodeType == IDefaultMutableTreeNode.EVENTS_CREATEEVENTS_ITEM
            || this.currentNodeType == IDefaultMutableTreeNode.EVENTS_SIGNALEVENTS
            || this.currentNodeType == IDefaultMutableTreeNode.EVENTS_SIGNALEVENTS_ITEM) {
            if (!mev.check()) {
                return;
            }
            mev.save();
            mev.setViewMode(true);
            this.xmlen = mev.getEvents();
        }

        if (this.currentNodeType == IDefaultMutableTreeNode.HOBS
            || this.currentNodeType == IDefaultMutableTreeNode.HOBS_HOB_ITEM) {
            if (!mh.check()) {
                return;
            }
            mh.save();
            mh.setViewMode(true);
            this.xmlhob = mh.getHobs();
        }

        if (this.currentNodeType == IDefaultMutableTreeNode.PPIS
            || this.currentNodeType == IDefaultMutableTreeNode.PPIS_PPI
            || this.currentNodeType == IDefaultMutableTreeNode.PPIS_PPI_ITEM
            || this.currentNodeType == IDefaultMutableTreeNode.PPIS_PPINOTIFY
            || this.currentNodeType == IDefaultMutableTreeNode.PPIS_PPINOTIFY_ITEM) {
            if (!mpp.check()) {
                return;
            }
            mpp.save();
            mpp.setViewMode(true);
            this.xmlppi = mpp.getPpis();
        }

        if (this.currentNodeType == IDefaultMutableTreeNode.VARIABLES
            || this.currentNodeType == IDefaultMutableTreeNode.VARIABLES_VARIABLE_ITEM) {
            if (!mv.check()) {
                return;
            }
            mv.save();
            mv.setViewMode(true);
            this.xmlvb = mv.getVariables();
        }

        if (this.currentNodeType == IDefaultMutableTreeNode.BOOTMODES
            || this.currentNodeType == IDefaultMutableTreeNode.BOOTMODES_BOOTMODE_ITEM) {
            if (!mbm.check()) {
                return;
            }
            mbm.save();
            mbm.setViewMode(true);
            this.xmlbm = mbm.getBootModes();
        }

        if (this.currentNodeType == IDefaultMutableTreeNode.SYSTEMTABLES
            || this.currentNodeType == IDefaultMutableTreeNode.SYSTEMTABLES_SYSTEMTABLE_ITEM) {
            if (!mst.check()) {
                return;
            }
            mst.save();
            mst.setViewMode(true);
            this.xmlst = mst.getSystemTables();
        }

        if (this.currentNodeType == IDefaultMutableTreeNode.DATAHUBS
            || this.currentNodeType == IDefaultMutableTreeNode.DATAHUBS_DATAHUB_ITEM) {
            if (!mdh.check()) {
                return;
            }
            mdh.save();
            mdh.setViewMode(true);
            this.xmldh = mdh.getDataHubs();
        }

        if (this.currentNodeType == IDefaultMutableTreeNode.FORMSETS
            || this.currentNodeType == IDefaultMutableTreeNode.FORMSETS_FORMSET_ITEM) {
            if (!mf.check()) {
                return;
            }
            mf.save();
            mf.setViewMode(true);
            this.xmlfs = mf.getFormsets();
        }

        if (this.currentNodeType == IDefaultMutableTreeNode.GUIDS
            || this.currentNodeType == IDefaultMutableTreeNode.GUIDS_GUIDENTRY_ITEM) {
            if (!mg.check()) {
                return;
            }
            mg.save();
            mg.setViewMode(true);
            this.xmlgu = mg.getGuids();
        }

        if (this.currentNodeType == IDefaultMutableTreeNode.EXTERNS
            || this.currentNodeType == IDefaultMutableTreeNode.EXTERNS_EXTERN_ITEM) {
            if (!met.check()) {
                return;
            }
            met.save();
            met.setViewMode(true);
            this.xmlet = met.getExterns();
        }

        if (this.currentNodeType == IDefaultMutableTreeNode.PCDS
            || this.currentNodeType == IDefaultMutableTreeNode.PCDS_PCDDATA_ITEM) {
            if (!mpcd.check()) {
                return;
            }
            mpcd.save();
            mpcd.setViewMode(true);
            this.xmlpcd = mpcd.getPcds();
        }

        reloadTreeAndTable(NEW_WITH_CHANGE);
    }
}
