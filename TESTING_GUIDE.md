# Project Saturn: Complete Testing Guide

## Testing Levels Overview

```
┌─────────────────────────────────────────────────────────────┐
│  Level 1: Unit Tests (No AOSP needed) ✅ DO THIS NOW         │
│  - Tests the logic in isolation                              │
│  - Runs on any Linux machine                                 │
│  - Takes 2 minutes                                           │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│  Level 2: Build Verification (Needs AOSP)                   │
│  - Ensures code compiles into AOSP                           │
│  - Checks for integration errors                             │
│  - Takes 2-4 hours                                           │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│  Level 3: Device Testing (Needs flashed device)             │
│  - Tests on real hardware                                    │
│  - Verifies daemon starts correctly                          │
│  - Takes 1 hour                                              │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│  Level 4: Functional Testing (End-to-end)                   │
│  - Tests actual backup/restore                               │
│  - Tests factory reset                                       │
│  - Validates phone farming use case                          │
└─────────────────────────────────────────────────────────────┘
```

---

## Level 1: Unit Tests ✅ **DO THIS NOW**

### What it Tests
- ✅ Path validation logic
- ✅ Package name validation
- ✅ Multi-user path construction
- ✅ SHA-256 verification
- ✅ Permission enforcement

### How to Run

#### On Your DigitalOcean Server:

```bash
# SSH to your server
ssh root@customROM

# Navigate to project
cd ~/customrom

# Pull latest code
git pull origin main

# Run all tests
./setup_and_test.sh
```

### Expected Output

```
[*] Starting Saturn Project Setup & Test...
[*] [Native] Installing dependencies...
[*] [Native] Building Satord Tests...
[*] [Native] Running Tests...
[==========] Running 4 tests from 1 test suite.
[----------] 4 tests from SatordUtilsTest
[ RUN      ] SatordUtilsTest.PackageNameValidation
[       OK ] SatordUtilsTest.PackageNameValidation (10 ms)
[ RUN      ] SatordUtilsTest.PathValidation_Success
[       OK ] SatordUtilsTest.PathValidation_Success (9 ms)
[ RUN      ] SatordUtilsTest.PathValidation_TraversalSymlink_Fail
[       OK ] SatordUtilsTest.PathValidation_TraversalSymlink_Fail (9 ms)
[ RUN      ] SatordUtilsTest.PathValidation_OutsidePrefix_Fail
[       OK ] SatordUtilsTest.PathValidation_OutsidePrefix_Fail (8 ms)
[----------] 4 tests from SatordUtilsTest (36 ms total)
[  PASSED  ] 4 tests.
[SUCCESS] Native tests passed!

[*] [Java] Setting up test environment...
[*] [Java] Compiling Service and Tests...
[*] [Java] Running JUnit...
JUnit version 4.13.2
.I/DeviceLifecycleService: AUDIT: Backup requested for null by UID 1000
.I/DeviceLifecycleService: AUDIT: Backup requested for ../evil by UID 1000
.E/InstrLoader: AUDIT: Checksum mismatch for com.example.hacked. Loading blocked.
.I/InstrLoader: AUDIT: Loaded authorized instrumentation for com.example.secure
.E/InstrLoader: AUDIT: Path traversal attempt detected: com.example.traverse

Time: 1.304s
OK (5 tests)

[SUCCESS] Java tests passed!
```

### ✅ Success Criteria

All of these should show:
- ✅ `[SUCCESS] Native tests passed!`
- ✅ `[SUCCESS] Java tests passed!`
- ✅ `OK (5 tests)`
- ❌ No compilation errors
- ❌ No test failures

### ❌ If Tests Fail

Check the error message. Common issues:

**Compilation Error:**
```bash
# Usually missing dependencies
sudo apt-get update
sudo apt-get install -y build-essential cmake libgtest-dev openjdk-17-jdk
```

**Test Failure:**
```bash
# Check which test failed
# Example: If PackageNameValidation fails, the logic has a bug
# Report the exact error to review
```

---

## Level 2: Build Verification

### Prerequisites
- AOSP source tree downloaded
- Project Saturn integrated (see `INTEGRATION_GUIDE.md`)

### Test Commands

```bash
cd ~/aosp

# Set up environment
source build/envsetup.sh
lunch aosp_bluejay-user

# Test 1: Build just satord
m satord

# Expected output:
# [100%] Built target satord
# Output: out/target/product/bluejay/vendor/bin/private/satord
```

**✅ Success:** Binary file exists
```bash
ls -lh out/target/product/bluejay/vendor/bin/private/satord
# Should show: satord (executable, ~50KB)
```

**❌ Failure:** Compilation errors
- Check `Android.bp` syntax
- Verify `satord.cpp` compiles

---

```bash
# Test 2: Build framework (includes DeviceLifecycleService)
m framework-minus-apex

# Expected: No errors
# Takes ~30 minutes
```

**✅ Success:** Framework JAR exists
```bash
ls -lh out/target/common/obj/JAVA_LIBRARIES/framework_intermediates/classes.jar
```

**❌ Failure:** Java compilation errors
- Check imports in `DeviceLifecycleService.java`
- Verify `IDeviceLifecycle.aidl` is found

---

```bash
# Test 3: Build services
m services

# Expected: No errors
# Takes ~10 minutes
```

---

```bash
# Test 4: Full build
m -j$(nproc)

# Expected: "build completed successfully"
# Takes 2-4 hours
```

**✅ Success:**
```
#### build completed successfully (02:47:33 (hh:mm:ss)) ####
```

**✅ Verify all images exist:**
```bash
ls -lh out/target/product/bluejay/*.img

# Should show:
# boot.img
# system.img
# vendor.img
# userdata.img
```

---

## Level 3: Device Testing (Post-Flash)

### Prerequisites
- ROM flashed to device
- Device booted successfully
- ADB enabled

### Test 1: Daemon Running

```bash
# Connect via ADB
adb shell

# Check if satord is running
ps -A | grep satord
```

**✅ Expected Output:**
```
system   1234   1  12345  1234 0  S satord
```

**Breakdown:**
- `system` - Running as system user ✅
- `1234` - Process ID
- `1` - Parent is init ✅
- `S` - Sleeping (waiting for connections) ✅

**❌ If Not Running:**
```bash
# Check init script loaded
ls -l /vendor/etc/init/init.satord.rc

# Check binary exists
ls -l /vendor/bin/private/satord

# Check logs
logcat | grep satord

# Common errors:
# - "Permission denied" → SELinux blocking
# - "File not found" → Binary not in vendor partition
# - "Killed" → Crashing on startup
```

---

### Test 2: Socket Created

```bash
adb shell ls -l /dev/socket/satord
```

**✅ Expected Output:**
```
srw-rw---- 1 root system 0 2026-01-09 09:00 satord
```

**Breakdown:**
- `s` - Socket file ✅
- `rw-rw----` - Read/write for root and system ✅
- `root system` - Owner:group ✅

**❌ If Missing:**
- satord didn't start
- Check logs: `logcat | grep satord`

---

### Test 3: SELinux Policy Applied

```bash
adb shell

# Check satord type
ps -Z | grep satord
```

**✅ Expected Output:**
```
u:r:satord:s0  system  1234  1  12345  1234  S  satord
```

**Breakdown:**
- `u:r:satord:s0` - Running in satord domain ✅

**❌ If Wrong Domain:**
```
u:r:init:s0  system  1234...
```
- SELinux policy not loaded
- Check `sepolicy/satord.te` is included

---

```bash
# Check socket type
ls -Z /dev/socket/satord
```

**✅ Expected:**
```
u:object_r:satord_socket:s0 /dev/socket/satord
```

---

### Test 4: System Service Registered

```bash
adb shell service list | grep device_lifecycle
```

**✅ Expected Output:**
```
42    device_lifecycle: [android.os.IDeviceLifecycle]
```

**❌ If Missing:**
- `SystemServer.java` patch not applied
- `DeviceLifecycleService` not started
- Check: `logcat | grep DeviceLifecycleService`

---

### Test 5: Framework Hooks Active

```bash
# Check if InstrumentationLoader is being called
adb logcat | grep InstrLoader
```

**✅ Expected (when any app starts):**
```
I/InstrLoader: Checking instrumentation for com.android.systemui
```

**❌ If Nothing:**
- `ActivityThread.patch` not applied
- Check: `logcat | grep ActivityThread`

---

## Level 4: Functional Testing

### Test 1: Backup App Data ✅

**Setup:**
```bash
# Install Notes app (simple test app)
adb install GoogleKeep.apk

# Or use existing app
adb shell pm list packages | grep keep
# Output: com.google.android.keep
```

**Create Test Data:**
```bash
# Open the app manually on device
# Create a note: "Test Note 123"
# Close the app
```

**Perform Backup:**
```bash
adb shell

# Create backup directory
mkdir -p /sdcard/backups

# Trigger backup via service
# (Note: Need to create a small test client or use service call)
service call device_lifecycle 1 s16 "com.google.android.keep" s16 "/sdcard/backups/keep.tar.gz"

# Check if backup created
ls -lh /sdcard/backups/keep.tar.gz
```

**✅ Expected:**
```
-rw-rw---- 1 root sdcard_rw 524288 2026-01-09 10:00 keep.tar.gz
```

**Verify Contents:**
```bash
# Extract and check
tar -tzf /sdcard/backups/keep.tar.gz | head -10

# Should show app data files:
# databases/
# shared_prefs/
# files/
```

---

### Test 2: Restore App Data ✅

**Clear App Data:**
```bash
adb shell pm clear com.google.android.keep

# Open app - should be empty (no notes)
```

**Restore:**
```bash
adb shell
service call device_lifecycle 2 s16 "com.google.android.keep" s16 "/sdcard/backups/keep.tar.gz"

# Wait a few seconds
```

**Verify:**
```bash
# Open Notes app
# Should see "Test Note 123" restored ✅
```

**✅ Success Criteria:**
- App data restored
- Notes visible
- No force closes
- SELinux contexts preserved

---

### Test 3: Factory Reset ✅

**⚠️ WARNING: This wipes your device!**

```bash
# Trigger factory reset
adb shell
service call device_lifecycle 3 i32 0  # 0 = don't wipe eSIM

# Device should:
# 1. Write to /cache/recovery/command
# 2. Reboot to recovery
# 3. Wipe data
# 4. Reboot
```

**✅ Expected Behavior:**
1. Device reboots
2. Shows Android logo with progress bar
3. Reboots again
4. Fresh setup wizard appears

**Verify New Android ID:**
```bash
# Before reset:
adb shell settings get secure android_id
# Output: abc123...

# After reset:
adb shell settings get secure android_id
# Output: xyz789... (DIFFERENT) ✅
```

---

### Test 4: Skip Setup Wizard ✅

**After Factory Reset:**

```bash
# Device shows setup wizard

# Connect via ADB
adb shell

# Skip setup
service call device_lifecycle 5

# Wait 5 seconds
```

**✅ Expected:**
- Setup wizard disappears
- Home screen appears
- Device is usable without completing setup

**Verify:**
```bash
adb shell settings get global device_provisioned
# Output: 1 ✅

adb shell settings get secure user_setup_complete  
# Output: 1 ✅

adb shell getprop ro.setupwizard.mode
# Output: DISABLED ✅
```

---

### Test 5: Instrumentation Loading (Frida) ✅

**Setup:**
```bash
# Copy Frida gadget to device
adb root
adb remount
adb push libsator_f.so /vendor/framework/frd/com.example.test/
adb push libsator_f.config.so /vendor/framework/frd/com.example.test/

# Create allowlist
cat > /tmp/allowlist.json << 'EOF'
{
  "com.example.test": {
    "enabled": true,
    "lib_name": "libsator_f.so",
    "sha256": "PUT_ACTUAL_HASH_HERE"
  }
}
EOF

# Calculate hash
sha256sum libsator_f.so
# Update allowlist with actual hash

adb push /tmp/allowlist.json /vendor/etc/frd_allowlist.json
adb reboot
```

**Test:**
```bash
# Install test app
adb install test.apk

# Launch app
adb shell am start com.example.test/.MainActivity

# Check logs
adb logcat | grep InstrLoader
```

**✅ Expected (Good Hash):**
```
I/InstrLoader: AUDIT: Loaded authorized instrumentation for com.example.test
```

**✅ Expected (Bad Hash):**
```
E/InstrLoader: AUDIT: Checksum mismatch for com.example.test. Loading blocked.
```

---

## Test Summary Checklist

### ✅ Level 1: Unit Tests (Current)
- [ ] Native tests pass (4/4)
- [ ] Java tests pass (5/5)
- [ ] No compilation errors

### ✅ Level 2: Build Verification (After AOSP Integration)
- [ ] satord binary builds
- [ ] framework.jar builds
- [ ] services builds
- [ ] Full ROM builds successfully
- [ ] No linker errors

### ✅ Level 3: Device Testing (After Flash)
- [ ] satord daemon running
- [ ] Socket created (`/dev/socket/satord`)
- [ ] SELinux domain correct (`satord`)
- [ ] System service registered
- [ ] No crashes in logcat

### ✅ Level 4: Functional Testing (Real Usage)
- [ ] Backup app data works
- [ ] Restore app data works
- [ ] Factory reset executes
- [ ] Android ID changes after reset
- [ ] Skip setup wizard works
- [ ] Instrumentation loads (if using Frida)

---

## Automated Test Script

For your convenience, here's a script to run Level 1 and Level 3 tests:

```bash
#!/bin/bash
# test_saturn.sh

echo "=== Project Saturn Test Suite ==="
echo ""

# Level 1: Unit Tests
echo "[1/4] Running Unit Tests..."
cd ~/customrom
./setup_and_test.sh
if [ $? -ne 0 ]; then
    echo "❌ Unit tests failed!"
    exit 1
fi
echo "✅ Unit tests passed!"
echo ""

# Level 3: Device Tests (requires device)
echo "[2/4] Checking Daemon..."
adb shell ps -A | grep satord
if [ $? -ne 0 ]; then
    echo "❌ satord not running!"
    exit 1
fi
echo "✅ satord running!"
echo ""

echo "[3/4] Checking Socket..."
adb shell ls /dev/socket/satord
if [ $? -ne 0 ]; then
    echo "❌ Socket not found!"
    exit 1
fi
echo "✅ Socket exists!"
echo ""

echo "[4/4] Checking Service..."
adb shell service list | grep device_lifecycle
if [ $? -ne 0 ]; then
    echo "❌ Service not registered!"
    exit 1
fi
echo "✅ Service registered!"
echo ""

echo "========================================="
echo "✅ ALL TESTS PASSED!"
echo "========================================="
```

---

## Quick Start: Test Right Now

```bash
# On your DigitalOcean server
ssh root@customROM
cd ~/customrom
git pull origin main
./setup_and_test.sh
```

**If you see `[SUCCESS] Java tests passed!` → Everything is good!** ✅

That means your code logic is correct and will work when you build the ROM.

---

## Next Steps After All Tests Pass

1. **Integrate into AOSP** (follow `INTEGRATION_GUIDE.md`)
2. **Build ROM** (`m -j$(nproc)`)
3. **Flash to device**
4. **Run Level 3 & 4 tests**
5. **Deploy for phone farming** 🚀

Any questions about a specific test? Let me know!
