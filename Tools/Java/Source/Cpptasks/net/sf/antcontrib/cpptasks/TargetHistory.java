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
/**
 * A description of a file built or to be built
 */
public final class TargetHistory {
    private/* final */String config;
    private/* final */String output;
    private/* final */long outputLastModified;
    private/* final */SourceHistory[] sources;
    /**
     * Constructor from build step
     */
    public TargetHistory(String config, String output, long outputLastModified,
            SourceHistory[] sources) {
        if (config == null) {
            throw new NullPointerException("config");
        }
        if (sources == null) {
            throw new NullPointerException("source");
        }
        if (output == null) {
            throw new NullPointerException("output");
        }
        this.config = config;
        this.output = output;
        this.outputLastModified = outputLastModified;
        this.sources = (SourceHistory[]) sources.clone();
    }
    public String getOutput() {
        return output;
    }
    public long getOutputLastModified() {
        return outputLastModified;
    }
    public String getProcessorConfiguration() {
        return config;
    }
    public SourceHistory[] getSources() {
        SourceHistory[] clone = (SourceHistory[]) sources.clone();
        return clone;
    }
}
