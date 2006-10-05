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
package net.sf.antcontrib.cpptasks.compiler;
import java.io.File;

import net.sf.antcontrib.cpptasks.CCTask;
import net.sf.antcontrib.cpptasks.LinkerDef;
import net.sf.antcontrib.cpptasks.ProcessorDef;
import net.sf.antcontrib.cpptasks.TargetDef;


import org.apache.tools.ant.types.Environment;
/**
 * An abstract Linker implementation.
 * 
 * @author Adam Murdoch
 */
public abstract class AbstractLinker extends AbstractProcessor
        implements
            Linker {
    public AbstractLinker(String[] objExtensions, String[] ignoredExtensions) {
        super(objExtensions, ignoredExtensions);
    }
    /**
     * Returns the bid of the processor for the file.
     * 
     * A linker will bid 1 on any unrecognized file type.
     * 
     * @param inputFile
     *            filename of input file
     * @return bid for the file, 0 indicates no interest, 1 indicates that the
     *         processor recognizes the file but doesn't process it (header
     *         files, for example), 100 indicates strong interest
     */
    public int bid(String inputFile) {
        int bid = super.bid(inputFile);
        switch (bid) {
            //
            //  unrecognized extension, take the file
            //
            case 0 :
                return 1;
            //
            //   discard the ignored extensions
            //
            case 1 :
                return 0;
        }
        return bid;
    }
    public Processor changeEnvironment(boolean newEnvironment, Environment env) {
        return this;
    }
    abstract protected LinkerConfiguration createConfiguration(CCTask task,
            LinkType linkType, ProcessorDef[] baseConfigs,
            LinkerDef specificConfig, TargetDef targetPlatform);
    public ProcessorConfiguration createConfiguration(CCTask task,
            LinkType linkType, ProcessorDef[] baseConfigs,
            ProcessorDef specificConfig,
			TargetDef targetPlatform) {
        if (specificConfig == null) {
            throw new NullPointerException("specificConfig");
        }
        return createConfiguration(task, linkType, baseConfigs,
                (LinkerDef) specificConfig, targetPlatform);
    }
    public String getLibraryKey(File libfile) {
        return libfile.getName();
    }
    public abstract String getOutputFileName(String fileName);
}
