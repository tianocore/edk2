/*
 * 
 * Copyright 2001-2005 The Ant-Contrib project
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

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.util.Vector;

import net.sf.antcontrib.cpptasks.compiler.Assembler;
import net.sf.antcontrib.cpptasks.compiler.Processor;
import net.sf.antcontrib.cpptasks.gcc.GccAssembler;
import net.sf.antcontrib.cpptasks.types.AssemblerArgument;
import net.sf.antcontrib.cpptasks.types.ConditionalPath;
import net.sf.antcontrib.cpptasks.types.IncludePath;
import net.sf.antcontrib.cpptasks.types.SystemIncludePath;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Project;

/**
 * A assembler definition. Assembler elements may be placed either as children
 * of a cc element or the project element. A assembler element with an id
 * attribute may be referenced from assembler elements with refid or extends
 * attributes.
 * 
 */
public final class AssemblerDef extends ProcessorDef {

    private final Vector includePaths = new Vector();

    private final Vector sysIncludePaths = new Vector();

    private Boolean defaultflag = new Boolean(true);

    public AssemblerDef () {
    }

    /**
     * Adds a assembler command-line arg.
     */
    public void addConfiguredAssemblerArg(AssemblerArgument arg) {
        if (isReference()) {
            throw noChildrenAllowed();
        }
        addConfiguredProcessorArg(arg);
    }

    /**
     * Creates an include path.
     */
    public IncludePath createIncludePath() {
        Project p = getProject();
        if (p == null) {
            throw new java.lang.IllegalStateException("project must be set");
        }
        if (isReference()) {
            throw noChildrenAllowed();
        }
        IncludePath path = new IncludePath(p);
        includePaths.addElement(path);
        return path;
    }

    /**
     * Creates an include path.
     */
    public SystemIncludePath createSysIncludePath() {
        Project p = getProject();
        if (p == null) {
            throw new java.lang.IllegalStateException("project must be set");
        }
        if (isReference()) {
            throw noChildrenAllowed();
        }
        SystemIncludePath path = new SystemIncludePath(p);
        sysIncludePaths.addElement(path);
        return path;
    }

    /**
     * Add a <includepath>or <sysincludepath> if specify the file attribute
     * 
     * @throws BuildException
     *             if the specify file not exist
     */
    protected void loadFile(Vector activePath, File file) throws BuildException {
        FileReader fileReader;
        BufferedReader in;
        String str;
        if (!file.exists()) {
            throw new BuildException("The file " + file + " is not existed");
        }
        try {
            fileReader = new FileReader(file);
            in = new BufferedReader(fileReader);
            while ((str = in.readLine()) != null) {
                if (str.trim() == "") {
                    continue;
                }
                str = getProject().replaceProperties(str);
                activePath.addElement(str.trim());
            }
        } catch (Exception e) {
            throw new BuildException(e.getMessage());
        }
    }

    public void execute() throws org.apache.tools.ant.BuildException {
        throw new org.apache.tools.ant.BuildException(
                        "Not an actual task, but looks like one for documentation purposes");
    }

    /**
     * Returns the assembler-specific include path.
     */
    public String[] getActiveIncludePaths() {
        if (isReference()) {
            return ((AssemblerDef) getCheckedRef(AssemblerDef.class,
                            "AssemblerDef")).getActiveIncludePaths();
        }
        return getActivePaths(includePaths);
    }

    /**
     * Returns the assembler-specific sysinclude path.
     */
    public String[] getActiveSysIncludePaths() {
        if (isReference()) {
            return ((AssemblerDef) getCheckedRef(AssemblerDef.class,
                            "AssemblerDef")).getActiveSysIncludePaths();
        }
        return getActivePaths(sysIncludePaths);
    }

    private String[] getActivePaths(Vector paths) {
        Project p = getProject();
        if (p == null) {
            throw new java.lang.IllegalStateException("project not set");
        }
        Vector activePaths = new Vector(paths.size());
        for (int i = 0; i < paths.size(); i++) {
            ConditionalPath path = (ConditionalPath) paths.elementAt(i);
            if (path.isActive(p)) {
                if (path.getFile() == null) {
                    String[] pathEntries = path.list();
                    for (int j = 0; j < pathEntries.length; j++) {
                        activePaths.addElement(pathEntries[j]);
                    }
                } else {
                    loadFile(activePaths, path.getFile());
                }
            }
        }
        String[] pathNames = new String[activePaths.size()];
        activePaths.copyInto(pathNames);
        return pathNames;
    }

    public final Boolean getDefaultflag(AssemblerDef[] defaultProviders,
                    int index) {
        if (isReference()) {
            return ((AssemblerDef) getCheckedRef(AssemblerDef.class,
                            "AssemblerDef")).getDefaultflag(defaultProviders,
                            index);
        }
        return defaultflag;
    }

    public Processor getProcessor() {
        Processor processor = super.getProcessor();
        if (processor == null) {
            processor = GccAssembler.getInstance();
        }
        return processor;
    }

    /**
     * Sets r type.
     * 
     * <table width="100%" border="1"> <thead>Supported assemblers</thead>
     * <tr>
     * <td>gcc (default)</td>
     * <td>GAS assembler</td>
     * </tr>
     * <tr>
     * <td>masm</td>
     * <td>MASM assembler</td>
     * </tr>
     * </table>
     * 
     */
    public void setName(AssemblerEnum name) throws BuildException {
        if (isReference()) {
            throw tooManyAttributes();
        }
        Assembler assembler = name.getAssembler();
        setProcessor(assembler);
    }

    protected void setProcessor(Processor proc) throws BuildException {
        try {
            super.setProcessor((Assembler) proc);
        } catch (ClassCastException ex) {
            throw new BuildException(ex);
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
