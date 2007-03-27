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

import java.io.File;
import java.io.IOException;
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
  7. No MSA file, no ACTIVE_PLATFORM, and more than one FPD files, Report Error!
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

    private Set<File> fpdFiles = new LinkedHashSet<File>();
    
    private Set<File> msaFiles = new LinkedHashSet<File>();
    
    ///
    /// This is only for none-multi-thread build to reduce overriding message
    ///
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
    public static int MAX_CONCURRENT_THREAD_NUMBER = 2;

    ///
    /// there are three type: all (build), clean and cleanall
    ///
    private String type = "all";
    
    public void execute() throws BuildException {
        //
        // set Logger
        //
        GenBuildLogger logger = new GenBuildLogger(getProject());
        EdkLog.setLogLevel(EdkLog.EDK_DEBUG);
        EdkLog.setLogLevel(getProject().getProperty("env.LOGLEVEL"));
        EdkLog.setLogger(logger);
        
        try {
            processFrameworkBuild();
        }catch (BuildException e) {
            //
            // Add more logic process here
            //
            BuildException buildException = new BuildException(e.getMessage());
            buildException.setStackTrace(e.getStackTrace());
            throw buildException;
        } catch (PcdAutogenException e) {
            //
            // Add more logic process here
            //
            BuildException buildException = new BuildException(e.getMessage());
            buildException.setStackTrace(e.getStackTrace());
            throw buildException;
        } catch (AutoGenException e) {
            //
            // Add more logic process here
            //
            BuildException buildException = new BuildException(e.getMessage());
            buildException.setStackTrace(e.getStackTrace());
            throw buildException;
        } catch (PlatformPcdPreprocessBuildException e) {
            //
            // Add more logic process here
            //
            BuildException buildException = new BuildException(e.getMessage());
            buildException.setStackTrace(e.getStackTrace());
            throw buildException;
        } catch (GenBuildException e) {
            //
            // Add more logic process here
            //
            BuildException buildException = new BuildException(e.getMessage());
            buildException.setStackTrace(e.getStackTrace());
            throw buildException;
        } catch (EdkException e) {
            //
            // Add more logic process here
            //
            BuildException buildException = new BuildException(e.getMessage());
            buildException.setStackTrace(e.getStackTrace());
            throw buildException;
        }
    }
    
    private void processFrameworkBuild() throws EdkException, GenBuildException, AutoGenException, PcdAutogenException, PlatformPcdPreprocessBuildException {
        try {
            //
            // Get current working dir
            //
            File dummyFile = new File(".");
            File cwd = dummyFile.getCanonicalFile();
            File[] files = cwd.listFiles();
            
            //
            // Scan current dir, and find out all .FPD and .MSA files
            //
            for (int i = 0; i < files.length; i++) {
                if (files[i].isFile()) {
                    if (files[i].getName().endsWith(ToolDefinitions.FPD_EXTENSION)) {
                        //
                        // Found FPD file
                        //
                        fpdFiles.add(files[i]);
                    } else if (files[i].getName().endsWith(ToolDefinitions.MSA_EXTENSION)) {
                        //
                        // Found MSA file
                        //
                        msaFiles.add(files[i]);
                    }
                }
            }
        } catch (IOException ex) {
            BuildException buildException = new BuildException("Scanning current directory error. \n" + ex.getMessage());
            buildException.setStackTrace(ex.getStackTrace());
            throw buildException;
        }
        
        //
        // Import all system environment variables to ANT properties
        //
        importSystemEnvVariables();
        
        //
        // Read target.txt file
        //
        readTargetFile();

        //
        // Global Data initialization
        //
        File workspacePath = new File(getProject().getProperty("WORKSPACE"));
        getProject().setProperty("WORKSPACE_DIR", workspacePath.getPath().replaceAll("(\\\\)", "/"));
        GlobalData.initInfo(getProject(), dbFilename, workspacePath.getPath(), toolsDefFilename);
        
        //
        // If find MSA file and ACTIVE_PLATFORM is set, build the module; 
        // else fail build. 
        // If without MSA file, and ACTIVE_PLATFORM is set, build the ACTIVE_PLATFORM. 
        // If ACTIVE_PLATFORM is not set, and only find one FPD file, build the platform; 
        // If find more than one FPD files, report error.  
        //
        File buildFile = null;
        if (msaFiles.size() > 0) {
            if (activePlatform == null) {
                throw new BuildException("If trying to build a single module, please set ACTIVE_PLATFORM in file [" + targetFilename + "]. ");
            }
            //
            // Build the single module
            //
            buildFile = msaFiles.toArray(new File[1])[0];
        } else if (activePlatform != null) {
            buildFile = new File(GlobalData.getWorkspacePath() + File.separatorChar + activePlatform);
        } else if (fpdFiles.size() == 1) {
            buildFile = fpdFiles.toArray(new File[1])[0];
        } else if (fpdFiles.size() > 1) {
            throw new BuildException("Found " + fpdFiles.size() + " FPD files in current dir. ");
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
                    new File(getProject().getProperty("BUILD_DIR") 
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
            if (multithread) {
                EdkLog.log(this, EdkLog.EDK_WARNING, "Multi-Thead do not take effect on Stand-Alone (Single) module build. ");
                multithread = false;
            }
            File tmpFile = new File(GlobalData.getWorkspacePath() + File.separatorChar + activePlatform);
            EdkLog.log(this, "Using the FPD file [" + tmpFile.getPath() + "] for the active platform. ");

            File[] moduleFiles = msaFiles.toArray(new File[msaFiles.size()]);
            for (int i = 0; i < moduleFiles.length; ++i) {
                EdkLog.log(this, "Processing the MSA file [" + moduleFiles[i].getPath() + "] ..>> ");
                GenBuildTask genBuildTask = new GenBuildTask();
                genBuildTask.setSingleModuleBuild(true);
                genBuildTask.setType(type);
                getProject().setProperty("PLATFORM_FILE", activePlatform);
                if( !multithread) {
                    originalProperties.put("PLATFORM_FILE", activePlatform);
                }
                genBuildTask.setProject(getProject());
                genBuildTask.setMsaFile(moduleFiles[i]);
                genBuildTask.perform();
            }
        }
    }
    
    /**
      Import system environment variables to ANT properties. If system variable 
      already exiests in ANT properties, skip it.
      
    **/
    private void importSystemEnvVariables() {
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

    public void setType(String type) {
        this.type = type.toLowerCase();
    }
    
    private void readTargetFile() throws EdkException{
        String targetFile = getProject().getProperty("WORKSPACE_DIR") + File.separatorChar + targetFilename;
        
        String[][] targetFileInfo = ConfigReader.parse(getProject(), targetFile);
        
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
        //
        // Need to check the # of threads iff multithread is enabled.
        //
        if ((multithread) && (str != null )) {
            try {
                int threadNum = Integer.parseInt(str);
                if (threadNum > 0) {
                    MAX_CONCURRENT_THREAD_NUMBER = threadNum;
                }
            } catch (Exception ex) {
                //
                // Give a warning message, and keep the default value
                //
                EdkLog.log(this, EdkLog.EDK_WARNING, "Incorrent number specified for MAX_CONCURRENT_THREAD_NUMBER in file [" + targetFilename + "]");
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
