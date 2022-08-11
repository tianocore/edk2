# Title: Improving Delayed Dispatch PPI

## Status: Draft

## Document: UEFI Platform Initialization Specification Version 1.7 Errata A

## License

SPDX-License-Identifier: CC-BY-4.0

## Submitter: [TianoCore Community](https://www.tianocore.org)

## Summary of the change

Add `WaitOnEvent` interface into existing PPI and updated a few descriptions related to this PPI.

## Benefits of the change

The newly added interface `WaitOnEvent` will allow function execution sequence being managed with finer granularity without blocking spins.

It also resolved a few definition confusions in the existing PI spec.

## Impact of the change

PEI Core touch to invoke these events upon each dispatch action, so needs to be included in new specification

IN/OUT change should be OK since they are just comments, not something honored by C language. So shouldn’t have backward compatibility issue

The newly added interface will be an expansion to the existing PPI. But the Register interface will start to intake an extra but optional input argument (UniqueId). Platforms consuming this PPI will need to recompile the firmware and update the implementation accordingly.

## Detailed description of the change [normative updates]

### Specification Changes

1. In PI Specification v1.7 Errata A: Vol. 6.3, 6.3 EFI DELAYED DISPATCH PPI, add one new entry under `EFI_DELAYED_DISPATCH_REGISTER      Register;`:

  ```c
  EFI_DELAYED_DISPATCH WAIT_ON_EVENT WaitOnEvent;
  ```

2. The corresponding entry description is added as:

  ```text
  WaitOnEvent
  Pause current process and allow other components to continue to dispatch until all entries with specified "Unique ID" have completed.
  ```

3. In PI Specification v1.7 Errata A: EFI_DELAYED_DISPATCH_PPI.Register(): Prototype, replace the entry `OUT UINT32                     Delay` with below:

    ```c
    IN EFI_GUID                   *UniqueId OPTIONAL,
    IN UINT32                     Delay
    ```

4. The corresponding description is updated as:

    ```text
    Parameters

    This:
    Pointer to the EFI_DELAYED_DISPATCH_PPI instance.
    Function:
    Function to call back. The function prototype is defined in the "Related Definitions" below.
    Context:
    Context data.
    UniqueId:
    GUID for identifying the delayed dispatch request.
    Delay:
    Delay interval. This parameter describes a multiple of microseconds.

    Description

    This function is invoked by a PEIM to have a handler returned. The handler will be invoked after at the minimum of specified Delay. Note that if the Register service is called with a Delay of zero, then the handler will be dispatched immediately. If the UniqueId is specified, this request will be marked with GUID for WaitOnEvent usage.
    ```

5. In PI Specification v1.7 Errata A: EFI_DELAYED_DISPATCH_PPI.Register(), add a new section:

    ```text
    Related Definitions

    //***********************************************
    // EFI_DELAYED_DISPATCH_FUNCTION //***********************************************

    typedef
    VOID
    (EFIAPI *EFI_DELAYED_DISPATCH_FUNCTION)(
      IN  OUT UINT64     *Context,
          OUT UINT32     *NewDelay
      );

    Context
    Pointer to Context Data. Can be updated by routine.
    NewDelay
    The new delay in us.  Leave at 0 to unregister this callback. Upon return, if NewDelay is 0, the function is unregistered.  If NewDelay is not zero, this routine will be called again after the new delay period.

    ```

6. In PI Specification v1.7 Errata A: EFI_DELAYED_DISPATCH_PPI.Register() "Status Codes Returned", add a suffix: (EFI_DELAYED_DISPATCH_PPI.Register)

7. After PI Specification v1.7 Errata A: EFI_DELAYED_DISPATCH_PPI.Register(), add a new section:

    ```text
    EFI_DELAYED_DISPATCH_PPI.WaitOnEvent()

    Summary

    Pause current process and allow other components to continue to dispatch until all entries with specified "Unique ID" have completed.

    Prototype

    typedef
    EFI_STATUS
    (EFIAPI *EFI_DELAYED_DISPATCH_WAIT ON EVENT)(
      IN  EFI_DELAYED_DISPATCH_PPI      *This,
      IN  EFI_GUID                      UniqueId
      );

    Parameters

    This
    Pointer to the EFI_DELAYED_DISPATCH_PPI instance.
    UniqueId
    Delayed dispatch request ID the caller will wait on.

    Description

    This function is invoked by a PEIM to wait until all specified UniqueId events have been dispatched. The other events will continue to dispatch while this process is being paused.

    Status Codes Returned

    | EFI_SUCCESS | Function successfully invoked |
    | EFI_INVALID_PARAMETER | One of the arguments is not supported |
    ```

### Code Changes

1. Update DelayedDispatch.h to fix the existing definitions.

1. Update DelayedDispatch.h to add the new `WaitOnEvent` related definitions.

1. Introduce implementation of Delayed Dispatch PPI into PEI core.
