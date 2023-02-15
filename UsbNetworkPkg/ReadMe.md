# UsbNetworkPkg

This document is intend to provide package information, include the interface details.

# INDEX
  * [Introduction](#introduction)
  * [Components](#components)
    * [[NetworkCommon]](#networkcommon)
    * [[UsbCdcEcm]](#usbcdcecm)
    * [[UsbCdcNcm]](#usbcdcncm)
    * [[UsbRndis]](#usbrndis)

#  Introduction
UsbNetworkPkg provides network functions for USB LAN devices.

# Components
Below module is included in this package:<br>
- NetworkCommon
- UsbCdcEcm
- UsbCdcNcm
- UsbRndis

## [NetworkCommon]
Provides a LAN driver based on UEFI specification(UNDI). It supports USB communication class subclass devices and USB Rndis devices, depending on the UsbEthernetProtocol.

## Required Components
- NetworkPkg

## [UsbCdcEcm]
This driver provides a communication interface for USB Ethernet devices that follows the ECM protocol. The driver installs UsbEthernetProtocol with ECM functions which are consumed by the NetworkCommon driver.

The driver is compatible with the following USB class codes:
|Class Code|SubClass Code|Protocol Code|
|:--------:|:-----------:|:-----------:|
|0x02|0x06|0x00|

## Required Components
- NetworkCommon
- MdeModulePkg(USB bus driver)

## [UsbCdcNcm]
This driver provides a communication interface for USB Ethernet devices that follows the NCM protocol. The driver installs UsbEthernetProtocol with NCM functions which are consumed by the NetworkCommon driver.

The driver is compatible with the following USB class codes:
|Class Code|SubClass Code|Protocol Code|
|:--------:|:-----------:|:-----------:|
|0x02|0x0D|0x00|

## Required Components
- NetworkCommon
- MdeModulePkg(USB bus driver)

## [UsbRndis]
This driver provides a communication interface for USB Ethernet devices that follows the RNDIS protocol. The driver installs UsbEthernetProtocol with RNDIS functions which are consumed by the NetworkCommon driver.

The driver is compatible with the following USB class codes:
|Class Code|SubClass Code|Protocol Code|
|:--------:|:-----------:|:-----------:|
|0x02|0x02|0xFF|
|0xEF|0x04|0x01|

## Required Components
- NetworkCommon
- MdeModulePkg(USB bus driver)

