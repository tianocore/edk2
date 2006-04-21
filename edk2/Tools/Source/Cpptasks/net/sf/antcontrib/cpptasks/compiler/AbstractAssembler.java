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
package net.sf.antcontrib.cpptasks.compiler;

import java.io.File;

import net.sf.antcontrib.cpptasks.AssemblerDef;
import net.sf.antcontrib.cpptasks.CCTask;
import net.sf.antcontrib.cpptasks.ProcessorDef;
import net.sf.antcontrib.cpptasks.TargetDef;
/**
 * An abstract assembler implementation.
 * 
 */
public abstract class AbstractAssembler extends AbstractProcessor
implements Assembler {
    private String outputSuffix;
    protected AbstractAssembler(String[] sourceExtensions,
            String[] headerExtensions, String outputSuffix) {
        super(sourceExtensions, headerExtensions);
        this.outputSuffix = outputSuffix;
    }    
    abstract protected AssemblerConfiguration createConfiguration(CCTask task,
            LinkType linkType, ProcessorDef[] baseConfigs,
            AssemblerDef specificConfig, TargetDef targetPlatform);
    public ProcessorConfiguration createConfiguration(CCTask task,
            LinkType linkType, ProcessorDef[] baseConfigs,
            ProcessorDef specificConfig, TargetDef targetPlatform) {
        if (specificConfig == null) {
            throw new NullPointerException("specificConfig");
        }
        return createConfiguration(task, linkType, baseConfigs,
                (AssemblerDef) specificConfig, targetPlatform);
    }
    public String getOutputFileName(String inputFile) {
        if (bid(inputFile) > 1) {
            String baseName = getBaseOutputName(inputFile);
            return baseName + outputSuffix;
        }
        return null;
    }
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
}