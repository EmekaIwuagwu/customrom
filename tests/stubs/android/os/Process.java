package android.os;

public class Process {
    public static final int SYSTEM_UID = 1000;
    public static final int ROOT_UID = 0;
    public static final int myUid() { return SYSTEM_UID; }
}
