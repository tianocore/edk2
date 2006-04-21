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
package net.sf.antcontrib.cpptasks.compaq;
import java.io.File;
import java.util.Vector;

import net.sf.antcontrib.cpptasks.compiler.CommandLineLinker;
import net.sf.antcontrib.cpptasks.compiler.LinkType;
import net.sf.antcontrib.cpptasks.compiler.Linker;
import net.sf.antcontrib.cpptasks.devstudio.DevStudioLibrarian;
import net.sf.antcontrib.cpptasks.devstudio.DevStudioProcessor;
import net.sf.antcontrib.cpptasks.types.LibraryTypeEnum;

/**
 * Adapter for the Compaq(r) Visual Fortran Librarian
 * 
 * @author Curt Arnold
 */
public class CompaqVisualFortranLibrarian extends CommandLineLinker {
    private static final CompaqVisualFortranLibrarian instance = new CompaqVisualFortranLibrarian();
    public static CompaqVisualFortranLibrarian getInstance() {
        return instance;
    }
    private CompaqVisualFortranLibrarian() {
        super("lib", "/bogus", new String[]{".obj"}, new String[0], ".lib",
                false, null);
    }
    protected void addBase(long base, Vector args) {
    }
    protected void addFixed(Boolean fixed, Vector args) {
    }
    protected void addImpliedArgs(boolean debug, LinkType linkType, Vector args, Boolean defaultflag) {
        args.addElement("/nologo");
    }
    protected void addIncremental(boolean incremental, Vector args) {
    }
    protected void addMap(boolean map, Vector args) {
    }
    protected void addStack(int stack, Vector args) {
    }
    /* (non-Javadoc)
     * @see net.sf.antcontrib.cpptasks.compiler.CommandLineLinker#addEntry(int, java.util.Vector)
     */
    protected void addEntry(String entry, Vector args) {
    }
    
    protected String getCommandFileSwitch(String commandFile) {
        return DevStudioProcessor.getCommandFileSwitch(commandFile);
    }
    public File[] getLibraryPath() {
        return new File[0];
    }
    public String[] getLibraryPatterns(String[] libnames, LibraryTypeEnum libType) {
        return new String[0];
    }
    public Linker getLinker(LinkType type) {
        return CompaqVisualFortranLinker.getInstance().getLinker(type);
    }
    protected int getMaximumCommandLength() {
        return DevStudioLibrarian.getInstance().getMaximumCommandLength();
    }
    protected String[] getOutputFileSwitch(String outputFile) {
        return DevStudioLibrarian.getInstance().getOutputFileSwitch(outputFile);
    }
    public boolean isCaseSensitive() {
        return false;
    }
}
