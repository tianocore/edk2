/*
 * 
 * Copyright 2002-2004 The Ant-Contrib project
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
package net.sf.antcontrib.cpptasks.devstudio;
import java.io.File;
import java.util.Vector;

import net.sf.antcontrib.cpptasks.CUtil;
import net.sf.antcontrib.cpptasks.compiler.CommandLineLinker;
import net.sf.antcontrib.cpptasks.compiler.LinkType;
import net.sf.antcontrib.cpptasks.types.LibraryTypeEnum;
/**
 * Abstract base class for linkers that try to mimic the command line arguments
 * for the Microsoft (r) Incremental Linker
 * 
 * @author Curt Arnold
 */
public abstract class DevStudioCompatibleLinker extends CommandLineLinker {
    private static String[] defaultflags = new String[]{"/NOLOGO"};
    public DevStudioCompatibleLinker(String command, String identifierArg,
            String outputSuffix) {
        super(command, identifierArg, new String[]{".obj", ".lib", ".res"},
                new String[]{".map", ".pdb", ".lnk", ".dll"}, outputSuffix,
                false, null);
    }
    protected void addBase(long base, Vector args) {
        if (base >= 0) {
            String baseAddr = Long.toHexString(base);
            args.addElement("/BASE:0x" + baseAddr);
        }
    }
    protected void addFixed(Boolean fixed, Vector args) {
        if (fixed != null) {
            if (fixed.booleanValue()) {
                args.addElement("/FIXED");
            } else {
                args.addElement("/FIXED:NO");
            }
        }
    }
    protected void addImpliedArgs(boolean debug, LinkType linkType,
        Vector args, Boolean defaultflag) {
        if(defaultflag != null && defaultflag.booleanValue()){
            for (int i = 0; i < defaultflags.length; i++) {
                args.addElement(defaultflags[i]);
            }
        }
        if (debug) {
            args.addElement("/DEBUG");
        }
        if (linkType.isSharedLibrary()) {
            args.addElement("/DLL");
        }
        /*
         * if(linkType.isSubsystemGUI()) {
         * args.addElement("/SUBSYSTEM:WINDOWS"); } else {
         * if(linkType.isSubsystemConsole()) {
         * args.addElement("/SUBSYSTEM:CONSOLE"); } }
         */
    }
    protected void addIncremental(boolean incremental, Vector args) {
        if (incremental) {
            args.addElement("/INCREMENTAL:YES");
        } else {
            args.addElement("/INCREMENTAL:NO");
        }
    }
    protected void addMap(boolean map, Vector args) {
        if (map) {
            args.addElement("/MAP");
        }
    }
    protected void addStack(int stack, Vector args) {
        if (stack >= 0) {
            String stackStr = Integer.toHexString(stack);
            args.addElement("/STACK:0x" + stackStr);
        }
    }
    /* (non-Javadoc)
     * @see net.sf.antcontrib.cpptasks.compiler.CommandLineLinker#addEntry(int, java.util.Vector)
     */
    protected void addEntry(String entry, Vector args) {
    	if (entry != null) {
    		args.addElement("/ENTRY:" + entry);
    	}
    }
    
    public String getCommandFileSwitch(String commandFile) {
        return "@" + commandFile;
    }
    public File[] getLibraryPath() {
        return CUtil.getPathFromEnvironment("LIB", ";");
    }
    public String[] getLibraryPatterns(String[] libnames, LibraryTypeEnum libType) {
        StringBuffer buf = new StringBuffer();
        String[] patterns = new String[libnames.length];
        for (int i = 0; i < libnames.length; i++) {
            buf.setLength(0);
            buf.append(libnames[i]);
            buf.append(".lib");
            patterns[i] = buf.toString();
        }
        return patterns;
    }
    public int getMaximumCommandLength() {
        return 4096;
    }
    public String[] getOutputFileSwitch(String outputFile) {
        return new String[]{"/OUT:" + outputFile};
    }
    public boolean isCaseSensitive() {
        return false;
    }
}
