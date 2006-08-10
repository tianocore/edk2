package org.tianocore.migration;

import java.util.*;

public interface UI {
	
	public boolean yesOrNo(String question);
	
	public void print(String message);
	
	public void println(String message);
	
	public void println(Set<String> hash);
}
