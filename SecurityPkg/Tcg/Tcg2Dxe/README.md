# Tcg2Dxe

Tcg2Dxe is a DXE-phase UEFI driver that publishes the TCG2 protocol defined
by the [TCG EFI Protocol Specification](https://trustedcomputinggroup.org/resource/tcg-efi-protocol-specification/).
Its main responsibilites are to expose a standard interface to a TPM device,
measure components and events into PCRs, support measured boot, and enable
secure boot attestation.

## Dynamic Event Log Scaling

The TCG event log is initially allocated with a fixed size defined by a
PCD: PcdTcgLogAreaMinLen. As firmware components log measured boot
events the log fills up. Traditionally, when the log is full, subsequent events
are dropped and the log is marked as truncated.

Tcg2Dxe extends this behavior with **dynamic scaling**: when the log is about
to overflow, the driver doubles its allocation, copies the existing log into
the new buffer, and frees the old one. This allows the log to grow as needed
and avoids losing events.

### How It Works

1. **Scaling check** — Before logging a TCG 2.0 event,
   `TcgLogDynamicScalingNeeded` calculates whether the new event would exceed
   the current allocation (`EventLogAreaStruct->Laml`).

2. **Reallocation** — When scaling is needed, `TcgScaleEventLog` allocates a
   new `EfiBootServicesData` region at twice the current size, copies the
   existing log, updates the `Lasa`/`Laml` fields in the event log area
   struct, and frees the old region.

3. **Logging** — After scaling, the new event is logged into the resized buffer
   via `TcgDxeLogEvent` inside a TPL-raised critical section.

### NormalEventLog vs. FinalEventLog

Tcg2Dxe maintains two distinct event log regions:

| Log | Memory Type | Lifetime | Can Scale |
| --- | ----------- | -------- | --------- |
| **Normal log** | `EfiBootServicesData` | Available until `ExitBootServices` | Yes |
| **Final Events log** | `EfiACPIMemoryNVS` | Persistent | No |

- The **Normal log** is the main log copy which is returned via `GetEventLog`.
  It can grow dynamically via scaling. Note that previous calls to `GetEventLog`
  could contain stale data if the log was scaled after. It is recommended to
  call `GetEventLog` each time access is required.
- The **Final Events log** (`EFI_TCG2_FINAL_EVENTS_TABLE`) records events
  logged after `GetEventLog` has been called. It is installed as a UEFI
  configuration table so the OS can discover events that occurred between its
  call to `GetEventLog` and `ExitBootServices`. Because the **Final Events log**
  does not scale, it can become truncated.

### Scale Limit

The number of times the normal event log region may be dynamically scaled is
capped by `TCG_EVENT_LOG_MAX_SCALE_COUNT`. Each successful scale doubles the
allocation, so this caps total growth at `PcdTcgLogAreaMinLen << TCG_EVENT_LOG_MAX_SCALE_COUNT`.
Once the limit is reached:

1. `TcgScaleEventLog` refuses to scale further and returns
   `EFI_OUT_OF_RESOURCES`.
2. `EventLogAreaStruct->EventLogTruncated` is set to `TRUE`, so subsequent
   `GetEventLog` callers see `EventLogTruncated == TRUE`.
3. `HashLogExtendEvent` returns `EFI_VOLUME_FULL` for events that would have
   triggered the refused scale.

### Scaling Notification (`gTcg2EventLogScaledGuid`)

Each time the normal log is successfully resized, `TcgScaleEventLog` calls
`EfiEventGroupSignal (&gTcg2EventLogScaledGuid)` to notify interested parties
that the log moved in memory.

Consumers that cache the log base address returned by `GetEventLog` (for
example, parsers walking the log incrementally) must invalidate their cache
on this signal and call `GetEventLog` again to get the current `Lasa`/last
entry. A typical consumer:

```c
gBS->CreateEventEx (
       EVT_NOTIFY_SIGNAL,
       TPL_CALLBACK,
       OnTcgEventLogScaled,
       Context,
       &gTcg2EventLogScaledGuid,
       &Event
       );
```

The event is declared in `gTcg2EventLogScaledGuid` (see
`SecurityPkg/Include/Guid/Tcg2EventLogScaled.h`) and listed in `Tcg2Dxe.inf`.

## FinalEventLog Truncation Marker

Because the **FinalEventLog** is fixed-size and cannot scale, it can fill
up before `ExitBootServices`. When the next event would overflow the log,
`TcgDxeLogEvent` calls `AppendTruncationMarker` which writes a final
`EV_NO_ACTION` event whose payload is the ASCII string
`TCG_LOG_TRUNCATION_EVENT_STRING`. The `NumberOfEvents` counter in
`EFI_TCG2_FINAL_EVENTS_TABLE` is incremented to include the marker, and
`EventLogTruncated` is set so subsequent attempts return `EFI_VOLUME_FULL`
without re-appending the marker.

To guarantee the marker always fits, FinalEventLog initialization in
`SetupEventLog` subtracts `GetTruncationEventSize()` from the usable `Laml`:

```c
mTcgDxeData.FinalEventLogAreaStruct[Index].Laml =
    PcdGet32 (PcdTcg2FinalLogAreaLen)
  - sizeof (EFI_TCG2_FINAL_EVENTS_TABLE)
  - GetTruncationEventSize ();
```

`AppendTruncationMarker` temporarily restores this reserved space so
`TcgCommLogEvent` will accept the marker write.

OS-side and pre-OS consumers can detect FinalEventLog truncation by:

- walking `EFI_TCG2_FINAL_EVENTS_TABLE` and inspecting the last entry for an
  `EV_NO_ACTION` event whose payload begins with `"TCG Event Log Truncated"`.
