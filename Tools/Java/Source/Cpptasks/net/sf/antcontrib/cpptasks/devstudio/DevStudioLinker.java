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
package net.sf.antcontrib.cpptasks.devstudio;
import net.sf.antcontrib.cpptasks.compiler.LinkType;
import net.sf.antcontrib.cpptasks.compiler.Linker;
/**
 * Adapter for the Microsoft (r) Incremental Linker
 * 
 * @author Adam Murdoch
 * @author Curt Arnold
 */
public final class DevStudioLinker extends DevStudioCompatibleLinker {
    private static final DevStudioLinker dllLinker = new DevStudioLinker(".dll");
    private static final DevStudioLinker instance = new DevStudioLinker(".exe");
    public static DevStudioLinker getInstance() {
        return instance;
    }
    private DevStudioLinker(String outputSuffix) {
        super("link", "/DLL", outputSuffix);
    }
    public Linker getLinker(LinkType type) {
        if (type.isSharedLibrary()) {
            return dllLinker;
        }
        if (type.isStaticLibrary()) {
            return DevStudioLibrarian.getInstance();
        }
        return instance;
    }
}
