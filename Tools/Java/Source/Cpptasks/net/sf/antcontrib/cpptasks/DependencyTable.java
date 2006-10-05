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
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.io.UnsupportedEncodingException;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Vector;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.parsers.SAXParser;
import javax.xml.parsers.SAXParserFactory;
import net.sf.antcontrib.cpptasks.compiler.CompilerConfiguration;
import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Project;
import org.xml.sax.Attributes;
import org.xml.sax.SAXException;
import org.xml.sax.helpers.DefaultHandler;
/**
 * @author Curt Arnold
 */
public final class DependencyTable {
    /**
     * This class handles populates the TargetHistory hashtable in response to
     * SAX parse events
     */
    private class DependencyTableHandler extends DefaultHandler {
        private File baseDir;
        private final DependencyTable dependencyTable;
        private String includePath;
        private Vector includes;
        private String source;
        private long sourceLastModified;
        private Vector sysIncludes;
        /**
         * Constructor
         * 
         * @param history
         *            hashtable of TargetHistory keyed by output name
         * @param outputFiles
         *            existing files in output directory
         */
        private DependencyTableHandler(DependencyTable dependencyTable,
                File baseDir) {
            this.dependencyTable = dependencyTable;
            this.baseDir = baseDir;
            includes = new Vector();
            sysIncludes = new Vector();
            source = null;
        }
        public void endElement(String namespaceURI, String localName,
                String qName) throws SAXException {
            //
            //   if </source> then
            //       create Dependency object and add to hashtable
            //           if corresponding source file exists and
            //           has the same timestamp
            //
            if (qName.equals("source")) {
                if (source != null && includePath != null) {
                    File existingFile = new File(baseDir, source);
                    //
                    //   if the file exists and the time stamp is right
                    //       preserve the dependency info
                    if (existingFile.exists()) {
                        //
                        //   would have expected exact matches
                        //       but was seeing some unexpected difference by
                        //       a few tens of milliseconds, as long
                        //       as the times are within a second
                        long existingLastModified = existingFile.lastModified();
                        long diff = existingLastModified - sourceLastModified;
                        if (diff >= -500 && diff <= 500) {
                            DependencyInfo dependInfo = new DependencyInfo(
                                    includePath, source, sourceLastModified,
                                    includes, sysIncludes);
                            dependencyTable.putDependencyInfo(source,
                                    dependInfo);
                        }
                    }
                    source = null;
                    includes.setSize(0);
                }
            } else {
                //
                //    this causes any <source> elements outside the
                //       scope of an <includePath> to be discarded
                //
                if (qName.equals("includePath")) {
                    includePath = null;
                }
            }
        }
        /**
         * startElement handler
         */
        public void startElement(String namespaceURI, String localName,
                String qName, Attributes atts) throws SAXException {
            //
            //   if includes, then add relative file name to vector
            //
            if (qName.equals("include")) {
                includes.addElement(atts.getValue("file"));
            } else {
                if (qName.equals("sysinclude")) {
                    sysIncludes.addElement(atts.getValue("file"));
                } else {
                    //
                    //    if source then
                    //        capture source file name,
                    //        modification time and reset includes vector
                    //
                    if (qName.equals("source")) {
                        source = atts.getValue("file");
                        sourceLastModified = Long.parseLong(atts
                                .getValue("lastModified"), 16);
                        includes.setSize(0);
                        sysIncludes.setSize(0);
                    } else {
                        if (qName.equals("includePath")) {
                            includePath = atts.getValue("signature");
                        }
                    }
                }
            }
        }
    }
    public abstract class DependencyVisitor {
        /**
         * Previews all the children of this source file.
         * 
         * May be called multiple times as DependencyInfo's for children are
         * filled in.
         * 
         * @return true to continue towards recursion into included files
         */
        public abstract boolean preview(DependencyInfo parent,
                DependencyInfo[] children);
        /**
         * Called if the dependency depth exhausted the stack.
         */
        public abstract void stackExhausted();
        /**
         * Visits the dependency info.
         * 
         * @returns true to continue towards recursion into included files
         */
        public abstract boolean visit(DependencyInfo dependInfo);
    }
    public class TimestampChecker extends DependencyVisitor {
        private boolean noNeedToRebuild;
        private long outputLastModified;
        private boolean rebuildOnStackExhaustion;
        public TimestampChecker(final long outputLastModified,
                boolean rebuildOnStackExhaustion) {
            this.outputLastModified = outputLastModified;
            noNeedToRebuild = true;
            this.rebuildOnStackExhaustion = rebuildOnStackExhaustion;
        }
        public boolean getMustRebuild() {
            return !noNeedToRebuild;
        }
        public boolean preview(DependencyInfo parent, DependencyInfo[] children) {
            int withCompositeTimes = 0;
            long parentCompositeLastModified = parent.getSourceLastModified();
            for (int i = 0; i < children.length; i++) {
                if (children[i] != null) {
                    //
                    //  expedient way to determine if a child forces us to
                    // rebuild
                    //
                    visit(children[i]);
                    long childCompositeLastModified = children[i]
                            .getCompositeLastModified();
                    if (childCompositeLastModified != Long.MIN_VALUE) {
                        withCompositeTimes++;
                        if (childCompositeLastModified > parentCompositeLastModified) {
                            parentCompositeLastModified = childCompositeLastModified;
                        }
                    }
                }
            }
            if (withCompositeTimes == children.length) {
                parent.setCompositeLastModified(parentCompositeLastModified);
            }
            //
            //  may have been changed by an earlier call to visit()
            //
            return noNeedToRebuild;
        }
        public void stackExhausted() {
            if (rebuildOnStackExhaustion) {
                noNeedToRebuild = false;
            }
        }
        public boolean visit(DependencyInfo dependInfo) {
            if (noNeedToRebuild) {
                if (dependInfo.getSourceLastModified() > outputLastModified
                        || dependInfo.getCompositeLastModified() > outputLastModified) {
                    noNeedToRebuild = false;
                }
            }
            //
            //   only need to process the children if
            //      it has not yet been determined whether
            //      we need to rebuild and the composite modified time
            //         has not been determined for this file
            return noNeedToRebuild
                    && dependInfo.getCompositeLastModified() == Long.MIN_VALUE;
        }
    }
    private/* final */File baseDir;
    private String baseDirPath;
    /**
     * a hashtable of DependencyInfo[] keyed by output file name
     */
    private final Hashtable dependencies = new Hashtable();
    /** The file the cache was loaded from. */
    private/* final */File dependenciesFile;
    /** Flag indicating whether the cache should be written back to file. */
    private boolean dirty;
    /**
     * Creates a target history table from dependencies.xml in the prject
     * directory, if it exists. Otherwise, initializes the dependencies empty.
     * 
     * @param task
     *            task used for logging history load errors
     * @param baseDir
     *            output directory for task
     */
    public DependencyTable(File baseDir) {
        if (baseDir == null) {
            throw new NullPointerException("baseDir");
        }
        this.baseDir = baseDir;
        try {
            baseDirPath = baseDir.getCanonicalPath();
        } catch (IOException ex) {
            baseDirPath = baseDir.toString();
        }
        dirty = false;
        //
        //   load any existing dependencies from file
        dependenciesFile = new File(baseDir, "dependencies.xml");
    }
    public void commit(CCTask task) {
        //
        //   if not dirty, no need to update file
        //
        if (dirty) {
            //
            //   walk through dependencies to get vector of include paths
            // identifiers
            //
            Vector includePaths = getIncludePaths();
            //
            //
            //   write dependency file
            //
            try {
                FileOutputStream outStream = new FileOutputStream(
                        dependenciesFile);
                OutputStreamWriter streamWriter;
                //
                //    Early VM's may not have UTF-8 support
                //       fallback to default code page which
                //           "should" be okay unless there are
                //            non ASCII file names
                String encodingName = "UTF-8";
                try {
                    streamWriter = new OutputStreamWriter(outStream, "UTF-8");
                } catch (UnsupportedEncodingException ex) {
                    streamWriter = new OutputStreamWriter(outStream);
                    encodingName = streamWriter.getEncoding();
                }
                BufferedWriter writer = new BufferedWriter(streamWriter);
                writer.write("<?xml version='1.0' encoding='");
                writer.write(encodingName);
                writer.write("'?>\n");
                writer.write("<dependencies>\n");
                StringBuffer buf = new StringBuffer();
                Enumeration includePathEnum = includePaths.elements();
                while (includePathEnum.hasMoreElements()) {
                    writeIncludePathDependencies((String) includePathEnum
                            .nextElement(), writer, buf);
                }
                writer.write("</dependencies>\n");
                writer.close();
                dirty = false;
            } catch (IOException ex) {
                task.log("Error writing " + dependenciesFile.toString() + ":"
                        + ex.toString());
            }
        }
    }
    /**
     * Returns an enumerator of DependencyInfo's
     */
    public Enumeration elements() {
        return dependencies.elements();
    }
    /**
     * This method returns a DependencyInfo for the specific source file and
     * include path identifier
     *  
     */
    public DependencyInfo getDependencyInfo(String sourceRelativeName,
            String includePathIdentifier) {
        DependencyInfo dependInfo = null;
        DependencyInfo[] dependInfos = (DependencyInfo[]) dependencies
                .get(sourceRelativeName);
        if (dependInfos != null) {
            for (int i = 0; i < dependInfos.length; i++) {
                dependInfo = dependInfos[i];
                if (dependInfo.getIncludePathIdentifier().equals(
                        includePathIdentifier)) {
                    return dependInfo;
                }
            }
        }
        return null;
    }
    private Vector getIncludePaths() {
        Vector includePaths = new Vector();
        DependencyInfo[] dependInfos;
        Enumeration dependenciesEnum = dependencies.elements();
        while (dependenciesEnum.hasMoreElements()) {
            dependInfos = (DependencyInfo[]) dependenciesEnum.nextElement();
            for (int i = 0; i < dependInfos.length; i++) {
                DependencyInfo dependInfo = dependInfos[i];
                boolean matchesExisting = false;
                final String dependIncludePath = dependInfo
                        .getIncludePathIdentifier();
                Enumeration includePathEnum = includePaths.elements();
                while (includePathEnum.hasMoreElements()) {
                    if (dependIncludePath.equals(includePathEnum.nextElement())) {
                        matchesExisting = true;
                        break;
                    }
                }
                if (!matchesExisting) {
                    includePaths.addElement(dependIncludePath);
                }
            }
        }
        return includePaths;
    }
    public void load() throws IOException, ParserConfigurationException,
            SAXException {
        dependencies.clear();
        if (dependenciesFile.exists()) {
            SAXParserFactory factory = SAXParserFactory.newInstance();
            factory.setValidating(false);
            SAXParser parser = factory.newSAXParser();
            parser.parse(dependenciesFile, new DependencyTableHandler(this,
                    baseDir));
            dirty = false;
        }
    }
    /**
     * Determines if the specified target needs to be rebuilt.
     * 
     * This task may result in substantial IO as files are parsed to determine
     * their dependencies
     */
    public boolean needsRebuild(CCTask task, TargetInfo target,
            int dependencyDepth) {
        //    look at any files where the compositeLastModified
        //    is not known, but the includes are known
        //
        boolean mustRebuild = false;
        CompilerConfiguration compiler = (CompilerConfiguration) target
                .getConfiguration();
        String includePathIdentifier = compiler.getIncludePathIdentifier();
        File[] sources = target.getSources();
        DependencyInfo[] dependInfos = new DependencyInfo[sources.length];
        long outputLastModified = target.getOutput().lastModified();
        //
        //   try to solve problem using existing dependency info
        //      (not parsing any new files)
        //
        DependencyInfo[] stack = new DependencyInfo[50];
        boolean rebuildOnStackExhaustion = true;
        if (dependencyDepth >= 0) {
            if (dependencyDepth < 50) {
                stack = new DependencyInfo[dependencyDepth];
            }
            rebuildOnStackExhaustion = false;
        }
        TimestampChecker checker = new TimestampChecker(outputLastModified,
                rebuildOnStackExhaustion);
        for (int i = 0; i < sources.length && !mustRebuild; i++) {
            File source = sources[i];
            String relative = CUtil.getRelativePath(baseDirPath, source);
            DependencyInfo dependInfo = getDependencyInfo(relative,
                    includePathIdentifier);
            if (dependInfo == null) {
                task.log("Parsing " + relative, Project.MSG_VERBOSE);
                dependInfo = parseIncludes(task, compiler, source);
            }
            walkDependencies(task, dependInfo, compiler, stack, checker);
            mustRebuild = checker.getMustRebuild();
        }
        return mustRebuild;
    }
    public DependencyInfo parseIncludes(CCTask task,
            CompilerConfiguration compiler, File source) {
        DependencyInfo dependInfo = compiler.parseIncludes(task, baseDir,
                source);
        String relativeSource = CUtil.getRelativePath(baseDirPath, source);
        putDependencyInfo(relativeSource, dependInfo);
        return dependInfo;
    }
    private void putDependencyInfo(String key, DependencyInfo dependInfo) {
        //
        //   optimistic, add new value
        //
        DependencyInfo[] old = (DependencyInfo[]) dependencies.put(key,
                new DependencyInfo[]{dependInfo});
        dirty = true;
        //
        //   something was already there
        //
        if (old != null) {
            //
            //   see if the include path matches a previous entry
            //       if so replace it
            String includePathIdentifier = dependInfo
                    .getIncludePathIdentifier();
            for (int i = 0; i < old.length; i++) {
                DependencyInfo oldDepend = old[i];
                if (oldDepend.getIncludePathIdentifier().equals(
                        includePathIdentifier)) {
                    old[i] = dependInfo;
                    dependencies.put(key, old);
                    return;
                }
            }
            //
            //   no match prepend the new entry to the array
            //      of dependencies for the file
            DependencyInfo[] combined = new DependencyInfo[old.length + 1];
            combined[0] = dependInfo;
            for (int i = 0; i < old.length; i++) {
                combined[i + 1] = old[i];
            }
            dependencies.put(key, combined);
        }
        return;
    }
    public void walkDependencies(CCTask task, DependencyInfo dependInfo,
            CompilerConfiguration compiler, DependencyInfo[] stack,
            DependencyVisitor visitor) throws BuildException {
        //
        //   visit this node
        //       if visit returns true then
        //          visit the referenced include and sysInclude dependencies
        //
        if (visitor.visit(dependInfo)) {
            //
            //   find first null entry on stack
            //
            int stackPosition = -1;
            for (int i = 0; i < stack.length; i++) {
                if (stack[i] == null) {
                    stackPosition = i;
                    stack[i] = dependInfo;
                    break;
                } else {
                    //
                    //   if we have appeared early in the calling history
                    //      then we didn't exceed the criteria
                    if (stack[i] == dependInfo) {
                        return;
                    }
                }
            }
            if (stackPosition == -1) {
                visitor.stackExhausted();
                return;
            }
            //
            //   locate dependency infos
            //
            String[] includes = dependInfo.getIncludes();
            String includePathIdentifier = compiler.getIncludePathIdentifier();
            DependencyInfo[] includeInfos = new DependencyInfo[includes.length];
            for (int i = 0; i < includes.length; i++) {
                DependencyInfo includeInfo = getDependencyInfo(includes[i],
                        includePathIdentifier);
                includeInfos[i] = includeInfo;
            }
            //
            //   preview with only the already available dependency infos
            //
            if (visitor.preview(dependInfo, includeInfos)) {
                //
                //   now need to fill in the missing DependencyInfos
                //
                int missingCount = 0;
                for (int i = 0; i < includes.length; i++) {
                    if (includeInfos[i] == null) {
                        missingCount++;
                        task.log("Parsing " + includes[i], Project.MSG_VERBOSE);
                        // If the include is part of a UNC don't go building a
                        // relative file name.
                        File src = includes[i].startsWith("\\\\") ? new File(
                                includes[i]) : new File(baseDir, includes[i]);
                        DependencyInfo includeInfo = parseIncludes(task,
                                compiler, src);
                        includeInfos[i] = includeInfo;
                    }
                }
                //
                //   if it passes a review the second time
                //      then recurse into all the children
                if (missingCount == 0
                        || visitor.preview(dependInfo, includeInfos)) {
                    //
                    //   recurse into
                    //
                    for (int i = 0; i < includeInfos.length; i++) {
                        DependencyInfo includeInfo = includeInfos[i];
                        walkDependencies(task, includeInfo, compiler, stack,
                                visitor);
                    }
                }
            }
            stack[stackPosition] = null;
        }
    }
    private void writeDependencyInfo(BufferedWriter writer, StringBuffer buf,
            DependencyInfo dependInfo) throws IOException {
        String[] includes = dependInfo.getIncludes();
        String[] sysIncludes = dependInfo.getSysIncludes();
        //
        //   if the includes have not been evaluted then
        //       it is not worth our time saving it
        //       and trying to distiguish between files with
        //       no dependencies and those with undetermined dependencies
        buf.setLength(0);
        buf.append("      <source file=\"");
        buf.append(CUtil.xmlAttribEncode(dependInfo.getSource()));
        buf.append("\" lastModified=\"");
        buf.append(Long.toHexString(dependInfo.getSourceLastModified()));
        buf.append("\">\n");
        writer.write(buf.toString());
        for (int i = 0; i < includes.length; i++) {
            buf.setLength(0);
            buf.append("         <include file=\"");
            buf.append(CUtil.xmlAttribEncode(includes[i]));
            buf.append("\"/>\n");
            writer.write(buf.toString());
        }
        for (int i = 0; i < sysIncludes.length; i++) {
            buf.setLength(0);
            buf.append("         <sysinclude file=\"");
            buf.append(CUtil.xmlAttribEncode(sysIncludes[i]));
            buf.append("\"/>\n");
            writer.write(buf.toString());
        }
        writer.write("      </source>\n");
        return;
    }
    private void writeIncludePathDependencies(String includePathIdentifier,
            BufferedWriter writer, StringBuffer buf) throws IOException {
        //
        //  include path element
        //
        buf.setLength(0);
        buf.append("   <includePath signature=\"");
        buf.append(CUtil.xmlAttribEncode(includePathIdentifier));
        buf.append("\">\n");
        writer.write(buf.toString());
        Enumeration dependenciesEnum = dependencies.elements();
        while (dependenciesEnum.hasMoreElements()) {
            DependencyInfo[] dependInfos = (DependencyInfo[]) dependenciesEnum
                    .nextElement();
            for (int i = 0; i < dependInfos.length; i++) {
                DependencyInfo dependInfo = dependInfos[i];
                //
                //   if this is for the same include path
                //      then output the info
                if (dependInfo.getIncludePathIdentifier().equals(
                        includePathIdentifier)) {
                    writeDependencyInfo(writer, buf, dependInfo);
                }
            }
        }
        writer.write("   </includePath>\n");
    }
}
