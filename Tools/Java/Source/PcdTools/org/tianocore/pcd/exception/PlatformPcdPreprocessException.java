/** @file
  PlatformPcdPreprocessException class.

  The class handle the exception throwed by PlatformPcdPreprocessAction class.

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
package org.tianocore.pcd.exception;

public class PlatformPcdPreprocessException extends Exception {
    /**
	  serial version ID
	**/
	private static final long serialVersionUID = 2858398552845888282L;

	/**
      Constructure function

      @param expStr exception message string.
    **/
    public PlatformPcdPreprocessException(String expStr) {
        super("\r\n[PlatformPcdPreprocess Failure] #############################\r\n" + expStr);
    }

    public PlatformPcdPreprocessException() {
        super();
    }
}
