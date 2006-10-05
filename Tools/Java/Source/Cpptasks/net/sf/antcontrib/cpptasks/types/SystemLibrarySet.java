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
package net.sf.antcontrib.cpptasks.types;
/**
 * A set of system library names. Timestamp or location of system libraries are
 * not considered in dependency analysis.
 * 
 * Libraries can also be added to a link by specifying them in a fileset.
 * 
 * For most Unix-like compilers, syslibset will result in a series of -l and -L
 * linker arguments. For Windows compilers, the library names will be used to
 * locate the appropriate library files which will be added to the linkers
 * input file list as if they had been specified in a fileset.
 */
public class SystemLibrarySet extends LibrarySet {
    public SystemLibrarySet() {
        super();
    }
    public void execute() throws org.apache.tools.ant.BuildException {
        throw new org.apache.tools.ant.BuildException(
                "Not an actual task, but looks like one for documentation purposes");
    }
}
