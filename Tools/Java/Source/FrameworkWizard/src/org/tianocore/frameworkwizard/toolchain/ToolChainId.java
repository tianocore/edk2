/** @file

 This file is used to init tool chain and tool preference data
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php

 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 **/

package org.tianocore.frameworkwizard.toolchain;

import java.io.*;
import java.io.File;
import java.lang.Integer;
import java.util.ArrayList;

import javax.swing.JOptionPane;

import org.tianocore.frameworkwizard.FrameworkWizardUI;
import org.tianocore.frameworkwizard.common.DataType;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.Log;
import org.tianocore.frameworkwizard.workspace.Workspace;

public class ToolChainId {
    //
    // Define class members
    //
    
    private final boolean Debug = false;
    
    private String toolDefinitionFile = null;

    private final int toolDefFieldCount = 5;

    private String toolsDefIdentifier = null;

    private String toolsDefTargetNames = null;

    private final int toolsDefTargetNameField = 0;

    private String toolsDefTagNames = null;

    private final int toolsDefTagNameField = 1;

    private String toolsDefArchNames = null;

    private final int toolsDefArchNameField = 2;

    private final int toolsDefToolArgumentField = 4;
    
    private String toolCommandCodes = null;
    
    private final int toolsDefCmdCodeArgumentField = 3;
    

    private String activePlatform = null;

    private String targetArchs = null;

    private String tagNames = null;

    private String buildTargets = null;

    private String toolFamilies = null;
    
    private ArrayList<String> toolDefinitionContents = new ArrayList<String>(50);

    private static boolean threadEnabled = false;

    private static int maxThreadCount = 0;

    private String toolsDir = Workspace.getCurrentWorkspace() + System.getProperty("file.separator") + "Tools"
                              + System.getProperty("file.separator") + "Conf";

    private String strTargetFile = toolsDir + DataType.FILE_SEPARATOR + "target.txt";

    private String defaultToolsConf = toolsDir + DataType.FILE_SEPARATOR + "tools_def.txt";

    public void init() {
        readTargetTxtFile();
        readToolDefinitionFile();
    }
    
    public ToolChainId() {
        super();
        init();
    }

    public String getToolDefinitionFile() {
        return toolDefinitionFile;
    }

    public String getActivePlatform() {
        return activePlatform;
    }

    public String getBuildTargets() {
        return buildTargets;
    }

    public String getTagNames() {
        return tagNames;
    }

    public String getTargetArchitectures() {
        return targetArchs;
    }

    public boolean getThreadEnabled() {
        return threadEnabled;
    }

    public int getMaxThreadCount() {
        return maxThreadCount;
    }

    public String getToolFamilies() {
        return toolFamilies;
    }
    
    public String getToolDefinitionIdentifier() {
        return toolsDefIdentifier;
    }
    
    public ArrayList<String> getToolDefinitionStatements() {
        return toolDefinitionContents;
    }

    public String getToolsDefTagNames() {
        return toolsDefTagNames;
    }
    
    public String getToolsDefTargetNames() {
        return toolsDefTargetNames;
    }
    
    public String getToolsDefCommandCodes() {
        return toolCommandCodes;
    }
    
    public String getToolsDefArchNames() {
        return toolsDefArchNames;
    }
    
    private void readTargetTxtFile() {
        File tFile = new File(strTargetFile);

        if (tFile.exists()) {
            try {
                FileReader fileReader = new FileReader(strTargetFile);
                BufferedReader reader = new BufferedReader(fileReader);
                String rLine = null;
                String inLine[] = new String[2];
                while ((rLine = reader.readLine()) != null) {
                	rLine = rLine.trim();
                    if ((rLine.startsWith("ACTIVE_PLATFORM")) && (activePlatform == null)) {
                        // Only one active platform is permitted!
                        inLine = rLine.split("=");
                        if (inLine.length > 1) {
                        	activePlatform = inLine[1].trim();
                        }
                    }
                    if ((rLine.startsWith("TARGET" + " ")) || (rLine.startsWith("TARGET" + "\t"))
                        || (rLine.startsWith("TARGET="))) {
                        // Handle multiple Target Names
                        if (rLine.contains(",")) {
                            inLine = rLine.split("=");
                            if (inLine.length > 1) {
                            	buildTargets = inLine[1].trim().replaceAll(",", " ");
                            }
                        } else {
                            inLine = rLine.trim().split("=");
                            if (inLine.length > 1) {
                            	buildTargets = inLine[1].trim();
                            }
                        }
                    }
                    if (rLine.startsWith("TARGET_ARCH")) {
                        // Handle multiple Target Architectures
                        if (rLine.contains(",")) {
                            inLine = rLine.split("=");
                            if (inLine.length > 1) {
                            	targetArchs = inLine[1].trim().replaceAll(",", " ");
                            }
                        } else {
                            inLine = rLine.split("=");
                            if (inLine.length > 1) {
                            	targetArchs = inLine[1].trim();
                            }
                        }
                    }
                    if (rLine.startsWith("TOOL_CHAIN_CONF")) {
                        // Only one file is permitted
                        inLine = rLine.split("=");
                        if (inLine.length > 1) {
                        	toolDefinitionFile = inLine[1].trim();
                        }
                    }

                    if (rLine.startsWith("TOOL_CHAIN_TAG")) {
                        // Handle multiple Tool TagNames
                        if (rLine.contains(",")) {
                            inLine = rLine.split("=");
                            if (inLine.length > 1) {
                            	tagNames = inLine[1].trim().replaceAll(",", " ");
                            }
                        } else {
                            inLine = rLine.split("=");
                            if (inLine.length > 1) {
                            	tagNames = inLine[1].trim();
                            }
                        }
                    }

                    if (rLine.startsWith("MULTIPLE_THREAD")) {
                        // Handle Thread Enable flag
                        if ((rLine.toLowerCase().contains("enabled"))
                            || (rLine.toLowerCase().contains("true"))) {
                            threadEnabled = true;
                        } else {
                            threadEnabled = false;
                        }
                    }

                    if (rLine.startsWith("MAX_CONCURRENT_THREAD_NUMBER")) {
                        // Handle Thread Enable flag
                        inLine = rLine.split("=");
                        if (inLine.length > 1) {
                        	maxThreadCount = Integer.valueOf(inLine[1].trim());
                        }
                    }
                }
                reader.close();
            } catch (IOException e) {
                Log.log(this.strTargetFile + " Read Error ", e.getMessage());
                e.printStackTrace();
            }
        } else {
            JOptionPane.showMessageDialog(FrameworkWizardUI.getInstance(), "<html>" + "Tool Preferences file: <br>" + strTargetFile
                                                + "<br>does not exist!</html>");
        }
    }

    private void readToolDefinitionFile() {

        // Parse the tool definition file looking for targets and architectures
        String toolsConfFile = null;
        if (toolDefinitionFile != null) {
            String resString = (Tools.convertPathToCurrentOsType(toolDefinitionFile)).trim();
            toolsConfFile = Workspace.getCurrentWorkspace() + System.getProperty("file.separator") + resString.trim();
            File toolsDefFile = new File(toolsConfFile);
            if (!toolsDefFile.exists()) {
                JOptionPane.showMessageDialog(FrameworkWizardUI.getInstance(), "<html>" + "Tool Definition file, " + toolDefinitionFile
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
                while ((rLine = reader.readLine()) != null) {
                    if ((rLine.startsWith("IDENTIFIER")) && (toolsDefIdentifier == null)) {
                        result = rLine.split("=");
                        toolsDefIdentifier = (result[1]).trim();
                    } else if ((!rLine.startsWith("#")) && (rLine.contains("="))) {
                        result = rLine.split("=");
                        toolsDefFields = ((result[0]).trim()).split("_");
                        if (toolsDefTargetNames == null) {
                            toolsDefTargetNames = (toolsDefFields[toolsDefTargetNameField]).toUpperCase().trim() + " ";
                        } else if (!toolsDefTargetNames.contains((toolsDefFields[toolsDefTargetNameField]).toUpperCase().trim())) {
                            toolsDefTargetNames += (toolsDefFields[toolsDefTargetNameField]).toUpperCase().trim() + " ";
                        }
                        if (toolsDefTagNames == null) {
                            toolsDefTagNames = (toolsDefFields[toolsDefTagNameField]).toUpperCase().toUpperCase().trim() + " ";
                        } else if (!toolsDefTagNames.contains((toolsDefFields[toolsDefTagNameField]).toUpperCase().trim())) {
                            toolsDefTagNames += (toolsDefFields[toolsDefTagNameField]).toUpperCase().trim() + " ";
                        }
                        if (toolsDefArchNames == null) {
                            toolsDefArchNames = (toolsDefFields[toolsDefArchNameField]).toUpperCase().trim() + " ";
                        } else if (!toolsDefArchNames.contains((toolsDefFields[toolsDefArchNameField]).toUpperCase().trim())) {
                            toolsDefArchNames += (toolsDefFields[toolsDefArchNameField]).toUpperCase().trim() + " ";
                        }
                        if ((toolFamilies == null) && (rLine.trim().contains("FAMILY"))) {
                            toolFamilies = (toolsDefFields[toolsDefToolArgumentField]).toUpperCase().trim() + " ";
                        } else if ((rLine.trim().contains("FAMILY"))
                                   && (!toolFamilies.contains((toolsDefFields[toolsDefToolArgumentField]).toUpperCase().trim()))) {
                            toolFamilies += (toolsDefFields[toolsDefToolArgumentField]).toUpperCase().trim() + " ";
                        }
                        if ((toolCommandCodes == null)) {
                            toolCommandCodes = (toolsDefFields[toolsDefCmdCodeArgumentField]).toUpperCase().trim() + " ";
                        } else if ((!toolCommandCodes.contains((toolsDefFields[toolsDefCmdCodeArgumentField]).toUpperCase().trim()))) {
                            toolCommandCodes += (toolsDefFields[toolsDefCmdCodeArgumentField].toUpperCase().trim()) + " ";
                        }
                        
                        toolDefinitionContents.add(rLine.trim().replaceAll(" ", ""));
                    }
                }
                reader.close();
                if (!toolsDefTargetNames.matches("[A-Z]+")) {
                    toolsDefTargetNames = toolsDefTargetNames.replace("* ", "").trim();
                    if (Debug)
                        System.out.println("tools_def file does not define build targets: '" + toolsDefTargetNames
                                           + "'");
                }
            } catch (IOException e) {
                Log.log(toolsConfFile + " Read Error ", e.getMessage());
                e.printStackTrace();
            }
        }
    }

}
