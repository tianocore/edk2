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
package net.sf.antcontrib.cpptasks.devstudio;
import java.io.File;
import java.util.Vector;

import net.sf.antcontrib.cpptasks.CUtil;
import net.sf.antcontrib.cpptasks.compiler.CommandLineCompiler;
import net.sf.antcontrib.cpptasks.compiler.LinkType;
import net.sf.antcontrib.cpptasks.compiler.Linker;
import net.sf.antcontrib.cpptasks.compiler.Processor;
import net.sf.antcontrib.cpptasks.parser.CParser;
import net.sf.antcontrib.cpptasks.parser.Parser;
import net.sf.antcontrib.cpptasks.OptimizationEnum;

import org.apache.tools.ant.types.Environment;
/**
 * Adapter for the Microsoft (r) Windows 32 Resource Compiler
 * 
 * @author Curt Arnold
 */
public final class DevStudioResourceCompiler extends CommandLineCompiler {
    private static final DevStudioResourceCompiler instance = new DevStudioResourceCompiler(
            false, null);
    public static DevStudioResourceCompiler getInstance() {
        return instance;
    }
    private String identifier;
    private DevStudioResourceCompiler(boolean newEnvironment, Environment env) {
        super("rc", null, new String[]{".rc"}, new String[]{".h", ".hpp",
                ".inl"}, ".res", false, null, newEnvironment, env);
    }
    protected void addImpliedArgs(final Vector args, 
    		final boolean debug,
            final boolean multithreaded, 
			final boolean exceptions, 
			final LinkType linkType,
			final Boolean rtti,
			final OptimizationEnum optimization,
   final Boolean defaultflag) {
        if (debug) {
            args.addElement("/D_DEBUG");
        } else {
            args.addElement("/DNDEBUG");
        }
    }
    protected void addWarningSwitch(Vector args, int level) {
    }
    public Processor changeEnvironment(boolean newEnvironment, Environment env) {
        if (newEnvironment || env != null) {
            return new DevStudioResourceCompiler(newEnvironment, env);
        }
        return this;
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
        DevStudioProcessor.getDefineSwitch(buffer, define, value);
    }
    protected File[] getEnvironmentIncludePath() {
        return CUtil.getPathFromEnvironment("INCLUDE", ";");
    }
    protected String getIncludeDirSwitch(String includeDir) {
        return DevStudioProcessor.getIncludeDirSwitch(includeDir);
    }
    protected String getInputFileArgument(File outputDir, String filename,
            int index) {
        if (index == 0) {
            String outputFileName = getOutputFileName(filename);
            String fullOutputName = new File(outputDir, outputFileName)
                    .toString();
            return "/fo" + fullOutputName;
        }
        return filename;
    }
    public Linker getLinker(LinkType type) {
        return DevStudioLinker.getInstance().getLinker(type);
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
        DevStudioProcessor.getUndefineSwitch(buffer, define);
    }
}
