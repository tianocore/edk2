package org.tianocore.context;

public class ContextMain {
    
    public static void main(String[] args) {

        if(ParseParameter.checkParameter(args) == false){
            System.exit(0);
        }

        if (TargetFile.parsePath("target.txt") == false) {
            System.exit(0);
        }
        
        System.out.printf("%n%s", "Target.txt generate successfully!");
    }
}
