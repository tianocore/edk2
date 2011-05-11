set confirm off
set output-radix 16
b SecGdbScriptBreak
command
silent
source SecMain.gdb
c
end
