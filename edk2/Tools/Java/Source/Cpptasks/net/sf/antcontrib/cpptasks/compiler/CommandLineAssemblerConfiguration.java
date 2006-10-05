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
 * A configuration for an assember
 * 
 */
public final class CommandLineAssemblerConfiguration implements
                AssemblerConfiguration {

    private String[] args;

    private CommandLineAssembler assembler;

    private String[] endArgs;

    //
    // include path from environment variable
    // not explicitly stated in Ant script
    //
    private File[] envIncludePath;

    private String[] exceptFiles;

    private File[] includePath;

    private boolean rebuild;

    private File[] sysIncludePath;

    public CommandLineAssemblerConfiguration (CommandLineAssembler assembler,
                    File[] includePath, File[] sysIncludePath,
                    File[] envIncludePath, String[] args, boolean rebuild,
                    String[] endArgs, String[] exceptFiles) {
        if (assembler == null) {
            throw new NullPointerException("assembler");
        }
        if (args == null) {
            this.args = new String[0];
        } else {
            this.args = (String[]) args.clone();
        }
        if (includePath == null) {
            this.includePath = new File[0];
        } else {
            this.includePath = (File[]) includePath.clone();
        }
        if (sysIncludePath == null) {
            this.sysIncludePath = new File[0];
        } else {
            this.sysIncludePath = (File[]) sysIncludePath.clone();
        }
        if (envIncludePath == null) {
            this.envIncludePath = new File[0];
        } else {
            this.envIncludePath = (File[]) envIncludePath.clone();
        }
        this.assembler = assembler;
        this.rebuild = rebuild;
        this.endArgs = (String[]) endArgs.clone();
        this.exceptFiles = (String[]) exceptFiles.clone();
    }

    public int bid(String inputFile) {
        int assembleBid = assembler.bid(inputFile);
        return assembleBid;
    }

    public void assembler(CCTask task, File outputDir, String[] sourceFiles)
                    throws BuildException {
        try {
            assembler.assembler(task, outputDir, sourceFiles, args, endArgs);
        } catch (BuildException ex) {
            throw ex;
        }
    }

    public String getOutputFileName(String inputFile) {
        return assembler.getOutputFileName(inputFile);
    }

    public String getIdentifier() {
        return assembler.getCommand();
    }

    public ProcessorParam[] getParams() {
        return new ProcessorParam[0];
    }

    public boolean getRebuild() {
        return rebuild;
    }

    public String[] getPreArguments() {
        return (String[]) args.clone();
    }

    public String[] getEndArguments() {
        return (String[]) endArgs.clone();
    }
}
