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
package net.sf.antcontrib.cpptasks.gcc.cross;
import java.io.File;

import net.sf.antcontrib.cpptasks.CCTask;
import net.sf.antcontrib.cpptasks.LinkerParam;
import net.sf.antcontrib.cpptasks.compiler.CommandLineLinkerConfiguration;
import net.sf.antcontrib.cpptasks.compiler.LinkType;
import net.sf.antcontrib.cpptasks.compiler.Linker;
import net.sf.antcontrib.cpptasks.gcc.AbstractArLibrarian;

import org.apache.tools.ant.BuildException;
/**
 * Adapter for the 'ar' archiver
 * 
 * @author Adam Murdoch
 */
public final class GccLibrarian extends AbstractArLibrarian {
    private static String[] objFileExtensions = new String[]{".o"};
    private static GccLibrarian instance = new GccLibrarian("ar",
            objFileExtensions, false, new GccLibrarian("ar", objFileExtensions,
                    true, null));
    public static GccLibrarian getInstance() {
        return instance;
    }
    private GccLibrarian(String command, String[] inputExtensions,
            boolean isLibtool, GccLibrarian libtoolLibrarian) {
        super(command, "V", inputExtensions, new String[0], "lib", ".a",
                isLibtool, libtoolLibrarian);
    }
    protected Object clone() throws CloneNotSupportedException {
        GccLibrarian clone = (GccLibrarian) super.clone();
        return clone;
    }
    public Linker getLinker(LinkType type) {
        return GccLinker.getInstance().getLinker(type);
    }
    public void link(CCTask task, File outputFile, String[] sourceFiles,
            CommandLineLinkerConfiguration config) throws BuildException {
        try {
            GccLibrarian clone = (GccLibrarian) this.clone();
            LinkerParam param = config.getParam("target");
            if (param != null)
                clone.setCommand(param.getValue() + "-" + this.getCommand());
            clone.superlink(task, outputFile, sourceFiles, config);
        } catch (CloneNotSupportedException e) {
            superlink(task, outputFile, sourceFiles, config);
        }
    }
    private void superlink(CCTask task, File outputFile, String[] sourceFiles,
            CommandLineLinkerConfiguration config) throws BuildException {
        super.link(task, outputFile, sourceFiles, config);
    }
}
