LoongArchVirt Secure Boot key placeholder layout.

This directory is reserved for platform-specific PK, KEK, db, and dbx
artifacts when validating Secure Boot flows locally.

No default certificates or private keys are provided by the platform tree.
If local testing needs embedded key material, add the files out of tree and
wire them into the build explicitly.
