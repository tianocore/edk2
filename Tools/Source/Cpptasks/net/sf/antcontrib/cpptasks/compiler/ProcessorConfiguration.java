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
import net.sf.antcontrib.cpptasks.ProcessorParam;
/**
 * A configuration for a C++ compiler, linker or other processor
 * 
 * @author Curt Arnold
 */
public interface ProcessorConfiguration {
    /**
     * An indication of how much this compiler would like to process this file
     * 
     * @return 0 is no interest to process, 100 is strong interest to process
     */
    int bid(String filename);
    /**
     * Returns a string representation of this configuration. Should be
     * canonical so that equivalent configurations will have equivalent string
     * representations
     */
    String getIdentifier();
    /**
     * Output file name (no path components) corresponding to source file
     * 
     * @param inputFile
     *            input file
     * @return output file name or null if no output file or name not
     *         determined by input file
     */
    String getOutputFileName(String inputFile);
    ProcessorParam[] getParams();
    /**
     * If true, all files using this configuration should be rebuilt and any
     * existing output files should be ignored
     */
    boolean getRebuild();
}
