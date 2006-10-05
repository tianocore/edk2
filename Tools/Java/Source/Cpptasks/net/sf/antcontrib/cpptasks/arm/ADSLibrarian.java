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

import net.sf.antcontrib.cpptasks.compiler.CommandLineLinker;
import net.sf.antcontrib.cpptasks.compiler.LinkType;
import net.sf.antcontrib.cpptasks.compiler.Linker;
import net.sf.antcontrib.cpptasks.types.LibraryTypeEnum;

/**
 * Adapter for ARM Librarian
 *
 * @author Curt Arnold
 */
public class ADSLibrarian extends CommandLineLinker {

    private static final ADSLibrarian instance = new ADSLibrarian();

    public static ADSLibrarian getInstance() {
      return instance;
    }

    private ADSLibrarian()
    {
        super("armar",null,
          new String[] { ".o" }, new String[0], ".lib", false, null);
    }

    /* (non-Javadoc)
     * @see net.sf.antcontrib.cpptasks.compiler.CommandLineLinker#addBase(long, java.util.Vector)
     */
    protected void addBase(long base, Vector args) {
        // TODO Auto-generated method stub

    }

    /* (non-Javadoc)
     * @see net.sf.antcontrib.cpptasks.compiler.CommandLineLinker#addFixed(java.lang.Boolean, java.util.Vector)
     */
    protected void addFixed(Boolean fixed, Vector args) {
        // TODO Auto-generated method stub

    }

    /* (non-Javadoc)
     * @see net.sf.antcontrib.cpptasks.compiler.CommandLineLinker#addImpliedArgs(boolean, net.sf.antcontrib.cpptasks.compiler.LinkType, java.util.Vector)
     */
    protected void addImpliedArgs(
        boolean debug,
        LinkType linkType,
        Vector args,
        Boolean defaultflag) {
        // TODO Auto-generated method stub

    }

    /* (non-Javadoc)
     * @see net.sf.antcontrib.cpptasks.compiler.CommandLineLinker#addIncremental(boolean, java.util.Vector)
     */
    protected void addIncremental(boolean incremental, Vector args) {
        // TODO Auto-generated method stub

    }

    /* (non-Javadoc)
     * @see net.sf.antcontrib.cpptasks.compiler.CommandLineLinker#addMap(boolean, java.util.Vector)
     */
    protected void addMap(boolean map, Vector args) {
        // TODO Auto-generated method stub

    }

    /* (non-Javadoc)
     * @see net.sf.antcontrib.cpptasks.compiler.CommandLineLinker#addStack(int, java.util.Vector)
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

    /* (non-Javadoc)
     * @see net.sf.antcontrib.cpptasks.compiler.CommandLineLinker#getCommandFileSwitch(java.lang.String)
     */
    protected String getCommandFileSwitch(String commandFile) {
        // TODO Auto-generated method stub
        return null;
    }

    /* (non-Javadoc)
     * @see net.sf.antcontrib.cpptasks.compiler.Linker#getLibraryPath()
     */
    public File[] getLibraryPath() {
        // TODO Auto-generated method stub
        return null;
    }

    /* (non-Javadoc)
     * @see net.sf.antcontrib.cpptasks.compiler.Linker#getLibraryPatterns(java.lang.String[])
     */
    public String[] getLibraryPatterns(String[] libnames, LibraryTypeEnum libType) {
        return new String[0];
    }

    /* (non-Javadoc)
     * @see net.sf.antcontrib.cpptasks.compiler.Processor#getLinker(net.sf.antcontrib.cpptasks.compiler.LinkType)
     */
    public Linker getLinker(LinkType linkType) {
        // TODO Auto-generated method stub
        return null;
    }

    /* (non-Javadoc)
     * @see net.sf.antcontrib.cpptasks.compiler.CommandLineLinker#getMaximumCommandLength()
     */
    protected int getMaximumCommandLength() {
        // TODO Auto-generated method stub
        return 0;
    }

    /* (non-Javadoc)
     * @see net.sf.antcontrib.cpptasks.compiler.CommandLineLinker#getOutputFileSwitch(java.lang.String)
     */
    protected String[] getOutputFileSwitch(String outputFile) {
        // TODO Auto-generated method stub
        return null;
    }

    /* (non-Javadoc)
     * @see net.sf.antcontrib.cpptasks.compiler.Linker#isCaseSensitive()
     */
    public boolean isCaseSensitive() {
        // TODO Auto-generated method stub
        return false;
    }

}
