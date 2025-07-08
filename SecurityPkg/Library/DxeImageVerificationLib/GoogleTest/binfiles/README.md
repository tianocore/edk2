### How to reproduce the binary file

1. Apply the following patch.
2. Make adjustments if you are using non OvmfPkgIa32X64 file to build binary files.
3. Please use the following [guide](https://github.com/tianocore/tianocore.github.io/wiki/How-to-build-OVMF) for building OVMF.
4. Check the `Build` directory.

```
From c41a1a8c2cab120df9409685877e92f1ce755167 Mon Sep 17 00:00:00 2001
From: Alexander Gryanko <xpahos@gmail.com>
Date: Tue, 23 Sep 2025 01:44:27 +0300
Subject: [PATCH] Empty file example

---
 OvmfPkg/Empty/Empty.c      | 13 +++++++++++++
 OvmfPkg/Empty/Empty.inf    | 28 ++++++++++++++++++++++++++++
 OvmfPkg/OvmfPkgIa32X64.dsc |  1 +
 3 files changed, 42 insertions(+)
 create mode 100644 OvmfPkg/Empty/Empty.c
 create mode 100644 OvmfPkg/Empty/Empty.inf

diff --git a/OvmfPkg/Empty/Empty.c b/OvmfPkg/Empty/Empty.c
new file mode 100644
index 0000000000..3e448367bc
--- /dev/null
+++ b/OvmfPkg/Empty/Empty.c
@@ -0,0 +1,13 @@
+#include <Uefi.h>
+#include <Library/UefiLib.h>
+#include <Library/UefiBootServicesTableLib.h>
+
+EFI_STATUS
+EFIAPI
+EmptyPoint (
+  IN EFI_HANDLE        ImageHandle,
+  IN EFI_SYSTEM_TABLE  *SystemTable
+)
+{
+  return EFI_SUCCESS;
+}
diff --git a/OvmfPkg/Empty/Empty.inf b/OvmfPkg/Empty/Empty.inf
new file mode 100644
index 0000000000..e98ab6ff56
--- /dev/null
+++ b/OvmfPkg/Empty/Empty.inf
@@ -0,0 +1,28 @@
+## @file
+#  Enroll default PK, KEK, db, dbx.
+#
+#  Copyright (C) 2014-2019, Red Hat, Inc.
+#
+#  SPDX-License-Identifier: BSD-2-Clause-Patent
+##
+
+[Defines]
+  INF_VERSION                    = 1.28
+  BASE_NAME                      = EmptyFile
+  FILE_GUID                      = FF089297-5305-43B5-8FA8-08A10ACEB552
+  MODULE_TYPE                    = UEFI_APPLICATION
+  VERSION_STRING                 = 0.1
+  ENTRY_POINT                    = EmptyPoint
+
+[Sources]
+  Empty.c
+
+[Packages]
+  OvmfPkg/OvmfPkg.dec
+  MdePkg/MdePkg.dec
+
+[LibraryClasses]
+  UefiApplicationEntryPoint
+  UefiBootServicesTableLib
+  UefiLib
+
diff --git a/OvmfPkg/OvmfPkgIa32X64.dsc b/OvmfPkg/OvmfPkgIa32X64.dsc
index 06fc031ab4..27e1b7f223 100644
--- a/OvmfPkg/OvmfPkgIa32X64.dsc
+++ b/OvmfPkg/OvmfPkgIa32X64.dsc
@@ -969,6 +969,7 @@
   }
 !endif

+  OvmfPkg/Empty/Empty.inf
   #
   # TPM support
   #
--
2.39.5 (Apple Git-154)
```
