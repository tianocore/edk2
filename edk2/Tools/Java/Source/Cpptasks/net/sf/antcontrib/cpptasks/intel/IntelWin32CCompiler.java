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
 * Adapter for the Intel (r) C++ compiler for 32-bit applications
 * 
 * The Intel (r) C++ compiler for IA32 Windows mimics the command options for
 * the Microsoft (r) C++ compiler.
 * 
 * @author Curt Arnold
 */
public final class IntelWin32CCompiler extends DevStudioCompatibleCCompiler {
    private static final IntelWin32CCompiler instance = new IntelWin32CCompiler(
            false, null);
    public static IntelWin32CCompiler getInstance() {
        return instance;
    }
    private IntelWin32CCompiler(boolean newEnvironment, Environment env) {
        super("icl", null, newEnvironment, env);
    }
    public Processor changeEnvironment(boolean newEnvironment, Environment env) {
        if (newEnvironment || env != null) {
            return new IntelWin32CCompiler(newEnvironment, env);
        }
        return this;
    }
    public Linker getLinker(LinkType type) {
        return IntelWin32Linker.getInstance().getLinker(type);
    }
    public int getMaximumCommandLength() {
        return 1024;
    }
}
