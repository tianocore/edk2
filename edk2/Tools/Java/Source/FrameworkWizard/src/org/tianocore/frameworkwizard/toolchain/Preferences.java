/** @file
 <<The file is used to update the Build Preferences file, target.txt>>
 
 <<The program will use target.txt, the tools config file specified in that file,
 or it will use the default tools_def.txt file, and it will also scan the 
 FrameworkDatabase.db file for certain parameters. >>
 
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

package org.tianocore.frameworkwizard.toolchain;

import java.awt.event.ActionEvent;
import java.io.*;
import java.util.Vector;
import java.util.Iterator;
import java.util.Scanner;

import javax.swing.*;
import javax.swing.JScrollPane;
import javax.swing.JTextField;

import org.tianocore.frameworkwizard.common.GlobalData;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.Log;
import org.tianocore.frameworkwizard.common.ui.ArchCheckBox;
import org.tianocore.frameworkwizard.common.ui.iCheckBoxList.*;
import org.tianocore.frameworkwizard.common.ui.IFrame;
import org.tianocore.frameworkwizard.workspace.Workspace;
import org.tianocore.frameworkwizard.workspace.WorkspaceTools;
import org.tianocore.frameworkwizard.platform.PlatformIdentification;
import org.tianocore.PlatformSurfaceAreaDocument;

/**
 * The class is used to update the target.txt file.
 * 
 * It extends IDialog
 * 
 */
public class Preferences extends IFrame {

    // /
    // / Define class Serial Version UID
    // /
    private static final long serialVersionUID = -4777906991966638888L;

    private final boolean Debug = false;

    //
    // Define class members
    //
    private final int oneRowHeight = 20;

    private final int twoRowHeight = 40;

    private final int threeRowHeight = 60;

    private final int sepHeight = 6;

    private final int rowOne = 12;

    private final int rowTwo = rowOne + oneRowHeight + sepHeight;

    private final int rowThree = rowTwo + oneRowHeight + sepHeight;

    private final int rowFour = rowThree + threeRowHeight + sepHeight;

    private final int rowFive = rowFour + threeRowHeight + sepHeight;

    private final int rowSix = rowFive + oneRowHeight + sepHeight;

    private final int buttonRow = rowSix + oneRowHeight + sepHeight + sepHeight;

    private final int dialogHeight = buttonRow + twoRowHeight + twoRowHeight;

    private final int dialogWidth = 540;

    private final int lastButtonXLoc = 430;

    private final int next2LastButtonLoc = 329;

    /*
     * Define the contents for this dialog box
     */
    private static Preferences bTarget = null;

    private WorkspaceTools wt = new WorkspaceTools();

    private final int activePlatformId = 0;

    private final int buildTargetId = 1;

    private final int targetArchId = 2;

    private final int toolDefFileId = 3;

    private final int tagNameId = 4;

    private final int threadEnableId = 5;

    private final int threadCountId = 6;

    private final int maxTargetLines = threadCountId + 1;

    private JPanel jContentPane = null;

    private JLabel jLabelToolsConfigFile = null;

    private JTextField jTextFieldToolsConfigFile = null;

    private final int toolConfigFileRow = rowOne;

    private JLabel jLabelActivePlatform = null;

    private JComboBox jComboBoxActivePlatform = null;

    private final int activePlatformRow = rowTwo;

    private JLabel jLabelToolChainTagName = null;

    private JScrollPane jScrollPaneTagName = null;

    private ICheckBoxList iCheckBoxListTagName = null;

    private final int toolChainTagNameRow = rowThree;

    private JLabel jLabelBuildTarget = null;

    private JScrollPane jScrollPaneBuildTarget = null;

    private ICheckBoxList iCheckBoxListBuildTarget = null;

    private final int buildTargetRow = rowFour;

    private JLabel jLabelTargetArch = null;

    private ArchCheckBox jArchCheckBox = null;

    private final int targetArchRow = rowFive;

    private JLabel jLabelEnableThreads = null;

    private JLabel jLabelThreadCount = null;

    private final int threadRow = rowSix;

    private JCheckBox jCheckBoxEnableThreads = null;

    private JTextField jTextFieldThreadCount = null;
    
    private String threadCount = "";
    
    private boolean threadEnabled = false;

    private JButton jButtonBrowse = null;

    private JButton jButtonSave = null;

    private JButton jButtonCancel = null;

    private final int labelColumn = 12;

    private final int labelWidth = 155;

    private final int valueColumn = 168;

    private final int valueWidth = 352;

    private final int valueWidthShort = 260;

    private final int buttonWidth = 90;

    private String workspaceDir = Workspace.getCurrentWorkspace() + System.getProperty("file.separator");

    private String toolsDir = Workspace.getCurrentWorkspace() + System.getProperty("file.separator") + "Tools"
                              + System.getProperty("file.separator") + "Conf";

    private String defaultToolsConf = toolsDir + System.getProperty("file.separator") + "tools_def.txt";

    private String targetFile = toolsDir + System.getProperty("file.separator") + "target.txt";

    private String[] targetFileContents = new String[500];

    // private String[] toolsConfContents;

    private String[] targetLines = new String[maxTargetLines];

    private int targetLineNumber[] = new int[maxTargetLines];

    private String toolsConfFile;

    private String toolsDefTargetNames = null;

    private final int toolsDefTargetNameField = 0;

    private String toolsDefTagNames = null;

    private final int toolsDefTagNameField = 1;

    private String toolsDefArchNames = null;

    private final int toolsDefArchNameField = 2;

    private String toolsDefIdentifier = null;

    private int targetLineNumberMax;
    
    private final int toolDefFieldCount = 5;

    private Vector<String> vArchList = null;

    private Vector<String> vDisableArchList = null;
    
    //
    // Not used by UI
    //
    //    private Preferences id = null;

    //    private EnumerationData ed = new EnumerationData();

    /**
     This method initializes jTextFieldToolsConfigFile  
     
     @return javax.swing.JTextField  jTextFieldToolsConfigFile
     **/
    private JTextField getJTextFieldToolsConfigFile() {
        if (jTextFieldToolsConfigFile == null) {
            if (targetLines[toolDefFileId] != null) {
                String sLine[] = targetLines[toolDefFileId].trim().split("=");
                jTextFieldToolsConfigFile = new JTextField(sLine[1].trim());
            } else
                jTextFieldToolsConfigFile = new JTextField();

            jTextFieldToolsConfigFile.setBounds(new java.awt.Rectangle(valueColumn, toolConfigFileRow, valueWidthShort,
                                                                       oneRowHeight));
            jTextFieldToolsConfigFile.setPreferredSize(new java.awt.Dimension(valueWidthShort, oneRowHeight));
            jTextFieldToolsConfigFile
                                     .setToolTipText("<html>"
                                                     + "Specify the name of the filename to use for specifying"
                                                     + "<br>the tools to use for the build.  If not specified,"
                                                     + "<br>tools_def.txt will be used for the build.  This file"
                                                     + "<br>MUST be located in the WORKSPACE/Tools/Conf directory.</html>");

        }
        return jTextFieldToolsConfigFile;
    }

    /**
     * This method initializes jComboBoxActivePlatform
     * 
     * @return javax.swing.JComboBox jComboBoxActivePlatform
     * 
     */
    private JComboBox getActivePlatform() {
        Vector<PlatformIdentification> vPlatformId = wt.getAllPlatforms();

        if (jComboBoxActivePlatform == null) {
            jComboBoxActivePlatform = new JComboBox();
            jComboBoxActivePlatform.setBounds(new java.awt.Rectangle(valueColumn, activePlatformRow, valueWidth,
                                                                     oneRowHeight));
            jComboBoxActivePlatform.setPreferredSize(new java.awt.Dimension(valueWidth, oneRowHeight));
            jComboBoxActivePlatform
                                   .setToolTipText("<html>Select &quot;Do Not Set&quot; if you want to build a platform"
                                                   + "<br>from the directory where the FPD file exists,"
                                                   + "<br>otherwise scroll down to select the platform.</html>");

            /*
             * Generate the data, selecting what is in target.txt
             */
            jComboBoxActivePlatform.addItem("Do Not Set");
            Iterator<PlatformIdentification> iter = vPlatformId.iterator();
            while (iter.hasNext()) {
                PlatformIdentification item = iter.next();
                String path = item.getPath().trim();
                String str = path.substring(workspaceDir.length(), path.length());
                str.replace(System.getProperty("file.separator"), "/");
                jComboBoxActivePlatform.addItem(str.trim());
            }
            if (targetLines[activePlatformId] == null)
                jComboBoxActivePlatform.setSelectedItem("Do Not Set");
            else
                jComboBoxActivePlatform.setSelectedItem(targetLines[activePlatformId]);
        }
        return jComboBoxActivePlatform;
    }

    /**
     * This method initializes jScrollPaneTagName
     * 
     * @return javax.swing.JScrollPane jScrollPaneTagName
     * 
     */
    private JScrollPane getJScrollPaneTagName() {

        if (jScrollPaneTagName == null) {
            jScrollPaneTagName = new JScrollPane();
            jScrollPaneTagName.setBounds(new java.awt.Rectangle(valueColumn, toolChainTagNameRow, valueWidth,
                                                                threeRowHeight));
            jScrollPaneTagName.setPreferredSize(new java.awt.Dimension(valueWidth, threeRowHeight));
            jScrollPaneTagName.setHorizontalScrollBarPolicy(JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);
            jScrollPaneTagName.setVerticalScrollBarPolicy(JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED);
            jScrollPaneTagName.setViewportView(getICheckBoxListTagName());
            jScrollPaneTagName.setToolTipText("<html>"
                                              + "Specify the TagName(s) from the tool configuration file to use"
                                              + "<br>for your builds.  If not specified, all applicable TagName"
                                              + " <br>tools will be used for the build.</html>");
            jScrollPaneTagName.setVisible(true);

        }
        return jScrollPaneTagName;
    }

    private ICheckBoxList getICheckBoxListTagName() {
        if (iCheckBoxListTagName == null) {
            iCheckBoxListTagName = new ICheckBoxList();

            if (toolsDefTagNames != null) {
                toolsDefTagNames.trim();
                String aTagNames[] = toolsDefTagNames.trim().split(" ");
                Vector<String> vTags = new Vector<String>();
                for (int i = 0; i < aTagNames.length; i++) {
                    vTags.add(aTagNames[i]);
                }
                iCheckBoxListTagName.setAllItems(vTags);
            } else {
                Vector<String> defaultTags = stringToVector("MYTOOLS");
                iCheckBoxListTagName.setAllItems(defaultTags);
            }

            iCheckBoxListTagName.setAllItemsUnchecked();
            iCheckBoxListTagName.setToolTipText("<html>"
                                                + "Specify the TagName(s) from the tool configuration file to use"
                                                + "<br>for your builds.  If not specified, all applicable TagName"
                                                + " <br>tools will be used for the build.</html>");
            Vector<String> vSelectedTags = new Vector<String>();
            if (targetLines[tagNameId] != null) {
                targetLines[tagNameId].trim();
                String targetTags[] = targetLines[tagNameId].trim().split(" ");
                for (int j = 0; j < targetTags.length; j++)
                    vSelectedTags.add(targetTags[j]);
                iCheckBoxListTagName.initCheckedItem(true, vSelectedTags);
            }
        }
        return iCheckBoxListTagName;
    }

    /**
     * This method initializes jScrollPaneBuildTarget
     * 
     * @return javax.swing.JComboBox jScrollPaneBuildTarget
     * 
     */
    private JScrollPane getJScrollPaneBuildTarget() {
        if (jScrollPaneBuildTarget == null) {
            jScrollPaneBuildTarget = new JScrollPane();
            jScrollPaneBuildTarget.setBounds(new java.awt.Rectangle(valueColumn, buildTargetRow, valueWidth,
                                                                    threeRowHeight));
            jScrollPaneBuildTarget.setPreferredSize(new java.awt.Dimension(valueWidth, threeRowHeight));
            jScrollPaneBuildTarget.setHorizontalScrollBarPolicy(JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);
            jScrollPaneBuildTarget.setVerticalScrollBarPolicy(JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED);
            jScrollPaneBuildTarget.setViewportView(getICheckBoxListBuildTarget());
            jScrollPaneBuildTarget.setVisible(true);
            jScrollPaneBuildTarget.setToolTipText("<html>"
                                                  + "Select the TARGET Names that you want to build, such as<BR>"
                                                  + "BUILD or BUILD and RELEASE"
                                                  + "<br>If you do not set any of these, all available targets"
                                                  + "<br>will be built.</html>");

        }
        return jScrollPaneBuildTarget;
    }

    private JCheckBox getCheckBoxEnableThreads() {
        if (jCheckBoxEnableThreads == null) {
            jCheckBoxEnableThreads = new JCheckBox();
            jCheckBoxEnableThreads.setBounds(valueColumn, threadRow, 20, oneRowHeight);
            jCheckBoxEnableThreads.setToolTipText("Select this if you want to enable parallel compilation.");
            jCheckBoxEnableThreads.setSelected(threadEnabled);
            jCheckBoxEnableThreads.addActionListener(this);
            
        }
        return jCheckBoxEnableThreads;
    }

    private JTextField getTextFieldThreadCount() {
        if (jTextFieldThreadCount == null) {
            jTextFieldThreadCount = new JTextField();
            jTextFieldThreadCount.setBounds(valueColumn + 215, threadRow, 30, oneRowHeight);
            if (threadCount.length() > 0)
                jTextFieldThreadCount.setText(threadCount);
            jTextFieldThreadCount.setToolTipText("<html>Recommended setting is N+1,<br> where N is the number of physical processors or cores in the system</html>");
            // If CheckBoxEnableThreads is selected, then enable editing

        }
        return jTextFieldThreadCount;
    }

    private ICheckBoxList getICheckBoxListBuildTarget() {
        if (iCheckBoxListBuildTarget == null) {

            String aBuildTargets[] = toolsDefTargetNames.trim().split(" ");
            Vector<String> vBuildTargets = new Vector<String>();
            for (int i = 0; i < aBuildTargets.length; i++) {
                vBuildTargets.add(aBuildTargets[i]);
            }
            iCheckBoxListBuildTarget = new ICheckBoxList();
            iCheckBoxListBuildTarget.setAllItems(vBuildTargets);
            iCheckBoxListBuildTarget.setAllItemsUnchecked();
            iCheckBoxListBuildTarget.setToolTipText("<html>"
                                                    + "Select the TARGET Names that you want to build, such as<BR>"
                                                    + "BUILD or BUILD and RELEASE"
                                                    + "<br>If you do not set any of these, all available targets"
                                                    + "<br>will be built.</html>");

            Vector<String> vSelectedTags = new Vector<String>();
            if (targetLines[buildTargetId] != null) {
                targetLines[buildTargetId].trim();
                String targetTags[] = targetLines[buildTargetId].trim().split(" ");
                for (int j = 0; j < targetTags.length; j++)
                    vSelectedTags.add(targetTags[j]);
                iCheckBoxListBuildTarget.initCheckedItem(true, vSelectedTags);
            }
        }
        return iCheckBoxListBuildTarget;
    }

    /**
     This method initializes jButtonBrowse   
     
     @return javax.swing.JButton 
     **/
    private JButton getJButtonBrowse() {
        if (jButtonBrowse == null) {
            jButtonBrowse = new JButton();
            jButtonBrowse
                         .setBounds(new java.awt.Rectangle(lastButtonXLoc, toolConfigFileRow, buttonWidth, oneRowHeight));
            jButtonBrowse.setText("Browse");
            jButtonBrowse.setPreferredSize(new java.awt.Dimension(buttonWidth, oneRowHeight));
            jButtonBrowse.addActionListener(new AbstractAction() {
                /**
                 * 
                 */
                private static final long serialVersionUID = 1L;

                public void actionPerformed(ActionEvent e) {
                    //
                    // Select files from current workspace
                    //
                    String dirPrefix = toolsDir + System.getProperty("file.separator");
                    JFileChooser chooser = new JFileChooser(dirPrefix);
                    File theFile = null;
                    //                    String headerDest = null;

                    chooser.setMultiSelectionEnabled(false);
                    chooser.setFileSelectionMode(JFileChooser.FILES_ONLY);
                    int retval = chooser.showOpenDialog(Preferences.this);
                    if (retval == JFileChooser.APPROVE_OPTION) {

                        theFile = chooser.getSelectedFile();
                        String file = theFile.getPath();
                        if (!file.startsWith(dirPrefix)) {
                            JOptionPane.showMessageDialog(Preferences.this, "You can only select files in the Tools"
                                                                 + System.getProperty("file.separator")
                                                                 + "Conf directory!");

                            return;
                        }

                        jTextFieldToolsConfigFile.setText("Tools/Conf/" + theFile.getName());
                    } else {
                        return;
                    }
                }
            });
        }
        return jButtonBrowse;
    }

    /**
     * This method initializes jButtonOk
     * 
     * @return javax.swing.JButton
     * 
     */
    private JButton getJButtonSave() {
        if (jButtonSave == null) {
            jButtonSave = new JButton();
            jButtonSave.setBounds(new java.awt.Rectangle(next2LastButtonLoc, buttonRow, buttonWidth, oneRowHeight));
            jButtonSave.setText("Save");
            jButtonSave.addActionListener(this);
        }
        return jButtonSave;
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
            jButtonCancel.setBounds(new java.awt.Rectangle(lastButtonXLoc, buttonRow, buttonWidth, oneRowHeight));
            jButtonCancel.setText("Cancel");
            jButtonCancel.addActionListener(this);
        }
        return jButtonCancel;
    }

    public static void main(String[] args) {
        // TODO Auto-generated method stub

    }

    public static Preferences getInstance() {
        if (bTarget == null) {
            bTarget = new Preferences();
        }
        return bTarget;
    }

    /**
     * This is the default constructor
     */
    public Preferences() {
        super();
        init();
    }

    /**
     * This method initializes this
     * 
     */
    private void init() {
        for (int i = 0; i < maxTargetLines; i++) {
            targetLines[i] = null;
            targetLineNumber[i] = -1;
        }
        initReadFiles();
        this.setSize(dialogWidth, dialogHeight);
        this.setContentPane(getJContentPane());
        this.setTitle("Build Preferences [" + toolsDefIdentifier + "]");
        this.setDefaultCloseOperation(IFrame.DISPOSE_ON_CLOSE);
        this.centerWindow();
        this.setVisible(true);
    }

    /**
     * This method initializes this Fill values to all fields if these values are
     * not empty
     * 
     * @param initReadFiles
     * 
     */
    private void initReadFiles() {
        /*
         * TODO
         * Read Current target.txt file first
         * Read TOOL_CHAIN_CONF file if specified, otherwise use tools_def.txt
         */
        readTargetTxtFile();
        boolean haveBuildTargets = readToolDefinitionFile();
        if (!haveBuildTargets) {
            // Lookup Build Targets from the platforms
            readPlatformFileBuildTargets();
        }
    }

    private void readPlatformFileBuildTargets() {
        Vector<PlatformIdentification> vPlatformId = wt.getAllPlatforms();
        String sBuildTargets = "";

        // foreach platform, build a list of BuildTargets
        Iterator<PlatformIdentification> iter = vPlatformId.iterator();
        while (iter.hasNext()) {
            PlatformIdentification item = iter.next();
            PlatformSurfaceAreaDocument.PlatformSurfaceArea fpd = GlobalData.openingPlatformList
                                                                                                .getOpeningPlatformById(
                                                                                                                        item)
                                                                                                .getXmlFpd();
            sBuildTargets += fpd.getPlatformDefinitions().getBuildTargets().toString() + " ";
        }
        String allTargets[] = sBuildTargets.trim().split(" ");
        for (int i = 0; i < allTargets.length; i++) {
            if (!toolsDefTargetNames.contains(allTargets[i])) {
                toolsDefTargetNames += allTargets[i] + " ";
            }
        }
    }

    private boolean readToolDefinitionFile() {

        // Parse the tool definition file looking for targets and architectures
        toolsConfFile = null;
        boolean buildTargetsExist = true;

        if (targetLines[toolDefFileId] != null) {
            String[] result = new String[2];
            targetLines[toolDefFileId].trim();
            result = (targetLines[toolDefFileId]).split("=");
            String resString = (Tools.convertPathToCurrentOsType(result[1])).trim();
            toolsConfFile = workspaceDir.trim() + resString.trim();
            File toolsDefFile = new File(toolsConfFile);
            if (!toolsDefFile.exists()) {
                JOptionPane.showMessageDialog(this, "<html>" + "Tool Definition file, " + toolsConfFile
                                                    + "<br>specified in the target.txt file does not exist!"
                                                    + "<br>Using the default Tool Definition File:<br>"
                                                    + defaultToolsConf);
                toolsConfFile = defaultToolsConf;
            }
        } else {
            toolsConfFile = defaultToolsConf;
        }
        String[] toolsDefFields = new String[toolDefFieldCount];
        for (int i = 0; i < toolDefFieldCount; i++)
            toolsDefFields[i] = null;
        File toolDefFile = new File(toolsConfFile);
        if (toolDefFile.exists()) {
            try {
                FileReader fileReader = new FileReader(toolDefFile);
                BufferedReader reader = new BufferedReader(fileReader);
                String rLine = null;
                String result[];
                int lineCounter = 0;
                while ((rLine = reader.readLine()) != null) {

                    if (rLine.startsWith("IDENTIFIER")) {
                        result = rLine.split("=");
                        toolsDefIdentifier = (result[1]).trim();
                    } else if ((!rLine.startsWith("#")) && (rLine.contains("="))) {
                        result = rLine.split("=");
                        toolsDefFields = ((result[0]).trim()).split("_");
                        if (toolsDefTargetNames == null) {
                            toolsDefTargetNames = (toolsDefFields[toolsDefTargetNameField]).trim() + " ";
                        } else if (!toolsDefTargetNames.contains((toolsDefFields[toolsDefTargetNameField]).trim())) {
                            toolsDefTargetNames += (toolsDefFields[toolsDefTargetNameField]).trim() + " ";
                        }
                        if (toolsDefTagNames == null) {
                            toolsDefTagNames = (toolsDefFields[toolsDefTagNameField]).trim() + " ";
                        } else if (!toolsDefTagNames.contains((toolsDefFields[toolsDefTagNameField]).trim())) {
                            toolsDefTagNames += (toolsDefFields[toolsDefTagNameField]).trim() + " ";
                        }
                        if (toolsDefArchNames == null) {
                            toolsDefArchNames = (toolsDefFields[toolsDefArchNameField]).trim() + " ";
                        } else if (!toolsDefArchNames.contains((toolsDefFields[toolsDefArchNameField]).trim())) {
                            toolsDefArchNames += (toolsDefFields[toolsDefArchNameField]).trim() + " ";
                        }
                    }
                    lineCounter++;
                }
                reader.close();
                // Only enable Architecture selection based on tool chain installations
                String turnOff = "";
                if (!toolsDefArchNames.contains("EBC"))
                    turnOff = "EBC ";
                if (!toolsDefArchNames.contains("PPC"))
                    turnOff += "PPC ";
                if (!toolsDefArchNames.contains("IPF"))
                    turnOff += "IPF ";
                if (!toolsDefArchNames.contains("X64"))
                    turnOff += "X64 ";
                if (!toolsDefArchNames.contains("IA32"))
                    turnOff += "X64 ";
                if (!toolsDefArchNames.contains("ARM"))
                    turnOff += "ARM ";
                turnOff = turnOff.trim();
                vDisableArchList = stringToVector(turnOff);

                if (!toolsDefTargetNames.matches("[A-Z]+")) {
                    toolsDefTargetNames = toolsDefTargetNames.replace("* ", "").trim();
                    if (Debug)
                        System.out.println("tools_def file does not define build targets: '" + toolsDefTargetNames
                                           + "'");
                    buildTargetsExist = false;
                }
            } catch (IOException e) {
                Log.log(toolsConfFile + " Read Error ", e.getMessage());
                e.printStackTrace();
            }
        }
        return buildTargetsExist;
    }

    private void readTargetTxtFile() {
        File tFile = new File(targetFile);

        if (tFile.exists()) {
            try {
                FileReader fileReader = new FileReader(targetFile);
                BufferedReader reader = new BufferedReader(fileReader);
                targetLineNumberMax = 0;
                String rLine = null;
                while ((rLine = reader.readLine()) != null) {
                    targetFileContents[targetLineNumberMax] = rLine;
                    if (rLine.startsWith("ACTIVE_PLATFORM")) {
                        // Only one active platform is permitted!
                        targetLines[activePlatformId] = rLine;
                        targetLineNumber[activePlatformId] = targetLineNumberMax;
                    }
                    if ((rLine.startsWith("TARGET" + " ")) || (rLine.startsWith("TARGET" + "\t"))
                        || (rLine.startsWith("TARGET="))) {
                        // Handle multiple Target Names
                        if (rLine.contains(","))
                            targetLines[buildTargetId] = rLine.trim().replaceAll(",", " ");
                        else
                            targetLines[buildTargetId] = rLine.trim();
                        targetLineNumber[buildTargetId] = targetLineNumberMax;
                    }
                    if (rLine.startsWith("TARGET_ARCH")) {
                        // Handle multiple Target Architectures
                        if (rLine.contains(","))
                            targetLines[targetArchId] = rLine.trim().replaceAll(",", " ");
                        else
                            targetLines[targetArchId] = rLine.trim();
                        targetLineNumber[targetArchId] = targetLineNumberMax;
                    }
                    if (rLine.startsWith("TOOL_CHAIN_CONF")) {
                        // Only one file is permitted
                        targetLines[toolDefFileId] = rLine.trim();
                        targetLineNumber[toolDefFileId] = targetLineNumberMax;
                    }

                    if (rLine.startsWith("TOOL_CHAIN_TAG")) {
                        // Handle multiple Tool TagNames
                        if (rLine.contains(","))
                            targetLines[tagNameId] = rLine.trim().replaceAll(",", " ");
                        else
                            targetLines[tagNameId] = rLine.trim();
                        targetLineNumber[tagNameId] = targetLineNumberMax;
                    }
                    
                    if (rLine.startsWith("MULTIPLE_THREAD")) {
                        // Handle Thread Enable flag
                        targetLines[threadEnableId] = rLine.trim();
                        targetLineNumber[threadEnableId] = targetLineNumberMax;
                        if ((rLine.trim().toLowerCase().contains("enabled")) || (rLine.trim().toLowerCase().contains("true"))) { 
                            threadEnabled = true;
                        } else {
                            threadEnabled = false;
                        }
                    }
                    
                    if (rLine.startsWith("MAX_CONCURRENT_THREAD_NUMBER")) {
                        // Handle Thread Enable flag
                        targetLines[threadCountId] = rLine.trim();
                        targetLineNumber[threadCountId] = targetLineNumberMax;
                    }
                    targetLineNumberMax++;
                }
                reader.close();
                String archLine[] = new String[2];
                if (targetLines[targetArchId] != null) {
                    if (targetLines[targetArchId].contains("=")) {
                        if (targetLines[targetArchId].contains(","))
                            targetLines[targetArchId] = targetLines[targetArchId].trim().replaceAll(",", " ");
                        if (targetLines[targetArchId].length() > 0)
                            archLine = targetLines[targetArchId].trim().split("=");
                        vArchList = stringToVector(archLine[1]);
                    }
                }

                if (targetLines[threadCountId] != null) {
                    String tcLine[] = new String[2];
                    tcLine = targetLines[threadCountId].trim().split("=");
                    threadCount = tcLine[1];
                } else
                    threadCount = "";
                
                if (Debug == true)
                    for (int i = 0; i <= maxTargetLines; i++)
                        System.out.println("targetLines[" + i + "] contains: " + targetLines[i] + " index is: "
                                           + targetLineNumber[i]);
            } catch (IOException e) {
                Log.log(this.targetFile + " Read Error ", e.getMessage());
                e.printStackTrace();
            }
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
            jLabelToolsConfigFile = new JLabel();
            jLabelToolsConfigFile.setBounds(new java.awt.Rectangle(labelColumn, toolConfigFileRow, labelWidth,
                                                                   oneRowHeight));
            jLabelToolsConfigFile.setText("Tool Chain Definition File");
            jLabelActivePlatform = new JLabel();
            jLabelActivePlatform.setText("Select Active Platform");
            jLabelActivePlatform.setBounds(new java.awt.Rectangle(labelColumn, activePlatformRow, labelWidth,
                                                                  oneRowHeight));
            jLabelToolChainTagName = new JLabel();
            jLabelToolChainTagName.setBounds(new java.awt.Rectangle(labelColumn, toolChainTagNameRow, labelWidth,
                                                                    oneRowHeight));
            jLabelToolChainTagName.setText("Select Tool Tag Name");
            jLabelBuildTarget = new JLabel();
            jLabelBuildTarget.setBounds(new java.awt.Rectangle(labelColumn, buildTargetRow, labelWidth, oneRowHeight));
            jLabelBuildTarget.setText("Select Build Target");
            jLabelTargetArch = new JLabel();
            jLabelTargetArch.setBounds(new java.awt.Rectangle(labelColumn, targetArchRow, labelWidth, oneRowHeight));
            jLabelTargetArch.setText("Build Architectures");

            jArchCheckBox = new ArchCheckBox();
            jArchCheckBox.setBounds(new java.awt.Rectangle(valueColumn, targetArchRow, valueWidth, oneRowHeight));
            jArchCheckBox.setPreferredSize(new java.awt.Dimension(valueWidth, oneRowHeight));

            jLabelEnableThreads = new JLabel();
            jLabelEnableThreads.setBounds(new java.awt.Rectangle(labelColumn, threadRow, labelWidth, oneRowHeight));
            jLabelEnableThreads.setText("Enable Compiler Threading");

            jLabelThreadCount = new JLabel();
            jLabelThreadCount.setBounds(new java.awt.Rectangle(valueColumn + 60, threadRow, labelWidth, oneRowHeight));
            jLabelThreadCount.setText("Number of threads to start");

            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.setPreferredSize(new java.awt.Dimension(dialogWidth - 10, dialogHeight - 10));

            jContentPane.add(jLabelToolsConfigFile, null);
            jContentPane.add(getJTextFieldToolsConfigFile(), null);
            jContentPane.add(getJButtonBrowse(), null);

            jContentPane.add(jLabelActivePlatform, null);
            jContentPane.add(getActivePlatform(), null);

            jContentPane.add(jLabelToolChainTagName, null);
            jContentPane.add(getJScrollPaneTagName(), null);

            jContentPane.add(jLabelBuildTarget, null);
            jContentPane.add(getJScrollPaneBuildTarget(), null);

            jContentPane.add(jLabelTargetArch, null);

            jArchCheckBox.setDisabledItems(vDisableArchList);
            jArchCheckBox.setSelectedItems(vArchList);
            jContentPane.add(jArchCheckBox, null);

            jContentPane.add(jLabelEnableThreads, null);
            jContentPane.add(getCheckBoxEnableThreads(), null);

            jContentPane.add(jLabelThreadCount, null);
            jContentPane.add(getTextFieldThreadCount(), null);

            jContentPane.add(getJButtonSave(), null);
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

        if (arg0.getSource() == jButtonBrowse) {
            // TODO: Call file browser, starting in $WORKSPACE/Tools/Conf directory

        }

        if (arg0.getSource() == jButtonSave) {
            saveTargetFile();
            JOptionPane.showMessageDialog(this, "<html>The target.txt file has been saved!"
                                                + "<br>A copy of the original file, target.txt.bak has"
                                                + "<br>also been created.</html>");
            this.exit();
        }

        if (arg0.getSource() == jButtonCancel) {
            this.exit();
        }
        
        if (arg0.getSource() == jCheckBoxEnableThreads) {
            if (jCheckBoxEnableThreads.isSelected() == false) {
                threadCount = "";
                jTextFieldThreadCount.setText(threadCount);
            }
        }
    }
    

    private void updateActivePlatform() {
        int lineAP;
        if (targetLines[activePlatformId] != null) {
            lineAP = targetLineNumber[activePlatformId];
        } else {
            lineAP = targetLineNumberMax;
            targetLineNumber[activePlatformId] = lineAP;
            targetLineNumberMax++;
        }
        if (jComboBoxActivePlatform.getSelectedItem() == "Do Not Set") {
            targetFileContents[lineAP] = "";
            targetLines[activePlatformId] = "";
        } else {
            targetFileContents[lineAP] = "ACTIVE_PLATFORM = " + jComboBoxActivePlatform.getSelectedItem() + "\r\n";
            targetLines[activePlatformId] = targetFileContents[lineAP];
        }
        if (Debug)
            System.out.println("Active Platform: " + targetFileContents[lineAP]);
    }

    private void updateToolDefFile() {
        int lineTDF;
        if (targetLines[toolDefFileId] != null) {
            lineTDF = targetLineNumber[toolDefFileId];
        } else {
            lineTDF = targetLineNumberMax;
            targetLineNumber[toolDefFileId] = lineTDF;
            targetLineNumberMax++;
        }
        if (Debug)
            System.out.println("Tool Config File: " + jTextFieldToolsConfigFile.getText());
        if (jTextFieldToolsConfigFile.getText() == null) {
            targetFileContents[lineTDF] = "#MT#";
            targetLines[toolDefFileId] = "";
        } else {
            targetFileContents[lineTDF] = "TOOL_CHAIN_CONF = " + jTextFieldToolsConfigFile.getText();
            targetLines[toolDefFileId] = targetFileContents[lineTDF];
        }
    }

    private void updateToolTagNames() {
        String sTagNames = vectorToString(iCheckBoxListTagName.getAllCheckedItemsString());
        int lineTTN;

        if (targetLines[tagNameId] != null) {
            lineTTN = targetLineNumber[tagNameId];
        } else {
            lineTTN = targetLineNumberMax;
            targetLineNumber[tagNameId] = lineTTN;
            targetLineNumberMax++;
        }

        if (Debug)
            System.out.println("Tag Name(s): " + sTagNames);

        if (sTagNames.length() > 0) {
            targetFileContents[lineTTN] = "TOOL_CHAIN_TAG = " + sTagNames;
            targetLines[tagNameId] = targetFileContents[lineTTN];
        } else {
            targetFileContents[lineTTN] = "#MT#";
            targetLines[tagNameId] = "";
        }
    }

    private void updateBuildTargets() {
        String sBuildTargets = vectorToString(iCheckBoxListBuildTarget.getAllCheckedItemsString());
        int lineBT;

        if (targetLines[buildTargetId] != null) {
            lineBT = targetLineNumber[buildTargetId];
        } else {
            lineBT = targetLineNumberMax;
            targetLineNumber[buildTargetId] = lineBT;
            targetLineNumberMax++;
        }
        if (Debug)
            System.out.println("Build Target(s): " + sBuildTargets);
        if (sBuildTargets.length() > 0) {
            targetFileContents[lineBT] = "TARGET = " + sBuildTargets;
            targetLines[buildTargetId] = targetFileContents[lineBT];
        } else {
            targetFileContents[lineBT] = "#MT#";
            targetLines[buildTargetId] = "";
        }

    }

    private void updateArchitectures() {
        String sArchList = jArchCheckBox.getSelectedItemsString().trim();

        if (Debug)
            System.out.println("Architectures: " + sArchList);

        int lineSA;
        if (targetLines[targetArchId] != null) {
            lineSA = targetLineNumber[targetArchId];
        } else {
            lineSA = targetLineNumberMax;
            targetLineNumber[targetArchId] = lineSA;
            targetLineNumberMax++;
        }
        if (sArchList == "") {
            targetFileContents[lineSA] = "#MT#";
            targetLines[targetArchId] = "";
        } else {
            targetFileContents[lineSA] = "TARGET_ARCH = " + sArchList;
            targetLines[targetArchId] = targetFileContents[lineSA];
        }

    }

    private void updateEnableThreads() {
        int lineET;
        if (targetLines[threadEnableId] != null) {
            lineET = targetLineNumber[threadEnableId];
        } else {
            lineET = targetLineNumberMax;
            targetLineNumber[threadEnableId] = lineET;
            targetLineNumberMax++;
        }
        if (jCheckBoxEnableThreads.isSelected() == true) {
            targetFileContents[lineET] = "MULTIPLE_THREAD = enabled";
            targetLines[threadEnableId] = targetFileContents[lineET];
        } else {
            targetFileContents[lineET] = "#MT#";
            targetLines[threadEnableId] = "";
        }
    }
    
    private void updateThreadCount() {
        int lineTC;

        if (targetLines[threadCountId] != null) {
            lineTC = targetLineNumber[threadCountId];
        } else {
            lineTC = targetLineNumberMax;
            targetLineNumber[threadCountId] = lineTC;
            targetLineNumberMax++;
        }
        if (jCheckBoxEnableThreads.isSelected() == true) {
            // Threading must be enabled
            if (jTextFieldThreadCount.getText().length() > 0) {
                // Thread Count must be greater than 0
                Scanner scan = new Scanner(jTextFieldThreadCount.getText().trim()); 
                if (scan.nextInt() > 0) {      
                    targetFileContents[lineTC] = "MAX_CONCURRENT_THREAD_NUMBER = " + jTextFieldThreadCount.getText().trim();
                    targetLines[threadCountId] = targetFileContents[lineTC];
                } else {
                    Log.wrn("Build Preferences", "Threading Enabled, but thread count is not set, setting to default of 1.");
                    targetFileContents[lineTC] = "MAX_CONCURRENT_THREAD_NUMBER = 1";
                    targetLines[threadCountId] = "MAX_CONCURRENT_THREAD_NUMBER = 1";
                }
            } else {
                Log.wrn("Build Preferences", "Threading Enabled, but thread count is not set, setting to default of 1.");
                targetFileContents[lineTC] = "MAX_CONCURRENT_THREAD_NUMBER = 1";
                targetLines[threadCountId] = "MAX_CONCURRENT_THREAD_NUMBER = 1";
            }
        } else {
            // Don't track threads if threading is not enabled
            targetFileContents[lineTC] = "#MT#";
            targetLines[threadCountId] = "";
            threadCount = "";
        }
        
    }
    private String vectorToString(Vector<String> v) {
        String s = " ";
        for (int i = 0; i < v.size(); ++i) {
            s += v.get(i);
            s += " ";
        }
        return s.trim();
    }

    protected Vector<String> stringToVector(String s) {
        if (s == null) {
            return null;
        }
        String[] sArray = s.split(" ");
        Vector<String> v = new Vector<String>();
        for (int i = 0; i < sArray.length; ++i) {
            v.add(sArray[i]);
        }
        return v;
    }

    private void saveTargetFile() {
        updateActivePlatform();
        updateToolDefFile();
        updateToolTagNames();
        updateBuildTargets();
        updateArchitectures();
        updateEnableThreads();
        updateThreadCount();

        try {
            copy(targetFile, targetFile + ".bak");
            FileWriter fileWriter = new FileWriter(targetFile);
            BufferedWriter writer = new BufferedWriter(fileWriter);
            for (int i = 0; i < targetLineNumberMax; i++) {
                if (! targetFileContents[i].contains("#MT#"))
                    writer.write(targetFileContents[i] + "\n");
            }
            writer.close();
        } catch (IOException e) {
            Log.err(toolsConfFile + " Write Error ", e.getMessage());
            e.printStackTrace();
        }
    }

    private void copy(String txtFile, String bakFile) throws IOException {
        File fromFile = new File(txtFile);
        File toFile = new File(bakFile);
        FileInputStream fromTxt = null;
        FileOutputStream toBak = null;
        if (!fromFile.exists()) {
            fromFile.createNewFile();
        }
        try {
            fromTxt = new FileInputStream(fromFile);
            toBak = new FileOutputStream(toFile);
            byte[] buffer = new byte[4096];
            int bytes_read;
            while ((bytes_read = fromTxt.read(buffer)) != -1) {
                toBak.write(buffer, 0, bytes_read);
            }
        } finally {
            if (fromTxt != null)
                try {
                    fromTxt.close();
                } catch (IOException e) {
                    Log.err(toolsConfFile + " Read Error ", e.getMessage());

                }
            if (toBak != null)
                try {
                    toBak.close();
                } catch (IOException e) {
                    Log.err(toolsConfFile + ".bak Write Error ", e.getMessage());
                }
        }
    }

    private void exit() {
        this.setVisible(false);
        if (bTarget != null) {
            bTarget.dispose();
        }
    }
}
