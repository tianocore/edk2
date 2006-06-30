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

import org.apache.tools.ant.BuildException;
import org.tianocore.build.exception.EdkException;
import org.tianocore.build.toolchain.ToolChainKey;
import org.tianocore.build.toolchain.ToolChainMap;

import java.io.File;
import java.util.Iterator;
import java.util.Set;


/**
  This class parse all config files and get STD_FLAGS, GLOBAL_FLAGS, and also 
  command path + name.
  
  @since GenBuild 1.0
**/
public class ToolChainConfig {
    ///
    /// list of attributes
    ///
    private final static String[] attributes = {"NAME", "PATH", "DPATH", "SPATH", "EXT", "FAMILY", "FLAGS"};
    ///
    /// elements which are used to define one type of tool
    ///
    private final static String[] elements = {"TARGET", "TOOLCHAIN", "ARCH", "CMD", "ATTRIBUTE" };

    ///
    /// tool chain definitions
    ///
    private ToolChainMap config = null;
    private ToolChainInfo info = new ToolChainInfo();

    /**
      Public construct method.
    **/
    public ToolChainConfig () {
    }

    /**
      Public construct method.
      
      @param confPath the path of config files
      @param toolChainTag TOOL_CHAIN name
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

    /// 
    /// 
    /// 
    public void parseToolChainDefKey (Set<ToolChainKey> toolChainDefKey) {
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
     public Set<String> getTargets() {
         return info.getTargets();
     }

     public Set<String> getTagnames() {
         return info.getTagnames();
     }

     public Set<String> getArchs() {
         return info.getArchs();
     }

     public Set<String> getCommands() {
         return info.getCommands();
     }

     public String getValue(String key) {
         return config.get(key);
     }

     public String getValue(String[] keySet) {
         return config.get(keySet);
     }

     public String getValue(ToolChainKey key) {
         return config.get(key);
     }
 **/

    public ToolChainMap getConfig() {
        return config;
    }

    public ToolChainInfo getConfigInfo() {
        return info;
    }

    ///
    /// override toString()
    /// 
    public String toString() {
        StringBuffer ts = new StringBuffer(10240);

        Iterator it = config.keySet().iterator();
        while (it.hasNext()) {
            ToolChainKey key = (ToolChainKey)it.next();
            ts.append(key.toString() + " = ");
//            ts.append(config.get(key) + "\n");
        }

        return ts.toString();
    }
}

