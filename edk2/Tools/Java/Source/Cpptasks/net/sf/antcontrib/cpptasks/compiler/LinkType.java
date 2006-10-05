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
import net.sf.antcontrib.cpptasks.OutputTypeEnum;
import net.sf.antcontrib.cpptasks.SubsystemEnum;
/**
 * This class represents the target platform for the compile and link step. The
 * name is an anachronism and should be changed.
 * 
 * @author Curt Arnold
 */
public class LinkType {
    private OutputTypeEnum outputType = new OutputTypeEnum();
    private boolean staticRuntime = false;
    private SubsystemEnum subsystem = new SubsystemEnum();
    /**
     * Constructor
     * 
     * By default, an gui executable with a dynamically linked runtime
     *  
     */
    public LinkType() {
    }
    /**
     * Gets whether the link should produce an executable
     * 
     * @return boolean
     */
    public boolean isExecutable() {
        String value = outputType.getValue();
        return value.equals("executable");
    }
    /**
     * Gets whether the link should produce a plugin module.
     * 
     * @return boolean
     */
    public boolean isPluginModule() {
        String value = outputType.getValue();
        return value.equals("plugin");
    }
    /**
     * Gets whether the link should produce a shared library.
     * 
     * @return boolean
     */
    public boolean isSharedLibrary() {
        String value = outputType.getValue();
        return value.equals("shared") || value.equals("plugin");
    }
    /**
     * Gets whether the link should produce a static library.
     * 
     * @return boolean
     */
    public boolean isStaticLibrary() {
        String value = outputType.getValue();
        return value.equals("static");
    }
    /**
     * Gets whether the module should use a statically linked runtime library.
     * 
     * @return boolean
     */
    public boolean isStaticRuntime() {
        return staticRuntime;
    }
    /**
     * Gets whether the link should produce a module for a console subsystem.
     * 
     * @return boolean
     */
    public boolean isSubsystemConsole() {
        String value = subsystem.getValue();
        return value.equals("console");
    }
    /**
     * Gets whether the link should produce a module for a graphical user
     * interface subsystem.
     * 
     * @return boolean
     */
    public boolean isSubsystemGUI() {
        String value = subsystem.getValue();
        return value.equals("gui");
    }
    /**
     * Sets the output type (execuable, shared, etc).
     * 
     * @param outputType,
     *            may not be null
     */
    public void setOutputType(OutputTypeEnum outputType) {
        if (outputType == null) {
            throw new IllegalArgumentException("outputType");
        }
        this.outputType = outputType;
    }
    /**
     * Requests use of a static runtime library.
     * 
     * @param staticRuntime
     *            if true, use static runtime library if possible.
     */
    public void setStaticRuntime(boolean staticRuntime) {
        this.staticRuntime = staticRuntime;
    }
    /**
     * Sets the subsystem (gui, console, etc).
     * 
     * @param subsystem
     *            subsystem, may not be null
     */
    public void setSubsystem(SubsystemEnum subsystem) {
        if (subsystem == null) {
            throw new IllegalArgumentException("subsystem");
        }
        this.subsystem = subsystem;
    }
}
