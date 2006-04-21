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

import net.sf.antcontrib.cpptasks.compiler.ProcessorConfiguration;
/**
 * A description of a file built or to be built
 */
public final class TargetInfo {
    private static final File[] emptyFileArray = new File[0];
    private/* final */ProcessorConfiguration config;
    private/* final */File output;
    private boolean rebuild;
    private/* final */File[] sources;
    private File[] sysSources;
    public TargetInfo(ProcessorConfiguration config, File[] sources,
            File[] sysSources, File output, boolean rebuild) {
        if (config == null) {
            throw new NullPointerException("config");
        }
        if (sources == null) {
            throw new NullPointerException("sources");
        }
        if (output == null) {
            throw new NullPointerException("output");
        }
        this.config = config;
        this.sources = (File[]) sources.clone();
        if (sysSources == null) {
            this.sysSources = emptyFileArray;
        } else {
            this.sysSources = (File[]) sysSources.clone();
        }
        this.output = output;
        this.rebuild = rebuild;
        //
        //   if the output doesn't exist, must rebuild it
        //
        if (!output.exists()) {
            rebuild = true;
        }
    }
    public String[] getAllSourcePaths() {
        String[] paths = new String[sysSources.length + sources.length];
        for (int i = 0; i < sysSources.length; i++) {
            paths[i] = sysSources[i].toString();
        }
        int offset = sysSources.length;
        for (int i = 0; i < sources.length; i++) {
            paths[offset + i] = sources[i].toString();
        }
        return paths;
    }
    public File[] getAllSources() {
        File[] allSources = new File[sources.length + sysSources.length];
        for (int i = 0; i < sysSources.length; i++) {
            allSources[i] = sysSources[i];
        }
        int offset = sysSources.length;
        for (int i = 0; i < sources.length; i++) {
            allSources[i + offset] = sources[i];
        }
        return allSources;
    }
    public ProcessorConfiguration getConfiguration() {
        return config;
    }
    public File getOutput() {
        return output;
    }
    public boolean getRebuild() {
        return rebuild;
    }
    /**
     * Returns an array of SourceHistory objects (contains relative path and
     * last modified time) for the source[s] of this target
     */
    public SourceHistory[] getSourceHistories(String basePath) {
        SourceHistory[] histories = new SourceHistory[sources.length];
        for (int i = 0; i < sources.length; i++) {
            String relativeName = CUtil.getRelativePath(basePath, sources[i]);
            long lastModified = sources[i].lastModified();
            histories[i] = new SourceHistory(relativeName, lastModified);
        }
        return histories;
    }
    public String[] getSourcePaths() {
        String[] paths = new String[sources.length];
        for (int i = 0; i < sources.length; i++) {
            paths[i] = sources[i].toString();
        }
        return paths;
    }
    public File[] getSources() {
        File[] clone = (File[]) sources.clone();
        return clone;
    }
    public String[] getSysSourcePaths() {
        String[] paths = new String[sysSources.length];
        for (int i = 0; i < sysSources.length; i++) {
            paths[i] = sysSources[i].toString();
        }
        return paths;
    }
    public File[] getSysSources() {
        File[] clone = (File[]) sysSources.clone();
        return clone;
    }
    public void mustRebuild() {
        this.rebuild = true;
    }
}
