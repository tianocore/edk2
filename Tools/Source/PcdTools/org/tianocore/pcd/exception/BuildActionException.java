/** @file
  BuildActionException class.

  BuildAction Exception deals with all build action exceptions.
 
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php
 
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/   
package org.tianocore.pcd.exception;

import org.apache.tools.ant.BuildException;

/**
  BuildAction Exception deals with all build action exceptions.
**/
public class BuildActionException extends BuildException {
    static final long serialVersionUID = -7034897190740066939L;
    /**
      Constructure function
      
      @param reason exception message string.
    **/
    public BuildActionException(String reason) {
        super(reason);
    }
}
