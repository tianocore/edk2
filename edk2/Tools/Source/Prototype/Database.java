import java.util.*;

public class Database
{
  Database()
  {
  }
  Database(String n)
  {
    name=n;
  }
  public String name;
  Map<String,Set<Package>> packages;
}
