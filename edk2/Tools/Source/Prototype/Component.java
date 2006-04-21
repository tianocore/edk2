import java.util.*;

public class Component extends Module
{
  Component()
  {
  }
  Component(String n)
  {
    name=n;
  }
  String name;

  // These are the libs we want to build with.
  public Set<LibInst> buildLibs;

  public String name() { return name; }

  public boolean autoBuild()
  {
    // buildLibs must contain a list of libInstances. We need to check that
    // These libs meet certain criterea.
    if(!duplicateLibClasses(buildLibs).isEmpty())
    {
      // Error: The lib instance implement the same lib twice.
      return false;
    }
    if(! libClassesProduced(buildLibs).containsAll(consumesLibClasses))
    {
      // We can not cover the libclasses consumed with these instances.
      return false;
    }
    getConstructorOrder(buildLibs);
    getDestructorOrder(buildLibs);

    // Get PPI, Protocol, GUID, PCDs from the lib instances. These are all simple unions of
    // the corresponding sets in the modules. There is no ordering needed.
    // TODO

    return true;
  }

}
