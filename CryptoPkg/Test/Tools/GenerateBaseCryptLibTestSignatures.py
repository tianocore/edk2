## @file
#  Generate precomputed signatures for BaseCryptLib verification tests.
#
#  Copyright (c) 2026, Intel Corporation. All rights reserved.<BR>
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##
"""Generate offline signature vectors for BaseCryptLib verification tests.

Select one vector profile with ``--algorithm`` and update its arrays in an
existing header. The checked-in private keys are test-only fixtures; this tool
must only be run manually by maintainers and is never part of firmware or
ordinary unit-test execution.

The tool supports Windows and Linux. OpenSSL 3.5 or newer is required for
ML-DSA and SLH-DSA, and the EdDSA context profile requires support for
``hexcontext-string``. Empty-message ML-DSA and SLH-DSA signatures use EVP
through a matching ``libcrypto`` library because ``pkeyutl`` cannot sign an
empty input.
"""

import argparse
import ctypes
import ctypes.util
import os
import re
import shutil
import subprocess
import tempfile
from abc import ABC, abstractmethod
from dataclasses import dataclass
from pathlib import Path
from typing import Any, ClassVar, Final, Sequence


TEST_DIR: Final[Path] = (
    Path(__file__).resolve().parents[1] / "UnitTest" / "Library" / "BaseCryptLib"
)
EC_MESSAGE: Final[bytes] = b"Test message for ECDSA verification"
EDDSA_MESSAGE: Final[bytes] = b"Test message for EdDSA signing and verification"
EDDSA_CONTEXT: Final[bytes] = b"test-context"
ML_DSA_MESSAGE: Final[bytes] = b"Test message for ML-DSA signing and verification"
ML_DSA_CONTEXT: Final[bytes] = b"ML-DSA test context"
ML_DSA_MAX_CONTEXT: Final[bytes] = b"A" * 255
ML_DSA_MESSAGES: Final[tuple[bytes, ...]] = (b"First message", b"Second message", b"Third message")
PKCS7_MESSAGE: Final[bytes] = b"PKCS7 verification test message"
PKCS7_PARTIAL_CHAIN_MESSAGE: Final[bytes] = b"Payload Data for PKCS#7 Signing"
RSA_CERT_MESSAGE: Final[bytes] = b"RSA certificate verification test message"
SLH_DSA_MESSAGE: Final[bytes] = b"Test message for SLH-DSA signing and verification"
SLH_DSA_CONTEXT: Final[bytes] = b"SLH-DSA test context"
SLH_DSA_MAX_CONTEXT: Final[bytes] = bytes(range(255))
SLH_DSA_SECOND_MESSAGE: Final[bytes] = b"Second message"


@dataclass(frozen=True)
class SignatureVector:
    """Represent one signature array to write into a C header.

    Attributes:
        name: C identifier of the signature array.
        data: Signature bytes stored in the array.
        description: Human-readable explanation emitted above the array.
        global_remove_if_unreferenced: Whether to emit an unreferenced global
            definition instead of a definition and matching extern declaration.
    """

    name: str
    data: bytes
    description: str
    global_remove_if_unreferenced: bool = False

@dataclass(frozen=True)
class RawSignatureCase:
    """Describe one message, context, and output name for raw signing.

    Attributes:
        name: C identifier and temporary signature-file stem.
        message: Input message, or ``None`` to request EVP empty-message signing.
        description: Human-readable explanation emitted above the array.
        context: Optional algorithm context passed as hexadecimal bytes.
    """

    name: str
    message: bytes | None
    description: str
    context: bytes | None = None


@dataclass(frozen=True)
class GeneratorOptions:
    """Hold settings shared by all profile generators.

    Attributes:
        openssl: OpenSSL executable name or path.
        test_dir: Directory containing the checked-in test fixtures.
        key_path: Optional SLH-DSA private-key override.
    """

    openssl: str
    test_dir: Path
    key_path: Path | None = None


def read_c_array(source_path: Path, array_name: str) -> bytes:
    """Extract hexadecimal byte values from a C array initializer.

    Args:
        source_path: C source or header containing the initializer.
        array_name: Identifier of the array to read.

    Returns:
        The initializer bytes in declaration order.

    Raises:
        ValueError: If the named array is missing or contains no hexadecimal
            byte values.
        OSError: If the source file cannot be read.
    """
    source = source_path.read_text(encoding="utf-8")
    match = re.search(
        rf"\b{re.escape(array_name)}\s*\[\s*\]\s*=\s*\{{(.*?)\}};",
        source,
        re.DOTALL,
    )
    if match is None:
        raise ValueError(f"Array {array_name} not found in {source_path}")

    values = re.findall(r"0x([0-9a-fA-F]{1,2})", match.group(1))
    if not values:
        raise ValueError(f"Array {array_name} is empty in {source_path}")
    return bytes(int(value, 16) for value in values)


def run_openssl(openssl: str, *arguments: str) -> None:
    """Run one OpenSSL command and report its captured diagnostics on failure.

    Args:
        openssl: OpenSSL executable name or path.
        *arguments: Command and arguments passed to the executable.

    Raises:
        RuntimeError: If OpenSSL exits unsuccessfully.
        OSError: If the executable cannot be started.
    """
    result = subprocess.run(
        [openssl, *arguments],
        check=False,
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        details = result.stderr.strip() or result.stdout.strip()
        raise RuntimeError(f"OpenSSL command failed: {details}")


def read_der_length(data: bytes, offset: int) -> tuple[int, int]:
    """Read a definite, minimally encoded DER length at ``offset``.

    Args:
        data: DER-encoded input.
        offset: Index of the first length octet.

    Returns:
        A pair containing the decoded length and the index after its encoding.

    Raises:
        ValueError: If the length is truncated, indefinite, oversized, or not
            minimally encoded.
    """
    if offset >= len(data):
        raise ValueError("Truncated DER length")

    first = data[offset]
    offset += 1
    if first < 0x80:
        return first, offset

    length_bytes = first & 0x7F
    if length_bytes == 0 or length_bytes > 4 or offset + length_bytes > len(data):
        raise ValueError("Invalid DER length")
    if data[offset] == 0:
        raise ValueError("Non-minimal DER length")

    length = int.from_bytes(data[offset:offset + length_bytes], "big")
    if length < 0x80:
        raise ValueError("Non-minimal DER length")
    return length, offset + length_bytes


def ecdsa_der_to_raw(signature: bytes, component_size: int) -> bytes:
    """Convert a DER ECDSA signature to fixed-width ``r || s`` bytes.

    Args:
        signature: DER sequence containing the two ECDSA integers.
        component_size: Required byte width of each integer.

    Returns:
        The concatenated, zero-padded ``r`` and ``s`` components.

    Raises:
        ValueError: If the sequence or either integer is malformed, negative,
            non-minimal, too wide, or followed by trailing data.
    """
    if not signature or signature[0] != 0x30:
        raise ValueError("OpenSSL did not return a DER ECDSA sequence")

    sequence_size, offset = read_der_length(signature, 1)
    sequence_end = offset + sequence_size
    if sequence_end != len(signature):
        raise ValueError("Invalid DER ECDSA sequence length")

    components = []
    for _ in range(2):
        if offset >= sequence_end or signature[offset] != 0x02:
            raise ValueError("Invalid DER ECDSA integer")
        integer_size, offset = read_der_length(signature, offset + 1)
        integer_end = offset + integer_size
        if integer_size == 0 or integer_end > sequence_end:
            raise ValueError("Invalid DER ECDSA integer length")

        integer = signature[offset:integer_end]
        if integer[0] & 0x80:
            raise ValueError("Negative DER ECDSA integer")
        if len(integer) > 1 and integer[0] == 0:
            if integer[1] & 0x80 == 0:
                raise ValueError("Non-minimal DER ECDSA integer")
            integer = integer[1:]
        if len(integer) > component_size:
            raise ValueError("ECDSA integer exceeds the curve width")

        components.append(integer.rjust(component_size, b"\x00"))
        offset = integer_end

    if offset != sequence_end:
        raise ValueError("Trailing data in DER ECDSA sequence")
    return b"".join(components)


def sign_raw(
    openssl: str,
    key_path: Path,
    message: bytes,
    output_path: Path,
    context: bytes | None = None,
) -> bytes:
    """Sign a non-empty raw message with OpenSSL ``pkeyutl``.

    Args:
        openssl: OpenSSL executable name or path.
        key_path: PEM private key used for signing.
        message: Non-empty message bytes to sign.
        output_path: Temporary path for the generated signature.
        context: Optional raw-signature context, encoded for OpenSSL as hex.

    Returns:
        The generated signature bytes.

    Raises:
        RuntimeError: If OpenSSL rejects the signing operation.
        OSError: If temporary files cannot be read or written.

    The temporary message file is removed whether signing succeeds or fails.
    """
    message_path = output_path.with_suffix(".message")
    message_path.write_bytes(message)
    command = [
        "pkeyutl",
        "-sign",
        "-rawin",
        "-inkey",
        str(key_path),
        "-in",
        str(message_path),
        "-out",
        str(output_path),
    ]
    if context is not None:
        command.extend(["-pkeyopt", f"hexcontext-string:{context.hex()}"])

    try:
        run_openssl(openssl, *command)
        return output_path.read_bytes()
    finally:
        message_path.unlink(missing_ok=True)


def _resolve_openssl_executable(openssl: str) -> Path:
    """Resolve an executable name through ``PATH`` or normalize its path.

    Args:
        openssl: Executable name or filesystem path supplied by the caller.

    Returns:
        An absolute path used to locate the matching shared library.
    """
    executable = shutil.which(openssl)
    return Path(executable or openssl).resolve()


def _find_windows_crypto_library(openssl: str) -> Path:
    """Find the versioned ``libcrypto`` DLL beside the OpenSSL executable.

    Args:
        openssl: OpenSSL executable name or path.

    Returns:
        The only matching ``libcrypto-*-x64.dll`` path in the executable's
        directory.

    Raises:
        RuntimeError: If zero or multiple matching DLLs are found.
    """
    executable_path = _resolve_openssl_executable(openssl)
    candidates = sorted(executable_path.parent.glob("libcrypto-*-x64.dll"))
    if len(candidates) != 1:
        raise RuntimeError(
            f"Expected one OpenSSL libcrypto DLL beside {executable_path}, "
            f"found {len(candidates)}"
        )
    return candidates[0]


def _get_openssl_major_version(openssl: str) -> int:
    """Query the selected executable and return its OpenSSL major version.

    Args:
        openssl: OpenSSL executable name or path.

    Returns:
        The integer major version reported by ``openssl version``.

    Raises:
        RuntimeError: If the executable fails or its output is not recognized.
        OSError: If the executable cannot be started.
    """
    executable_path = _resolve_openssl_executable(openssl)
    result = subprocess.run(
        [str(executable_path), "version"],
        check=False,
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        details = result.stderr.strip() or result.stdout.strip()
        raise RuntimeError(f"Could not determine OpenSSL version: {details}")

    match = re.search(r"\bOpenSSL\s+(\d+)\.", result.stdout)
    if match is None:
        raise RuntimeError(f"Could not determine OpenSSL major version from: {result.stdout.strip()}")
    return int(match.group(1))


def _find_posix_crypto_library(openssl: str) -> str:
    """Find ``libcrypto`` with the same major version as the selected executable.

    The executable prefix's ``lib`` and ``lib64`` directories are checked
    before the system library lookup, which prevents loading an incompatible
    system library when OpenSSL is installed under a separate prefix.

    Args:
        openssl: OpenSSL executable name or path.

    Returns:
        A library path or loader name suitable for ``ctypes.CDLL``.

    Raises:
        RuntimeError: If no library is discoverable or the system library has
            a different major version.
    """
    executable_path = _resolve_openssl_executable(openssl)
    major_version = _get_openssl_major_version(openssl)
    library_filename = f"libcrypto.so.{major_version}"
    library_directories = (
        executable_path.parent,
        executable_path.parent.parent / "lib",
        executable_path.parent.parent / "lib64",
    )
    for directory in library_directories:
        library_path = directory / library_filename
        if library_path.is_file():
            return str(library_path)

    library_name = ctypes.util.find_library("crypto")
    if library_name is None:
        raise RuntimeError("OpenSSL libcrypto shared library not found")
    soname_match = re.search(r"libcrypto\.so\.(\d+)(?:\.|$)", Path(library_name).name)
    if soname_match is not None and int(soname_match.group(1)) != major_version:
        raise RuntimeError(
            f"OpenSSL executable is version {major_version}, but system libcrypto is "
            f"version {soname_match.group(1)}; install the matching shared library"
        )
    return library_name


def _load_posix_crypto_library(openssl: str) -> Any:
    """Load the POSIX ``libcrypto`` matching ``openssl``.

    Args:
        openssl: OpenSSL executable name or path used for version matching.

    Returns:
        The loaded shared-library handle.
    """
    return ctypes.CDLL(_find_posix_crypto_library(openssl))


def _load_crypto_library(openssl: str) -> tuple[Any, Any | None]:
    """Load the matching ``libcrypto`` and retain Windows DLL search state.

    Args:
        openssl: OpenSSL executable name or path.

    Returns:
        A pair containing the loaded library and, on Windows, the DLL-directory
        handle that must remain open while the library is in use.

    Raises:
        RuntimeError: If a matching shared library cannot be found or loaded.
        OSError: If the operating system cannot load the library.
    """
    if os.name == "nt":
        library_path = _find_windows_crypto_library(openssl)
        dll_directory = os.add_dll_directory(str(library_path.parent))
        return ctypes.WinDLL(str(library_path)), dll_directory

    return _load_posix_crypto_library(openssl), None


def _configure_crypto_api(crypto: Any) -> None:
    """Declare the ctypes signatures used by the empty-message EVP path.

    Args:
        crypto: Loaded OpenSSL ``libcrypto`` handle to configure.
    """
    crypto.BIO_new_mem_buf.argtypes = (ctypes.c_void_p, ctypes.c_int)
    crypto.BIO_new_mem_buf.restype = ctypes.c_void_p
    crypto.BIO_free.argtypes = (ctypes.c_void_p,)
    crypto.PEM_read_bio_PrivateKey.argtypes = (
        ctypes.c_void_p,
        ctypes.c_void_p,
        ctypes.c_void_p,
        ctypes.c_void_p,
    )
    crypto.PEM_read_bio_PrivateKey.restype = ctypes.c_void_p
    crypto.EVP_MD_CTX_new.restype = ctypes.c_void_p
    crypto.EVP_MD_CTX_free.argtypes = (ctypes.c_void_p,)
    crypto.EVP_PKEY_free.argtypes = (ctypes.c_void_p,)
    crypto.EVP_DigestSignInit.argtypes = (
        ctypes.c_void_p,
        ctypes.POINTER(ctypes.c_void_p),
        ctypes.c_void_p,
        ctypes.c_void_p,
        ctypes.c_void_p,
    )
    crypto.EVP_DigestSign.argtypes = (
        ctypes.c_void_p,
        ctypes.c_void_p,
        ctypes.POINTER(ctypes.c_size_t),
        ctypes.c_void_p,
        ctypes.c_size_t,
    )


def _sign_empty_message(crypto: Any, private_key: Any) -> bytes:
    """Create an empty-message signature using the EVP two-pass sequence.

    Args:
        crypto: Configured OpenSSL ``libcrypto`` handle.
        private_key: OpenSSL ``EVP_PKEY`` handle.

    Returns:
        The generated signature bytes.

    Raises:
        RuntimeError: If OpenSSL cannot allocate, initialize, or complete
            signing.
    """
    context = crypto.EVP_MD_CTX_new()
    if not context:
        raise RuntimeError("OpenSSL could not allocate a signing context")

    try:
        key_context = ctypes.c_void_p()
        if crypto.EVP_DigestSignInit(context, ctypes.byref(key_context), None, None, private_key) != 1:
            raise RuntimeError("OpenSSL could not initialize signing")

        signature_size = ctypes.c_size_t()
        if crypto.EVP_DigestSign(context, None, ctypes.byref(signature_size), None, 0) != 1:
            raise RuntimeError("OpenSSL could not determine the empty-message signature size")
        signature = (ctypes.c_ubyte * signature_size.value)()
        if crypto.EVP_DigestSign(
            context,
            signature,
            ctypes.byref(signature_size),
            None,
            0,
        ) != 1:
            raise RuntimeError("OpenSSL could not sign the empty message")
        return bytes(signature[:signature_size.value])
    finally:
        crypto.EVP_MD_CTX_free(context)


def sign_empty_raw(openssl: str, key_path: Path) -> bytes:
    """Sign an empty message with the selected key through OpenSSL EVP.

    Args:
        openssl: OpenSSL executable name or path used to locate matching
            ``libcrypto``.
        key_path: PEM private key used for signing.

    Returns:
        The generated signature bytes.

    Raises:
        RuntimeError: If the library cannot be loaded, the key cannot be
            parsed, or signing fails.
        OSError: If the key cannot be read or the shared library cannot load.
    """
    crypto, dll_directory = _load_crypto_library(openssl)
    bio = None
    private_key = None
    try:
        _configure_crypto_api(crypto)
        key_data = ctypes.create_string_buffer(key_path.read_bytes())
        bio = crypto.BIO_new_mem_buf(key_data, len(key_data) - 1)
        if not bio:
            raise RuntimeError("OpenSSL could not read the private key")
        private_key = crypto.PEM_read_bio_PrivateKey(bio, None, None, None)
        if not private_key:
            raise RuntimeError("OpenSSL could not parse the private key")
        return _sign_empty_message(crypto, private_key)
    finally:
        if private_key:
            crypto.EVP_PKEY_free(private_key)
        if bio:
            crypto.BIO_free(bio)
        if dll_directory is not None:
            dll_directory.close()


def format_c_array(vector: SignatureVector, newline: str = "\n") -> str:
    """Format a signature vector as a C definition and optional extern.

    Args:
        vector: Signature data and declaration metadata to format.
        newline: Line separator used between generated lines.

    Returns:
        The complete C declaration as text.
    """
    lines = ["//", f"// {vector.description}", "//"]
    if vector.global_remove_if_unreferenced:
        lines.append(f"GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  {vector.name}[] = {{")
    else:
        lines.extend(
            [
                "#ifdef BASE_CRYPT_LIB_VERIFY_TEST_SIGNATURES_DEFINE",
                f"CONST UINT8  {vector.name}[] = {{",
            ]
        )

    lines.extend(format_byte_rows(vector.data))
    lines.append("};")
    if not vector.global_remove_if_unreferenced:
        lines.extend(
            [
                "#else",
                f"extern CONST UINT8  {vector.name}[{len(vector.data)}];",
                "#endif",
            ]
        )
    return newline.join(lines)


def format_byte_rows(data: bytes) -> list[str]:
    """Format bytes as indented C initializer rows of twelve values.

    Args:
        data: Byte sequence to format.

    Returns:
        C initializer rows, each containing at most twelve hexadecimal bytes.
    """
    return [
        "  " + ", ".join(f"0x{value:02x}" for value in data[offset:offset + 12]) + ","
        for offset in range(0, len(data), 12)
    ]


def update_signature_header(output_path: Path, vectors: Sequence[SignatureVector]) -> None:
    """Replace selected vector arrays in an existing header, preserving others.

    Array definitions and extern lengths are updated for each supplied vector.
    The header's original newline convention is preserved.

    Args:
        output_path: Existing header file to update.
        vectors: Signature vectors whose named arrays should be replaced.

    Raises:
        FileNotFoundError: If ``output_path`` is not an existing file.
        ValueError: If a vector definition or extern declaration is missing or
            ambiguous.
        OSError: If the header cannot be read or written.
    """
    if not output_path.is_file():
        raise FileNotFoundError(f"Output header does not exist: {output_path}")

    header = output_path.read_bytes().decode("utf-8")
    newline = "\r\n" if "\r\n" in header else "\n"
    for vector in vectors:
        name = re.escape(vector.name)
        definition_pattern = re.compile(
            rf"(?P<prefix>(?:GLOBAL_REMOVE_IF_UNREFERENCED\s+)?CONST UINT8\s+{name}\s*\[\s*\]\s*=\s*\{{)"
            rf"(?P<body>.*?)"
            rf"(?P<suffix>\}};)",
            re.DOTALL,
        )
        matches = list(definition_pattern.finditer(header))
        if len(matches) != 1:
            raise ValueError(
                f"Expected one definition of {vector.name} in {output_path}, found {len(matches)}"
            )

        match = matches[0]
        rows = newline.join(format_byte_rows(vector.data))
        replacement = f"{match.group('prefix')}{newline}{rows}{newline}{match.group('suffix')}"
        header = header[:match.start()] + replacement + header[match.end():]

        if not vector.global_remove_if_unreferenced:
            declaration_pattern = re.compile(
                rf"(extern\s+CONST UINT8\s+{name}\s*\[)\s*\d+\s*(\];)"
            )
            header, count = declaration_pattern.subn(
                lambda declaration: (
                    f"{declaration.group(1)}{len(vector.data)}{declaration.group(2)}"
                ),
                header,
                count=1,
            )
            if count != 1:
                raise ValueError(
                    f"Expected one extern declaration of {vector.name} in {output_path}"
                )

    output_path.write_bytes(header.encode("utf-8"))


class SignatureGenerator(ABC):
    """Base class for one selectable signature-generation profile.

    Subclasses declare ``output_header`` and implement :meth:`generate`.

    Attributes:
        output_header: Default header name updated by the profile.
        options: OpenSSL executable, fixture directory, and optional key.
    """

    output_header: ClassVar[str]

    def __init__(self, options: GeneratorOptions) -> None:
        """Initialize the profile with shared generator options.

        Args:
            options: OpenSSL executable, fixture directory, and optional key.
        """
        self.options = options

    def write_fixture(
        self,
        temporary_directory: Path,
        source_name: str,
        array_name: str,
        output_name: str,
    ) -> Path:
        """Extract a fixture array and write it to a temporary file.

        Args:
            temporary_directory: Directory that owns the temporary fixture.
            source_name: Fixture source file relative to ``test_dir``.
            array_name: C array identifier to extract.
            output_name: Filename to use for the extracted bytes.

        Returns:
            Path to the temporary fixture file.

        Raises:
            ValueError: If the requested array is missing or empty.
            OSError: If the source or destination file cannot be accessed.
        """
        fixture = read_c_array(self.options.test_dir / source_name, array_name)
        fixture_path = temporary_directory / output_name
        fixture_path.write_bytes(fixture)
        return fixture_path

    @abstractmethod
    def generate(self) -> list[SignatureVector]:
        """Generate all vectors defined by this profile.

        Returns:
            Signature vectors ready to replace arrays in ``output_header``.

        Raises:
            RuntimeError: If OpenSSL cannot perform a required operation.
            OSError: If a fixture or temporary file cannot be accessed.
        """


def generate_raw_signature_vectors(
    openssl: str,
    key_path: Path,
    temporary_directory: Path,
    cases: Sequence[RawSignatureCase],
    global_remove_if_unreferenced: bool = False,
) -> list[SignatureVector]:
    """Sign each declarative raw-message case and build its output vector.

    A case with ``message=None`` is signed through EVP as an empty message;
    other cases use ``pkeyutl`` and preserve their optional context.

    Args:
        openssl: OpenSSL executable name or path.
        key_path: PEM private key used by all cases.
        temporary_directory: Directory for temporary signature files.
        cases: Cases describing message, context, name, and output description.
        global_remove_if_unreferenced: Declaration style used for every result.

    Returns:
        One signature vector per case, in input order.

    Raises:
        RuntimeError: If OpenSSL fails to sign any case.
        OSError: If key or temporary files cannot be accessed.
    """
    vectors: list[SignatureVector] = []
    for case in cases:
        if case.message is None:
            signature = sign_empty_raw(openssl, key_path)
        else:
            signature = sign_raw(
                openssl,
                key_path,
                case.message,
                temporary_directory / f"{case.name}.sig",
                case.context,
            )
        vectors.append(
            SignatureVector(
                case.name,
                signature,
                case.description,
                global_remove_if_unreferenced,
            )
        )
    return vectors


class EcdsaGenerator(SignatureGenerator):
    """Generate the ECDSA P-256 verifier vector."""

    output_header: ClassVar[str] = "VerifyTestSignatures.h"

    def generate(self) -> list[SignatureVector]:
        """Generate the P-256 SHA-256 verifier signature.

        Returns:
            The ECDSA signature as one fixed-width raw-format vector.

        Raises:
            RuntimeError: If OpenSSL cannot sign the fixture message.
            ValueError: If OpenSSL returns malformed DER signature data.
            OSError: If the fixture or temporary files cannot be accessed.
        """
        with tempfile.TemporaryDirectory() as temporary_directory:
            temp_dir = Path(temporary_directory)
            key_path = self.write_fixture(
                temp_dir, "EcTests.c", "mEccTestPemKey", "ec-key.pem"
            )
            message_path = temp_dir / "ec-message.bin"
            signature_path = temp_dir / "ec-signature.der"
            message_path.write_bytes(EC_MESSAGE)
            run_openssl(
                self.options.openssl,
                "dgst",
                "-sha256",
                "-sign",
                str(key_path),
                "-out",
                str(signature_path),
                str(message_path),
            )
            signature = ecdsa_der_to_raw(signature_path.read_bytes(), 32)
        return [
            SignatureVector(
                "mEcDsaTestSignature",
                signature,
                "ECDSA P-256 signature for the SHA-256 hash of the ECDSA test message",
            )
        ]


class EdDsaGenerator(SignatureGenerator):
    """Generate Ed448 verifier vectors, including the context-bound case."""

    output_header: ClassVar[str] = "VerifyTestSignatures.h"

    def generate(self) -> list[SignatureVector]:
        """Generate Ed448 message and context-bound verifier signatures.

        Returns:
            The message vector followed by the context-bound vector.

        Raises:
            RuntimeError: If OpenSSL cannot generate either signature.
            OSError: If the fixture or temporary files cannot be accessed.
        """
        with tempfile.TemporaryDirectory() as temporary_directory:
            temp_dir = Path(temporary_directory)
            key_path = self.write_fixture(
                temp_dir, "EdDsaTests.c", "mEd448TestPemKey", "eddsa-key.pem"
            )
            cases = (
                RawSignatureCase(
                    "mEdDsaTestSignature",
                    EDDSA_MESSAGE,
                    "Ed448 signature for the EdDSA test message",
                ),
                RawSignatureCase(
                    "mEdDsaTestContextSignature",
                    EDDSA_MESSAGE,
                    "Ed448 signature for the EdDSA test message and test context",
                    EDDSA_CONTEXT,
                ),
            )
            return generate_raw_signature_vectors(
                self.options.openssl, key_path, temp_dir, cases
            )


class MlDsaGenerator(SignatureGenerator):
    """Generate ML-DSA-87 verifier vectors and boundary cases."""

    output_header: ClassVar[str] = "VerifyTestSignatures.h"

    def generate(self) -> list[SignatureVector]:
        """Generate ML-DSA-87 message, context, and boundary-case signatures.

        Returns:
            Vectors in the order expected by the ML-DSA verifier tests.

        Raises:
            RuntimeError: If OpenSSL cannot generate a requested signature.
            OSError: If the fixture or temporary files cannot be accessed.
        """
        with tempfile.TemporaryDirectory() as temporary_directory:
            temp_dir = Path(temporary_directory)
            key_path = self.write_fixture(
                temp_dir,
                "MlDsaTestVectors.h",
                "mMlDsa87TestPemKey",
                "ml-dsa-key.pem",
            )
            cases = (
                RawSignatureCase(
                    "mMlDsaTestSignature",
                    ML_DSA_MESSAGE,
                    "ML-DSA-87 signature for the ML-DSA test message",
                ),
                RawSignatureCase(
                    "mMlDsaTestContextSignature",
                    ML_DSA_MESSAGE,
                    "ML-DSA-87 signature for the ML-DSA test message and test context",
                    ML_DSA_CONTEXT,
                ),
                RawSignatureCase(
                    "mMlDsaEmptyMessageSignature",
                    None,
                    "ML-DSA-87 signature for an empty message",
                ),
                RawSignatureCase(
                    "mMlDsaMaxContextSignature",
                    ML_DSA_MESSAGE,
                    "ML-DSA-87 signature using a 255-byte context",
                    ML_DSA_MAX_CONTEXT,
                ),
                *(
                    RawSignatureCase(
                        f"mMlDsaMultipleMessage{index}Signature",
                        message,
                        f"ML-DSA-87 signature for multiple-message test message {index}",
                    )
                    for index, message in enumerate(ML_DSA_MESSAGES, start=1)
                ),
            )
            return generate_raw_signature_vectors(
                self.options.openssl, key_path, temp_dir, cases
            )


class Pkcs7Generator(SignatureGenerator):
    """Generate attached PKCS#7 verifier vectors."""

    output_header: ClassVar[str] = "VerifyTestSignatures.h"

    def write_pem_certificate(
        self,
        temporary_directory: Path,
        source_name: str,
        array_name: str,
        der_name: str,
        pem_name: str,
    ) -> Path:
        """Extract a DER certificate fixture and convert it to PEM.

        Args:
            temporary_directory: Directory that owns the temporary files.
            source_name: Fixture source file relative to ``test_dir``.
            array_name: C array identifier containing the DER certificate.
            der_name: Filename for the extracted DER bytes.
            pem_name: Filename for the converted PEM certificate.

        Returns:
            Path to the converted PEM certificate.

        Raises:
            RuntimeError: If OpenSSL cannot convert the certificate.
            ValueError: If the DER fixture array is missing or empty.
            OSError: If fixture or temporary files cannot be accessed.
        """
        der_path = self.write_fixture(
            temporary_directory, source_name, array_name, der_name
        )
        pem_path = temporary_directory / pem_name
        run_openssl(
            self.options.openssl,
            "x509",
            "-inform",
            "DER",
            "-in",
            str(der_path),
            "-out",
            str(pem_path),
        )
        return pem_path

    def sign_pkcs7(
        self,
        temporary_directory: Path,
        message: bytes,
        certificate_path: Path,
        key_path: Path,
        password: str,
        output_name: str,
    ) -> bytes:
        """Create attached PKCS#7 SignedData for one test message.

        Args:
            temporary_directory: Directory for the message and signature files.
            message: Payload embedded in the attached SignedData object.
            certificate_path: PEM signing certificate.
            key_path: PEM private key corresponding to the certificate.
            password: Passphrase supplied to OpenSSL for the private key.
            output_name: Filename for the DER-encoded CMS output.

        Returns:
            The DER-encoded attached SignedData bytes.

        Raises:
            RuntimeError: If OpenSSL cannot create the SignedData object.
            OSError: If temporary files cannot be accessed.
        """
        message_path = temporary_directory / f"{output_name}.message"
        signature_path = temporary_directory / output_name
        message_path.write_bytes(message)
        run_openssl(
            self.options.openssl,
            "cms",
            "-sign",
            "-binary",
            "-nodetach",
            "-nosmimecap",
            "-md",
            "sha256",
            "-in",
            str(message_path),
            "-signer",
            str(certificate_path),
            "-inkey",
            str(key_path),
            "-passin",
            f"pass:{password}",
            "-outform",
            "DER",
            "-out",
            str(signature_path),
        )
        return signature_path.read_bytes()

    def generate(self) -> list[SignatureVector]:
        """Generate one RSA certificate signature and two attached CMS vectors.

        Returns:
            Vectors in the order expected by the RSA and PKCS#7 verifier tests.

        Raises:
            RuntimeError: If OpenSSL cannot sign or convert a fixture.
            ValueError: If a required fixture array is missing or empty.
            OSError: If fixture or temporary files cannot be accessed.
        """
        with tempfile.TemporaryDirectory() as temporary_directory:
            temp_dir = Path(temporary_directory)
            key_path = self.write_fixture(
                temp_dir, "RsaPkcs7Tests.c", "TestKeyPem", "rsa-key.pem"
            )
            return [
                self.generate_rsa_certificate_signature(temp_dir, key_path),
                *self.generate_attached_pkcs7_signatures(temp_dir, key_path),
            ]

    def generate_rsa_certificate_signature(
        self,
        temporary_directory: Path,
        key_path: Path,
    ) -> SignatureVector:
        """Create the RSA PKCS#1 SHA-1 certificate-verification vector.

        Args:
            temporary_directory: Directory for the message and signature files.
            key_path: PEM private key used to sign the certificate test message.

        Returns:
            The signed message as a ``SignatureVector``.

        Raises:
            RuntimeError: If OpenSSL cannot create the signature.
            OSError: If temporary files cannot be accessed.
        """
        message_path = temporary_directory / "rsa-cert.message"
        signature_path = temporary_directory / "rsa-cert.signature"
        message_path.write_bytes(RSA_CERT_MESSAGE)
        run_openssl(
            self.options.openssl,
            "dgst",
            "-sha1",
            "-sign",
            str(key_path),
            "-passin",
            "pass:client",
            "-out",
            str(signature_path),
            str(message_path),
        )
        return SignatureVector(
            "mRsaCertPkcs1TestSignature",
            signature_path.read_bytes(),
            "RSA PKCS#1 SHA-1 signature for the certificate public-key verification test",
        )

    def generate_attached_pkcs7_signatures(
        self,
        temporary_directory: Path,
        key_path: Path,
    ) -> list[SignatureVector]:
        """Create attached SignedData vectors for normal and partial-chain tests.

        Args:
            temporary_directory: Directory for extracted fixtures and outputs.
            key_path: PEM key for the normal PKCS#7 signing certificate.

        Returns:
            The normal attached vector followed by the partial-chain vector.

        Raises:
            RuntimeError: If OpenSSL cannot convert certificates or sign data.
            ValueError: If a required fixture array is missing or empty.
            OSError: If fixture or temporary files cannot be accessed.
        """
        certificate_path = self.write_pem_certificate(
            temporary_directory,
            "RsaPkcs7Tests.c",
            "TestCert",
            "rsa-cert.der",
            "rsa-cert.pem",
        )
        partial_key_path = self.write_fixture(
            temporary_directory,
            "RsaPkcs7Tests.c",
            "TestKeyCert2Pem",
            "rsa-partial-chain-key.pem",
        )
        partial_certificate_path = self.write_pem_certificate(
            temporary_directory,
            "RsaPkcs7Tests.c",
            "TestCert2",
            "rsa-partial-chain-cert.der",
            "rsa-partial-chain-cert.pem",
        )
        return [
            SignatureVector(
                "mRsaPkcs7TestSignature",
                self.sign_pkcs7(
                    temporary_directory,
                    PKCS7_MESSAGE,
                    certificate_path,
                    key_path,
                    "client",
                    "pkcs7-signature.der",
                ),
                "Attached PKCS#7 SignedData for the PKCS#7 verification test message",
            ),
            SignatureVector(
                "mRsaPkcs7PartialChainTestSignature",
                self.sign_pkcs7(
                    temporary_directory,
                    PKCS7_PARTIAL_CHAIN_MESSAGE,
                    partial_certificate_path,
                    partial_key_path,
                    "non-self-issued",
                    "pkcs7-partial-chain-signature.der",
                ),
                "Attached PKCS#7 SignedData signed by the non-self-issued test certificate",
            ),
        ]


class SlhDsaGenerator(SignatureGenerator):
    """Generate SLH-DSA-SHAKE-256s verifier and context test vectors."""

    output_header: ClassVar[str] = "SlhDsaTestVectors.h"

    def generate(self) -> list[SignatureVector]:
        """Generate SLH-DSA-SHAKE-256s message and boundary-case signatures.

        Returns:
            The five vectors in the order expected by the SLH-DSA tests.

        Raises:
            FileNotFoundError: If the caller-supplied private key does not exist.
            RuntimeError: If OpenSSL cannot generate a requested signature.
            ValueError: If the embedded key fixture is missing or empty.
            OSError: If fixture or temporary files cannot be accessed.
        """
        with tempfile.TemporaryDirectory() as temporary_directory:
            temp_dir = Path(temporary_directory)
            key_path = self.options.key_path
            if key_path is None:
                key_path = self.write_fixture(
                    temp_dir,
                    "SlhDsaTestVectors.h",
                    "mSlhDsaShake256sTestPemKey",
                    "slh-dsa-key.pem",
                )
            elif not key_path.is_file():
                raise FileNotFoundError(f"SLH-DSA private key not found: {key_path}")

            cases = (
                RawSignatureCase(
                    "mSlhDsaShake256sTestSignature",
                    SLH_DSA_MESSAGE,
                    "Precomputed signature for mSlhDsaTestMessage (no context)",
                ),
                RawSignatureCase(
                    "mSlhDsaShake256sTestContextSignature",
                    SLH_DSA_MESSAGE,
                    "Precomputed signature for mSlhDsaTestMessage with mSlhDsaTestContext",
                    SLH_DSA_CONTEXT,
                ),
                RawSignatureCase(
                    "mSlhDsaShake256sTestEmptyMsgSignature",
                    None,
                    "Precomputed signature for empty message (no context)",
                ),
                RawSignatureCase(
                    "mSlhDsaShake256sTestMaxContextSignature",
                    SLH_DSA_MESSAGE,
                    "Precomputed signature for mSlhDsaTestMessage with 255-byte max context",
                    SLH_DSA_MAX_CONTEXT,
                ),
                RawSignatureCase(
                    "mSlhDsaShake256sTestMsg2Signature",
                    SLH_DSA_SECOND_MESSAGE,
                    "Precomputed signature for Second message (no context)",
                ),
            )
            return generate_raw_signature_vectors(
                self.options.openssl,
                key_path,
                temp_dir,
                cases,
                global_remove_if_unreferenced=True,
            )


GENERATOR_TYPES: Final[dict[str, type[SignatureGenerator]]] = {
    "ecdsa": EcdsaGenerator,
    "eddsa": EdDsaGenerator,
    "mldsa": MlDsaGenerator,
    "pkcs7": Pkcs7Generator,
    "slh-dsa": SlhDsaGenerator,
}


def parse_arguments(arguments: Sequence[str] | None = None) -> argparse.Namespace:
    """Parse command-line options and enforce profile-specific constraints.

    Args:
        arguments: Argument strings to parse, or ``None`` to use ``sys.argv``.

    Returns:
        An ``argparse.Namespace`` containing the validated options.

    Raises:
        SystemExit: With status 2 if parsing fails or ``--key`` is used with a
            profile other than ``slh-dsa``.
    """
    parser = argparse.ArgumentParser(
        description="Generate offline BaseCryptLib verification signature vectors."
    )
    parser.add_argument(
        "--algorithm",
        required=True,
        choices=tuple(GENERATOR_TYPES),
        help="Algorithm profile to generate",
    )
    parser.add_argument(
        "--openssl",
        default="openssl",
        help="Path to OpenSSL executable; version requirements depend on the profile",
    )
    parser.add_argument(
        "--test-dir",
        type=Path,
        default=TEST_DIR,
        help=f"Directory containing test fixtures (default: {TEST_DIR})",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=TEST_DIR,
        help=f"Directory for the generated header (default: {TEST_DIR})",
    )
    output_group = parser.add_mutually_exclusive_group()
    output_group.add_argument(
        "--output-header",
        type=Path,
        help="Output header name or path (default: selected profile's header name)",
    )
    output_group.add_argument(
        "--output",
        type=Path,
        help="Full output-header path override (compatibility option)",
    )
    parser.add_argument(
        "--key",
        type=Path,
        help="SLH-DSA PEM private-key override (default: fixture in SlhDsaTestVectors.h)",
    )
    parsed = parser.parse_args(arguments)
    if parsed.key is not None and parsed.algorithm != "slh-dsa":
        parser.error("--key is supported only with --algorithm slh-dsa")
    return parsed


def resolve_output_path(
    output: Path | None,
    output_dir: Path,
    output_header: Path | None,
    default_header: str,
) -> Path:
    """Choose the output header path from the CLI overrides and profile default.

    Relative header names are resolved beneath ``output_dir``. For the legacy
    ``--output`` option, an existing relative path is preserved as supplied.

    Args:
        output: Optional legacy full-path override.
        output_dir: Directory used for relative output-header names.
        output_header: Optional header name or explicit path.
        default_header: Profile header name used when no header was specified.

    Returns:
        The resolved path passed to :func:`update_signature_header`.
    """
    if output is not None:
        if output.is_absolute() or output.exists():
            return output
        return output_dir / output

    header_path = output_header or Path(default_header)
    if header_path.is_absolute():
        return header_path
    return output_dir / header_path


def main(arguments: Sequence[str] | None = None) -> None:
    """Generate one profile and update its selected arrays in an existing header.

    Args:
        arguments: Command-line arguments, or ``None`` to use ``sys.argv``.

    Raises:
        SystemExit: If argument parsing fails.
        FileNotFoundError: If a required fixture, key, or output header is absent.
        RuntimeError: If OpenSSL cannot complete a required operation.
        ValueError: If a fixture or target header is malformed.
        OSError: If input or output files cannot be accessed.
    """
    parsed = parse_arguments(arguments)
    options = GeneratorOptions(
        parsed.openssl,
        parsed.test_dir,
        parsed.key,
    )
    generator = GENERATOR_TYPES[parsed.algorithm](options)
    vectors = generator.generate()

    output_path = resolve_output_path(
        parsed.output,
        parsed.output_dir,
        parsed.output_header,
        generator.output_header,
    )
    update_signature_header(output_path, vectors)
    print(f"Updated {len(vectors)} {parsed.algorithm} vector(s) in {output_path}")


if __name__ == "__main__":
    main()
