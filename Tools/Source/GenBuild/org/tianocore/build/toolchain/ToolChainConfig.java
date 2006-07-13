/** @file
  ToolChainConfig class.
  
  ToolChainFactory class parse all config files and get tool chain information.
  
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
package org.tianocore.build.toolchain;

import org.apache.tools.ant.BuildException;
import org.tianocore.exception.EdkException;
import org.tianocore.build.toolchain.ToolChainKey;
import org.tianocore.build.toolchain.ToolChainMap;

import java.io.File;
import java.util.Iterator;
import java.util.Set;


/**
 
  ToolChainFactory class parse all config files and get tool chain information.
  
 **/
public class ToolChainConfig {
    ///
    /// tool chain definitions
    ///
    private ToolChainMap config = null;
    ///
    /// tool chain information (how many targets, archs, etc.)
    /// 
    private ToolChainInfo info = new ToolChainInfo();

    /**
      Public construct method.
     **/
    public ToolChainConfig () {
    }

    /**
      Public construct method.
      
      @param toolChainFile File object representing the tool chain configuration file
    **/
    public ToolChainConfig (File toolChainFile) {
        try {
            config = ConfigReader.parseToolChainConfig(toolChainFile);
            parseToolChainDefKey(config.keySet());
        }
        catch (EdkException ex) {
            throw new BuildException(ex.getMessage());
        }
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
            info.addTargets(keySet[0]);
            info.addTagnames(keySet[1]);
            info.addArchs(keySet[2]);
            info.addCommands(keySet[1], keySet[3]);
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
     
      @return ToolChainInfo
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

