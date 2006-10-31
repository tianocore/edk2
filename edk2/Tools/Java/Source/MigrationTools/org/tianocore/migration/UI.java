/** @file
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/
package org.tianocore.migration;

import java.util.Set;

public interface UI {

	public boolean yesOrNo(String question);

	public void print(String message);

	public void println(String message);

	public void println(Set<String> hash);

	public String choose(String message, Object[] choicelist);

	public String getInput(String message);

	public String getFilepath(String title, int mode); // necessary ?
}
