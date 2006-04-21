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
package net.sf.antcontrib.cpptasks.gcc;
import java.io.File;
import java.util.Vector;

import net.sf.antcontrib.cpptasks.CCTask;
import net.sf.antcontrib.cpptasks.CUtil;
import net.sf.antcontrib.cpptasks.compiler.CommandLineLinker;
import net.sf.antcontrib.cpptasks.compiler.CommandLineLinkerConfiguration;
import net.sf.antcontrib.cpptasks.compiler.LinkType;
import net.sf.antcontrib.cpptasks.types.LibrarySet;
import net.sf.antcontrib.cpptasks.types.LibraryTypeEnum;

/**
 * Abstract adapter for ld-like linkers
 * 
 * @author Curt Arnold
 */
public abstract class AbstractLdLinker extends CommandLineLinker {
    private String outputPrefix;
    private static String[] defaultflags = new String[]{};
    protected AbstractLdLinker(String command, String identifierArg,
            String[] extensions, String[] ignoredExtensions,
            String outputPrefix, String outputSuffix, boolean isLibtool,
            AbstractLdLinker libtoolLinker) {
        super(command, identifierArg, extensions, ignoredExtensions,
                outputSuffix, isLibtool, libtoolLinker);
        this.outputPrefix = outputPrefix;
    }
    public void addBase(long base, Vector args) {
    	if (base >= 0) {
    		args.addElement("--image-base");
    		args.addElement(Long.toHexString(base));
    	}
    }
    public void addFixed(Boolean fixed, Vector args) {
    }
    protected void addImpliedArgs(boolean debug, LinkType linkType, Vector args, Boolean defaultflag) {
        if(defaultflag != null && defaultflag.booleanValue()){
            for (int i = 0; i < defaultflags.length; i++) {
                args.addElement(defaultflags[i]);
            }
        }
        if (debug) {
            args.addElement("-g");
        }
        if (isDarwin()) {
            if (linkType.isPluginModule()) {
                args.addElement("-bundle");
            } else {
                if (linkType.isSharedLibrary()) {
                    args.addElement("-prebind");
                    args.addElement("-dynamiclib");
                }
            }
        } else {
            if (linkType.isStaticRuntime()) {
                args.addElement("-static");
            }
            if (linkType.isPluginModule()) {
                args.addElement("-shared");
            } else {
                if (linkType.isSharedLibrary()) {
                    args.addElement("-shared");
                }
            }
        }
    }
    public void addIncremental(boolean incremental, Vector args) {
    	if (incremental) {
    		args.addElement("-i");
    	}
    }
    protected int addLibraryPatterns(String[] libnames, StringBuffer buf,
            String prefix, String extension, String[] patterns, int offset) {
        for (int i = 0; i < libnames.length; i++) {
            buf.setLength(0);
            buf.append(prefix);
            buf.append(libnames[i]);
            buf.append(extension);
            patterns[offset + i] = buf.toString();
        }
        return offset + libnames.length;
    }
    public String[] addLibrarySets(CCTask task, LibrarySet[] libsets,
            Vector preargs, Vector midargs, Vector endargs) {
        Vector libnames = new Vector();
        super.addLibrarySets(task, libsets, preargs, midargs, endargs);
        LibraryTypeEnum previousLibraryType = null;
        for (int i = 0; i < libsets.length; i++) {
            LibrarySet set = libsets[i];
            File libdir = set.getDir(null);
            String[] libs = set.getLibs();
            if (libdir != null) {
            	if (set.getType() != null && 
            			"framework".equals(set.getType().getValue()) &&
						isDarwin()) {
            		endargs.addElement("-F" + libdir.getAbsolutePath());            		
            	} else {
            		endargs.addElement("-L" + libdir.getAbsolutePath());
            	}
            }
            //
            //  if there has been a change of library type
            //
            if (set.getType() != previousLibraryType) {
            	if (set.getType() != null && "static".equals(set.getType().getValue())) {
            		endargs.addElement("-Bstatic");
            		previousLibraryType = set.getType();
            	} else {
            		if (set.getType() == null || 
            				!"framework".equals(set.getType().getValue()) ||
							!isDarwin()) {
            			endargs.addElement("-Bdynamic");
            			previousLibraryType = set.getType();
            		}
            	}
            }
            StringBuffer buf = new StringBuffer("-l");
            if (set.getType() != null && 
            		"framework".equals(set.getType().getValue()) && 
					isDarwin()) {
            	buf.setLength(0);
            	buf.append("-framework ");
            }
            int initialLength = buf.length();
            for (int j = 0; j < libs.length; j++) {
                //
                //  reset the buffer to just "-l"
                //
                buf.setLength(initialLength);
                //
                //  add the library name
                buf.append(libs[j]);
                libnames.addElement(libs[j]);
                //
                //  add the argument to the list
                endargs.addElement(buf.toString());
            }
        }
        String rc[] = new String[libnames.size()];
        for (int i = 0; i < libnames.size(); i++) {
            rc[i] = (String) libnames.elementAt(i);
        }
        return rc;
    }
    public void addMap(boolean map, Vector args) {
    	if (map) {
    		args.addElement("-M");
    	}
    }
    public void addStack(int stack, Vector args) {
    	if (stack > 0) {
    		args.addElement("--stack");
    		args.addElement(Integer.toString(stack));
    	}
    }
    /* (non-Javadoc)
     * @see net.sf.antcontrib.cpptasks.compiler.CommandLineLinker#addEntry(int, java.util.Vector)
     */
    protected void addEntry(String entry, Vector args) {
    	if (entry != null) {
    		args.addElement("-e");
    		args.addElement(entry);
    	}
    }
    
    public String getCommandFileSwitch(String commandFile) {
        throw new IllegalStateException("ld does not support command files");
    }
    /**
     * Returns library path.
     *  
     */
    protected File[] getEnvironmentIncludePath() {
        return CUtil.getPathFromEnvironment("LIB", ":");
    }
    public String getLibraryKey(File libfile) {
        String libname = libfile.getName();
        int lastDot = libname.lastIndexOf('.');
        if (lastDot >= 0) {
            return libname.substring(0, lastDot);
        }
        return libname;
    }
    /**
     * Returns library path.
     *  
     */
    public File[] getLibraryPath() {
        return new File[0];
    }
    public String[] getLibraryPatterns(String[] libnames, LibraryTypeEnum libType) {
        StringBuffer buf = new StringBuffer();
        int patternCount = libnames.length;
        if (libType == null) {
        	patternCount *= 2;
        }
        String[] patterns = new String[patternCount];
        int offset = 0;
        if (libType == null || "static".equals(libType.getValue())) {
        	offset = addLibraryPatterns(libnames, buf, "lib", ".a", patterns, 0);
        }
        if (libType != null && "framework".equals(libType.getValue()) && isDarwin()) {
        	for(int i = 0; i < libnames.length; i++) {
        		buf.setLength(0);
        		buf.append(libnames[i]);
        		buf.append(".framework/");
        		buf.append(libnames[i]);
        		patterns[offset++] = buf.toString();
        	}
        } else {
        	if (libType == null || !"static".equals(libType.getValue())) {
        		if (isHPUX()) {
        			offset = addLibraryPatterns(libnames, buf, "lib", ".sl", patterns,
        					offset);
        		} else {
        			offset = addLibraryPatterns(libnames, buf, "lib", ".so", patterns,
        					offset);
        		}
        	}
        }
        return patterns;
    }
    public int getMaximumCommandLength() {
        return Integer.MAX_VALUE;
    }
    public String getOutputFileName(String baseName) {
        return outputPrefix + super.getOutputFileName(baseName);
    }
    public String[] getOutputFileSwitch(String outputFile) {
        return GccProcessor.getOutputFileSwitch("-o", outputFile);
    }
    public boolean isCaseSensitive() {
        return true;
    }
    protected boolean isHPUX() {
        String osname = System.getProperty("os.name").toLowerCase();
        if (osname.indexOf("hp") >= 0 && osname.indexOf("ux") >= 0) {
            return true;
        }
        return false;
    }
    /**
     * Prepares argument list for exec command. Will return null if command
     * line would exceed allowable command line buffer.
     * 
     * @param outputFile
     *            linker output file
     * @param sourceFiles
     *            linker input files (.obj, .o, .res)
     * @param args
     *            linker arguments
     * @return arguments for runTask
     */
    public String[] prepareArguments(CCTask task, String outputDir,
            String outputFile, String[] sourceFiles,
            CommandLineLinkerConfiguration config) {
        //
        //   need to suppress sources that correspond to
        //        library set entries since they are already
        //        in the argument list
        String[] libnames = config.getLibraryNames();
        if (libnames == null || libnames.length == 0) {
            return super.prepareArguments(task, outputDir, outputFile,
                    sourceFiles, config);
        }
        //
        //
        //   null out any sources that correspond to library names
        //
        String[] localSources = (String[]) sourceFiles.clone();
        int extra = 0;
        for (int i = 0; i < libnames.length; i++) {
            String libname = libnames[i];
            for (int j = 0; j < localSources.length; j++) {
                if (localSources[j] != null
                        && localSources[j].indexOf(libname) > 0
                        && localSources[j].indexOf("lib") > 0) {
                    String filename = new File(localSources[j]).getName();
                    if (filename.startsWith("lib")
                            && filename.substring(3).startsWith(libname)) {
                        String extension = filename
                                .substring(libname.length() + 3);
                        if (extension.equals(".a") || extension.equals(".so")
                                || extension.equals(".sl")) {
                            localSources[j] = null;
                            extra++;
                        }
                    }
                }
            }
        }
        if (extra == 0) {
            return super.prepareArguments(task, outputDir, outputFile,
                    sourceFiles, config);
        }
        String[] finalSources = new String[localSources.length - extra];
        int index = 0;
        for (int i = 0; i < localSources.length; i++) {
            if (localSources[i] != null) {
                finalSources[index++] = localSources[i];
            }
        }
        return super.prepareArguments(task, outputDir, outputFile,
                finalSources, config);
    }
}
