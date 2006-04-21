/*
 * 
 * Copyright 2002-2004 The Ant-Contrib project
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
package net.sf.antcontrib.cpptasks;
import java.io.File;
import java.io.IOException;
/**
 * The history of a source file used to build a target
 * 
 * @author Curt Arnold
 */
public final class SourceHistory {
    private/* final */long lastModified;
    private/* final */String relativePath;
    /**
     * Constructor
     */
    public SourceHistory(String relativePath, long lastModified) {
        if (relativePath == null) {
            throw new NullPointerException("relativePath");
        }
        this.relativePath = relativePath;
        this.lastModified = lastModified;
    }
    public String getAbsolutePath(File baseDir) {
        try {
            return new File(baseDir, relativePath).getCanonicalPath();
        } catch (IOException ex) {
        }
        return relativePath;
    }
    public long getLastModified() {
        return lastModified;
    }
    public String getRelativePath() {
        return relativePath;
    }
}
