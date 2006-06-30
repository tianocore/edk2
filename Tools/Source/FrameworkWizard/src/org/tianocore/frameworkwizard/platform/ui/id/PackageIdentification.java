package org.tianocore.frameworkwizard.platform.ui.id;
import java.io.File;

import org.tianocore.frameworkwizard.platform.ui.global.GlobalData;

public class PackageIdentification extends Identification{
    
    //
    // It is optional
    //
    private File spdFile;
    
    public PackageIdentification(String name, String guid, String version){
        super(name, guid, version);
    }
    
    public PackageIdentification(String name, String guid, String version, String spdFilename){
        super(name, guid, version);
        this.spdFile = new File(spdFilename);
    }
    
    public PackageIdentification(String name, String guid, String version, File spdFile){
        super(name, guid, version);
        this.spdFile = spdFile;
    }
    
    public void setSpdFile(File spdFile) {
        this.spdFile = spdFile;
    }

    public File getSpdFile() {
        return spdFile;
    }

    public String toString(){
        if (version == null || version.trim().equalsIgnoreCase("")) {
            return "package [" + name + "]";
        }
        else {
            return "package [" + name + " " + version + "]";
        }
    }
    
    public String getPackageDir()throws Exception{
        prepareSpdFile();
        return spdFile.getParent();
    }
    
    public String getPackageRelativeDir()throws Exception{
        prepareSpdFile();
        return spdFile.getParent().substring(GlobalData.getWorkspacePath().length() + 1);
    }
    
    private void prepareSpdFile() throws Exception{
        if (spdFile == null) {
            spdFile = GlobalData.getPackageFile(this);
        }
    }
}
