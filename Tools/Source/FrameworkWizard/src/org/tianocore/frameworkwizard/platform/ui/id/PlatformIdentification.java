package org.tianocore.frameworkwizard.platform.ui.id;
import java.io.File;

import org.tianocore.frameworkwizard.platform.ui.global.WorkspaceProfile;

public class PlatformIdentification extends Identification{
    
    private File fpdFile;
    
    public PlatformIdentification(String name, String guid, String version){
        super(name, guid, version);
    }
    
    public PlatformIdentification(String name, String guid, String version, String fpdFilename){
        super(name, guid, version);
        this.fpdFile = new File(fpdFilename);
    }
    
    public PlatformIdentification(String name, String guid, String version, File fpdFile){
        super(name, guid, version);
        this.fpdFile = fpdFile;
    }
    
    public String toString(){
        return "Platform " + name + "["+guid+"]";
    }

    public void setFpdFile(File fpdFile) {
        this.fpdFile = fpdFile;
    }

    public File getFpdFile() {
        return fpdFile;
    }
    
    public String getPlatformRelativeDir(){
        return fpdFile.getParent().substring(WorkspaceProfile.getWorkspacePath().length());
    }
}