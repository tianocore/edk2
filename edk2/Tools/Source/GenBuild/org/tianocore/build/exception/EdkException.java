/** @file
  EntityException class.

  The class handle the exception throwed by entity class.
 
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php
 
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/   
package org.tianocore.build.exception;

/**
  The class handle the exception throwed by entity class.
**/
public class EdkException extends Exception {
    static final long serialVersionUID = -8034897190740066938L;
    /**
      Constructure function
        
      @param expStr exception message string.
    **/
    public EdkException(String expStr) {
        super("[EdkException]:" + expStr);
    }
    
    public EdkException() {
        super();
    }
}

