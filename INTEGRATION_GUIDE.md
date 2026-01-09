# How to Compile Project Saturn (AOSP Integration)

This repository contains the **source code** for the secure components. To compile them into a working Android OS (ROM), you must integrate them into a full Android Source Protocol (AOSP) tree.

## Prerequisites
*   A downloaded AOSP source tree (e.g., `~/aosp` or `~/lineage`).
*   A configured build environment (Linux).

---

## Step 1: Copy Components to AOSP Tree

Assuming your AOSP root is `~/aosp` and this repo is at `~/customrom`.

### 1. Copy the Native Daemon (`satord`)
The daemon goes into the `vendor` directory.
```bash
mkdir -p ~/aosp/vendor/google/services/satord
cp -r ~/customrom/vendor/google/services/satord/* ~/aosp/vendor/google/services/satord/
```

### 2. Copy the Java Framework Components
Copy the new System Service and Loader to `frameworks/base`.
```bash
# Service
cp ~/customrom/frameworks/base/services/core/java/com/android/server/lifecycle/DeviceLifecycleService.java \
   ~/aosp/frameworks/base/services/core/java/com/android/server/lifecycle/

# Loader
cp ~/customrom/frameworks/base/core/java/android/app/InstrumentationLoader.java \
   ~/aosp/frameworks/base/core/java/android/app/
```

### 3. Copy Device Configuration (SELinux & Init)
Merge the device-specific inputs (SELinux policy and init scripts) into your target device tree (e.g., Pixel 6a "bluejay").
```bash
# Example for Google Bluejay
cp ~/customrom/device/google/bluejay/init.satord.rc ~/aosp/device/google/bluejay/
cp -r ~/customrom/device/google/bluejay/sepolicy/* ~/aosp/device/google/bluejay/sepolicy/
```

---

## Step 2: Apply System Patches

You need to modify existing core Android files to "hook" your new components.

```bash
cd ~/aosp

# Apply patches (Paths may vary slightly depending on Android version)
patch -p1 < ~/customrom/patches/SystemServer.patch
patch -p1 < ~/customrom/patches/Context.patch
patch -p0 < ~/customrom/frameworks/base/core/java/android/app/ActivityThread.patch
```
*Note: If patches fail due to version differences, manually check the file contents and apply the changes.*

---

## Step 3: Register in Makefiles

You must tell the Android Build System (`soong`/`make`) to include your new daemon.

1.  **Open your Device Makefile**:
    e.g., `~/aosp/device/google/bluejay/device-bluejay.mk`
2.  **Add to `PRODUCT_PACKAGES`**:
    ```makefile
    PRODUCT_PACKAGES += satord
    ```
3.  **Register SELinux Policies**:
    Ensure the `sepolicy` directory is picked up. Often this is automatic if inside `device/.../sepolicy`, but check verifying `BOARD_SEPOLICY_DIRS` in `BoardConfig.mk`.

---

## Step 4: Build the ROM

Now you are ready to compile.

```bash
cd ~/aosp
source build/envsetup.sh

# Select your target (e.g., Pixel 6a userdebug)
lunch aosp_bluejay-userdebug

# Build the whole system
m

# OR, Build just your modules to verify (faster)
m satord framework-minus-apex services
```

## Step 5: Flash
```bash
adb reboot bootloader
fastboot flashall -w
```
