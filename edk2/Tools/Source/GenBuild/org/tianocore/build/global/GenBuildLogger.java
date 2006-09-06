/*++

 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php

 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 Module Name:
 GenBuildLogger.java

 Abstract:

 --*/

package org.tianocore.build.global;

import java.io.File;
import java.util.List;
import java.util.Vector;

import org.apache.tools.ant.Project;
import org.apache.tools.ant.Task;

import org.tianocore.common.logger.EdkLog;
import org.tianocore.common.logger.LogMethod;

public class GenBuildLogger implements LogMethod {
    private Project project = null;

    ///
    /// flag to present whether cache all msg or not
    /// true means not to cache.
    ///
    private boolean flag = true;

    private List<String> v = null;

    public GenBuildLogger (Project project) {
        this.project = project;
    }

    public GenBuildLogger (Project project, boolean flag) {
        this.project = project;
        this.flag = flag;

        //
        // Only flag is false, v will be initialized and used.
        //
        if (!flag) {
            v = new Vector<String>(2048);
        }
    }

    /**
      Rules: flag = true: means no cache Action: Print it to console
      
      flag = false: mean cache all msg exception some special Action: loglevel
      is EDK_ALWAYS -- Print but no cache loglevel is EDK_ERROR -- Print and
      cache the msg others -- No print and cache the msg
    **/
    public synchronized void putMessage(Object msgSource, int msgLevel,
                    String msg) {
        if (this.project == null) {
            return;
        }

        //
        // If msgLevel is always print, then print it
        //
        switch (msgLevel) {
        case EdkLog.EDK_ALWAYS:
            log(msgSource, msg, Project.MSG_INFO);
            break;
        case EdkLog.EDK_ERROR:
            if (flag) {
                log(msgSource, msg, Project.MSG_ERR);
            } else {
                log(msgSource, msg, Project.MSG_ERR);
                v.add(msg);
            }
            break;
        case EdkLog.EDK_WARNING:
            if (flag) {
                log(msgSource, msg, Project.MSG_WARN);
            } else {
                v.add(msg);
            }
            break;
        case EdkLog.EDK_INFO:
            if (flag) {
                log(msgSource, msg, Project.MSG_INFO);
            } else {
                v.add(msg);
            }
            break;
        case EdkLog.EDK_VERBOSE:
            if (flag) {
                log(msgSource, msg, Project.MSG_VERBOSE);
            } else {
                v.add(msg);
            }
            break;
        case EdkLog.EDK_DEBUG:
            if (flag) {
                log(msgSource, msg, Project.MSG_DEBUG);
            } else {
                v.add(msg);
            }
            break;
        }
    }

    public void flushToFile(File file) {
        //
        // Sort msg and store to the file (TBD)
        //

    }
    
    private void log(Object msgSource, String msg, int level) {
        if (msgSource instanceof Task) {
            this.project.log((Task)msgSource, msg, level);
        } else {
            this.project.log(msg, level);
        }
    }
}