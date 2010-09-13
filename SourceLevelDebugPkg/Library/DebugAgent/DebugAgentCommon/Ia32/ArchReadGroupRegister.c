/** @file
  IA32 Group registers read support functions.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "DebugAgent.h"

/**
  Read group register of Segment Base.

  @param[in] CpuContext           Pointer to saved CPU context.
  @param[in] RegisterGroupSegBase Pointer to Group registers.

**/
VOID
ReadRegisterGroupSegBase (
  IN DEBUG_CPU_CONTEXT                              *CpuContext,
  IN DEBUG_DATA_REPONSE_READ_REGISTER_GROUP_SEGBASE *RegisterGroupSegBase
  )
{
  IA32_DESCRIPTOR               *Ia32Descriptor;
  IA32_GDT                      *Ia32Gdt;
  UINTN                         Index;

  Ia32Descriptor = (IA32_DESCRIPTOR *) CpuContext->Gdtr;
  Ia32Gdt = (IA32_GDT *) (Ia32Descriptor->Base);

  Index = CpuContext->Cs / 8;
  RegisterGroupSegBase->CsBas = (Ia32Gdt[Index].Bits.BaseLow) + (Ia32Gdt[Index].Bits.BaseMid << 16) + (Ia32Gdt[Index].Bits.BaseMid << 24);
  Index = CpuContext->Ss / 8;
  RegisterGroupSegBase->SsBas = (Ia32Gdt[Index].Bits.BaseLow) + (Ia32Gdt[Index].Bits.BaseMid << 16) + (Ia32Gdt[Index].Bits.BaseMid << 24);
  Index = CpuContext->Gs / 8;
  RegisterGroupSegBase->GsBas = (Ia32Gdt[Index].Bits.BaseLow) + (Ia32Gdt[Index].Bits.BaseMid << 16) + (Ia32Gdt[Index].Bits.BaseMid << 24);
  Index = CpuContext->Fs / 8;
  RegisterGroupSegBase->FsBas = (Ia32Gdt[Index].Bits.BaseLow) + (Ia32Gdt[Index].Bits.BaseMid << 16) + (Ia32Gdt[Index].Bits.BaseMid << 24);
  Index = CpuContext->Es / 8;
  RegisterGroupSegBase->EsBas = (Ia32Gdt[Index].Bits.BaseLow) + (Ia32Gdt[Index].Bits.BaseMid << 16) + (Ia32Gdt[Index].Bits.BaseMid << 24);
  Index = CpuContext->Ds / 8;
  RegisterGroupSegBase->DsBas = (Ia32Gdt[Index].Bits.BaseLow) + (Ia32Gdt[Index].Bits.BaseMid << 16) + (Ia32Gdt[Index].Bits.BaseMid << 24);

  RegisterGroupSegBase->LdtBas = 0;
  RegisterGroupSegBase->TssBas = 0;
}

/**
  Read gourp register of Segment Limit.

  @param[in] CpuContext           Pointer to saved CPU context.
  @param[in] RegisterGroupSegLim  Pointer to Group registers.

**/
VOID
ReadRegisterGroupSegLim (
  IN DEBUG_CPU_CONTEXT                             *CpuContext,
  IN DEBUG_DATA_REPONSE_READ_REGISTER_GROUP_SEGLIM *RegisterGroupSegLim
  )
{
  IA32_DESCRIPTOR               *Ia32Descriptor;
  IA32_GDT                      *Ia32Gdt;
  UINTN                         Index;

  Ia32Descriptor = (IA32_DESCRIPTOR *) CpuContext->Gdtr;
  Ia32Gdt = (IA32_GDT *) (Ia32Descriptor->Base);

  Index = CpuContext->Cs / 8;
  RegisterGroupSegLim->CsLim = Ia32Gdt[Index].Bits.LimitLow + (Ia32Gdt[Index].Bits.LimitHigh << 16);
  if (Ia32Gdt[Index].Bits.Granularity == 1) {
    RegisterGroupSegLim->CsLim = (RegisterGroupSegLim->CsLim << 12) | 0xfff;
  }

  Index = CpuContext->Ss / 8;
  RegisterGroupSegLim->SsLim = Ia32Gdt[Index].Bits.LimitLow + (Ia32Gdt[Index].Bits.LimitHigh << 16);
  if (Ia32Gdt[Index].Bits.Granularity == 1) {
    RegisterGroupSegLim->SsLim = (RegisterGroupSegLim->SsLim << 12) | 0xfff;
  }

  Index = CpuContext->Gs / 8;
  RegisterGroupSegLim->GsLim = Ia32Gdt[Index].Bits.LimitLow + (Ia32Gdt[Index].Bits.LimitHigh << 16);
  if (Ia32Gdt[Index].Bits.Granularity == 1) {
    RegisterGroupSegLim->GsLim = (RegisterGroupSegLim->GsLim << 12) | 0xfff;
  }

  Index = CpuContext->Fs / 8;
  RegisterGroupSegLim->FsLim = Ia32Gdt[Index].Bits.LimitLow + (Ia32Gdt[Index].Bits.LimitHigh << 16);
  if (Ia32Gdt[Index].Bits.Granularity == 1) {
    RegisterGroupSegLim->FsLim = (RegisterGroupSegLim->FsLim << 12) | 0xfff;
  }

  Index = CpuContext->Es / 8;
  RegisterGroupSegLim->EsLim = Ia32Gdt[Index].Bits.LimitLow + (Ia32Gdt[Index].Bits.LimitHigh << 16);
  if (Ia32Gdt[Index].Bits.Granularity == 1) {
    RegisterGroupSegLim->EsLim = (RegisterGroupSegLim->EsLim << 12) | 0xfff;
  }

  Index = CpuContext->Ds / 8;
  RegisterGroupSegLim->DsLim = Ia32Gdt[Index].Bits.LimitLow + (Ia32Gdt[Index].Bits.LimitHigh << 16);
  if (Ia32Gdt[Index].Bits.Granularity == 1) {
    RegisterGroupSegLim->DsLim = (RegisterGroupSegLim->DsLim << 12) | 0xfff;
  }

  RegisterGroupSegLim->LdtLim = 0xffff;
  RegisterGroupSegLim->TssLim = 0xffff;
}

/**
  Read group register by group index.

  @param[in] CpuContext           Pointer to saved CPU context.
  @param[in] GroupIndex           Group Index.

  @retval RETURN_SUCCESS         Read successfully.
  @retval RETURN_NOT_SUPPORTED   Group index cannot be supported.

**/
RETURN_STATUS
ArchReadRegisterGroup (
  IN DEBUG_CPU_CONTEXT                             *CpuContext,
  IN UINT8                                         GroupIndex
  )
{
  DEBUG_DATA_REPONSE_READ_REGISTER_GROUP  RegisterGroup;
  DEBUG_DATA_REPONSE_READ_REGISTER_GROUP_SEGLIM   RegisterGroupSegLim;
  DEBUG_DATA_REPONSE_READ_REGISTER_GROUP_SEGBASE  RegisterGroupSegBase;

  switch (GroupIndex) {
  case SOFT_DEBUGGER_REGISTER_GROUP_GPDRS32:
    ReadRegisterGroup (CpuContext, &RegisterGroup);
    SendDataResponsePacket (CpuContext, (UINT8 *) &RegisterGroup, (UINT16) sizeof (DEBUG_DATA_REPONSE_READ_REGISTER_GROUP));
    break;

  case SOFT_DEBUGGER_REGISTER_GROUP_SEGMENT_LIMITS32:
    ReadRegisterGroupSegLim (CpuContext, &RegisterGroupSegLim);
    SendDataResponsePacket (CpuContext, (UINT8 *) &RegisterGroupSegLim, (UINT16) sizeof (DEBUG_DATA_REPONSE_READ_REGISTER_GROUP_SEGLIM));
    break;

  case SOFT_DEBUGGER_REGISTER_GROUP_SEGMENT_BASES32:
    ReadRegisterGroupSegBase (CpuContext, &RegisterGroupSegBase);
    SendDataResponsePacket (CpuContext, (UINT8 *) &RegisterGroupSegBase, (UINT16) sizeof (DEBUG_DATA_REPONSE_READ_REGISTER_GROUP_SEGBASE));
    break;

  default:
    return RETURN_UNSUPPORTED;
  }

  return RETURN_SUCCESS;
}

/**
  Read segment selector by register index.

  @param[in] CpuContext           Pointer to saved CPU context.
  @param[in] RegisterIndex        Register Index.

  @return Value of segment selector.

**/
UINT64
ReadRegisterSelectorByIndex (
  IN DEBUG_CPU_CONTEXT                       *CpuContext,
  IN UINT8                                   RegisterIndex
  )
{
  IA32_DESCRIPTOR               *Ia32Descriptor;
  IA32_GDT                      *Ia32Gdt;
  UINT16                        Selector;
  UINT32                        Data32;

  Ia32Descriptor = (IA32_DESCRIPTOR *) CpuContext->Gdtr;
  Ia32Gdt = (IA32_GDT *) (Ia32Descriptor->Base);

  Selector = 0;

  switch (RegisterIndex) {
  case SOFT_DEBUGGER_REGISTER_CSAS:
    Selector = (UINT16) CpuContext->Cs;
    break;
  case SOFT_DEBUGGER_REGISTER_SSAS:
    Selector = (UINT16) CpuContext->Ss;
    break;
  case SOFT_DEBUGGER_REGISTER_GSAS:
    Selector = (UINT16) CpuContext->Gs;
    break;
  case SOFT_DEBUGGER_REGISTER_FSAS:
    Selector = (UINT16) CpuContext->Fs;
    break;
  case SOFT_DEBUGGER_REGISTER_ESAS:
    Selector = (UINT16) CpuContext->Es;
    break;
  case SOFT_DEBUGGER_REGISTER_DSAS:
    Selector = (UINT16) CpuContext->Ds;
  case SOFT_DEBUGGER_REGISTER_LDTAS:
  case SOFT_DEBUGGER_REGISTER_TSSAS:
    return 0x00820000;
    break;
  }

  Data32 = (UINT32) RShiftU64 (Ia32Gdt[Selector / 8].Uint64, 24);
  return (Data32 & (UINT32)(~0xff)) | Selector;

}

