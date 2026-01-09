package android.os;

public class Binder {
    public static int getCallingUid() {
        return 1000; // Default to SYSTEM_UID for tests
    }
    
    public static final void restoreCallingIdentity(long token) {}
    public static final long clearCallingIdentity() { return 0; }
}
