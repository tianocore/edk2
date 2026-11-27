## @file
#  Generate precomputed SLH-DSA-SHAKE-256s signatures for unit test vectors.
#
#  Copyright (c) 2026, Intel Corporation. All rights reserved.<BR>
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##
"""Script to generate SLH-DSA-SHAKE-256s test signatures using OpenSSL 3.5+.

This script extracts the private key from SlhDsaTestVectors.h (or accepts a PEM file)
and generates precomputed signatures for unit test messages using OpenSSL CLI.
The signatures are formatted as C constant arrays suitable for inclusion in
SlhDsaTestVectors.h.
"""

import argparse
import os
import re
import subprocess
import tempfile

DEFAULT_MESSAGE = b"Test message for SLH-DSA signing and verification"
DEFAULT_CONTEXT = b"SLH-DSA test context"
SECOND_MESSAGE = b"Second message"
MAX_CONTEXT = bytes(range(255))


def format_c_array(name: str, data: bytes, comment: str = "") -> str:
    """Format binary data as a C constant byte array."""
    lines = []
    if comment:
        lines.append(f"//\n// {comment}\n//")
    lines.append(f"GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  {name}[] = {{")
    for i in range(0, len(data), 12):
        chunk = data[i:i + 12]
        hex_vals = ", ".join(f"0x{b:02x}" for b in chunk)
        lines.append(f"  {hex_vals},")
    lines.append("};\n")
    return "\n".join(lines)


def sign_message(
    openssl_bin: str,
    key_file: str,
    message: bytes,
    context: bytes = None
) -> bytes:
    """Sign a message using OpenSSL CLI with SLH-DSA-SHAKE-256s."""
    with tempfile.NamedTemporaryFile(delete=False) as msg_f:
        msg_f.write(message)
        msg_path = msg_f.name

    with tempfile.NamedTemporaryFile(delete=False) as sig_f:
        sig_path = sig_f.name

    try:
        cmd = [
            openssl_bin,
            "pkeyutl",
            "-sign",
            "-rawin",
            "-inkey",
            key_file,
            "-in",
            msg_path,
            "-out",
            sig_path,
        ]
        if context is not None and len(context) > 0:
            cmd.extend(["-pkeyopt", f"context-string:{context.decode('latin1')}"])

        subprocess.run(cmd, check=True, capture_output=True)

        with open(sig_path, "rb") as f:
            signature = f.read()
        return signature
    finally:
        if os.path.exists(msg_path):
            os.remove(msg_path)
        if os.path.exists(sig_path):
            os.remove(sig_path)


def main():
    parser = argparse.ArgumentParser(
        description="Generate precomputed SLH-DSA test signatures."
    )
    parser.add_argument(
        "--openssl",
        default="openssl",
        help="Path to OpenSSL 3.5+ binary (default: openssl)",
    )
    parser.add_argument(
        "--key",
        default=None,
        help="Path to SLH-DSA-SHAKE-256s PEM private key file (optional, extracted from SlhDsaTestVectors.h if omitted)",
    )
    parser.add_argument(
        "--output",
        default=None,
        help="Output file for C arrays (default: stdout)",
    )
    args = parser.parse_args()

    temp_key_file = None
    key_path = args.key
    if not key_path:
        # Extract PEM key from SlhDsaTestVectors.h
        header_path = os.path.join(
            os.path.dirname(__file__), "SlhDsaTestVectors.h"
        )
        if not os.path.exists(header_path):
            raise FileNotFoundError(f"Cannot find {header_path}")

        with open(header_path, "r") as f:
            content = f.read()

        match = re.search(
            r"mSlhDsaShake256sTestPemKey\[\]\s*=\s*\{([^}]+)\};", content
        )
        if not match:
            raise ValueError("mSlhDsaShake256sTestPemKey array not found")

        hex_bytes = [
            int(x.strip(), 16)
            for x in match.group(1).split(",")
            if x.strip().startswith("0x")
        ]
        pem_data = bytes(hex_bytes)

        with tempfile.NamedTemporaryFile(
            delete=False, suffix=".pem"
        ) as key_f:
            key_f.write(pem_data)
            temp_key_file = key_f.name
            key_path = temp_key_file

    try:
        results = []

        # 1. Default signature (no context)
        sig_default = sign_message(
            args.openssl, key_path, DEFAULT_MESSAGE, context=None
        )
        results.append(
            format_c_array(
                "mSlhDsaShake256sTestSignature",
                sig_default,
                "Precomputed signature for mSlhDsaTestMessage (no context)",
            )
        )

        # 2. Context-bound signature
        sig_ctx = sign_message(
            args.openssl, key_path, DEFAULT_MESSAGE, context=DEFAULT_CONTEXT
        )
        results.append(
            format_c_array(
                "mSlhDsaShake256sTestContextSignature",
                sig_ctx,
                "Precomputed signature for mSlhDsaTestMessage with mSlhDsaTestContext",
            )
        )

        # 3. Empty message signature
        sig_empty = sign_message(
            args.openssl, key_path, b"", context=None
        )
        results.append(
            format_c_array(
                "mSlhDsaShake256sTestEmptyMsgSignature",
                sig_empty,
                "Precomputed signature for empty message (no context)",
            )
        )

        # 4. Max context signature
        sig_max_ctx = sign_message(
            args.openssl, key_path, DEFAULT_MESSAGE, context=MAX_CONTEXT
        )
        results.append(
            format_c_array(
                "mSlhDsaShake256sTestMaxContextSignature",
                sig_max_ctx,
                "Precomputed signature for mSlhDsaTestMessage with 255-byte max context",
            )
        )

        # 5. Second message signature
        sig_msg2 = sign_message(
            args.openssl, key_path, SECOND_MESSAGE, context=None
        )
        results.append(
            format_c_array(
                "mSlhDsaShake256sTestMsg2Signature",
                sig_msg2,
                "Precomputed signature for Second message (no context)",
            )
        )

        output_text = "\n".join(results)
        if args.output:
            with open(args.output, "w") as f:
                f.write(output_text)
            print(f"Signatures written to {args.output}")
        else:
            print(output_text)

    finally:
        if temp_key_file and os.path.exists(temp_key_file):
            os.remove(temp_key_file)


if __name__ == "__main__":
    main()
