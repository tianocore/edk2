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
import net.sf.antcontrib.cpptasks.devstudio.DevStudioCompatibleLinker;
/**
 * Adapter for the Intel (r) linker for 32-bit applications
 * 
 * @author Curt Arnold
 */
public final class IntelWin32Linker extends DevStudioCompatibleLinker {
    private static final IntelWin32Linker dllLinker = new IntelWin32Linker(
            ".dll");
    private static final IntelWin32Linker instance = new IntelWin32Linker(
            ".exe");
    public static IntelWin32Linker getInstance() {
        return instance;
    }
    private IntelWin32Linker(String outputSuffix) {
        super("xilink", "/bogus", outputSuffix);
    }
    public Linker getLinker(LinkType type) {
        if (type.isStaticLibrary()) {
            return IntelWin32Librarian.getInstance();
        }
        if (type.isSharedLibrary()) {
            return dllLinker;
        }
        return instance;
    }
}
