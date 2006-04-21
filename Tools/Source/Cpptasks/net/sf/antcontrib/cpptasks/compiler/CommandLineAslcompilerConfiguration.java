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
package net.sf.antcontrib.cpptasks.compiler;

import java.io.File;

import org.apache.tools.ant.BuildException;

import net.sf.antcontrib.cpptasks.CCTask;
import net.sf.antcontrib.cpptasks.ProcessorParam;

/**
 * A configuration for an ASL compiler
 * 
 */
public final class CommandLineAslcompilerConfiguration implements
                AslcompilerConfiguration {

    private String[] args;

    private CommandLineAslcompiler acpi;

    private String[] endArgs;

    private boolean rebuild;

    public CommandLineAslcompilerConfiguration (CommandLineAslcompiler acpi,
                    String[] args, boolean rebuild, String[] endArgs) {
        if (acpi == null) {
            throw new NullPointerException("acpi");
        }
        if (args == null) {
            this.args = new String[0];
        } else {
            this.args = (String[]) args.clone();
        }
        this.acpi = acpi;
        this.rebuild = rebuild;
        this.endArgs = (String[]) endArgs.clone();
    }

    public int bid (String inputFile) {
        int acpiBid = acpi.bid(inputFile);
        return acpiBid;
    }

    public void aslcompiler (CCTask task, File outputDir, String[] sourceFiles)
                    throws BuildException {
        try {
            acpi.aslcompiler(task, outputDir, sourceFiles, args, endArgs);
        } catch (BuildException ex) {
            throw ex;
        }
    }

    public String getIdentifier () {
        return acpi.getCommand();
    }

    public ProcessorParam[] getParams () {
        return new ProcessorParam[0];
    }

    public boolean getRebuild () {
        return rebuild;
    }

    public String[] getPreArguments () {
        return (String[]) args.clone();
    }

    public String[] getEndArguments () {
        return (String[]) endArgs.clone();
    }

    public String getOutputFileName (String inputFile) {
        return acpi.getOutputFileName(inputFile);
    }
}
