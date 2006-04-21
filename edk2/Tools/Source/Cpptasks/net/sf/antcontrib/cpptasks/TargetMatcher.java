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
import java.io.File;
import java.util.Hashtable;
import java.util.Vector;

import net.sf.antcontrib.cpptasks.compiler.LinkerConfiguration;
import net.sf.antcontrib.cpptasks.compiler.ProcessorConfiguration;

import org.apache.tools.ant.BuildException;
/**
 * This class matches each visited file with an appropriate compiler
 * 
 * @author Curt Arnold
 */
public final class TargetMatcher implements FileVisitor {
    private LinkerConfiguration linker;
    private Vector objectFiles;
    private File outputDir;
    private ProcessorConfiguration[] processors;
    private final File sourceFiles[] = new File[1];
    private Hashtable targets;
    private CCTask task;
    public TargetMatcher(CCTask task, File outputDir,
            ProcessorConfiguration[] processors, LinkerConfiguration linker,
            Vector objectFiles, Hashtable targets) {
        this.task = task;
        this.outputDir = outputDir;
        this.processors = processors;
        this.targets = targets;
        this.linker = linker;
        this.objectFiles = objectFiles;
    }
    public void visit(File parentDir, String filename) throws BuildException {
        //
        //   see if any processor wants to bid
        //       on this one
        ProcessorConfiguration selectedCompiler = null;
        int bid = 0;
        if (processors != null) {
            for (int k = 0; k < processors.length; k++) {
                int newBid = processors[k].bid(filename);
                if (newBid > bid) {
                    bid = newBid;
                    selectedCompiler = processors[k];
                }
            }
        }
        //
        //   no processor interested in file
        //      log diagnostic message
        if (bid <= 0) {
            if (linker != null) {
                int linkerbid = linker.bid(filename);
                if (linkerbid > 0) {
                    File objFile = new File(parentDir, filename);
                    objectFiles.addElement(objFile);
                    if (linkerbid == 1) {
                        task.log("Unrecognized file type " + objFile.toString()
                                + " will be passed to linker");
                    }
                }
            }
        } else {
            //
            //  get output file name
            //
            String outputFileName = selectedCompiler
                    .getOutputFileName(filename);
            //
            //   if there is some output for this task
            //      (that is a source file and not an header file)
            //
            if (outputFileName != null) {
                sourceFiles[0] = new File(parentDir, filename);
                //
                //   see if the same output file has already been registered
                //
                TargetInfo previousTarget = (TargetInfo) targets
                        .get(outputFileName);
                if (previousTarget == null) {
                    targets.put(outputFileName, new TargetInfo(
                            selectedCompiler, sourceFiles, null, new File(
                                    outputDir, outputFileName),
                            selectedCompiler.getRebuild()));
                } else {
                    if (!previousTarget.getSources()[0].equals(sourceFiles[0])) {
                        StringBuffer builder = new StringBuffer(
                                "Output filename conflict: ");
                        builder.append(outputFileName);
                        builder.append(" would be produced from ");
                        builder.append(previousTarget.getSources()[0]
                                .toString());
                        builder.append(" and ");
                        builder.append(filename);
                        throw new BuildException(builder.toString());
                    }
                }
            }
        }
    }
}
