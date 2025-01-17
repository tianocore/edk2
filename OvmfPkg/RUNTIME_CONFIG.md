# OVMF runtime configuration

Some aspects of OVMF can be configured from the host, mostly by adding
firmware config files using the qemu command line option `-fw_cfg`.
The official namespace prefix for edk2 is `opt/org.tianocore/` which
is used by most options.  Some options are elsewhere for historical
reasons.


## TLS: etc/edk2/https/cacerts

This is a list of root CA certificates which the firmware should
accept when talking to TLS servers, stored in EFI signature database
format.  Fedora and CentOS provide a copy of the host root CA database
(which is used by openssl & friends too) in the correct format.  Using
that is as simple as:

```
qemu-system-x86_64 \
  -fw_cfg name=etc/edk2/https/cacerts,file=/etc/pki/ca-trust/extracted/edk2/cacerts.bin
```

For a more strict configuration where only one or very few servers are
allowed (instead of pretty much any https server on the public
internet) you can use the `virt-fw-sigdb` utility from the
[virt-firmware project](https://gitlab.com/kraxel/virt-firmware) to
create a database with only these server certificates:

```
GUID=$(uuidgen)
virt-fw-sigdb -o mycerts.esl \
  --add-cert ${GUID} /etc/pki/tls/certs/${server1}.crt \
  --add-cert ${GUID} /etc/pki/tls/certs/${server2}.crt
qemu-system-x86_64 \
  -fw_cfg name=etc/edk2/https/cacerts,file=mycerts.esl
```


## TLS: etc/edk2/https/ciphers

Configres the allowed TLS chiper suites.  Using the host's system
configuration works this way:

```
qemu-system-x86_64 \
  -object tls-cipher-suites,id=mysuite0,priority=@SYSTEM \
  -fw_cfg name=etc/edk2/https/ciphers,gen_id=mysuite0
```


## Network: opt/org.tianocore/IPv4Support

As the name suggests, this enables/disables IPv4 support.  Default:
enabled.  Usage:

```
qemu-system-x86_64 -fw_cfg name=opt/org.tianocore/IPv4Support,string=no
```

## Network: opt/org.tianocore/IPv6Support

Same for IPv6.


## Network: opt/org.tianocore/IPv4PXESupport

This enables/disables PXE network boot over IPv4.  This does not
affect HTTP boot.  Default: enabled.  Usage:

```
qemu-system-x86_64 -fw_cfg name=opt/org.tianocore/IPv4PXESupport,string=no
```


## Network: opt/org.tianocore/IPv6PXESupport

Same for IPv6.


## Network: opt/org.tianocore/VirtioNetSupport

This enables/disables the edk2 driver for virtio net devices.
Default: enabled.  Usage:

```
qemu-system-x86_64 -fw_cfg name=opt/org.tianocore/VirtioNetSupport,string=no
```


## Network: opt/org.tianocore/ISCSISupport

This enables/disables the edk2 driver for iSCSI.
Default: disabled.  Usage:

```
qemu-system-x86_64 -fw_cfg name=opt/org.tianocore/ISCSISupport,string=yes
```


## Security: opt/ovmf/PcdSetNxForStack

Set stack pages as not executable in page tables.  Usage:

```
qemu-system-x86_64 -fw_cfg name=opt/ovmf/PcdSetNxForStack,string=yes
```


## Security: opt/org.tianocore/UninstallMemAttrProtocol

Do not provide the `EFI_MEMORY_ATTRIBUTE_PROTOCOL` protocol.  This is
a workaround for a bug in shim version 15.6.  Usage:

```
qemu-system-x86_64 -fw_cfg name=opt/org.tianocore/UninstallMemAttrProtocol,string=yes
```


## Shell: opt/org.tianocore/EFIShellSupport

This enables/disables the EFI shell.
Default: enabled.  Usage:

```
qemu-system-x86_64 -fw_cfg name=opt/org.tianocore/EFIShellSupport,string=no
```


## Platform: opt/org.tianocore/X-Cpuhp-Bugcheck-Override

On some older qemu versions CPU hotplug support was broken.  OVMF
detects that and will by default log an error message and stop.  When
setting this option OVMF will continue despite qemu being buggy.  It
is only safe to set that option if you do not use CPU hotplug.  Usage:

```
qemu-system-x86_64 -fw_cfg name=opt/org.tianocore/X-Cpuhp-Bugcheck-Override,string=yes
```


## Platform: opt/ovmf/X-PciMmio64Mb

Historically OVMF has by default used 32GB of address space for the
64-bit PCI MMIO window.  This option can be used to change that size.

Recent OVMF versions have started to use a larger MMIO window by
default, with the actual size depending on the address space size
supported by the CPU.  With this the default configuration should
cover alot more use cases and it should be rather rare that this
option must be used to enlarge the MMIO window.

Usage example (128GB / 131072 MB):

```
qemu-system-x86_64 -fw_cfg name=opt/ovmf/X-PciMmio64Mb,string=131072
```


## Platform: physical address space bits

OVMF will check the supported physical address space bits (via CPUID)
when creating the address space layout.  OVMF will use the complete
available address space and place the 64-bit PCI MMIO window near the
top of the address space.

The `host-phys-bits-limit` property of the `-cpu` option can be used
to reduce the address space used by OVMF.

Usage:
```
qemu-system-x86_64 -cpu ${name},host-phys-bits=on,host-phys-bits-limit=42
```


## Platform: opt/org.tianocore/PagingLevel

Configure the number of paging levels (4 or 5) OVMF will use.  Four
paging levels are better for compatibility, not all OSes can handle
5-level paging being active at ExitBootServices time.  Five paging
levels are better for huge VMs, the address space managed by OVMF can
be larger.

Default: Use 5-level paging in case more than 1TB of memory is
present, 4-level paging otherwise.

Usage:
```
qemu-system-x86_64 -fw_cfg name=opt/org.tianocore/PagingLevel,string=5
```


## Other: opt/org.tianocore/UsbStorageSupport

This enables/disables the edk2 driver for USB storage devices.
Default: enabled.  Usage:

```
qemu-system-x86_64 -fw_cfg name=opt/org.tianocore/UsbStorageSupport,string=yes
```


