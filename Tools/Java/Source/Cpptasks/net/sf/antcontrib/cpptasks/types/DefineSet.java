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
import java.util.Vector;

import net.sf.antcontrib.cpptasks.CUtil;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.types.DataType;
import org.apache.tools.ant.types.Reference;
/**
 * Set of preprocessor macro defines and undefines.
 * 
 * @author Mark A Russell <a
 *         href="mailto:mark_russell@csgsystems.com">mark_russell@csg_systems.com
 *         </a>
 * @author Adam Murdoch
 */
public class DefineSet extends DataType {
    private Vector defineList = new Vector();
    private String ifCond = null;
    private String unlessCond = null;
    /**
     * 
     * Adds a define element.
     * 
     * @throws BuildException
     *             if reference
     */
    public void addDefine(DefineArgument arg) throws BuildException {
        if (isReference()) {
            throw noChildrenAllowed();
        }
        defineList.addElement(arg);
    }
    /** Adds defines/undefines. */
    private void addDefines(String[] defs, boolean isDefine) {
        for (int i = 0; i < defs.length; i++) {
            UndefineArgument def;
            if (isDefine) {
                def = new DefineArgument();
            } else {
                def = new UndefineArgument();
            }
            def.setName(defs[i]);
            defineList.addElement(def);
        }
    }
    /**
     * 
     * Adds an undefine element.
     * 
     * @throws BuildException
     *             if reference
     */
    public void addUndefine(UndefineArgument arg) throws BuildException {
        if (isReference()) {
            throw noChildrenAllowed();
        }
        defineList.addElement(arg);
    }
    public void execute() throws org.apache.tools.ant.BuildException {
        throw new org.apache.tools.ant.BuildException(
                "Not an actual task, but looks like one for documentation purposes");
    }
    /** Returns the defines and undefines in this set. */
    public UndefineArgument[] getDefines() throws BuildException {
        if (isReference()) {
            DefineSet defset = (DefineSet) getCheckedRef(DefineSet.class,
                    "DefineSet");
            return defset.getDefines();
        } else {
            if (isActive()) {
                UndefineArgument[] defs = new UndefineArgument[defineList
                        .size()];
                defineList.copyInto(defs);
                return defs;
            } else {
                return new UndefineArgument[0];
            }
        }
    }
    /**
     * Returns true if the define's if and unless conditions (if any) are
     * satisfied.
     * 
     * @exception BuildException
     *                throws build exception if name is not set
     */
    public final boolean isActive() throws BuildException {
        return CUtil.isActive(getProject(), ifCond, unlessCond);
    }
    /**
     * A comma-separated list of preprocessor macros to define. Use nested
     * define elements to define macro values.
     * 
     * @param defList
     *            comma-separated list of preprocessor macros
     * @throws BuildException
     *             throw if defineset is a reference
     */
    public void setDefine(CUtil.StringArrayBuilder defList)
            throws BuildException {
        if (isReference()) {
            throw tooManyAttributes();
        }
        addDefines(defList.getValue(), true);
    }
    /**
     * Sets a description of the current data type.
     */
    public void setDescription(String desc) {
        super.setDescription(desc);
    }
    /**
     * Sets an id that can be used to reference this element.
     * 
     * @param id
     *            id
     */
    public void setId(String id) {
        //
        //  this is actually accomplished by a different
        //     mechanism, but we can document it
        //
    }
    /**
     * Sets the property name for the 'if' condition.
     * 
     * The define will be ignored unless the property is defined.
     * 
     * The value of the property is insignificant, but values that would imply
     * misinterpretation ("false", "no") will throw an exception when
     * evaluated.
     * 
     * @param propName
     *            property name
     */
    public final void setIf(String propName) {
        ifCond = propName;
    }
    /**
     * Specifies that this element should behave as if the content of the
     * element with the matching id attribute was inserted at this location. If
     * specified, no other attributes or child content should be specified,
     * other than "description".
     *  
     */
    public void setRefid(Reference r) throws BuildException {
        if (!defineList.isEmpty()) {
            throw tooManyAttributes();
        }
        super.setRefid(r);
    }
    /**
     * A comma-separated list of preprocessor macros to undefine.
     * 
     * @param undefList
     *            comma-separated list of preprocessor macros
     * @throws BuildException
     *             throw if defineset is a reference
     */
    public void setUndefine(CUtil.StringArrayBuilder undefList)
            throws BuildException {
        if (isReference()) {
            throw tooManyAttributes();
        }
        addDefines(undefList.getValue(), false);
    }
    /**
     * Set the property name for the 'unless' condition.
     * 
     * If named property is set, the define will be ignored.
     * 
     * The value of the property is insignificant, but values that would imply
     * misinterpretation ("false", "no") of the behavior will throw an
     * exception when evaluated.
     * 
     * @param propName
     *            name of property
     */
    public final void setUnless(String propName) {
        unlessCond = propName;
    }
}
