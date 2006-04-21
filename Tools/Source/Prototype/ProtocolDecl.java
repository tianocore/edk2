import java.util.*;

public class ProtocolDecl
{
  ProtocolDecl()
  {
  }
  ProtocolDecl(String n)
  {
    name=n;
  }
  public String name;
  public String cName;
  public String guid;
  public String name() { return name; }
}
