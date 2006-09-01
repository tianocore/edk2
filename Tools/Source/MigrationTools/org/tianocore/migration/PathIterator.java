package org.tianocore.migration;

import java.util.*;

public final class PathIterator implements Common.ForDoAll {
//	 this PathIterator is based on HashSet, an thread implementation is required.
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

	public final void toDo(String path) throws Exception {
		pathlist.add(path);
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
