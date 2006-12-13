/** @file
  ToolChainConfig class.
  
  ToolChainConfig class parse all config files and get tool chain information.
  
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
package org.tianocore.build.toolchain;

import java.io.File;
import java.util.Iterator;
import java.util.Set;

import org.apache.tools.ant.Project;
import org.tianocore.build.exception.GenBuildException;
import org.tianocore.build.toolchain.ToolChainKey;
import org.tianocore.build.toolchain.ToolChainMap;


/**
 
  ToolChainConfig class parse all config files and get tool chain information.
  
 **/
public class ToolChainConfig {
    //
    // tool chain definitions
    //
    private ToolChainMap config = null;
    //
    // tool chain information (how many targets, archs, etc.)
    // 
    private ToolChainInfo info = new ToolChainInfo();

    /**
      Public construct method.
      
      @param toolChainFile File object representing the tool chain configuration file
    **/
    public ToolChainConfig (Project prj, File toolChainFile) throws GenBuildException {
        config = getToolChainConfig(prj, toolChainFile);
        parseToolChainDefKey(config.keySet());
    }

    /**
       Read tool chain definitions from specified file and put them in 
       ToolChainMap class.

       @param ConfigFile    The file containing tool chain definitions
       
       @return ToolChainMap
     **/
    private ToolChainMap getToolChainConfig(Project prj, File ConfigFile) throws GenBuildException {
        ToolChainMap map = new ToolChainMap();
        String[][] toolChainDef = ConfigReader.parse(prj, ConfigFile);
    
        for (int i = 0; i < toolChainDef[0].length; ++i) {
            map.put(toolChainDef[0][i], toolChainDef[1][i]);
        }

        return map;
    }

    /**
     Collect target, tool chain tag, arch and command information from key part
     of configuration
      
     @param toolChainDefKey The set of keys in tool chain configuration
     **/
    private void parseToolChainDefKey (Set<ToolChainKey> toolChainDefKey) {
        Iterator it = toolChainDefKey.iterator();
        while (it.hasNext()) {
            ToolChainKey key = (ToolChainKey)it.next();
            String[] keySet = key.getKeySet();
            info.addTargets(keySet[ToolChainElement.TARGET.value]);
            info.addTagnames(keySet[ToolChainElement.TOOLCHAIN.value]);
            info.addArchs(keySet[ToolChainElement.ARCH.value]);
            info.addCommands(keySet[ToolChainElement.TOOLCODE.value]);
            info.normalize();
        }
    }

    /**
     Return the tool chain configuration information in a Map form 
      
     @return ToolChainMap Tool chain configurations in a ToolChainMap
     **/
    public ToolChainMap getConfig() {
        return config;
    }

    /**
     Return the tool chain's target, arch, tag and commands information
     
      @return ToolChainInfo Tool chain information summary
     **/
    public ToolChainInfo getConfigInfo() {
        return info;
    }

    /**
     override toString()
     
     @return String The converted configuration string in name=value form
     **/
    public String toString() {
        StringBuffer ts = new StringBuffer(10240);

        Iterator it = config.keySet().iterator();
        while (it.hasNext()) {
            ToolChainKey key = (ToolChainKey)it.next();
            ts.append(key.toString() + " = ");
            ts.append(config.get(key) + "\n");
        }

        return ts.toString();
    }
}

