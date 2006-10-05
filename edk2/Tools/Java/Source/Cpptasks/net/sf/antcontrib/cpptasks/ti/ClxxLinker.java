/*
 * 
 * Copyright 2001-2004 The Ant-Contrib project
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
package net.sf.antcontrib.cpptasks.ti;
import java.io.File;
import java.util.Vector;

import net.sf.antcontrib.cpptasks.compiler.CommandLineLinker;
import net.sf.antcontrib.cpptasks.compiler.LinkType;
import net.sf.antcontrib.cpptasks.compiler.Linker;
import net.sf.antcontrib.cpptasks.types.LibraryTypeEnum;

/**
 * Adapter for TI DSP linkers
 *  *
 * @author CurtA
 *  
 */
public class ClxxLinker extends CommandLineLinker {
    private static final ClxxLinker cl55DllInstance = new ClxxLinker("lnk55",
            ".dll");
    private static final ClxxLinker cl55Instance = new ClxxLinker("lnk55",
            ".exe");
    private static final ClxxLinker cl6xDllInstance = new ClxxLinker("lnk6x",
            ".dll");
    private static final ClxxLinker cl6xInstance = new ClxxLinker("lnk6x",
            ".exe");
    public static ClxxLinker getCl55DllInstance() {
        return cl55DllInstance;
    }
    public static ClxxLinker getCl55Instance() {
        return cl55Instance;
    }
    public static ClxxLinker getCl6xDllInstance() {
        return cl6xDllInstance;
    }
    public static ClxxLinker getCl6xInstance() {
        return cl6xInstance;
    }
    private ClxxLinker(String command, String outputSuffix) {
        super(command, "-h", new String[]{".o", ".lib", ".res"}, new String[]{
                ".map", ".pdb", ".lnk"}, outputSuffix, false, null);
    }
    /*
     * (non-Javadoc)
     * 
     * @see net.sf.antcontrib.cpptasks.compiler.CommandLineLinker#addBase(long,
     *      java.util.Vector)
     */
    protected void addBase(long base, Vector args) {
    }
    /*
     * (non-Javadoc)
     * 
     * @see net.sf.antcontrib.cpptasks.compiler.CommandLineLinker#addFixed(java.lang.Boolean,
     *      java.util.Vector)
     */
    protected void addFixed(Boolean fixed, Vector args) {
    }
    /*
     * (non-Javadoc)
     * 
     * @see net.sf.antcontrib.cpptasks.compiler.CommandLineLinker#addImpliedArgs(boolean,
     *      net.sf.antcontrib.cpptasks.compiler.LinkType, java.util.Vector)
     */
    protected void addImpliedArgs(boolean debug, LinkType linkType, Vector args, Boolean defaultflag) {
        if (linkType.isSharedLibrary()) {
            args.addElement("-abs");
        }
    }
    /*
     * (non-Javadoc)
     * 
     * @see net.sf.antcontrib.cpptasks.compiler.CommandLineLinker#addIncremental(boolean,
     *      java.util.Vector)
     */
    protected void addIncremental(boolean incremental, Vector args) {
    }
    /*
     * (non-Javadoc)
     * 
     * @see net.sf.antcontrib.cpptasks.compiler.CommandLineLinker#addMap(boolean,
     *      java.util.Vector)
     */
    protected void addMap(boolean map, Vector args) {
        if (map) {
            args.addElement("-m");
        }
    }
    /*
     * (non-Javadoc)
     * 
     * @see net.sf.antcontrib.cpptasks.compiler.CommandLineLinker#addStack(int,
     *      java.util.Vector)
     */
    protected void addStack(int stack, Vector args) {
    }
    /* (non-Javadoc)
     * @see net.sf.antcontrib.cpptasks.compiler.CommandLineLinker#addEntry(int, java.util.Vector)
     */
    protected void addEntry(String entry, Vector args) {
    }
    
    /*
     * (non-Javadoc)
     * 
     * @see net.sf.antcontrib.cpptasks.compiler.CommandLineLinker#getCommandFileSwitch(java.lang.String)
     */
    protected String getCommandFileSwitch(String commandFile) {
        return "@" + commandFile;
    }
    /*
     * (non-Javadoc)
     * 
     * @see net.sf.antcontrib.cpptasks.compiler.Linker#getLibraryPath()
     */
    public File[] getLibraryPath() {
        return new File[0];
    }
    /*
     * (non-Javadoc)
     * 
     * @see net.sf.antcontrib.cpptasks.compiler.Linker#getLibraryPatterns(java.lang.String[])
     */
    public String[] getLibraryPatterns(String[] libnames, LibraryTypeEnum libType) {
    	//
    	//  TODO: Looks bogus, should be .a or .so's not .o's
    	//
    	String[] libpats = new String[libnames.length];
        for (int i = 0; i < libnames.length; i++) {
            libpats[i] = libnames[i] + ".o";
        }
        return libpats;
    }
    /*
     * (non-Javadoc)
     * 
     * @see net.sf.antcontrib.cpptasks.compiler.Processor#getLinker(net.sf.antcontrib.cpptasks.compiler.LinkType)
     */
    public Linker getLinker(LinkType linkType) {
        return this;
    }
    /*
     * (non-Javadoc)
     * 
     * @see net.sf.antcontrib.cpptasks.compiler.CommandLineLinker#getMaximumCommandLength()
     */
    protected int getMaximumCommandLength() {
        return 1024;
    }
    /*
     * (non-Javadoc)
     * 
     * @see net.sf.antcontrib.cpptasks.compiler.CommandLineLinker#getOutputFileSwitch(java.lang.String)
     */
    protected String[] getOutputFileSwitch(String outputFile) {
        return new String[]{"-o", outputFile};
    }
    /*
     * (non-Javadoc)
     * 
     * @see net.sf.antcontrib.cpptasks.compiler.Linker#isCaseSensitive()
     */
    public boolean isCaseSensitive() {
        // TODO Auto-generated method stub
        return false;
    }
}
