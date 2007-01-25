/** @file
This file is to define interface for manipulating section file

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
package org.tianocore.framework.tasks;

import java.io.DataOutputStream;

/**
 Section interface is for geting the contain buffer form compress, tool, and sectFile  
 **/
public interface Section {
    int alignment = 0;
    public void toBuffer (DataOutputStream buffer);
    public void setAlignment(int alignment);
    public int getAlignment();
}