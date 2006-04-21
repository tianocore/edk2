import java.util.*;

public class Package
{
  Package()
  {
  }
  Package(String n)
  {
    name=n;
  }
  public String name;

  public Set<LibClass>     libClassDecls;
  public Set<GuidDecl>     guidDecls;
  public Set<PpiDecl>      ppiDecls;
  public Set<ProtocolDecl> protocolDecls;
  public Set<Module>       modules;
  public Set<Package>      depends;

  public void genBuild()
  {
    for(Module m : modules)
    {
      m.autoBuild();
    }
  }

  // Figure out what this package depends on based on what the modules 
  // depend on.
  public void calculateDependencies()
  {
    depends = new HashSet<Package>();
    for(Module m : modules)
    {
      depends.addAll(m.packageDeps());
    }
  }

  public void makeJar(String name) {};

  public void addModule(Module m) {};
  public void removeModule(Module m) {};
}
