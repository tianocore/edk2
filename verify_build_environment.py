#!/usr/bin/env python3
"""
Build environment verification script for EDK II
"""

import os
import sys
import subprocess
import shutil

def check_python():
    """Check Python version"""
    version = sys.version_info
    if version.major >= 3 and version.minor >= 8:
        return True, f"Python {version.major}.{version.minor}.{version.micro}"
    else:
        return False, f"Python {version.major}.{version.minor}.{version.micro} (requires 3.8+)"

def check_git():
    """Check if git is available"""
    try:
        result = subprocess.run(['git', '--version'], capture_output=True, text=True)
        if result.returncode == 0:
            return True, result.stdout.strip()
        else:
            return False, "Git not found"
    except FileNotFoundError:
        return False, "Git not found"

def check_edk2_tools():
    """Check if EDK2 Python tools are installed"""
    try:
        import edk2toolext
        return True, "EDK2 tools installed"
    except ImportError:
        return False, "EDK2 tools not installed (run: pip install -r pip-requirements.txt)"

def check_build_tools():
    """Check for build tools"""
    tools_status = {}
    
    # Check for common build tools
    tools = ['make', 'gcc', 'nasm']
    
    for tool in tools:
        if shutil.which(tool):
            try:
                result = subprocess.run([tool, '--version'], capture_output=True, text=True)
                if result.returncode == 0:
                    version = result.stdout.split('\n')[0]
                    tools_status[tool] = (True, version)
                else:
                    tools_status[tool] = (False, "Available but version check failed")
            except:
                tools_status[tool] = (False, "Available but version check failed")
        else:
            tools_status[tool] = (False, "Not found")
    
    return tools_status

def check_workspace():
    """Check if we're in the EDK2 workspace"""
    required_files = [
        'edksetup.sh',
        'edksetup.bat', 
        'BaseTools',
        'MdeModulePkg',
        'OvmfPkg'
    ]
    
    missing = []
    for item in required_files:
        if not os.path.exists(item):
            missing.append(item)
    
    if not missing:
        return True, "EDK2 workspace detected"
    else:
        return False, f"Missing: {', '.join(missing)}"

def main():
    """Main verification function"""
    print("=" * 60)
    print("EDK II Build Environment Verification")
    print("=" * 60)
    
    checks = [
        ("Python Version", check_python),
        ("Git", check_git),
        ("EDK2 Workspace", check_workspace),
        ("EDK2 Python Tools", check_edk2_tools),
    ]
    
    all_passed = True
    
    for check_name, check_func in checks:
        print(f"\n[CHECK] {check_name}")
        try:
            if check_name == "Build Tools":
                # Special handling for build tools
                continue
            else:
                passed, message = check_func()
                status = "PASS" if passed else "FAIL"
                print(f"[{status}] {message}")
                if not passed:
                    all_passed = False
        except Exception as e:
            print(f"[ERROR] Check failed with exception: {str(e)}")
            all_passed = False
    
    # Check build tools separately
    print(f"\n[CHECK] Build Tools")
    tools_status = check_build_tools()
    for tool, (available, message) in tools_status.items():
        status = "PASS" if available else "WARN"
        print(f"  [{status}] {tool}: {message}")
        # Don't fail for missing build tools on Windows as they might be in VS
    
    print("\n" + "=" * 60)
    if all_passed:
        print("[SUCCESS] Build environment verification passed!")
        print("\nNext steps:")
        print("1. If EDK2 tools are not installed, run: pip install -r pip-requirements.txt")
        print("2. Setup environment: ./edksetup.sh (Linux) or .\\edksetup.bat (Windows)")
        print("3. Build OVMF: build -p OvmfPkg/OvmfPkgX64.dsc -a X64 -t GCC5 -b DEBUG")
    else:
        print("[WARNING] Some checks failed. Please review the issues above.")
        print("The build may still work depending on your platform and available tools.")
    
    print("=" * 60)
    
    return 0 if all_passed else 1

if __name__ == "__main__":
    sys.exit(main())