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
package net.sf.antcontrib.cpptasks.compiler;
import net.sf.antcontrib.cpptasks.CCTask;
import net.sf.antcontrib.cpptasks.LinkerParam;
import net.sf.antcontrib.cpptasks.ProcessorParam;
import net.sf.antcontrib.cpptasks.TargetInfo;

import org.apache.tools.ant.BuildException;
/**
 * A configuration for a command line linker
 * 
 * @author Curt Arnold
 */
public final class CommandLineLinkerConfiguration
        implements
            LinkerConfiguration {
    private/* final */String[][] args;
    private/* final */String identifier;
    private String[] libraryNames;
    private/* final */CommandLineLinker linker;
    private/* final */boolean map;
    private/* final */ProcessorParam[] params;
    private/* final */boolean rebuild;
    private String startupObject;
    public CommandLineLinkerConfiguration(CommandLineLinker linker,
            String identifier, String[][] args, ProcessorParam[] params,
            boolean rebuild, boolean map, String[] libraryNames,
            String startupObject) {
        if (linker == null) {
            throw new NullPointerException("linker");
        }
        if (args == null) {
            throw new NullPointerException("args");
        } else {
            this.args = (String[][]) args.clone();
        }
        this.linker = linker;
        this.params = (ProcessorParam[]) params.clone();
        this.rebuild = rebuild;
        this.identifier = identifier;
        this.map = map;
        if (libraryNames == null) {
            this.libraryNames = new String[0];
        } else {
            this.libraryNames = (String[]) libraryNames.clone();
        }
        this.startupObject = startupObject;
    }
    public int bid(String filename) {
        return linker.bid(filename);
    }
    public String[] getEndArguments() {
        String[] clone = (String[]) args[1].clone();
        return clone;
    }
    /**
     * Returns a string representation of this configuration. Should be
     * canonical so that equivalent configurations will have equivalent string
     * representations
     */
    public String getIdentifier() {
        return identifier;
    }
    public String[] getLibraryNames() {
        String[] clone = (String[]) libraryNames.clone();
        return clone;
    }
    public boolean getMap() {
        return map;
    }
    public String getOutputFileName(String inputFile) {
        return linker.getOutputFileName(inputFile);
    }
    public LinkerParam getParam(String name) {
        for (int i = 0; i < params.length; i++) {
            if (name.equals(params[i].getName()))
                return (LinkerParam) params[i];
        }
        return null;
    }
    public ProcessorParam[] getParams() {
        return params;
    }
    public String[] getPreArguments() {
        String[] clone = (String[]) args[0].clone();
        return clone;
    }
    public boolean getRebuild() {
        return rebuild;
    }
    public String getStartupObject() {
        return startupObject;
    }
    public void link(CCTask task, TargetInfo linkTarget) throws BuildException {
        //
        //  AllSourcePath's include any syslibsets
        //
        String[] sourcePaths = linkTarget.getAllSourcePaths();
        linker.link(task, linkTarget.getOutput(), sourcePaths, this);
    }
    public String toString() {
        return identifier;
    }
}
