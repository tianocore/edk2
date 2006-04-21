/*
 * 
 * Copyright 2004 The Ant-Contrib project
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

import java.io.IOException;
import java.io.Writer;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Project;
import org.apache.tools.ant.types.DataType;
import org.apache.tools.ant.types.Reference;

/**
 * Version Information. (Non-functional prototype)
 * 
 */
public class VersionInfo extends DataType {
	/**
	 * if property.
	 */
    private String ifCond;
    /**
     * unless property.
     */
    private String unlessCond;

    /**
     * extends property.
     */
    private String extendsId;
    
    /**
     * file version.
     *
     */
    private String fileVersion;
    /**
     * Product version.
     *
     */
    private String productVersion;
    /**
     * file language.
     *
     */
    private String language;
    
    /**
     * comments.
     *
     */
    private String fileComments;
    /**
     * Company name.
     *
     */
    private String companyName;
    /**
     * Description.
     *
     */
    private String description;
    /**
     * internal name.
     */
    private String internalName;
    /**
     * legal copyright.
     *
     */
    private String legalCopyright;
    /**
     * legal trademark.
     *
     */
    private String legalTrademark;
    /**
     * original filename.
     *
     */
    private String originalFilename;
    /**
     * private build.
     *
     */
    private String privateBuild;
    /**
     * product name.
     *
     */
    private String productName;
    /**
     * Special build
     */
    private String specialBuild;
    /**
     * compatibility version
     *
     */
    private String compatibilityVersion;
	

    /**
     * Constructor.
     *
     */
    public VersionInfo() {
    }
    public void execute() throws org.apache.tools.ant.BuildException {
        throw new org.apache.tools.ant.BuildException(
                "Not an actual task, but looks like one for documentation purposes");
    }
    /**
     * Returns true if the define's if and unless conditions (if any) are
     * satisfied.
     * 
     * @exception BuildException
     *                throws build exception if name is not set
     */
    public final boolean isActive() throws BuildException {
        return CUtil.isActive(getProject(), ifCond, unlessCond);
    }
    /**
     * Sets an id that can be used to reference this element.
     * 
     * @param id
     *            id
     */
    public void setId(String id) {
        //
        //  this is actually accomplished by a different
        //     mechanism, but we can document it
        //
    }
    /**
     * Sets the name of a version info that this info extends.
     * 
     * @param id
     *            id
     */
    public void setExtends(String id) {
    	extendsId = id;
    }

    
    /**
     * Sets the property name for the 'if' condition.
     * 
     * The define will be ignored unless the property is defined.
     * 
     * The value of the property is insignificant, but values that would imply
     * misinterpretation ("false", "no") will throw an exception when
     * evaluated.
     * 
     * @param propName
     *            property name
     */
    public final void setIf(String propName) {
        ifCond = propName;
    }
    /**
     * Specifies that this element should behave as if the content of the
     * element with the matching id attribute was inserted at this location. If
     * specified, no other attributes should be specified.
     *  
     */
    public void setRefid(Reference r) throws BuildException {
        super.setRefid(r);
    }
    /**
     * Set the property name for the 'unless' condition.
     * 
     * If named property is set, the define will be ignored.
     * 
     * The value of the property is insignificant, but values that would imply
     * misinterpretation ("false", "no") of the behavior will throw an
     * exception when evaluated.
     * 
     * @param propName
     *            name of property
     */
    public final void setUnless(String propName) {
        unlessCond = propName;
    }
    /**
     * Gets file version.
     * @return file version, may be null.
     *
     */
    public String getFileversion() {
        if (isReference()) {
            VersionInfo refVersion = (VersionInfo) 
				getCheckedRef(VersionInfo.class,
                    "VersionInfo");
            return refVersion.getFileversion();
        }
    	return fileVersion;
    }
    /**
     * Gets Product version.
     * @return product version, may be null
     */
    public String getProductversion() {
        if (isReference()) {
            VersionInfo refVersion = (VersionInfo) 
				getCheckedRef(VersionInfo.class,
                    "VersionInfo");
            return refVersion.getProductversion();
        }
    	return productVersion;
    }
    /**
     * Gets compatibility version.
     * @return compatibility version, may be null
     */
    public String getCompatibilityversion() {
        if (isReference()) {
            VersionInfo refVersion = (VersionInfo) 
				getCheckedRef(VersionInfo.class,
                    "VersionInfo");
            return refVersion.getCompatibilityversion();
        }
    	return compatibilityVersion;
    }
    /**
     * Gets file language, should be an IETF RFC 3066 identifier, for example, en-US.
     * @return language, may be null.
     */
    public String getLanguage() {
        if (isReference()) {
            VersionInfo refVersion = (VersionInfo) 
				getCheckedRef(VersionInfo.class,
                    "VersionInfo");
            return refVersion.getLanguage();
        }
    	return language;
    }
    
    /**
     * Gets comments.
     * @return comments, may be null.
     */
    public String getFilecomments() {
        if (isReference()) {
            VersionInfo refVersion = (VersionInfo) 
				getCheckedRef(VersionInfo.class,
                    "VersionInfo");
            return refVersion.getFilecomments();
        }
    	return fileComments;
    }
    /**
     * Gets Company name.
     * @return company name, may be null.
     */
    public String getCompanyname() {
        if (isReference()) {
            VersionInfo refVersion = (VersionInfo) 
				getCheckedRef(VersionInfo.class,
                    "VersionInfo");
            return refVersion.getCompanyname();
        }
    	return companyName;
    }
    /**
     * Gets Description.
     * @return description, may be null.
     */
    public String getDescription() {
        if (isReference()) {
            VersionInfo refVersion = (VersionInfo) 
				getCheckedRef(VersionInfo.class,
                    "VersionInfo");
            return refVersion.getDescription();
        }
    	return description;
    }
    /**
     * Gets internal name.
     * @return internal name, may be null.
     */
    public String getInternalname() {
        if (isReference()) {
            VersionInfo refVersion = (VersionInfo) 
				getCheckedRef(VersionInfo.class,
                    "VersionInfo");
            return refVersion.getInternalname();
        }
    	return internalName;
    }
    /**
     * Gets legal copyright.
     * @return legal copyright, may be null.
     */
    public String getLegalcopyright() {
        if (isReference()) {
            VersionInfo refVersion = (VersionInfo) 
				getCheckedRef(VersionInfo.class,
                    "VersionInfo");
            return refVersion.getLegalcopyright();
        }
    	return legalCopyright;
    }
    /**
     * Gets legal trademark.
     * @return legal trademark, may be null;
     */
    public String getLegaltrademark() {
        if (isReference()) {
            VersionInfo refVersion = (VersionInfo) 
				getCheckedRef(VersionInfo.class,
                    "VersionInfo");
            return refVersion.getLegaltrademark();
        }
    	return legalTrademark;
    }
    /**
     * Gets original filename.
     * @return original filename, may be null.
     */
    public String getOriginalfilename() {
        if (isReference()) {
            VersionInfo refVersion = (VersionInfo) 
				getCheckedRef(VersionInfo.class,
                    "VersionInfo");
            return refVersion.getOriginalfilename();
        }
    	return originalFilename;
    }
    /**
     * Gets private build.
     * @return private build, may be null.
     */
    public String getPrivatebuild() {
        if (isReference()) {
            VersionInfo refVersion = (VersionInfo) 
				getCheckedRef(VersionInfo.class,
                    "VersionInfo");
            return refVersion.getPrivatebuild();
        }
    	return privateBuild;
    }
    /**
     * Gets product name.
     * @return product name, may be null.
     */
    public String getProductname() {
        if (isReference()) {
            VersionInfo refVersion = (VersionInfo) 
				getCheckedRef(VersionInfo.class,
                    "VersionInfo");
            return refVersion.getProductname();
        }
    	return productName;
    }
    /**
     * Special build
     * @return special build, may be null.
     */
    public String getSpecialbuild() {
        if (isReference()) {
            VersionInfo refVersion = (VersionInfo) 
				getCheckedRef(VersionInfo.class,
                    "VersionInfo");
            return refVersion.getSpecialbuild();
        }
    	return specialBuild;
    }

    /**
     * Sets file version.
     * @param value new value
     * @throws BuildException if specified with refid
     */
    public void setFileversion(String value) throws BuildException {
        if (isReference()) {
            throw tooManyAttributes();
        }
    	fileVersion = value;
    }
    /**
     * Sets product version.
     * @param value new value
     * @throws BuildException if specified with refid
     */
    public void setProductversion(String value) throws BuildException {
        if (isReference()) {
            throw tooManyAttributes();
        }
    	productVersion = value;
    }
    /**
     * Sets compatibility version.
     * @param value new value
     * @throws BuildException if specified with refid
     */
    public void setCompatibilityversion(String value) throws BuildException {
        if (isReference()) {
            throw tooManyAttributes();
        }
    	compatibilityVersion = value;
    }
    /**
     * Sets language.
     * @param value new value, should be an IETF RFC 3066 language identifier.
     * @throws BuildException if specified with refid
     */
    public void setLanguage(String value) throws BuildException {
        if (isReference()) {
            throw tooManyAttributes();
        }
        language = value;
    }
    /**
     * Sets comments.
     * @param value new value
     * @throws BuildException if specified with refid
     */
    public void setFilecomments(String value) throws BuildException {
        if (isReference()) {
            throw tooManyAttributes();
        }
    	fileComments = value;
    }

    /**
     * Sets company name.
     * @param value new value
     * @throws BuildException if specified with refid
     */
    public void setCompanyname(String value) throws BuildException {
        if (isReference()) {
            throw tooManyAttributes();
        }
    	companyName = value;
    }


    /**
     * Sets internal name.  Internal name will automatically be
     * specified from build step, only set this value if
     * intentionally overriding that value.
     * 
     * @param value new value
     * @throws BuildException if specified with refid
     */
    public void setInternalname(String value) throws BuildException {
        if (isReference()) {
            throw tooManyAttributes();
        }
    	internalName = value;
    }
    
    /**
     * Sets legal copyright.
     * @param value new value
     * @throws BuildException if specified with refid
     */
    public void setLegalcopyright(String value) throws BuildException {
        if (isReference()) {
            throw tooManyAttributes();
        }
    	legalCopyright = value;
    }
    /**
     * Sets legal trademark.
     * @param value new value
     * @throws BuildException if specified with refid
     */
    public void setLegaltrademark(String value) throws BuildException {
        if (isReference()) {
            throw tooManyAttributes();
        }
    	legalTrademark = value;
    }
    /**
     * Sets original name.  Only set this value if
     * intentionally overriding the value from the build set.
     * 
     * @param value new value
     * @throws BuildException if specified with refid
     */
    public void setOriginalfilename(String value) throws BuildException {
        if (isReference()) {
            throw tooManyAttributes();
        }
    	originalFilename = value;
    }
    /**
     * Sets private build.
     * @param value new value
     * @throws BuildException if specified with refid
     */
    public void setPrivatebuild(String value) throws BuildException {
        if (isReference()) {
            throw tooManyAttributes();
        }
    	privateBuild = value;
    }
    /**
     * Sets product name.
     * @param value new value
     * @throws BuildException if specified with refid
     */
    public void setProductname(String value) throws BuildException {
        if (isReference()) {
            throw tooManyAttributes();
        }
    	productName= value;
    }
    /**
     * Sets private build.
     * @param value new value
     * @throws BuildException if specified with refid
     */
    public void setSpecialbuild(String value) throws BuildException {
        if (isReference()) {
            throw tooManyAttributes();
        }
    	specialBuild = value;
    }
    
    /**
     * Writes windows resource 
     * @param writer writer, may not be null.
     * @param project project, may not be null
     * @param executableName name of executable
     */
    public void writeResource(final Writer writer, 
    		final Project p, 
			final String executableName) throws IOException {
    	// TODO:
    	
    }
    
}
