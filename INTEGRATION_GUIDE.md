# Project Saturn: Complete AOSP Integration Guide

This comprehensive guide will walk you through integrating Project Saturn into an Android Open Source Project (AOSP) or LineageOS build from start to finish.

---

## Table of Contents
1. [Understanding the Architecture](#1-understanding-the-architecture)
2. [Prerequisites](#2-prerequisites)
3. [Setting Up Your Build Environment](#3-setting-up-your-build-environment)
4. [Obtaining the Android Source Code](#4-obtaining-the-android-source-code)
5. [Integrating Project Saturn Components](#5-integrating-project-saturn-components)
6. [Applying System Patches](#6-applying-system-patches)
7. [Configuring the Build System](#7-configuring-the-build-system)
8. [Building the ROM](#8-building-the-rom)
9. [Flashing and Testing](#9-flashing-and-testing)
10. [Troubleshooting](#10-troubleshooting)
11. [Verification Checklist](#11-verification-checklist)

---

## 1. Understanding the Architecture

Before you begin, it's important to understand what Project Saturn does and how it fits into Android.

### What is Project Saturn?

Project Saturn is a **security framework** that adds three critical components to Android:

1. **`satord` (Native Daemon)**: A C++ daemon running as a privileged service that handles:
   - Secure app data backup/restore
   - Factory reset operations
   - Provisioning flag management
   
2. **`DeviceLifecycleService` (System Service)**: A Java service that:
   - Exposes Binder APIs for privileged operations
   - Enforces permissions and validates inputs
   - Communicates with the `satord` daemon via Unix sockets

3. **`InstrumentationLoader` (Security Module)**: A Java component that:
   - Validates and loads vendor-signed libraries
   - Enforces SHA-256 checksum verification
   - Prevents unauthorized code injection

### Architecture Diagram
```
┌─────────────────────────────────────────────┐
│           App Layer (User Apps)             │
└──────────────────┬──────────────────────────┘
                   │ Binder IPC
┌──────────────────▼──────────────────────────┐
│      DeviceLifecycleService (System)        │
│  - Permission enforcement                    │
│  - Input validation                          │
└──────────────────┬──────────────────────────┘
                   │ Unix Socket
┌──────────────────▼──────────────────────────┐
│         satord (Native Daemon)              │
│  - Path validation                           │
│  - Secure command execution                  │
└─────────────────────────────────────────────┘
```

---

## 2. Prerequisites

### Hardware Requirements
- **CPU**: Multi-core processor (8+ cores recommended)
- **RAM**: Minimum 16GB (32GB+ recommended)
- **Storage**: 250GB+ free space (SSD strongly recommended)
- **Internet**: Stable, high-bandwidth connection

### Software Requirements
- **Operating System**: Linux (Ubuntu 20.04 LTS or 22.04 LTS recommended)
- **Python**: Version 3.6 or higher
- **Git**: Version 2.17 or higher
- **Java**: OpenJDK 11 (for Android 12+)

### Knowledge Prerequisites
- Basic Linux command line usage
- Understanding of Android build system concepts
- Familiarity with Git version control
- Basic understanding of C++ and Java (helpful but not required)

---

## 3. Setting Up Your Build Environment

### Step 3.1: Install Required Packages

Open a terminal and run the following commands:

```bash
# Update package lists
sudo apt-get update

# Install essential build tools
sudo apt-get install -y \
    git-core gnupg flex bison build-essential zip curl zlib1g-dev \
    libc6-dev-i386 libncurses5 lib32ncurses5-dev x11proto-core-dev \
    libx11-dev lib32z1-dev libgl1-mesa-dev libxml2-utils xsltproc \
    unzip fontconfig
```

### Step 3.2: Install Repo Tool

The `repo` tool manages multiple Git repositories in AOSP.

```bash
# Create bin directory
mkdir -p ~/bin

# Download repo
curl https://storage.googleapis.com/git-repo-downloads/repo > ~/bin/repo

# Make it executable
chmod a+x ~/bin/repo

# Add to PATH
echo 'export PATH=~/bin:$PATH' >> ~/.bashrc
source ~/.bashrc

# Verify installation
repo version
```

**Expected Output:**
```
repo version v2.x
```

### Step 3.3: Configure Git

```bash
git config --global user.name "Your Name"
git config --global user.email "your.email@example.com"
```

### Step 3.4: Install JDK

For Android 12 and above:
```bash
sudo apt-get install -y openjdk-11-jdk
```

Verify:
```bash
java -version
```

**Expected Output:**
```
openjdk version "11.0.x"
```

---

## 4. Obtaining the Android Source Code

### Option A: AOSP (Pure Android)

#### Step 4.1: Create Source Directory
```bash
mkdir -p ~/aosp
cd ~/aosp
```

#### Step 4.2: Initialize Repository

For the latest Android version:
```bash
repo init -u https://android.googlesource.com/platform/manifest -b main
```

For a specific version (e.g., Android 13):
```bash
repo init -u https://android.googlesource.com/platform/manifest -b android-13.0.0_r1
```

#### Step 4.3: Download Source Code

This will take 2-4 hours depending on your connection:
```bash
repo sync -c -j8
```

**Flags explained:**
- `-c`: Current branch only (saves space)
- `-j8`: Use 8 parallel jobs (adjust based on your CPU cores)

### Option B: LineageOS (Community ROM)

#### Step 4.1: Create Source Directory
```bash
mkdir -p ~/lineage
cd ~/lineage
```

#### Step 4.2: Initialize LineageOS Repository

For LineageOS 20 (Android 13):
```bash
repo init -u https://github.com/LineageOS/android.git -b lineage-20.0
```

#### Step 4.3: Download Source Code
```bash
repo sync -c -j8
```

---

## 5. Integrating Project Saturn Components

Now we'll copy the Project Saturn files into your Android source tree. This guide assumes:
- Your AOSP/LineageOS source is at `~/aosp` (adjust paths as needed)
- This repository is cloned at `~/customrom`

### Step 5.1: Verify Source Tree

First, confirm your source tree is ready:
```bash
cd ~/aosp
ls -la

# You should see directories like:
# frameworks/, vendor/, device/, build/, etc.
```

### Step 5.2: Copy Native Daemon

The `satord` daemon needs to be placed in the vendor directory.

```bash
# Create the directory structure
mkdir -p ~/aosp/vendor/google/services/satord

# Copy all daemon files
cp -r ~/customrom/vendor/google/services/satord/* \
      ~/aosp/vendor/google/services/satord/

# Verify the files
ls ~/aosp/vendor/google/services/satord/
```

**Expected files:**
- `satord.cpp`
- `satord_utils.cpp`
- `satord_utils.h`
- `Android.bp`
- `init.satord.rc`

### Step 5.3: Copy Java Framework Components

#### System Service
```bash
# Ensure the directory exists
mkdir -p ~/aosp/frameworks/base/services/core/java/com/android/server/lifecycle

# Copy the service
cp ~/customrom/frameworks/base/services/core/java/com/android/server/lifecycle/DeviceLifecycleService.java \
   ~/aosp/frameworks/base/services/core/java/com/android/server/lifecycle/

# Verify
ls -l ~/aosp/frameworks/base/services/core/java/com/android/server/lifecycle/DeviceLifecycleService.java
```

#### Instrumentation Loader
```bash
# Copy the loader
cp ~/customrom/frameworks/base/core/java/android/app/InstrumentationLoader.java \
   ~/aosp/frameworks/base/core/java/android/app/

# Verify
ls -l ~/aosp/frameworks/base/core/java/android/app/InstrumentationLoader.java
```

### Step 5.4: Copy AIDL Interface

```bash
# Create AIDL directory
mkdir -p ~/aosp/frameworks/base/core/java/android/os

# Copy the interface definition
cp ~/customrom/frameworks/base/core/java/android/os/IDeviceLifecycle.aidl \
   ~/aosp/frameworks/base/core/java/android/os/

# Verify
ls -l ~/aosp/frameworks/base/core/java/android/os/IDeviceLifecycle.aidl
```

### Step 5.5: Copy Device Configuration

This step depends on your target device. For Google Pixel 6a (codename: bluejay):

```bash
# Create device-specific directories
mkdir -p ~/aosp/device/google/bluejay/sepolicy

# Copy init script
cp ~/customrom/device/google/bluejay/init.satord.rc \
   ~/aosp/device/google/bluejay/

# Copy SELinux policy
cp ~/customrom/device/google/bluejay/sepolicy/satord.te \
   ~/aosp/device/google/bluejay/sepolicy/

cp ~/customrom/device/google/bluejay/sepolicy/file_contexts \
   ~/aosp/device/google/bluejay/sepolicy/

# Verify
ls -l ~/aosp/device/google/bluejay/
```

**For Other Devices:**
Replace `google/bluejay` with your device path, e.g.:
- Samsung Galaxy: `samsung/a52s`
- OnePlus: `oneplus/bacon`
- Xiaomi: `xiaomi/mido`

You can find your device path by checking existing device trees in `~/aosp/device/`.

---

## 6. Applying System Patches

Project Saturn requires modifications to core Android files. We provide patches for easy integration.

### Step 6.1: Apply SystemServer Patch

This patch registers the `DeviceLifecycleService` with the Android system.

```bash
cd ~/aosp

# Apply the patch
patch -p1 < ~/customrom/patches/SystemServer.patch
```

**If the patch fails:**
This usually means your Android version differs from the patch. Manually edit:
```bash
nano ~/aosp/frameworks/base/services/java/com/android/server/SystemServer.java
```

Find the `startOtherServices()` method and add:
```java
// Around line 1800-2000, after other service registrations
t.traceBegin("StartDeviceLifecycleService");
mSystemServiceManager.startService(DeviceLifecycleService.class);
t.traceEnd();
```

### Step 6.2: Apply Context Patch

This adds the service constant to Android's Context class.

```bash
cd ~/aosp
patch -p1 < ~/customrom/patches/Context.patch
```

**Manual approach if patch fails:**
```bash
nano ~/aosp/frameworks/base/core/java/android/content/Context.java
```

Add this constant (around line 5000):
```java
/**
 * Use with {@link #getSystemService(String)} to retrieve a
 * {@link android.os.IDeviceLifecycle} for secure device operations.
 * @hide
 */
public static final String DEVICE_LIFECYCLE_SERVICE = "device_lifecycle";
```

### Step 6.3: Apply ActivityThread Patch

This enables secure instrumentation loading.

```bash
cd ~/aosp
patch -p0 < ~/customrom/frameworks/base/core/java/android/app/ActivityThread.patch
```

**Manual approach:**
```bash
nano ~/aosp/frameworks/base/core/java/android/app/ActivityThread.java
```

Find the `handleBindApplication()` method (around line 6540) and add:
```java
// After: data.info = getPackageInfoNoCheck(data.appInfo, data.compatInfo);
// Add:
try {
    android.app.InstrumentationLoader.attemptLoad(
        data.appInfo.packageName, 
        data.info.getSourceDir()
    );
} catch (Throwable t) {
    android.util.Slog.e(TAG, "Failed to load Saturn instrumentation", t);
}
```

### Step 6.4: Verify All Patches Applied

```bash
# Check if files were modified
git -C ~/aosp/frameworks/base status

# You should see modified files:
# - services/java/com/android/server/SystemServer.java
# - core/java/android/content/Context.java
# - core/java/android/app/ActivityThread.java
```

---

## 7. Configuring the Build System

### Step 7.1: Register the AIDL Interface

Edit the Framework Makefile to include the new AIDL:
```bash
nano ~/aosp/frameworks/base/Android.bp
```

Find the `filegroup` named `framework-non-updatable-sources` and ensure AIDL files are included (they usually are by default with `**/*.aidl`).

### Step 7.2: Add satord to Product Packages

Find your device's makefile. For bluejay:
```bash
nano ~/aosp/device/google/bluejay/device-bluejay.mk
```

Add `satord` to the `PRODUCT_PACKAGES` list:
```makefile
PRODUCT_PACKAGES += \
    satord \
    # ... other packages
```

### Step 7.3: Configure SELinux

Edit your device's SELinux configuration:
```bash
nano ~/aosp/device/google/bluejay/BoardConfig.mk
```

Ensure the sepolicy directory is included:
```makefile
BOARD_SEPOLICY_DIRS += \
    device/google/bluejay/sepolicy
```

### Step 7.4: Add Init Script

Ensure the init script is installed. Edit:
```bash
nano ~/aosp/device/google/bluejay/device-bluejay.mk
```

Add:
```makefile
PRODUCT_COPY_FILES += \
    device/google/bluejay/init.satord.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/init.satord.rc
```

---

## 8. Building the ROM

### Step 8.1: Clean Previous Builds (Optional)

If this is a fresh build, skip this. Otherwise:
```bash
cd ~/aosp
make clean
```

### Step 8.2: Set Up Build Environment

```bash
cd ~/aosp
source build/envsetup.sh
```

**Expected output:**
```
including device/...
...
```

### Step 8.3: Choose Your Target

```bash
lunch
```

A menu will appear. Select your device, e.g.:
- For AOSP Pixel 6a: `aosp_bluejay-userdebug`
- For LineageOS: `lineage_bluejay-userdebug`

Or directly:
```bash
lunch aosp_bluejay-userdebug
```

### Step 8.4: Build the System

**Option A: Full Build (Recommended for first time)**
```bash
m -j$(nproc)
```

This will take 2-6 hours depending on your machine.

**Option B: Module-Specific Build (For testing)**
```bash
# Build just the modified components
m satord
m framework-minus-apex
m services
```

### Step 8.5: Monitor Build Progress

The build will show progress. Watch for:
- ✅ `[ nn% nnnn/nnnn]` - Build progress
- ❌ Red text - Compilation errors

**Common build output:**
```
[  1% 120/12000] target  C++: satord <= vendor/google/services/satord/satord.cpp
[ 45% 5400/12000] target Java: framework (out/target/common/obj/JAVA_LIBRARIES/framework_intermediates/classes)
[100% 12000/12000] Install system fs image: out/target/product/bluejay/system.img
```

### Step 8.6: Verify Build Success

At the end, you should see:
```
#### build completed successfully (HH:MM:SS (hh:mm:ss)) ####
```

The output files will be in:
```
~/aosp/out/target/product/bluejay/
```

---

## 9. Flashing and Testing

### Step 9.1: Prepare Your Device

1. **Enable Developer Options**:
   - Settings → About Phone → Tap "Build Number" 7 times

2. **Enable USB Debugging**:
   - Settings → System → Developer Options → USB Debugging (ON)

3. **Enable OEM Unlocking** (if not already unlocked):
   - Settings → System → Developer Options → OEM Unlocking (ON)

4. **Unlock Bootloader** (if needed - THIS WILL WIPE YOUR DEVICE):
   ```bash
   adb reboot bootloader
   fastboot flashing unlock
   ```

### Step 9.2: Flash the ROM

```bash
cd ~/aosp/out/target/product/bluejay

# Reboot to bootloader
adb reboot bootloader

# Flash all partitions (WARNING: WIPES DEVICE)
fastboot flashall -w

# Or flash individually:
fastboot flash boot boot.img
fastboot flash system system.img
fastboot flash vendor vendor.img
fastboot flash userdata userdata.img
```

### Step 9.3: First Boot

```bash
fastboot reboot
```

**First boot takes 5-10 minutes** - be patient!

### Step 9.4: Verify Installation

Once booted, connect via ADB:
```bash
adb shell

# Check if satord is running
ps -A | grep satord

# Expected output:
# system   1234   1  12345  1234 0  S satord

# Check the socket
ls -l /dev/socket/satord

# Expected:
# srw-rw---- 1 root system 0 2026-01-09 09:00 satord

exit
```

---

## 10. Troubleshooting

### Build Errors

#### Error: "AIDL interface not found"
**Solution:**
```bash
# Ensure AIDL is in the right place
ls ~/aosp/frameworks/base/core/java/android/os/IDeviceLifecycle.aidl

# Clean framework build
rm -rf ~/aosp/out/target/common/obj/JAVA_LIBRARIES/framework_intermediates
m framework
```

#### Error: "satord: undefined reference"
**Solution:**
```bash
# Check Android.bp syntax
cd ~/aosp/vendor/google/services/satord
cat Android.bp

# Rebuild module
m satord
```

#### Error: "SELinux denials"
**Solution:**
```bash
# Check logcat for denials
adb logcat | grep avc

# Add missing rules to sepolicy/satord.te
```

### Runtime Errors

#### satord Not Starting
**Check logs:**
```bash
adb logcat | grep satord
```

**Common causes:**
- SELinux blocking (check `adb logcat | grep denials`)
- Socket path permissions
- Missing dependencies

**Fix:**
```bash
# Check SELinux mode
adb shell getenforce

# Temporarily set to permissive for testing
adb shell setenforce 0

# Check if it starts now
adb shell ps -A | grep satord
```

---

## 11. Verification Checklist

After successful build and flash, verify each component:

### ✅ Native Daemon
```bash
adb shell ps -A | grep satord
# Should show: system ... satord
```

### ✅ Socket Created
```bash
adb shell ls -l /dev/socket/satord
# Should show: srw-rw---- ... root system ... satord
```

### ✅ SELinux Policy
```bash
adb shell sesearch -s satord -t satord_socket -c sock_file -A
# Should show allowed rules
```

### ✅ System Service Registered
```bash
adb shell service list | grep device_lifecycle
# Should show: device_lifecycle: [android.os.IDeviceLifecycle]
```

### ✅ Framework Integration
```bash
adb logcat | grep "InstrLoader"
# Should show loader attempting to check instrumentation
```

---

## Congratulations! 🎉

You have successfully integrated Project Saturn into your Android build. Your ROM now includes:
- ✅ Secure backup/restore via `satord`
- ✅ Privileged lifecycle management
- ✅ SHA-256 verified instrumentation loading

For questions or issues, please refer to the project's GitHub issues page.
