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
package net.sf.antcontrib.cpptasks.sun;
import java.io.File;
import java.util.Vector;

import net.sf.antcontrib.cpptasks.CCTask;
import net.sf.antcontrib.cpptasks.CUtil;
import net.sf.antcontrib.cpptasks.compiler.CommandLineLinker;
import net.sf.antcontrib.cpptasks.compiler.LinkType;
import net.sf.antcontrib.cpptasks.compiler.Linker;
import net.sf.antcontrib.cpptasks.types.LibrarySet;
import net.sf.antcontrib.cpptasks.types.LibraryTypeEnum;

/**
 * Adapter for the Sun C89 Linker
 * 
 * @author Hiram Chirino (cojonudo14@hotmail.com)
 */
public final class C89Linker extends CommandLineLinker {
    private static final C89Linker dllLinker = new C89Linker("lib", ".so");
    private static final C89Linker instance = new C89Linker("", "");
    public static C89Linker getInstance() {
        return instance;
    }
    private String outputPrefix;
    private C89Linker(String outputPrefix, String outputSuffix) {
        super("ld", "/bogus", new String[]{".o", ".a", ".lib", ".x"},
                new String[]{}, outputSuffix, false, null);
        this.outputPrefix = outputPrefix;
    }
    protected void addBase(long base, Vector args) {
    }
    protected void addFixed(Boolean fixed, Vector args) {
    }
    protected void addImpliedArgs(boolean debug, LinkType linkType, Vector args, Boolean defaultflag) {
        if (linkType.isSharedLibrary()) {
            args.addElement("-G");
        }
    }
    protected void addIncremental(boolean incremental, Vector args) {
    }
    public String[] addLibrarySets(CCTask task, LibrarySet[] libsets,
            Vector preargs, Vector midargs, Vector endargs) {
        super.addLibrarySets(task, libsets, preargs, midargs, endargs);
        StringBuffer buf = new StringBuffer("-l");
        for (int i = 0; i < libsets.length; i++) {
            LibrarySet set = libsets[i];
            File libdir = set.getDir(null);
            String[] libs = set.getLibs();
            if (libdir != null) {
                endargs.addElement("-L");
                endargs.addElement(libdir.getAbsolutePath());
            }
            for (int j = 0; j < libs.length; j++) {
                //
                //  reset the buffer to just "-l"
                //
                buf.setLength(2);
                //
                //  add the library name
                buf.append(libs[j]);
                //
                //  add the argument to the list
                endargs.addElement(buf.toString());
            }
        }
        return null;
    }
    protected void addMap(boolean map, Vector args) {
    }
    protected void addStack(int stack, Vector args) {
    }
    /* (non-Javadoc)
     * @see net.sf.antcontrib.cpptasks.compiler.CommandLineLinker#addEntry(int, java.util.Vector)
     */
    protected void addEntry(String entry, Vector args) {
    }
    
    public String getCommandFileSwitch(String commandFile) {
        return "@" + commandFile;
    }
    public File[] getLibraryPath() {
        return CUtil.getPathFromEnvironment("LIB", ";");
    }
    public String[] getLibraryPatterns(String[] libnames, LibraryTypeEnum libType) {
        return C89Processor.getLibraryPatterns(libnames, libType);
    }
    public Linker getLinker(LinkType linkType) {
        if (linkType.isSharedLibrary()) {
            return dllLinker;
        }
        /*
         * if(linkType.isStaticLibrary()) { return
         * OS390Librarian.getInstance(); }
         */
        return instance;
    }
    public int getMaximumCommandLength() {
        return Integer.MAX_VALUE;
    }
    public String getOutputFileName(String baseName) {
        return outputPrefix + super.getOutputFileName(baseName);
    }
    public String[] getOutputFileSwitch(String outputFile) {
        return new String[]{"-o", outputFile};
    }
    public boolean isCaseSensitive() {
        return C89Processor.isCaseSensitive();
    }
}
