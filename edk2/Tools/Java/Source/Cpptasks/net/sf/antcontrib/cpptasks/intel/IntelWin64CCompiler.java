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
package net.sf.antcontrib.cpptasks.intel;
import net.sf.antcontrib.cpptasks.compiler.LinkType;
import net.sf.antcontrib.cpptasks.compiler.Linker;
import net.sf.antcontrib.cpptasks.compiler.Processor;
import net.sf.antcontrib.cpptasks.devstudio.DevStudioCompatibleCCompiler;

import org.apache.tools.ant.types.Environment;
/**
 * Adapter for the Intel C++ compiler for Itanium(TM) Applications
 * 
 * @author Curt Arnold
 */
public final class IntelWin64CCompiler extends DevStudioCompatibleCCompiler {
    private static final IntelWin64CCompiler instance = new IntelWin64CCompiler(
            false, null);
    public static IntelWin64CCompiler getInstance() {
        return instance;
    }
    private IntelWin64CCompiler(boolean newEnvironment, Environment env) {
        super("ecl", null, newEnvironment, env);
    }
    public Processor changeEnvironment(boolean newEnvironment, Environment env) {
        if (newEnvironment || env != null) {
            return new IntelWin64CCompiler(newEnvironment, env);
        }
        return this;
    }
    public Linker getLinker(LinkType type) {
        //
        //   currently the Intel Win32 and Win64 linkers
        //      are command line equivalent
        return IntelWin32Linker.getInstance().getLinker(type);
    }
    public int getMaximumCommandLength() {
        return 1024;
    }
}
