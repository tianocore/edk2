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
package net.sf.antcontrib.cpptasks.hp;
import java.io.File;
import java.util.Vector;

import net.sf.antcontrib.cpptasks.CUtil;
import net.sf.antcontrib.cpptasks.compiler.LinkType;
import net.sf.antcontrib.cpptasks.compiler.Linker;
import net.sf.antcontrib.cpptasks.gcc.GccCompatibleCCompiler;
import net.sf.antcontrib.cpptasks.OptimizationEnum;

import org.apache.tools.ant.types.Environment;
/**
 * Adapter for the HP aC++ C++ compiler
 * 
 * @author Curt Arnold
 */
public final class aCCCompiler extends GccCompatibleCCompiler {
    private static final aCCCompiler instance = new aCCCompiler("aCC", false,
            null);
    /**
     * Gets singleton instance of this class
     */
    public static aCCCompiler getInstance() {
        return instance;
    }
    private String identifier;
    private File[] includePath;
    /**
     * Private constructor. Use GccCCompiler.getInstance() to get singleton
     * instance of this class.
     */
    private aCCCompiler(String command, boolean newEnvironment, Environment env) {
        super(command, "-help", false, null, newEnvironment, env);
    }
    public void addImpliedArgs(Vector args, boolean debug,
            boolean multithreaded, boolean exceptions, LinkType linkType,
			final Boolean rtti,
			final OptimizationEnum optimization) {
        args.addElement("-c");
        if (debug) {
            args.addElement("-g");
        }
        /*
         * if (multithreaded) { args.addElement("-mt"); }
         */
        if (linkType.isSharedLibrary()) {
            args.addElement("+z");
        }
    }
    public void addWarningSwitch(Vector args, int level) {
        switch (level) {
            case 0 :
                args.addElement("-w");
                break;
            case 1 :
            case 2 :
                args.addElement("+w");
                break;
        /*
         * case 3: case 4: case 5: args.addElement("+w2"); break;
         */
        }
    }
    public File[] getEnvironmentIncludePath() {
        if (includePath == null) {
            File ccLoc = CUtil.getExecutableLocation("aCC");
            if (ccLoc != null) {
                File compilerIncludeDir = new File(
                        new File(ccLoc, "../include").getAbsolutePath());
                if (compilerIncludeDir.exists()) {
                    includePath = new File[2];
                    includePath[0] = compilerIncludeDir;
                }
            }
            if (includePath == null) {
                includePath = new File[1];
            }
            includePath[includePath.length - 1] = new File("/usr/include");
        }
        return includePath;
    }
    public Linker getLinker(LinkType linkType) {
        return aCCLinker.getInstance().getLinker(linkType);
    }
    public int getMaximumCommandLength() {
        return Integer.MAX_VALUE;
    }
}
