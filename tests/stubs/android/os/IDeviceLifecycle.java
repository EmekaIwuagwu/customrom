package android.os;

public interface IDeviceLifecycle extends android.os.IInterface {
    public static abstract class Stub implements IDeviceLifecycle {
        public static IDeviceLifecycle asInterface(android.os.IBinder obj) { return null; }
        public android.os.IBinder asBinder() { return null; }
    }
    
    void backupAppData(String packageName, String destPath) throws android.os.RemoteException;
    void restoreAppData(String packageName, String srcPath) throws android.os.RemoteException;
    void requestFactoryReset(boolean wipeEsim) throws android.os.RemoteException;
    void setProvisioningFlags(java.util.Map flags) throws android.os.RemoteException;
}
