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
import net.sf.antcontrib.cpptasks.arm.ADSLinker;
import net.sf.antcontrib.cpptasks.borland.BorlandLinker;
import net.sf.antcontrib.cpptasks.compaq.CompaqVisualFortranLinker;
import net.sf.antcontrib.cpptasks.compiler.Linker;
import net.sf.antcontrib.cpptasks.devstudio.DevStudioLinker;
import net.sf.antcontrib.cpptasks.gcc.GccLibrarian;
import net.sf.antcontrib.cpptasks.gcc.GccLinker;
import net.sf.antcontrib.cpptasks.gcc.GppLinker;
import net.sf.antcontrib.cpptasks.gcc.LdLinker;
import net.sf.antcontrib.cpptasks.hp.aCCLinker;
import net.sf.antcontrib.cpptasks.ibm.VisualAgeLinker;
import net.sf.antcontrib.cpptasks.intel.IntelLinux32Linker;
import net.sf.antcontrib.cpptasks.intel.IntelLinux64Linker;
import net.sf.antcontrib.cpptasks.intel.IntelWin32Linker;
import net.sf.antcontrib.cpptasks.os390.OS390Linker;
import net.sf.antcontrib.cpptasks.os400.IccLinker;
import net.sf.antcontrib.cpptasks.sun.C89Linker;
import net.sf.antcontrib.cpptasks.sun.ForteCCLinker;
import net.sf.antcontrib.cpptasks.ti.ClxxLinker;
import org.apache.tools.ant.types.EnumeratedAttribute;
/**
 * Enumeration of supported linkers
 * 
 * @author Curt Arnold
 *  
 */
public class LinkerEnum extends EnumeratedAttribute {
    private final static ProcessorEnumValue[] linkers = new ProcessorEnumValue[]{
            new ProcessorEnumValue("gcc", GccLinker.getInstance()),
            new ProcessorEnumValue("g++", GppLinker.getInstance()),
            new ProcessorEnumValue("ld", LdLinker.getInstance()),
            new ProcessorEnumValue("ar", GccLibrarian.getInstance()),
            new ProcessorEnumValue("msvc", DevStudioLinker.getInstance()),
            new ProcessorEnumValue("bcc", BorlandLinker.getInstance()),
            new ProcessorEnumValue("df", CompaqVisualFortranLinker
                    .getInstance()),
            new ProcessorEnumValue("icl", IntelWin32Linker.getInstance()),
            new ProcessorEnumValue("ecl", IntelWin32Linker.getInstance()),
            new ProcessorEnumValue("icc", IntelLinux32Linker.getInstance()),
            new ProcessorEnumValue("ecc", IntelLinux64Linker.getInstance()),
            new ProcessorEnumValue("CC", ForteCCLinker.getInstance()),
            new ProcessorEnumValue("aCC", aCCLinker.getInstance()),
            new ProcessorEnumValue("os390", OS390Linker.getInstance()),
            new ProcessorEnumValue("os390batch", OS390Linker
                    .getDataSetInstance()),
            new ProcessorEnumValue("os400", IccLinker.getInstance()),
            new ProcessorEnumValue("sunc89", C89Linker.getInstance()),
            new ProcessorEnumValue("xlC", VisualAgeLinker.getInstance()),
            new ProcessorEnumValue("cl6x", ClxxLinker.getCl6xInstance()),
            new ProcessorEnumValue("cl55", ClxxLinker.getCl55Instance()),
            new ProcessorEnumValue("armcc", ADSLinker.getInstance()),
            new ProcessorEnumValue("armcpp", ADSLinker.getInstance()),
            new ProcessorEnumValue("tcc", ADSLinker.getInstance()),
            new ProcessorEnumValue("tcpp", ADSLinker.getInstance()),
            // gcc cross compilers
            new ProcessorEnumValue(
                    "sparc-sun-solaris2-gcc",
                    net.sf.antcontrib.cpptasks.gcc.cross.sparc_sun_solaris2.GccLinker
                            .getInstance()),
            new ProcessorEnumValue(
                    "sparc-sun-solaris2-g++",
                    net.sf.antcontrib.cpptasks.gcc.cross.sparc_sun_solaris2.GppLinker
                            .getInstance()),
            new ProcessorEnumValue(
                    "sparc-sun-solaris2-ld",
                    net.sf.antcontrib.cpptasks.gcc.cross.sparc_sun_solaris2.LdLinker
                            .getInstance()),
            new ProcessorEnumValue(
                    "sparc-sun-solaris2-ar",
                    net.sf.antcontrib.cpptasks.gcc.cross.sparc_sun_solaris2.GccLibrarian
                            .getInstance()),
            new ProcessorEnumValue("gcc-cross",
                    net.sf.antcontrib.cpptasks.gcc.cross.GccLinker
                            .getInstance()),
            new ProcessorEnumValue("g++-cross",
                    net.sf.antcontrib.cpptasks.gcc.cross.GppLinker
                            .getInstance()),
            new ProcessorEnumValue("ld-cross",
                    net.sf.antcontrib.cpptasks.gcc.cross.LdLinker.getInstance()),
            new ProcessorEnumValue("ar-cross",
                    net.sf.antcontrib.cpptasks.gcc.cross.GccLibrarian
                            .getInstance()),};
    public Linker getLinker() {
        return (Linker) linkers[getIndex()].getProcessor();
    }
    public String[] getValues() {
        return ProcessorEnumValue.getValues(linkers);
    }
}
