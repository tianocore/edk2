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
import net.sf.antcontrib.cpptasks.gcc.GccCompatibleCCompiler;

import org.apache.tools.ant.types.Environment;
/**
 * Adapter for the Intel (r) C/C++ compiler for IA-32 Linux (r)
 * 
 * The Intel (r) C/C++ compiler for IA32 Linux mimics the command options for
 * gcc compiler.
 * 
 * @author Curt Arnold
 */
public final class IntelLinux32CCompiler extends GccCompatibleCCompiler {
    private static final IntelLinux32CCompiler instance = new IntelLinux32CCompiler(
            false, new IntelLinux32CCompiler(true, null, false, null), false,
            null);
    public static IntelLinux32CCompiler getInstance() {
        return instance;
    }
    private IntelLinux32CCompiler(boolean isLibtool,
            IntelLinux32CCompiler libtoolCompiler, boolean newEnvironment,
            Environment env) {
        super("icc", "-V", isLibtool, libtoolCompiler, newEnvironment, env);
    }
    public Processor changeEnvironment(boolean newEnvironment, Environment env) {
        if (newEnvironment || env != null) {
            return new IntelLinux32CCompiler(getLibtool(),
                    (IntelLinux32CCompiler) getLibtoolCompiler(),
                    newEnvironment, env);
        }
        return this;
    }
    public Linker getLinker(LinkType type) {
        return IntelLinux32Linker.getInstance().getLinker(type);
    }
    public int getMaximumCommandLength() {
        return Integer.MAX_VALUE;
    }
}
