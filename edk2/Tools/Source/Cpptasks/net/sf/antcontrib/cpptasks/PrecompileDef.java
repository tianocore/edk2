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
import java.util.Enumeration;
import java.util.Vector;

import net.sf.antcontrib.cpptasks.types.ConditionalFileSet;

import org.apache.tools.ant.Project;
import org.apache.tools.ant.DirectoryScanner;
import org.apache.tools.ant.types.DataType;
/**
 * An element that specifies a prototype file and rules for source files that
 * should not use precompiled headers
 * 
 * @author Curt Arnold
 */
public final class PrecompileDef extends DataType {
    private final Vector exceptSets = new Vector();
    private String ifCond;
    /**
     * Directory of prototype file
     */
    private File prototype = new File("stdafx.cpp");
    private String unlessCond;
    /**
     * Constructor
     *  
     */
    public PrecompileDef() {
    }
    /**
     * Method used by PrecompileExceptDef to add exception set to
     * PrecompileDef.
     */
    public void appendExceptFileSet(ConditionalFileSet exceptSet) {
        exceptSet.setProject(getProject());
        exceptSets.addElement(exceptSet);
    }
    /**
     * Adds filesets that specify files that should not be processed with
     * precompiled headers enabled.
     * 
     * @param exceptSet
     *            FileSet specify files that should not be processed with
     *            precompiled headers enabled.
     */
    public PrecompileExceptDef createExcept() {
        return new PrecompileExceptDef(this);
    }
    public void execute() throws org.apache.tools.ant.BuildException {
        throw new org.apache.tools.ant.BuildException(
                "Not an actual task, but looks like one for documentation purposes");
    }
    public String[] getExceptFiles() {
        PrecompileDef ref = getRef();
        if (ref != null) {
            return ref.getExceptFiles();
        }
        if (exceptSets.size() == 0) {
            return new String[0];
        }
        Project p = getProject();
        String[] exceptFiles = null;
        Enumeration setEnum = exceptSets.elements();
        while (setEnum.hasMoreElements()) {
            ConditionalFileSet exceptSet = (ConditionalFileSet) setEnum
                    .nextElement();
            if (exceptSet.isActive()) {
                DirectoryScanner scanner = exceptSet
                        .getDirectoryScanner(p);
                String[] scannerFiles = scanner.getIncludedFiles();
                if (exceptFiles == null) {
                    exceptFiles = scannerFiles;
                } else {
                    if (scannerFiles.length > 0) {
                        String[] newFiles = new String[exceptFiles.length
                                + scannerFiles.length];
                        for (int i = 0; i < exceptFiles.length; i++) {
                            newFiles[i] = exceptFiles[i];
                        }
                        int index = exceptFiles.length;
                        for (int i = 0; i < scannerFiles.length; i++) {
                            newFiles[index++] = scannerFiles[i];
                        }
                        exceptFiles = newFiles;
                    }
                }
            }
        }
        if (exceptFiles == null) {
            exceptFiles = new String[0];
        }
        return exceptFiles;
    }
    /**
     * Gets prototype source file
     *  
     */
    public File getPrototype() {
        PrecompileDef ref = getRef();
        if (ref != null) {
            return ref.getPrototype();
        }
        return prototype;
    }
    private PrecompileDef getRef() {
        if (isReference()) {
            return ((PrecompileDef) getCheckedRef(PrecompileDef.class,
                    "PrecompileDef"));
        }
        return null;
    }
    public boolean isActive() {    	
        boolean isActive = CUtil.isActive(getProject(), ifCond, unlessCond);
        if (!isActive) {
            PrecompileDef ref = getRef();
            if (ref != null) {
                return ref.isActive();
            }
        }
        return isActive;
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
     * Set the 'if' condition.
     * 
     * The processor will be ignored unless the property is defined.
     * 
     * The value of property is insignificant, but values that would imply
     * misinterpretation ("false", "no") will throw an exception when
     * isActive() is evaluated.
     * 
     * @param propName
     *            name of property
     */
    public void setIf(String propName) {
        ifCond = propName;
    }
    /**
     * Sets file to precompile.
     * 
     * Should be a source file that includes only one unguarded header file.
     * Default value is "stdafx.cpp".
     * 
     * @param prototype
     *            file path for prototype source file
     */
    public void setPrototype(File prototype) {
        if (isReference()) {
            throw tooManyAttributes();
        }
        if (prototype == null) {
            throw new NullPointerException("prototype");
        }
        this.prototype = prototype;
    }
    /**
     * Specifies that this element should behave as if the content of the
     * element with the matching id attribute was inserted at this location.
     * 
     * @param ref
     *            Reference to other element
     *  
     */
    public void setRefid(org.apache.tools.ant.types.Reference ref) {
        super.setRefid(ref);
    }
    /**
     * Set the 'unless' condition. If named property exists at execution time,
     * the processor will be ignored.
     * 
     * Value of property is insignificant, but values that would imply
     * misinterpretation ("false", "no") of the behavior will throw an
     * exception when isActive is called.
     * 
     * @param propName
     *            name of property
     */
    public void setUnless(String propName) {
        unlessCond = propName;
    }
}
