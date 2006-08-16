package org.tianocore.migration;

import java.util.regex.*;

public class Critic implements Common.ForDoAll {
	Critic() {
		filepath = null;
	}
	Critic(String path) {
		filepath = path;
	}
	
	private String filepath = null;
	
	private static Pattern ptnheadcomment = Pattern.compile("^\\/\\*\\+\\+(.*?)\\-\\-\\*\\/",Pattern.DOTALL);
	private static Matcher mtrheadcomment;
	
	public void toDo(String filepath) throws Exception {
		if (filepath.contains(".c") || filepath.contains(".h")) {
			String wholeline = Common.file2string(filepath);
			mtrheadcomment = ptnheadcomment.matcher(wholeline);
			if (mtrheadcomment.find()) {			//as we find only the head comment here, use 'if' not 'while'
				wholeline = mtrheadcomment.replaceFirst("/** @file$1**/");
				Common.string2file(wholeline, filepath + "_");
			}
		}
	}
	
	public static void fireAt(String path) throws Exception {
		Common.toDoAll(path, new Critic());
		System.out.println("Critic Done");
	}
}
