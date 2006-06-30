package org.tianocore.frameworkwizard.platform.ui.id;

import java.io.File;

import org.tianocore.frameworkwizard.platform.ui.global.GlobalData;

public class ModuleIdentification extends Identification {
    
    private PackageIdentification packageId;
    
    private File msaFile;
    
    private String moduleType;
    
    private boolean isLibrary = false;

    public boolean isLibrary() {
        return isLibrary;
    }

    public void setLibrary(boolean isLibrary) {
        this.isLibrary = isLibrary;
    }

    public File getMsaFile() throws Exception{
        prepareMsaFile();
        return msaFile;
    }
    
    public String getModuleRelativePath() throws Exception{
        prepareMsaFile();
        return msaFile.getParent().substring(packageId.getPackageDir().length() + 1);
    }

    private void prepareMsaFile()throws Exception{
        if (msaFile == null) {
            msaFile = GlobalData.getModuleFile(this);
        }
    }
    public void setMsaFile(File msaFile) {
        this.msaFile = msaFile;
    }

    public ModuleIdentification(String name, String guid, String version){
        super(name, guid, version);
    }
    
    public ModuleIdentification(String name, String guid, String version, PackageIdentification packageId){
        super(name, guid, version);
        this.packageId = packageId;
    }
    
    public boolean equals(Object obj) {
        if (obj instanceof ModuleIdentification) {
            ModuleIdentification id = (ModuleIdentification)obj;
            if (guid.equals(id.getGuid()) && packageId.equals(id.getPackage())) {
                if (version == null || id.version == null) {
                    updateName(name, id.name);
                    return true;
                }
                else if (version.trim().equalsIgnoreCase("") || id.version.trim().equalsIgnoreCase("")){
                    updateName(name, id.name);
                    return true;
                }
                else if (version.equalsIgnoreCase(id.version)) {
                    updateName(name, id.name);
                    return true;
                }
            }
            return false;
        }
        else {
            return super.equals(obj);
        }
    }
    
    public String toString(){
        if (version == null || version.trim().equalsIgnoreCase("")) {
            return "Module [" + name + "] in " + packageId;
        }
        else {
            return "Module [" + name + " " + version + "] in " + packageId; 
        }
    }

    public void setPackage(PackageIdentification packageId) {
        this.packageId = packageId;
    }

    public PackageIdentification getPackage() {
        return packageId;
    }

    public String getModuleType() {
        return moduleType;
    }

    public void setModuleType(String moduleType) {
        this.moduleType = moduleType;
    }
}
