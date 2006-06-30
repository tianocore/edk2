package org.tianocore.frameworkwizard.platform.ui.id;

public class Identification {

    String name;
    
    String guid;
    
    String version;
    
    String type; // Optional
    
    Identification(String name, String guid, String version){
        this.name = name;
        this.guid = guid;
        this.version = version;
    }
    
    public boolean equals(Object obj) {
        if (obj instanceof Identification) {
            Identification id = (Identification)obj;
            if ( guid.equalsIgnoreCase(id.guid)) {
                if (version == null || id.version == null) {
                    updateName(name, id.name);
                    return true;
                }
                else if (version.trim().equalsIgnoreCase("") || id.version.trim().equalsIgnoreCase("")){
                    return true;
                }
                else if (version.equalsIgnoreCase(id.version)) {
                    return true;
                }
            }
            return false;
        }
        else {
            return super.equals(obj);
        }
    }
    
    void updateName(String name1, String name2) {
        if (name1 == null) {
            name1 = name2;
        }
        if (name2 == null) {
            name2 = name1;
        }
    }
    
    public void setName(String name) {
        this.name = name;
    }

    public void setGuid(String guid) {
        this.guid = guid;
    }

    public void setVersion(String version) {
        this.version = version;
    }

    public String getGuid() {
        return guid;
    }

    public String getName() {
        return name;
    }

    public String getVersion() {
        return version;
    }
    
    public int hashCode(){
        return guid.hashCode();
    }
}
