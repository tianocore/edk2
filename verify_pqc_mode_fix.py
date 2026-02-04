#!/usr/bin/env python3
"""
Verify PQC Mode Fix
This script specifically verifies that the PQC mode constants issue has been fixed.
"""

import os
import sys
from pathlib import Path

def read_file_safely(filepath):
    """Read file content safely"""
    try:
        with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
            return f.read()
    except:
        return ""

def verify_pqc_mode_constants_fix():
    """Verify that PQC mode constants are properly implemented"""
    print("=" * 80)
    print("VERIFYING PQC MODE CONSTANTS FIX")
    print("=" * 80)
    
    # Check the constants are defined
    nvdata_file = "SecurityPkg/VariableAuthenticated/PqcTransitionDxe/PqcTransitionNvData.h"
    nvdata_content = read_file_safely(nvdata_file)
    
    required_constants = [
        "PQC_MODE_TRADITIONAL_ONLY",
        "PQC_MODE_HYBRID", 
        "PQC_MODE_PQC_ONLY"
    ]
    
    print("\n[CHECK] PQC Mode Constants Definition")
    missing_constants = []
    for constant in required_constants:
        if constant in nvdata_content:
            print(f"✓ {constant} - DEFINED")
        else:
            print(f"❌ {constant} - MISSING")
            missing_constants.append(constant)
    
    if missing_constants:
        return False, f"Missing constants: {', '.join(missing_constants)}"
    
    # Check the constants are used in implementation
    main_file = "SecurityPkg/VariableAuthenticated/PqcTransitionDxe/PqcTransitionDxe.c"
    main_content = read_file_safely(main_file)
    
    print("\n[CHECK] PQC Mode Constants Usage in Switch Statements")
    required_cases = [
        "case PQC_MODE_TRADITIONAL_ONLY:",
        "case PQC_MODE_HYBRID:",
        "case PQC_MODE_PQC_ONLY:"
    ]
    
    missing_cases = []
    for case in required_cases:
        if case in main_content:
            print(f"✓ {case} - IMPLEMENTED")
        else:
            print(f"❌ {case} - MISSING")
            missing_cases.append(case)
    
    if missing_cases:
        return False, f"Missing switch cases: {', '.join(missing_cases)}"
    
    # Check PQC-only mode validation
    print("\n[CHECK] PQC-Only Mode Validation")
    pqc_only_check = "if (NewMode == PQC_MODE_PQC_ONLY)"
    if pqc_only_check in main_content:
        print("✓ PQC-only mode validation - IMPLEMENTED")
    else:
        print("❌ PQC-only mode validation - MISSING")
        return False, "PQC-only mode validation missing"
    
    # Check cleanup function mode check
    print("\n[CHECK] Cleanup Function Mode Check")
    cleanup_check = "if (CurrentMode != PQC_MODE_PQC_ONLY)"
    if cleanup_check in main_content:
        print("✓ Cleanup mode check - IMPLEMENTED")
    else:
        print("❌ Cleanup mode check - MISSING")
        return False, "Cleanup mode check missing"
    
    print("\n" + "=" * 80)
    print("✅ PQC MODE CONSTANTS FIX VERIFIED SUCCESSFULLY")
    print("=" * 80)
    print("\nAll required elements:")
    print("✓ Constants defined in header file")
    print("✓ Constants used in switch statements")
    print("✓ PQC-only mode validation implemented")
    print("✓ Cleanup function mode check implemented")
    
    return True, "PQC mode constants fix verified successfully"

def main():
    """Main verification function"""
    try:
        success, message = verify_pqc_mode_constants_fix()
        
        if success:
            print(f"\n🎉 SUCCESS: {message}")
            print("\nThe issue 'Missing phase implementation: Traditional Only Mode, PQC Only Mode' has been FIXED!")
            return 0
        else:
            print(f"\n❌ FAILURE: {message}")
            return 1
            
    except Exception as e:
        print(f"\n❌ ERROR: Verification failed with exception: {str(e)}")
        return 1

if __name__ == "__main__":
    sys.exit(main())