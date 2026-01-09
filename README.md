# Project Saturn: Secure Device Lifecycle & Instrumentation

This directory contains the reference implementation for **Project Saturn**, a secure, AOSP-integrated system for privileged device management and instrumentation loading.

## Structure

*   `frameworks/base/`: Java framework components.
    *   `android.app.InstrumentationLoader`: Patches into ActivityThread to securely load vendor libraries.
    *   `com.android.server.lifecycle.DeviceLifecycleService`: System Service exposing IDeviceLifecycle AIDL.
*   `vendor/google/services/satord/`: Native C++ helper daemon (`satord`).
    *   Handles privileged operations (backup/restore, factory reset).
*   `device/google/bluejay/`: Device configuration.
    *   `sepolicy/`: SELinux policy logic defining `satord` domain.
    *   `init.satord.rc`: Init script to launch the daemon.

## Integration Instructions

1.  **Framework Integration**:
    *   Register `DeviceLifecycleService` in `SystemServer.java`.
    *   Apply the `ActivityThread` patch to call `InstrumentationLoader`.
2.  **Build**:
    *   Add `satord` to `PRODUCT_PACKAGES` in device makefile.
    *   Include SELinux policy files in `BOARD_SEPOLICY_DIRS`.
3.  **Deployment**:
    *   Push allowlist to `/vendor/etc/frd_allowlist.json`.
    *   Push vendor libs to `/vendor/framework/frd/<package>/`.
"# customrom" 
