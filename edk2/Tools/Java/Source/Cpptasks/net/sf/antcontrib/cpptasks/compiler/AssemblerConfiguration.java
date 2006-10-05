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

import net.sf.antcontrib.cpptasks.CCTask;
import org.apache.tools.ant.BuildException;
/**
 * A configuration for an assembler
 *
 */
public interface AssemblerConfiguration extends ProcessorConfiguration {
    void assembler(CCTask task, File outputDir, String[] sourceFiles) throws BuildException;
}
