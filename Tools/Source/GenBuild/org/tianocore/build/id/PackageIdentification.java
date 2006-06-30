package org.tianocore.build.id;
import java.io.File;

import org.tianocore.build.global.GlobalData;

public class PackageIdentification extends Identification{
    
    //
    // It is optional
    //
    private File spdFile;
    
    public PackageIdentification(String guid, String version){
        super(guid, version);
    }
    
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
        if (name == null) {
            GlobalData.refreshPackageIdentification(this);
        }
        if (version == null || version.trim().equalsIgnoreCase("")) {
            return "package [" + name + "]";
        }
        else {
            return "package [" + name + " " + version + "]";
        }
    }
    
    public String getPackageDir(){
        prepareSpdFile();
        return spdFile.getParent();
    }
    
    public String getPackageRelativeDir(){
        prepareSpdFile();
        return spdFile.getParent().substring(GlobalData.getWorkspacePath().length() + 1);
    }
    
    private void prepareSpdFile(){
        if (spdFile == null) {
            GlobalData.refreshPackageIdentification(this);
        }
    }
    
    public String getName() {
        if (name == null) {
            GlobalData.refreshPackageIdentification(this);
        }
        return name;
    }
}
