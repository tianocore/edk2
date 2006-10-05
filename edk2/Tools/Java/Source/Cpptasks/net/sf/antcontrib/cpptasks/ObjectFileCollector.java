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
package net.sf.antcontrib.cpptasks;
import java.io.File;
import java.util.Vector;

import net.sf.antcontrib.cpptasks.compiler.Linker;

import org.apache.tools.ant.BuildException;
/**
 * Collects object files for the link step.
 * 
 *  
 */
public final class ObjectFileCollector implements FileVisitor {
    private final Vector files;
    private final Linker linker;
    public ObjectFileCollector(Linker linker, Vector files) {
        this.linker = linker;
        this.files = files;
    }
    public void visit(File parentDir, String filename) throws BuildException {
        int bid = linker.bid(filename);
        if (bid >= 1) {
            files.addElement(new File(parentDir, filename));
        }
    }
}
