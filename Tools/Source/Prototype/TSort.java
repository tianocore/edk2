import java.util.*;

public class TSort
{
  public static void main(String args[]) 
  {
    DAG<String> dag = new DAG<String>();
    int i;

    if(args.length % 2==1)
    {
      System.out.println("Error: Odd number of elements");
      return;
    }
    for(i=0; i< args.length/2; i++)
    {
      dag.add(args[i*2], args[i*2+1]);
      // System.out.println(pair.left);
      // System.out.println(pair.right);
    }
    System.out.println(dag.sort().toString());
    System.out.println(dag.sort().toString());
  }
}
