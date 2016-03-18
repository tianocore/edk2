
=== HOW TO USE VTF0 ===

Add this line to your FDF FV section:
INF  RuleOverride=RESET_VECTOR USE = IA32 UefiCpuPkg/ResetVector/Vtf0/Bin/ResetVector.inf
(For X64 SEC/PEI change IA32 to X64 => 'USE = X64')

In your FDF FFS file rules sections add:
[Rule.Common.SEC.RESET_VECTOR]
  FILE RAW = $(NAMED_GUID) {
    RAW RAW                |.raw
  }

=== VTF0 Boot Flow ===

1. Transition to IA32 flat mode
2. Locate BFV (Boot Firmware Volume) by checking every 4kb boundary
3. Locate SEC image
4. X64 VTF0 transitions to X64 mode
5. Call SEC image entry point

== VTF0 SEC input parameters ==

All inputs to SEC image are register based:
EAX/RAX - Initial value of the EAX register (BIST: Built-in Self Test)
DI      - 'BP': boot-strap processor, or 'AP': application processor
EBP/RBP - Pointer to the start of the Boot Firmware Volume

=== HOW TO BUILD VTF0 ===

Dependencies:
* Python 2.5~2.7
* Nasm 2.03 or newer

To rebuild the VTF0 binaries:
1. Change to VTF0 source dir: UefiCpuPkg/ResetVector/Vtf0
2. nasm and python should be in executable path
3. Run this command:
   python Build.py
4. Binaries output will be in UefiCpuPkg/ResetVector/Vtf0/Bin

