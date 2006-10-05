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
package net.sf.antcontrib.cpptasks;
import java.io.File;
import java.util.Enumeration;
import java.util.Vector;

import net.sf.antcontrib.cpptasks.compiler.CommandLineLinker;
import net.sf.antcontrib.cpptasks.compiler.LinkType;
import net.sf.antcontrib.cpptasks.compiler.Linker;
import net.sf.antcontrib.cpptasks.compiler.Processor;
import net.sf.antcontrib.cpptasks.compiler.ProcessorConfiguration;
import net.sf.antcontrib.cpptasks.gcc.GccLinker;
import net.sf.antcontrib.cpptasks.types.FlexLong;
import net.sf.antcontrib.cpptasks.types.LibrarySet;
import net.sf.antcontrib.cpptasks.types.LinkerArgument;
import net.sf.antcontrib.cpptasks.types.SystemLibrarySet;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Project;
import org.apache.tools.ant.types.FlexInteger;
/**
 * A linker definition. linker elements may be placed either as children of a
 * cc element or the project element. A linker element with an id attribute may
 * be referenced by linker elements with refid or extends attributes.
 * 
 * @author Adam Murdoch
 * @author Curt Arnold
 */
public class LinkerDef extends ProcessorDef {
    private long base;
    private String entry;
    private Boolean fixed;
    private Boolean incremental;
    private final Vector librarySets = new Vector();
    private Boolean map;
    private int stack;
    private final Vector sysLibrarySets = new Vector();
    private final Vector versionInfos = new Vector();
    private Boolean defaultflag = new Boolean(true);
    /**
     * Default constructor
     * 
     * @see java.lang.Object#Object()
     */
    public LinkerDef() {
        base = -1;
        stack = -1;
    }
    private void addActiveLibrarySet(Project project, Vector libsets,
            Vector srcSets) {
        Enumeration srcenum = srcSets.elements();
        while (srcenum.hasMoreElements()) {
            LibrarySet set = (LibrarySet) srcenum.nextElement();
            if (set.isActive(project)) {
                libsets.addElement(set);
            }
        }
    }
    private void addActiveSystemLibrarySets(Project project, Vector libsets) {
        addActiveLibrarySet(project, libsets, sysLibrarySets);
    }
    private void addActiveUserLibrarySets(Project project, Vector libsets) {
        addActiveLibrarySet(project, libsets, librarySets);
    }
    /**
     * Adds a linker command-line arg.
     */
    public void addConfiguredLinkerArg(LinkerArgument arg) {
        addConfiguredProcessorArg(arg);
    }
    /**
     * Adds a compiler command-line arg.
     */
    public void addConfiguredLinkerParam(LinkerParam param) {
        if (isReference()) {
            throw noChildrenAllowed();
        }
        addConfiguredProcessorParam(param);
    }
    /**
     * Adds a system library set.
     */
    public void addLibset(LibrarySet libset) {
        if (isReference()) {
            throw super.noChildrenAllowed();
        }
        if (libset == null) {
            throw new NullPointerException("libset");
        }
        librarySets.addElement(libset);
    }
    /**
     * Adds a system library set.
     */
    public void addSyslibset(SystemLibrarySet libset) {
        if (isReference()) {
            throw super.noChildrenAllowed();
        }
        if (libset == null) {
            throw new NullPointerException("libset");
        }
        sysLibrarySets.addElement(libset);
    }
    
    /**
     * Adds desriptive version information to be included in the
     * generated file.  The first active version info block will
     * be used.
     */
    public void addConfiguredVersioninfo(VersionInfo newVersionInfo) {
        if (isReference()) {
            throw noChildrenAllowed();
        }
        newVersionInfo.setProject(this.getProject());
        versionInfos.addElement(newVersionInfo);
    }
    
    public ProcessorConfiguration createConfiguration(CCTask task,
            LinkType linkType, ProcessorDef baseDef, TargetDef targetPlatform) {
        //
        //    must combine some local context (the linkType)
        //       with the referenced element
        //
        //    get a pointer to the definition (either local or referenced)
        ProcessorDef thisDef = this;
        if (isReference()) {
            thisDef = ((ProcessorDef) getCheckedRef(ProcessorDef.class,
                    "ProcessorDef"));
        }
        //
        //    find the appropriate processor (combines local linkType
        //       with possibly remote linker name)
        Processor proc = getProcessor();
        proc = proc.getLinker(linkType);
        ProcessorDef[] defaultProviders = getDefaultProviders(baseDef);
        return proc.createConfiguration(task, linkType, defaultProviders,
                thisDef, targetPlatform);
    }
    public void execute() throws org.apache.tools.ant.BuildException {
        throw new org.apache.tools.ant.BuildException(
                "Not an actual task, but looks like one for documentation purposes");
    }
    /**
     * Returns an array of active library sets for this linker definition.
     */
    public LibrarySet[] getActiveLibrarySets(LinkerDef[] defaultProviders,
            int index) {    	
        if (isReference()) {
            return ((LinkerDef) getCheckedRef(LinkerDef.class, "LinkerDef"))
                    .getActiveUserLibrarySets(defaultProviders, index);
        }
        Project p = getProject();
        Vector libsets = new Vector();
        for (int i = index; i < defaultProviders.length; i++) {
            defaultProviders[i].addActiveUserLibrarySets(p, libsets);
            defaultProviders[i].addActiveSystemLibrarySets(p, libsets);
        }
        addActiveUserLibrarySets(p, libsets);
        addActiveSystemLibrarySets(p, libsets);
        LibrarySet[] sets = new LibrarySet[libsets.size()];
        libsets.copyInto(sets);
        return sets;
    }
    /**
     * Returns an array of active library sets for this linker definition.
     */
    public LibrarySet[] getActiveSystemLibrarySets(
            LinkerDef[] defaultProviders, int index) {
        if (isReference()) {
            return ((LinkerDef) getCheckedRef(LinkerDef.class, "LinkerDef"))
                    .getActiveUserLibrarySets(defaultProviders, index);
        }
        Project p = getProject();
        Vector libsets = new Vector();
        for (int i = index; i < defaultProviders.length; i++) {
            defaultProviders[i].addActiveSystemLibrarySets(p, libsets);
        }
        addActiveSystemLibrarySets(p, libsets);
        LibrarySet[] sets = new LibrarySet[libsets.size()];
        libsets.copyInto(sets);
        return sets;
    }
    /**
     * Returns an array of active library sets for this linker definition.
     */
    public LibrarySet[] getActiveUserLibrarySets(LinkerDef[] defaultProviders,
            int index) {
        if (isReference()) {
            return ((LinkerDef) getCheckedRef(LinkerDef.class, "LinkerDef"))
                    .getActiveUserLibrarySets(defaultProviders, index);
        }
        Project p = getProject();
        Vector libsets = new Vector();
        for (int i = index; i < defaultProviders.length; i++) {
            defaultProviders[i].addActiveUserLibrarySets(p, libsets);
        }
        addActiveUserLibrarySets(p, libsets);
        LibrarySet[] sets = new LibrarySet[libsets.size()];
        libsets.copyInto(sets);
        return sets;
    }
    public long getBase(LinkerDef[] defaultProviders, int index) {
        if (isReference()) {
            return ((LinkerDef) getCheckedRef(LinkerDef.class, "LinkerDef"))
                    .getBase(defaultProviders, index);
        }
        if (base <= 0) {
            if (defaultProviders != null && index < defaultProviders.length) {
                return defaultProviders[index].getBase(defaultProviders,
                        index + 1);
            }
        }
        return base;
    }
    public Boolean getFixed(LinkerDef[] defaultProviders, int index) {
        if (isReference()) {
            return ((LinkerDef) getCheckedRef(LinkerDef.class, "LinkerDef"))
                    .getFixed(defaultProviders, index);
        }
        if (fixed == null) {
            if (defaultProviders != null && index < defaultProviders.length) {
                return defaultProviders[index].getFixed(defaultProviders,
                        index + 1);
            }
        }
        return fixed;
    }
    public boolean getIncremental(LinkerDef[] defaultProviders, int index) {
        if (isReference()) {
            return ((LinkerDef) getCheckedRef(LinkerDef.class, "LinkerDef"))
                    .getIncremental(defaultProviders, index);
        }
        if (incremental != null) {
            return incremental.booleanValue();
        }
        if (defaultProviders != null && index < defaultProviders.length) {
            return defaultProviders[index].getIncremental(defaultProviders, index + 1);
        }
        return false;
    }
    public boolean getMap(LinkerDef[] defaultProviders, int index) {
        if (isReference()) {
            return ((LinkerDef) getCheckedRef(LinkerDef.class, "LinkerDef"))
                    .getMap(defaultProviders, index);
        }
        if (map != null) {
            return map.booleanValue();
        }
        if (defaultProviders != null && index < defaultProviders.length) {
            return defaultProviders[index].getMap(defaultProviders, index + 1);
        }
        return false;
    }
    public final Boolean getDefaultflag(LinkerDef[] defaultProviders, int index) {
        if (isReference()) {
            return ((LinkerDef) getCheckedRef(LinkerDef.class,
                  "LinkerDef")).getDefaultflag(defaultProviders, index);
        }
        return defaultflag;
    }
    public String getEntry(LinkerDef[] defaultProviders, int index) {
        if (isReference()) {
            return ((LinkerDef) getCheckedRef(LinkerDef.class, "LinkerDef"))
                    .getEntry(defaultProviders, index);
        }
        if (entry != null) {
            return entry;
        }
        if (defaultProviders != null && index < defaultProviders.length) {
            return defaultProviders[index].getEntry(defaultProviders, index + 1);
        }
        return null;
    }

    public Processor getProcessor() {
        Linker linker = (Linker) super.getProcessor();
        if (linker == null) {
            linker = GccLinker.getInstance();
        }
        if (getLibtool() && linker instanceof CommandLineLinker) {
            CommandLineLinker cmdLineLinker = (CommandLineLinker) linker;
            linker = cmdLineLinker.getLibtoolLinker();
        }
        return linker;
    }
    public int getStack(LinkerDef[] defaultProviders, int index) {
        if (isReference()) {
            return ((LinkerDef) getCheckedRef(LinkerDef.class, "LinkerDef"))
                    .getStack(defaultProviders, index);
        }
        if (stack < 0) {
            if (defaultProviders != null && index < defaultProviders.length) {
                return defaultProviders[index].getStack(defaultProviders,
                        index + 1);
            }
        }
        return stack;
    }
    /**
     * Sets the base address. May be specified in either decimal or hex.
     * 
     * @param base
     *            base address
     *  
     */
    public void setBase(FlexLong base) {
        if (isReference()) {
            throw tooManyAttributes();
        }
        this.base = base.longValue();
    }
    /**
     * Sets the starting address.
     * 
     * @param name
     *            function name
     */
    public void setEntry(String entry) {
        if (isReference()) {
            throw tooManyAttributes();
        }
        this.entry = entry;
    }
    /**
     * If true, marks the file to be loaded only at its preferred address.
     */
    public void setFixed(boolean fixed) {
        if (isReference()) {
            throw tooManyAttributes();
        }
        this.fixed = booleanValueOf(fixed);
    }
    /**
     * If true, allows incremental linking.
     *  
     */
    public void setIncremental(boolean incremental) {
        if (isReference()) {
            throw tooManyAttributes();
        }
        this.incremental = booleanValueOf(incremental);
    }
    /**
     * If set to true, a map file will be produced.
     */
    public void setMap(boolean map) {
        if (isReference()) {
            throw tooManyAttributes();
        }
        this.map = booleanValueOf(map);
    }
    /**
     * Sets linker type.
     * 
     * 
     * <table width="100%" border="1"> <thead>Supported linkers </thead>
     * <tr>
     * <td>gcc</td>
     * <td>Gcc Linker</td>
     * </tr>
     * <tr>
     * <td>g++</td>
     * <td>G++ Linker</td>
     * </tr>
     * <tr>
     * <td>ld</td>
     * <td>Ld Linker</td>
     * </tr>
     * <tr>
     * <td>ar</td>
     * <td>Gcc Librarian</td>
     * </tr>
     * <tr>
     * <td>msvc</td>
     * <td>Microsoft Linker</td>
     * </tr>
     * <tr>
     * <td>bcc</td>
     * <td>Borland Linker</td>
     * </tr>
     * <tr>
     * <td>df</td>
     * <td>Compaq Visual Fortran Linker</td>
     * </tr>
     * <tr>
     * <td>icl</td>
     * <td>Intel Linker for Windows (IA-32)</td>
     * </tr>
     * <tr>
     * <td>ecl</td>
     * <td>Intel Linker for Windows (IA-64)</td>
     * </tr>
     * <tr>
     * <td>icc</td>
     * <td>Intel Linker for Linux (IA-32)</td>
     * </tr>
     * <tr>
     * <td>ecc</td>
     * <td>Intel Linker for Linux (IA-64)</td>
     * </tr>
     * <tr>
     * <td>CC</td>
     * <td>Sun ONE Linker</td>
     * </tr>
     * <tr>
     * <td>aCC</td>
     * <td>HP aC++ Linker</td>
     * </tr>
     * <tr>
     * <td>os390</td>
     * <td>OS390 Linker</td>
     * </tr>
     * <tr>
     * <td>os390batch</td>
     * <td>OS390 Linker</td>
     * </tr>
     * <tr>
     * <td>os400</td>
     * <td>IccLinker</td>
     * </tr>
     * <tr>
     * <td>sunc89</td>
     * <td>C89 Linker</td>
     * </tr>
     * <tr>
     * <td>xlC</td>
     * <td>VisualAge Linker</td>
     * </tr>
     * </table>
     *  
     */
    public void setName(LinkerEnum name) throws BuildException {
        if (isReference()) {
            throw tooManyAttributes();
        }
        Linker linker = name.getLinker();
        super.setProcessor(linker);
    }
    protected void setProcessor(Processor proc) throws BuildException {
        Linker linker = null;
        if (proc instanceof Linker) {
            linker = (Linker) proc;
        } else {
            LinkType linkType = new LinkType();
            linker = proc.getLinker(linkType);
        }
        super.setProcessor(linker);
    }
    /**
     * Sets stack size in bytes.
     */
    public void setStack(FlexInteger stack) {
        if (isReference()) {
            throw tooManyAttributes();
        }
        this.stack = stack.intValue();
    }
    public void visitSystemLibraries(Linker linker, FileVisitor libraryVisitor) {
    	Project p = getProject();
        if (p == null) {
            throw new java.lang.IllegalStateException("project must be set");
        }
        if (isReference()) {
            LinkerDef master = ((LinkerDef) getCheckedRef(LinkerDef.class,
                    "Linker"));
            master.visitSystemLibraries(linker, libraryVisitor);
        } else {
            //
            //   if this linker extends another,
            //      visit its libraries first
            //
            LinkerDef extendsDef = (LinkerDef) getExtends();
            if (extendsDef != null) {
                extendsDef.visitSystemLibraries(linker, libraryVisitor);
            }
            if (sysLibrarySets.size() > 0) {
                File[] libpath = linker.getLibraryPath();
                for (int i = 0; i < sysLibrarySets.size(); i++) {
                    LibrarySet set = (LibrarySet) sysLibrarySets.elementAt(i);
                    if (set.isActive(p)) {
                        set.visitLibraries(p, linker, libpath,
                                libraryVisitor);
                    }
                }
            }
        }
    }
    public void visitUserLibraries(Linker linker, FileVisitor libraryVisitor) {
    	Project p = getProject();
    	if (p == null) {
            throw new java.lang.IllegalStateException("project must be set");
        }
        if (isReference()) {
            LinkerDef master = ((LinkerDef) getCheckedRef(LinkerDef.class,
                    "Linker"));
            master.visitUserLibraries(linker, libraryVisitor);
        } else {
            //
            //   if this linker extends another,
            //      visit its libraries first
            //
            LinkerDef extendsDef = (LinkerDef) getExtends();
            if (extendsDef != null) {
                extendsDef.visitUserLibraries(linker, libraryVisitor);
            }
            //
            //   visit the user libraries
            //
            if (librarySets.size() > 0) {
                File[] libpath = linker.getLibraryPath();
                for (int i = 0; i < librarySets.size(); i++) {
                    LibrarySet set = (LibrarySet) librarySets.elementAt(i);
                    if (set.isActive(p)) {
                        set.visitLibraries(p, linker, libpath,
                                libraryVisitor);
                    }
                }
            }
        }
    }
    /**
     * Enables or disables default flags.
     * 
     * @param defaultflag
     *            if true, default flags will add to command line.
     *  
     */
    public void setDefaultflag(boolean defaultflag) {
        if (isReference()) {
            throw tooManyAttributes();
        }
        this.defaultflag = booleanValueOf(defaultflag);
    }
}
