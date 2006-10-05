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
package net.sf.antcontrib.cpptasks.types;
import java.io.File;

import net.sf.antcontrib.cpptasks.CUtil;
import net.sf.antcontrib.cpptasks.FileVisitor;
import net.sf.antcontrib.cpptasks.compiler.Linker;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.DirectoryScanner;
import org.apache.tools.ant.Project;
import org.apache.tools.ant.ProjectComponent;
import org.apache.tools.ant.types.FileSet;
import org.apache.tools.ant.types.PatternSet;
/**
 * A set of library names. Libraries can also be added to a link by specifying
 * them in a fileset.
 * 
 * For most Unix-like compilers, libset will result in a series of -l and -L
 * linker arguments. For Windows compilers, the library names will be used to
 * locate the appropriate library files which will be added to the linkers
 * input file list as if they had been specified in a fileset.
 * 
 * @author Mark A Russell <a
 *         href="mailto:mark_russell@csgsystems.com">mark_russell@csg_systems.com
 *         </a>
 * @author Adam Murdoch
 * @author Curt Arnold
 */
public class LibrarySet extends ProjectComponent {
    private String dataset;
    private boolean explicitCaseSensitive;
    private String ifCond;
    private String[] libnames;
    private final FileSet set = new FileSet();
    private String unlessCond;
    private LibraryTypeEnum libraryType;
    public LibrarySet() {
        libnames = new String[0];
    }
    public void execute() throws org.apache.tools.ant.BuildException {
        throw new org.apache.tools.ant.BuildException(
                "Not an actual task, but looks like one for documentation purposes");
    }
    /**
     * Gets the dataset. Used on OS390 if the libs are in a dataset.
     * 
     * @return Returns a String
     */
    public String getDataset() {
        return dataset;
    }
    public File getDir(Project project) {
        return set.getDir(project);
    }
    protected FileSet getFileSet() {
        return set;
    }
    public String[] getLibs() {
        String[] retval = (String[]) libnames.clone();
        return retval;
    }
    
    /**
     * Gets preferred library type
     * 
     * @return library type, may be null.
     */
    public LibraryTypeEnum getType() {
    	return libraryType;
    }
    /**
     * Returns true if the define's if and unless conditions (if any) are
     * satisfied.
     */
    public boolean isActive(org.apache.tools.ant.Project p) {
        if (p == null) {
            throw new NullPointerException("p");
        }
        if (ifCond != null) {
            String ifValue = p.getProperty(ifCond);
            if (ifValue != null) {
                if (ifValue.equals("no") || ifValue.equals("false")) {
                    throw new BuildException(
                            "property "
                                    + ifCond
                                    + " used as if condition has value "
                                    + ifValue
                                    + " which suggests a misunderstanding of if attributes");
                }
            } else {
                return false;
            }
        }
        if (unlessCond != null) {
            String unlessValue = p.getProperty(unlessCond);
            if (unlessValue != null) {
                if (unlessValue.equals("no") || unlessValue.equals("false")) {
                    throw new BuildException(
                            "property "
                                    + unlessCond
                                    + " used as unless condition has value "
                                    + unlessValue
                                    + " which suggests a misunderstanding of unless attributes");
                }
                return false;
            }
        }
        if (libnames.length == 0) {
            p.log("libnames not specified or empty.", Project.MSG_WARN);
            return false;
        }
        return true;
    }
    /**
     * Sets case sensitivity of the file system. If not set, will default to
     * the linker's case sensitivity.
     * 
     * @param isCaseSensitive
     *            "true"|"on"|"yes" if file system is case sensitive,
     *            "false"|"off"|"no" when not.
     */
    public void setCaseSensitive(boolean isCaseSensitive) {
        explicitCaseSensitive = true;
        set.setCaseSensitive(isCaseSensitive);
    }
    /**
     * Sets the dataset. Used on OS390 if the libs are in a dataset.
     * 
     * @param dataset
     *            The dataset to set
     */
    public void setDataset(String dataset) {
        this.dataset = dataset;
    }
    /**
     * Library directory.
     * 
     * @param dir
     *            library directory
     *  
     */
    public void setDir(File dir) throws BuildException {
        set.setDir(dir);
    }
    /**
     * Sets the property name for the 'if' condition.
     * 
     * The library set will be ignored unless the property is defined.
     * 
     * The value of the property is insignificant, but values that would imply
     * misinterpretation ("false", "no") will throw an exception when
     * evaluated.
     * 
     * @param propName
     *            property name
     */
    public void setIf(String propName) {
        ifCond = propName;
    }
    /**
     * Comma-separated list of library names without leading prefixes, such as
     * "lib", or extensions, such as ".so" or ".a".
     *  
     */
    public void setLibs(CUtil.StringArrayBuilder libs) throws BuildException {
        libnames = libs.getValue();
        // If this is not active.. then it's ok if the lib names are invalid.
        // so we can do a: <libset if="x.lib" dir="." libs="${x.lib}"/>
        if (!isActive(getProject()))
            return;
        for (int i = 0; i < libnames.length; i++) {
            int lastDot = libnames[i].lastIndexOf('.');
            if (lastDot >= 0) {
                String extension = libnames[i].substring(lastDot);
                if (extension.equalsIgnoreCase(".lib")
                        || extension.equalsIgnoreCase(".so")
                        || extension.equalsIgnoreCase(".a")) {
                    getProject().log(
                            "Suspicious library name ending with \""
                                    + extension + "\": " + libnames[i], Project.MSG_DEBUG );
                }
            }
            if (libnames[i].length() >= 3
                    && libnames[i].substring(0, 3).equalsIgnoreCase("lib")) {
                getProject().log(
                        "Suspicious library name starting with \"lib\": "
                                + libnames[i], Project.MSG_DEBUG);
            }
        }
    }
    public void setProject(Project project) {
        set.setProject(project);
        super.setProject(project);
    }
    /**
     * Set the property name for the 'unless' condition.
     * 
     * If named property is set, the library set will be ignored.
     * 
     * The value of the property is insignificant, but values that would imply
     * misinterpretation ("false", "no") of the behavior will throw an
     * exception when evaluated.
     * 
     * @param propName
     *            name of property
     */
    public void setUnless(String propName) {
        unlessCond = propName;
    }
    
    /**
     * Sets the preferred library type. Supported values "shared", "static", and
     * "framework".  "framework" is equivalent to "shared" on non-Darwin platforms. 
     */
    public void setType(LibraryTypeEnum type) {
    	this.libraryType = type;
    }
    
    public void visitLibraries(Project project, Linker linker, File[] libpath,
            FileVisitor visitor) throws BuildException {
        FileSet localSet = (FileSet) set.clone();
        //
        //   unless explicitly set
        //      will default to the linker case sensitivity
        //
        if (!explicitCaseSensitive) {
            boolean linkerCaseSensitive = linker.isCaseSensitive();
            localSet.setCaseSensitive(linkerCaseSensitive);
        }
        //
        //  if there was a libs attribute then
        //     add the corresponding patterns to the FileSet
        //
        if (libnames != null && libnames.length > 0) {
            String[] patterns = linker.getLibraryPatterns(libnames, libraryType);
            //
            //  if no patterns, then linker does not support libraries
            //
            if (patterns.length > 0) {
		        for (int i = 0; i < patterns.length; i++) {
		             PatternSet.NameEntry entry = localSet.createInclude();
		             entry.setName(patterns[i]);
		        }
		        //
		        //  if there was no specified directory then
		        //     run through the libpath backwards
		        //
		        if (localSet.getDir(project) == null) {
		            //
		            //  scan libpath in reverse order
		            //     to give earlier entries priority
		            //
		            for (int j = libpath.length - 1; j >= 0; j--) {
		                FileSet clone = (FileSet) localSet.clone();
		                clone.setDir(libpath[j]);
		                DirectoryScanner scanner = clone.getDirectoryScanner(project);
		                File basedir = scanner.getBasedir();
		                String[] files = scanner.getIncludedFiles();
		                for (int k = 0; k < files.length; k++) {
		                    visitor.visit(basedir, files[k]);
		                }
		            }
		        } else {
		            DirectoryScanner scanner = localSet.getDirectoryScanner(project);
		            File basedir = scanner.getBasedir();
		            String[] files = scanner.getIncludedFiles();
		            for (int k = 0; k < files.length; k++) {
		                visitor.visit(basedir, files[k]);
		            }
		        }
        	}
        }
    }
}
