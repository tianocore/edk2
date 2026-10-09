## @file
#  Unit tests for the offline BaseCryptLib signature-vector generator.
#
#  Copyright (c) 2026, Intel Corporation. All rights reserved.<BR>
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##
"""Unit tests for the offline BaseCryptLib signature-vector generator."""

import contextlib
import io
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

import GenerateBaseCryptLibTestSignatures as generator


class GeneratorCommandTests(unittest.TestCase):
    """Test the generator's command-line interface and profile registry."""

    def test_help_lists_all_supported_algorithms(self) -> None:
        """The script's help command succeeds without invoking OpenSSL."""
        script_path = Path(generator.__file__).resolve()
        result = subprocess.run(
            [sys.executable, str(script_path), "--help"],
            check=False,
            capture_output=True,
            text=True,
            timeout=10,
        )

        self.assertEqual(result.returncode, 0, result.stderr)
        for algorithm in ("ecdsa", "eddsa", "mldsa", "pkcs7", "slh-dsa"):
            with self.subTest(algorithm=algorithm):
                self.assertIn(algorithm, result.stdout)

    def test_algorithm_is_required(self) -> None:
        """Argument parsing rejects an invocation without a profile."""
        with contextlib.redirect_stderr(io.StringIO()):
            with self.assertRaises(SystemExit) as error:
                generator.parse_arguments([])

        self.assertEqual(error.exception.code, 2)

    def test_key_override_is_limited_to_slh_dsa(self) -> None:
        """The optional private-key override is accepted only for SLH-DSA."""
        parsed = generator.parse_arguments(
            ["--algorithm", "slh-dsa", "--key", "test-key.pem"]
        )
        self.assertEqual(parsed.key, Path("test-key.pem"))

        with contextlib.redirect_stderr(io.StringIO()):
            with self.assertRaises(SystemExit) as error:
                generator.parse_arguments(
                    ["--algorithm", "mldsa", "--key", "test-key.pem"]
                )

        self.assertEqual(error.exception.code, 2)

    def test_profiles_select_expected_headers(self) -> None:
        """Each registered profile declares the header it updates by default."""
        self.assertEqual(
            set(generator.GENERATOR_TYPES),
            {"ecdsa", "eddsa", "mldsa", "pkcs7", "slh-dsa"},
        )
        for algorithm, generator_type in generator.GENERATOR_TYPES.items():
            expected_header = (
                "SlhDsaTestVectors.h"
                if algorithm == "slh-dsa"
                else "VerifyTestSignatures.h"
            )
            with self.subTest(algorithm=algorithm):
                self.assertEqual(generator_type.output_header, expected_header)

    def test_fixture_and_output_paths_have_profile_defaults(self) -> None:
        """Default paths retain the checked-in BaseCryptLib vector locations."""
        parsed = generator.parse_arguments(["--algorithm", "slh-dsa"])

        self.assertEqual(parsed.test_dir, generator.TEST_DIR)
        self.assertEqual(parsed.output_dir, generator.TEST_DIR)
        self.assertEqual(
            generator.resolve_output_path(
                parsed.output,
                parsed.output_dir,
                parsed.output_header,
                generator.SlhDsaGenerator.output_header,
            ),
            generator.TEST_DIR / "SlhDsaTestVectors.h",
        )

    def test_fixture_and_output_paths_are_overridable(self) -> None:
        """Fixture directory, output directory, and output header are configurable."""
        parsed = generator.parse_arguments(
            [
                "--algorithm",
                "ecdsa",
                "--test-dir",
                "fixtures",
                "--output-dir",
                "generated",
                "--output-header",
                "CustomVectors.h",
            ]
        )

        self.assertEqual(parsed.test_dir, Path("fixtures"))
        self.assertEqual(parsed.output_dir, Path("generated"))
        self.assertEqual(
            generator.resolve_output_path(
                parsed.output,
                parsed.output_dir,
                parsed.output_header,
                generator.EcdsaGenerator.output_header,
            ),
            Path("generated") / "CustomVectors.h",
        )

    def test_legacy_output_path_override_is_preserved(self) -> None:
        """The existing --output option still selects a complete header path."""
        with tempfile.TemporaryDirectory() as directory:
            output_path = Path(directory) / "vectors.h"
            parsed = generator.parse_arguments(
                ["--algorithm", "ecdsa", "--output", str(output_path)]
            )

            self.assertEqual(
                generator.resolve_output_path(
                    parsed.output,
                    parsed.output_dir,
                    parsed.output_header,
                    generator.EcdsaGenerator.output_header,
                ),
                output_path,
            )

    def test_missing_relative_legacy_output_uses_output_directory(self) -> None:
        """A relative legacy output path resolves beneath the selected output directory."""
        output_path = Path("not-created-vectors.h")
        self.assertFalse(output_path.exists())

        self.assertEqual(
            generator.resolve_output_path(
                output_path,
                Path("generated"),
                None,
                generator.EcdsaGenerator.output_header,
            ),
            Path("generated") / output_path,
        )

    def test_main_passes_fixture_and_resolved_header_paths(self) -> None:
        """Main keeps fixture and destination paths separate in its CLI flow."""
        instances = []

        class FakeGenerator:
            output_header = "default.h"

            def __init__(self, options: generator.GeneratorOptions) -> None:
                self.options = options
                instances.append(self)

            def generate(self) -> list[generator.SignatureVector]:
                return []

        with (
            patch.dict(generator.GENERATOR_TYPES, {"ecdsa": FakeGenerator}),
            patch.object(generator, "update_signature_header") as update_header,
            patch("builtins.print"),
        ):
            generator.main(
                [
                    "--algorithm",
                    "ecdsa",
                    "--test-dir",
                    "fixtures",
                    "--output-dir",
                    "generated",
                    "--output-header",
                    "custom.h",
                ]
            )

        self.assertEqual(instances[0].options.test_dir, Path("fixtures"))
        update_header.assert_called_once_with(Path("generated/custom.h"), [])

    def test_output_header_and_legacy_output_are_mutually_exclusive(self) -> None:
        """Callers cannot provide competing output path overrides."""
        with contextlib.redirect_stderr(io.StringIO()):
            with self.assertRaises(SystemExit) as error:
                generator.parse_arguments(
                    [
                        "--algorithm",
                        "ecdsa",
                        "--output",
                        "existing.h",
                        "--output-header",
                        "custom.h",
                    ]
                )

        self.assertEqual(error.exception.code, 2)


class GeneratorFormatTests(unittest.TestCase):
    """Test parsing and updating the generator's C-vector representation."""

    def test_raw_signature_cases_dispatch_context_and_empty_messages(self) -> None:
        """Raw cases preserve context and route empty input through EVP."""
        cases = (
            generator.RawSignatureCase("message", b"data", "message vector", b"ctx"),
            generator.RawSignatureCase("empty", None, "empty vector"),
        )
        with tempfile.TemporaryDirectory() as directory:
            temporary_directory = Path(directory)
            with (
                patch.object(generator, "sign_raw", return_value=b"raw signature") as raw_sign,
                patch.object(generator, "sign_empty_raw", return_value=b"empty signature") as empty_sign,
            ):
                vectors = generator.generate_raw_signature_vectors(
                    "openssl",
                    temporary_directory / "key.pem",
                    temporary_directory,
                    cases,
                    global_remove_if_unreferenced=True,
                )

        self.assertEqual([vector.data for vector in vectors], [b"raw signature", b"empty signature"])
        self.assertTrue(all(vector.global_remove_if_unreferenced for vector in vectors))
        raw_sign.assert_called_once_with(
            "openssl",
            temporary_directory / "key.pem",
            b"data",
            temporary_directory / "message.sig",
            b"ctx",
        )
        empty_sign.assert_called_once_with("openssl", temporary_directory / "key.pem")

    def test_read_c_array_extracts_bytes(self) -> None:
        """Hexadecimal UINT8 initializer values are extracted in order."""
        with tempfile.TemporaryDirectory() as directory:
            source_path = Path(directory) / "fixture.h"
            source_path.write_text(
                "CONST UINT8  Fixture[] = { 0x00, 0x7f, 0xff };\n",
                encoding="utf-8",
            )

            self.assertEqual(generator.read_c_array(source_path, "Fixture"), b"\x00\x7f\xff")

    def test_read_c_array_rejects_missing_array(self) -> None:
        """A missing fixture array produces a useful value error."""
        with tempfile.TemporaryDirectory() as directory:
            source_path = Path(directory) / "fixture.h"
            source_path.write_text("CONST UINT8  Other[] = { 0x01 };\n", encoding="utf-8")

            with self.assertRaisesRegex(ValueError, "Fixture"):
                generator.read_c_array(source_path, "Fixture")

    def test_ecdsa_der_signature_converts_to_fixed_width_raw_form(self) -> None:
        """DER integers become fixed-width r || s without sign-padding bytes."""
        der_signature = bytes.fromhex("3006020101020102")

        self.assertEqual(
            generator.ecdsa_der_to_raw(der_signature, component_size=2),
            bytes.fromhex("00010002"),
        )

    def test_ecdsa_der_signature_rejects_trailing_data(self) -> None:
        """DER conversion rejects bytes outside the declared sequence."""
        with self.assertRaisesRegex(ValueError, "sequence length"):
            generator.ecdsa_der_to_raw(bytes.fromhex("300602010102010200"), 2)

    def test_header_update_replaces_vector_and_extern_length(self) -> None:
        """An update changes the selected vector but preserves other content."""
        initial_header = (
            "#ifdef DEFINE_SIGNATURE\n"
            "CONST UINT8  Signature[] = {\n"
            "  0x01,\n"
            "};\n"
            "#else\n"
            "extern CONST UINT8  Signature[1];\n"
            "#endif\n"
            "CONST UINT8  Other[] = { 0xaa };\n"
        )
        vector = generator.SignatureVector("Signature", b"\x02\x03", "test")

        with tempfile.TemporaryDirectory() as directory:
            output_path = Path(directory) / "vectors.h"
            output_path.write_text(initial_header, encoding="utf-8")
            generator.update_signature_header(output_path, [vector])
            updated_header = output_path.read_text(encoding="utf-8")

        self.assertIn("0x02, 0x03", updated_header)
        self.assertIn("extern CONST UINT8  Signature[2];", updated_header)
        self.assertIn("CONST UINT8  Other[] = { 0xaa };", updated_header)

    def test_sign_raw_passes_binary_context_as_hex(self) -> None:
        """Raw signing encodes arbitrary context bytes and removes its message file."""
        with tempfile.TemporaryDirectory() as directory:
            temporary_directory = Path(directory)
            key_path = temporary_directory / "key.pem"
            key_path.write_bytes(b"test key")
            output_path = temporary_directory / "signature.bin"

            def fake_run_openssl(*_: str) -> None:
                output_path.write_bytes(b"signature")

            with patch.object(generator, "run_openssl", side_effect=fake_run_openssl) as run:
                signature = generator.sign_raw(
                    "openssl",
                    key_path,
                    b"message",
                    output_path,
                    context=b"\x00\xff",
                )

            self.assertEqual(signature, b"signature")
            self.assertIn("hexcontext-string:00ff", run.call_args.args)
            self.assertFalse(output_path.with_suffix(".message").exists())

    def test_windows_crypto_library_lookup_supports_openssl_four(self) -> None:
        """Version-neutral lookup discovers the OpenSSL 4 libcrypto DLL."""
        with tempfile.TemporaryDirectory() as directory:
            executable_path = Path(directory) / "openssl.exe"
            library_path = Path(directory) / "libcrypto-4-x64.dll"
            executable_path.touch()
            library_path.touch()

            self.assertEqual(
                generator._find_windows_crypto_library(str(executable_path)),
                library_path,
            )

    def test_windows_openssl_command_resolves_through_path(self) -> None:
        """The default OpenSSL command resolves before finding its adjacent DLL."""
        with tempfile.TemporaryDirectory() as directory:
            executable_path = Path(directory) / "openssl.exe"
            library_path = Path(directory) / "libcrypto-4-x64.dll"
            executable_path.touch()
            library_path.touch()
            with patch.object(generator.shutil, "which", return_value=str(executable_path)):
                self.assertEqual(
                    generator._find_windows_crypto_library("openssl"),
                    library_path,
                )

    def test_posix_crypto_library_uses_system_lookup(self) -> None:
        """The POSIX loader passes the resolved shared library to ctypes."""
        with (
            patch.object(generator, "_find_posix_crypto_library", return_value="libcrypto.so.3"),
            patch.object(generator.ctypes, "CDLL") as load_library,
        ):
            library = generator._load_posix_crypto_library("openssl")

        load_library.assert_called_once_with("libcrypto.so.3")
        self.assertIs(library, load_library.return_value)

    def test_posix_lookup_prefers_matching_prefix_library(self) -> None:
        """A private OpenSSL 4 prefix selects libcrypto.so.4 over system 3.x."""
        with tempfile.TemporaryDirectory() as directory:
            prefix = Path(directory)
            executable_path = prefix / "bin" / "openssl"
            library_path = prefix / "lib" / "libcrypto.so.4"
            executable_path.parent.mkdir()
            library_path.parent.mkdir()
            executable_path.touch()
            library_path.touch()
            with patch.object(generator, "_get_openssl_major_version", return_value=4):
                self.assertEqual(
                    generator._find_posix_crypto_library(str(executable_path)),
                    str(library_path),
                )

    def test_posix_lookup_rejects_mismatched_system_library(self) -> None:
        """A system OpenSSL 3 library cannot be used with an OpenSSL 4 executable."""
        with (
            patch.object(generator, "_get_openssl_major_version", return_value=4),
            patch.object(generator.ctypes.util, "find_library", return_value="libcrypto.so.3"),
            self.assertRaisesRegex(RuntimeError, "does not match|but system libcrypto is"),
        ):
            generator._find_posix_crypto_library("openssl")


if __name__ == "__main__":
    unittest.main()
