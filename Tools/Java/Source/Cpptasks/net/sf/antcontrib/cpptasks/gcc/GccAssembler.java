/*
 * 
 * Copyright 2001-2005 The Ant-Contrib project
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
import net.sf.antcontrib.cpptasks.compiler.CommandLineAssembler;
import net.sf.antcontrib.cpptasks.compiler.LinkType;
import net.sf.antcontrib.cpptasks.compiler.Linker;

/**
 * Adapter for gcc assemble
 * 
 */
public final class GccAssembler extends CommandLineAssembler {
    private final static String[] sourceExtensions = new String[] { ".asm" };

    private final static String[] headerExtensions = new String[] { ".h",
                    ".inc" };

    private static final GccAssembler instance = new GccAssembler("gas",
                    sourceExtensions, headerExtensions, false);

    /**
     * Gets gcc adapter
     */
    public static GccAssembler getInstance() {
        return instance;
    }

    /**
     * Private constructor. Use GccAssembler.getInstance() to get singleton
     * instance of this class.
     */
    private GccAssembler (String command, String[] sourceExtensions,
                    String[] headerExtensions, boolean isLibtool) {
        super(command, null, sourceExtensions, headerExtensions,
                        isLibtool ? ".fo" : ".o");
    }

    public void addImpliedArgs(Vector args, boolean debug, Boolean defaultflag) {

    }

    public int getMaximumCommandLength() {
        return Integer.MAX_VALUE;
    }

    public Linker getLinker(LinkType linkType) {
        return GccLinker.getInstance().getLinker(linkType);
    }

    protected File[] getEnvironmentIncludePath() {
        return CUtil.getPathFromEnvironment("INCLUDE", ":");
    }

    protected String getIncludeDirSwitch(String includeDir) {
        return "-I" + includeDir;
    }
}
