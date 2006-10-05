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
package net.sf.antcontrib.cpptasks;
import java.io.File;

import net.sf.antcontrib.cpptasks.types.ConditionalFileSet;

import org.apache.tools.ant.BuildException;
/**
 * Specifies files that should not be compiled using precompiled headers.
 * 
 * @author Curt Arnold
 */
public final class PrecompileExceptDef {
    private ConditionalFileSet localSet = null;
    /**
     * Collection of <fileset>contained by definition
     */
    private PrecompileDef owner;
    /**
     * Constructor
     *  
     */
    public PrecompileExceptDef(PrecompileDef owner) {
        this.owner = owner;
    }
    /**
     * Adds filesets that specify files that should not be processed using
     * precompiled headers.
     * 
     * @param exceptSet
     *            FileSet specify files that should not be processed with
     *            precompiled headers enabled.
     */
    public void addFileset(ConditionalFileSet exceptSet) {
        owner.appendExceptFileSet(exceptSet);
    }
    public void execute() throws org.apache.tools.ant.BuildException {
        throw new org.apache.tools.ant.BuildException(
                "Not an actual task, but looks like one for documentation purposes");
    }
    /**
     * Sets the base-directory
     */
    public void setDir(File dir) throws BuildException {
        if (localSet == null) {
            localSet = new ConditionalFileSet();
            owner.appendExceptFileSet(localSet);
        }
        localSet.setDir(dir);
    }
    /**
     * Comma or space separated list of file patterns that should not be
     * compiled using precompiled headers.
     * 
     * @param includes
     *            the string containing the include patterns
     */
    public void setIncludes(String includes) {
        if (localSet == null) {
            localSet = new ConditionalFileSet();
            owner.appendExceptFileSet(localSet);
        }
        localSet.setIncludes(includes);
    }
}
