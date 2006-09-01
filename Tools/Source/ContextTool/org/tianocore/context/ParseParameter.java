package org.tianocore.context;

public class ParseParameter {

    
    /** 
     * check the validity of user's input args
     * @param args -- user's input
     * @return true or false
     **/
    public static boolean checkParameter(String[] args) {
        
        if(args.length == 0){
            HelpInfo.outputUsageInfo();
            return false;
        } else {
            if( args[0].charAt(0) != '-' ){
                HelpInfo.outputUsageInfo();
                return false;
            }
            for(int i=0; i<args.length; i++){
                if( (args[i].compareToIgnoreCase("-h") == 0) || 
                    (args[i].startsWith("-") && ((args[i].charAt(1) != 'a') && (args[i].charAt(1) != 'c') 
                    && (args[i].charAt(1) != 'n') && (args[i].charAt(1) != 'p') && (args[i].charAt(1) != 't')))){
                    HelpInfo.outputUsageInfo();
                    return false;
                }
            }
        }
        
        standardizeParameter(args);
        return true; 
    }
    
    /** 
     * standardize user's input args
     * @param args -- user's input
     * @return no return value
     **/
    private static void standardizeParameter(String[] args) {
        
        length  = pstr.length();
        
        StringBuffer InputData = new StringBuffer();
        for (int i = 0; i < args.length; i++) {
            InputData.append(args[i]);
            InputData.append(" ");
        }

        int i = 0;
        while (i < InputData.length()) {
            int j = InputData.indexOf("-", i + 1);
            if (j == -1)
                j = InputData.length();

            String argstr = InputData.substring(i, j);

            if (argstr.charAt(1) == 'p') {
                pstr += argstr.substring(2);
 //               pstr += "\n";
            } else if (argstr.charAt(1) == 't') {
                tstr += argstr.substring(2);
 //               tstr += "\n";
            } else if (argstr.charAt(1) == 'a') {
                astr += argstr.substring(2);
//                astr += "\n";
            } else if (argstr.charAt(1) == 'c') {
                cstr += argstr.substring(2);
//                cstr += "\n";
            } else if (argstr.charAt(1) == 'n') {
                nstr += argstr.substring(2);
//                nstr += "\n";
            }
            i = j;
        }

    }
     
    public static int length  = 0;
    public static String pstr = new String("ACTIVE_PLATFORM       = ");
    public static String tstr = new String("TARGET                = ");
    public static String astr = new String("TARGET_ARCH           = ");
    public static String cstr = new String("TOOL_CHAIN_CONF       = ");
    public static String nstr = new String("TOOL_CHAIN_TAG        = ");

}
