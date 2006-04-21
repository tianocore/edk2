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

import net.sf.antcontrib.cpptasks.compiler.Assembler;
import net.sf.antcontrib.cpptasks.devstudio.DevStudioAssembler;
import net.sf.antcontrib.cpptasks.gcc.GccAssembler;

import org.apache.tools.ant.types.EnumeratedAttribute;

/**
 * Enumeration of supported assemblers
 * 
 * <table width="100%" border="1"> <thead>Supported assemblers </thead>
 * <tr>
 * <td>gas (default)</td>
 * <td>GAS assembler</td>
 * </tr>
 * <tr>
 * <td>masm</td>
 * <td>MASM assembler</td>
 * </tr>
 * </table>
 * 
 */
public class AssemblerEnum extends EnumeratedAttribute {
    private final static ProcessorEnumValue[] assemblers = new ProcessorEnumValue[] {
                    new ProcessorEnumValue("gas", GccAssembler.getInstance()),
                    new ProcessorEnumValue("masm", DevStudioAssembler
                                    .getInstance()), };

    public Assembler getAssembler() {
        return (Assembler) assemblers[getIndex()].getProcessor();
    }

    public String[] getValues() {
        return ProcessorEnumValue.getValues(assemblers);
    }
}