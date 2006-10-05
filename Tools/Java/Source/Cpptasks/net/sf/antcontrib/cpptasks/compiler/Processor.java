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
import net.sf.antcontrib.cpptasks.CCTask;
import net.sf.antcontrib.cpptasks.ProcessorDef;
import net.sf.antcontrib.cpptasks.TargetDef;
import org.apache.tools.ant.types.Environment;
/**
 * A processor. Base interface for Compiler and Linker
 * 
 * @author Curt Arnold
 */
public interface Processor {
    /**
     * Returns a bid indicating the desire of this compiler to process the
     * file.
     * 
     * @param inputFile
     *            input file
     * @return 0 = no interest, 100 = high interest
     */
    int bid(String inputFile);
    Processor changeEnvironment(boolean newEnvironment, Environment env);
    /**
     * Returns the compiler configuration for <cc>or <compiler>element.
     * 
     * @param defaultProviders
     *            When specificConfig corresponds to a <compiler>or linker
     *            element, defaultProvider will be a zero to two element array.
     *            If there is an extends attribute, the first element will be
     *            the referenced ProcessorDef, unless inherit = false, the last
     *            element will be the containing <cc>element
     * @param specificConfig
     *            A <cc>or <compiler>element.
     * @return resulting configuration
     */
    ProcessorConfiguration createConfiguration(CCTask task, LinkType linkType,
            ProcessorDef[] defaultProviders, ProcessorDef specificConfig,
			TargetDef targetPlatform);
    /**
     * Retrieve an identifier that identifies the specific version of the
     * compiler. Compilers with the same identifier should produce the same
     * output files for the same input files and command line switches.
     */
    String getIdentifier();
    /**
     * Gets the linker that is associated with this processors
     */
    Linker getLinker(LinkType type);
    /**
     * Output file name (no path components) corresponding to source file
     * 
     * @param inputFile
     *            input file
     * @return output file name or null if no output file or name not
     *         determined by input file
     */
    String getOutputFileName(String inputFile);
}
