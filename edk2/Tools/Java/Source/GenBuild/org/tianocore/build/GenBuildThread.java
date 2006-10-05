/** @file
  This file is for single module thread definition. 

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
package org.tianocore.build;

import java.util.Hashtable;
import java.util.Iterator;
import java.util.LinkedHashSet;
import java.util.Set;
import java.util.Vector;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.BuildListener;
import org.apache.tools.ant.Project;
import org.apache.tools.ant.taskdefs.Property;
import org.tianocore.build.GenBuildTask;
import org.tianocore.build.fpd.FpdParserForThread;
import org.tianocore.build.global.GenBuildLogger;
import org.tianocore.build.id.FpdModuleIdentification;
import org.tianocore.build.id.ModuleIdentification;
import org.tianocore.common.logger.EdkLog;

/**
  Add more comment here. 

  @since GenBuild 1.0
**/
public class GenBuildThread implements Runnable {

    private ModuleIdentification parentModuleId = null;

    private ModuleIdentification moduleId = null;

    private Set<FpdModuleIdentification> dependencies = new LinkedHashSet<FpdModuleIdentification>();
    
    private int status = FpdParserForThread.STATUS_DEPENDENCY_NOT_READY;

    private Project project = null;

    public Object semaphore = new Object();

    private String arch = null;

    private boolean highPriority = false;

    private Thread thread;

    public GenBuildThread(ModuleIdentification moduleId, String arch) {
        this.moduleId = moduleId;
        this.arch = arch;
        thread = new Thread(FpdParserForThread.tg, this, moduleId + ":" + arch);
    }

    public boolean start() {
        if (highPriority) {
            thread.setPriority(Thread.MAX_PRIORITY);
        }

        status = FpdParserForThread.STATUS_START_RUN;

        thread.start();

        return true;
    }

    public void run() {

        FpdModuleIdentification fpdModuleId = new FpdModuleIdentification(moduleId, arch);
        
        try {
            //
            // Prepare pass down properties
            // ARCH, MODULE_GUID, MODULE_VERSION, PACKAGE_GUID, PACKAGE_VERSION, PLATFORM_FILE
            //
            Vector<Property> properties = new Vector<Property>();
            Property property = new Property();
            property.setName("ARCH");
            property.setValue(arch);
            properties.add(property);
    
            property = new Property();
            property.setName("MODULE_GUID");
            property.setValue(moduleId.getGuid());
            properties.add(property);
    
            property = new Property();
            property.setName("MODULE_VERSION");
            if (moduleId.getVersion() == null) {
                property.setValue("");
            } else {
                property.setValue(moduleId.getVersion());
            }
            properties.add(property);
    
            property = new Property();
            property.setName("PACKAGE_GUID");
            property.setValue(moduleId.getPackage().getGuid());
            properties.add(property);
    
            property = new Property();
            property.setName("PACKAGE_VERSION");
            if (moduleId.getPackage().getVersion() == null) {
                property.setValue("");
            } else {
                property.setValue(moduleId.getPackage().getVersion());
            }
            properties.add(property);
    
            //
            // Build the Module
            //
            GenBuildTask genBuildTask = new GenBuildTask();
    
            Project newProject = new Project();
    
            Hashtable passdownProperties = project.getProperties();
            Iterator iter = passdownProperties.keySet().iterator();
            while (iter.hasNext()) {
                String item = (String) iter.next();
                newProject.setProperty(item, (String) passdownProperties.get(item));
            }
    
            newProject.setInputHandler(project.getInputHandler());
    
            Iterator listenerIter = project.getBuildListeners().iterator();
            GenBuildLogger newLogger = null;
            while (listenerIter.hasNext()) {
                BuildListener item = (BuildListener)listenerIter.next();
                if (item instanceof GenBuildLogger) {
                    newLogger = (GenBuildLogger)((GenBuildLogger)item).clone();
                    newLogger.setId(fpdModuleId);
                    newProject.addBuildListener(newLogger);
                } else {
                    newProject.addBuildListener(item);
                }
            }
    
            project.initSubProject(newProject);
            
            genBuildTask.setProject(newProject);
    
            genBuildTask.setExternalProperties(properties);
    
            genBuildTask.parentId = parentModuleId;

            genBuildTask.perform();
        } catch (BuildException be) {
            
            EdkLog.log("GenBuild", EdkLog.EDK_ALWAYS, fpdModuleId + " build error. \n" + be.getMessage());
            
            if (FpdParserForThread.errorModule == null) {
                FpdParserForThread.errorModule = fpdModuleId;
            }
            
            synchronized (FpdParserForThread.deamonSemaphore) {
                FpdParserForThread.subCount();
                FpdParserForThread.deamonSemaphore.notifyAll();
            }
            
            return ;
        }
        
        status = FpdParserForThread.STATUS_END_RUN;

        EdkLog.log("GenBuild", EdkLog.EDK_ALWAYS, fpdModuleId + " build finished. ");
        
        //
        // 
        //
        synchronized (FpdParserForThread.deamonSemaphore) {
            FpdParserForThread.subCount();
            FpdParserForThread.deamonSemaphore.notifyAll();
        }
    }

    public void setArch(String arch) {
        this.arch = arch;
    }

    public void setDependencies(Set<FpdModuleIdentification> dependencies) {
        this.dependencies = dependencies;
    }

    public void setModuleId(ModuleIdentification moduleId) {
        this.moduleId = moduleId;
    }

    public void setParentModuleId(ModuleIdentification parentModuleId) {
        this.parentModuleId = parentModuleId;
    }

    public void setProject(Project project) {
        this.project = project;
    }

    public void setHighPriority(boolean highPriority) {
        this.highPriority = highPriority;
    }


    public Set<FpdModuleIdentification> getDependencies() {
        return dependencies;
    }

    public ModuleIdentification getModuleId() {
        return moduleId;
    }

    public int getStatus() {
        //
        // Add code here to judge dependency
        //
        if (status == FpdParserForThread.STATUS_DEPENDENCY_NOT_READY) {
            Iterator<FpdModuleIdentification> iter = dependencies.iterator();
            boolean flag = true;
            while (iter.hasNext()) {
                FpdModuleIdentification item = iter.next();
                if (FpdParserForThread.allThreads.get(item).getStatus() == 1) {
                    flag = false;
                    break ;
                }
            }
            if (flag) {
                status = FpdParserForThread.STATUS_DEPENDENCY_READY;
            }
        }
        return status;
    }

    public void setStatus(int status) {
        this.status = status;
    }
}
