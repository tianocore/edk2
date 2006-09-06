/** @file FrameworkBuildTask.java
  
  The file is ANT task to find MSA or FPD file and build them. 
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php

 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 **/
package org.tianocore.build;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.Hashtable;
import java.util.Iterator;
import java.util.LinkedHashSet;
import java.util.Map;
import java.util.Set;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Task;
import org.tianocore.build.exception.AutoGenException;
import org.tianocore.build.exception.GenBuildException;
import org.tianocore.build.exception.PcdAutogenException;
import org.tianocore.build.exception.PlatformPcdPreprocessBuildException;
import org.tianocore.build.fpd.FpdParserForThread;
import org.tianocore.build.fpd.FpdParserTask;
import org.tianocore.build.global.GenBuildLogger;
import org.tianocore.build.global.GlobalData;
import org.tianocore.build.toolchain.ConfigReader;
import org.tianocore.build.toolchain.ToolChainInfo;
import org.tianocore.common.definitions.ToolDefinitions;
import org.tianocore.common.exception.EdkException;
import org.tianocore.common.logger.EdkLog;

/**
  <p>
  <code>FrameworkBuildTask</code> is an Ant task. The main function is finding
  and processing a FPD or MSA file, then building a platform or stand-alone 
  module. 
  
  <p>
  The task search current directory and find out all MSA and FPD files by file
  extension. Base on ACTIVE_PLATFORM policy, decide to build a platform or a
  stand-alone module. The ACTIVE_PLATFORM policy is: 
  
  <pre>
  1. More than one MSA files, report error; 
  2. Only one MSA file, but ACTIVE_PLATFORM is not specified, report error;
  3. Only one MSA file, and ACTIVE_PLATFORM is also specified, build this module;
  4. No MSA file, and ACTIVE_PLATFORM is specified, build the active platform;
  5. No MSA file, no ACTIVE_PLATFORM, and no FPD file, report error;
  6. No MSA file, no ACTIVE_PLATFORM, and only one FPD file, build the platform;
  7. No MSA file, no ACTIVE_PLATFORM, and more than one FPD files, list all platform
  and let user choose one. 
  </pre>
  
  <p>
  Framework build task also parse target file [${WORKSPACE_DIR}/Tools/Conf/target.txt].
  And load all system environment variables to Ant properties.  
  
  <p>
  The usage for this task is : 
  
  <pre>
  &lt;FrameworkBuild type="cleanall" /&gt;
  </pre>
  
  @since GenBuild 1.0
**/
public class FrameworkBuildTask extends Task{

    private Set<File> buildFiles = new LinkedHashSet<File>();
    
    private Set<File> fpdFiles = new LinkedHashSet<File>();
    
    private Set<File> msaFiles = new LinkedHashSet<File>();
    
    //
    // This is only for none-multi-thread build to reduce overriding message
    //
    public static Hashtable<String, String> originalProperties = new Hashtable<String, String>();
    
    String toolsDefFilename = ToolDefinitions.DEFAULT_TOOLS_DEF_FILE_PATH;
    
    String targetFilename = ToolDefinitions.TARGET_FILE_PATH;
    
    String dbFilename = ToolDefinitions.FRAMEWORK_DATABASE_FILE_PATH;
    
    String activePlatform = null;

    ///
    /// The flag to present current is multi-thread enabled
    ///
    public static boolean multithread = false;

    ///
    /// The concurrent thread number
    ///
    public static int MAX_CONCURRENT_THREAD_NUMBER = 1;

    ///
    /// there are three type: all (build), clean and cleanall
    ///
    private String type = "all";
    
    public void execute() throws BuildException {
        try {
            processFrameworkBuild();
        } catch (PcdAutogenException e) {
            //
            // Add more logic process here
            //
            throw new BuildException(e.getMessage());
        } catch (AutoGenException e) {
            //
            // Add more logic process here
            //
            throw new BuildException(e.getMessage());
        } catch (PlatformPcdPreprocessBuildException e) {
            //
            // Add more logic process here
            //
            throw new BuildException(e.getMessage());
        } catch (GenBuildException e) {
            //
            // Add more logic process here
            //
            throw new BuildException(e.getMessage());
        } catch (EdkException e) {
            //
            // Add more logic process here
            //
            throw new BuildException(e.getMessage());
        }
    }
    
    private void processFrameworkBuild() throws EdkException, GenBuildException, AutoGenException, PcdAutogenException, PlatformPcdPreprocessBuildException {
        //
        // set Logger
        //
        GenBuildLogger logger = new GenBuildLogger(getProject());
        EdkLog.setLogLevel(getProject().getProperty("env.LOGLEVEL"));
        EdkLog.setLogger(logger);

        //
        // Seach build.xml -> .FPD -> .MSA file
        //
        try {
            //
            // Gen Current Working Directory
            //
            File dummyFile = new File(".");
            File cwd = dummyFile.getCanonicalFile();
            File[] files = cwd.listFiles();
            for (int i = 0; i < files.length; i++) {
                if (files[i].isFile()) {
                    if (files[i].getName().equalsIgnoreCase("build.xml")) {
                        //
                        // First, search build.xml, if found, ANT call it
                        //
                        buildFiles.add(files[i]);

                    } else if (files[i].getName().endsWith(ToolDefinitions.FPD_EXTENSION)) {
                        //
                        // Second, search FPD file, if found, build it
                        //
                        fpdFiles.add(files[i]);
                    } else if (files[i].getName().endsWith(ToolDefinitions.MSA_EXTENSION)) {
                        //
                        // Third, search MSA file, if found, build it
                        //
                        msaFiles.add(files[i]);
                    }
                }
            }
        } catch (IOException e) {
            throw new BuildException(e.getMessage());
        }
        
        //
        // Deal with all environment variable (Add them to properties)
        //
        backupSystemProperties();
        
        //
        // Read target.txt file
        //
        readTargetFile();

        //
        // Global Data initialization
        //
        File workspacePath = new File(getProject().getProperty("WORKSPACE"));
        getProject().setProperty("WORKSPACE_DIR", workspacePath.getPath().replaceAll("(\\\\)", "/"));
        GlobalData.initInfo(dbFilename, workspacePath.getPath(), toolsDefFilename);
        
        //
        // If find MSA file and ACTIVE_PLATFORM is set, build the module; 
        // else fail build. 
        // If without MSA file, and ACTIVE_PLATFORM is set, build the ACTIVE_PLATFORM. 
        // If ACTIVE_PLATFORM is not set, and only find one FPD file, build the platform; 
        // If find more than one FPD files, let user select one. 
        //
        File buildFile = null;
        if (msaFiles.size() > 1) {
            throw new BuildException("Having more than one MSA file in a directory is not allowed!");
        } else if (msaFiles.size() == 1 && activePlatform == null) {
            throw new BuildException("If trying to build a single module, please set ACTIVE_PLATFORM in file [" + targetFilename + "]. ");
        } else if (msaFiles.size() == 1 && activePlatform != null) {
            //
            // Build the single module
            //
            buildFile = msaFiles.toArray(new File[1])[0];
        } else if (activePlatform != null) {
            buildFile = new File(GlobalData.getWorkspacePath() + File.separatorChar + activePlatform);
        } else if (fpdFiles.size() == 1) {
            buildFile = fpdFiles.toArray(new File[1])[0];
        } else if (fpdFiles.size() > 1) {
            buildFile = intercommuniteWithUser();
        }
        //
        // If there is no build files or FPD files or MSA files, stop build
        //
        else {
            throw new BuildException("Can't find any FPD or MSA files in the current directory. ");
        }

        //
        // Build every FPD files (PLATFORM build)
        //
        if (buildFile.getName().endsWith(ToolDefinitions.FPD_EXTENSION)) {
            EdkLog.log(this, "Processing the FPD file [" + buildFile.getPath() + "] ..>> ");
            //
            // Iff for platform build will enable the multi-thread if set in target.txt
            //
            if (multithread && type.equalsIgnoreCase("all")) {
                EdkLog.log(this, "Multi-thread build is enabled. ");
                FpdParserForThread fpdParserForThread = new FpdParserForThread();
                fpdParserForThread.setType(type);
                fpdParserForThread.setProject(getProject());
                fpdParserForThread.setFpdFile(buildFile);
                fpdParserForThread.perform();
                return ;
            }
            
            FpdParserTask fpdParserTask = new FpdParserTask();
            fpdParserTask.setType(type);
            fpdParserTask.setProject(getProject());
            fpdParserTask.setFpdFile(buildFile);
            fpdParserTask.perform();
            
            //
            // If cleanall delete the Platform_build.xml
            //
            if (type.compareTo("cleanall") == 0) {
                File platformBuildFile = 
                    new File(getProject().getProperty("PLATFORM_DIR") 
                                    + File.separatorChar 
                                    + getProject().getProperty("PLATFORM") 
                                    + "_build.xml");
                platformBuildFile.deleteOnExit();
            }
        }
        
        //
        // Build every MSA files (SINGLE MODULE BUILD)
        //
        else if (buildFile.getName().endsWith(ToolDefinitions.MSA_EXTENSION)) {
            File tmpFile = new File(GlobalData.getWorkspacePath() + File.separatorChar + activePlatform);
            EdkLog.log(this, "Using the FPD file [" + tmpFile.getPath() + "] for the active platform. ");
            EdkLog.log(this, "Processing the MSA file [" + buildFile.getPath() + "] ..>> ");
            GenBuildTask genBuildTask = new GenBuildTask();
            genBuildTask.setSingleModuleBuild(true);
            genBuildTask.setType(type);
            getProject().setProperty("PLATFORM_FILE", activePlatform);
            if( !multithread) {
                originalProperties.put("PLATFORM_FILE", activePlatform);
            }
            genBuildTask.setProject(getProject());
            genBuildTask.setMsaFile(buildFile);
            genBuildTask.perform();
        }
    }
    
    /**
      Transfer system environment variables to ANT properties. If system variable 
      already exiests in ANT properties, skip it.
      
    **/
    private void backupSystemProperties() {
        Map<String, String> sysProperties = System.getenv();
        Iterator<String> iter = sysProperties.keySet().iterator();
        while (iter.hasNext()) {
            String name = iter.next();
            
            //
            // If system environment variable is not in ANT properties, add it
            //
            if (getProject().getProperty(name) == null) {
                getProject().setProperty(name, sysProperties.get(name));
            }
        }
        
        Hashtable allProperties = getProject().getProperties();
        Iterator piter = allProperties.keySet().iterator();
        while (piter.hasNext()) {
            String name = (String)piter.next();
            originalProperties.put(new String(name), new String((String)allProperties.get(name)));
        }
    }

    private File intercommuniteWithUser(){
        File file = null;
        if (fpdFiles.size() > 1) {
            File[] allFiles = new File[fpdFiles.size()];
            int index = 0;
            Iterator<File> iter = fpdFiles.iterator();
            while (iter.hasNext()) {
                allFiles[index] = iter.next();
                index++;
            }

            EdkLog.log(this, "Finding " + allFiles.length + " FPD files: ");
            for (int i = 0; i < allFiles.length; i++) {
                System.out.println("[" + (i + 1) + "]: " + allFiles[i].getName());
            }
            
            boolean flag = true;
            EdkLog.log(this, "Please select one of the following FPD files to build:[1] ");
            do{
                BufferedReader br = new BufferedReader(new InputStreamReader(System.in));
                try {
                     String str = br.readLine();
                     if (str.trim().length() == 0) {
                         file = allFiles[0];
                         flag = false;
                         continue ;
                     }
                     int indexSelect = Integer.parseInt(str);
                     if (indexSelect <=0 || indexSelect > allFiles.length) {
                         EdkLog.log(this, "Please enter a number between [1.." + allFiles.length + "]:[1] ");
                         continue ;
                     } else {
                         file = allFiles[indexSelect - 1];
                         flag = false;
                         continue ;
                     }
                } catch (Exception e) {
                    EdkLog.log(this, "Please enter a valid number:[1] ");
                    flag = true;
                }
            } while (flag);
        } else if (fpdFiles.size() == 1) {
            file = fpdFiles.toArray(new File[1])[0];
        }
        return file;
    }
    
    
    public void setType(String type) {
        if (type.equalsIgnoreCase("clean") || type.equalsIgnoreCase("cleanall")) {
            this.type = type.toLowerCase();
        } else {
            this.type = "all";
        }
    }
    
    private void readTargetFile() throws EdkException{
        String targetFile = getProject().getProperty("WORKSPACE_DIR") + File.separatorChar + targetFilename;
        
        String[][] targetFileInfo = ConfigReader.parse(targetFile);
        
        //
        // Get ToolChain Info from target.txt
        //
        ToolChainInfo envToolChainInfo = new ToolChainInfo(); 
        String str = getValue(ToolDefinitions.TARGET_KEY_TARGET, targetFileInfo);
        if (str == null || str.trim().equals("")) {
            envToolChainInfo.addTargets("*");
        } else {
            envToolChainInfo.addTargets(str);
        }
        str = getValue(ToolDefinitions.TARGET_KEY_TOOLCHAIN, targetFileInfo);
        if (str == null || str.trim().equals("")) {
            envToolChainInfo.addTagnames("*");
        } else {
            envToolChainInfo.addTagnames(str);
        }
        str = getValue(ToolDefinitions.TARGET_KEY_ARCH, targetFileInfo);
        if (str == null || str.trim().equals("")) {
            envToolChainInfo.addArchs("*");
        } else {
            envToolChainInfo.addArchs(str);
        }
        GlobalData.setToolChainEnvInfo(envToolChainInfo);
        
        str = getValue(ToolDefinitions.TARGET_KEY_TOOLS_DEF, targetFileInfo);
        if (str != null && str.trim().length() > 0) {
            toolsDefFilename = str;
        }
        
        str = getValue(ToolDefinitions.TARGET_KEY_ACTIVE_PLATFORM, targetFileInfo);
        if (str != null && ! str.trim().equals("")) {
            if ( ! str.endsWith(".fpd")) {
                throw new BuildException("FPD file's extension must be \"" + ToolDefinitions.FPD_EXTENSION + "\"!");
            }
            activePlatform = str;
        }
        
        str = getValue(ToolDefinitions.TARGET_KEY_MULTIPLE_THREAD, targetFileInfo);
        if (str != null && str.trim().equalsIgnoreCase("Enable")) {
            multithread = true;
        }
            
        str = getValue(ToolDefinitions.TARGET_KEY_MAX_CONCURRENT_THREAD_NUMBER, targetFileInfo);
        if (str != null ) {
            try {
                int threadNum = Integer.parseInt(str);
                if (threadNum > 0) {
                    MAX_CONCURRENT_THREAD_NUMBER = threadNum;
                }
            } catch (Exception enuma) {
            }
        }
    }
    
    private String getValue(String key, String[][] map) {
        for (int i = 0; i < map[0].length; i++){
            if (key.equalsIgnoreCase(map[0][i])) {
                return map[1][i];
            }
        }
        return null;
    }
}
