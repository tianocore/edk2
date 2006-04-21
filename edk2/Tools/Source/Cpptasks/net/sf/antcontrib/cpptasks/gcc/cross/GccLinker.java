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
package net.sf.antcontrib.cpptasks.gcc.cross;
import java.io.File;
import java.util.Vector;
import net.sf.antcontrib.cpptasks.CCTask;
import net.sf.antcontrib.cpptasks.CUtil;
import net.sf.antcontrib.cpptasks.LinkerParam;
import net.sf.antcontrib.cpptasks.compiler.CommandLineLinkerConfiguration;
import net.sf.antcontrib.cpptasks.compiler.LinkType;
import net.sf.antcontrib.cpptasks.compiler.Linker;
import net.sf.antcontrib.cpptasks.gcc.AbstractLdLinker;
import org.apache.tools.ant.BuildException;
/**
 * Adapter for the GCC linker
 * 
 * @author Adam Murdoch
 */
public class GccLinker extends AbstractLdLinker {
    private static final String[] discardFiles = new String[0];
    private static final String[] objFiles = new String[]{".o", ".a", ".lib",
            ".dll", ".so", ".sl"};
    private static final GccLinker dllLinker = new GccLinker("gcc", objFiles,
            discardFiles, "lib", ".so", false, new GccLinker("gcc", objFiles,
                    discardFiles, "lib", ".so", true, null));
    private static final GccLinker instance = new GccLinker("gcc", objFiles,
            discardFiles, "", "", false, null);
    private static final String[] libtoolObjFiles = new String[]{".fo", ".a",
            ".lib", ".dll", ".so", ".sl"};
    private static String[] linkerOptions = new String[]{"-bundle",
            "-dynamiclib", "-nostartfiles", "-nostdlib", "-prebind", "-s",
            "-static", "-shared", "-symbolic", "-Xlinker",
            "--export-all-symbols", "-static-libgcc",};
    private static final GccLinker machBundleLinker = new GccLinker("gcc",
            objFiles, discardFiles, "lib", ".bundle", false, null);
    private static final GccLinker machDllLinker = new GccLinker("gcc",
            objFiles, discardFiles, "lib", ".dylib", false, null);
    public static GccLinker getInstance() {
        return instance;
    }
    private File[] libDirs;
    protected GccLinker(String command, String[] extensions,
            String[] ignoredExtensions, String outputPrefix,
            String outputSuffix, boolean isLibtool, GccLinker libtoolLinker) {
        super(command, "-dumpversion", extensions, ignoredExtensions,
                outputPrefix, outputSuffix, isLibtool, libtoolLinker);
    }
    protected void addImpliedArgs(boolean debug, LinkType linkType, Vector args, Boolean defaultflag) {
        super.addImpliedArgs(debug, linkType, args, defaultflag);
        if (getIdentifier().indexOf("mingw") >= 0) {
            if (linkType.isSubsystemConsole()) {
                args.addElement("-mconsole");
            }
            if (linkType.isSubsystemGUI()) {
                args.addElement("-mwindows");
            }
        }
    }
    protected Object clone() throws CloneNotSupportedException {
        GccLinker clone = (GccLinker) super.clone();
        return clone;
    }
    /**
     * Allows drived linker to decorate linker option. Override by GccLinker to
     * prepend a "-Wl," to pass option to through gcc to linker.
     * 
     * @param buf
     *            buffer that may be used and abused in the decoration process,
     *            must not be null.
     * @param arg
     *            linker argument
     */
    public String decorateLinkerOption(StringBuffer buf, String arg) {
        String decoratedArg = arg;
        if (arg.length() > 1 && arg.charAt(0) == '-') {
            switch (arg.charAt(1)) {
                //
                //   passed automatically by GCC
                //
                case 'g' :
                case 'f' :
                case 'F' :
                /* Darwin */
                case 'm' :
                case 'O' :
                case 'W' :
                case 'l' :
                case 'L' :
                case 'u' :
                case 'v' :
                    break;
                default :
                    boolean known = false;
                    for (int i = 0; i < linkerOptions.length; i++) {
                        if (linkerOptions[i].equals(arg)) {
                            known = true;
                            break;
                        }
                    }
                    if (!known) {
                        buf.setLength(0);
                        buf.append("-Wl,");
                        buf.append(arg);
                        decoratedArg = buf.toString();
                    }
                    break;
            }
        }
        return decoratedArg;
    }
    /**
     * Returns library path.
     *  
     */
    public File[] getLibraryPath() {
        if (libDirs == null) {
            //
            //   construct gcc lib path from machine and version
            //
            StringBuffer buf = new StringBuffer("/lib/gcc-lib/");
            buf.append(GccProcessor.getMachine());
            buf.append('/');
            buf.append(GccProcessor.getVersion());
            //
            //   build default path from gcc and system /lib and /lib/w32api
            //
            String[] impliedLibPath = new String[]{buf.toString(),
                    "/lib/w32api", "/lib"};
            //
            //     read gcc specs file for other library paths
            //
            String[] specs = GccProcessor.getSpecs();
            String[][] libpaths = GccProcessor.parseSpecs(specs, "*link:",
                    new String[]{"%q"});
            String[] libpath;
            if (libpaths[0].length > 0) {
                libpath = new String[libpaths[0].length + 3];
                int i = 0;
                for (; i < libpaths[0].length; i++) {
                    libpath[i] = libpaths[0][i];
                }
                libpath[i++] = buf.toString();
                libpath[i++] = "/lib/w32api";
                libpath[i++] = "/lib";
            } else {
                //
                //   if a failure to find any matches then
                //      use some default values for lib path entries
                libpath = new String[]{"/usr/local/lib/mingw",
                        "/usr/local/lib", "/usr/lib/w32api", "/usr/lib/mingw",
                        "/usr/lib", buf.toString(), "/lib/w32api", "/lib"};
            }
            for (int i = 0; i < libpath.length; i++) {
                if (libpath[i].indexOf("mingw") >= 0) {
                    libpath[i] = null;
                }
            }
            //
            //   if cygwin then
            //     we have to prepend location of gcc32
            //       and .. to start of absolute filenames to
            //       have something that will exist in the
            //       windows filesystem
            if (GccProcessor.isCygwin()) {
                GccProcessor.convertCygwinFilenames(libpath);
            }
            //
            //  check that remaining entries are actual directories
            //
            int count = CUtil.checkDirectoryArray(libpath);
            //
            //   populate return array with remaining entries
            //
            libDirs = new File[count];
            int index = 0;
            for (int i = 0; i < libpath.length; i++) {
                if (libpath[i] != null) {
                    libDirs[index++] = new File(libpath[i]);
                }
            }
        }
        return libDirs;
    }
    public Linker getLinker(LinkType type) {
        if (type.isStaticLibrary()) {
            return GccLibrarian.getInstance();
        }
        if (type.isPluginModule()) {
            if (isDarwin()) {
                return machBundleLinker;
            } else {
                return dllLinker;
            }
        }
        if (type.isSharedLibrary()) {
            if (isDarwin()) {
                return machDllLinker;
            } else {
                return dllLinker;
            }
        }
        return instance;
    }
    public void link(CCTask task, File outputFile, String[] sourceFiles,
            CommandLineLinkerConfiguration config) throws BuildException {
        try {
            GccLinker clone = (GccLinker) this.clone();
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
