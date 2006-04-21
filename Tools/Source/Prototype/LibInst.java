import java.util.*;

public class LibInst extends Module
{
  LibInst()
  {
  }
  LibInst(String n)
  {
    name=n;
  }

  public Set<LibClass> producesLibClasses;

  public String constructorName, destructorName;

  public boolean autoBuild()
  {
    // A simple compile, without link.
    return true;
  }
}
