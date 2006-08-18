/** @file
ToolChainInfo class

This file is to define ToolChainInfo class.

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

package org.tianocore.build.toolchain;

import java.util.LinkedHashSet;
import java.util.Set;

/**
   ToolChainInfo collects valid build targets, tool chain tag, ARCHs and commands 
   information for real build use.
 **/
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

    /**
     Add a list of targets in the form of string separated by space

     @param targetList target list string
     **/
    public void addTargets(String targetList) {
        //
        // targetList some targets separated by space " "
        //
        if (targetList == null || targetList.length() == 0) {
            targets.add("*");
        } else {
            addTargets(targetList.split(" "));
        }
    }

    /**
     Add a list of targets in the form of string array
     
     @param targetArray target string array
     **/
    public void addTargets(String[] targetArray) {
        if (targetArray != null ) {
            for (int i = 0; i < targetArray.length; i++) {
                targets.add(targetArray[i]);
            }
        }
    }

    /**
     Add a list of target in the form of set
     
     @param targetSet target string set
     **/
    public void addTargets(Set<String> targetSet) {
        if (targetSet != null) {
            targets.addAll(targetSet);
        }
    }

    /**
       Add a list of tool chain tag name in the form of string separated by space

       @param tagnameList   Tool chain tag name list string
     **/
    public void addTagnames(String tagnameList) {
        //
        // tagnameList some tagnames separated by space " "
        //
        if (tagnameList == null || tagnameList.length() == 0) {
            tagnames.add("*");
        } else {
            addTagnames(tagnameList.split(" "));
        }
    }

    /**
       Add a list of tool chain tag name in the form of string array
       
       @param tagnameArray  Tool chain tag names array
     **/
    public void addTagnames(String[] tagnameArray) {
        if (tagnameArray != null ) {
            for (int i = 0; i < tagnameArray.length; i++) {
                tagnames.add(tagnameArray[i]);
            }
        }
    }

    /**
       Add a list of tool chain tag name in the form of Set
       
       @param tagnameSet    Tool chain tag names set
     **/
    public void addTagnames(Set<String> tagnameSet) {
        if (tagnameSet != null) {
            tagnames.addAll(tagnameSet);
        }
    }

    /**
       Add a list of ARCH in the form of string
       
       @param archList  ARCH string
     **/
    public void addArchs(String archList) {
        //
        // archList some archs separated by space " "
        //
        if (archList == null || archList.length() == 0) {
            archs.add("*");
        } else {
            addArchs(archList.split(" "));
        }
    }

    /**
       Add a list of ARCH in the form of string array
       
       @param archArray ARCH array
     **/
    public void addArchs(String[] archArray) {
        if (archArray != null ) {
            for (int i = 0; i < archArray.length; i++) {
                archs.add(archArray[i]);
            }
        }
    }

    /**
       Add a list of ARCH in the form of set
       
       @param archSet   ARCH set
     **/
    public void addArchs(Set<String> archSet) {
        if (archSet != null) {
            archs.addAll(archSet);
        }
    }

    /**
       Add a list of command in the form of string
       
       @param commandList   Command list string
     **/
    public void addCommands(String commandList) {
        //
        // archList some archs separated by space " "
        //
        if (commandList == null || commandList.length() == 0) {
            commands.add("*");
        } else {
            addCommands(commandList.split(" "));
        }
    }

    /**
       Add a list of ARCH in the form of array
       
       @param commandArray  Commands array
     **/
    public void addCommands(String[] commandArray) {
        if (commandArray != null ) {
            for (int i = 0; i < commandArray.length; i++) {
                commands.add(commandArray[i]);
            }
        }
    }

    /**
       Add a list of ARCH in the form of set
       
       @param commandSet    Commands set
     **/
    public void addCommands(Set<String> commandSet) {
        if (commandSet != null) {
            commands.addAll(commandSet);
        }
    }

    /**
       Make a union operation on this ToolChainInfo and the given one.

       @param info  Another ToolChainInfo object to merge with
       
       @return ToolChainInfo    Merged ToolChainInfo object
     **/
    public ToolChainInfo union(ToolChainInfo info) {
        ToolChainInfo result = new ToolChainInfo();
        result.addTargets(union(this.targets, info.targets));
        result.addTagnames(union(this.tagnames, info.tagnames));
        result.addArchs(union(this.archs, info.archs));
        return result;
    }

    /**
       Make a intersection operation on this ToolChainInfo and the given one

       @param info  Another ToolChainInfo object to intersect with
       
       @return ToolChainInfo    Intersected ToolChainInfo object
     **/
    public ToolChainInfo intersection(ToolChainInfo info) {
        ToolChainInfo result = new ToolChainInfo();
        result.addTargets(intersection(this.targets, info.targets));
        result.addTagnames(intersection(this.tagnames, info.tagnames));
        result.addArchs(intersection(this.archs, info.archs));
        return result;
    }

    /**
       Make a union operation on two Sets

       @param set1  One Set
       @param set2  Another Set
       
       @return Set<String>  Merged Set object
     **/
    private Set<String> union(Set<String> set1, Set<String> set2) {
        Set<String> result = new LinkedHashSet<String>();
        result.addAll(set1);
        result.addAll(set2);
        result.remove("*");
        return result;
    }

    /**
       Make a intersection operation on two Sets with the consideration of wildcard.

       @param set1  One Set
       @param set2  Another Set
       
       @return Set<String>  The intersected Set object
     **/
    private Set<String> intersection(Set<String> set1, Set<String> set2) {
        Set<String> result = new LinkedHashSet<String>();
        boolean set1HasWildcard = set1.contains("*");
        boolean set2HasWildcard = set2.contains("*");

        if (set1HasWildcard && set2HasWildcard) {
            //
            // Both Sets have wildcard, the result will have all elements in them
            // 
            result.addAll(set1);
            result.addAll(set2);
        } else if (set1HasWildcard) {
            //
            // Only set1 has wildcard, then result will have only set2 elements.
            // 
            result.addAll(set2);
        } else if (set2HasWildcard) {
            //
            // Only set2 has wildcard, then result will have only set1 elements.
            // 
            result.addAll(set1);
        } else {
            //
            // No wildcard in both Sets, the result will have the elements in both Sets.
            // 
            result.addAll(set1);
            result.retainAll(set2);
        }

        return result;
    }

    /**
       Get target array.

       @return String[]
     **/
    public String[] getTargets() {
        return (String[])targets.toArray(new String[targets.size()]);
    }

    /**
       Get tool chain tag name array.

       @return String[]
     **/
    public String[] getTagnames() {
        return (String[])tagnames.toArray(new String[tagnames.size()]);
    }

    /**
       Get ARCH array.

       @return String[]
     **/
    public String[] getArchs() {
        return (String[])archs.toArray(new String[archs.size()]);
    }

    /**
       Get command name array.

       @return String[]
     **/
    public String[] getCommands() {
        return (String[])commands.toArray(new String[commands.size()]);
    }

    /**
       Override the Object's toString().

       @return String
     **/
    public String toString() {
        return  "  TARGET :" + targets + "\n" + 
                "  TAGNAME:" + tagnames + "\n" + 
                "  ARCH   :" + archs + "\n" + 
                "  COMMAND:" + commands;
    }

    /**
       Remove the wildcard element in the tool chain information because they
       are useless when retrieved.
     **/
    public void normalize() {
        targets.remove("*");
        tagnames.remove("*");
        archs.remove("*");
        commands.remove("*");
    }
}
