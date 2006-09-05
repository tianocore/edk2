/*++

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:
  FrameworkLogger.java

Abstract:

--*/

package org.tianocore.framework.tasks;

import org.apache.tools.ant.Project;
import java.io.File;
import org.tianocore.common.logger.LogMethod;

class FrameworkLogger implements LogMethod {
    private Project project;
    private String  titleName;
    public FrameworkLogger(Project project, String taskName) {
        this.project = project;
        this.titleName = taskName;
    }

    public void putMessage(Object msgSource, int msgLevel, String msg) {
        String frameworkMsg = " [" + this.titleName + "] " + msg;
        this.project.log(frameworkMsg, Project.MSG_INFO);
    }
    
    public void flushToFile(File file) {
    }
}