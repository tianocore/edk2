/** @file
  GenBuildException class.

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

import org.tianocore.common.exception.EdkException;

/**
  The class handle the exception throwed by entity class.
**/
public class GenBuildException extends EdkException {
    static final long serialVersionUID = -8034897190740066937L;
    /**
      Constructure function

      @param expStr exception message string.
    **/
    public GenBuildException(String expStr) {
        super(expStr);
    }

    public GenBuildException() {
        super();
    }
    public GenBuildException(Exception e, String message){
        super(e, message);
    }
}
