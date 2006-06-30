package org.tianocore.build.toolchain;
/**
* TODO: Add class description
* 
* @author   jwang36
*/
public class ToolChainElement {
    private static int nextValue = 0;

    //"NAME", "PATH", "DPATH", "SPATH", "EXT", "FAMILY", "FLAGS"
    public final static ToolChainElement TARGET = new ToolChainElement("TARGET");
    public final static ToolChainElement TOOLCHAIN = new ToolChainElement("TOOLCHAIN");
    public final static ToolChainElement ARCH = new ToolChainElement("ARCH");
    public final static ToolChainElement TOOLCODE = new ToolChainElement("TOOLCODE");
    public final static ToolChainElement ATTRIBUTE = new ToolChainElement("ATTRIBUTE");

    private final String name;
    public final int value = nextValue++;

    /**
     * Default constructor
     */
    private ToolChainElement(String name) {
        this.name = name;
    }

    public String toString() {
        return name;
    }
}





