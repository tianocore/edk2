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
import java.io.File;

import net.sf.antcontrib.cpptasks.CCTask;
import net.sf.antcontrib.cpptasks.CompilerParam;
import net.sf.antcontrib.cpptasks.DependencyInfo;
import net.sf.antcontrib.cpptasks.ProcessorParam;

import org.apache.tools.ant.BuildException;
/**
 * A configuration for a C++ compiler
 * 
 * @author Curt Arnold
 */
public final class CommandLineCompilerConfiguration
        implements
            CompilerConfiguration {
    private/* final */String[] args;
    private/* final */CommandLineCompiler compiler;
    private String[] endArgs;
    //
    //    include path from environment variable not
    //       explicitly stated in Ant script
    private/* final */File[] envIncludePath;
    private String[] exceptFiles;
    private/* final */String identifier;
    private/* final */File[] includePath;
    private/* final */String includePathIdentifier;
    private boolean isPrecompiledHeaderGeneration;
    private/* final */ProcessorParam[] params;
    private/* final */boolean rebuild;
    private/* final */File[] sysIncludePath;
    public CommandLineCompilerConfiguration(CommandLineCompiler compiler,
            String identifier, File[] includePath, File[] sysIncludePath,
            File[] envIncludePath, String includePathIdentifier, String[] args,
            ProcessorParam[] params, boolean rebuild, String[] endArgs) {
        if (compiler == null) {
            throw new NullPointerException("compiler");
        }
        if (identifier == null) {
            throw new NullPointerException("identifier");
        }
        if (includePathIdentifier == null) {
            throw new NullPointerException("includePathIdentifier");
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
        this.compiler = compiler;
        this.params = (ProcessorParam[]) params.clone();
        this.rebuild = rebuild;
        this.identifier = identifier;
        this.includePathIdentifier = includePathIdentifier;
        this.endArgs = (String[]) endArgs.clone();
        exceptFiles = null;
        isPrecompiledHeaderGeneration = false;
    }
    public CommandLineCompilerConfiguration(
            CommandLineCompilerConfiguration base, String[] additionalArgs,
            String[] exceptFiles, boolean isPrecompileHeaderGeneration) {
        compiler = base.compiler;
        identifier = base.identifier;
        rebuild = base.rebuild;
        includePath = (File[]) base.includePath.clone();
        sysIncludePath = (File[]) base.sysIncludePath.clone();
        endArgs = (String[]) base.endArgs.clone();
        envIncludePath = (File[]) base.envIncludePath.clone();
        includePathIdentifier = base.includePathIdentifier;
        if (exceptFiles != null) {
            this.exceptFiles = (String[]) exceptFiles.clone();
        }
        this.isPrecompiledHeaderGeneration = isPrecompileHeaderGeneration;
        args = new String[base.args.length + additionalArgs.length];
        for (int i = 0; i < base.args.length; i++) {
            args[i] = base.args[i];
        }
        int index = base.args.length;
        for (int i = 0; i < additionalArgs.length; i++) {
            args[index++] = additionalArgs[i];
        }
    }
    public int bid(String inputFile) {
        int compilerBid = compiler.bid(inputFile);
        if (compilerBid > 0 && exceptFiles != null) {
            for (int i = 0; i < exceptFiles.length; i++) {
                if (inputFile.equals(exceptFiles[i])) {
                    return 0;
                }
            }
        }
        return compilerBid;
    }
    public void compile(CCTask task, File outputDir, String[] sourceFiles,
            boolean relentless, ProgressMonitor monitor) throws BuildException {
        if (monitor != null) {
            monitor.start(this);
        }
        try {
            compiler.compile(task, outputDir, sourceFiles, args, endArgs,
                    relentless, this, monitor);
            if (monitor != null) {
                monitor.finish(this, true);
            }
        } catch (BuildException ex) {
            if (monitor != null) {
                monitor.finish(this, false);
            }
            throw ex;
        }
    }
    /**
     * 
     * This method may be used to get two distinct compiler configurations, one
     * for compiling the specified file and producing a precompiled header
     * file, and a second for compiling other files using the precompiled
     * header file.
     * 
     * The last (preferrably only) include directive in the prototype file will
     * be used to mark the boundary between pre-compiled and normally compiled
     * headers.
     * 
     * @param prototype
     *            A source file (for example, stdafx.cpp) that is used to build
     *            the precompiled header file. @returns null if precompiled
     *            headers are not supported or a two element array containing
     *            the precompiled header generation configuration and the
     *            consuming configuration
     *  
     */
    public CompilerConfiguration[] createPrecompileConfigurations(
            File prototype, String[] nonPrecompiledFiles) {
        if (compiler instanceof PrecompilingCompiler) {
            return ((PrecompilingCompiler) compiler)
                    .createPrecompileConfigurations(this, prototype,
                            nonPrecompiledFiles);
        }
        return null;
    }
    /**
     * Returns a string representation of this configuration. Should be
     * canonical so that equivalent configurations will have equivalent string
     * representations
     */
    public String getIdentifier() {
        return identifier;
    }
    public String getIncludePathIdentifier() {
        return includePathIdentifier;
    }
    public String getOutputFileName(String inputFile) {
        return compiler.getOutputFileName(inputFile);
    }
    public CompilerParam getParam(String name) {
        for (int i = 0; i < params.length; i++) {
            if (name.equals(params[i].getName()))
                return (CompilerParam) params[i];
        }
        return null;
    }
    public ProcessorParam[] getParams() {
        return params;
    }
    public boolean getRebuild() {
        return rebuild;
    }
    public boolean isPrecompileGeneration() {
        return isPrecompiledHeaderGeneration;
    }
    public DependencyInfo parseIncludes(CCTask task, File baseDir, File source) {
        return compiler.parseIncludes(task, source, includePath,
                sysIncludePath, envIncludePath, baseDir,
                getIncludePathIdentifier());
    }
    public String toString() {
        return identifier;
    }
    public String[] getPreArguments() {
    	return (String[]) args.clone();
    }
    public String[] getEndArguments() {
    	return (String[]) endArgs.clone();
    }
}
