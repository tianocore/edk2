/*
 * 
 * Copyright 2001-2005 The Ant-Contrib project
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

import net.sf.antcontrib.cpptasks.compiler.Aslcompiler;
import net.sf.antcontrib.cpptasks.devstudio.DevStudioAslcompiler;
import net.sf.antcontrib.cpptasks.intel.IntelWin32Aslcompiler;

import org.apache.tools.ant.types.EnumeratedAttribute;

/**
 * Enumeration of supported ASL Compilers
 * 
 * <table width="100%" border="1"> <thead>Supported ASL Compilers </thead>
 * <tr>
 * <td>iasl (default)</td>
 * <td>Intel ACPI Source Language</td>
 * </tr>
 * <tr>
 * <td>asl</td>
 * <td>Microsoft ACPI Source Language</td>
 * </tr>
 * </table>
 * 
 */
public class AslcompilerEnum extends EnumeratedAttribute {
    private final static ProcessorEnumValue[] aslcompiler = new ProcessorEnumValue[] {
                    new ProcessorEnumValue("iasl", IntelWin32Aslcompiler
                                    .getInstance()),
                    new ProcessorEnumValue("asl", DevStudioAslcompiler
                                    .getInstance()), };

    public Aslcompiler getAslcompiler() {
        return (Aslcompiler) aslcompiler[getIndex()].getProcessor();
    }

    public String[] getValues() {
        return ProcessorEnumValue.getValues(aslcompiler);
    }
}