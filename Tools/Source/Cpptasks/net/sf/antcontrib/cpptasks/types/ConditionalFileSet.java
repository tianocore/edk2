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
import net.sf.antcontrib.cpptasks.CUtil;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Project;
import org.apache.tools.ant.types.AbstractFileSet;
import org.apache.tools.ant.types.FileSet;
/**
 * An Ant FileSet object augmented with if and unless conditions.
 * 
 * @author Curt Arnold
 */
public class ConditionalFileSet extends FileSet {
    private String ifCond;
    private String unlessCond;
    public ConditionalFileSet() {
    }
    public void execute() throws org.apache.tools.ant.BuildException {
        throw new org.apache.tools.ant.BuildException(
                "Not an actual task, but looks like one for documentation purposes");
    }
    /**
     * overrides FileSet's implementation which would throw an exception since
     * the referenced object isn't this type.
     */
    protected AbstractFileSet getRef(Project p) {
        return (AbstractFileSet) getRefid().getReferencedObject(p);
    }
    /**
     * Returns true if the Path's if and unless conditions (if any) are
     * satisfied.
     */
    public boolean isActive() throws BuildException {
        Project p = getProject();
        if (p == null) {
            throw new java.lang.IllegalStateException(
                    "setProject() should have been called");
        }
        return CUtil.isActive(p, ifCond, unlessCond);
    }
    /**
     * Sets the property name for the 'if' condition.
     * 
     * The fileset will be ignored unless the property is defined.
     * 
     * The value of the property is insignificant, but values that would imply
     * misinterpretation ("false", "no") will throw an exception when
     * evaluated.
     */
    public void setIf(String propName) {
        ifCond = propName;
    }
    /**
     * Set the property name for the 'unless' condition.
     * 
     * If named property is set, the fileset will be ignored.
     * 
     * The value of the property is insignificant, but values that would imply
     * misinterpretation ("false", "no") of the behavior will throw an
     * exception when evaluated.
     * 
     * @param propName
     *            name of property
     */
    public void setUnless(String propName) {
        unlessCond = propName;
    }
}
