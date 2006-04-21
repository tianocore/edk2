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

import net.sf.antcontrib.cpptasks.compiler.Aslcompiler;
import net.sf.antcontrib.cpptasks.compiler.Processor;
import net.sf.antcontrib.cpptasks.intel.IntelWin32Aslcompiler;
import net.sf.antcontrib.cpptasks.types.AslcompilerArgument;

import org.apache.tools.ant.BuildException;

/**
 * A asl compiler definition. asl compiler elements may be placed either as
 * children of a cc element or the project element. A asl compiler element with
 * an id attribute may be referenced from asl compiler elements with refid or
 * extends attributes.
 * 
 */
public final class AslcompilerDef extends ProcessorDef {

    private Boolean defaultflag = new Boolean(true);

    public AslcompilerDef () {
    }

    /**
     * Adds a asl compiler command-line arg.
     */
    public void addConfiguredAslcompilerArg(AslcompilerArgument arg) {
        if (isReference()) {
            throw noChildrenAllowed();
        }
        addConfiguredProcessorArg(arg);
    }

    public void execute() throws org.apache.tools.ant.BuildException {
        throw new org.apache.tools.ant.BuildException(
                        "Not an actual task, but looks like one for documentation purposes");
    }

    public final Boolean getDefaultflag(AslcompilerDef[] defaultProviders,
                    int index) {
        if (isReference()) {
            return ((AslcompilerDef) getCheckedRef(AslcompilerDef.class,
                            "AslcompilerDef")).getDefaultflag(defaultProviders,
                            index);
        }
        return defaultflag;
    }

    public Processor getProcessor() {
        Processor processor = super.getProcessor();
        if (processor == null) {
            processor = IntelWin32Aslcompiler.getInstance();
        }
        return processor;
    }

    /**
     * Sets r type.
     * 
     * <table width="100%" border="1"> <thead>Supported ASL Compilers</thead>
     * <tr>
     * <td>iasl (default)</td>
     * <td>Intel ACPI Source Language</td>
     * </tr>
     * <tr>
     * <td>asl</td>
     * <td>Microsoft ACPI Source Language</td>
     * </tr>
     * </table>
     * 
     */
    public void setName(AslcompilerEnum name) throws BuildException {
        if (isReference()) {
            throw tooManyAttributes();
        }
        Aslcompiler aslcompiler = name.getAslcompiler();
        setProcessor(aslcompiler);
    }

    protected void setProcessor(Processor proc) throws BuildException {
        try {
            super.setProcessor((Aslcompiler) proc);
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
