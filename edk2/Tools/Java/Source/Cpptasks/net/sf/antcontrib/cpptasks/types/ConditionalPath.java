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
import java.io.File;

import net.sf.antcontrib.cpptasks.CUtil;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Project;
import org.apache.tools.ant.types.Path;
/**
 * An Ant Path object augmented with if and unless conditionals
 * 
 * @author Curt Arnold
 */
public class ConditionalPath extends Path {
    private String ifCond;
    private String unlessCond;
    private File file;
    public ConditionalPath(Project project) {
        super(project);
    }
    public ConditionalPath(Project p, String path) {
        super(p, path);
    }
    public File getFile() {
      return file;
    }
    /**
     * Returns true if the Path's if and unless conditions (if any) are
     * satisfied.
     */
    public boolean isActive(org.apache.tools.ant.Project p)
            throws BuildException {
        return CUtil.isActive(p, ifCond, unlessCond);
    }
    /**
     * Sets the property name for the 'if' condition.
     * 
     * The path will be ignored unless the property is defined.
     * 
     * The value of the property is insignificant, but values that would imply
     * misinterpretation ("false", "no") will throw an exception when
     * evaluated.
     * 
     * @param propName
     *            property name
     */
    public void setIf(String propName) {
        ifCond = propName;
    }
    /**
     * Set the property name for the 'unless' condition.
     * 
     * If named property is set, the path will be ignored.
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
    /**
     * Specifies the file which lists many include paths that should appear on 
     * the command line. Each line is an include path. The includepath will be 
     * quated if it contains embedded blanks. 
     * 
     * @param file
     *          name of the file
     */
    public void setFile(File file) {
        this.file = file;
    }
}
