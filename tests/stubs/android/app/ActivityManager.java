package android.app;

public class ActivityManager {
    public static IActivityManager getService() {
        // Return a dummy implementation or null. 
        // For tests using Mockito, we might mock this static if we used PowerMock, 
        // but here we just return a simple no-op proxy to avoid NPE if possible.
        return new IActivityManager() {
             public void forceStopPackage(String packageName, int userId) throws android.os.RemoteException {}
        };
    }
}
