  .globl  internal_FPU_rmode
  .proc internal_FPU_rmode
internal_FPU_rmode::
  // get the floating point rounding control bits
  // bits 10 and 11 are the rc bits from main status field fpsr.sf0
  mov   r8= ar.fpsr;;
  shr   r8 = r8, 10
  mov   r9 = 3;;
  and   r8 = r8, r9;;
  br.sptk.few b0

  .endp internal_FPU_rmode
