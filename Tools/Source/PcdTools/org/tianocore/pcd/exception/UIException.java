/** @file
  UIException class.

  The class handle the exception throwed by UI action class.
 
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php
 
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/   
package org.tianocore.pcd.exception;

/**
  The class handle the exception throwed by UI action class.
**/
public class UIException extends Exception {
    static final long serialVersionUID = -7034897190740066930L;
    /**
      Constructure function
        
      @param reason exception message string.
    **/
    public UIException(String reason) {
        super(reason);
    }
}
