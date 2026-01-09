# Project Saturn: Secure Device Lifecycle & Instrumentation

![Status](https://img.shields.io/badge/Status-Verified-brightgreen)
![Tests](https://img.shields.io/badge/Tests-Passing-blue)

This directory contains the reference implementation for **Project Saturn**, a secure, AOSP-integrated system for privileged device management and instrumentation loading.

## 🚀 Verified Environment

This project has been tested and verified in a standalone Linux environment (mimicking Android native layer).

*   **Native Daemon (`satord`)**: Verified path traversal prevention, package name validation, and execution logic.
*   **System Service (`DeviceLifecycleService`)**: Verified permissions enforcement and logic flow via JUnit.

To run the verification suite:
```bash
./setup_and_test.sh
# Installs dependencies, compiles C++/Java components, and runs all unit tests.
```

## 📂 Structure

*   `frameworks/base/`: Java framework components.
    *   `android.app.InstrumentationLoader`: Patches into ActivityThread to securely load vendor libraries.
    *   `com.android.server.lifecycle.DeviceLifecycleService`: System Service exposing IDeviceLifecycle AIDL.
*   `vendor/google/services/satord/`: Native C++ helper daemon (`satord`).
    *   Handles privileged operations (backup/restore, factory reset).
*   `device/google/bluejay/`: Device configuration.
    *   `sepolicy/`: SELinux policy logic defining `satord` domain.
    *   `init.satord.rc`: Init script to launch the daemon.

## 🛠 Integration Instructions

### 1. Framework Integration
*   **System Server**: Register `DeviceLifecycleService` in `SystemServer.java`.
*   **ActivityThread**: Apply `frameworks/base/core/java/android/app/ActivityThread.patch`:
    ```bash
    cd frameworks/base
    patch -p0 < .../ActivityThread.patch
    ```

### 2. Build Configuration
*   Add `satord` to `PRODUCT_PACKAGES` in your device makefile (`device.mk`).
*   Add the SELinux policy files to `BOARD_SEPOLICY_DIRS`.

### 3. Deployment
*   **Allowlist**: Push your configuration to `/vendor/etc/frd_allowlist.json`.
*   **Libraries**: Push signed vendor libs to `/vendor/framework/frd/<package>/`.
