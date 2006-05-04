/** @file
  ToolChainFactory class.
  
  ToolChainFactory class parse all config files and get STD_FLAGS, GLOBAL_FLAGS,
  and also command path + name.
  
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
package org.tianocore.build.toolchain;

import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;
import java.util.StringTokenizer;
import java.io.File;

import org.apache.tools.ant.Project;


/**
  This class parse all config files and get STD_FLAGS, GLOBAL_FLAGS, and also 
  command path + name.
  
  @since GenBuild 1.0
**/
public class ToolChainFactory {
    ///
    /// list of Arch: EBC, ARM, IA32, X64, IPF, PPC
    ///
    public final static String[] arch = { "EBC", "ARM", "IA32", "X64", "IPF",
                    "PPC"};

    ///
    /// list of OS: Linux, Windows
    ///
    public final static String[] os = { "WINDOWS", "LINUX" };

    ///
    /// list of Command Type: CC, LIB, LINK, ASL, ASM, ASMLINK, PP
    ///
    public final static String[] commandType = { "CC", "LIB", "LINK", "ASL",
                    "ASM", "ASMLINK", "PP" };

    ///
    /// default command name for every command
    ///
    public final static String[][] defaultCmdName = { { "CC", "cl" },
                    { "LIB", "lib" }, { "LINK", "link" }, { "ASL", "iasl" },
                    { "ASM", "ml" }, { "ASMLINK", "link" }, { "PP", "cl" } };

    private String confPath = ".";
    
    private String toolChainName = "MSFT";

    private String sTargetFilename = "target.txt";

    private String sToolsdefFilename = "tools_def.txt";

    private String sWorkspaceTarget = "WORKSPACE_TARGET";

    private String sTargetArch = "TARGET_ARCH";

    private HashMap<String,String[][]> filesMap = new HashMap<String,String[][]>();
    
    private HashMap<String,String> globalFlagsMap = new HashMap<String,String>();
    
    private String[][] globalFlagTable;
    
    private String currentTarget = "RELEASE";

    ///
    /// toolchain array list all results by parsing config files
    ///
    public static String[][] toolchain = null;
    
    /**
      Public construct method.
    **/
    public ToolChainFactory () {
    }

    /**
      Public construct method.
      
      @param project current ANT Project.
    **/
    public ToolChainFactory (Project project) {
        this.confPath = project.replaceProperties("${WORKSPACE_DIR}" + File.separatorChar + "Tools" + File.separatorChar + "Conf");
    }
    
    /**
      Public construct method.
      
      @param confPath the path of config files
      @param toolChainName TOOL_CHAIN name
    **/
    public ToolChainFactory (String confPath, String toolChainName) {
        this.confPath = confPath;
        //
        // If set tool used the set one, otherwise use default one.
        // toolChain used to define open tools define txt file.
        //
        if (toolChainName != null && toolChainName.length() > 0){
            this.toolChainName = toolChainName;
        }
    }

    /**
      Parse all config files, following are the detail steps:
      
      <ul>
        <li>Parse target.txt file. This file define the current build TARGET 
        and supported ARCH list. </li>
        <li>Parse tools_def.txt file. This file define every command name, path
        and vendor. </li>
        <li>For every supported ARCH and Command Type, find out STD_FLAGS, 
        GLOBAL_ADD_FLAGS, GLOBAL_SUB_FLAGS. </li>
      </ul>
      
      <p>Note that this method will be called only once during the whole build
      process. </p>
    **/
    public void setupToolChain() {
        if (toolchain != null) {
            return ;
        }
        Map<String, String> map = new HashMap<String, String>(40);
        //
        // parse target.txt
        //
        String[][] target = ConfigReader.parse(confPath, sTargetFilename);
        //
        // get workspace_target and initialize global flags setting
        //
        currentTarget = getValue(sWorkspaceTarget, target);
        parseGlobalSetting(currentTarget);
        String[] archList = getArchs(getValue(sTargetArch, target));
        
        //
        // If user write the ${toolChain}_Tools_Def.txt use this one,
        // otherwise used "tools_def.txt" file.
        //
        File tempFile = new File (confPath + File.separator + toolChainName.toLowerCase() + "_tools_def.txt");
        if (tempFile.exists()){
            sToolsdefFilename = toolChainName.toLowerCase() + "_tools_def.txt";
        }
        
        System.out.println("Tools definition file is: " + sToolsdefFilename);
        //
        // parse tools_def.txt
        //
        String[][] tools_def = ConfigReader.parse(confPath, sToolsdefFilename);
        //
        // for each arch find all command's path&name and flags
        //
        for (int i = 0; i < archList.length; i++) {
            for (int j = 0; j < commandType.length; j++) {
                //
                // Path & Name
                //
                map.put(archList[i] + "_" + commandType[j], getAbsoluteCmdPath(
                                archList[i], commandType[j], tools_def));
                //
                // Flags: CMD_STD_FLAGS + CMD_GLOBAL_FLAGS + CMD_PROJ_FLAGS
                // ARCH_CMD_STD_FLAGS
                //
                map.put(archList[i] + "_" + commandType[j] + "_STD_FLAGS",
                                getStdFlags(archList[i], commandType[j],
                                                tools_def));
                //
                // Flags:ARCH_CMD_VENDOR or ARCH_VENDOR
                //
                map.put(archList[i]+ "_"+commandType[j]+"_VENDOR", getVendorFlag(archList[i],
                        commandType[j], tools_def));
                //
                // ARCH_CMD_GLOBAL_FLAGS
                //
                String[] globalFlags = getGlobalFlags(archList[i], commandType[j],
                                tools_def);
                map.put(archList[i] + "_" + commandType[j] + "_GLOBAL_ADD_FLAGS",
                                globalFlags[0]);
                map.put(archList[i] + "_" + commandType[j] + "_GLOBAL_SUB_FLAGS",
                                globalFlags[1]);
                //
                // ARCH_CMD_GLOBAL_FLAGS, default is "".
                //
                map.put(archList[i] + "_" + commandType[j] + "_PROJ_FLAGS", "");
            }
            map.put(archList[i]+"_VENDOR", getVendorFlag(archList[i], null, tools_def));
        }
        Set keyset = map.keySet();
        Iterator iter = keyset.iterator();
        String[][] result = new String[map.size()][2];
        int i = 0;
        while (iter.hasNext()) {
            String key = (String) iter.next();
            result[i][0] = key;
            result[i++][1] = (String) map.get(key);
        }
        toolchain = result;
    }

    /**
      Get the standard flags (STD_FLAGS) for specified arch and command type. 
      
      <ul>
        <li>Find out Vendor that cmd Command Type with arch ARCH used. The 
        search sequence is ARCH_CMD_VENDOR -> ARCH_VENDOR -> "MSFT". Here
        we suppose default Vendor is MSFT.</li>
        <li>Search ${Vendor}_tools.txt file, and get the corrsponding flags. 
        </li>
      </ul>
      
      @param arch the ARCH
      @param cmd the command type
      @param map detail flags information of tools_def.txt
      @return the standard flags of arch ARCH and cmd Command Type 
    **/
    private String getStdFlags(String arch, String cmd, String[][] map) {
        //
        // first is to find out its Vendor in map
        // ARCH_CMD_VENDOR -> ARCH_VENDOR -> "MSFT"
        // Here we suppose default Vendor is MSFT.
        //
        String vendor = "MSFT";
        String str;
        if ((str = getValue(arch + "_" + cmd + "_VENDOR", map)) != null) {
            vendor = str;
        } else if ((str = getValue(arch + "_VENDOR", map)) != null) {
            vendor = str;
        }
        //
        // change to low letter
        //
        vendor = vendor.toLowerCase();
        //
        // parse the corresponding file and get arch_cmd value
        //
        String filename = vendor + "_tools.txt";
        String[][] flagsMap;
        if (filesMap.containsKey(filename)) {
            flagsMap = (String[][]) filesMap.get(filename);
        } else {
            //
            // read file and store in filesMap
            //
            flagsMap = ConfigReader.parse(confPath, vendor + "_tools.txt");
            filesMap.put(filename, flagsMap);
        }
        if ((str = getValue(arch + "_" + cmd, flagsMap)) != null) {
            return str;
        }
        return "";
    }

    /**
      Get the global flags (GLOBAL_ADD_FLAGS & GLOBAL_SUB_FLAGS) for specified 
      arch and command type. 
      
      <ul>
        <li>Find out Vendor that cmd Command Type with arch ARCH used. The 
        search sequence is ARCH_CMD_VENDOR -> ARCH_VENDOR -> "MSFT". Here
        we suppose default Vendor is MSFT.</li>
        <li>Search efi_flags_table.txt file, and get the corrsponding flags. 
        </li>
      </ul>
      
      @param arch the ARCH
      @param cmd the command type
      @param map detail flags information of tools_def.txt
      @return two values, first is GLOBAL_ADD_FLAGS and another value is 
      GLOBAL_SUB_FLAGS
    **/
    private String[] getGlobalFlags(String arch, String cmd, String[][] map) {
        String addStr = "";
        String subStr = "";
        //
        // first is to find out its Vendor in map
        // ARCH_CMD_VENDOR -> ARCH_VENDOR -> "MSFT"
        // Here we suppose default Vendor is MSFT.
        //
        String vendor = "MSFT";
        String str;
        if ((str = getValue(arch + "_" + cmd + "_VENDOR", map)) != null) {
            vendor = str;
        } else if ((str = getValue(arch + "_VENDOR", map)) != null) {
            vendor = str;
        }
        //
        // parse global flags table
        //
        if (globalFlagTable == null) {
            globalFlagTable =  ConfigReader.parseTable(confPath, "efi_flags_table.txt");
        }
        for (int i=0; i < globalFlagTable.length; i++){
            String[] item = globalFlagTable[i];
            if (item[2].equalsIgnoreCase(vendor + "_" + arch + "_" + cmd)){
                //
                // if item[0] == item[1] is existed in globalFlagsMap
                //
                if (globalFlagsMap.containsKey(item[0])){
                    if( item[1].equalsIgnoreCase((String)globalFlagsMap.get(item[0]))){
                        addStr += item[3] + " ";
                        subStr += item[4] + " ";
                    }
                }
            }
        }
        
        return new String[]{addStr, subStr};
    }

    /**
      Find out command path and command name. 
      
      <pre>
        Command path searching sequence in tools_def.txt file:
        Path: ARCH_CMD_PATH -> ARCH_PATH -> Set to "".
        
        Command name searching sequence in tools_def.txt file:
        Name: ARCH_CMD_NAME -> CMD_NAME -> Default Value.
      </pre>
      
      @param arch the ARCH
      @param cmd the Command Type
      @param map detail flags information of tools_def.txt
      @return the absolute command path and name
    **/
    private String getAbsoluteCmdPath(String arch, String cmd, String[][] map) {
        String path = "";
        String name = "";
        String str;
        //
        // find Path
        //
        if ((str = getValue(arch + "_" + cmd + "_PATH", map)) != null) {
            path = str;
        } else if ((str = getValue(arch + "_PATH", map)) != null) {
            path = str;
        }
        //
        // find Name
        //
        if ((str = getValue(arch + "_" + cmd + "_NAME", map)) != null) {
            name = str;
        } else if ((str = getValue(cmd + "_NAME", map)) != null) {
            name = str;
        } else {
            name = getValue(cmd, defaultCmdName);
        }
        if (path.equalsIgnoreCase("")) {
            return name;
        }
        return path + File.separatorChar + name;
    }

    /**
      Find out all global flags value, such as EFI_DEBUG equal YES or NO. Here 
      are three type files: global_efi_flags.txt, ${TARGET}_efi_flags.txt, 
      my_efi_flags.txt. global_efi_flags.txt with the highest priority while 
      my_efi_flags.txt with the lowest priority. 
      
      <p>All global flags value will store in <code>globalFlagsMap</code> for 
      getGlobalFlags using. </p> 
      
      @param target current build TARGET value
    **/
    private void parseGlobalSetting(String target){
        //
        // parse global_efi_flags -> ${TARGET}_efi_flags -> my_efi_flags
        // parse global_efi_flags
        //
        String[][] map = ConfigReader.parse(confPath, "global_efi_flags.txt");
        for (int i = 0; i < map.length; i++){
            if(globalFlagsMap.containsKey(map[i][0])){
                globalFlagsMap.remove(map[i][0]);
            }
            globalFlagsMap.put(map[i][0], map[i][1]);
        }
        //
        // parse ${TARGET}_efi_flags
        //
        map = ConfigReader.parse(confPath, target.toLowerCase() + "_efi_flags.txt");
        for (int i = 0; i < map.length; i++){
            if(globalFlagsMap.containsKey(map[i][0])){
                globalFlagsMap.remove(map[i][0]);
            }
            globalFlagsMap.put(map[i][0], map[i][1]);
        }
        //
        // parse my_efi_flags.txt
        //
        map = ConfigReader.parse(confPath, "my_efi_flags.txt");
        for (int i = 0; i < map.length; i++){
            if(globalFlagsMap.containsKey(map[i][0])){
                globalFlagsMap.remove(map[i][0]);
            }
            globalFlagsMap.put(map[i][0], map[i][1]);
        }
    }
    
    /**
      Find value with key from map. If not found, return null. 
      
      <p>Note that default is case-insensitive</p>
      
      @param key key value
      @param map mapping information
      @return the related value of key
    **/
    private String getValue(String key, String[][] map) {
        return getValue(key, map, false);
    }

    /**
      Find value with key from map. If not found, return null. 
      
      @param key key value
      @param map mapping information
      @param caseSensitive whether case sesitive or not
      @return the related value of key
    **/
    private String getValue(String key, String[][] map, boolean caseSensitive) {
        for (int i = 0; i < map.length; i++) {
            if (caseSensitive) {
                if (key.compareTo(map[i][0]) == 0) {
                    return map[i][1];
                }
            } else {
                if (key.compareToIgnoreCase(map[i][0]) == 0) {
                    return map[i][1];
                }
            }
        }
        return null;
    }

    /**
      Find value with key from <code>toolchain</code>. If not found, return null. 
    
      @param key key value
      @return the related value of key
    **/
    public static String getValue(String key){
        for (int i = 0; i < toolchain.length; i++) {
            if (key.compareToIgnoreCase(toolchain[i][0]) == 0) {
                return toolchain[i][1];
            }
        }
        return null;
    }
    
    /**
      Get Arch list from a string separated with comma. 
      
      <pre>
        For example:
          If the arch string is "IA32, X64, EBC".
          Then the result is {"IA32", "X64", "EBC"}. 
      </pre>
    
      @param arch string separated with comma
      @return Arch list
    **/
    public String[] getArchs(String arch) {
        if (arch == null) {
            return new String[0];
        }
        StringTokenizer st = new StringTokenizer(arch, " \t,");
        String[] archs = new String[st.countTokens()];
        int i = 0;
        while (st.hasMoreTokens()) {
            archs[i++] = st.nextToken().toUpperCase();
        }
        return archs;
    }

    /**
      Get current target value.
    
      @return current target value
    **/
    public String getCurrentTarget() {
        return currentTarget;
    }

    /**
      Find out Vendor that cmd Command Type with arch ARCH used. The 
      search sequence is ARCH_CMD_VENDOR -> ARCH_VENDOR -> "MSFT". Here
      we suppose default Vendor is MSFT.
      
      @param arch the ARCH
      @param cmd the Command Type
      @param map detail flags information of tools_def.txt
      @return the related vendor name
    **/
    public String getVendorFlag (String arch, String cmdType, String[][] map){
        //
        // ARCH_CMD_VENDOR -> ARCH_VENDOR -> "MSFT"
        // Here we suppose default Vendor is MSFT.
        //
        String str;
        String vendor = "";
        if (cmdType != null){
            if ((str = getValue(arch + "_" + cmdType + "_VENDOR", map)) != null) {
                vendor = str; 
            }else {
                vendor = "";
            }
        }else if (arch != null){
            if ((str = getValue(arch + "_VENDOR", map)) != null) {
                vendor = str; 
            }else {
                vendor = "";
            }
        }
        return vendor;
    }
    
}
