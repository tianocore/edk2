/** @file
  Main PAL API's defined in IPF PAL Spec.

  Copyright (c) 2006 - 2007, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  PalApi.h

**/

#ifndef __PAL_API_H__
#define __PAL_API_H__

//
// IPF Specific Functions
//
#ifdef _MSC_VER
//
// Disabling bitfield type checking warnings.
//
#pragma warning ( disable : 4214 )
#endif
  
typedef struct {
  UINT64                    Status;
  UINT64                    r9;
  UINT64                    r10;
  UINT64                    r11;
} PAL_CALL_RETURN;



//
// CacheType of PAL_CACHE_FLUSH.
//
#define PAL_CACHE_FLUSH_INSTRUCTION_ALL 	1
#define PAL_CACHE_FLUSH_DATA_ALL 					2
#define PAL_CACHE_FLUSH_ALL								3
#define PAL_CACHE_FLUSH_SYNC_TO_DATA   		4


//
// Bitmask of Opearation of PAL_CACHE_FLUSH.
// 
#define PAL_CACHE_FLUSH_INVIDED_LINES			BIT0
#define PAL_CACHE_FLUSH_PROBE_INTERRUPT		BIT1

/**
   
  Flush the instruction or data caches. It is required by IPF.
  The PAL procedure supports the Static Registers calling
  convention. It could be called at virtual mode and physical
  mode.

  @param Index 							Index of PAL_CACHE_FLUSH within the
														list of PAL procedures.
  
  @param CacheType					Unsigned 64-bit integer indicating
														which cache to flush.

  @param Operation 					Formatted bit vector indicating the
														operation of this call.

  @param ProgressIndicator  Unsigned 64-bit integer specifying
                            the starting position of the flush
                            operation.
  
  @return R9			Unsigned 64-bit integer specifying the vector
									number of the pending interrupt.
  
  @return R10			Unsigned 64-bit integer specifying the
                  starting position of the flush operation.
  
  @return R11			Unsigned 64-bit integer specifying the vector
									number of the pending interrupt.
  
  @return Status	2 - Call completed without error, but a PMI
                  was taken during the execution of this
                  procedure.

  @return Status	1 - Call has not completed flushing due to
                  a pending interrupt.

  @return Status  0 - Call completed without error

  @return Status  -2 - Invalid argument

  @return Status  -3 - Call completed with error
   
**/
#define PAL_CACHE_FLUSH 	1


//
// Attributes of PAL_CACHE_CONFIG_INFO1
// 
#define PAL_CACHE_ATTR_WT 	0
#define PAL_CACHE_ATTR_WB		1

//
// PAL_CACHE_CONFIG_INFO1.StoreHint
// 
#define PAL_CACHE_STORE_TEMPORAL			0
#define PAL_CACHE_STORE_NONE_TEMPORAL	3

//
// PAL_CACHE_CONFIG_INFO1.StoreHint
// 
#define PAL_CACHE_STORE_TEMPORAL_LVL_1				0
#define PAL_CACHE_STORE_NONE_TEMPORAL_LVL_ALL	3

//
// PAL_CACHE_CONFIG_INFO1.StoreHint
// 
#define PAL_CACHE_LOAD_TEMPORAL_LVL_1					0
#define PAL_CACHE_LOAD_NONE_TEMPORAL_LVL_1		1
#define PAL_CACHE_LOAD_NONE_TEMPORAL_LVL_ALL	3

//
// Detail the characteristics of a given processor controlled
// cache in the cache hierarchy.
// 
typedef struct {
	UINT64	IsUnified 	: 1;
	UINT64	Attributes	:	2;
	UINT64	Associativity:8;
	UINT64	LineSize:8;
	UINT64	Stride:8;
	UINT64	StoreLatency:8;
	UINT64	StoreHint:8;
	UINT64	LoadHint:8;
} PAL_CACHE_INFO_RETURN1;

//
// Detail the characteristics of a given processor controlled
// cache in the cache hierarchy.
// 
typedef struct {
	UINT64	CacheSize:32;
	UINT64	AliasBoundary:8;
	UINT64	TagLsBits:8;
	UINT64	TagMsBits:8;
} PAL_CACHE_INFO_RETURN2;

/**
   
  Return detailed instruction or data cache information. It is
  required by IPF. The PAL procedure supports the Static
  Registers calling convention. It could be called at virtual
  mode and physical mode.
  
  @param Index 				Index of PAL_CACHE_INFO within the list of
											PAL procedures.
  
  @param CacheLevel		Unsigned 64-bit integer specifying the
                      level in the cache hierarchy for which
                      information is requested. This value must
                      be between 0 and one less than the value
                      returned in the cache_levels return value
                      from PAL_CACHE_SUMMARY.
  
  @param CacheType    Unsigned 64-bit integer with a value of 1
                      for instruction cache and 2 for data or
                      unified cache. All other values are
                      reserved.
  
  @param Reserved 		Should be 0.
  
  
  @return R9			Detail the characteristics of a given
                  processor controlled cache in the cache
                  hierarchy. See PAL_CACHE_INFO_RETURN1.
  
  @return R10			Detail the characteristics of a given
                  processor controlled cache in the cache
                  hierarchy. See PAL_CACHE_INFO_RETURN2.
  
  @return R11			Reserved with 0.
  
  
  @return Status  0 - Call completed without error

  @return Status  -2 - Invalid argument

  @return Status  -3 - Call completed with error
   
**/
#define PAL_CACHE_INFO 		2



//
// Level of PAL_CACHE_INIT.
// 
#define PAL_CACHE_INIT_ALL	0xffffffffffffffffULL

//
// Restrict of PAL_CACHE_INIT.
// 
#define PAL_CACHE_INIT_NO_RESTRICT	0
#define PAL_CACHE_INIT_RESTRICTED		1

/**
   
  Initialize the instruction or data caches. It is required by
  IPF. The PAL procedure supports the Static Registers calling
  convention. It could be called at physical mode.

  @param Index 	Index of PAL_CACHE_INIT within the list of PAL
                procedures.
  
  @param Level 	Unsigned 64-bit integer containing the level of
                cache to initialize. If the cache level can be
                initialized independently, only that level will
                be initialized. Otherwise
                implementation-dependent side-effects will
                occur.
  
  @param CacheType 	Unsigned 64-bit integer with a value of 1 to
                    initialize the instruction cache, 2 to
                    initialize the data cache, or 3 to
                    initialize both. All other values are
                    reserved.

  @param Restrict 	Unsigned 64-bit integer with a value of 0 or
                    1. All other values are reserved. If
                    restrict is 1 and initializing the specified
                    level and cache_type of the cache would
                    cause side-effects, PAL_CACHE_INIT will
                    return -4 instead of initializing the cache.
 
  
  @return Status  0 - Call completed without error

  @return Status  -2 - Invalid argument

  @return Status  -3 - Call completed with error.
  
  @return Status  -4 - Call could not initialize the specified
                  level and cache_type of the cache without
									side-effects and restrict was 1.  
   
**/
#define PAL_CACHE_INIT 		3 


//
// PAL_CACHE_PROTECTION.Method.
// 
#define PAL_CACHE_PROTECTION_NONE_PROTECT		0
#define PAL_CACHE_PROTECTION_ODD_PROTECT		1
#define PAL_CACHE_PROTECTION_EVEN_PROTECT		2
#define PAL_CACHE_PROTECTION_ECC_PROTECT		3



//
// PAL_CACHE_PROTECTION.TagOrData.
// 
#define PAL_CACHE_PROTECTION_PROTECT_DATA		0
#define PAL_CACHE_PROTECTION_PROTECT_TAG		1
#define PAL_CACHE_PROTECTION_PROTECT_TAG_ANDTHEN_DATA		2
#define PAL_CACHE_PROTECTION_PROTECT_DATA_ANDTHEN_TAG		3

//
// 32-bit protection information structures.
// 
typedef struct {
	UINT32 	DataBits:8;
	UINT32 	TagProtLsb:6;
	UINT32	TagProtMsb:6;
	UINT32	ProtBits:6;
	UINT32	Method:4;
	UINT32	TagOrData:2;
} PAL_CACHE_PROTECTION;

/**
   
  Return instruction or data cache protection information. It is
  required by IPF. The PAL procedure supports the Static
  Registers calling convention. It could be called at physical
  mode and Virtual mode.

  @param Index 	Index of PAL_CACHE_PROT_INFO within the list of
                PAL procedures.

  @param CacheLevel	Unsigned 64-bit integer specifying the level
                    in the cache hierarchy for which information
                    is requested. This value must be between 0
                    and one less than the value returned in the
                    cache_levels return value from
                    PAL_CACHE_SUMMARY.

  @param CacheType	Unsigned 64-bit integer with a value of 1
                    for instruction cache and 2 for data or
                    unified cache. All other values are
                    reserved.
  
  @return R9			Detail the characteristics of a given
                  processor controlled cache in the cache
                  hierarchy. See PAL_CACHE_PROTECTION[0..1].
  
  @return R10			Detail the characteristics of a given
                  processor controlled cache in the cache
                  hierarchy. See PAL_CACHE_PROTECTION[2..3].
  
  @return R11			Detail the characteristics of a given
                  processor controlled cache in the cache
                  hierarchy. See PAL_CACHE_PROTECTION[4..5].
  
  
  @return Status  0 - Call completed without error

  @return Status  -2 - Invalid argument

  @return Status  -3 - Call completed with error.
   
**/
#define PAL_CACHE_PROT_INFO 		38







///
// ?????????



/**
   
  Returns information on which logical processors share caches.
  It is optional.

  @param CallingConvention  Static Registers

  @param Mode               Physical/Virtual
   
**/
#define PAL_CACHE_SHARED_INFO 	43


/**
   
  Return a summary of the cache hierarchy. It is required by
  IPF.

  @param CallingConvention  Static Registers

  @param Mode               Physical/Virtual
   
**/
#define PAL_CACHE_SUMMARY		4

/**
   
  Return a list of supported memory attributes.. It is required
  by IPF.

  @param CallingConvention  Static Registers

  @param Mode               Physical/Virtual
   
**/
#define PAL_MEM_ATTRIB 			5

/**
   
  Used in architected sequence to transition pages from a
  cacheable, speculative attribute to an uncacheable attribute.
  It is required by IPF.

  @param CallingConvention  Static Registers

  @param Mode               Physical/Virtual
   
**/
#define PAL_PREFETCH_VISIBILITY 	41

/**
   
  Return information needed for ptc.e instruction to purge
  entire TC. It is required by IPF.

  @param CallingConvention  Static Registers

  @param Mode               Physical/Virtual
   
**/
#define PAL_PTCE_INFO 		6

/**
   
  Return detailed information about virtual memory features
  supported in the processor. It is required by IPF.

  @param CallingConvention  Static Registers

  @param Mode               Physical/Virtual
   
**/
#define PAL_VM_INFO 			7


/**
   
  Return virtual memory TC and hardware walker page sizes
  supported in the processor. It is required by IPF.

  @param CallingConvention  Static Registers

  @param Mode               Physical
   
**/
#define PAL_VM_PAGE_SIZE 34

/**
   
  Return summary information about virtual memory features
  supported in the processor. It is required by IPF.

  @param CallingConvention  Static Registers

  @param Mode               Physical/Virtual
   
**/
#define PAL_VM_SUMMARY 	8

/**
   
  Read contents of a translation register. It is required by
  IPF.

  @param CallingConvention  Stacked Register

  @param Mode               Physical
   
**/
#define PAL_VM_TR_READ 	261 

/**
   
  Return configurable processor bus interface features and their
  current settings. It is required by IPF.

  @param CallingConvention  Static Registers

  @param Mode               Physical
   
**/
#define PAL_BUS_GET_FEATURES 9


/**
   
  Enable or disable configurable features in processor bus
  interface. It is required by IPF.

  @param CallingConvention  Static Registers

  @param Mode               Physical
   
**/
#define PAL_BUS_SET_FEATURES 10


/**
   
  Return the number of instruction and data breakpoint
  registers. It is required by IPF.

  @param CallingConvention  Static Registers

  @param Mode               Physical/Virtual
   
**/
#define PAL_DEBUG_INFO 	11

/**
   
  Return the fixed component of a processor¡¯s directed address.
  It is required by IPF.

  @param CallingConvention  Static Registers

  @param Mode               Physical/Virtual
   
**/
#define PAL_FIXED_ADDR 12

/**
   
  Return the frequency of the output clock for use by the
  platform, if generated by the processor. It is optinal.

  @param CallingConvention  Static Registers

  @param Mode               Physical/Virtual
   
**/
#define PAL_FREQ_BASE 13

/**
   
  Return ratio of processor, bus, and interval time counter to
  processor input clock or output clock for platform use, if
  generated by the processor. It is required by IPF.

  @param CallingConvention  Static Registers

  @param Mode               Physical/Virtual
   
**/
#define PAL_FREQ_RATIOS 14

/**
   
  Return information on which logical processors map to a
  physical processor die. It is optinal.

  @param CallingConvention  Static Registers

  @param Mode               Physical/Virtual
   
**/
#define PAL_LOGICAL_TO_PHYSICAL 42

/**
   
  Return the number and type of performance monitors. It is
  required by IPF.

  @param CallingConvention  Static Registers

  @param Mode               Physical/Virtual
   
**/
#define PAL_PERF_MON_INFO 15

/**
   
  Specify processor interrupt block address and I/O port space
  address. It is required by IPF.

  @param CallingConvention  Static Registers

  @param Mode               Physical/Virtual
   
**/
#define PAL_PLATFORM_ADDR 16


/**
   
  Return configurable processor features and their current
  setting. It is required by IPF.

  @param CallingConvention  Static Registers

  @param Mode               Physical/Virtual
   
**/
#define PAL_PROC_GET_FEATURES 17


/**
   
  Enable or disable configurable processor features. It is
  required by IPF.

  @param CallingConvention  Static Registers

  @param Mode               Physical
   
**/
#define PAL_PROC_SET_FEATURES 18

/**
   
  Return AR and CR register information. It is required by IPF.

  @param CallingConvention  Static Registers

  @param Mode               Physical/Virtual
   
**/
#define PAL_REGISTER_INFO 39 

/**
   
  Return RSE information. It is required by
  IPF.

  @param CallingConvention  Static Registers

  @param Mode               Physical/Virtual
   
**/
#define PAL_RSE_INFO 19

/**
   
  Return version of PAL code. It is required by IPF.

  @param CallingConvention  Static Registers

  @param Mode               Physical/Virtual
   
**/
#define PAL_VERSION 20

/**
   
  Clear all error information from processor error logging
  registers. It is required by IPF.

  @param CallingConvention  Static Registers

  @param Mode               Physical/Virtual
   
**/
#define PAL_MC_CLEAR_LOG 21 

/**
   
  Ensure that all operations that could cause an MCA have
  completed. It is required by IPF.

  @param CallingConvention  Static Registers

  @param Mode               Physical/Virtual
   
**/
#define PAL_MC_DRAIN 22

/**
   
  Return Processor Dynamic State for logging by SAL. It is
  optional.

  @param CallingConvention  Static Registers

  @param Mode               Physical
   
**/
#define PAL_MC_DYNAMIC_STATE 24 

/**
   
  Return Processor Machine Check Information and Processor
  Static State for logging by SAL. It is required by IPF.

  @param CallingConvention  Static Registers

  @param Mode               Physical/Virtual
   
**/
#define PAL_MC_ERROR_INFO 25 Req. Static Both 

/**
   
  Set/Reset Expected Machine Check Indicator. It is required by
  IPF.

  @param CallingConvention  Static Registers

  @param Mode               Physical
   
**/
#define PAL_MC_EXPECTED 23 

/**
   
  Register min-state save area with PAL for machine checks and
  inits. It is required by IPF.

  @param CallingConvention  Static Registers

  @param Mode               Physical
   
**/
#define PAL_MC_REGISTER_MEM 27 

/**
   
  Restore minimal architected state and return to interrupted
  process. It is required by IPF.

  @param CallingConvention  Static Registers

  @param Mode               Physical
   
**/
#define PAL_MC_RESUME 26 

/**
   
  Enter the low-power HALT state or an implementation-dependent
  low-power state. It is optinal.

  @param CallingConvention  Static Registers

  @param Mode               Physical
   
**/
#define PAL_HALT 28


/**
   
  Return the low power capabilities of the processor. It is
  required by IPF.

  @param CallingConvention  Stacked Register

  @param Mode               Physical/Virtual
   
**/
#define PAL_HALT_INFO 257


/**
   
  Enter the low power LIGHT HALT state. It is required by
  IPF.

  @param CallingConvention  Static Registers

  @param Mode               Physical/Virtual
   
**/
#define PAL_HALT_LIGHT 29 

/**
   
  Initialize tags and data of a cache line for processor
  testing. It is required by IPF.

  @param CallingConvention  Static Registers

  @param Mode               Physical
   
**/
#define PAL_CACHE_LINE_INIT 31

/**
   
  Read tag and data of a cache line for diagnostic testing. It
  is optional.

  @param CallingConvention  Satcked Register

  @param Mode               Physical
   
**/
#define PAL_CACHE_READ 259 

/**
   
  Write tag and data of a cache for diagnostic testing. It is
  optional.

  @param CallingConvention  Satcked Registers

  @param Mode               Physical
   
**/
#define PAL_CACHE_WRITE 260

/**
   
  Returns alignment and size requirements needed for the memory
  buffer passed to the PAL_TEST_PROC procedure as well as
  information on self-test control words for the processor self
  tests. It is required by IPF.

  @param CallingConvention  Static Registers

  @param Mode               Physical
   
**/
#define PAL_TEST_INFO 37

/**
   
  Perform late processor self test. It is required by
  IPF.

  @param CallingConvention  Stacked Registers

  @param Mode               Physical
   
**/
#define PAL_TEST_PROC 258

/**
   
  Return information needed to relocate PAL procedures and PAL
  PMI code to memory. It is required by IPF.

  @param CallingConvention  Static Registers

  @param Mode               Physical
   
**/
#define PAL_COPY_INFO 30

/**
   
  Relocate PAL procedures and PAL PMI code to memory. It is
  required by IPF.

  @param CallingConvention  Stacked Registers

  @param Mode               Physical
   
**/
#define PAL_COPY_PAL 256

/**
   
  Enter IA-32 System environment. It is optional.

  @param CallingConvention  Static Registers

  @param Mode               Physical
   
**/
#define PAL_ENTER_IA_32_ENV 33

/**
   
  Register PMI memory entrypoints with processor. It is required
  by IPF.

  @param CallingConvention  Static Registers

  @param Mode               Physical
   
**/
#define PAL_PMI_ENTRYPOINT 32



#endif
