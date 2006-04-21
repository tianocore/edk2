import java.util.*;

// A directed Acyclic Graph class. The main purpose is to provide a set of nodes
// and the dependency relations between them.  Then ask for a topological sort of
// the nodes.

public class DAG<Node>
{
  // Constructor.
  DAG()
  {
    children = new HashMap<Node,Set<Node>>();
  }

  public Set<Node> nodes() { return children.keySet(); }
  public Set<Node> children(Node parent) { return children.get(parent); }

  // Add the relations from a compatible DAG to this one.
  public void add(DAG<Node> newDag)
  {
    for(Node parent : newDag.children.keySet())
    {
      children.put(parent, newDag.children(parent));
    }
  }

  // The central data structure is a one-to-many map. Each node is 
  // treated as a parent. It is mapped to a list of its children. Leaf
  // nodes are also treated as parents and map to an empty list of
  // children.
  Map<Node,Set<Node>> children;

  public void remove(Collection<Node> nodes)
  {
    // Remove it as a parent
    for(Node node : nodes)
    {
      children.remove(node);
    }

    for(Set<Node> childlist : children.values())
    {
      // Remove it as a child
      childlist.removeAll(nodes);
    }
  }

  // Remove every occurrence of node from the DAG.
  public void remove(Node node)
  {
    // Remove it as a parent
    children.remove(node);

    for(Set<Node> childlist : children.values())
    {
      // Remove it as a child
      childlist.remove(node);
    }
  }

  // Return true iff parent is a direct parent of child.
  public boolean directDepends(Node parent, Node child)
  {
    return children.containsKey(parent) ? 
           children(parent).contains(child) :
           false;
  }

  // Return true iff parent is a direct or indirect parent of child.
  // This is the transitive closure of the dependency relation.
  public boolean depends(Node parent, Node child)
  {
    if(!children.containsKey(parent))
    {
      return false;
    }
    if( directDepends(parent, child) )
    {
      return true;
    }
    else
    {
      for(Node descendent : children(parent) )
      {
        // Recursively call depends() to compute the transitive closure of
        // the relation.
        if(depends(descendent, child))
        {
          return true;
        }
      }
      return false;
    } 
  }

  // Add a parent child relation to the dag. Fail if there is already
  // a dependency from the child to the parent. This implies a cycle.
  // Our invariant is that the DAG must never contain a cycle. That
  // way it lives up to its name.
  public void add(Node parent, Node child)
  {
    if(depends(child, parent))
    {
      System.out.format("Error: There is a cycle from %s to %s.\n", parent, child);
      return;
    }
    if(children.containsKey(parent))
    {
      children(parent).add(child);
    }
    else
    {
      Set<Node> cs = new HashSet<Node>();
      cs.add(child);
      children.put(parent, cs);
    }
    if(!children.containsKey(child))
    {
      children.put(child,new HashSet<Node>());
    }
  }

  // Perform a topological sort on the DAG.
  public List<Node> sort()
  {
    // Make an ordered list to hold the topo sort.
    List<Node> sorted = new LinkedList<Node>();

    // We add the leaves to the beginning of the list until
    // the sorted list contains all the nodes in the DAG.
    while(!sorted.containsAll(nodes()))
    {
      // Ignoring the ones we have found, what are the leaves of this
      // DAG?
      Set<Node> leaves = leaves(sorted);
      // Put the new leaves at the beginning of the list.
      sorted.addAll(0, leaves);
    }
    return sorted;
  }

  // Return the set of nodes that have no children. Pretend
  // the nodes in the exclude list are not present.
  public Set<Node> leaves(Collection<Node> exclude)
  {
    Set<Node> leaves=new HashSet<Node>();
    for(Node parent : children.keySet())
    {
      if(exclude.contains(parent))
      {
        continue;
      }
      // If the children of parent are a subset of the exclude set,
      // then parent is a leaf.
      if(exclude.containsAll(children(parent)))
      {
        leaves.add(parent);
      }
    }
    return leaves;
  }

  // Return the set of nodes that have no children.
  public Set<Node> leaves()
  {
    Set<Node> leaves=new HashSet<Node>();
    for(Node parent : children.keySet())
    {
      if( children(parent).isEmpty())
      {
        leaves.add(parent);
      }
    }
    return leaves;
  }
}
