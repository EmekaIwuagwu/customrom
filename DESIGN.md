# Project Saturn: Design & Security Architecture

## 1. System Overview

**Project Saturn** enables controlled, privileged device lifecycle management (backup, restore, factory reset) and vendor-signed instrumentation loading on strict AOSP builds (Pixel 6a target).

The system is designed with **Least Privilege** as the core tenet. It splits responsibilities between a policy-enforcing Java System Service and a minimal, capabilities-constrained Native Daemon.

### 1.1 Components

| Component | Responsibility | Privileges |
| :--- | :--- | :--- |
| **ActivityThread Patch** | Hooks app startup to load instrumentation. | App Context (u:r:untrusted_app...) |
| **InstrumentationLoader** | Verifies allowlist/signatures and calls dlopen(). | App Context |
| **DeviceLifecycleService** | Validates caller UID, enforces business logic, talks to daemon. | `system_server` (u:r:system_server:s0) |
| **Satord (Native Daemon)** | Executes commands: tar, restore, wipe. | `satord` (u:r:satord:s0) |
| **Management Client** | CLI or Signed App to invoke service. | `shell` or `system_app` |

---

## 2. Security Architecture & Threat Model

### 2.1 Threat Model

| Threat | Mitigation |
| :--- | :--- |
| **Malicious App invokes Service** | Service checks `Binder.getCallingUid()` against allowlisted Management UIDs (System/Root/DeviceOwner). |
| **Attacker compromsies Satord** | `satord` runs in a restricted SELinux domain. It cannot access user data (photos/messages) generally, only specific `/data/data/<pkg>` when instructed, and cannot make network connections. |
| **Path Traversal (e.g. `../../etc/passwd`)** | `satord` uses `realpath()` to resolve paths and checks strict prefixes (`/data/data/`, `/sdcard/`). |
| **Malicious Instrumentation Lib** | Loader enforces SHA-256 allowlist and vendor signature check. Libs must be in read-only `/vendor` partition. |
| **Data Leak via Backup** | Backup destination restricted to specific directories. Service logs all export events. |

### 2.2 SELinux Policy

The `satord` domain is defined to be as restrictive as possible:
*   **No Network**: `satord` has no `net_raw` or `internet` permissions.
*   **IPC Isolation**: Only `system_server` can connect to `/dev/socket/satord`.
*   **Filesystem**:
    *   Read/Write: `/cache/recovery/command` (for reset).
    *   Read/Write: `/data/data/` (scoped by request logic, though policy allows broad access to `app_data_file`, logic constrains it).
    *   Read: `/vendor/framework/frd/`.

### 2.3 Interface Security (Binder & Socket)

1.  **AIDL Interface**:
    *   Protected by signature-level permission: `android.permission.MANAGE_DEVICE_LIFECYCLE`.
    *   Explicit checks for `Process.SYSTEM_UID` or `Process.ROOT_UID`.

2.  **Native Socket Protocol**:
    *   **Transport**: `AF_UNIX` / `SOCK_STREAM`.
    *   **Access Control**: Socket file 0660 `root:system`.
    *   **Peer Auth**: usage of `SO_PEERCRED` to reject any connection not from UID 1000 (`system`).
    *   **Input Validation**: 4KB max payload, strict OpCode checking.

---

## 3. Data Flow

### 3.1 Instrumentation Loading
1.  App starts -> `ActivityThread.handleBindApplication`.
2.  `InstrumentationLoader.attemptLoad(pkg)` called.
3.  Loader reads `/vendor/etc/frd_allowlist.json`.
4.  If matches:
    *   Verify file exists at `/vendor/framework/frd/<pkg>/<lib>`.
    *   Calculate SHA-256 of lib.
    *   Compare with allowlist hash.
    *   Call `System.load(path)`.

### 3.2 Backup Operation
1.  Admin calls `IDeviceLifecycle.backupAppData("com.foo", "/sdcard/backups/foo.tar")`.
2.  Service validates caller permission.
3.  Service sends OpCode 1 + payload to `/dev/socket/satord`.
4.  `satord`:
    *   Verifies peer is `system_server`.
    *   Resolves and validates paths (must start with `/data/data` and `/sdcard`).
    *   Executes `tar` (or internal logic).
    *   Returns status code.

---

## 4. Rollback & OTA Compatibility

*   **OTA Survival**:
    *   `DeviceLifecycleService` is part of `framework.jar`; persists across updates if integrated into build.
    *   `satord` is in `vendor` partition.
    *   Allowlist config is in `/vendor/etc`, preserved or updated via OTA.
*   **Rollback**:
    *   If a bad update occurs, standard A/B slot rollback applies.
    *   To revoke a specific instrumentation library, push an OTA with an updated `frd_allowlist.json` removing the entry or updating the hash.

## 5. Auditing

*   All privileged ops logged to `logcat -b system` (DeviceLifecycleService) and `logcat -b kernel` (satord via kmsg).
*   **Production Audit**: Monitor logs for `AUDIT:` tag.
