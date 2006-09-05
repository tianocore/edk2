/** @file
 This file is ANT task FpdParserTask. 
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php

 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 **/
package org.tianocore.build.fpd;

import java.io.File;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.taskdefs.Ant;
import org.apache.xmlbeans.XmlObject;

import org.tianocore.build.global.GlobalData;
import org.tianocore.build.global.OutputManager;
import org.tianocore.build.id.FpdModuleIdentification;
import org.tianocore.build.id.ModuleIdentification;
import org.tianocore.build.FrameworkBuildTask;
import org.tianocore.build.GenBuildThread;
import org.tianocore.common.exception.EdkException;

/**

  @since GenBuild 1.0
**/
public class FpdParserForThread extends FpdParserTask {
    
    public static Map<FpdModuleIdentification, GenBuildThread> allThreads = new LinkedHashMap<FpdModuleIdentification, GenBuildThread>();
    
    List<String> queueList = new ArrayList<String>();
    
    public static Object deamonSemaphore =  new Object();
    
    static Object countSemaphore =  new Object();
    
    public static int STATUS_DEPENDENCY_NOT_READY = 1;
    
    public static int STATUS_DEPENDENCY_READY = 2;
    
    public static int STATUS_START_RUN = 3;
    
    public static int STATUS_END_RUN = 4;
    
    private int currentQueueCode = 0;
    
    public static int currentRunNumber = 0;
    
    /**
      Public construct method. It is necessary for ANT task.
    **/
    public FpdParserForThread() {
    }

    /**
     

    **/
    public void execute() throws BuildException {
        //
        // Parse FPD file
        //
        parseFpdFile();

        //
        // Prepare BUILD_DIR
        //
        isUnified = OutputManager.getInstance().prepareBuildDir(getProject());

        //
        // For every Target and ToolChain
        //
        String[] targetList = GlobalData.getToolChainInfo().getTargets();
        for (int i = 0; i < targetList.length; i++) {
            String[] toolchainList = GlobalData.getToolChainInfo().getTagnames();
            for(int j = 0; j < toolchainList.length; j++) {
                //
                // Prepare FV_DIR
                //
                String ffsCommonDir = getProject().getProperty("BUILD_DIR") + File.separatorChar
                                + targetList[i] + File.separatorChar
                                + toolchainList[j];
                File fvDir = new File(ffsCommonDir + File.separatorChar + "FV");
                fvDir.mkdirs();
                getProject().setProperty("FV_DIR", fvDir.getPath().replaceAll("(\\\\)", "/"));

                //
                // Gen Fv.inf files
                //
                genFvInfFiles(ffsCommonDir);
            }
        }

        //
        // Gen build.xml
        //
        PlatformBuildFileGenerator fileGenerator = new PlatformBuildFileGenerator(getProject(), outfiles, fvs, isUnified, saq);
        fileGenerator.genBuildFile();
        
        //
        // Prepare Queue
        //
        queueList.add("libqueue");
        
        String[] validFv = saq.getFpdValidImageNames();
        
        for (int i = 0; i < validFv.length; i++) {
            queueList.add(validFv[i]);
        }
        
        Iterator<String> fvsNameIter = fvs.keySet().iterator();
        
        while (fvsNameIter.hasNext()) {
            String fvName = fvsNameIter.next();
            if (!isContain(validFv, fvName)) {
                queueList.add(fvName);
            }
        }
        
        //
        // Ant call ${PLATFORM}_build.xml
        //
        Ant ant = new Ant();
        ant.setProject(getProject());
        ant.setAntfile(platformId.getFpdFile().getParent() + File.separatorChar + platformId.getName() + "_build.xml");
        ant.setTarget("prebuild");
        ant.setInheritAll(true);
        ant.init();
        ant.execute();
        
        System.out.println("Task number is " + allThreads.size());
        
        //
        // Waiting for all thread over, or time out
        //
        synchronized (deamonSemaphore) {
            //
            // Initialize BUGBUG
            //
            
            while (true) {
                //
                // If all modules are already built
                //
                if (currentQueueCode >= queueList.size()) {
                    break ;
                }

                Set<FpdModuleIdentification> currentQueueModules = fvs.get(queueList.get(currentQueueCode));
                
                if (currentQueueModules == null) {
                    ++currentQueueCode;
                    continue ;
                }
                Iterator<FpdModuleIdentification> currentIter = currentQueueModules.iterator();

                GenBuildThread a = null;

                boolean existNoneReady = false;

                while (currentIter.hasNext()) {
                    GenBuildThread item = allThreads.get(currentIter.next()); 
                    if (item.getStatus() == STATUS_DEPENDENCY_NOT_READY) {
                        existNoneReady = true;
                    } else if (item.getStatus() == STATUS_DEPENDENCY_READY) {
                        a = item;
                        addCount();
                        a.start();
                        if (currentRunNumber == FrameworkBuildTask.MAX_CONCURRENT_THREAD_NUMBER) {
                            break ;
                        }
                    }
                }

                if (a != null) {
                    //
                    // Exist ready thread
                    //
                    System.out.println("## Exist ready thread");

                } else if (existNoneReady && currentRunNumber == 0) {
                    //
                    // No active thread, but still have dependency not read thread
                    //
                    throw new BuildException("Found can't resolve dependencies. ");
                } else if (!existNoneReady && currentRunNumber == 0) {
                    //
                    // Current queue build finish, move to next
                    //
                    System.out.println("## Current queue build finish, move to next");
                    ++currentQueueCode;
                    continue ;
                } else {
                    //
                    // active thread exist, but no ready thread
                    //
                    System.out.println("## active thread exist, but no ready thread" + currentRunNumber);
                }

                try {
                    deamonSemaphore.wait();
                } catch (InterruptedException e) {
                   e.printStackTrace();
                }
            }
        }
        
        //
        // call fvs, postbuild
        //
        ant = new Ant();
        ant.setProject(getProject());
        ant.setAntfile(platformId.getFpdFile().getParent() + File.separatorChar + platformId.getName() + "_build.xml");
        ant.setTarget("fvs");
        ant.setInheritAll(true);
        ant.init();
        ant.execute();
        
        ant = new Ant();
        ant.setProject(getProject());
        ant.setAntfile(platformId.getFpdFile().getParent() + File.separatorChar + platformId.getName() + "_build.xml");
        ant.setTarget("postbuild");
        ant.setInheritAll(true);
        ant.init();
        ant.execute();
        
    }

    
    /**
      Parse all modules listed in FPD file. 
    **/
    void parseModuleSAFiles() throws EdkException{
        
        Map<FpdModuleIdentification, Map<String, XmlObject>> moduleSAs = saq.getFpdModules();

        //
        // For every Module lists in FPD file.
        //
        Set<FpdModuleIdentification> keys = moduleSAs.keySet();
        Iterator<FpdModuleIdentification> iter = keys.iterator();
        while (iter.hasNext()) {
            FpdModuleIdentification fpdModuleId = iter.next();
            
            //
            // Generate GenBuildThread
            //
            GenBuildThread genBuildThread = new GenBuildThread();
            genBuildThread.setArch(fpdModuleId.getArch());
            genBuildThread.setParentModuleId(null);
            genBuildThread.setModuleId(fpdModuleId.getModule());
            genBuildThread.setProject(getProject());
            
            Set<FpdModuleIdentification> dependencies = new LinkedHashSet<FpdModuleIdentification>();
            
            GlobalData.registerFpdModuleSA(fpdModuleId, moduleSAs.get(fpdModuleId));
            
            //
            // Add all dependent Library Instance
            //
            saq.push(GlobalData.getDoc(fpdModuleId));

            ModuleIdentification[] libinstances = saq.getLibraryInstance(fpdModuleId.getArch());
            saq.pop();
            
            for (int i = 0; i < libinstances.length; i++) {
                FpdModuleIdentification libFpdModuleId = new FpdModuleIdentification(libinstances[i], fpdModuleId.getArch());
                //
                // Add to dependencies
                //
                dependencies.add(libFpdModuleId);
                
                //
                // Create thread for library instances
                //
                GenBuildThread liBuildThread = new GenBuildThread();
                liBuildThread.setArch(fpdModuleId.getArch());
                liBuildThread.setParentModuleId(fpdModuleId.getModule());
                liBuildThread.setModuleId(libinstances[i]);
                liBuildThread.setProject(getProject());
                liBuildThread.setStatus(STATUS_DEPENDENCY_READY);
                liBuildThread.setHighPriority(true);
                allThreads.put(libFpdModuleId, liBuildThread);
                
                updateFvs("libqueue", libFpdModuleId);
            }
            
            genBuildThread.setDependencies(dependencies); 
//            if (dependencies.size() == 0) {
                genBuildThread.setStatus(STATUS_DEPENDENCY_READY);
//            }
            
            allThreads.put(fpdModuleId, genBuildThread);
            
            //
            // Put fpdModuleId to the corresponding FV
            //
            saq.push(GlobalData.getDoc(fpdModuleId));
            String fvBinding = saq.getModuleFvBindingKeyword();

            fpdModuleId.setFvBinding(fvBinding);
            updateFvs(fvBinding, fpdModuleId);

            //
            // Prepare for out put file name
            //
            ModuleIdentification moduleId = fpdModuleId.getModule();

            String baseName = saq.getModuleOutputFileBasename();
            
            if (baseName == null) {
                baseName = moduleId.getName();
            }
            outfiles.put(fpdModuleId, fpdModuleId.getArch() + File.separatorChar
                         + moduleId.getGuid() + "-" + baseName
                         + getSuffix(moduleId.getModuleType()));

            //
            // parse module build options, if any
            //
            GlobalData.addModuleToolChainOption(fpdModuleId, parseModuleBuildOptions(false));
            GlobalData.addModuleToolChainFamilyOption(fpdModuleId, parseModuleBuildOptions(true));
            saq.pop();
        }
    }
    
    private boolean isContain(String[] list, String item) {
        for (int i = 0; i < list.length; i++) {
            if (list[i].equalsIgnoreCase(item)) {
                return true;
            }
        }
        return false;
    }
    
    public synchronized static void addCount() {
        synchronized (countSemaphore) {
            ++currentRunNumber;
        }
    }
    
    public synchronized static void subCount() {
        synchronized (countSemaphore) {
            --currentRunNumber;
        }
    }
}
