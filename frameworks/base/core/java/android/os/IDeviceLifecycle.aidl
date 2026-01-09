package android.os;

/**
 * Privileged interface for device lifecycle management.
 * @hide
 */
interface IDeviceLifecycle {
    void backupAppData(String packageName, String destPath);
    void restoreAppData(String packageName, String srcPath);
    void requestFactoryReset(boolean wipeEsim);
    void setProvisioningFlags(in Map flags);
}
