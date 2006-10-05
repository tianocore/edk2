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
package net.sf.antcontrib.cpptasks.borland;
import java.io.File;
import java.io.IOException;
import java.util.Vector;
import org.apache.tools.ant.BuildException;

import net.sf.antcontrib.cpptasks.CUtil;
import net.sf.antcontrib.cpptasks.compiler.CommandLineLinker;
import net.sf.antcontrib.cpptasks.compiler.LinkType;
import net.sf.antcontrib.cpptasks.compiler.Linker;
import net.sf.antcontrib.cpptasks.CCTask;
import net.sf.antcontrib.cpptasks.compiler.CommandLineLinkerConfiguration;
import net.sf.antcontrib.cpptasks.types.LibraryTypeEnum;

/**
 * Adapter for the Borland(r) tlib Librarian
 * 
 * @author Curt Arnold
 */
public class BorlandLibrarian extends CommandLineLinker {
    private static final BorlandLibrarian instance = new BorlandLibrarian();
    public static BorlandLibrarian getInstance() {
        return instance;
    }
    private BorlandLibrarian() {
        super("tlib", "--version", new String[]{".obj"}, new String[0], ".lib", false,
                null);
    }
    protected void addBase(long base, Vector args) {
    }
    protected void addFixed(Boolean fixed, Vector args) {
    }
    protected void addImpliedArgs(boolean debug, LinkType linkType, Vector args, Boolean defaultflag) {
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
    
    protected String getCommandFileSwitch(String cmdFile) {
        return BorlandProcessor.getCommandFileSwitch(cmdFile);
    }
    public File[] getLibraryPath() {
        return CUtil.getPathFromEnvironment("LIB", ";");
    }
    public String[] getLibraryPatterns(String[] libnames, LibraryTypeEnum libType) {
        return BorlandProcessor.getLibraryPatterns(libnames, libType);
    }
    public Linker getLinker(LinkType type) {
        return BorlandLinker.getInstance().getLinker(type);
    }
    public int getMaximumCommandLength() {
        return 1024;
    }
    public String[] getOutputFileSwitch(String outFile) {
        return BorlandProcessor.getOutputFileSwitch(outFile);
    }
    public boolean isCaseSensitive() {
        return BorlandProcessor.isCaseSensitive();
    }
    /**
     * Gets identifier for the linker.
     * 
     * TLIB will lockup when attempting to get version
     * information.  Since the Librarian version isn't critical
     * just return a stock response.
     */
    public String getIdentifier() {
    	return "TLIB 4.5 Copyright (c) 1987, 1999 Inprise Corporation";
    }
    
    /**
     * Prepares argument list for exec command.
     * 
     * @param outputFile
     *            linker output file
     * @param sourceFiles
     *            linker input files (.obj, .o, .res)
     * @param args
     *            linker arguments
     * @return arguments for runTask
     */
    protected String[] prepareArguments(
    		CCTask task,
    		String outputDir, 
			String outputName,
            String[] sourceFiles, 
			CommandLineLinkerConfiguration config) {
    	String[] preargs = config.getPreArguments();
        String[] endargs = config.getEndArguments();
        StringBuffer buf = new StringBuffer();
        Vector execArgs = new Vector(preargs.length + endargs.length + 10
                + sourceFiles.length);
        
        execArgs.addElement(this.getCommand());
        String outputFileName = new File(outputDir, outputName).toString();
        execArgs.addElement(quoteFilename(buf, outputFileName));

        for (int i = 0; i < preargs.length; i++) {
            execArgs.addElement(preargs[i]);
        }

        //
        //   add a place-holder for page size
        //
        int pageSizeIndex = execArgs.size();
        execArgs.addElement(null);
        
        int objBytes = 0;
        
        for (int i = 0; i < sourceFiles.length; i++) {
            String last4 = sourceFiles[i]
                    .substring(sourceFiles[i].length() - 4).toLowerCase();
            if (last4.equals(".def")) {
            } else {
                if (last4.equals(".res")) {
                } else {
                    if (last4.equals(".lib")) {
                    } else {
                        execArgs.addElement("+" + quoteFilename(buf, sourceFiles[i]));
                        objBytes += new File(sourceFiles[i]).length();
                    }
                }
            }
        }
        
        for (int i = 0; i < endargs.length; i++) {
            execArgs.addElement(endargs[i]);
        }
        
        String[] execArguments = new String[execArgs.size()];
        execArgs.copyInto(execArguments);

    	int minPageSize = objBytes >> 16;
    	int pageSize = 0;
    	for(int i = 4; i <= 15; i++) {
    		pageSize = 1 << i;
    		if (pageSize > minPageSize) break;
    	}
        execArguments[pageSizeIndex] = "/P" + Integer.toString(pageSize);
        
        return execArguments;
    }
    
    /**
     * Prepares argument list to execute the linker using a response file.
     * 
     * @param outputFile
     *            linker output file
     * @param args
     *            output of prepareArguments
     * @return arguments for runTask
     */
    protected String[] prepareResponseFile(File outputFile, String[] args)
            throws IOException {
    	return BorlandProcessor.prepareResponseFile(outputFile, args, " & \n");
    }
    
    /**
     * Builds a library
     *
     */
    public void link(CCTask task,
                     File outputFile,
                     String[] sourceFiles,
                     CommandLineLinkerConfiguration config)
                     throws BuildException
    {
    	//
    	//  delete any existing library
    	outputFile.delete();
    	//
    	//  build a new library
    	super.link(task, outputFile, sourceFiles, config);
    }
    
}
