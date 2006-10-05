/** @file
This file is to define an ANT task which wraps StrGather.exe tool.

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
package org.tianocore.framework.tasks;

import java.io.File;
import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Project;
import org.apache.tools.ant.Task;
import org.apache.tools.ant.taskdefs.Execute;
import org.apache.tools.ant.taskdefs.LogStreamHandler;
import org.apache.tools.ant.types.Commandline;
import org.tianocore.common.logger.EdkLog;

/**
 StrGather Task Class
 class memberg
      -commandType    : command type [parse/scan/dump] 
      -baseName       : base name of component 
      -verbose        : level of verbose [all/read/write]
      -outputDatabase : file name of output database file
      -databaseList   : file name list of database files
      -inputFileList  : file name list of input files
      -newDatabase    : whether to need new database [ture/false]
      -unquotedString : whether to unquoted strings are used [ture/false]
      -includePathList: path list of include paths
      -ignoreNotFound : whether to ignore a given STRING_TOKEN(STR) is not found in database [ture/false]
      -skipExtList    : skip scan of files with extension name
      -outputString   : write string data to filename 
      -outputDefines  : write string defines to filename 
      -outputUnicode  : dump database to unicode file filename
      -lang           : only dump for the language 
      -indirectionFile: specify an indirection file
      -outputHpk      : create an HII export pack of the strings
 **/
public class StrGatherTask extends Task implements EfiDefine {
    //
    // Tool name
    // 
    private static String toolName = "StrGather";

    //
    // common options
    //
    private ToolArg commandType = new ToolArg();

    private ToolArg baseName = new ToolArg();

    //
    // "all/read/write"
    //
    private ToolArg verbose = new ToolArg();

    private FileArg outputDatabase = new FileArg();

    private Database databaseList = new Database();

    private InputFile inputFileList = new InputFile();

    //
    // parse options newDatabase -- "ture/false" unquoteString -- "ture/false"
    //
    private ToolArg newDatabase = new ToolArg();

    private ToolArg unquotedString = new ToolArg();

    private IncludePath includePathList = new IncludePath();

    //
    // scan options ignoreNotFound -- "ture/false"
    //
    private ToolArg ignoreNotFound = new ToolArg();

    private SkipExt skipExtList = new SkipExt();

    //
    // dump options
    //
    private ToolArg outputString = new ToolArg();

    private ToolArg outputDefines = new ToolArg();

    private ToolArg outputUnicode = new ToolArg();

    private ToolArg lang = new ToolArg();

    private FileArg indirectionFile = new FileArg();

    private FileArg outputHpk = new FileArg();

    //
    // global variable
    //
    static private Project project;

    /**
     assemble tool command line & execute tool command line
     
     @throws BuildException
     **/
    public void execute() throws BuildException {

        project = this.getOwningTarget().getProject();
        ///
        /// absolute path of efi tools
        ///
        String path = project.getProperty("env.FRAMEWORK_TOOLS_PATH"); 
        String command;
        if (path == null) {
            command = toolName;
        } else {
            command = path + File.separator + toolName;
        }

        ///
        /// assemble argument
        ///
        String argument = "" + commandType + verbose + databaseList + baseName
                + outputDatabase + includePathList + newDatabase + unquotedString
                + skipExtList + ignoreNotFound + outputString + outputDefines
                + outputUnicode + lang + indirectionFile + outputHpk
                + inputFileList;
        ///
        /// return value of fwimage execution
        ///
        int revl = -1;

        try {
            Commandline cmdline = new Commandline();
            cmdline.setExecutable(command);
            cmdline.createArgument().setLine(argument);

            LogStreamHandler streamHandler = new LogStreamHandler(this,
                    Project.MSG_INFO, Project.MSG_WARN);
            Execute runner = new Execute(streamHandler, null);

            runner.setAntRun(project);
            runner.setCommandline(cmdline.getCommandline());

            String cmdType = getCommandType();
            if (cmdType.equalsIgnoreCase("parse")) {
                EdkLog.log(this, "(parse) " + inputFileList.toFileList() + " => " 
                    + databaseList.toFileList());
            } else if (cmdType.equalsIgnoreCase("scan")) {
                EdkLog.log(this, "(scan) " + databaseList.toFileList() + " => " 
                    + outputDatabase.toFileList());
            } else {
                EdkLog.log(this, "(dump) " + databaseList.toFileList() + " => " 
                    + outputDefines.toFileList() + outputString.toFileList() + outputHpk.toFileList());
            }
            EdkLog.log(this, EdkLog.EDK_VERBOSE, Commandline.toString(cmdline.getCommandline()));

            revl = runner.execute();
            if (EFI_SUCCESS == revl) {
                EdkLog.log(this, EdkLog.EDK_VERBOSE, toolName + " succeeded!");
            } else {
                ///
                /// command execution fail
                ///
                EdkLog.log(this, "ERROR = " + Integer.toHexString(revl));
                throw new BuildException(toolName + " failed!");
            }
        } catch (Exception e) {
            throw new BuildException(e.getMessage());
        }
    }

    /**
     get class member "commandType"
     
     @returns commandType parameter
     **/
    public String getCommandType() {
        return this.commandType.getValue();
    }

    /**
     set class member ""
     
     @param     commandType     type of strgather command [parse/scan/dump]
     **/
    public void setCommandType(String commandType) {
        this.commandType.setArg(" -", commandType);
    }

    /**
     get class member "verbose"
     
     @returns verbose parameter
     **/
    public String getVerbose() {
        return this.verbose.getValue();
    }

    /**
     set class member "verbose"
     
     @param     verbose     verbose level [all/read/write]
     **/
    public void setVerbose(String verbose) {
        if (verbose.equals("all")) {
            ///
            /// for verbose output
            ///
            this.verbose.setArg(" -", "v");
        } else if (verbose.equals("read")) {
            ///
            /// for verbose output when reading database
            ///
            this.verbose.setArg(" -", "vdbr");
        } else if (verbose.equals("write")) {
            ///
            /// for verbose output when writing database
            ///
            this.verbose.setArg(" -", "vdbw");
        }
    }

    /**
     get class member baseName
     
     @returns baseName parameter
     **/
    public String getBaseName() {
        return this.baseName.getValue();
    }

    /**
     set class member baseName
     
     @param     baseName    name of the output files of .c and .h
     **/
    public void setBaseName(String baseName) {
        this.baseName.setArg(" -bn ", baseName);
    }

    /**
     get class member "outputDatabase"
     
     @returns outputDatabase parameter
     **/
    public String getOutputDatabase() {
        return this.outputDatabase.getValue();
    }

    /**
     set class member "outputDatabase"
     
     @param     outputDatabase  filename of output database file
     **/
    public void setOutputDatabase(String outputDatabase) {
        this.outputDatabase.setArg(" -od ", outputDatabase);
    }

    /**
     get class member "newDatabase"
     
     @returns newDatabase parameter
     **/
    public boolean getNewDatabse() {
        return this.newDatabase.getPrefix().length() > 0;
    }

    /**
     set class member "newDatabase"
     
     @param     newDatabase     whether to not read in existing database file
     **/
    public void setNewDatabase(boolean newDatabase) {
        if (newDatabase) {
            this.newDatabase.setArg(" -", "newdb");
        }
    }

    /**
     get class member "unquotedString"
     
     @returns unquotedString parameter
     **/
    public boolean getUnquotedString() {
        return this.unquotedString.getValue().length() > 0;
    }

    /**
     set class member "unquotedString"
     
     @param unquotedString :
                whether to indicate that unquoted strings are used
     **/
    public void setUnquotedString(boolean unquotedString) {
        if (unquotedString) {
            this.unquotedString.setArg(" -", "uqs");
        }
    }

    /**
     get class member "ignoreNotFound"
     
     @returns ignoreNotFound parameter
     **/
    public boolean getIgnoreNotFound() {
        return this.ignoreNotFound.getValue().length() > 0;
    }

    /**
     set class member "ignoreNotFound"
     
     @param     ignoreNotFound  whether to ignore if a given STRING_TOKEN(STR) 
                                is not found in the database
     **/
    public void setIgnoreNotFound(boolean ignoreNotFound) {
        if (ignoreNotFound) {
            this.ignoreNotFound.setArg(" -", "ignorenotfound");
        }
    }

    /**
     get class member "outputString"
     
     @returns outputString parameter
     **/
    public String getOutputString() {
        return this.outputString.getValue();
    }

    /**
     set class member "outputString"
     
     @param     outputString    filename of string data file
     **/
    public void setOutputString(String outputString) {
        this.outputString.setArg(" -oc ", outputString);
    }

    /**
     get class member "outputDefines"
     
     @returns outputDefines parameter
     **/
    public String getOutputDefines() {
        return this.outputDefines.getValue();
    }

    /**
     set class member "outputDefines"
     
     @param     outputDefines   filename of string defines file
     **/
    public void setOutputDefines(String outputDefines) {
        this.outputDefines.setArg(" -oh ", outputDefines);
    }

    /**
     get class member "outputUnicode"
     
     @returns outputUnicode parameter
     **/
    public String getOutputUnicode() {
        return this.outputUnicode.getValue();
    }

    /**
     set class member "outputUnicode"
     
     @param     outputUnicode   filename of unicode file to be dumped database
     **/
    public void setOutputUnicode(String outputUnicode) {
        this.outputUnicode.setArg(" -ou ", outputUnicode);
    }

    /**
     get class member "lang"
     
     @returns lang parameter
     **/
    public String getLang() {
        return this.lang.getValue();
    }

    /**
     set class member "lang"
     
     @param     lang    language of dump
     **/
    public void setLang(String lang) {
        this.lang.setArg(" -lang ", lang);
    }

    /**
     get class member "indirectionFile"
     
     @returns indirectionFile parameter
     **/
    public String getIndirectionFile() {
        return this.indirectionFile.getValue();
    }

    /**
     set class member "indirectionFile"
     
     @param     indirectionFile     filename of indirection file
     **/
    public void setIndirectionFile(String indirectionFile) {
        this.indirectionFile.setArg(" -if ", indirectionFile);
    }

    /**
     get class member "outputHpk"
     
     @returns outputHpk parameter
     **/
    public String getOutputHpk() {
        return this.outputHpk.getValue();
    }

    /**
     set class member "outputHpk"
     
     @param     outputHpk   filename of output HII export pack of the strings
     **/
    public void setOutputHpk(String outputHpk) {
        this.outputHpk.setArg(" -hpk ", outputHpk);
    }

    /**
     add a skipExt element into list
     
     @param     skipExt     skipExt element
     **/
    public void addConfiguredSkipext(SkipExt skipExt) {
        this.skipExtList.insert(skipExt);
    };

    /**
     add a includePath element into list
     
     @param     includePath     includePath element
     **/
    public void addConfiguredIncludepath(IncludePath includePath) {
        this.includePathList.insert(includePath);
    };

    /**
     add a inputFile element into list
     
     @param     inputFile   inputFile element
     **/
    public void addConfiguredInputfile(InputFile inputFile) {
        this.inputFileList.insert(inputFile);
    };

    /**
     add a database element into list
     
     @param database :
                database element
     **/
    public void addConfiguredDatabase(Database database) {
        this.databaseList.insert(database);
    }
}
