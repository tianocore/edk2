package org.tianocore.frameworkwizard.far;

import java.io.Reader;
import java.util.Map;
import java.util.Set;

import org.tianocore.frameworkwizard.packaging.PackageIdentification;
import org.tianocore.frameworkwizard.platform.PlatformIdentification;

public interface FarInterface {

    public Reader getManifestFile();

    public void hibernateToFile();

    public boolean extract(Map<PackageIdentification, String> packagePathes,
                           Map<PlatformIdentification, String> platformPathes);

    public Set<PackageIdentification> getPackageDependencies(PackageIdentification packageId);

}
