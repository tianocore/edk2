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
package net.sf.antcontrib.cpptasks.compiler;
import java.io.File;
import net.sf.antcontrib.cpptasks.types.LibraryTypeEnum;
/**
 * A linker for executables, and static and dynamic libraries.
 * 
 * @author Adam Murdoch
 */
public interface Linker extends Processor {
    /**
     * Extracts the significant part of a library name to ensure there aren't
     * collisions
     */
    String getLibraryKey(File libname);
    /**
     * returns the library path for the linker
     */
    File[] getLibraryPath();
    /**
     * Returns a set of filename patterns corresponding to library names.
     * 
     * For example, "advapi32" would be expanded to "advapi32.dll" by
     * DevStudioLinker and to "libadvapi32.a" and "libadvapi32.so" by
     * GccLinker.
     * 
     * @param libnames
     *            array of library names
     */
    String[] getLibraryPatterns(String[] libnames, LibraryTypeEnum libraryType);
    /**
     * Gets the linker for the specified link type.
     * 
     * @return appropriate linker or null, will return this if this linker can
     *         handle the specified link type
     */
    Linker getLinker(LinkType linkType);
    /**
     * Returns true if the linker is case-sensitive
     */
    boolean isCaseSensitive();
}
