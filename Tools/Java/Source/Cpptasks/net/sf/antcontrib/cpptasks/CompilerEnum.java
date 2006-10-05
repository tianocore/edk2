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
import net.sf.antcontrib.cpptasks.arm.ADSCCompiler;
import net.sf.antcontrib.cpptasks.borland.BorlandCCompiler;
import net.sf.antcontrib.cpptasks.borland.BorlandResourceCompiler;
import net.sf.antcontrib.cpptasks.compaq.CompaqVisualFortranCompiler;
import net.sf.antcontrib.cpptasks.compiler.Compiler;
import net.sf.antcontrib.cpptasks.devstudio.DevStudioCCompiler;
import net.sf.antcontrib.cpptasks.devstudio.DevStudioMIDLCompiler;
import net.sf.antcontrib.cpptasks.devstudio.DevStudioResourceCompiler;
import net.sf.antcontrib.cpptasks.gcc.GccCCompiler;
import net.sf.antcontrib.cpptasks.hp.aCCCompiler;
import net.sf.antcontrib.cpptasks.ibm.VisualAgeCCompiler;
import net.sf.antcontrib.cpptasks.intel.IntelLinux32CCompiler;
import net.sf.antcontrib.cpptasks.intel.IntelLinux64CCompiler;
import net.sf.antcontrib.cpptasks.intel.IntelWin32CCompiler;
import net.sf.antcontrib.cpptasks.intel.IntelWin64CCompiler;
import net.sf.antcontrib.cpptasks.os390.OS390CCompiler;
import net.sf.antcontrib.cpptasks.os400.IccCompiler;
import net.sf.antcontrib.cpptasks.sun.C89CCompiler;
import net.sf.antcontrib.cpptasks.sun.ForteCCCompiler;
import net.sf.antcontrib.cpptasks.ti.ClxxCCompiler;

import org.apache.tools.ant.types.EnumeratedAttribute;
/**
 * Enumeration of supported compilers
 * 
 * <table width="100%" border="1"> <thead>Supported compilers </thead>
 * <tr>
 * <td>gcc (default)</td>
 * <td>GCC C++ compiler</td>
 * </tr>
 * <tr>
 * <td>g++</td>
 * <td>GCC C++ compiler</td>
 * </tr>
 * <tr>
 * <td>c++</td>
 * <td>GCC C++ compiler</td>
 * </tr>
 * <tr>
 * <td>g77</td>
 * <td>GNU FORTRAN compiler</td>
 * </tr>
 * <tr>
 * <td>msvc</td>
 * <td>Microsoft Visual C++</td>
 * </tr>
 * <tr>
 * <td>bcc</td>
 * <td>Borland C++ Compiler</td>
 * </tr>
 * <tr>
 * <td>msrc</td>
 * <td>Microsoft Resource Compiler</td>
 * </tr>
 * <tr>
 * <td>brc</td>
 * <td>Borland Resource Compiler</td>
 * </tr>
 * <tr>
 * <td>df</td>
 * <td>Compaq Visual Fortran Compiler</td>
 * </tr>
 * <tr>
 * <td>midl</td>
 * <td>Microsoft MIDL Compiler</td>
 * </tr>
 * <tr>
 * <td>icl</td>
 * <td>Intel C++ compiler for Windows (IA-32)</td>
 * </tr>
 * <tr>
 * <td>ecl</td>
 * <td>Intel C++ compiler for Windows (IA-64)</td>
 * </tr>
 * <tr>
 * <td>icc</td>
 * <td>Intel C++ compiler for Linux (IA-32)</td>
 * </tr>
 * <tr>
 * <td>ecc</td>
 * <td>Intel C++ compiler for Linux (IA-64)</td>
 * </tr>
 * <tr>
 * <td>CC</td>
 * <td>Sun ONE C++ compiler</td>
 * </tr>
 * <tr>
 * <td>aCC</td>
 * <td>HP aC++ C++ Compiler</td>
 * </tr>
 * <tr>
 * <td>os390</td>
 * <td>OS390 C Compiler</td>
 * </tr>
 * <tr>
 * <td>os400</td>
 * <td>Icc Compiler</td>
 * </tr>
 * <tr>
 * <td>sunc89</td>
 * <td>Sun C89 C Compiler</td>
 * </tr>
 * <tr>
 * <td>xlC</td>
 * <td>VisualAge C Compiler</td>
 * </tr>
 * <tr>
 * <td>cl6x</td>
 * <td>TI TMS320C6000 Optimizing Compiler</td>
 * </tr>
 * <tr>
 * <td>cl55</td>
 * <td>TI TMS320C55x Optimizing C/C++ Compiler</td>
 * </tr>
 * <tr>
 * <td>armcpp</td>
 * <td>ARM 32-bit C++ compiler</td>
 * </tr>
 * <tr>
 * <td>armcc</td>
 * <td>ARM 32-bit C compiler</td>
 * </tr>
 * <tr>
 * <td>tcpp</td>
 * <td>ARM 16-bit C++ compiler</td>
 * </tr>
 * <tr>
 * <td>tcc</td>
 * <td>ARM 16-bit C compiler</td>
 * </tr>
 * </table>
 * 
 * @author Curt Arnold
 *  
 */
public class CompilerEnum extends EnumeratedAttribute {
    private final static ProcessorEnumValue[] compilers = new ProcessorEnumValue[]{
            new ProcessorEnumValue("gcc", GccCCompiler.getInstance()),
            new ProcessorEnumValue("g++", GccCCompiler.getGppInstance()),
            new ProcessorEnumValue("c++", GccCCompiler.getCppInstance()),
            new ProcessorEnumValue("g77", GccCCompiler.getG77Instance()),
            new ProcessorEnumValue("msvc", DevStudioCCompiler.getInstance()),
            new ProcessorEnumValue("bcc", BorlandCCompiler.getInstance()),
            new ProcessorEnumValue("msrc", DevStudioResourceCompiler
                    .getInstance()),
            new ProcessorEnumValue("brc", BorlandResourceCompiler.getInstance()),
            new ProcessorEnumValue("df", CompaqVisualFortranCompiler
                    .getInstance()),
            new ProcessorEnumValue("midl", DevStudioMIDLCompiler.getInstance()),
            new ProcessorEnumValue("icl", IntelWin32CCompiler.getInstance()),
            new ProcessorEnumValue("ecl", IntelWin64CCompiler.getInstance()),
            new ProcessorEnumValue("icc", IntelLinux32CCompiler.getInstance()),
            new ProcessorEnumValue("ecc", IntelLinux64CCompiler.getInstance()),
            new ProcessorEnumValue("CC", ForteCCCompiler.getInstance()),
            new ProcessorEnumValue("aCC", aCCCompiler.getInstance()),
            new ProcessorEnumValue("os390", OS390CCompiler.getInstance()),
            new ProcessorEnumValue("os400", IccCompiler.getInstance()),
            new ProcessorEnumValue("sunc89", C89CCompiler.getInstance()),
            new ProcessorEnumValue("xlC", VisualAgeCCompiler.getInstance()),
            new ProcessorEnumValue("cl6x", ClxxCCompiler.getCl6xInstance()),
            new ProcessorEnumValue("cl55", ClxxCCompiler.getCl55Instance()),
            new ProcessorEnumValue("armcc", ADSCCompiler.getArmCC()),
            new ProcessorEnumValue("armcpp", ADSCCompiler.getArmCpp()),
            new ProcessorEnumValue("tcc", ADSCCompiler.getThumbCC()),
            new ProcessorEnumValue("tcpp", ADSCCompiler.getThumbCpp()),
            // userdefined
            //new ProcessorEnumValue("userdefine", UserdefineCompiler.getInstance()),
            // GCC Cross Compilers
            new ProcessorEnumValue(
                    "sparc-sun-solaris2-gcc",
                    net.sf.antcontrib.cpptasks.gcc.cross.sparc_sun_solaris2.GccCCompiler
                            .getInstance()),
            new ProcessorEnumValue(
                    "sparc-sun-solaris2-g++",
                    net.sf.antcontrib.cpptasks.gcc.cross.sparc_sun_solaris2.GccCCompiler
                            .getGppInstance()),
            new ProcessorEnumValue(
                    "sparc-sun-solaris2-c++",
                    net.sf.antcontrib.cpptasks.gcc.cross.sparc_sun_solaris2.GccCCompiler
                            .getCppInstance()),
            new ProcessorEnumValue(
                    "sparc-sun-solaris2-g77",
                    net.sf.antcontrib.cpptasks.gcc.cross.sparc_sun_solaris2.GccCCompiler
                            .getG77Instance()),
            // GCC Cross Compilers
            new ProcessorEnumValue("gcc-cross",
                    net.sf.antcontrib.cpptasks.gcc.cross.GccCCompiler
                            .getInstance()),
            new ProcessorEnumValue("g++-cross",
                    net.sf.antcontrib.cpptasks.gcc.cross.GccCCompiler
                            .getGppInstance()),
            new ProcessorEnumValue("c++-cross",
                    net.sf.antcontrib.cpptasks.gcc.cross.GccCCompiler
                            .getCppInstance()),
            new ProcessorEnumValue("g77-cross",
                    net.sf.antcontrib.cpptasks.gcc.cross.GccCCompiler
                            .getG77Instance()),};
    public Compiler getCompiler() {
        return (Compiler) compilers[getIndex()].getProcessor();
    }
    public String[] getValues() {
        return ProcessorEnumValue.getValues(compilers);
    }
}
