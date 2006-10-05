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
package net.sf.antcontrib.cpptasks.ibm;
import java.util.Vector;

import net.sf.antcontrib.cpptasks.compiler.LinkType;
import net.sf.antcontrib.cpptasks.compiler.Linker;
import net.sf.antcontrib.cpptasks.gcc.AbstractLdLinker;
import net.sf.antcontrib.cpptasks.gcc.GccLibrarian;
/**
 * Adapter for IBM(r) Visual Age(tm) Linker for AIX(tm)
 * 
 * @author Curt Arnold
 */
public final class VisualAgeLinker extends AbstractLdLinker {
    private static final String[] discardFiles = new String[]{};
    private static final String[] objFiles = new String[]{".o", ".a", ".lib",
            ".dll", ".so", ".sl"};
    private static final VisualAgeLinker dllLinker = new VisualAgeLinker(
            "makeC++SharedLib", objFiles, discardFiles, "lib", ".so");
    private static final VisualAgeLinker instance = new VisualAgeLinker("xlC",
            objFiles, discardFiles, "", "");
    public static VisualAgeLinker getInstance() {
        return instance;
    }
    private VisualAgeLinker(String command, String[] extensions,
            String[] ignoredExtensions, String outputPrefix, String outputSuffix) {
        //
        //  just guessing that -? might display something useful
        //
        super(command, "-?", extensions, ignoredExtensions, outputPrefix,
                outputSuffix, false, null);
    }
    public void addImpliedArgs(boolean debug, LinkType linkType, Vector args) {
        if (debug) {
            //args.addElement("-g");
        }
        if (linkType.isSharedLibrary()) {
            //args.addElement("-G");
        }
    }
    public Linker getLinker(LinkType type) {
        if (type.isStaticLibrary()) {
            return GccLibrarian.getInstance();
        }
        if (type.isSharedLibrary()) {
            return dllLinker;
        }
        return instance;
    }
    /**
     * Gets identifier for the compiler.
     * 
     * Initial attempt at extracting version information
     * would lock up.  Using a stock response.
     */
    public String getIdentifier() {
    	return "VisualAge linker - unidentified version";
    }
    
}
