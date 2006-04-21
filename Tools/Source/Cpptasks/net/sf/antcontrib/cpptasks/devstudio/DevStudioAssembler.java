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
package net.sf.antcontrib.cpptasks.devstudio;

import java.io.File;
import java.util.Vector;

import net.sf.antcontrib.cpptasks.CUtil;
import net.sf.antcontrib.cpptasks.compiler.CommandLineAssembler;
import net.sf.antcontrib.cpptasks.compiler.LinkType;
import net.sf.antcontrib.cpptasks.compiler.Linker;

/**
 * Adaptor for Microsoft MASM
 * 
 */
public final class DevStudioAssembler extends CommandLineAssembler {
    private final static String[] sourceExtensions = new String[] { ".asm" };

    private final static String[] headerExtensions = new String[] { ".h",
                    ".inc" };

    private final static String[] defaultflags = new String[] { "/nologo", "/c" };

    private static final DevStudioAssembler instance = new DevStudioAssembler(
                    "ml", sourceExtensions, headerExtensions, false);

    /**
     * Gets masm adapter
     */
    public static DevStudioAssembler getInstance() {
        return instance;
    }

    /**
     * Private constructor. Use DevStudioAssembler.getInstance() to get
     * singleton instance of this class.
     */
    private DevStudioAssembler (String command, String[] sourceExtensions,
                    String[] headerExtensions, boolean isLibtool) {
        super(command, null, sourceExtensions, headerExtensions, ".obj");
    }

    public void addImpliedArgs(Vector args, boolean debug, Boolean defaultflag) {
        if (defaultflag != null && defaultflag.booleanValue()) {
            for (int i = 0; i < defaultflags.length; i++) {
                args.addElement(defaultflags[i]);
            }
        }
        if (debug) {
            args.addElement("Zi");
        }
    }

    public int getMaximumCommandLength() {
        return Integer.MAX_VALUE;
    }

    public Linker getLinker(LinkType linkType) {
        return DevStudioLinker.getInstance().getLinker(linkType);
    }

    protected File[] getEnvironmentIncludePath() {
        return CUtil.getPathFromEnvironment("INCLUDE", ";");
    }

    protected String getIncludeDirSwitch(String includeDir) {
        return "/I" + includeDir.replace('/', '\\');
    }
}
