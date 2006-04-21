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

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.util.Enumeration;
import java.util.Vector;

import net.sf.antcontrib.cpptasks.CCTask;
import net.sf.antcontrib.cpptasks.CUtil;
import net.sf.antcontrib.cpptasks.LinkerDef;
import net.sf.antcontrib.cpptasks.ProcessorDef;
import net.sf.antcontrib.cpptasks.ProcessorParam;
import net.sf.antcontrib.cpptasks.types.CommandLineArgument;
import net.sf.antcontrib.cpptasks.types.LibrarySet;
import net.sf.antcontrib.cpptasks.TargetDef;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.types.Environment;


/**
 * An abstract Linker implementation that performs the link via an external
 * command.
 *
 * @author Adam Murdoch
 */
public abstract class CommandLineLinker extends AbstractLinker
{
    private String command;
    private Environment env = null;
    private String identifier;
    private String identifierArg;
    private boolean isLibtool;
    private String[] librarySets;
    private CommandLineLinker libtoolLinker;
    private boolean newEnvironment = false;
    private String outputSuffix;


    /** Creates a comand line linker invocation */
    public CommandLineLinker(String command,
        String identifierArg,
        String[] extensions,
        String[] ignoredExtensions, String outputSuffix,
        boolean isLibtool, CommandLineLinker libtoolLinker)
    {
        super(extensions, ignoredExtensions);
        this.command = command;
        this.identifierArg = identifierArg;
        this.outputSuffix = outputSuffix;
        this.isLibtool = isLibtool;
        this.libtoolLinker = libtoolLinker;
    }
    protected abstract void addBase(long base, Vector args);

    protected abstract void addFixed(Boolean fixed, Vector args);

    abstract protected void addImpliedArgs(boolean debug,
      LinkType linkType, Vector args, Boolean defaultflag);
    protected abstract void addIncremental(boolean incremental, Vector args);

      //
      //  Windows processors handle these through file list
      //
    protected String[] addLibrarySets(CCTask task, LibrarySet[] libsets, Vector preargs,
        Vector midargs, Vector endargs) {
        return null;
    }
    protected abstract void addMap(boolean map, Vector args);
    protected abstract void addStack(int stack, Vector args);
    protected abstract void addEntry(String entry, Vector args);
    
    protected LinkerConfiguration createConfiguration(
      CCTask task,
      LinkType linkType,
      ProcessorDef[] baseDefs, LinkerDef specificDef, TargetDef targetPlatform) {

      Vector preargs = new Vector();
      Vector midargs = new Vector();
      Vector endargs = new Vector();
      Vector[] args = new Vector[] { preargs, midargs, endargs };

      LinkerDef[] defaultProviders = new LinkerDef[baseDefs.length+1];
      defaultProviders[0] = specificDef;
      for(int i = 0; i < baseDefs.length; i++) {
        defaultProviders[i+1] = (LinkerDef) baseDefs[i];
      }
      //
      //   add command line arguments inherited from <cc> element
      //     any "extends" and finally the specific CompilerDef
      CommandLineArgument[] commandArgs;
      for(int i = defaultProviders.length-1; i >= 0; i--) {
        commandArgs = defaultProviders[i].getActiveProcessorArgs();
        for(int j = 0; j < commandArgs.length; j++) {
          args[commandArgs[j].getLocation()].
                addElement(commandArgs[j].getValue());
        }
      }

        Vector params = new Vector();
        //
        //   add command line arguments inherited from <cc> element
        //     any "extends" and finally the specific CompilerDef
        ProcessorParam[] paramArray;
        for (int i = defaultProviders.length - 1; i >= 0; i--) {
            paramArray = defaultProviders[i].getActiveProcessorParams();
            for (int j = 0; j < paramArray.length; j++) {
                params.add(paramArray[j]);
            }
        }

        paramArray = (ProcessorParam[])(params.toArray(new ProcessorParam[params.size()]));

        boolean debug = specificDef.getDebug(baseDefs,0);

      
      String startupObject = getStartupObject(linkType);
      Boolean defaultflag = specificDef.getDefaultflag(defaultProviders, 1);
      addImpliedArgs(debug, linkType, preargs, defaultflag);
      addIncremental(specificDef.getIncremental(defaultProviders,1), preargs);
      addFixed(specificDef.getFixed(defaultProviders,1), preargs);
      addMap(specificDef.getMap(defaultProviders,1), preargs);
      addBase(specificDef.getBase(defaultProviders,1), preargs);
      addStack(specificDef.getStack(defaultProviders,1), preargs);
      addEntry(specificDef.getEntry(defaultProviders, 1), preargs);

      String[] libnames = null;
      LibrarySet[] libsets = specificDef.getActiveLibrarySets(defaultProviders,1);
      if (libsets.length > 0) {
        libnames = addLibrarySets(task, libsets, preargs, midargs, endargs);
      }

      StringBuffer buf = new StringBuffer(getIdentifier());
      for (int i = 0; i < 3; i++) {
        Enumeration argenum = args[i].elements();
        while (argenum.hasMoreElements()) {
           buf.append(' ');
           buf.append(argenum.nextElement().toString());
        }
      }
      String configId = buf.toString();

      String[][] options = new String[][] {
        new String[args[0].size() + args[1].size()],
        new String[args[2].size()] };
      args[0].copyInto(options[0]);
      int offset = args[0].size();
      for (int i = 0; i < args[1].size(); i++) {
        options[0][i+offset] = (String) args[1].elementAt(i);
      }
      args[2].copyInto(options[1]);


      boolean rebuild = specificDef.getRebuild(baseDefs,0);
      boolean map = specificDef.getMap(defaultProviders,1);

      //task.log("libnames:"+libnames.length, Project.MSG_VERBOSE);
      return new CommandLineLinkerConfiguration(this,configId,options,
              paramArray,
              rebuild,map,libnames, startupObject);
    }

    /**
     * Allows drived linker to decorate linker option.
     * Override by GccLinker to prepend a "-Wl," to
     * pass option to through gcc to linker.
     *
     * @param buf buffer that may be used and abused in the decoration process,
     * must not be null.
     * @param arg linker argument
     */
    protected String decorateLinkerOption(StringBuffer buf, String arg) {
      return arg;
    }

    protected final String getCommand() {
      return command;
    }
    protected abstract String getCommandFileSwitch(String commandFile);


     public String getIdentifier() {
      if(identifier == null) {
        if (identifierArg == null) {
          identifier = getIdentifier(new String[] { command }, command);
        } else {
          identifier = getIdentifier(new String[] { command, identifierArg },
            command);
        }
      }
      return identifier;
    }
    public final CommandLineLinker getLibtoolLinker() {
      if (libtoolLinker != null) {
        return libtoolLinker;
      }
      return this;
    }
    protected abstract int getMaximumCommandLength();

    public String getOutputFileName(String baseName) {
        return baseName + outputSuffix;
    }

    protected String[] getOutputFileSwitch(CCTask task, String outputFile) {
        return getOutputFileSwitch(outputFile);
    }
    protected abstract String[] getOutputFileSwitch(String outputFile);
    protected String getStartupObject(LinkType linkType) {
      return null;
    }

    /**
     * Performs a link using a command line linker
     *
     */
    public void link(CCTask task,
                     File outputFile,
                     String[] sourceFiles,
                     CommandLineLinkerConfiguration config)
                     throws BuildException
    {
        File parentDir = new File(outputFile.getParent());
        String parentPath;
        try {
          parentPath = parentDir.getCanonicalPath();
        } catch(IOException ex) {
          parentPath = parentDir.getAbsolutePath();
        }
        String[] execArgs = prepareArguments(task, parentPath,outputFile.getName(),
            sourceFiles, config);
        int commandLength = 0;
        for(int i = 0; i < execArgs.length; i++) {
          commandLength += execArgs[i].length() + 1;
        }

        //
        //   if command length exceeds maximum
        //       (1024 for Windows) then create a temporary
        //       file containing everything but the command name
        if(commandLength >= this.getMaximumCommandLength()) {
          try {
            execArgs = prepareResponseFile(outputFile,execArgs);
          }
          catch(IOException ex) {
            throw new BuildException(ex);
          }
        }
        
        int retval = runCommand(task,parentDir,execArgs);        
        //
        //   if the process returned a failure code then
        //       throw an BuildException
        //
        if(retval != 0) {
          //
          //   construct the exception
          //
          throw new BuildException(this.getCommand() + " failed with return code " + retval, task.getLocation());
        }
        
    }


    /**
     * Prepares argument list for exec command.  Will return null
     * if command line would exceed allowable command line buffer.
     *
     * @param outputFile linker output file
     * @param sourceFiles linker input files (.obj, .o, .res)
     * @param args linker arguments
     * @return arguments for runTask
     */
    protected String[] prepareArguments(
        CCTask task,
        String outputDir,
        String outputFile,
        String[] sourceFiles,
        CommandLineLinkerConfiguration config) {
                        
        String[] preargs = config.getPreArguments();
        String[] endargs = config.getEndArguments();
        String outputSwitch[] =  getOutputFileSwitch(task, outputFile);
        int allArgsCount = preargs.length + 1 + outputSwitch.length +
                sourceFiles.length + endargs.length;
        if (isLibtool) {
          allArgsCount++;
        }
        String[] allArgs = new String[allArgsCount];
        int index = 0;
        if (isLibtool) {
          allArgs[index++] = "libtool";
        }
        allArgs[index++] = this.getCommand();
        StringBuffer buf = new StringBuffer();
        for (int i = 0; i < preargs.length; i++) {
          allArgs[index++] = decorateLinkerOption(buf, preargs[i]);
        }
        for (int i = 0; i < outputSwitch.length; i++) {
          allArgs[index++] = outputSwitch[i];
        }
        for (int i = 0; i < sourceFiles.length; i++) {
          allArgs[index++] = prepareFilename(buf,outputDir,sourceFiles[i]);
        }
        for (int i = 0; i < endargs.length; i++) {
          allArgs[index++] = decorateLinkerOption(buf, endargs[i]);
        }
        return allArgs;
    }

    /**
     * Processes filename into argument form
     *
     */
    protected String prepareFilename(StringBuffer buf,
      String outputDir, String sourceFile) {
      String relativePath = CUtil.getRelativePath(outputDir,
        new File(sourceFile));
      return quoteFilename(buf,relativePath);
    }

    /**
     * Prepares argument list to execute the linker using a
     * response file.
     *
     * @param outputFile linker output file
     * @param args output of prepareArguments
     * @return arguments for runTask
     */
    protected String[] prepareResponseFile(File outputFile,String[] args) throws IOException
    {
        String baseName = outputFile.getName();
        File commandFile = new File(outputFile.getParent(),baseName + ".rsp");
        FileWriter writer = new FileWriter(commandFile);
        int execArgCount = 1;
        if (isLibtool) {
          execArgCount++;
        }
        String[] execArgs = new String[execArgCount+1];
        for (int i = 0; i < execArgCount; i++) {
          execArgs[i] = args[i];
        }
        execArgs[execArgCount] = getCommandFileSwitch(commandFile.toString());
        for(int i = execArgCount; i < args.length; i++) {
        	//
        	//   if embedded space and not quoted then
        	//       quote argument
          if (args[i].indexOf(" ") >= 0 && args[i].charAt(0) != '\"') {
          	writer.write('\"');
          	writer.write(args[i]);
          	writer.write("\"\n");
          } else {
          	writer.write(args[i]);
            writer.write('\n');
          }
        }
        writer.close();
        return execArgs;
    }


    protected String quoteFilename(StringBuffer buf,String filename) {
      if(filename.indexOf(' ') >= 0) {
        buf.setLength(0);
        buf.append('\"');
        buf.append(filename);
        buf.append('\"');
        return buf.toString();
      }
      return filename;
    }

    /**
     * This method is exposed so test classes can overload
     * and test the arguments without actually spawning the
     * compiler
     */
    protected int runCommand(CCTask task, File workingDir,String[] cmdline)
      throws BuildException {
      return CUtil.runCommand(task,workingDir,cmdline, newEnvironment, env);
    }

    protected final void setCommand(String command) {
        this.command = command;
    }

}
