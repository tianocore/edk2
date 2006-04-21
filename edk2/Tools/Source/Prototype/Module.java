import java.util.*;

public class Module
{
  Module()
  {
  }
  Module(String n)
  {
    name=n;
  }
  String name;

  public String name() { return name; }

  public Set<LibClass> consumesLibClasses;

  // The set of packages that this module depends upon.
  Set<Package> packageDepends;
  public Set<Package> packageDeps() { return packageDepends; }

  public boolean autoBuild()
  {
    // This should be implemented in the derived class.
    return true;
  }

  // Make sure that each class in this set of libclasses is declared in one
  // of the packages that this module depends on.
  public boolean validateLibClasses(Set<LibClass> classes)
  {
    for(LibClass lc : classes)
    {
      // Assume we will not find it.
      boolean found = false;

      for(Package p : packageDepends)
      {
        if(p.libClassDecls.contains(lc))
        {
          found=true;
          break;
        }
      }
      if(found == false)
      {
        // Error: This LibClass is not found in any of our Packages.
        return false;
      }
    }
    // Well, we never came up empty handed, so it looks good.
    return true;
  }

  public Set<LibClass> libClassesProduced(Collection<LibInst> instances)
  {
    // given a set of lib instances, what is the set of lib classes produced?

    Set<LibClass> classes = new HashSet<LibClass>();

    for(LibInst li : instances)
    {
      classes.addAll(li.producesLibClasses);
    }
    return classes;
  }

  // Search the given set of lib instance to see if, among them, they
  // produce the same LibClass more than once.
  public Set<LibClass> duplicateLibClasses(Set<LibInst> libs)
  {
    // Return true iff each class produced is produced only once.

    List<LibClass> classes = new LinkedList<LibClass>();
    Set<LibClass> dups = new HashSet<LibClass>();

    for(LibInst li : libs)
    {
      classes.addAll(li.producesLibClasses);
    }

    for(LibClass c : classes)
    {
      for(LibClass inner : classes)
      {
        if(c.equals(inner))
        {
          dups.add(c);
        }
      }
    }
    return dups;
  }

  public Set<LibInst> getProducers(LibClass lc, Set<LibInst> libs)
  {
    // Return the subset of the given libs that produce this LibClass.

    Set<LibInst> producers = new HashSet<LibInst>();

    for(LibInst li : libs)
    {
      if(li.producesLibClasses.contains(lc))
      {
        producers.add(li);
      }
    }
    return producers;
  }
 
  // 
  // The central dependency relationship between library instances is as follows.
  // A LibInst "A" depends upon LibInst "B" if, and only if, there exists a LibClass
  // "C" such that A consumes C and B produces C. This is the partial order over which 
  // we construct a Directed Acyclic Graph (DAG). The DAG can be used to detect
  // cycles in the depends relation (which are illegal) and it can be used to implement a
  // topological sort which is a total ordering over LibInstances. This total order on 
  // lib instances is what is needed in order to call the constructors and destructors
  // in the proper sequence.
  //
  public DAG<LibInst> makeDAG(Set<LibInst> libs)
  {
    DAG<LibInst> dag = new DAG<LibInst>();

    if(duplicateLibClasses(libs).size()>0)
    {
      System.out.format("Error: The library instances implement at least one "
        + "library class more than once.\n");
    }

    for(LibInst consumer : libs)
    {
      // Find all the producers for each LC that li consumes. 
      for(LibClass lc : consumer.consumesLibClasses )
      {
        Set<LibInst> producers = getProducers(lc, libs);
        if(producers.isEmpty())
        {
          System.out.format("Error: Unmet dependency libclass:%s .", lc.name() );
          return null;
        }

        // There is exactly one lib inst that produces this class.
        LibInst producer = producers.iterator().next();

        // Now we are ready to add the dependency to the dag. It will flag  
        // circular dependencies for us.
        dag.add(consumer, producer);
      }
    }
    return dag;
  }

  // As you evaluate each node in the graph (starting with the module node), you
  // must call the constructors for all the child nodes before you call the
  // constructor for the current node.   
  public List<LibInst> getConstructorOrder(Set<LibInst> libs)
  {
    List<LibInst> rev = new LinkedList<LibInst>();

    for(LibInst li : getDestructorOrder(libs))
      rev.add(0, li);    

    return rev;
  }

  // The destructor order is exactly the reverese of the constructor order.
  // As you evaluate each node in the graph (starting with the module node), you
  // must call the destructor for the all the parent nodes before calling the
  // destructors for the current node, and then call the destructors for all the
  // child nodes. 
  public List<LibInst> getDestructorOrder(Set<LibInst> libs)
  {
    DAG<LibInst> dag = makeDAG(libs);

    return dag.sort();
  }
}
