/*
 * 
 * Copyright 2003-2004 The Ant-Contrib project
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
package net.sf.antcontrib.cpptasks.arm;
import java.io.File;
import java.util.Vector;

import net.sf.antcontrib.cpptasks.CUtil;
import net.sf.antcontrib.cpptasks.compiler.CommandLineLinker;
import net.sf.antcontrib.cpptasks.compiler.LinkType;
import net.sf.antcontrib.cpptasks.compiler.Linker;
import net.sf.antcontrib.cpptasks.types.LibraryTypeEnum;

/**
 * Adapter for the ARM Linker
 * 
 * @author CurtA
 */
public class ADSLinker extends CommandLineLinker {
    private static final ADSLinker dllInstance = new ADSLinker(".o");
    private static final ADSLinker instance = new ADSLinker(".axf");
    public static ADSLinker getDllInstance() {
        return dllInstance;
    }
    public static ADSLinker getInstance() {
        return instance;
    }
    private ADSLinker(String outputSuffix) {
        super("armlink", "-vsn", new String[]{".o", ".lib", ".res"},
                new String[]{".map", ".pdb", ".lnk"}, outputSuffix, false, null);
    }
    /*
     * (non-Javadoc)
     * 
     * @see net.sf.antcontrib.cpptasks.compiler.CommandLineLinker#addBase(long,
     *      java.util.Vector)
     */
    protected void addBase(long base, Vector args) {
        // TODO Auto-generated method stub
    }
    /*
     * (non-Javadoc)
     * 
     * @see net.sf.antcontrib.cpptasks.compiler.CommandLineLinker#addFixed(java.lang.Boolean,
     *      java.util.Vector)
     */
    protected void addFixed(Boolean fixed, Vector args) {
        // TODO Auto-generated method stub
    }
    /*
     * (non-Javadoc)
     * 
     * @see net.sf.antcontrib.cpptasks.compiler.CommandLineLinker#addImpliedArgs(boolean,
     *      net.sf.antcontrib.cpptasks.compiler.LinkType, java.util.Vector)
     */
    protected void addImpliedArgs(boolean debug, LinkType linkType, Vector args, Boolean defaultflag) {
        if (debug) {
            args.addElement("-debug");
        }
    }
    /*
     * (non-Javadoc)
     * 
     * @see net.sf.antcontrib.cpptasks.compiler.CommandLineLinker#addIncremental(boolean,
     *      java.util.Vector)
     */
    protected void addIncremental(boolean incremental, Vector args) {
        // TODO Auto-generated method stub
    }
    /*
     * (non-Javadoc)
     * 
     * @see net.sf.antcontrib.cpptasks.compiler.CommandLineLinker#addMap(boolean,
     *      java.util.Vector)
     */
    protected void addMap(boolean map, Vector args) {
        // TODO Auto-generated method stub
    }
    /*
     * (non-Javadoc)
     * 
     * @see net.sf.antcontrib.cpptasks.compiler.CommandLineLinker#addStack(int,
     *      java.util.Vector)
     */
    protected void addStack(int stack, Vector args) {
        // TODO Auto-generated method stub
    }
    /* (non-Javadoc)
     * @see net.sf.antcontrib.cpptasks.compiler.CommandLineLinker#addEntry(int, java.util.Vector)
     */
    protected void addEntry(String entry, Vector args) {
        // TODO Auto-generated method stub

    }
    
    /**
     * May have to make this String array return
     * 
     * @see net.sf.antcontrib.cpptasks.compiler.CommandLineLinker#getCommandFileSwitch(java.lang.String)
     */
    protected String getCommandFileSwitch(String commandFile) {
        return "-via" + commandFile;
    }
    /*
     * (non-Javadoc)
     * 
     * @see net.sf.antcontrib.cpptasks.compiler.Linker#getLibraryPath()
     */
    public File[] getLibraryPath() {
        return CUtil.getPathFromEnvironment("ARMLIB", ";");
    }
    /*
     * @see net.sf.antcontrib.cpptasks.compiler.Linker#getLibraryPatterns(java.lang.String[])
     */
    public String[] getLibraryPatterns(String[] libnames, LibraryTypeEnum libType) {
    	//
    	//  TODO: looks like bad extension
    	//
        return new String[]{".o"};
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
        return new String[]{"-output", outputFile};
    }
    /*
     * (non-Javadoc)
     * 
     * @see net.sf.antcontrib.cpptasks.compiler.Linker#isCaseSensitive()
     */
    public boolean isCaseSensitive() {
        return false;
    }
}
