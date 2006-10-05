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
import java.util.Enumeration;
import java.util.Vector;

import net.sf.antcontrib.cpptasks.AslcompilerDef;
import net.sf.antcontrib.cpptasks.CCTask;
import net.sf.antcontrib.cpptasks.CUtil;
import net.sf.antcontrib.cpptasks.ProcessorDef;
import net.sf.antcontrib.cpptasks.TargetDef;
import net.sf.antcontrib.cpptasks.types.CommandLineArgument;

import org.apache.tools.ant.BuildException;
/**
 * An abstract ASL Compiler implementation which uses an external program to
 * perform the ASL compile.
 * 
 */
public abstract class CommandLineAslcompiler extends AbstractAslcompiler{

    private String command;
    private String identifier;
    private String identifierArg;
    
    protected CommandLineAslcompiler(String command, String identifierArg,
            String[] sourceExtensions, String[] headerExtensions,
            String outputSuffix) {
        super(sourceExtensions, headerExtensions, outputSuffix);
        this.command = command;
        this.identifierArg = identifierArg;
    }
    
    abstract protected void addImpliedArgs(Vector args, boolean debug,
        Boolean defaultflag);
   
    /**
     * Compile a ACPI source file
     * 
     */
    public void aslcompiler(CCTask task, File outputDir, String[] sourceFiles,
        String[] args, String[] endArgs) throws BuildException{
        String command = getCommand();
        int baseLength = command.length() + args.length + endArgs.length; 
        for (int i = 0; i < args.length; i++) {
          baseLength += args[i].length();
        }
        for (int i = 0; i < endArgs.length; i++) {
            baseLength += endArgs[i].length();
        }
        if (baseLength > getMaximumCommandLength()) {
            throw new BuildException(
                "Command line is over maximum length without sepcifying source file");
        }
        int maxInputFilesPerCommand = getMaximumInputFilesPerCommand();
        int argumentCountPerInputFile = getArgumentCountPerInputFIle();
        for (int sourceIndex = 0; sourceIndex < sourceFiles.length;) {
            int cmdLength = baseLength;
            int firstFileNextExec;
            for (firstFileNextExec = sourceIndex; firstFileNextExec < sourceFiles.length
                && (firstFileNextExec - sourceIndex) < maxInputFilesPerCommand; firstFileNextExec++) {
                cmdLength += getTotalArgumentLengthForInputFile(outputDir, 
                    sourceFiles[firstFileNextExec]);  
                if (cmdLength >= getMaximumCommandLength()) 
                    break;
            }
            if (firstFileNextExec == sourceIndex) {
                throw new BuildException(
                    "Extremely long file name, can't fit on command line");
            }
            int argCount = args.length + 1 + endArgs.length
                    + (firstFileNextExec - sourceIndex)
                    * argumentCountPerInputFile;
            String[] commandline = new String[argCount];
            int index = 0;
            commandline[index++] = command;
            for (int j = 0; j < args.length; j++) {
                commandline[index++] = args[j];
            }
            for (int j = sourceIndex; j < firstFileNextExec; j++) {
                for (int k = 0; k < argumentCountPerInputFile; k++) {
                    commandline[index++] = getInputFileArgument(outputDir, 
                            sourceFiles[j], k);                    
                }
            }
            for (int j = 0; j < endArgs.length; j++) {
                commandline[index++] = endArgs[j];
            }
            int retval = runCommand(task, outputDir, commandline);
            // if with monitor, add more code
            if (retval != 0) {
               throw new BuildException(this.getCommand()
                       + " failed with return code " + retval,
                       task.getLocation());
            }
            sourceIndex = firstFileNextExec;
        }
    }
    
    protected AslcompilerConfiguration createConfiguration(final CCTask task,
          final LinkType linkType, 
          final ProcessorDef[] baseDefs, 
          final AslcompilerDef specificDef,
          final TargetDef targetPlatform) {
        Vector args = new Vector();
        AslcompilerDef[] defaultProviders = new AslcompilerDef[baseDefs.length +1];
        for (int i = 0; i < baseDefs.length; i++) {
            defaultProviders[i + 1] = (AslcompilerDef) baseDefs[i];
        }
        defaultProviders[0] = specificDef;
        Vector cmdArgs = new Vector();
        //
        //  add command line arguments inherited from <cc> element
        //    any "extends" and finally and specific AslcompilerDef
        //
        CommandLineArgument[] commandArgs;
        for (int i = defaultProviders.length - 1; i >=0; i--){
            commandArgs = defaultProviders[i].getActiveProcessorArgs();
            for (int j = 0; j < commandArgs.length; j++) {
                if (commandArgs[j].getLocation() == 0) {
                    args.addElement(commandArgs[j].getValue());
                }
                else {
                    cmdArgs.addElement(commandArgs[j]);
                }
            }
        }
        // omit param
        boolean debug = specificDef.getDebug(baseDefs, 0);
        Boolean defaultflag = specificDef.getDefaultflag(defaultProviders, 1);
        this.addImpliedArgs(args, debug, defaultflag);
        Enumeration argEnum = cmdArgs.elements();
        int endCount = 0;
        while( argEnum.hasMoreElements()) {
            CommandLineArgument arg = (CommandLineArgument) argEnum.nextElement();
            switch (arg.getLocation()) {
                case 1 :
                    args.addElement(arg.getValue());
                    break;
                case 2 :
                    endCount++;
                    break;
            }
        }
        String[] endArgs = new String[endCount];
        argEnum = cmdArgs.elements();
        int index = 0;
        while (argEnum.hasMoreElements()) {
            CommandLineArgument arg = (CommandLineArgument) argEnum.nextElement();
            if (arg.getLocation() == 2) {
                endArgs[index++] = arg.getValue();
            }
        }
        String[] argArray = new String[args.size()];
        args.copyInto(argArray);
        return new CommandLineAslcompilerConfiguration(this, argArray, true, endArgs);
    }
    
    protected int getArgumentCountPerInputFile() {
        return 1;
    }
    
    public String getIdentifier() {
        if (identifier == null) {
            if (identifierArg == null) {
               identifier = getIdentifier(new String[]{command}, command);
            }
            else {
                identifier = getIdentifier(
                    new String[]{command, identifierArg}, command);
            }
        }
        return identifier;
    }
    
    public final String getCommand() {
      return command;
    }
    abstract public int getMaximumCommandLength();
    public void setCommand(String command) {
      this.command = command;
    }
    protected int getTotalArgumentLengthForInputFile(File outputDir,
        String inputFile) {
        return inputFile.length() + 1;
    }
    protected int runCommand(CCTask task, File workingDir, String[] cmdline) 
            throws BuildException {
        return CUtil.runCommand(task, workingDir, cmdline, false, null);
      
    }
    protected int getMaximumInputFilesPerCommand(){
        return 1;
    }
    protected int getArgumentCountPerInputFIle(){
        return 1;
    }
    protected String getInputFileArgument(File outputDir, String filename, int index) {
        //
        //   if there is an embedded space,
        //      must enclose in quotes
        if (filename.indexOf(' ') >= 0) {
            StringBuffer buf = new StringBuffer("\"");
            buf.append(filename);
            buf.append("\"");
            return buf.toString();
        }
        return filename;
    }
}
