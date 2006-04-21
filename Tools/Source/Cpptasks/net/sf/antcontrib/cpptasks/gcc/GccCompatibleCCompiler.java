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
package net.sf.antcontrib.cpptasks.gcc;
import java.io.File;
import java.util.Vector;
import net.sf.antcontrib.cpptasks.CUtil;
import net.sf.antcontrib.cpptasks.compiler.CommandLineCCompiler;
import net.sf.antcontrib.cpptasks.compiler.LinkType;
import org.apache.tools.ant.types.Environment;
import net.sf.antcontrib.cpptasks.OptimizationEnum;

/**
 * Abstract base class for compilers that attempt to be command line compatible
 * with GCC
 *
 * @author Adam Murdoch
 * @author Curt Arnold
 */
public abstract class GccCompatibleCCompiler extends CommandLineCCompiler {
    private final static String[] headerExtensions = new String[]{".h", ".hpp",
            ".inl"};
    private final static String[] sourceExtensions = new String[]{".c", ".cc",
            ".cpp", ".cxx", ".c++", ".i", ".f", ".for"};
    private final static String[] defaultflags = new String[]{"-c"};
    /**
     * Private constructor. Use GccCCompiler.getInstance() to get singleton
     * instance of this class.
     */
    protected GccCompatibleCCompiler(String command, String identifierArg,
            boolean libtool, GccCompatibleCCompiler libtoolCompiler,
            boolean newEnvironment, Environment env) {
        super(command, identifierArg, sourceExtensions, headerExtensions,
                libtool ? ".fo" : ".o", libtool, libtoolCompiler,
                newEnvironment, env);
    }
    /**
     * Private constructor. Use GccCCompiler.getInstance() to get singleton
     * instance of this class.
     */
    protected GccCompatibleCCompiler(String command, String identifierArg,
            String[] sourceExtensions, String[] headerExtensions,
            boolean libtool, GccCompatibleCCompiler libtoolCompiler,
            boolean newEnvironment, Environment env) {
        super(command, identifierArg, sourceExtensions, headerExtensions,
                libtool ? ".fo" : ".o", libtool, libtoolCompiler,
                newEnvironment, env);
    }
    public void addImpliedArgs(final Vector args,
                    final boolean debug,
            final boolean multithreaded,
                        final boolean exceptions,
                        final LinkType linkType,
                        final Boolean rtti,
                        final OptimizationEnum optimization,
                        final Boolean defaultflag) {
        //
        //  -fPIC is too much trouble
        //      users have to manually add it for
        //      operating systems that make sense
        //
        if (defaultflag != null && defaultflag.booleanValue()) {
            for (int i = 0; i < defaultflags.length; i++) {
                args.addElement(defaultflags[i]);
            }
        }
        if (debug) {
            args.addElement("-g");
        } else {
          if (optimization != null) {
            if (optimization.isSize()) {
              args.addElement("-Os");
            } else if (optimization.isSpeed()) {
              if ("full".equals(optimization.getValue())) {
                args.addElement("-O2");
              } else {
                if ("speed".equals(optimization.getValue())) {
                  args.addElement("-O1");
                } else {
                  args.addElement("-O3");
                }
              }
            }
          }
        }
        if (getIdentifier().indexOf("mingw") >= 0) {
            if (linkType.isSubsystemConsole()) {
                args.addElement("-mconsole");
            }
            if (linkType.isSubsystemGUI()) {
                args.addElement("-mwindows");
            }
        }
        if (rtti != null && !rtti.booleanValue()) {
          args.addElement("-fno-rtti");
        }

    }
    /**
     * Adds an include path to the command.
     */
    public void addIncludePath(String path, Vector cmd) {
        cmd.addElement("-I" + path);
    }
    public void addWarningSwitch(Vector args, int level) {
        switch (level) {
            case 0 :
                args.addElement("-w");
                break;
            case 5 :
                args.addElement("-Werror");
            /* nobreak */
            case 4 :
                args.addElement("-W");
            /* nobreak */
            case 3 :
                args.addElement("-Wall");
                break;
        }
    }
    public void getDefineSwitch(StringBuffer buffer, String define, String value) {
        buffer.append("-D");
        buffer.append(define);
        if (value != null && value.length() > 0) {
            buffer.append('=');
            buffer.append(value);
        }
    }
    protected File[] getEnvironmentIncludePath() {
        return CUtil.getPathFromEnvironment("INCLUDE", ":");
    }
    public String getIncludeDirSwitch(String includeDir) {
        return "-I" + includeDir;
    }
    public void getUndefineSwitch(StringBuffer buffer, String define) {
        buffer.append("-U");
        buffer.append(define);
    }
}
