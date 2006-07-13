package org.tianocore.frameworkwizard.far;

public class FarIdentification {

    private String guid;

    private String md5Value;

    private String path;

    public FarIdentification(String guid, String md5Value, String path) {
        this.guid = guid;
        this.md5Value = md5Value;
        this.path = path;
    }

    public String getGuid() {
        return guid;
    }

    public void setGuid(String guid) {
        this.guid = guid;
    }

    public String getMd5Value() {
        return md5Value;
    }

    public void setMd5Value(String md5Value) {
        this.md5Value = md5Value;
    }

    public String getPath() {
        return path;
    }

    public void setPath(String path) {
        this.path = path;
    }

    public String toString() {
        return path + " [" + guid + "]";
    }
}
