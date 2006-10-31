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

import java.io.File;
import java.util.HashSet;
import java.util.Iterator;

public final class PathIterator implements Common.ForDoAll {
	// this PathIterator is based on HashSet, an thread implementation is
	// required.
	PathIterator(String path, int md) throws Exception {
		startpath = path;
		mode = md;
		Common.toDoAll(startpath, this, mode);
		it = pathlist.iterator();
	}

	private String startpath = null;

	private int mode;

	private HashSet<String> pathlist = new HashSet<String>();

	private Iterator<String> it = null;

	public final void run(String path) throws Exception {
		pathlist.add(path);
	}

	public boolean filter(File dir) {
		return true;
	}

	public final String next() {
		return it.next();
	}

	public final boolean hasNext() {
		return it.hasNext();
	}

	public final String toString() {
		return pathlist.toString();
	}
}
