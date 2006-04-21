package net.sf.antcontrib.cpptasks.userdefine;

import java.io.File;
import java.util.Iterator;
import java.util.StringTokenizer;
import java.util.Vector;

import net.sf.antcontrib.cpptasks.CCTask;
import net.sf.antcontrib.cpptasks.CUtil;
import net.sf.antcontrib.cpptasks.types.CommandLineArgument;
import net.sf.antcontrib.cpptasks.types.ConditionalFileSet;
import net.sf.antcontrib.cpptasks.types.LibrarySet;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.DirectoryScanner;
import org.apache.tools.ant.Project;
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
public class CommandLineUserDefine {

    String command;
    
    /*
     * The follows variable set at child class.
     */
    String includeFileFlag = null;
    String entryPointFlag = null;
    String subSystemFlag = null;
    String mapFlag = null;
    String pdbFlag = null;
    String outputFileFlag = null;
    String includePathDelimiter = null;
    
    /*
     * get lib string if Vendor = "gcc", it should respectively aadd "-(" and ")-" 
     * at library set before and end. This value set at userDefineCompiler class.
     */
    Vector<String> libSetList = new Vector<String>();
    Vector<String> fileList = new Vector<String>();
    public void command(CCTask cctask, UserDefineDef userdefine){
        File workdir;
        File outdir;
        Project project = cctask.getProject();
        if(userdefine.getWorkdir() == null) {
            workdir = new File(".");
        }
        else {
            workdir = userdefine.getWorkdir();
        } 
        
        /*
         * generate cmdline= command + args + includepath + endargs + outfile
         */ 
        Vector args = new Vector();
        Vector argsWithoutSpace = new Vector();
        Vector endargs = new Vector();
        Vector endargsWithoutSpace = new Vector();
        Vector includePath = new Vector();
        
        /*
         * Generate cmdline = command        + 
         *                    general args   + 
         *                    outputflag     + outputfile
         *                    subsystemFlag  + subsystemValue  +
         *                    includeFlag    + includeFile     +
         *                    includeFileincludpath            + 
         *                    entryPointFlag + entryPointValue +
         *                    mapFlag        + mapValue        +
         *                    pdbFlag        + pdbValue        +
         *                    endargs                          + 
         *                   
         * 
         */
        /*
         * get Args.
         */
        CommandLineArgument[] argument = userdefine.getActiveProcessorArgs();
        for (int j = 0; j < argument.length; j++) {
            if (argument[j].getLocation() == 0) {
                args.addElement(argument[j].getValue());
            } else {
                endargs.addElement(argument[j].getValue());
            }
        }
        /*
         * get include path.
         */
        String[] incPath = userdefine.getActiveIncludePaths();
        for (int j = 0; j < incPath.length; j++) {
            if(incPath[j].indexOf(' ') >= 0) {
                includePath.addElement( includePathDelimiter + incPath[j]);
                //includePath.addElement( includePathDelimiter + "\"" + incPath[j] + "\"");
            }
            else {
                includePath.addElement( includePathDelimiter + incPath[j]);
            }
        }
        /*
         * Remove space in args and endargs.
         */
        for ( int i=0; i < args.size(); i++) {
            String str = (String)args.get(i);
            StringTokenizer st = new StringTokenizer(str);
            while(st.hasMoreTokens()) {
                argsWithoutSpace.addElement(st.nextToken());
            }
        }
        for ( int i=0; i < endargs.size(); i++) {
            String str = (String)endargs.get(i);
            StringTokenizer st = new StringTokenizer(str);
            while(st.hasMoreTokens()) {
                endargsWithoutSpace.addElement(st.nextToken());
            }
        }
        
        int cmdLen = 0;
        if(userdefine.getOutdir() == null) {
            outdir = new File(".");
            /*
             * command + args + endargs + includepath + sourcefile
             */
            cmdLen = 1 + argsWithoutSpace.size() + endargsWithoutSpace.size() + includePath.size() + 1;
        }
        else {
            outdir = userdefine.getOutdir();
            /*
             * command + args + endargs + includepath + sourcefile + outfile
             */
            cmdLen = 1 + argsWithoutSpace.size() + endargsWithoutSpace.size() + includePath.size() + 2;
        }
        if (includeFileFlag != null && includeFileFlag.trim().length() > 0){
            cmdLen++;
        }
        if (entryPointFlag != null && entryPointFlag.trim().length() > 0){
            cmdLen++;
        }
        if (subSystemFlag != null && subSystemFlag.trim().length() > 0){
            cmdLen++;
        }
        if (mapFlag != null && mapFlag.trim().length() > 0){
            cmdLen++;
        }
        if (pdbFlag != null && pdbFlag.trim().length() > 0){
            cmdLen++;
        }
        if (libSetList != null && libSetList.size() > 0){
            cmdLen = cmdLen + libSetList.size();
        }
        if (fileList != null){
            cmdLen = cmdLen + fileList.size();
        }
        /*
         * In gcc the "cr" flag should follow space then add outputfile name, otherwise
         * it will pop error. 
         */
        if (outputFileFlag != null && outputFileFlag.trim().length() > 0){
            if (outputFileFlag.trim().equalsIgnoreCase("-cr")){
                cmdLen = cmdLen + 2;
            }else {
                cmdLen++;
            }
            
        }
        /*
         *  for every source file
         *  if file is header file, just skip it (add later)
         */
        Vector srcSets = userdefine.getSrcSets();
        if (srcSets.size() == 0) {
            String[] cmd = new String[cmdLen - 1];
            int index = 0;
            cmd[index++] = this.command;
            
           
            
            Iterator iter = argsWithoutSpace.iterator();
            while (iter.hasNext()) {
                cmd[index++] = project.replaceProperties((String)iter.next());
                //cmd[index++] = (String)iter.next();
            }
            
            iter = endargsWithoutSpace.iterator();
            while (iter.hasNext()) {
                cmd[index++] = (String)iter.next();
            }
            
            /*
             * "OutputFlag + outputFile" as first option follow command.exe.
             */
            if (outputFileFlag != null && outputFileFlag.trim().length() > 0){
                if (outputFileFlag.trim().equalsIgnoreCase("-cr")){
                    cmd[index++] = outputFileFlag;
                    cmd[index++] = userdefine.getOutputFile();
                }else {
                    cmd[index++] =  outputFileFlag + userdefine.getOutputFile();
                }
            }
            
            /*
             * Add fileList to cmd 
             */
            if (fileList != null && fileList.size()> 0){
                for (int i = 0; i < fileList.size(); i++){
                    cmd[index++] = fileList.get(i);
                }
            }
            
            if (subSystemFlag != null && subSystemFlag.trim().length() > 0){
                cmd[index++] = subSystemFlag + userdefine.getSubSystemvalue();
            }
            if (includeFileFlag != null && includeFileFlag.trim().length() > 0){
                cmd[index++] = includeFileFlag + userdefine.getIncludeFile();
            }
         
            iter = includePath.iterator();
            while (iter.hasNext()) {
                cmd[index++] = (String)iter.next();
            }
            
            if (entryPointFlag != null && entryPointFlag.trim().length() > 0){
                //
                // If GCC link use __ModuleEntrypoint instead of _ModuleEntryPoint;
                //
                if (entryPointFlag.equalsIgnoreCase("-e")){
                    cmd[index++] = entryPointFlag + "_" + userdefine.getEntryPointvalue();
                } else {
                    cmd[index++] = entryPointFlag + userdefine.getEntryPointvalue();
                }
                
            }
            if (mapFlag != null && mapFlag.trim().length() > 0){
                cmd[index++] = mapFlag + userdefine.getMapvalue();
            }
            if (pdbFlag != null && pdbFlag.trim().length() > 0){
                cmd[index++] = pdbFlag + userdefine.getPdbvalue();
            }

            if (userdefine.getOutdir() != null){
                // will add code to generate outfile name and flag
                cmd[index++] = "/nologo";
            }
            
            if (libSetList != null && libSetList.size() > 0){
                for (int i = 0; i < libSetList.size(); i++){
                    cmd[index++] = libSetList.get(i);
                }
            }

            // execute the command
            int retval = runCommand(cctask, workdir, cmd);
            // if with monitor, add more code
            if (retval != 0) {
               throw new BuildException(this.command
                       + " failed with return code " + retval,
                       cctask.getLocation());
            }
        }
        
        //
        // if have source file append source file in command land.
        //
        for (int i = 0; i < srcSets.size(); i++) {
            ConditionalFileSet srcSet = (ConditionalFileSet) srcSets
                    .elementAt(i);
            if (srcSet.isActive()) {
                // Find matching source files
                DirectoryScanner scanner = srcSet.getDirectoryScanner(project);
                // Check each source file - see if it needs compilation
                String[] fileNames = scanner.getIncludedFiles();
                for (int j = 0; j < fileNames.length; j++){
                    String[] cmd = new String[cmdLen];
                    int index = 0;
                    cmd[index++] = this.command;
                    
                    
                    
                    Iterator iter = argsWithoutSpace.iterator();
                    while (iter.hasNext()) {
                        cmd[index++] = (String)iter.next();
                    }
                    
                    iter = endargsWithoutSpace.iterator();
                    while (iter.hasNext()) {
                        cmd[index++] = (String)iter.next();
                    }
                    
                    /*
                     * Add outputFileFlag and output file to cmd
                     */
                    if (outputFileFlag != null && outputFileFlag.length()> 0){
                        if (outputFileFlag.trim().equalsIgnoreCase("-cr")){
                            cmd[index++] = outputFileFlag;
                            cmd[index++] = userdefine.getOutputFile();
                        }else {
                            cmd[index++] =  outputFileFlag + userdefine.getOutputFile();
                        }
                    }
                    
                    /*
                     * Add fileList to cmd 
                     */
                    if (fileList != null && fileList.size()> 0){
                        for (int s = 0; s < fileList.size(); s++){
                            cmd[index++] = fileList.get(s);
                        }
                    }
                    if (subSystemFlag != null && subSystemFlag.length()> 0){
                        cmd[index++] = subSystemFlag + userdefine.getSubSystemvalue();
                    }
                    if (includeFileFlag != null && includeFileFlag.length()> 0){
                        cmd[index++] = includeFileFlag + userdefine.getIncludeFile();
                    }
                    
                    iter = includePath.iterator();
                    while (iter.hasNext()) {
                        cmd[index++] = (String)iter.next();
                    }
                    if (userdefine.getOutdir() != null){
                        // will add code to generate outfile name and flag
                        cmd[index++] = "/nologo";
                    }
                    
                    if (entryPointFlag != null && entryPointFlag.length()> 0){
                        cmd[index++] = entryPointFlag + userdefine.getEntryPointvalue();
                    }
                    if (mapFlag != null && mapFlag.length() > 0){
                        cmd[index++] = mapFlag + userdefine.getMapvalue();
                    }
                    if (pdbFlag != null && pdbFlag.length() > 0){
                        cmd[index++] = pdbFlag + userdefine.getPdbvalue();
                    }
                    
                    if (libSetList != null && libSetList.size() > 0){
                        for (int k = 0; k < libSetList.size(); k++){
                            cmd[index++] = libSetList.get(k);
                        }
                    }
                    
                    // execute the command
                    cmd[index++] = scanner.getBasedir() + "/" + fileNames[j];
                    for (int k = 0; k < cmd.length; k++){
                    }
                    int retval = runCommand(cctask, workdir, cmd);
                    // if with monitor, add more code
                    if (retval != 0) {
                       throw new BuildException(this.command
                               + " failed with return code " + retval,
                               cctask.getLocation());
                    }
                }
            }
        }
    }
    
    protected int runCommand(CCTask task, File workingDir, String[] cmdline)
                    throws BuildException {
        return CUtil.runCommand(task, workingDir, cmdline, false, null);

    }

    protected String getInputFileArgument(File outputDir, String filename,
                    int index) {
        //
        // if there is an embedded space,
        // must enclose in quotes
        if (filename.indexOf(' ') >= 0) {
            StringBuffer buf = new StringBuffer("\"");
            buf.append(filename);
            buf.append("\"");
            return buf.toString();
        }
        return filename;
    }

}
