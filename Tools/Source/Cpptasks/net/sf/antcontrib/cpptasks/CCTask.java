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

import java.io.File;
import java.io.IOException;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Iterator;
import java.util.Vector;

import net.sf.antcontrib.cpptasks.compiler.AslcompilerConfiguration;
import net.sf.antcontrib.cpptasks.compiler.AssemblerConfiguration;
import net.sf.antcontrib.cpptasks.compiler.CompilerConfiguration;
import net.sf.antcontrib.cpptasks.compiler.LinkType;
import net.sf.antcontrib.cpptasks.compiler.Linker;
import net.sf.antcontrib.cpptasks.compiler.LinkerConfiguration;
import net.sf.antcontrib.cpptasks.compiler.Processor;
import net.sf.antcontrib.cpptasks.compiler.ProcessorConfiguration;
import net.sf.antcontrib.cpptasks.types.AslcompilerArgument;
import net.sf.antcontrib.cpptasks.types.AssemblerArgument;
import net.sf.antcontrib.cpptasks.types.CompilerArgument;
import net.sf.antcontrib.cpptasks.types.ConditionalFileSet;
import net.sf.antcontrib.cpptasks.types.DefineSet;
import net.sf.antcontrib.cpptasks.types.IncludePath;
import net.sf.antcontrib.cpptasks.types.LibrarySet;
import net.sf.antcontrib.cpptasks.types.LinkerArgument;
import net.sf.antcontrib.cpptasks.types.SystemIncludePath;
import net.sf.antcontrib.cpptasks.types.SystemLibrarySet;
import net.sf.antcontrib.cpptasks.userdefine.UserDefineCompiler;
import net.sf.antcontrib.cpptasks.userdefine.UserDefineDef;
import net.sf.antcontrib.cpptasks.VersionInfo;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Project;
import org.apache.tools.ant.Task;
import org.apache.tools.ant.types.Environment;

/**
 * Compile, link, assembler and asl compile task.
 * 
 * <p>
 * This task can compile various source languages and produce executables,
 * shared libraries (aka DLL's) and static libraries. Compiler adaptors are
 * currently available for several C/C++ compilers, FORTRAN, MIDL and Windows
 * Resource files. Assembler adaptors are currently available for MASM and GAS.
 * And aslcompiler support to ASL and IASL command.
 * </p>
 * 
 * 
 * <p>
 * Copyright (c) 2001-2005, The Ant-Contrib project.
 * </p>
 * 
 * <p>
 * Licensed under the Apache Software License 2.0,
 * http://www.apache.org/licenses/LICENSE-2.0.
 * </p>
 * 
 * <p>
 * For use with Apache Ant 1.5 or later. This software is not a product of the
 * of the Apache Software Foundation and no endorsement is implied.
 * </p>
 * 
 * <p>
 * THIS SOFTWARE IS PROVIDED 'AS-IS', See
 * http://www.apache.org/licenses/LICENSE-2.0 for additional disclaimers.
 * </p>
 * 
 * To use:
 * <ol>
 * <li>Place cpptasks.jar into the lib directory of Ant 1.5 or later.</li>
 * <li>Add &lt;taskdef resource="cpptasks.tasks"/&gt; and &lt;typedef
 * resource="cpptasks.types"/&gt; to build.xml.</li>
 * <li>Add &lt;cc/&gt;, &lt;compiler/&gt; &lt;linker/&gt; &lt;assembler/&gt;
 * and &lt;aslcompiler/&gt elements to project.</li>
 * <li>Set path and environment variables to be able to run compiler from
 * command line.</li>
 * <li>Build project.</li>
 * </ol>
 * 
 * @author Adam Murdoch
 * @author Curt Arnold
 */
public class CCTask extends Task {
    private class SystemLibraryCollector implements FileVisitor {
        private Hashtable libraries;

        private Linker linker;

        public SystemLibraryCollector (Linker linker, Hashtable libraries) {
            this.linker = linker;
            this.libraries = libraries;
        }

        public void visit(File basedir, String filename) {
            if (linker.bid(filename) > 0) {
                File libfile = new File(basedir, filename);
                String key = linker.getLibraryKey(libfile);
                libraries.put(key, libfile);
            }
        }
    }

    private static final ProcessorConfiguration[] EMPTY_CONFIG_ARRAY = new ProcessorConfiguration[0];

    /**
     * Builds a Hashtable to targets needing to be rebuilt keyed by compiler
     * configuration
     */
    public static Hashtable getTargetsToBuildByConfiguration(Hashtable targets) {
        Hashtable targetsByConfig = new Hashtable();
        Enumeration targetEnum = targets.elements();
        while (targetEnum.hasMoreElements()) {
            TargetInfo target = (TargetInfo) targetEnum.nextElement();
            if (target.getRebuild()) {
                Vector targetsForSameConfig = (Vector) targetsByConfig
                                .get(target.getConfiguration());
                if (targetsForSameConfig != null) {
                    targetsForSameConfig.addElement(target);
                } else {
                    targetsForSameConfig = new Vector();
                    targetsForSameConfig.addElement(target);
                    targetsByConfig.put(target.getConfiguration(),
                                    targetsForSameConfig);
                }
            }
        }
        return targetsByConfig;
    }

    /** The userdefine definitions. */
    private Vector _userdefines = new Vector();
    
    /** The compiler definitions. */
    private Vector _compilers = new Vector();

    /** The output file type. */
    // private LinkType _linkType = LinkType.EXECUTABLE;
    /** The library sets. */
    private Vector _libsets = new Vector();

    /** The aslcompiler definitions. */
    private Vector _aslcompiler = new Vector();

    /** The assembler definitions. */
    private Vector _assemblers = new Vector();

    /** The linker definitions. */
    private Vector _linkers = new Vector();

    /** The object directory. */
    private File _objDir;

    /** The output file. */
    private File _outfile;
    
    private boolean userdefine = false;
    private String arch;
    private String os;
    private String vendor;
    
    /** the flag for assembler */
    private boolean assembler = true;

    /** the flag for aslcompiler */
    private boolean aslcompiler = true;

    /** The linker definitions. */
    private final Vector targetPlatforms = new Vector();

    /** The distributer definitions. */
    private Vector distributers = new Vector();

    /**
     * If true, stop build on compile failure.
     */
    protected boolean failOnError = true;

    /**
     * Content that appears in <cc>and also in <compiler>are maintained by a
     * captive CompilerDef instance
     */
    private final CompilerDef compilerDef = new CompilerDef();

    /**
     * Content that appears in <cc>and also in <aslcompiler>are maintained by a
     * captive AslcompilerDef instance
     */
    private final AslcompilerDef aslcompilerDef = new AslcompilerDef();

    /** The OS390 dataset to build to object to */
    private String dataset;

    /**
     * 
     * Depth of dependency checking
     * 
     * Values < 0 indicate full dependency checking Values >= 0 indicate partial
     * dependency checking and for superficial compilation checks. Will throw
     * BuildException before attempting link
     */
    private int dependencyDepth = -1;

    /**
     * Content that appears in <cc>and also in <assembler>are maintained by a
     * captive AssemblerDef instance
     */
    private final AssemblerDef assemblerDef = new AssemblerDef();

    /**
     * Content that appears in <cc>and also in <linker>are maintained by a
     * captive CompilerDef instance
     */
    private final LinkerDef linkerDef = new LinkerDef();

    /**
     * contains the subsystem, output type and
     * 
     */
    private final LinkType linkType = new LinkType();

    /**
     * The property name which will be set with the physical filename of the
     * file that is generated by the linker
     */
    private String outputFileProperty;

    /**
     * if relentless = true, compilations should attempt to compile as many
     * files as possible before throwing a BuildException
     */
    private boolean relentless;

    public CCTask () {
    }

    
    public void addConfiguredCommand(UserDefineDef userdefineDef) {
        if (userdefineDef == null) {
            throw new NullPointerException("UserDefineDef");
        }
        userdefineDef.setProject(getProject());
        _userdefines.addElement(userdefineDef);
    }
    /**
     * Adds a asl compiler definition or reference.
     * 
     * @param Aslcompiler
     *            aslcompiler
     * @throws NullPointerException
     *             if aslcompiler is null
     */
    public void addConfiguredAslcompiler(AslcompilerDef aslcompier) {
        if (aslcompier == null) {
            throw new NullPointerException("aslcompier");
        }
        aslcompier.setProject(getProject());
        _aslcompiler.addElement(aslcompier);
    }

    /**
     * Adds a asl command-line arg. Argument will be inherited by all nested
     * aslcompiler elements that do not have inherit="false".
     * 
     */
    public void addConfiguredAslcompilerArg(AslcompilerArgument arg) {
        aslcompilerDef.addConfiguredAslcompilerArg(arg);
    }

    /**
     * Adds a assembler definition or reference.
     * 
     * @param assembler
     *            assemblera
     * @throws NullPointerException
     *             if assembler is null
     */
    public void addConfiguredAssembler(AssemblerDef assembler) {
        if (assembler == null) {
            throw new NullPointerException("assembler");
        }
        assembler.setProject(getProject());
        _assemblers.addElement(assembler);
    }

    /**
     * Adds a assembler command-line arg. Argument will be inherited by all
     * nested assembler elements that do not have inherit="false".
     * 
     */
    public void addConfiguredAssemblerArg(AssemblerArgument arg) {
        assemblerDef.addConfiguredAssemblerArg(arg);
    }

    /**
     * Adds a compiler definition or reference.
     * 
     * @param compiler
     *            compiler
     * @throws NullPointerException
     *             if compiler is null
     */
    public void addConfiguredCompiler(CompilerDef compiler) {
        if (compiler == null) {
            throw new NullPointerException("compiler");
        }
        compiler.setProject(getProject());
        _compilers.addElement(compiler);
    }

    /**
     * Adds a compiler command-line arg. Argument will be inherited by all
     * nested compiler elements that do not have inherit="false".
     * 
     */
    public void addConfiguredCompilerArg(CompilerArgument arg) {
        compilerDef.addConfiguredCompilerArg(arg);
    }

    /**
     * Adds a defineset. Will be inherited by all compiler elements that do not
     * have inherit="false".
     * 
     * @param defs
     *            Define set
     */
    public void addConfiguredDefineset(DefineSet defs) {
        compilerDef.addConfiguredDefineset(defs);
    }

    /**
     * Adds a linker definition. The first linker that is not disqualified by
     * its "if" and "unless" attributes will perform the link. If no child
     * linker element is active, the linker implied by the cc elements name or
     * classname attribute will be used.
     * 
     * @param linker
     *            linker
     * @throws NullPointerException
     *             if linker is null
     */
    public void addConfiguredLinker(LinkerDef linker) {
        if (linker == null) {
            throw new NullPointerException("linker");
        }
        linker.setProject(getProject());
        _linkers.addElement(linker);
    }

    /**
     * Adds a linker command-line arg. Argument will be inherited by all nested
     * linker elements that do not have inherit="false".
     */
    public void addConfiguredLinkerArg(LinkerArgument arg) {
        linkerDef.addConfiguredLinkerArg(arg);
    }

    /**
     * Add an environment variable to the launched process.
     */
    public void addEnv(Environment.Variable var) {
        compilerDef.addEnv(var);
        linkerDef.addEnv(var);
        assemblerDef.addEnv(var);
        aslcompilerDef.addEnv(var);
    }

    /**
     * Adds a source file set.
     * 
     * Files in these filesets will be auctioned to the available compiler
     * configurations, with the default compiler implied by the cc element
     * bidding last. If no compiler is interested in the file, it will be passed
     * to the linker.
     * 
     * To have a file be processed by a particular compiler configuration, add a
     * fileset to the corresponding compiler element.
     */
    public void addFileset(ConditionalFileSet srcSet) {
        compilerDef.addFileset(srcSet);
    }

    /**
     * Adds a library set.
     * 
     * Library sets will be inherited by all linker elements that do not have
     * inherit="false".
     * 
     * @param libset
     *            library set
     * @throws NullPointerException
     *             if libset is null.
     */
    public void addLibset(LibrarySet libset) {
        if (libset == null) {
            throw new NullPointerException("libset");
        }
        linkerDef.addLibset(libset);
    }

    /**
     * Adds a system library set. Timestamps and locations of system library
     * sets are not used in dependency analysis.
     * 
     * Essential libraries (such as C Runtime libraries) should not be specified
     * since the task will attempt to identify the correct libraries based on
     * the multithread, debug and runtime attributes.
     * 
     * System library sets will be inherited by all linker elements that do not
     * have inherit="false".
     * 
     * @param libset
     *            library set
     * @throws NullPointerException
     *             if libset is null.
     */
    public void addSyslibset(SystemLibrarySet libset) {
        if (libset == null) {
            throw new NullPointerException("libset");
        }
        linkerDef.addSyslibset(libset);
    }

    /**
     * Checks all targets that are not forced to be rebuilt or are missing
     * object files to be checked for modified include files
     * 
     * @returns total number of targets to be rebuilt
     * 
     */
    protected int checkForChangedIncludeFiles(Hashtable targets) {
        int potentialTargets = 0;
        int definiteTargets = 0;
        Enumeration targetEnum = targets.elements();
        while (targetEnum.hasMoreElements()) {
            TargetInfo target = (TargetInfo) targetEnum.nextElement();
            if (!target.getRebuild()) {
                potentialTargets++;
            } else {
                definiteTargets++;
            }
        }
        //
        // If there were remaining targets that
        // might be out of date
        //
        if (potentialTargets > 0) {
            log("Starting dependency analysis for "
                            + Integer.toString(potentialTargets) + " files.");
            DependencyTable dependencyTable = new DependencyTable(_objDir);
            try {
                dependencyTable.load();
            } catch (Exception ex) {
                log("Problem reading dependencies.xml: " + ex.toString());
            }
            targetEnum = targets.elements();
            while (targetEnum.hasMoreElements()) {
                TargetInfo target = (TargetInfo) targetEnum.nextElement();
                if (!target.getRebuild()) {
                    if (dependencyTable.needsRebuild(this, target,
                                    dependencyDepth)) {
                        target.mustRebuild();
                    }
                }
            }
            dependencyTable.commit(this);
        }
        //
        // count files being rebuilt now
        //
        int currentTargets = 0;
        targetEnum = targets.elements();
        while (targetEnum.hasMoreElements()) {
            TargetInfo target = (TargetInfo) targetEnum.nextElement();
            if (target.getRebuild()) {
                currentTargets++;
            }
        }
        if (potentialTargets > 0) {
            log(Integer.toString(potentialTargets - currentTargets
                            + definiteTargets)
                            + " files are up to date.");
            log(Integer.toString(currentTargets - definiteTargets)
                            + " files to be recompiled from dependency analysis.");
        }
        log(Integer.toString(currentTargets) + " total files to be compiled.");
        return currentTargets;
    }

    protected LinkerConfiguration collectExplicitObjectFiles(
                    Vector objectFiles, Vector sysObjectFiles) {
        //
        // find the first eligible linker
        //
        //
        ProcessorConfiguration linkerConfig = null;
        LinkerDef selectedLinkerDef = null;
        Linker selectedLinker = null;
        Hashtable sysLibraries = new Hashtable();
        TargetDef targetPlatform = getTargetPlatform();
        FileVisitor objCollector = null;
        FileVisitor sysLibraryCollector = null;
        for (int i = 0; i < _linkers.size(); i++) {
            LinkerDef currentLinkerDef = (LinkerDef) _linkers.elementAt(i);
            if (currentLinkerDef.isActive()) {
                selectedLinkerDef = currentLinkerDef;
                selectedLinker = currentLinkerDef.getProcessor().getLinker(
                                linkType);
                //
                // skip the linker if it doesn't know how to
                // produce the specified link type
                if (selectedLinker != null) {
                    linkerConfig = currentLinkerDef.createConfiguration(this,
                                    linkType, linkerDef, targetPlatform);
                    if (linkerConfig != null) {
                        //
                        // create collectors for object files
                        // and system libraries
                        objCollector = new ObjectFileCollector(selectedLinker,
                                        objectFiles);
                        sysLibraryCollector = new SystemLibraryCollector(
                                        selectedLinker, sysLibraries);
                        //
                        // if the <linker> has embedded <fileset>'s
                        // (such as linker specific libraries)
                        // add them as object files.
                        //
                        if (currentLinkerDef.hasFileSets()) {
                            currentLinkerDef.visitFiles(objCollector);
                        }
                        //
                        // user libraries are just a specialized form
                        // of an object fileset
                        selectedLinkerDef.visitUserLibraries(selectedLinker,
                                        objCollector);
                    }
                    break;
                }
            }
        }
        if (linkerConfig == null) {
            linkerConfig = linkerDef.createConfiguration(this, linkType, null,
                            targetPlatform);
            selectedLinker = (Linker) linkerDef.getProcessor().getLinker(
                            linkType);
            objCollector = new ObjectFileCollector(selectedLinker, objectFiles);
            sysLibraryCollector = new SystemLibraryCollector(selectedLinker,
                            sysLibraries);
        }
        //
        // unless there was a <linker> element that
        // explicitly did not inherit files from
        // containing <cc> element
        if (selectedLinkerDef == null || selectedLinkerDef.getInherit()) {
            linkerDef.visitUserLibraries(selectedLinker, objCollector);
            linkerDef.visitSystemLibraries(selectedLinker, sysLibraryCollector);
        }
        //
        // if there was a <syslibset> in a nested <linker>
        // evaluate it last so it takes priority over
        // identically named libs from <cc> element
        //
        if (selectedLinkerDef != null) {
            //
            // add any system libraries to the hashtable
            // done in reverse order so the earliest
            // on the classpath takes priority
            selectedLinkerDef.visitSystemLibraries(selectedLinker,
                            sysLibraryCollector);
        }
        //
        // copy over any system libraries to the
        // object files vector
        //
        Enumeration sysLibEnum = sysLibraries.elements();
        while (sysLibEnum.hasMoreElements()) {
            sysObjectFiles.addElement(sysLibEnum.nextElement());
        }
        return (LinkerConfiguration) linkerConfig;
    }

    /**
     * Adds an include path.
     * 
     * Include paths will be inherited by nested compiler elements that do not
     * have inherit="false".
     */
    public IncludePath createIncludePath() {
        return compilerDef.createIncludePath();
    }

    /**
     * Specifies precompilation prototype file and exclusions. Inherited by all
     * compilers that do not have inherit="false".
     * 
     */
    public PrecompileDef createPrecompile() throws BuildException {
        return compilerDef.createPrecompile();
    }

    /**
     * Adds a system include path. Locations and timestamps of files located
     * using the system include paths are not used in dependency analysis.
     * 
     * 
     * Standard include locations should not be specified. The compiler adapters
     * should recognized the settings from the appropriate environment variables
     * or configuration files.
     * 
     * System include paths will be inherited by nested compiler elements that
     * do not have inherit="false".
     */
    public SystemIncludePath createSysIncludePath() {
        return compilerDef.createSysIncludePath();
    }

    /**
     * Executes the task. Compiles the given files.
     * 
     * @throws BuildException
     *             if someting goes wrong with the build
     */
    public void execute() throws BuildException {
        //
        // if link type allowed objdir to be defaulted
        // provide it from outfile
        if (_objDir == null) {
            if (_outfile != null) {
                _objDir = new File(_outfile.getParent());
            } else {
                _objDir = new File(".");
            }
        }

        //
        // if the object directory does not exist
        //
        if (!_objDir.exists()) {
            throw new BuildException("Object directory does not exist");
        }
        
        //
        // if userdefine is true, then run all user defined command
        //
        if (userdefine) {
            Iterator iter = _userdefines.iterator();
            while( iter.hasNext()) {
                UserDefineDef userdefineDef = (UserDefineDef)iter.next();
                UserDefineCompiler userdefineCompiler = new UserDefineCompiler(this, userdefineDef);
                userdefineCompiler.command(this, userdefineDef);
            }
            return ;
        }
        
        TargetHistoryTable objHistory = new TargetHistoryTable(this, _objDir);
        //
        // determine the eventual linker configuration
        // (may be null) and collect any explicit
        // object files or libraries
        Vector objectFiles = new Vector();
        Vector sysObjectFiles = new Vector();
        LinkerConfiguration linkerConfig = collectExplicitObjectFiles(
                        objectFiles, sysObjectFiles);
        //
        // Assembler hashtable of all files
        // that we know how to compile (keyed by output file name)
        //
        Hashtable targets = getTargets(linkerConfig, objectFiles);
        Hashtable acpiTarget = new Hashtable();
        if (aslcompiler) {
            acpiTarget = getAcpiTargets(linkerConfig, new Vector());
        }
        Hashtable assemblerTarget = new Hashtable();
        if (assembler) {
            assemblerTarget = getAssemblerTargets(linkerConfig, objectFiles);
        }
        TargetInfo linkTarget = null;
        //
        // if output file is not specified,
        // then skip link step
        //
        if (_outfile != null) {
            linkTarget = getLinkTarget(linkerConfig, objectFiles,
                            sysObjectFiles, targets, assemblerTarget);
        }
        //
        // If specify the aslcompiler, then call asl compiler
        //
        if (aslcompiler) {
            BuildException acpiException = null;
            Hashtable targetsByConfig = getTargetsToBuildByConfiguration(acpiTarget);
            Enumeration acpiTargetEnum = targetsByConfig.elements();
            Vector[] targetVectors = new Vector[targetsByConfig.size()];
            int index = 0;
            while (acpiTargetEnum.hasMoreElements()) {
                Vector targetsForConfig = (Vector) acpiTargetEnum.nextElement();
                targetVectors[index++] = targetsForConfig;
            }
            for (int i = 0; i < targetVectors.length; i++) {
                //
                // get the targets for this configuration
                //
                Vector targetsForConfig = targetVectors[i];
                //
                // get the configuration from the first entry
                //
                AslcompilerConfiguration config = (AslcompilerConfiguration) ((TargetInfo) targetsForConfig
                                .elementAt(0)).getConfiguration();
                //
                // prepare the list of source files
                //
                String[] sourceFiles = new String[targetsForConfig.size()];
                Enumeration targetsEnum = targetsForConfig.elements();
                index = 0;
                while (targetsEnum.hasMoreElements()) {
                    TargetInfo targetInfo = ((TargetInfo) targetsEnum
                                    .nextElement());
                    sourceFiles[index++] = targetInfo.getSources()[0]
                                    .toString();
                }
                try {
                    config.aslcompiler(this, _objDir, sourceFiles);
                    log(sourceFiles.length
                                    + " total ACPI source files to be compiled.");
                } catch (BuildException ex) {
                    if (acpiException == null) {
                        acpiException = ex;
                    }
                    if (!relentless)
                        break;
                }
            }
        }
        //
        // If specify the assembler, then call assembler
        //
        if (assembler) {
            BuildException assemblerException = null;
            Hashtable targetsByConfig = getTargetsToBuildByConfiguration(assemblerTarget);
            Enumeration assembleTargetEnum = targetsByConfig.elements();
            Vector[] targetVectors = new Vector[targetsByConfig.size()];
            int index = 0;
            while (assembleTargetEnum.hasMoreElements()) {
                Vector targetsForConfig = (Vector) assembleTargetEnum
                                .nextElement();
                targetVectors[index++] = targetsForConfig;
            }
            for (int i = 0; i < targetVectors.length; i++) {
                //
                // get the targets for this configuration
                //
                Vector targetsForConfig = targetVectors[i];
                //
                // get the configuration from the first entry
                //
                AssemblerConfiguration config = (AssemblerConfiguration) ((TargetInfo) targetsForConfig
                                .elementAt(0)).getConfiguration();
                //
                // prepare the list of source files
                //
                String[] sourceFiles = new String[targetsForConfig.size()];
                Enumeration targetsEnum = targetsForConfig.elements();
                index = 0;
                while (targetsEnum.hasMoreElements()) {
                    TargetInfo targetInfo = ((TargetInfo) targetsEnum
                                    .nextElement());
                    sourceFiles[index++] = targetInfo.getSources()[0]
                                    .toString();
                }
                try {
                    config.assembler(this, _objDir, sourceFiles);
                    log(sourceFiles.length + " total files to be assembled.");
                } catch (BuildException ex) {
                    if (assemblerException == null) {
                        assemblerException = ex;
                    }
                    if (!relentless)
                        break;
                }
            }
            //
            // if we threw a assembler exception and
            // didn't throw it at the time because
            // we were relentless then
            // save the history and
            // throw the exception
            //
            if (assemblerException != null) {
                if (failOnError) {
                    throw assemblerException;
                } else {
                    log(assemblerException.getMessage(), Project.MSG_ERR);
                    return;
                }
            }
        }

        //
        // mark targets that don't have a history record or
        // whose source last modification time is not
        // the same as the history to be rebuilt
        //
        objHistory.markForRebuild(targets);
        CCTaskProgressMonitor monitor = new CCTaskProgressMonitor(objHistory);
        //
        // check for changed include files
        //
        int rebuildCount = checkForChangedIncludeFiles(targets);
        if (rebuildCount > 0) {
            BuildException compileException = null;
            //
            // compile all targets with getRebuild() == true
            //
            Hashtable targetsByConfig = getTargetsToBuildByConfiguration(targets);
            //
            // build array containing Vectors with precompiled generation
            // steps going first
            //
            Vector[] targetVectors = new Vector[targetsByConfig.size()];
            int index = 0;
            Enumeration targetVectorEnum = targetsByConfig.elements();
            while (targetVectorEnum.hasMoreElements()) {
                Vector targetsForConfig = (Vector) targetVectorEnum
                                .nextElement();
                //
                // get the configuration from the first entry
                //
                CompilerConfiguration config = (CompilerConfiguration) ((TargetInfo) targetsForConfig
                                .elementAt(0)).getConfiguration();
                if (config.isPrecompileGeneration()) {
                    targetVectors[index++] = targetsForConfig;
                }
            }
            targetVectorEnum = targetsByConfig.elements();
            while (targetVectorEnum.hasMoreElements()) {
                Vector targetsForConfig = (Vector) targetVectorEnum
                                .nextElement();
                for (int i = 0; i < targetVectors.length; i++) {
                    if (targetVectors[i] == targetsForConfig) {
                        break;
                    }
                    if (targetVectors[i] == null) {
                        targetVectors[i] = targetsForConfig;
                        break;
                    }
                }
            }
            for (int i = 0; i < targetVectors.length; i++) {
                //
                // get the targets for this configuration
                //
                Vector targetsForConfig = targetVectors[i];
                //
                // get the configuration from the first entry
                //
                CompilerConfiguration config = (CompilerConfiguration) ((TargetInfo) targetsForConfig
                                .elementAt(0)).getConfiguration();
                //
                // prepare the list of source files
                //
                String[] sourceFiles = new String[targetsForConfig.size()];
                Enumeration targetsEnum = targetsForConfig.elements();
                index = 0;
                while (targetsEnum.hasMoreElements()) {
                    TargetInfo targetInfo = ((TargetInfo) targetsEnum
                                    .nextElement());
                    sourceFiles[index++] = targetInfo.getSources()[0]
                                    .toString();
                }
                try {
                    config.compile(this, _objDir, sourceFiles, relentless,
                                    monitor);
                } catch (BuildException ex) {
                    if (compileException == null) {
                        compileException = ex;
                    }
                    if (!relentless)
                        break;
                }
            }
            //
            // save the details of the object file compilation
            // settings to disk for dependency analysis
            //
            try {
                objHistory.commit();
            } catch (IOException ex) {
                this.log("Error writing history.xml: " + ex.toString());
            }
            //
            // if we threw a compile exception and
            // didn't throw it at the time because
            // we were relentless then
            // save the history and
            // throw the exception
            //
            if (compileException != null) {
                if (failOnError) {
                    throw compileException;
                } else {
                    log(compileException.getMessage(), Project.MSG_ERR);
                    return;
                }
            }
        }
        //
        // if the dependency tree was not fully
        // evaluated, then throw an exception
        // since we really didn't do what we
        // should have done
        //
        //
        if (dependencyDepth >= 0) {
            throw new BuildException(
                            "All files at depth "
                                            + Integer.toString(dependencyDepth)
                                            + " from changes successfully compiled.\n"
                                            + "Remove or change dependencyDepth to -1 to perform full compilation.");
        }
        //
        // if no link target then
        // commit the history for the object files
        // and leave the task
        if (linkTarget != null) {
            //
            // get the history for the link target (may be the same
            // as the object history)
            TargetHistoryTable linkHistory = getLinkHistory(objHistory);
            //
            // see if it needs to be rebuilt
            //
            linkHistory.markForRebuild(linkTarget);
            //
            // if it needs to be rebuilt, rebuild it
            //
            File output = linkTarget.getOutput();
            if (linkTarget.getRebuild()) {
                log("Starting link");
                LinkerConfiguration linkConfig = (LinkerConfiguration) linkTarget
                                .getConfiguration();
                if (failOnError) {
                    linkConfig.link(this, linkTarget);
                } else {
                    try {
                        linkConfig.link(this, linkTarget);
                    } catch (BuildException ex) {
                        log(ex.getMessage(), Project.MSG_ERR);
                        return;
                    }
                }
                if (outputFileProperty != null)
                    getProject().setProperty(outputFileProperty,
                                    output.getAbsolutePath());
                linkHistory.update(linkTarget);
                try {
                    linkHistory.commit();
                } catch (IOException ex) {
                    log("Error writing link history.xml: " + ex.toString());
                }
            } else {
                if (outputFileProperty != null)
                    getProject().setProperty(outputFileProperty,
                                    output.getAbsolutePath());
            }
        }
    }

    /**
     * Gets the dataset.
     * 
     * @return Returns a String
     */
    public String getDataset() {
        return dataset;
    }

    protected TargetHistoryTable getLinkHistory(TargetHistoryTable objHistory) {
        File outputFileDir = new File(_outfile.getParent());
        //
        // if the output file is being produced in the link
        // directory, then we can use the same history file
        //
        if (_objDir.equals(outputFileDir)) {
            return objHistory;
        }
        return new TargetHistoryTable(this, outputFileDir);
    }

    protected TargetInfo getLinkTarget(LinkerConfiguration linkerConfig,
                    Vector objectFiles, Vector sysObjectFiles,
                    Hashtable compileTargets, Hashtable assemblerTargets) {
        //
        // walk the compile phase targets and
        // add those sources that have already been
        // assigned to the linker or
        // our output files the linker knows how to consume
        // files the linker knows how to consume
        //
        Enumeration compileTargetsEnum = compileTargets.elements();
        while (compileTargetsEnum.hasMoreElements()) {
            TargetInfo compileTarget = (TargetInfo) compileTargetsEnum
                            .nextElement();
            //
            // output of compile tasks
            //
            int bid = linkerConfig.bid(compileTarget.getOutput().toString());
            if (bid > 0) {
                objectFiles.addElement(compileTarget.getOutput());
            }
        }
        //
        // walk the assembler phase targets and
        // add those sources that have already been
        // assigned to the linker or
        // our output files the linker knows how to consume
        // files the linker knows how to consume
        //
        Enumeration assembleTargetsEnum = assemblerTargets.elements();
        while (assembleTargetsEnum.hasMoreElements()) {
            TargetInfo assemblerTarget = (TargetInfo) assembleTargetsEnum
                            .nextElement();
            //
            // output of assemble tasks
            //
            int bid = linkerConfig.bid(assemblerTarget.getOutput().toString());
            if (bid > 0) {
                objectFiles.addElement(assemblerTarget.getOutput());
            }
        }
        File[] objectFileArray = new File[objectFiles.size()];
        objectFiles.copyInto(objectFileArray);
        File[] sysObjectFileArray = new File[sysObjectFiles.size()];
        sysObjectFiles.copyInto(sysObjectFileArray);
        String baseName = _outfile.getName();
        String fullName = linkerConfig.getOutputFileName(baseName);
        File outputFile = new File(_outfile.getParent(), fullName);
        return new TargetInfo(linkerConfig, objectFileArray,
                        sysObjectFileArray, outputFile, linkerConfig
                                        .getRebuild());
    }

    public File getObjdir() {
        return _objDir;
    }

    public File getOutfile() {
        return _outfile;
    }

    public TargetDef getTargetPlatform() {
        return null;
    }

    /**
     * This method collects a Hashtable, keyed by output file name, of
     * TargetInfo's for every source file that is specified in the filesets of
     * the <aslcompiler> elements. The TargetInfo's contain the appropriate ACPI
     * configurations for their possible acpi
     * 
     */
    private Hashtable getAcpiTargets(LinkerConfiguration linkerConfig,
                    Vector objectFiles) {
        Hashtable targets = new Hashtable(1000);
        TargetDef targetPlatform = getTargetPlatform();
        Vector biddingProcessors = new Vector(_aslcompiler.size());
        for (int i = 0; i < _aslcompiler.size(); i++) {
            AslcompilerDef currentAslDef = (AslcompilerDef) _aslcompiler
                            .elementAt(i);
            if (currentAslDef.isActive()) {
                ProcessorConfiguration config = currentAslDef
                                .createConfiguration(this, linkType,
                                                aslcompilerDef, targetPlatform);
                //
                // if the aslcompiler has a fileset
                // then allow it to add its files to
                // the set of potential targets
                //
                ProcessorConfiguration[] localConfigs = new ProcessorConfiguration[] { config };
                if (currentAslDef.hasFileSets()) {
                    TargetMatcher matcher = new TargetMatcher(this, _objDir,
                                    localConfigs, linkerConfig, objectFiles,
                                    targets);
                    currentAslDef.visitFiles(matcher);
                }
                biddingProcessors.addElement(config);
            }
        }
        //
        // add fallback compiler at the end
        //
        ProcessorConfiguration config = aslcompilerDef.createConfiguration(
                        this, linkType, null, targetPlatform);
        biddingProcessors.addElement(config);
        ProcessorConfiguration[] bidders = new ProcessorConfiguration[biddingProcessors
                        .size()];
        biddingProcessors.copyInto(bidders);
        TargetMatcher matcher = new TargetMatcher(this, _objDir, bidders,
                        linkerConfig, objectFiles, targets);
        aslcompilerDef.visitFiles(matcher);
        return targets;
    }

    /**
     * This method collects a Hashtable, keyed by output file name, of
     * TargetInfo's for every source file that is specified in the filesets of
     * the <assembler> elements. The TargetInfo's contain the appropriate
     * assembler configurations for their possible assembly
     * 
     */
    private Hashtable getAssemblerTargets(LinkerConfiguration linkerConfig,
                    Vector objectFiles) {
        Hashtable targets = new Hashtable(1000);
        TargetDef targetPlatform = getTargetPlatform();
        Vector biddingProcessors = new Vector(_assemblers.size());
        for (int i = 0; i < _assemblers.size(); i++) {
            AssemblerDef currentAssemblerDef = (AssemblerDef) _assemblers
                            .elementAt(i);
            if (currentAssemblerDef.isActive()) {
                ProcessorConfiguration config = currentAssemblerDef
                                .createConfiguration(this, linkType,
                                                assemblerDef, targetPlatform);
                //
                // if the assembler has a fileset
                // then allow it to add its files to
                // the set of potential targets
                //
                ProcessorConfiguration[] localConfigs = new ProcessorConfiguration[] { config };
                if (currentAssemblerDef.hasFileSets()) {
                    TargetMatcher matcher = new TargetMatcher(this, _objDir,
                                    localConfigs, linkerConfig, objectFiles,
                                    targets);
                    currentAssemblerDef.visitFiles(matcher);
                }
                biddingProcessors.addElement(config);
            }
        }
        //
        // add fallback assembler at the end
        //
        ProcessorConfiguration config = assemblerDef.createConfiguration(this,
                        linkType, null, targetPlatform);
        biddingProcessors.addElement(config);
        ProcessorConfiguration[] bidders = new ProcessorConfiguration[biddingProcessors
                        .size()];
        biddingProcessors.copyInto(bidders);
        TargetMatcher matcher = new TargetMatcher(this, _objDir, bidders,
                        linkerConfig, objectFiles, targets);
        assemblerDef.visitFiles(matcher);
        return targets;
    }

    /**
     * This method collects a Hashtable, keyed by output file name, of
     * TargetInfo's for every source file that is specified in the filesets of
     * the <cc>and nested <compiler>elements. The TargetInfo's contain the
     * appropriate compiler configurations for their possible compilation
     * 
     */
    private Hashtable getTargets(LinkerConfiguration linkerConfig,
                    Vector objectFiles) {
        Hashtable targets = new Hashtable(1000);
        TargetDef targetPlatform = getTargetPlatform();
        //
        // find active (specialized) compilers
        //
        Vector biddingProcessors = new Vector(_compilers.size());
        for (int i = 0; i < _compilers.size(); i++) {
            CompilerDef currentCompilerDef = (CompilerDef) _compilers
                            .elementAt(i);
            if (currentCompilerDef.isActive()) {
                ProcessorConfiguration config = currentCompilerDef
                                .createConfiguration(this, linkType,
                                                compilerDef, targetPlatform);
                //
                // see if this processor had a precompile child element
                //
                PrecompileDef precompileDef = currentCompilerDef
                                .getActivePrecompile(compilerDef);
                ProcessorConfiguration[] localConfigs = new ProcessorConfiguration[] { config };
                //
                // if it does then
                //
                if (precompileDef != null) {
                    File prototype = precompileDef.getPrototype();
                    //
                    // will throw exceptions if prototype doesn't exist, etc
                    //
                    if (!prototype.exists()) {
                        throw new BuildException("prototype ("
                                        + prototype.toString()
                                        + ") does not exist.");
                    }
                    if (prototype.isDirectory()) {
                        throw new BuildException("prototype ("
                                        + prototype.toString()
                                        + ") is a directory.");
                    }
                    String[] exceptFiles = precompileDef.getExceptFiles();
                    //
                    // create a precompile building and precompile using
                    // variants of the configuration
                    // or return null if compiler doesn't support
                    // precompilation
                    CompilerConfiguration[] configs = ((CompilerConfiguration) config)
                                    .createPrecompileConfigurations(prototype,
                                                    exceptFiles);
                    if (configs != null && configs.length == 2) {
                        //
                        // visit the precompiled file to add it into the
                        // targets list (just like any other file if
                        // compiler doesn't support precompilation)
                        TargetMatcher matcher = new TargetMatcher(
                                        this,
                                        _objDir,
                                        new ProcessorConfiguration[] { configs[0] },
                                        linkerConfig, objectFiles, targets);
                        matcher.visit(new File(prototype.getParent()),
                                        prototype.getName());
                        //
                        // only the configuration that uses the
                        // precompiled header gets added to the bidding list
                        biddingProcessors.addElement(configs[1]);
                        localConfigs = new ProcessorConfiguration[2];
                        localConfigs[0] = configs[1];
                        localConfigs[1] = config;
                    }
                }
                //
                // if the compiler has a fileset
                // then allow it to add its files
                // to the set of potential targets
                if (currentCompilerDef.hasFileSets()) {
                    TargetMatcher matcher = new TargetMatcher(this, _objDir,
                                    localConfigs, linkerConfig, objectFiles,
                                    targets);
                    currentCompilerDef.visitFiles(matcher);
                }
                biddingProcessors.addElement(config);
            }
        }
        //
        // add fallback compiler at the end
        //
        ProcessorConfiguration config = compilerDef.createConfiguration(this,
                        linkType, null, targetPlatform);
        biddingProcessors.addElement(config);
        ProcessorConfiguration[] bidders = new ProcessorConfiguration[biddingProcessors
                        .size()];
        biddingProcessors.copyInto(bidders);
        //
        // bid out the <fileset>'s in the cctask
        //
        TargetMatcher matcher = new TargetMatcher(this, _objDir, bidders,
                        linkerConfig, objectFiles, targets);
        compilerDef.visitFiles(matcher);
        return targets;
    }

    /**
     * Sets the default compiler adapter. Use the "name" attribute when the
     * compiler is a supported compiler.
     * 
     * @param classname
     *            fully qualified classname which implements CompilerAdapter
     */
    public void setClassname(String classname) {
        compilerDef.setClassname(classname);
        linkerDef.setClassname(classname);
        assemblerDef.setClassname(classname);
        aslcompilerDef.setClassname(classname);
    }

    /**
     * Sets the dataset for OS/390 builds.
     * 
     * @param dataset
     *            The dataset to set
     */
    public void setDataset(String dataset) {
        this.dataset = dataset;
    }

    /**
     * Enables or disables generation of debug info.
     */
    public void setDebug(boolean debug) {
        compilerDef.setDebug(debug);
        linkerDef.setDebug(debug);
        assemblerDef.setDebug(debug);
        aslcompilerDef.setDebug(debug);
    }

    /**
     * Deprecated.
     * 
     * Controls the depth of the dependency evaluation. Used to do a quick check
     * of changes before a full build.
     * 
     * Any negative value which will perform full dependency checking. Positive
     * values will truncate dependency checking. A value of 0 will cause only
     * those files that changed to be recompiled, a value of 1 which cause files
     * that changed or that explicitly include a file that changed to be
     * recompiled.
     * 
     * Any non-negative value will cause a BuildException to be thrown before
     * attempting a link or completing the task.
     * 
     */
    public void setDependencyDepth(int depth) {
        dependencyDepth = depth;
    }

    /**
     * Enables generation of exception handling code
     */
    public void setExceptions(boolean exceptions) {
        compilerDef.setExceptions(exceptions);
    }

    /**
     * Enables run-time type information.
     */
    public void setRtti(boolean rtti) {
        compilerDef.setRtti(rtti);
    }

    // public LinkType getLinkType() {
    // return linkType;
    // }
    /**
     * Enables or disables incremental linking.
     * 
     * @param incremental
     *            new state
     */
    public void setIncremental(boolean incremental) {
        linkerDef.setIncremental(incremental);
    }

    /**
     * Set use of libtool.
     * 
     * If set to true, the "libtool " will be prepended to the command line for
     * compatible processors
     * 
     * @param libtool
     *            If true, use libtool.
     */
    public void setLibtool(boolean libtool) {
        compilerDef.setLibtool(libtool);
        linkerDef.setLibtool(libtool);
        assemblerDef.setLibtool(libtool);
        aslcompilerDef.setLibtool(libtool);
    }

    /**
     * Sets the output file type. Supported values "executable", "shared", and
     * "static". Deprecated, specify outtype instead.
     * 
     * @deprecated
     */
    public void setLink(OutputTypeEnum outputType) {
        linkType.setOutputType(outputType);
    }

    /**
     * Enables or disables generation of multithreaded code
     * 
     * @param multi
     *            If true, generated code may be multithreaded.
     */
    public void setMultithreaded(boolean multi) {
        compilerDef.setMultithreaded(multi);
    }

    //
    // keep near duplicate comment at CompilerDef.setName in sync
    //
    /**
     * Sets type of the default compiler and linker.
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
     * </table>
     * 
     */
    public void setName(CompilerEnum name) {
        compilerDef.setName(name);
        Processor compiler = compilerDef.getProcessor();
        Linker linker = compiler.getLinker(linkType);
        linkerDef.setProcessor(linker);
    }

    /**
     * Do not propagate old environment when new environment variables are
     * specified.
     */
    public void setNewenvironment(boolean newenv) {
        compilerDef.setNewenvironment(newenv);
        linkerDef.setNewenvironment(newenv);
        assemblerDef.setNewenvironment(newenv);
        aslcompilerDef.setNewenvironment(newenv);
    }

    /**
     * Sets the destination directory for object files.
     * 
     * Generally this should be a property expression that evaluates to distinct
     * debug and release object file directories.
     * 
     * @param dir
     *            object directory
     */
    public void setObjdir(File dir) {
        if (dir == null) {
            throw new NullPointerException("dir");
        }
        _objDir = dir;
    }

    /**
     * Sets the output file name. If not specified, the task will only compile
     * files and not attempt to link. If an extension is not specified, the task
     * may use a system appropriate extension and prefix, for example,
     * outfile="example" may result in "libexample.so" being created.
     * 
     * @param outfile
     *            output file name
     */
    public void setOutfile(File outfile) {
        //
        // if file name was empty, skip link step
        //
        if (outfile == null || outfile.toString().length() > 0) {
            _outfile = outfile;
        }
    }

    /**
     * Specifies the name of a property to set with the physical filename that
     * is produced by the linker
     */
    public void setOutputFileProperty(String outputFileProperty) {
        this.outputFileProperty = outputFileProperty;
    }

    /**
     * Sets the output file type. Supported values "executable", "shared", and
     * "static".
     */
    public void setOuttype(OutputTypeEnum outputType) {
        linkType.setOutputType(outputType);
    }

    /**
     * Sets the project.
     */
    public void setProject(Project project) {
        super.setProject(project);
        compilerDef.setProject(project);
        linkerDef.setProject(project);
        assemblerDef.setProject(project);
        aslcompilerDef.setProject(project);
    }

    /**
     * If set to true, all files will be rebuilt.
     * 
     * @paran rebuildAll If true, all files will be rebuilt. If false, up to
     *        date files will not be rebuilt.
     */
    public void setRebuild(boolean rebuildAll) {
        compilerDef.setRebuild(rebuildAll);
        linkerDef.setRebuild(rebuildAll);
        assemblerDef.setRebuild(rebuildAll);
        aslcompilerDef.setRebuild(rebuildAll);
    }

    /**
     * If set to true, compilation errors will not stop the task until all files
     * have been attempted.
     * 
     * @param relentless
     *            If true, don't stop on the first compilation error
     * 
     */
    public void setRelentless(boolean relentless) {
        this.relentless = relentless;
    }

    /**
     * Sets the type of runtime library, possible values "dynamic", "static".
     */
    public void setRuntime(RuntimeType rtlType) {
        linkType.setStaticRuntime((rtlType.getIndex() == 1));
    }

    /**
     * Sets the nature of the subsystem under which that the program will
     * execute.
     * 
     * <table width="100%" border="1"> <thead>Supported subsystems </thead>
     * <tr>
     * <td>gui</td>
     * <td>Graphical User Interface</td>
     * </tr>
     * <tr>
     * <td>console</td>
     * <td>Command Line Console</td>
     * </tr>
     * <tr>
     * <td>other</td>
     * <td>Other</td>
     * </tr>
     * </table>
     * 
     * @param subsystem
     *            subsystem
     * @throws NullPointerException
     *             if subsystem is null
     */
    public void setSubsystem(SubsystemEnum subsystem) {
        if (subsystem == null) {
            throw new NullPointerException("subsystem");
        }
        linkType.setSubsystem(subsystem);
    }

    /**
     * Enumerated attribute with the values "none", "severe", "default",
     * "production", "diagnostic", and "failtask".
     */
    public void setWarnings(CompilerDef.WarningLevel level) {
        compilerDef.setWarnings(level);
    }

    /**
     * Indicates whether the build will continue even if there are compilation
     * errors; defaults to true.
     * 
     * @param fail
     *            if true halt the build on failure
     */
    public void setFailonerror(boolean fail) {
        failOnError = fail;
    }

    /**
     * Gets the failonerror flag.
     * 
     * @return the failonerror flag
     */
    public boolean getFailonerror() {
        return failOnError;
    }

    /**
     * Adds descriptive version information to be included in the generated
     * file. The first active version info block will be used. (Non-functional
     * prototype)
     */
    public void addConfiguredVersioninfo(VersionInfo info) {
        linkerDef.addConfiguredVersioninfo(info);
    }

    /**
     * Adds a target definition or reference (Non-functional prototype).
     * 
     * @param target
     *            target
     * @throws NullPointerException
     *             if compiler is null
     */
    public void addConfiguredTarget(TargetDef target) {
        if (target == null) {
            throw new NullPointerException("target");
        }
        target.setProject(getProject());
        targetPlatforms.addElement(target);
    }

    /**
     * Adds a distributer definition or reference (Non-functional prototype).
     * 
     * @param distributer
     *            distributer
     * @throws NullPointerException
     *             if compiler is null
     */
    public void addConfiguredDistributer(DistributerDef distributer) {
        if (distributer == null) {
            throw new NullPointerException("distributer");
        }
        distributer.setProject(getProject());
        distributers.addElement(distributer);
    }

    /**
     * Sets optimization.
     * @param optimization
     */
    public void setOptimize(OptimizationEnum optimization) {
        compilerDef.setOptimize(optimization);
    }

    public boolean isAssembler() {
        return assembler;
    }

    public void setAssembler(boolean assembler) {
        this.assembler = assembler;
    }

    public boolean isAslcompiler() {
        return aslcompiler;
    }

    public void setAslcompiler(boolean aslcompiler) {
        this.aslcompiler = aslcompiler;
    }

    public boolean isUserdefine() {
        return userdefine;
    }

    public void setUserdefine(boolean userdefine) {
        this.userdefine = userdefine;
    }

    public String getArch() {
        return arch;
    }

    public void setArch(String arch) {
        this.arch = arch;
    }

    public String getOs() {
        return os;
    }

    public void setOs(String os) {
        this.os = os;
    }

    public String getVendor() {
        return vendor;
    }

    public void setVendor(String vendor) {
        this.vendor = vendor;
    }
    
    
}
