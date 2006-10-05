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
package net.sf.antcontrib.cpptasks.os390;
import java.io.File;
import java.util.Vector;

import net.sf.antcontrib.cpptasks.CUtil;
import net.sf.antcontrib.cpptasks.CompilerDef;
import net.sf.antcontrib.cpptasks.compiler.AbstractCompiler;
import net.sf.antcontrib.cpptasks.compiler.CommandLineCCompiler;
import net.sf.antcontrib.cpptasks.compiler.LinkType;
import net.sf.antcontrib.cpptasks.compiler.Linker;
import net.sf.antcontrib.cpptasks.compiler.Processor;
import net.sf.antcontrib.cpptasks.types.DefineArgument;
import net.sf.antcontrib.cpptasks.types.UndefineArgument;
import net.sf.antcontrib.cpptasks.OptimizationEnum;


import org.apache.tools.ant.types.Environment;
/**
 * Adapter for the IBM (R) OS/390 (tm) C++ Compiler
 * 
 * @author Hiram Chirino (cojonudo14@hotmail.com)
 */
public class OS390CCompiler extends CommandLineCCompiler {
    private static final AbstractCompiler instance = new OS390CCompiler(false,
            null);
    public static AbstractCompiler getInstance() {
        return instance;
    }
    private OS390CCompiler(boolean newEnvironment, Environment env) {
        super("cxx", null, new String[]{".c", ".cc", ".cpp", ".cxx", ".c++",
                ".s"}, new String[]{".h", ".hpp"}, ".o", false, null,
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
        // Specifies that only compilations and assemblies be done.
        //  Link-edit is not done
        args.addElement("-c");
        args.addElement("-W");
        args.addElement("c,NOEXPMAC,NOSHOWINC");
        /*
         * if (exceptions) { args.addElement("/GX"); }
         */
        if (debug) {
            args.addElement("-g");
            args.addElement("-D");
            args.addElement("_DEBUG");
            /*
             * if (multithreaded) { args.addElement("/D_MT"); if (staticLink) {
             * args.addElement("/MTd"); } else { args.addElement("/MDd");
             * args.addElement("/D_DLL"); } } else { args.addElement("/MLd"); }
             */
        } else {
            args.addElement("-D");
            args.addElement("NEBUG");
            /*
             * if (multithreaded) { args.addElement("/D_MT"); if (staticLink) {
             * args.addElement("/MT"); } else { args.addElement("/MD");
             * args.addElement("/D_DLL"); } } else { args.addElement("/ML"); }
             */
        }
    }
    protected void addWarningSwitch(Vector args, int level) {
        OS390Processor.addWarningSwitch(args, level);
    }
    /**
     * The buildDefineArguments implementation CommandLineCCompiler is not good
     * for us because os390 defines are give by -D definex instead of
     * /Ddefinex, 2 args not 1! since we implement this ourslefs, we do not
     * have to implement the getDefineSwitch() and the getUndefineSwitch().
     */
    protected void buildDefineArguments(CompilerDef[] defs, Vector args) {
        //
        //   assume that we aren't inheriting defines from containing <cc>
        //
        UndefineArgument[] merged = defs[0].getActiveDefines();
        for (int i = 1; i < defs.length; i++) {
            //
            //  if we are inheriting, merge the specific defines with the
            //      containing defines
            merged = DefineArgument.merge(defs[i].getActiveDefines(), merged);
        }
        StringBuffer buf = new StringBuffer(30);
        for (int i = 0; i < merged.length; i++) {
            buf.setLength(0);
            UndefineArgument current = merged[i];
            if (current.isDefine()) {
                args.addElement("-D");
                buf.append(current.getName());
                if (current.getValue() != null
                        && current.getValue().length() > 0) {
                    buf.append('=');
                    buf.append(current.getValue());
                }
                args.addElement(buf.toString());
            } else {
                args.addElement("-U");
                args.addElement(current.getName());
            }
        }
    }
    public Processor changeEnvironment(boolean newEnvironment, Environment env) {
        if (newEnvironment || env != null) {
            return new OS390CCompiler(newEnvironment, env);
        }
        return this;
    }
    /*
     * @see CommandLineCompiler#getDefineSwitch(StringBuffer, String, String)
     */
    protected void getDefineSwitch(StringBuffer buffer, String define,
            String value) {
    }
    protected File[] getEnvironmentIncludePath() {
        return CUtil.getPathFromEnvironment("INCLUDE", ":");
    }
    protected String getIncludeDirSwitch(String includeDir) {
        return OS390Processor.getIncludeDirSwitch(includeDir);
    }
    public Linker getLinker(LinkType type) {
        return OS390Linker.getInstance().getLinker(type);
    }
    public int getMaximumCommandLength() {
        return Integer.MAX_VALUE;
    }
    /* Only compile one file at time for now */
    protected int getMaximumInputFilesPerCommand() {
        return Integer.MAX_VALUE;
    }
    /*
     * @see CommandLineCompiler#getUndefineSwitch(StringBuffer, String)
     */
    protected void getUndefineSwitch(StringBuffer buffer, String define) {
    }
}
