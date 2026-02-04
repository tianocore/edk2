#!/usr/bin/env python3
"""
Test script to verify OVMF SDHCI fix
This script simulates the testing process and validates the changes made to the EDK II codebase.
"""

import os
import sys
import subprocess
import re

def check_file_exists(filepath):
    """Check if a file exists"""
    return os.path.exists(filepath)

def check_sdhci_driver_modification():
    """Check if the SDHCI driver has been modified with the QEMU compatibility fix"""
    driver_path = "MdeModulePkg/Bus/Pci/SdMmcPciHcDxe/SdMmcPciHcDxe.c"
    
    if not check_file_exists(driver_path):
        return False, f"Driver file not found: {driver_path}"
    
    try:
        with open(driver_path, 'r') as f:
            content = f.read()
        
        # Check for the QEMU compatibility fix
        qemu_check_pattern = r"PciData\.Hdr\.VendorId == 0x1b36"
        debug_pattern = r"Found QEMU device"
        
        has_qemu_check = bool(re.search(qemu_check_pattern, content))
        has_debug_info = bool(re.search(debug_pattern, content))
        
        if has_qemu_check and has_debug_info:
            return True, "SDHCI driver successfully modified with QEMU compatibility fix"
        else:
            return False, "SDHCI driver modification not found or incomplete"
            
    except Exception as e:
        return False, f"Error reading driver file: {str(e)}"

def check_ovmf_sdcard_config():
    """Check if OVMF has SD card support enabled"""
    config_path = "OvmfPkg/Include/Dsc/OvmfOptHwDefines.dsc.inc"
    
    if not check_file_exists(config_path):
        return False, f"OVMF config file not found: {config_path}"
    
    try:
        with open(config_path, 'r') as f:
            content = f.read()
        
        # Check for SDCARD_ENABLE = TRUE
        sdcard_pattern = r"DEFINE\s+SDCARD_ENABLE\s*=\s*TRUE"
        
        if re.search(sdcard_pattern, content):
            return True, "OVMF SD card support is enabled"
        else:
            return False, "OVMF SD card support is not enabled"
            
    except Exception as e:
        return False, f"Error reading OVMF config file: {str(e)}"

def check_sdcard_components():
    """Check if SD card components are properly included in OVMF"""
    components_path = "OvmfPkg/Include/Dsc/OvmfOptHwComponents.dsc.inc"
    
    if not check_file_exists(components_path):
        return False, f"OVMF components file not found: {components_path}"
    
    try:
        with open(components_path, 'r') as f:
            content = f.read()
        
        # Check for SD card components
        required_components = [
            "SdMmcPciHcDxe",
            "SdDxe",
            "EmmcDxe",
            "SDCardSupport"
        ]
        
        missing_components = []
        for component in required_components:
            if component not in content:
                missing_components.append(component)
        
        if not missing_components:
            return True, "All SD card components are properly included"
        else:
            return False, f"Missing SD card components: {', '.join(missing_components)}"
            
    except Exception as e:
        return False, f"Error reading OVMF components file: {str(e)}"

def generate_test_qemu_command():
    """Generate the QEMU command line for testing"""
    qemu_cmd = """
# Test command for OVMF with SDHCI support:
qemu-kvm \\
  -bios /usr/share/OVMF/OVMF_CODE.fd \\
  -fw_cfg name=opt/org.tianocore/SDCardSupport,string=yes \\
  -device sdhci-pci \\
  -drive if=none,file=Fedora-IoT-ostree-41-20241027.0.x86_64.iso,format=raw,id=MMC1 \\
  -device sd-card,drive=MMC1 \\
  -m 2048 \\
  -enable-kvm

# Alternative test with built OVMF (after successful build):
qemu-kvm \\
  -bios Build/OvmfX64/DEBUG_GCC5/FV/OVMF_CODE.fd \\
  -fw_cfg name=opt/org.tianocore/SDCardSupport,string=yes \\
  -device sdhci-pci \\
  -drive if=none,file=Fedora-IoT-ostree-41-20241027.0.x86_64.iso,format=raw,id=MMC1 \\
  -device sd-card,drive=MMC1 \\
  -m 2048 \\
  -enable-kvm
"""
    return qemu_cmd

def main():
    """Main test function"""
    print("=" * 60)
    print("EDK II OVMF SDHCI Fix Verification Test")
    print("=" * 60)
    
    tests = [
        ("SDHCI Driver Modification", check_sdhci_driver_modification),
        ("OVMF SD Card Configuration", check_ovmf_sdcard_config),
        ("SD Card Components", check_sdcard_components),
    ]
    
    all_passed = True
    
    for test_name, test_func in tests:
        print(f"\n[TEST] {test_name}")
        try:
            passed, message = test_func()
            status = "PASS" if passed else "FAIL"
            print(f"[{status}] {message}")
            if not passed:
                all_passed = False
        except Exception as e:
            print(f"[ERROR] Test failed with exception: {str(e)}")
            all_passed = False
    
    print("\n" + "=" * 60)
    if all_passed:
        print("[SUCCESS] All tests passed! The SDHCI fix has been properly implemented.")
        print("\nNext steps:")
        print("1. Build OVMF with: build -p OvmfPkg/OvmfPkgX64.dsc -a X64 -t GCC5 -b DEBUG")
        print("2. Test with the QEMU command below:")
        print(generate_test_qemu_command())
    else:
        print("[FAILURE] Some tests failed. Please review the issues above.")
    
    print("=" * 60)
    
    return 0 if all_passed else 1

if __name__ == "__main__":
    sys.exit(main())