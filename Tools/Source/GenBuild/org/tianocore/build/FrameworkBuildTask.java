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
import java.io.InputStreamReader;
import java.util.Iterator;
import java.util.LinkedHashSet;
import java.util.Map;
import java.util.Set;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Task;
import org.tianocore.build.fpd.FpdParserTask;
import org.tianocore.build.global.GlobalData;
import org.tianocore.build.toolchain.ConfigReader;
import org.tianocore.build.toolchain.ToolChainConfig;
import org.tianocore.build.toolchain.ToolChainInfo;

public class FrameworkBuildTask extends Task{

    private Set<File> buildFiles = new LinkedHashSet<File>();
    
    private Set<File> fpdFiles = new LinkedHashSet<File>();
    
    private Set<File> msaFiles = new LinkedHashSet<File>();
    
    String toolsDefFilename = "Tools" + File.separatorChar + "Conf" + File.separatorChar + "tools_def.txt";
    
    String targetFilename = "target.txt";
    
    String activePlatform = null;
    
    ///
    /// there are three type: all (build), clean and cleanall
    ///
    private String type = "all";
    
    public void execute() throws BuildException {
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

                    } else if (files[i].getName().endsWith(".fpd")) {
                        //
                        // Second, search FPD file, if found, build it
                        //
                        fpdFiles.add(files[i]);
                    } else if (files[i].getName().endsWith(".msa")) {
                        //
                        // Third, search MSA file, if found, build it
                        //
                        msaFiles.add(files[i]);
                    }
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
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
        GlobalData.initInfo("Tools" + File.separatorChar + "Conf" + File.separatorChar + "FrameworkDatabase.db",
                            getProject().getProperty("WORKSPACE_DIR"), toolsDefFilename);
        

        
        //
        // If find MSA file and ACTIVE_PLATFORM is set, build the module; 
        // else fail build. 
        // If without MSA file, and ACTIVE_PLATFORM is set, build the ACTIVE_PLATFORM. 
        // If ACTIVE_PLATFORM is not set, and only find one FPD file, build the platform; 
        // If find more than one FPD files, let user select one. 
        //
        File buildFile = null;
        if (msaFiles.size() > 1) {
            throw new BuildException("More than one MSA file under current directory. It is not allowd. ");
        }
        else if (msaFiles.size() == 1 && activePlatform == null) {
            throw new BuildException("If try to build a single module, please set ACTIVE_PLATFORM in file [Tool/Conf/target.txt]. ");
        }
        else if (msaFiles.size() == 1 && activePlatform != null) {
            //
            // Build the single module
            //
            buildFile = msaFiles.toArray(new File[1])[0];
        }
        else if (activePlatform != null) {
            buildFile = new File(GlobalData.getWorkspacePath() + File.separatorChar + activePlatform);
        }
        else if (fpdFiles.size() == 1) {
            buildFile = fpdFiles.toArray(new File[1])[0];
        }
        else if (fpdFiles.size() > 1) {
            buildFile = intercommuniteWithUser();
        }
        //
        // If there is no build files or FPD files or MSA files, stop build
        //
        else {
            throw new BuildException("Can't find any FPD files or MSA files in current directory. ");
        }

        //
        // Build every FPD files (PLATFORM build)
        //
        if (buildFile.getName().endsWith(".fpd")) {
            System.out.println("Start to build FPD file [" + buildFile.getPath() + "] ..>> ");
            FpdParserTask fpdParserTask = new FpdParserTask();
            fpdParserTask.setType(type);
            fpdParserTask.setProject(getProject());
            fpdParserTask.setFpdFile(buildFile);
            fpdParserTask.execute();
        }
        
        //
        // Build every MSA files (SINGLE MODULE BUILD)
        //
        else if (buildFile.getName().endsWith(".msa")) {
            System.out.println("Start to build MSA file [" + buildFile.getPath() + "] ..>> ");
            GenBuildTask genBuildTask = new GenBuildTask();
            genBuildTask.setSingleModuleBuild(true);
            genBuildTask.setType(type);
            getProject().setProperty("PLATFORM_FILE", activePlatform);
            genBuildTask.setProject(getProject());
            genBuildTask.setMsaFile(buildFile);
            genBuildTask.execute();
        }
    }
    
    /**
      Transfer system environment variables to ANT properties. If system variable 
      already exiests in ANT properties, skip it.
      
    **/
    private void backupSystemProperties() {
        Map<String, String> sysProperties = System.getenv();
        Set<String> keys = sysProperties.keySet();
        Iterator<String> iter = keys.iterator();
        while (iter.hasNext()) {
            String name = iter.next();
            
            //
            // If system environment variable is not in ANT properties, add it
            //
            if (getProject().getProperty(name) == null) {
                getProject().setProperty(name, sysProperties.get(name));
            }
        }
    }

    private File intercommuniteWithUser(){
        File file = null;
        if (fpdFiles.size() + msaFiles.size() > 1) {
            File[] allFiles = new File[fpdFiles.size() + msaFiles.size()];
            int index = 0;
            Iterator<File> iter = fpdFiles.iterator();
            while (iter.hasNext()) {
                allFiles[index] = iter.next();
                index++;
            }
            iter = msaFiles.iterator();
            while (iter.hasNext()) {
                allFiles[index] = iter.next();
                index++;
            }
            System.out.println("Find " + allFiles.length + " FPD and MSA files: ");
            for (int i = 0; i < allFiles.length; i++) {
                System.out.println("[" + (i + 1) + "]: " + allFiles[i].getName());
            }
            
            boolean flag = true;
            System.out.print("Please select one file to build:[1] ");
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
                         System.out.print("Please enter a number between [1.." + allFiles.length + "]:[1] ");
                         continue ;
                     } else {
                         file = allFiles[indexSelect - 1];
                         flag = false;
                         continue ;
                     }
                } catch (Exception e) {
                    System.out.print("Please enter a valid number:[1] ");
                    flag = true;
                }
            } while (flag);
        }
        else if (fpdFiles.size() == 1) {
            file = fpdFiles.toArray(new File[1])[0];
        }
        else if (msaFiles.size() == 1) {
            file = msaFiles.toArray(new File[1])[0];
        }
        return file;
    }
    
    
    public void setType(String type) {
        if (type.equalsIgnoreCase("clean") || type.equalsIgnoreCase("cleanall")) {
            this.type = type.toLowerCase();
        }
        else {
            this.type = "all";
        }
    }
    
    private void readTargetFile(){
        try {
            String[][] targetFileInfo = ConfigReader.parse(getProject().getProperty("WORKSPACE_DIR"), "Tools" + File.separatorChar + "Conf" + File.separatorChar + targetFilename);
            
            //
            // Get ToolChain Info from target.txt
            //
            ToolChainInfo envToolChainInfo = new ToolChainInfo(); 
            String str = getValue("TARGET", targetFileInfo);
            if (str == null || str.trim().equals("")) {
                envToolChainInfo.addTargets("*");
            }
            else {
                envToolChainInfo.addTargets(str);
            }
            str = getValue("TOOL_CHAIN_TAG", targetFileInfo);
            if (str == null || str.trim().equals("")) {
                envToolChainInfo.addTagnames("*");
            }
            else {
                envToolChainInfo.addTagnames(str);
            }
            str = getValue("TARGET_ARCH", targetFileInfo);
            if (str == null || str.trim().equals("")) {
                envToolChainInfo.addArchs("*");
            }
            else {
                envToolChainInfo.addArchs(str);
            }
            GlobalData.setToolChainEnvInfo(envToolChainInfo);
            
            str = getValue("TOOL_CHAIN_CONF", targetFileInfo);
            if (str != null) {
                toolsDefFilename = str;
            }
            
            str = getValue("ACTIVE_PLATFORM", targetFileInfo);
            if (str != null && ! str.trim().equals("")) {
                if ( ! str.endsWith(".fpd")) {
                    throw new BuildException("FPD file's file extension must be \".fpd\"");
                }
                activePlatform = str;
            }
        }
        catch (Exception ex) {
            throw new BuildException(ex.getMessage());
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
