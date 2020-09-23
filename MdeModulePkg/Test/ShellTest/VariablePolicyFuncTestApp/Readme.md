# variable Policy Unit Tests

## &#x1F539; Copyright
Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

## About This Test
This test verifies functionality of the Variable Policy Protocol by registering various variable policies and exercising them, as well as tests locking the policy, disabling it, and dumping the policy entries.

Only policies that are created as a part of this test will be tested.
1. Try getting test context, if empty then get VP protocol, confirm that VP is not disabled by calling IsVariablePolicyEnabled. Log VP revision.
2. "No lock" policies:
    * check minsize enforcement
    * check maxsize enforcement
    * check musthave attr enforcement
    * check canthave attr enforcement
    * check one of the above with empty string policy i.e. name wildcard
    * check another one of the above with a "#" containing policy string
    * check policy prioritization by having a namespace-wide policy, a policy with a # wildcard, and a one-var specific policy and testing which one is enforced
3. "Lock now" policies (means if the var doesn't exist, it won't be created; if one exists, it can't be updated):
    * test a policy for an already existing variable, verify we can't write into that variable
    * create a policy for a non-existing variable and attempt to register such var
4. "Lock on create" policies (means the var can still be created, but no updates later, existing vars can't be updated):
    * create a var, lock it with LockOnCreate, attempt to update its contents
    * create LockOnCreate VP, attempt to create var with invalid size, then invalid attr, then create valid var, attempt to update its contents
5. "Lock on var state" policies (means the var protected by this policy can't be created or updated once the trigger is set)
    * create VP, trigger lock with a valid var, attempt to create a locked var, then modify the trigger var, create locked var
    * create VP, create targeted var, modify it, trigger lock, attempt to modify var
    * create VP, trigger lock with invalid (larger than one byte) var, see if VPE allows creation of the locked var (it should allow)
    * create VP, set locking var with wrong value, see if VPE allows creation of the locked var (should allow)
6. Attempt registering invalid policy entries
    * invalid required and banned attributes
    * large min size - let's say 2GB
    * max size equal to 0
    * invalid policy type
7. Exercise dumping policy. No need to check the validity of the dump blob.
8. Test registering a policy with a random version.
9. Lock VPE, make sure old policies are enforced, new ones can't be registered.
    * Register a LockOnCreate policy
    * Lock VPE
    * Test locking it again.
    * Verify one of the prior policies is enforced
    * Make sure we can create variables even if those are protected by LockOnCreate policy, after locking the VPE
    * Attempt to register new policies
    * Make sure can't disable VPE
    * Cleanup: save context and reboot
10. Disable variable policy and try some things
    * Locate Variable Policy Protocol
    * Make sure VP is enabled
    * Register a policy
    * Disable VPE
    * Call IsVariablePolicyEnabled to confirm it's disabled.
    * Make sure can't lock policy
    * Make sure the policy from a is no longer enforced
    * Final cleanup: delete vars that were created in some earlier test suites
