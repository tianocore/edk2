/*
 * 
 * Copyright 2001-2004 The Ant-Contrib project
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
package net.sf.antcontrib.cpptasks.ti;
import java.io.File;
import java.util.Vector;

import net.sf.antcontrib.cpptasks.CUtil;
import net.sf.antcontrib.cpptasks.compiler.CommandLineCCompiler;
import net.sf.antcontrib.cpptasks.compiler.LinkType;
import net.sf.antcontrib.cpptasks.compiler.Linker;
import net.sf.antcontrib.cpptasks.OptimizationEnum;


import org.apache.tools.ant.types.Environment;
/**
 * Adapter for TI DSP compilers with cl** commands
 * 
 * @author CurtA
 */
public class ClxxCCompiler extends CommandLineCCompiler {
    /**
     * Header file extensions
     */
    private static final String[] headerExtensions = new String[]{".h", ".hpp",
            ".inl"};
    /**
     * Source file extensions
     */
    private static final String[] sourceExtensions = new String[]{".c", ".cc",
            ".cpp", ".cxx", ".c++"};
    /**
     * Singleton for TMS320C55x
     */
    private static final ClxxCCompiler cl55 = new ClxxCCompiler("cl55", false,
            null);
    /**
     * Singleton for TMS320C6000
     */
    private static final ClxxCCompiler cl6x = new ClxxCCompiler("cl6x", false,
            null);
    public static ClxxCCompiler getCl55Instance() {
        return cl55;
    }
    public static ClxxCCompiler getCl6xInstance() {
        return cl6x;
    }
    /**
     * Private constructor
     * 
     * @param command
     *            executable name
     * @param newEnvironment
     *            Change environment
     * @param env
     *            New environment
     */
    private ClxxCCompiler(String command, boolean newEnvironment,
            Environment env) {
        super(command, "-h", sourceExtensions, headerExtensions, ".o", false,
                null, newEnvironment, env);
    }
    /*
     * (non-Javadoc)
     * 
     * @see net.sf.antcontrib.cpptasks.compiler.CommandLineCompiler#addImpliedArgs(java.util.Vector,
     *      boolean, boolean, boolean,
     *      net.sf.antcontrib.cpptasks.compiler.LinkType)
     */
    protected void addImpliedArgs(
    		final Vector args, 
			final boolean debug,
            final boolean multithreaded, 
			final boolean exceptions, 
			final LinkType linkType,
			final Boolean rtti,
			final OptimizationEnum optimization,
   final Boolean defaultflag) {
        if (debug) {
            args.addElement("-gw");
        }
    }
    /*
     * (non-Javadoc)
     * 
     * @see net.sf.antcontrib.cpptasks.compiler.CommandLineCompiler#addWarningSwitch(java.util.Vector,
     *      int)
     */
    protected void addWarningSwitch(Vector args, int warnings) {
        // TODO Auto-generated method stub
    }
    /*
     * (non-Javadoc)
     * 
     * @see net.sf.antcontrib.cpptasks.compiler.CommandLineCompiler#getDefineSwitch(java.lang.StringBuffer,
     *      java.lang.String, java.lang.String)
     */
    protected void getDefineSwitch(StringBuffer buffer, String define,
            String value) {
        buffer.append("-d");
        buffer.append(define);
        if (value != null) {
            buffer.append('=');
            buffer.append(value);
        }
    }
    /*
     * (non-Javadoc)
     * 
     * @see net.sf.antcontrib.cpptasks.compiler.CommandLineCompiler#getEnvironmentIncludePath()
     */
    protected File[] getEnvironmentIncludePath() {
        File[] c_dir = CUtil.getPathFromEnvironment("C_DIR", ";");
        File[] cx_dir = CUtil.getPathFromEnvironment("C6X_C_DIR", ";");
        if (c_dir.length == 0) {
            return cx_dir;
        }
        if (cx_dir.length == 0) {
            return c_dir;
        }
        File[] combo = new File[c_dir.length + cx_dir.length];
        for (int i = 0; i < cx_dir.length; i++) {
            combo[i] = cx_dir[i];
        }
        for (int i = 0; i < c_dir.length; i++) {
            combo[i + cx_dir.length] = c_dir[i];
        }
        return combo;
    }
    /*
     * (non-Javadoc)
     * 
     * @see net.sf.antcontrib.cpptasks.compiler.CommandLineCompiler#getIncludeDirSwitch(java.lang.String)
     */
    protected String getIncludeDirSwitch(String source) {
        return "-I" + source;
    }
    /*
     * (non-Javadoc)
     * 
     * @see net.sf.antcontrib.cpptasks.compiler.Processor#getLinker(net.sf.antcontrib.cpptasks.compiler.LinkType)
     */
    public Linker getLinker(LinkType type) {
        if (type.isStaticLibrary()) {
            if (this == cl6x) {
                return ClxxLibrarian.getCl6xInstance();
            }
            return ClxxLibrarian.getCl55Instance();
        }
        if (type.isSharedLibrary()) {
            if (this == cl6x) {
                return ClxxLinker.getCl6xDllInstance();
            }
            return ClxxLinker.getCl55DllInstance();
        }
        if (this == cl6x) {
            return ClxxLinker.getCl6xInstance();
        }
        return ClxxLinker.getCl55Instance();
    }
    /*
     * (non-Javadoc)
     * 
     * @see net.sf.antcontrib.cpptasks.compiler.CommandLineCompiler#getMaximumCommandLength()
     */
    public int getMaximumCommandLength() {
        return 1024;
    }
    /*
     * (non-Javadoc)
     * 
     * @see net.sf.antcontrib.cpptasks.compiler.CommandLineCompiler#getUndefineSwitch(java.lang.StringBuffer,
     *      java.lang.String)
     */
    protected void getUndefineSwitch(StringBuffer buffer, String define) {
        buffer.append("-u");
        buffer.append(define);
    }
}
