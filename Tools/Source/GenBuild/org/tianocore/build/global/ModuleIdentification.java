package org.tianocore.build.global;

public class ModuleIdentification {

    private String baseName;
    
    private String packageName;
    
    private String guid;
    
    private String version;
    
    public ModuleIdentification(String baseName, String packageName, String guid, String version){
        this.baseName = baseName;
        this.packageName = packageName;
        this.guid = guid;
        this.version = version;
    }
    
    public boolean equals(Object obj) {
        if (obj instanceof ModuleIdentification) {
            ModuleIdentification moduleIdObj = (ModuleIdentification)obj;
            if ( baseName.equalsIgnoreCase(moduleIdObj.baseName)) {
                return true;
            }
            // TBD
            return false;
        }
        else {
            return super.equals(obj);
        }
    }
    
    public String toString(){
        return packageName + ":" + guid + "_" + baseName + "_" + version;
    }

    public void setBaseName(String baseName) {
        this.baseName = baseName;
    }

    public void setGuid(String guid) {
        this.guid = guid;
    }

    public void setPackageName(String packageName) {
        this.packageName = packageName;
    }

    public void setVersion(String version) {
        this.version = version;
    }
    
    
}
