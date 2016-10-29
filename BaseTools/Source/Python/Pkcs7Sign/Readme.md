# Step by step to generate sample self-signed X.509 certificate chain and sign data with PKCS7 structure

This readme demonstrates how to generate 3-layer X.509 certificate chain (RootCA -> IntermediateCA -> SigningCert) with OpenSSL commands, and user MUST set a UNIQUE Subject Name ("Common Name") on these three different certificates.

## How to generate a self-signed X.509 certificate chain via OPENSSL
* Set OPENSSL environment.

NOTE: Below steps are required for Windows. Linux may already have the OPENSSL environment correctly.

    set OPENSSL_HOME=c:\home\openssl\openssl-[version]
    set OPENSSL_CONF=%OPENSSL_HOME%\apps\openssl.cnf

When a user uses OpenSSL (req or ca command) to generate the certificates, OpenSSL will use the openssl.cnf file as the configuration data (can use “-config path/to/openssl.cnf” to describe the specific config file).

The user need check the openssl.cnf file, to find your CA path setting, e.g. check if the path exists in [ CA_default ] section.

    [ CA_default ]
        dir = ./demoCA              # Where everything is kept

You may need the following steps for initialization:

    rd ./demoCA /S/Q
    mkdir ./demoCA
    echo "" > ./demoCA/index.txt
    echo 01 > ./demoCA/serial
    mkdir ./demoCA/newcerts

* Generate the certificate chain:

NOTE: User MUST set a UNIQUE "Common Name" on the different certificate

1) Generate the Root Pair:

Generate a root key:

    openssl genrsa -aes256 -out TestRoot.key 2048

Generate a self-signed root certificate:

    openssl req -new -x509 -days 3650 -key TestRoot.key -out TestRoot.crt
    openssl x509 -in TestRoot.crt -out TestRoot.cer -outform DER
    openssl x509 -inform DER -in TestRoot.cer -outform PEM -out TestRoot.pub.pem

2) Generate the Intermediate Pair:

Generate the intermediate key:

    openssl genrsa -aes256 -out TestSub.key 2048

Generate the intermediate certificate:

    openssl req -new -days 3650 -key TestSub.key -out TestSub.csr
    openssl ca -extensions v3_ca -in TestSub.csr -days 3650 -out TestSub.crt -cert TestRoot.crt -keyfile TestRoot.key
    openssl x509 -in TestSub.crt -out TestSub.cer -outform DER
    openssl x509 -inform DER -in TestSub.cer -outform PEM -out TestSub.pub.pem

3) Generate User Key Pair for Data Signing:

Generate User key:

    openssl genrsa -aes256 -out TestCert.key 2048

Generate User certificate:

    openssl req -new -days 3650 -key TestCert.key -out TestCert.csr
    openssl ca -in TestCert.csr -days 3650 -out TestCert.crt -cert TestSub.crt -keyfile TestSub.key`
    openssl x509 -in TestCert.crt -out TestCert.cer -outform DER
    openssl x509 -inform DER -in TestCert.cer -outform PEM -out TestCert.pub.pem

Convert Key and Certificate for signing. Password is removed with -nodes flag for convenience in this sample.

    openssl pkcs12 -export -out TestCert.pfx -inkey TestCert.key -in TestCert.crt
    openssl pkcs12 -in TestCert.pfx -nodes -out TestCert.pem

* Verify Data Signing & Verification with new X.509 Certificate Chain

1) Sign a Binary File to generate a detached PKCS7 signature:

    openssl smime -sign -binary -signer TestCert.pem -outform DER -md sha256 -certfile TestSub.pub.pem -out test.bin.p7 -in test.bin

2) Verify PKCS7 Signature of a Binary File:

    openssl smime -verify -inform DER -in test.bin.p7 -content test.bin -CAfile TestRoot.pub.pem -out test.org.bin

