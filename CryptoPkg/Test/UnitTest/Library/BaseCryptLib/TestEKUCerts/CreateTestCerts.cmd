@ECHO OFF
REM
REM  Use this file to create test certificates.
REM
call certreq.exe -new                                    TestEKUParsingRoot.ini               TestEKUParsingRoot.cer
call certreq.exe -new -q -cert "TestEKUParsingRoot"      TestEKUParsingPolicyCA.ini           TestEKUParsingPolicyCA.cer
call certreq.exe -new -q -cert "TestEKUParsingPolicyCA"  TestEKUParsingIssuingCA.ini          TestEKUParsingIssuingCA.cer
call certreq.exe -new -q -cert "TestEKUParsingIssuingCA" TestEKUParsingLeafSigner.ini         TestEKUParsingLeafSigner.cer
call certreq.exe -new -q -cert "TestEKUParsingIssuingCA" TestEKUParsingLeafSignerPid12345.ini TestEKUParsingLeafSignerPid12345.cer
call certreq.exe -new -q -cert "TestEKUParsingIssuingCA" TestEKUParsingNoEKUsInSigner.ini     TestEKUParsingNoEKUsInSigner.cer
call certreq.exe -new -q -cert "TestEKUParsingIssuingCA" TestEKUParsingLeafSignerPid1.ini     TestEKUParsingLeafSignerPid1.cer
