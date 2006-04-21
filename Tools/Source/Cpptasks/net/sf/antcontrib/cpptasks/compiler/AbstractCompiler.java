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
package net.sf.antcontrib.cpptasks.compiler;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.io.Reader;
import java.util.Vector;
import net.sf.antcontrib.cpptasks.CCTask;
import net.sf.antcontrib.cpptasks.CUtil;
import net.sf.antcontrib.cpptasks.CompilerDef;
import net.sf.antcontrib.cpptasks.DependencyInfo;
import net.sf.antcontrib.cpptasks.ProcessorDef;
import net.sf.antcontrib.cpptasks.parser.Parser;
import net.sf.antcontrib.cpptasks.TargetDef;

/**
 * An abstract compiler implementation.
 * 
 * @author Adam Murdoch
 * @author Curt Arnold
 */
public abstract class AbstractCompiler extends AbstractProcessor
        implements
            Compiler {
    private static final String[] emptyIncludeArray = new String[0];
    private String outputSuffix;
    protected AbstractCompiler(String[] sourceExtensions,
            String[] headerExtensions, String outputSuffix) {
        super(sourceExtensions, headerExtensions);
        this.outputSuffix = outputSuffix;
    }
    /**
     * Checks file name to see if parse should be attempted
     * 
     * Default implementation returns false for files with extensions '.dll',
     * 'tlb', '.res'
     *  
     */
    protected boolean canParse(File sourceFile) {
        String sourceName = sourceFile.toString();
        int lastPeriod = sourceName.lastIndexOf('.');
        if (lastPeriod >= 0 && lastPeriod == sourceName.length() - 4) {
            String ext = sourceName.substring(lastPeriod).toUpperCase();
            if (ext.equals(".DLL") || ext.equals(".TLB") || ext.equals(".RES")) {
                return false;
            }
        }
        return true;
    }
    abstract protected CompilerConfiguration createConfiguration(CCTask task,
            LinkType linkType, ProcessorDef[] baseConfigs,
            CompilerDef specificConfig, TargetDef targetPlatform);
    public ProcessorConfiguration createConfiguration(CCTask task,
            LinkType linkType, ProcessorDef[] baseConfigs,
            ProcessorDef specificConfig, TargetDef targetPlatform) {
        if (specificConfig == null) {
            throw new NullPointerException("specificConfig");
        }
        return createConfiguration(task, linkType, baseConfigs,
                (CompilerDef) specificConfig, targetPlatform);
    }
    abstract protected Parser createParser(File sourceFile);
    protected String getBaseOutputName(String inputFile) {
        int lastSlash = inputFile.lastIndexOf('/');
        int lastReverse = inputFile.lastIndexOf('\\');
        int lastSep = inputFile.lastIndexOf(File.separatorChar);
        if (lastReverse > lastSlash) {
            lastSlash = lastReverse;
        }
        if (lastSep > lastSlash) {
            lastSlash = lastSep;
        }
        int lastPeriod = inputFile.lastIndexOf('.');
        if (lastPeriod < 0) {
            lastPeriod = inputFile.length();
        }
        return inputFile.substring(lastSlash + 1, lastPeriod);
    }
    public String getOutputFileName(String inputFile) {
        //
        //  if a recognized input file
        //
        if (bid(inputFile) > 1) {
            String baseName = getBaseOutputName(inputFile);
            return baseName + outputSuffix;
        }
        return null;
    }
    /**
     * Returns dependency info for the specified source file
     * 
     * @param task
     *            task for any diagnostic output
     * @param source
     *            file to be parsed
     * @param includePath
     *            include path to be used to resolve included files
     * 
     * @param sysIncludePath
     *            sysinclude path from build file, files resolved using
     *            sysInclude path will not participate in dependency analysis
     * 
     * @param envIncludePath
     *            include path from environment variable, files resolved with
     *            envIncludePath will not participate in dependency analysis
     * 
     * @param baseDir
     *            used to produce relative paths in DependencyInfo
     * @param includePathIdentifier
     *            used to distinguish DependencyInfo's from different include
     *            path settings
     * 
     * @author Curt Arnold
     */
    public final DependencyInfo parseIncludes(CCTask task, File source,
            File[] includePath, File[] sysIncludePath, File[] envIncludePath,
            File baseDir, String includePathIdentifier) {
        //
        //  if any of the include files can not be identified
        //      change the sourceLastModified to Long.MAX_VALUE to
        //      force recompilation of anything that depends on it
        long sourceLastModified = source.lastModified();
        File[] sourcePath = new File[1];
        sourcePath[0] = new File(source.getParent());
        Vector onIncludePath = new Vector();
        Vector onSysIncludePath = new Vector();
        String baseDirPath;
        try {
            baseDirPath = baseDir.getCanonicalPath();
        } catch (IOException ex) {
            baseDirPath = baseDir.toString();
        }
        String relativeSource = CUtil.getRelativePath(baseDirPath, source);
        String[] includes = emptyIncludeArray;
        if (canParse(source)) {
            Parser parser = createParser(source);
            try {
                Reader reader = new BufferedReader(new FileReader(source));
                parser.parse(reader);
                includes = parser.getIncludes();
            } catch (IOException ex) {
                task.log("Error parsing " + source.toString() + ":"
                        + ex.toString());
                includes = new String[0];
            }
        }
        for (int i = 0; i < includes.length; i++) {
            String includeName = includes[i];
            if (!resolveInclude(includeName, sourcePath, onIncludePath)) {
                if (!resolveInclude(includeName, includePath, onIncludePath)) {
                    if (!resolveInclude(includeName, sysIncludePath,
                            onSysIncludePath)) {
                        if (!resolveInclude(includeName, envIncludePath,
                                onSysIncludePath)) {
                            //
                            //  this should be enough to require us to reparse
                            //     the file with the missing include for dependency
                            //     information without forcing a rebuild
                            sourceLastModified++;
                        }
                    }
                }
            }
        }
        for (int i = 0; i < onIncludePath.size(); i++) {
            String relativeInclude = CUtil.getRelativePath(baseDirPath,
                    (File) onIncludePath.elementAt(i));
            onIncludePath.setElementAt(relativeInclude, i);
        }
        for (int i = 0; i < onSysIncludePath.size(); i++) {
            String relativeInclude = CUtil.getRelativePath(baseDirPath,
                    (File) onSysIncludePath.elementAt(i));
            onSysIncludePath.setElementAt(relativeInclude, i);
        }
        return new DependencyInfo(includePathIdentifier, relativeSource,
                sourceLastModified, onIncludePath, onSysIncludePath);
    }
    protected boolean resolveInclude(String includeName, File[] includePath,
            Vector onThisPath) {
        for (int i = 0; i < includePath.length; i++) {
            File includeFile = new File(includePath[i], includeName);
            if (includeFile.exists()) {
                onThisPath.addElement(includeFile);
                return true;
            }
        }
        return false;
    }
}
