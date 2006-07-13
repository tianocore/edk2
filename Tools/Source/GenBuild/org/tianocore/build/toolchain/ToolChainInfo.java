/** @file
This file is to define  ToolChainInfo class.

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
import java.util.LinkedHashSet;
import java.util.Map;
import java.util.Set;

public class ToolChainInfo {
    //
    // build target set
    // 
    private Set<String> targets = new LinkedHashSet<String>();
    //
    // tool chain tag name set
    // 
    private Set<String> tagnames = new LinkedHashSet<String>();
    //
    // build archs set
    // 
    private Set<String> archs = new LinkedHashSet<String>();
    //
    // build commands set
    // 
    private Set<String> commands = new LinkedHashSet<String>();
    //
    // build commands for specific tool chain
    // 
    private Map<String, Set<String>> commandMap = new HashMap<String, Set<String>>();
    /**
     Add a list of targets in the form of string separated by space

     @param targetList target list string
     **/
    public void addTargets(String targetList) {
        //
        // targetList some targets separated by space " "
        //
        if (targetList == null) {
            targets.add("*");
            return ;
        }
        addTargets(targetList.split(" "));
    }
    /**
     Add a list of targets in the form of string array
     
     @param targetArray target string array
     **/
    public void addTargets(String[] targetArray) {
        if (targetArray == null ) {
            return ;
        }
        for (int i = 0; i < targetArray.length; i++) {
            targets.add(targetArray[i]);
        }
    }
    /**
     Add a list of target in the form of set
     
     @param targetSet target string set
     **/
    public void addTargets(Set<String> targetSet) {
        targets.addAll(targetSet);
    }
    /**
       Add a list of tool chain tag name in the form of string separated by space

       @param tagnameList
     **/
    public void addTagnames(String tagnameList) {
        //
        // tagnameList some tagnames separated by space " "
        //
        if (tagnameList == null) {
            tagnames.add("*");
            return ;
        }
        addTagnames(tagnameList.split(" "));
    }
    
    public void addTagnames(String[] tagnameArray) {
        if (tagnameArray == null ) {
            return ;
        }
        for (int i = 0; i < tagnameArray.length; i++) {
            tagnames.add(tagnameArray[i]);
        }
    }
    
    public void addTagnames(Set<String> tagnameSet) {
        tagnames.addAll(tagnameSet);
    }
    
    public void addArchs(String archList) {
        //
        // archList some archs separated by space " "
        //
        if (archList == null) {
            archs.add("*");
            return ;
        }
        addArchs(archList.split(" "));
    }
    
    public void addArchs(String[] archArray) {
        if (archArray == null ) {
            return ;
        }
        for (int i = 0; i < archArray.length; i++) {
            archs.add(archArray[i]);
        }
    }
    
    public void addArchs(Set<String> archSet) {
        archs.addAll(archSet);
    }
    
    public void addCommands(String toolChain, String commandList) {
        //
        // archList some archs separated by space " "
        //
        if (commandList == null || commandList.length() == 0) {
            return ;
        }
        addCommands(commandList.split(" "));
    }
    
    public void addCommands(String[] commandArray) {
        if (commandArray == null ) {
            return ;
        }
        for (int i = 0; i < commandArray.length; i++) {
            commands.add(commandArray[i]);
        }
    }
    
    public void addCommands(String toolChain, String[] commandArray) {
        if (commandArray == null) {
            return ;
        }

        Set<String> toolChainCommandSet = commandMap.get(toolChain);
        if (toolChainCommandSet == null) {
            toolChainCommandSet = new LinkedHashSet<String>();
            commandMap.put(toolChain, toolChainCommandSet);
        }
        for (int i = 0; i < commandArray.length; i++) {
            commands.add(commandArray[i]);
            toolChainCommandSet.add(commandArray[i]);
        }
    }
    
    public void addCommands(String toolChain, Set<String> commandSet) {
        if (commandSet == null) {
            return;
        }
        Set<String> toolChainCommandSet = commandMap.get(toolChain);
        if (toolChainCommandSet == null) {
            toolChainCommandSet = new LinkedHashSet<String>();
            commandMap.put(toolChain, toolChainCommandSet);
        }
        commands.addAll(commandSet);
        toolChainCommandSet.addAll(commandSet);
    }
    
    public ToolChainInfo union(ToolChainInfo info) {
        ToolChainInfo result = new ToolChainInfo();
        result.addTargets(union(this.targets, info.targets));
        result.addTagnames(union(this.tagnames, info.tagnames));
        result.addArchs(union(this.archs, info.archs));
        return result;
    }
    
    public ToolChainInfo intersection(ToolChainInfo info) {
        ToolChainInfo result = new ToolChainInfo();
        result.addTargets(intersection(this.targets, info.targets));
        result.addTagnames(intersection(this.tagnames, info.tagnames));
        result.addArchs(intersection(this.archs, info.archs));
        return result;
    }
    
    private Set<String> union(Set<String> set1, Set<String> set2) {
        Set<String> result = new LinkedHashSet<String>();
        result.addAll(set1);
        result.addAll(set2);
        result.remove("*");
        return result;
    }
    
    private Set<String> intersection(Set<String> set1, Set<String> set2) {
        Set<String> result = new LinkedHashSet<String>();
        boolean set1HasWildcard = set1.contains("*");
        boolean set2HasWildcard = set2.contains("*");

        if (set1HasWildcard && set2HasWildcard) {
            result.addAll(set1);
            result.addAll(set2);
        } else if (set1HasWildcard) {
            result.addAll(set2);
        } else if (set2HasWildcard) {
            result.addAll(set1);
        } else {
            result.addAll(set1);
            result.retainAll(set2);
        }

        return result;
    }
    
    public String[] getTargets() {
        return (String[])targets.toArray(new String[targets.size()]);
    }
    
    public String[] getTagnames() {
        return (String[])tagnames.toArray(new String[tagnames.size()]);
    }
    
    public String[] getArchs() {
        return (String[])archs.toArray(new String[archs.size()]);
    }

    public String[] getCommands() {
        return (String[])commands.toArray(new String[commands.size()]);
    }

    public Set<String> getCommands(String toolChain) {
        return commandMap.get(toolChain);
    }
    
    public String toString() {
        return targets + "\n" + tagnames + "\n" + archs + "\n" + commands;
    }
    
    public void normalize() {
        targets.remove("*");
        tagnames.remove("*");
        archs.remove("*");
        commands.remove("*");
    }
}
