package org.tianocore.build.toolchain;
/**
* TODO: Add class description
* 
* @author   jwang36
*/
public class ToolChainAttribute {
    private static int nextValue = 0;

    //"NAME", "PATH", "DPATH", "SPATH", "EXT", "FAMILY", "FLAGS"
    public final static ToolChainAttribute NAME = new ToolChainAttribute("NAME");
    public final static ToolChainAttribute PATH = new ToolChainAttribute("PATH");
    public final static ToolChainAttribute DPATH = new ToolChainAttribute("DPATH");
    public final static ToolChainAttribute SPATH = new ToolChainAttribute("SPATH");
    public final static ToolChainAttribute EXT = new ToolChainAttribute("EXT");
    public final static ToolChainAttribute FAMILY = new ToolChainAttribute("FAMILY");
    public final static ToolChainAttribute FLAGS = new ToolChainAttribute("FLAGS");

    private final String name;
    public final int value = nextValue++;

    /**
     * Default constructor
     */
    private ToolChainAttribute(String name) {
        this.name = name;
    }

    public String toString() {
        return name;
    }
}





