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

import java.util.*;
import org.apache.tools.ant.Project;
import org.apache.tools.ant.Task;
import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.taskdefs.Execute;
import org.apache.tools.ant.taskdefs.LogStreamHandler;
import org.apache.tools.ant.types.Commandline;

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
    ///
    /// common options
    ///
    private String commandType = "";

    private String baseName = "";

    ///
    /// "all/read/write"
    ///
    private String verbose = "";

    private String outputDatabase = "";

    private List<Object> databaseList = new ArrayList<Object>();

    private List<Object> inputFileList = new ArrayList<Object>();

    ///
    /// parse options newDatabase -- "ture/false" unquoteString -- "ture/false"
    ///
    private String newDatabase = "";

    private String unquotedString = "";

    private List<Object> includePathList = new ArrayList<Object>();

    ///
    /// scan options ignoreNotFound -- "ture/false"
    ///
    private String ignoreNotFound = "";

    private List<Object> skipExtList = new ArrayList<Object>();

    ///
    /// dump options
    ///
    private String outputString = "";

    private String outputDefines = "";

    private String outputUnicode = "";

    private String lang = "";

    private String indirectionFile = "";

    private String outputHpk = "";

    ///
    /// global variable
    ///
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
            command = "StrGather";
        } else {
            command = path + "/" + "StrGather";
        }

        ///
        /// transfer nested elements into string
        ///
        String databases = list2Str(databaseList, "-db");
        String skipExts = list2Str(skipExtList, "-skipext");
        String includePaths = list2Str(includePathList, "-I");
        String inputFiles = list2Str(inputFileList, "");

        ///
        /// assemble argument
        ///
        String argument = commandType + verbose + databases + baseName
                + outputDatabase + includePaths + newDatabase + unquotedString
                + skipExts + ignoreNotFound + outputString + outputDefines
                + outputUnicode + lang + indirectionFile + outputHpk
                + inputFiles;
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

            log(Commandline.toString(cmdline.getCommandline()), Project.MSG_VERBOSE);
            log(this.commandType.substring(2));
            revl = runner.execute();
            if (EFI_SUCCESS == revl) {
                log("StrGather succeeded!", Project.MSG_VERBOSE);
            } else {
                ///
                /// command execution fail
                ///
                log("ERROR = " + Integer.toHexString(revl));
                throw new BuildException("StrGather failed!");
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
        return this.commandType;
    }

    /**
     set class member ""
     
     @param     commandType     type of strgather command [parse/scan/dump]
     **/
    public void setCommandType(String commandType) {
        this.commandType = " -" + commandType;
    }

    /**
     get class member "verbose"
     
     @returns verbose parameter
     **/
    public String getVerbose() {
        return this.verbose;
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
            this.verbose = " -v ";
        } else if (verbose.equals("read")) {
            ///
            /// for verbose output when reading database
            ///
            this.verbose = " -vdbr ";
        } else if (verbose.equals("write")) {
            ///
            /// for verbose output when writing database
            ///
            this.verbose = " -vdbw ";
        }
    }

    /**
     get class member baseName
     
     @returns baseName parameter
     **/
    public String getBaseName() {
        return this.baseName;
    }

    /**
     set class member baseName
     
     @param     baseName    name of the output files of .c and .h
     **/
    public void setBaseName(String baseName) {
        this.baseName = " -bn " + baseName;
    }

    /**
     get class member "outputDatabase"
     
     @returns outputDatabase parameter
     **/
    public String getOutputDatabase() {
        return this.outputDatabase;
    }

    /**
     set class member "outputDatabase"
     
     @param     outputDatabase  filename of output database file
     **/
    public void setOutputDatabase(String outputDatabase) {
        this.outputDatabase = " -od " + outputDatabase;
    }

    /**
     get class member "newDatabase"
     
     @returns newDatabase parameter
     **/
    public String getNewDatabse() {
        return this.newDatabase;
    }

    /**
     set class member "newDatabase"
     
     @param     newDatabase     whether to not read in existing database file
     **/
    public void setNewDatabase(String newDatabase) {
        if (newDatabase.equals("true")) {
            this.newDatabase = " -newdb ";
        }
    }

    /**
     get class member "unquotedString"
     
     @returns unquotedString parameter
     **/
    public String getUnquotedString() {
        return this.unquotedString;
    }

    /**
     set class member "unquotedString"
     
     @param unquotedString :
                whether to indicate that unquoted strings are used
     **/
    public void setUnquotedString(String unquotedString) {
        if (unquotedString.equals("true")) {
            this.unquotedString = " -uqs ";
        }
    }

    /**
     get class member "ignoreNotFound"
     
     @returns ignoreNotFound parameter
     **/
    public String getIgnoreNotFound() {
        return this.ignoreNotFound;
    }

    /**
     set class member "ignoreNotFound"
     
     @param     ignoreNotFound  whether to ignore if a given STRING_TOKEN(STR) 
                                is not found in the database
     **/
    public void setIgnoreNotFound(String ignoreNotFound) {
        if (ignoreNotFound.equals("true")) {
            this.ignoreNotFound = " -ignorenotfound ";
        }
    }

    /**
     get class member "outputString"
     
     @returns outputString parameter
     **/
    public String getOutputString() {
        return this.outputString;
    }

    /**
     set class member "outputString"
     
     @param     outputString    filename of string data file
     **/
    public void setOutputString(String outputString) {
        this.outputString = " -oc " + outputString;
    }

    /**
     get class member "outputDefines"
     
     @returns outputDefines parameter
     **/
    public String getOutputDefines() {
        return this.outputDefines;
    }

    /**
     set class member "outputDefines"
     
     @param     outputDefines   filename of string defines file
     **/
    public void setOutputDefines(String outputDefines) {
        this.outputDefines = " -oh " + outputDefines;
    }

    /**
     get class member "outputUnicode"
     
     @returns outputUnicode parameter
     **/
    public String getOutputUnicode() {
        return this.outputUnicode;
    }

    /**
     set class member "outputUnicode"
     
     @param     outputUnicode   filename of unicode file to be dumped database
     **/
    public void setOutputUnicode(String outputUnicode) {
        this.outputUnicode = " -ou " + outputUnicode;
    }

    /**
     get class member "lang"
     
     @returns lang parameter
     **/
    public String getLang() {
        return this.lang;
    }

    /**
     set class member "lang"
     
     @param     lang    language of dump
     **/
    public void setLang(String lang) {
        this.lang = " -lang " + lang;
    }

    /**
     get class member "indirectionFile"
     
     @returns indirectionFile parameter
     **/
    public String getIndirectionFile() {
        return this.indirectionFile;
    }

    /**
     set class member "indirectionFile"
     
     @param     indirectionFile     filename of indirection file
     **/
    public void setIndirectionFile(String indirectionFile) {
        this.indirectionFile = " -if " + indirectionFile;
    }

    /**
     get class member "outputHpk"
     
     @returns outputHpk parameter
     **/
    public String getOutputHpk() {
        return this.outputHpk;
    }

    /**
     set class member "outputHpk"
     
     @param     outputHpk   filename of output HII export pack of the strings
     **/
    public void setOutputHpk(String outputHpk) {
        this.outputHpk = " -hpk " + outputHpk;
    }

    /**
     add a skipExt element into list
     
     @param     skipExt     skipExt element
     **/
    public void addSkipext(SkipExt skipExt) {
        skipExtList.add(skipExt);
    };

    /**
     add a includePath element into list
     
     @param     includePath     includePath element
     **/
    public void addIncludepath(IncludePath includePath) {
        includePathList.add(includePath);
    };

    /**
     add a inputFile element into list
     
     @param     inputFile   inputFile element
     **/
    public void addInputfile(InputFile inputFile) {
        inputFileList.add(inputFile);
    };

    /**
     add a database element into list
     
     @param database :
                database element
     **/
    public void addDatabase(Database database) {
        databaseList.add(database);
    }

    /**
     transfer List to String
     
     @param     list    nested element list
     @param     tag     interval tag of parameter

     @returns string line of parameters
     **/
    private String list2Str(List list, String tag) {
        ///
        /// string line for return
        ///
        String paraStr = "";
        ///
        /// nested element in list
        ///
        NestElement element;
        ///
        /// iterator of nested element list
        ///
        Iterator elementIter = list.iterator();
        ///
        /// string parameter list
        ///
        List<Object> strList = new ArrayList<Object>();

        while (elementIter.hasNext()) {
            element = (NestElement) elementIter.next();
            if (null != element.getFile()) {
                ///
                /// nested element include file
                ///
                FileParser.loadFile(project, strList, element.getFile(), tag);
            } 

            if (element.getName().length() > 0) {
                ///
                /// nested element include name 
                ///
                paraStr = paraStr + " " + element.getName();
            }

            List<String> nameList = element.getList();
            if (nameList.size() > 0) {
                Iterator nameIter = nameList.iterator();
                while (nameIter.hasNext()) {
                    paraStr = paraStr + " " + tag + " " + (String)nameIter.next();
                }
            }
        }
        ///
        /// iterator of string parameter list
        ///
        Iterator strIter = strList.iterator();
        while (strIter.hasNext()) {
            paraStr = paraStr + " " + strIter.next();
        }
        return paraStr;
    }

}
