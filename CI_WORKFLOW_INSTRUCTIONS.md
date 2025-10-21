# GitHub Actions Workflow Installation Instructions

## Issue

The comprehensive testing workflow cannot be automatically committed due to GitHub App permissions restrictions. GitHub Apps require explicit `workflows` permission to create or modify workflow files.

## Solution

The workflow file needs to be manually added by a user with appropriate permissions.

## Installation Steps

1. **Locate the workflow file:**
   ```bash
   cat .github/workflows/comprehensive-testing.yml
   ```

2. **Option A: Commit via Git Command Line (Recommended)**
   ```bash
   # If you have direct repository access
   git add .github/workflows/comprehensive-testing.yml
   git commit -m "Add comprehensive testing GitHub Actions workflow"
   git push
   ```

3. **Option B: Add via GitHub Web Interface**
   - Navigate to: https://github.com/dmaynor/edk2
   - Go to: `.github/workflows/` directory
   - Click "Add file" → "Create new file"
   - Name: `comprehensive-testing.yml`
   - Copy content from local file
   - Commit directly to branch

4. **Option C: Create Pull Request**
   - Fork repository
   - Add workflow file to fork
   - Create PR to main repository

## Workflow File Location

The workflow file is located at:
```
.github/workflows/comprehensive-testing.yml
```

## What This Workflow Does

The comprehensive testing workflow provides:

1. **Security Regression Tests**
   - Python vulnerability tests
   - Bandit static analysis
   - Safety dependency checks

2. **Host-Based Unit Tests**
   - Multi-platform (Linux, Windows)
   - Multi-architecture (X64, IA32)
   - Coverage tracking with 80% threshold

3. **Code Quality Checks**
   - Pylint for Python code
   - Uncrustify for C/C++ formatting
   - Bare exception detection
   - Spell checking

4. **Platform Tests**
   - OVMF build and QEMU boot tests
   - Basic boot scenario validation

5. **Static Analysis**
   - CodeQL security scanning
   - C/C++ and Python analysis

6. **Strict Failure Criteria**
   - Zero tolerance for test failures
   - All jobs must pass
   - Comprehensive test reporting

## Triggering the Workflow

Once installed, the workflow runs on:
- Push to `master`, `stable/**`, `claude/**` branches
- Pull requests to `master`
- Manual workflow dispatch

## Verification

After installation, verify the workflow:

1. Check workflow appears in GitHub Actions tab
2. Trigger a test run (push a commit or manual dispatch)
3. Monitor job execution
4. Review test results and artifacts

## Alternative: Use Existing CI

If you prefer not to add a new workflow, you can:

1. **Use existing Stuart CI:**
   ```bash
   python stuart_ci_build -c .pytool/CISettings.py
   ```

2. **Run tests locally:**
   ```bash
   cd SecurityTests/Python
   python test_command_injection.py -v
   ```

3. **Integrate into existing workflows:**
   - Add security tests to current CI pipeline
   - Merge with existing Azure Pipelines configuration

## Support

For issues or questions:
- EDK2 GitHub: https://github.com/tianocore/edk2
- Documentation: See `TESTING.md` for complete testing guide

---

**Note:** This is a documentation file explaining why the workflow file is untracked and how to add it manually. The workflow file itself (`.github/workflows/comprehensive-testing.yml`) contains the complete CI/CD configuration and is ready to use once committed by a user with appropriate permissions.
