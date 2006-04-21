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
import net.sf.antcontrib.cpptasks.compiler.CommandLineCompilerConfiguration;
import net.sf.antcontrib.cpptasks.compiler.CompilerConfiguration;
import net.sf.antcontrib.cpptasks.compiler.LinkType;
import net.sf.antcontrib.cpptasks.compiler.PrecompilingCommandLineCCompiler;
import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.types.Environment;
import net.sf.antcontrib.cpptasks.OptimizationEnum;

/**
 * An abstract base class for compilers that are basically command line
 * compatible with Microsoft(r) C/C++ Optimizing Compiler
 *
 * @author Curt Arnold
 */
public abstract class DevStudioCompatibleCCompiler
        extends
            PrecompilingCommandLineCCompiler {
    private static String[] mflags = new String[]{
    //
            //   first four are single-threaded
            //      (runtime=static,debug=false), (..,debug=true),
            //      (runtime=dynamic,debug=true), (..,debug=false), (not supported)
            //    next four are multi-threaded, same sequence
            "/ML", "/MLd", null, null, "/MT", "/MTd", "/MD", "/MDd"};
    private static String[] defaultflags = new String[]{"/nologo", "/c"};
    protected DevStudioCompatibleCCompiler(String command,
            String identifierArg, boolean newEnvironment, Environment env) {
        super(command, identifierArg, new String[]{".c", ".cc", ".cpp", ".cxx",
                ".c++"}, new String[]{".h", ".hpp", ".inl"}, ".obj", false,
                null, newEnvironment, env);
    }
    protected void addImpliedArgs(final Vector args,
                    final boolean debug,
            final boolean multithreaded,
                        final boolean exceptions,
                        final LinkType linkType,
                        final Boolean rtti,
                        final OptimizationEnum optimization,
                        final Boolean defaultflag) {
        if (defaultflag != null && defaultflag.booleanValue()) {
            for (int i = 0; i < defaultflags.length; i++) {
                args.addElement(defaultflags[i]);
            }
        }
        if (exceptions) {
            args.addElement("/GX");
        }
        int mindex = 0;
        if (multithreaded) {
            mindex += 4;
        }
        boolean staticRuntime = linkType.isStaticRuntime();
        if (!staticRuntime) {
            mindex += 2;
        }
        if (debug) {
            mindex += 1;
            args.addElement("/Zi");
            args.addElement("/Od");
            args.addElement("/GZ");
            args.addElement("/D_DEBUG");
        } else {
                if (optimization != null) {
                   if (optimization.isSize()) {
                     args.addElement("/O1");
                   }
                   if (optimization.isSpeed()) {
                     args.addElement("/O2");
                   }
                }
            args.addElement("/DNDEBUG");
        }
        String mflag = mflags[mindex];
        if (mflag == null) {
            throw new BuildException(
                    "multithread='false' and runtime='dynamic' not supported");
        }
        args.addElement(mflag);
        if (rtti != null && rtti.booleanValue()) {
                args.addElement("/GR");
        }
    }
    protected void addWarningSwitch(Vector args, int level) {
        DevStudioProcessor.addWarningSwitch(args, level);
    }
    protected CompilerConfiguration createPrecompileGeneratingConfig(
            CommandLineCompilerConfiguration baseConfig, File prototype,
            String lastInclude) {
        String[] additionalArgs = new String[]{
                "/Fp" + CUtil.getBasename(prototype) + ".pch", "/Yc"};
        return new CommandLineCompilerConfiguration(baseConfig, additionalArgs,
                null, true);
    }
    protected CompilerConfiguration createPrecompileUsingConfig(
            CommandLineCompilerConfiguration baseConfig, File prototype,
            String lastInclude, String[] exceptFiles) {
        String[] additionalArgs = new String[]{
                "/Fp" + CUtil.getBasename(prototype) + ".pch",
                "/Yu" + lastInclude};
        return new CommandLineCompilerConfiguration(baseConfig, additionalArgs,
                exceptFiles, false);
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
    protected void getUndefineSwitch(StringBuffer buffer, String define) {
        DevStudioProcessor.getUndefineSwitch(buffer, define);
    }
}
