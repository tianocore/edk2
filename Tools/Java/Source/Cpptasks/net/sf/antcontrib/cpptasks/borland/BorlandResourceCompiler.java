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
package net.sf.antcontrib.cpptasks.borland;
import java.io.File;
import java.util.Vector;
import net.sf.antcontrib.cpptasks.CCTask;
import net.sf.antcontrib.cpptasks.compiler.CommandLineCompiler;
import net.sf.antcontrib.cpptasks.compiler.CommandLineCompilerConfiguration;
import net.sf.antcontrib.cpptasks.compiler.LinkType;
import net.sf.antcontrib.cpptasks.compiler.Linker;
import net.sf.antcontrib.cpptasks.compiler.Processor;
import net.sf.antcontrib.cpptasks.compiler.ProgressMonitor;
import net.sf.antcontrib.cpptasks.parser.CParser;
import net.sf.antcontrib.cpptasks.parser.Parser;
import net.sf.antcontrib.cpptasks.OptimizationEnum;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.types.Environment;
/**
 * Adapter for the Borland(r) brc32 Resource compiler.
 * 
 * @author Curt Arnold
 */
public class BorlandResourceCompiler extends CommandLineCompiler {
    private static final BorlandResourceCompiler instance = new BorlandResourceCompiler(
            false, null);
    public static BorlandResourceCompiler getInstance() {
        return instance;
    }
    private BorlandResourceCompiler(boolean newEnvironment, Environment env) {
        super("brc32", "c:\\__bogus\\__bogus.rc", new String[]{".rc"},
                new String[]{".h", ".hpp", ".inl"}, ".res", false, null,
                newEnvironment, env);
    }
    protected void addImpliedArgs(final Vector args, 
    		final boolean debug,
            final boolean multithreaded, 
			final boolean exceptions, 
			final LinkType linkType,
			final Boolean rtti,
			final OptimizationEnum optimization,
   final Boolean defaultflag) {
        //
        //  compile only
        //
        args.addElement("-r");
    }
    protected void addWarningSwitch(Vector args, int level) {
    }
    public Processor changeEnvironment(boolean newEnvironment, Environment env) {
        if (newEnvironment || env != null) {
            return new BorlandResourceCompiler(newEnvironment, env);
        }
        return this;
    }
    public void compile(CCTask task, File outputDir, String[] sourceFiles,
            String[] args, String[] endArgs, boolean relentless,
            CommandLineCompilerConfiguration config, ProgressMonitor monitor)
            throws BuildException {
        super.compile(task, outputDir, sourceFiles, args, endArgs, relentless,
                config, monitor);
    }
    /**
     * The include parser for C will work just fine, but we didn't want to
     * inherit from CommandLineCCompiler
     */
    protected Parser createParser(File source) {
        return new CParser();
    }
    protected int getArgumentCountPerInputFile() {
        return 2;
    }
    protected void getDefineSwitch(StringBuffer buffer, String define,
            String value) {
        buffer.append("-d");
        buffer.append(define);
        if (value != null && value.length() > 0) {
            buffer.append('=');
            buffer.append(value);
        }
    }
    protected File[] getEnvironmentIncludePath() {
        return BorlandProcessor.getEnvironmentPath("brc32", 'i',
                new String[]{"..\\include"});
    }
    protected String getIncludeDirSwitch(String includeDir) {
        return BorlandProcessor.getIncludeDirSwitch("-i", includeDir);
    }
    protected String getInputFileArgument(File outputDir, String filename,
            int index) {
        if (index == 0) {
            String outputFileName = getOutputFileName(filename);
            String fullOutputName = new File(outputDir, outputFileName)
                    .toString();
            return "-fo" + fullOutputName;
        }
        return filename;
    }
    public Linker getLinker(LinkType type) {
        return BorlandLinker.getInstance().getLinker(type);
    }
    public int getMaximumCommandLength() {
        return 1024;
    }
    protected int getMaximumInputFilesPerCommand() {
        return 1;
    }
    protected int getTotalArgumentLengthForInputFile(File outputDir,
            String inputFile) {
        String arg1 = getInputFileArgument(outputDir, inputFile, 0);
        String arg2 = getInputFileArgument(outputDir, inputFile, 1);
        return arg1.length() + arg2.length() + 2;
    }
    protected void getUndefineSwitch(StringBuffer buffer, String define) {
    }
}
