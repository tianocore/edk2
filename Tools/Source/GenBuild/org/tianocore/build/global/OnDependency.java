/** @file
This file is to define OnDependency class.

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

--*/
package org.tianocore.build.global;

import java.io.File;
import java.util.Iterator;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Task;
import org.apache.tools.ant.taskdefs.Sequential;
import org.tianocore.common.logger.EdkLog;
import org.tianocore.common.cache.FileTimeStamp;

/**
 Class OnDepdendency is used to check the timestamp between source files and
 target files, which can be used to determine if the target files are needed to
 be re-generated from source files.
 **/
public class OnDependency extends Task {
    //
    // source files list
    //
    private DpFileList sources = null;

    //
    // target files list
    //
    private DpFileList targets = null;

    //
    // tasks to be performed to generate target files
    //
    private Sequential  task = null;

    /**
     An empty constructor for an ANT task can avoid some potential issues
     **/
    public OnDependency(){
    }

    /**
     Standard execute method of ANT task
     **/
    public void execute() throws BuildException {
        if (isOutOfDate() && task != null) {
            task.perform();
        }

        //
        // Update the time stamp of target files since they are just re-generated
        // 
        for (Iterator dstIt = targets.nameList.iterator(); dstIt.hasNext();) {
            FileTimeStamp.update((String)dstIt.next());
        }
    }

    //
    // check if the target files are outofdate
    //
    private boolean isOutOfDate() {
        ///
        /// if no source files specified, take it as a fresh start
        ///
        if (sources.nameList.size() == 0) {
            EdkLog.log(this, EdkLog.EDK_VERBOSE, "No source file spcified!");
            return true;
        }

        if (targets.nameList.size() == 0) {
            EdkLog.log(this, EdkLog.EDK_VERBOSE, "No target file found!");
            return true;
        }

        Iterator dstIt = targets.nameList.iterator();
        while (dstIt.hasNext()) {
            String dstFileName = (String)dstIt.next();
            File dstFile = new File(dstFileName);
            if (!dstFile.exists()) {
                EdkLog.log(this, EdkLog.EDK_VERBOSE, "Target file [" + dstFileName + "] doesn't exist!");
                return true;
            }

            long dstTimeStamp = FileTimeStamp.get(dstFileName);
            Iterator srcIt = sources.nameList.iterator();
            while (srcIt.hasNext()) {
                String srcFileName = (String)srcIt.next();
                long srcTimeStamp = FileTimeStamp.get(srcFileName);

                if (srcTimeStamp == 0) {
                    //
                    // time stamp 0 means that the file doesn't exist
                    // 
                    throw new BuildException("Source File name: " + srcFileName + " doesn't exist!!!");
                }

                if (dstTimeStamp < srcTimeStamp) {
                    EdkLog.log(this, EdkLog.EDK_VERBOSE, "Source file [" + srcFileName + "] has been changed since last build!");
                    return true;
                }
            }
        }

        EdkLog.log(this, EdkLog.EDK_VERBOSE, "Target files are up-to-date!");
        return false;
    }

    /**
     Add method of ANT task for nested element with Sequential type

     @param     task    Sequential object which contains tasks for generating target files
     **/
    public void addSequential(Sequential task) {
        this.task = task;
    }

    /**
     Add method of ANT task for nested element with DpFileList type

     @param     sources DpFileList object which contains the list of source files
     **/
    public void addSourcefiles(DpFileList sources) {
        this.sources = sources;
    }

    /**
     Add method of ANT task for nested element with DpFileList type

     @param     targets DpFileList object which contains the list of target files
     **/
    public void addTargetfiles(DpFileList targets) {
        this.targets = targets;
    }
}

