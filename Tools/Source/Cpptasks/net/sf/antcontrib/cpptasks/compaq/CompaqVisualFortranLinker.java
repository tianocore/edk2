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
import java.util.Vector;

import net.sf.antcontrib.cpptasks.compiler.LinkType;
import net.sf.antcontrib.cpptasks.compiler.Linker;
import net.sf.antcontrib.cpptasks.devstudio.DevStudioCompatibleLinker;
/**
 * Adapter for the Compaq(r) Visual Fortran linker.
 * 
 * @author Curt Arnold
 */
public final class CompaqVisualFortranLinker extends DevStudioCompatibleLinker {
    private static final CompaqVisualFortranLinker dllLinker = new CompaqVisualFortranLinker(
            ".dll");
    private static final CompaqVisualFortranLinker instance = new CompaqVisualFortranLinker(
            ".exe");
    public static CompaqVisualFortranLinker getInstance() {
        return instance;
    }
    private CompaqVisualFortranLinker(String outputSuffix) {
        super("DF", "__bogus__.xxx", outputSuffix);
    }
    protected void addImpliedArgs(boolean debug, LinkType linkType, Vector args) {
        args.addElement("/NOLOGO");
        boolean staticRuntime = linkType.isStaticRuntime();
        if (staticRuntime) {
            args.addElement("/libs:static");
        } else {
            args.addElement("/libs:dll");
        }
        if (debug) {
            args.addElement("/debug");
        } else {
        }
        if (linkType.isSharedLibrary()) {
            args.addElement("/dll");
        } else {
            args.addElement("/exe");
        }
    }
    public Linker getLinker(LinkType type) {
        if (type.isStaticLibrary()) {
            return CompaqVisualFortranLibrarian.getInstance();
        }
        if (type.isSharedLibrary()) {
            return dllLinker;
        }
        return instance;
    }
    public String[] getOutputFileSwitch(String outputFile) {
        StringBuffer buf = new StringBuffer("/OUT:");
        if (outputFile.indexOf(' ') >= 0) {
            buf.append('"');
            buf.append(outputFile);
            buf.append('"');
        } else {
            buf.append(outputFile);
        }
        return new String[]{buf.toString()};
    }
}
