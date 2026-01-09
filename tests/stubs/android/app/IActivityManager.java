package android.app;

public interface IActivityManager {
    public void forceStopPackage(String packageName, int userId) throws android.os.RemoteException;
}
