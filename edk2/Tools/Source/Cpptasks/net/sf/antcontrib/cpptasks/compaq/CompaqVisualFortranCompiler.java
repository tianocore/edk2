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
package net.sf.antcontrib.cpptasks.compaq;
import java.io.File;
import java.util.Vector;

import net.sf.antcontrib.cpptasks.CUtil;
import net.sf.antcontrib.cpptasks.compiler.CommandLineFortranCompiler;
import net.sf.antcontrib.cpptasks.compiler.LinkType;
import net.sf.antcontrib.cpptasks.compiler.Linker;
import net.sf.antcontrib.cpptasks.compiler.Processor;
import net.sf.antcontrib.cpptasks.OptimizationEnum;


import org.apache.tools.ant.types.Environment;
/**
 * Adapter for the Compaq(r) Visual Fortran compiler.
 * 
 * @author Curt Arnold
 */
public class CompaqVisualFortranCompiler extends CommandLineFortranCompiler {
    private static final CompaqVisualFortranCompiler[] instance = new CompaqVisualFortranCompiler[]{new CompaqVisualFortranCompiler(
            false, null)};
    public static CompaqVisualFortranCompiler getInstance() {
        return instance[0];
    }
    private CompaqVisualFortranCompiler(boolean newEnvironment, Environment env) {
        super("DF", null, new String[]{".f90", ".for", ".f"}, new String[]{
                ".i", ".i90", ".fpp", ".inc", ".bak", ".exe"}, ".obj", false,
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
        args.addElement("/nologo");
        args.addElement("/compile_only");
        if (debug) {
            args.addElement("/debug:full");
            args.addElement("/define:_DEBUG");
        } else {
            args.addElement("/debug:none");
            args.addElement("/define:NDEBUG");
        }
        if (multithreaded) {
            args.addElement("/threads");
            args.addElement("/define:_MT");
        } else {
            args.addElement("/nothreads");
        }
        boolean staticRuntime = linkType.isStaticRuntime();
        if (staticRuntime) {
            args.addElement("/libs:static");
        } else {
            args.addElement("/libs:dll");
        }
        if (linkType.isSharedLibrary()) {
            args.addElement("/dll");
            args.addElement("/define:_DLL");
        }
    }
    public void addWarningSwitch(Vector args, int level) {
        switch (level) {
            case 0 :
                args.addElement("/nowarn");
                break;
            case 1 :
                break;
            case 2 :
                break;
            case 3 :
                args.addElement("/warn:usage");
                break;
            case 4 :
                args.addElement("/warn:all");
                break;
            case 5 :
                args.addElement("/warn:errors");
                break;
        }
    }
    public Processor changeEnvironment(boolean newEnvironment, Environment env) {
        if (newEnvironment || env != null) {
            return new CompaqVisualFortranCompiler(newEnvironment, env);
        }
        return this;
    }
    protected void getDefineSwitch(StringBuffer buf, String define, String value) {
        buf.append("/define:");
        buf.append(define);
        if (value != null && value.length() > 0) {
            buf.append('=');
            buf.append(value);
        }
    }
    protected File[] getEnvironmentIncludePath() {
        return CUtil.getPathFromEnvironment("INCLUDE", ";");
    }
    protected String getIncludeDirSwitch(String includeDir) {
        StringBuffer buf = new StringBuffer("/include:");
        if (includeDir.indexOf(' ') >= 0) {
            buf.append('"');
            buf.append(includeDir);
            buf.append('"');
        } else {
            buf.append(includeDir);
        }
        return buf.toString();
    }
    public Linker getLinker(LinkType type) {
        return CompaqVisualFortranLinker.getInstance().getLinker(type);
    }
    public int getMaximumCommandLength() {
        return 1024;
    }
    protected void getUndefineSwitch(StringBuffer buf, String define) {
        buf.append("/undefine:");
        buf.append(define);
    }
}
