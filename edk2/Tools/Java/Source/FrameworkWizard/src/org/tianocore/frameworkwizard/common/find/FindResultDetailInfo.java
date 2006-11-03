/** @file

 The file is used to show detailed information of one of find results

 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php

 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 **/
package org.tianocore.frameworkwizard.common.find;

import java.awt.event.ActionEvent;
import java.awt.event.ComponentEvent;
import java.awt.event.WindowEvent;
import java.util.Vector;

import javax.swing.JButton;
import javax.swing.JFrame;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;

import org.tianocore.DynamicPcdBuildDefinitionsDocument.DynamicPcdBuildDefinitions;
import org.tianocore.DynamicPcdBuildDefinitionsDocument.DynamicPcdBuildDefinitions.PcdBuildData.SkuInfo;
import org.tianocore.DynamicPcdBuildDefinitionsDocument.DynamicPcdBuildDefinitions.PcdBuildData;
import org.tianocore.FrameworkModulesDocument.FrameworkModules;
import org.tianocore.LibrariesDocument.Libraries;
import org.tianocore.LibrariesDocument.Libraries.Instance;
import org.tianocore.LibraryClassDocument.LibraryClass;
import org.tianocore.ModuleSADocument.ModuleSA;
import org.tianocore.ModuleSurfaceAreaDocument.ModuleSurfaceArea;
import org.tianocore.PackageSurfaceAreaDocument.PackageSurfaceArea;
import org.tianocore.PcdBuildDefinitionDocument.PcdBuildDefinition;
import org.tianocore.PcdBuildDefinitionDocument.PcdBuildDefinition.PcdData;
import org.tianocore.PcdCodedDocument.PcdCoded;
import org.tianocore.PlatformSurfaceAreaDocument.PlatformSurfaceArea;
import org.tianocore.frameworkwizard.common.DataType;
import org.tianocore.frameworkwizard.common.GlobalData;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.ui.IFrame;
import org.tianocore.frameworkwizard.module.Identifications.ModuleIdentification;
import org.tianocore.frameworkwizard.platform.PlatformIdentification;
import org.tianocore.frameworkwizard.workspace.Workspace;
import org.tianocore.frameworkwizard.workspace.WorkspaceTools;

public class FindResultDetailInfo extends IFrame {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = -4888295869041881282L;

    private JPanel jContentPane = null;

    private JScrollPane jScrollPane = null;

    private JTextArea jTextArea = null;

    private JButton jButtonClose = null;

    //
    // Not for UI
    //
    private static String TAB = "  ";

    private String reportContent = "";

    private WorkspaceTools wt = new WorkspaceTools();

    /**
     * This method initializes jScrollPane	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPane() {
        if (jScrollPane == null) {
            jScrollPane = new JScrollPane();
            jScrollPane.setBounds(new java.awt.Rectangle(5, 5, 535, 280));
            jScrollPane.setPreferredSize(new java.awt.Dimension(535, 280));
            jScrollPane.setViewportView(getJTextArea());
        }
        return jScrollPane;
    }

    /**
     * This method initializes jTextArea	
     * 	
     * @return javax.swing.JTextArea	
     */
    private JTextArea getJTextArea() {
        if (jTextArea == null) {
            jTextArea = new JTextArea();
            jTextArea.setEditable(false);
        }
        return jTextArea;
    }

    /**
     * This method initializes jButtonClose	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonClose() {
        if (jButtonClose == null) {
            jButtonClose = new JButton();
            jButtonClose.setBounds(new java.awt.Rectangle(240, 290, 80, 20));
            jButtonClose.setPreferredSize(new java.awt.Dimension(80, 20));
            jButtonClose.addActionListener(this);
            jButtonClose.setText("Close");
        }
        return jButtonClose;
    }

    /**
     
     @param lci
     
     **/
    public FindResultDetailInfo(FindResultId frid) {
        super();
        init();
        this.setTitle(frid.getName());
        this.jTextArea.setText(createReport(frid));
        this.jTextArea.setSelectionStart(0);
        this.jTextArea.setSelectionEnd(0);
    }

    /**
     
     @param lci
     
     **/
    public FindResultDetailInfo(PcdFindResultId frid) {
        super();
        init();
        this.setTitle(frid.getName());
        this.jTextArea.setText(createReport(frid));
        this.jTextArea.setSelectionStart(0);
        this.jTextArea.setSelectionEnd(0);
    }

    /**
     Create detailed information report for pcd
     
     @param frid
     @return
     
     **/
    private String createReport(PcdFindResultId frid) {
        String name = frid.getName();

        //
        // Write Pcd Name
        //
        writeReportLn("PCD C Name: " + name);

        //
        // Wrtie Declared package
        //
        writeReportLn("Declared in Package: " + frid.getDeclaredBy().getName() + " ("
                      + Tools.getRelativePath(frid.getDeclaredBy().getPath(), Workspace.getCurrentWorkspace()) + ")");

        //
        // Write Token Space
        //
        writeReportLn("Token Space: " + frid.getTokenSpaceGuidCName());

        //
        // Write Token
        //
        writeReportLn("Token: " + frid.getToken());

        //
        // Write Datum Type
        //
        writeReportLn("Datum Type: " + frid.getDatumType());

        //
        // Write Default Value
        //
        writeReportLn("Default Value: " + frid.getValue());

        //
        // Write Usages
        //
        writeReportLn("Valid Usages: " + frid.getUsage());

        //
        // Write Help Text
        //
        writeReportLn("Help Text: ");
        writeReportLn(TAB + frid.getHelp());

        //
        // Write an Empty Line
        //
        writeReportLn("");

        //
        // Wriet all modules which use this PCD:
        //
        writeReportLn("Modules Coded to Use This PCD: ");

        Vector<ModuleIdentification> vModules = frid.getConsumedModules();
        if (vModules != null) {
            for (int index = 0; index < vModules.size(); index++) {
                //
                // Write Module Name and Path
                //
                writeReportLn(TAB + vModules.get(index).getName() + " ("
                              + Tools.getRelativePath(vModules.get(index).getPath(), Workspace.getCurrentWorkspace())
                              + ")");

                //
                // Write Module Pcd Info
                //
                ModuleSurfaceArea msa = GlobalData.openingModuleList.getModuleSurfaceAreaFromId(vModules.get(index));
                if (msa != null) {
                    PcdCoded pcdCoded = msa.getPcdCoded();
                    if (pcdCoded != null) {
                        for (int indexOfPcd = 0; indexOfPcd < pcdCoded.getPcdEntryList().size(); indexOfPcd++) {
                            if (pcdCoded.getPcdEntryList().get(indexOfPcd).getCName().equals(name)) {
                                //
                                // Write Pcd Item Type
                                //
                                writeReportLn(TAB + TAB + "PcdItemType: "
                                              + pcdCoded.getPcdEntryList().get(indexOfPcd).getPcdItemType().toString());

                                //
                                // Write Help Text
                                //
                                writeReportLn(TAB + TAB + "Help Text: ");
                                writeReportLn(TAB + TAB + TAB
                                              + pcdCoded.getPcdEntryList().get(indexOfPcd).getHelpText());
                            }
                        }
                    }
                }

                //
                // Write an Empty Line
                //
                writeReportLn("");
            }
        }

        //
        // Write an Empty Line
        //
        writeReportLn("");

        //
        // Write All Platforms Specifing this PCD
        //
        writeReportLn("Platforms Specifing this PCD: ");

        for (int index = 0; index < GlobalData.openingPlatformList.size(); index++) {
            PlatformSurfaceArea fpd = GlobalData.openingPlatformList.getOpeningPlatformByIndex(index).getXmlFpd();
            PlatformIdentification pid = GlobalData.openingPlatformList.getOpeningPlatformByIndex(index).getId();

            String tmp = "";
            //
            // Get Non-Dynamic Pcd
            //
            FrameworkModules fm = fpd.getFrameworkModules();
            if (fm != null) {
                for (int indexOfModuleSa = 0; indexOfModuleSa < fm.getModuleSAList().size(); indexOfModuleSa++) {
                    ModuleSA msa = fm.getModuleSAList().get(indexOfModuleSa);
                    if (msa != null) {
                        PcdBuildDefinition p = msa.getPcdBuildDefinition();
                        if (p != null) {
                            if (p.getPcdDataList() != null) {

                                for (int indexOfPcd = 0; indexOfPcd < p.getPcdDataList().size(); indexOfPcd++) {
                                    PcdData pd = p.getPcdDataList().get(indexOfPcd);
                                    //
                                    // Find this PCD
                                    //
                                    if (pd.getCName().equals(name)) {
                                        //
                                        // Write Module Sa Info
                                        //
                                        ModuleIdentification moduleSaId = GlobalData
                                                                                    .findModuleId(
                                                                                                  msa.getModuleGuid(),
                                                                                                  msa
                                                                                                     .getModuleVersion(),
                                                                                                  msa.getPackageGuid(),
                                                                                                  msa
                                                                                                     .getPackageVersion());
                                        tmp = tmp
                                              + TAB
                                              + TAB
                                              + "Module: "
                                              + moduleSaId.getName()
                                              + " ("
                                              + Tools.getRelativePath(moduleSaId.getPath(),
                                                                      Workspace.getCurrentWorkspace()) + ")"
                                              + DataType.UNIX_LINE_SEPARATOR;
                                        tmp = tmp + TAB + TAB + TAB + "Implementation: " + pd.getItemType().toString()
                                              + DataType.UNIX_LINE_SEPARATOR;
                                        tmp = tmp + TAB + TAB + TAB + "Specified / Implementation Value: "
                                              + pd.getValue() + DataType.UNIX_LINE_SEPARATOR;
                                        tmp = tmp + DataType.UNIX_LINE_SEPARATOR;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            //
            // Get Dynamic Pcd
            //
            DynamicPcdBuildDefinitions dpbd = fpd.getDynamicPcdBuildDefinitions();
            if (dpbd != null) {
                for (int indexOfDpbd = 0; indexOfDpbd < dpbd.getPcdBuildDataList().size(); indexOfDpbd++) {
                    PcdBuildData pbd = dpbd.getPcdBuildDataList().get(indexOfDpbd);
                    if (pbd != null) {
                        if (pbd.getCName().equals(name)) {
                            //
                            // Write Dynamic Pcd Build Definition
                            //
                            tmp = tmp + TAB + TAB + "Dynamic Pcd Build Definition: " + DataType.UNIX_LINE_SEPARATOR;
                            if (pbd.getSkuInfoList() != null) {
                                for (int indexOfPcd = 0; indexOfPcd < pbd.getSkuInfoList().size(); indexOfPcd++) {
                                    SkuInfo si = pbd.getSkuInfoList().get(indexOfPcd);
                                    if (si != null) {
                                        tmp = tmp + TAB + TAB + TAB + "Sku Id: " + si.getSkuId().toString()
                                              + DataType.UNIX_LINE_SEPARATOR;
                                        tmp = tmp + TAB + TAB + TAB + "Variable Name: " + si.getVariableName()
                                              + DataType.UNIX_LINE_SEPARATOR;
                                        tmp = tmp + TAB + TAB + TAB + "Variable GUID: " + si.getVariableGuid()
                                              + DataType.UNIX_LINE_SEPARATOR;
                                        tmp = tmp + TAB + TAB + TAB + "Variable Offset: " + si.getVariableOffset()
                                              + DataType.UNIX_LINE_SEPARATOR;
                                        tmp = tmp + TAB + TAB + TAB + "Hii Default Value: " + si.getHiiDefaultValue()
                                              + DataType.UNIX_LINE_SEPARATOR;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            //
            // If not empty, write this platform info
            //
            if (!Tools.isEmpty(tmp)) {
                tmp = TAB + "Platform: " + pid.getName() + " ("
                      + Tools.getRelativePath(pid.getPath(), Workspace.getCurrentWorkspace()) + ")"
                      + DataType.UNIX_LINE_SEPARATOR + tmp;
                this.writeReportLn(tmp);
            }
        }

        return reportContent;
    }

    //
    // Create detailed information report for library
    // 
    /**
     
     @param frid
     @return
     
     **/
    private String createReport(FindResultId frid) {
        String tmp = "";
        String name = frid.getName();

        //
        // Write Class Name
        //
        writeReportLn(name);

        //
        // Write Provided Package
        //
        writeReportLn(TAB + "Provided by: " + frid.getDeclaredBy().getName() + " ("
                      + Tools.getRelativePath(frid.getDeclaredBy().getPath(), Workspace.getCurrentWorkspace()) + ")");

        //
        // Write Header File
        //
        PackageSurfaceArea spd = GlobalData.openingPackageList.getPackageSurfaceAreaFromId(frid.getDeclaredBy());
        tmp = Tools.getRelativePath(frid.getDeclaredBy().getPath(), Workspace.getCurrentWorkspace());
        writeReportLn(TAB + "Header File: " + Tools.getFilePathOnly(tmp)
                      + wt.getHeaderFileFromPackageByLibraryClassName(spd, name));

        //
        // Write Supported Module Types
        //
        writeReportLn(TAB + "Supported Module Types: " + Tools.convertVectorToString(frid.getModuleType()));

        //
        // Write Supported Arch
        //
        writeReportLn(TAB + "Supported Architectures: " + Tools.convertVectorToString(frid.getArch()));

        //
        // Write Help Text
        //
        writeReportLn(TAB + "Help Text: ");
        writeReportLn(TAB + TAB + frid.getHelp());

        //
        // Write an empty line
        //
        writeReportLn("");

        //
        // Write Instances
        //
        writeReportLn("Library Instances:");

        //
        // Write Instances One by One
        //
        for (int index = 0; index < frid.getProducedModules().size(); index++) {
            ModuleIdentification mid = frid.getProducedModules().get(index);
            ModuleSurfaceArea msa = GlobalData.openingModuleList.getModuleSurfaceAreaFromId(mid);
            if (msa != null) {
                //
                // Write Instance Name
                //
                if (msa.getMsaHeader() != null) {
                    writeReportLn(TAB + msa.getMsaHeader().getModuleName());
                }

                //
                // Write Msa File Name
                //
                writeReportLn(TAB + TAB + "Provided by: "
                              + Tools.getRelativePath(mid.getPath(), Workspace.getCurrentWorkspace()));

                //
                // Find the produced library class
                //
                if (msa.getLibraryClassDefinitions() != null) {
                    for (int indexL = 0; indexL < msa.getLibraryClassDefinitions().getLibraryClassList().size(); indexL++) {
                        LibraryClass lc = msa.getLibraryClassDefinitions().getLibraryClassList().get(indexL);
                        if (lc.getKeyword().equals(name)) {
                            //
                            // Write Supported Module Types
                            //
                            writeReportLn(TAB + TAB + "Supported Module Types: "
                                          + Tools.convertListToString(lc.getSupModuleList()));

                            //
                            // Write Supported Arch
                            //
                            writeReportLn(TAB + TAB + "Supported Architectures: "
                                          + Tools.convertListToString(lc.getSupArchList()));

                            //
                            // Write Help Text
                            //
                            writeReportLn(TAB + TAB + "Help Text: ");
                            writeReportLn(TAB + TAB + (lc.getHelpText() == null ? "" : lc.getHelpText()));
                        }
                    }
                }

            }
        }

        //
        // Write an empty line
        //
        writeReportLn("");

        //
        // Write all modules which consumed this library
        //
        writeReportLn("Modules Requiring " + name + ":");
        for (int index = 0; index < frid.getConsumedModules().size(); index++) {
            //
            // Write
            //
            writeReportLn(TAB
                          + frid.getConsumedModules().get(index).getName()
                          + " ("
                          + Tools.getRelativePath(frid.getConsumedModules().get(index).getPath(),
                                                  Workspace.getCurrentWorkspace()) + ")");
        }

        //
        // Write an empty line
        //
        writeReportLn("");

        //
        // Write platforms using the library class instances
        //
        writeReportLn("Platforms Using " + name + ":");

        //
        // Write Instances One by One as Using Platforms
        //
        for (int index = 0; index < frid.getProducedModules().size(); index++) {
            ModuleIdentification mid = frid.getProducedModules().get(index);

            //
            // Write Instance Name 
            //
            writeReportLn(TAB + mid.getName());

            //
            // Go through each platform one by one
            //
            for (int indexOfPlatform = 0; indexOfPlatform < GlobalData.openingPlatformList.size(); indexOfPlatform++) {
                PlatformSurfaceArea fpd = GlobalData.openingPlatformList.getOpeningPlatformByIndex(indexOfPlatform)
                                                                        .getXmlFpd();
                PlatformIdentification pid = GlobalData.openingPlatformList.getOpeningPlatformByIndex(indexOfPlatform)
                                                                           .getId();
                Vector<ModuleIdentification> vModuleSa = new Vector<ModuleIdentification>();
                if (fpd != null) {
                    FrameworkModules fm = fpd.getFrameworkModules();
                    if (fm != null) {
                        for (int indexOfModule = 0; indexOfModule < fm.getModuleSAList().size(); indexOfModule++) {
                            ModuleSA msa = fm.getModuleSAList().get(indexOfModule);
                            if (msa != null) {
                                Libraries l = msa.getLibraries();
                                if (l != null) {
                                    if (l.getInstanceList() != null) {
                                        for (int indexOfInstance = 0; indexOfInstance < l.getInstanceList().size(); indexOfInstance++) {
                                            Instance i = l.getInstanceList().get(indexOfInstance);
                                            if (mid.equals(i.getModuleGuid(), i.getModuleVersion(), i.getPackageGuid(),
                                                           i.getPackageVersion())) {
                                                ModuleIdentification moduleSaId = GlobalData
                                                                                            .findModuleId(
                                                                                                          msa
                                                                                                             .getModuleGuid(),
                                                                                                          msa
                                                                                                             .getModuleVersion(),
                                                                                                          msa
                                                                                                             .getPackageGuid(),
                                                                                                          msa
                                                                                                             .getPackageVersion());
                                                if (moduleSaId != null) {
                                                    vModuleSa.addElement(moduleSaId);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        //
                        // Get finded moduleSa in this spd
                        //
                        if (vModuleSa.size() > 0) {
                            writeReportLn(TAB + TAB + pid.getName() + " ("
                                          + Tools.getRelativePath(pid.getPath(), Workspace.getCurrentWorkspace()) + ")");
                            for (int indexOfModuleSa = 0; indexOfModuleSa < vModuleSa.size(); indexOfModuleSa++) {
                                writeReportLn(TAB
                                              + TAB
                                              + TAB
                                              + vModuleSa.elementAt(indexOfModuleSa).getName()
                                              + " ("
                                              + Tools.getRelativePath(vModuleSa.elementAt(indexOfModuleSa).getPath(),
                                                                      Workspace.getCurrentWorkspace()) + ")");
                            }
                        }
                    }
                }
            }
            writeReportLn("");
        }

        return this.reportContent;
    }

    /**
     * This method initializes this
     * 
     * @return void
     */
    private void init() {
        this.setSize(550, 380);
        this.setContentPane(getJContentPane());
        this.setTitle("JFrame");
        this.setResizable(true);
        this.setDefaultCloseOperation(JFrame.HIDE_ON_CLOSE);
        this.centerWindow();

    }

    /**
     * This method initializes jContentPane
     * 
     * @return javax.swing.JPanel
     */
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.setSize(new java.awt.Dimension(550, 350));
            jContentPane.add(getJScrollPane(), null);
            jContentPane.add(getJButtonClose(), null);

            jContentPane.setPreferredSize(new java.awt.Dimension(550, 340));
        }
        return jContentPane;
    }

    /* (non-Javadoc)
     * @see java.awt.event.WindowListener#windowClosing(java.awt.event.WindowEvent)
     *
     * Override windowClosing to popup warning message to confirm quit
     * 
     */
    public void windowClosing(WindowEvent arg0) {
        this.dispose();
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

        Tools.resizeComponent(this.jScrollPane, intCurrentWidth, intCurrentHeight, intPreferredWidth,
                              intPreferredHeight);
        Tools.centerComponent(this.jButtonClose, intCurrentWidth, intCurrentHeight, intPreferredHeight,
                              DataType.SPACE_TO_BOTTOM_FOR_CLOSE_BUTTON);
    }

    /* (non-Javadoc)
     * @see java.awt.event.ActionListener#actionPerformed(java.awt.event.ActionEvent)
     *
     */
    public void actionPerformed(ActionEvent arg0) {
        if (arg0.getSource() == this.jButtonClose) {
            this.dispose();
        }
    }

    private void writeReportLn(String line) {
        this.reportContent = this.reportContent + line + DataType.LINE_SEPARATOR;
    }
}
