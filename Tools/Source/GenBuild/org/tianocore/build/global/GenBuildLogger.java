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

import org.apache.tools.ant.Project;

import org.tianocore.common.logger.LogMethod;

public class GenBuildLogger implements LogMethod {
    private Project project;
    public GenBuildLogger(Project project) {
        this.project = project;

    }

    public void putMessage(Object msgSource, int msgLevel, String msg) {
        if (this.project != null){
            this.project.log(msg, Project.MSG_INFO);
        }

    }
}