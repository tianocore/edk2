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
import net.sf.antcontrib.cpptasks.gcc.AbstractLdLinker;
import net.sf.antcontrib.cpptasks.gcc.GccLibrarian;
/**
 * Adapter for the Intel (r) linker for Linux for IA-64
 * 
 * @author Curt Arnold
 */
public final class IntelLinux64Linker extends AbstractLdLinker {
    private static final String[] discardFiles = new String[0];
    private static final String[] libtoolObjFiles = new String[]{".fo", ".a",
            ".lib", ".dll", ".so", ".sl"};
    private static final String[] objFiles = new String[]{".o", ".a", ".lib",
            ".dll", ".so", ".sl"};
    private static final IntelLinux64Linker dllLinker = new IntelLinux64Linker(
            "lib", ".so", false, new IntelLinux64Linker("lib", ".so", true,
                    null));
    private static final IntelLinux64Linker instance = new IntelLinux64Linker(
            "", "", false, null);
    public static IntelLinux64Linker getInstance() {
        return instance;
    }
    private IntelLinux64Linker(String outputPrefix, String outputSuffix,
            boolean isLibtool, IntelLinux64Linker libtoolLinker) {
        super("ecc", "-V", objFiles, discardFiles, outputPrefix, outputSuffix,
                isLibtool, libtoolLinker);
    }
    public Linker getLinker(LinkType type) {
        if (type.isStaticLibrary()) {
            return GccLibrarian.getInstance();
        }
        if (type.isSharedLibrary()) {
            return dllLinker;
        }
        return instance;
    }
}
