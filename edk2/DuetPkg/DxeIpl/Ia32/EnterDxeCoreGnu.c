VOID
EnterDxeMain (
  IN VOID *StackTop,
  IN VOID *DxeCoreEntryPoint,
  IN VOID *Hob,
  IN VOID *PageTable
  )
{
  __asm__ ( "movl  %0, %%esp \n\t"
            "pushl %2 \n\t"
            "pushl $0 \n\t"
            "movl  %1, %%ecx \n\t"
            "jmp  %%ecx"
            ::"q"(StackTop), "q"(DxeCoreEntryPoint), "q"(Hob)
          );
}  