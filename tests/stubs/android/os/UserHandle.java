package android.os;

public final class UserHandle {
    public static int getCallingUserId() { return 0; }
    public static final UserHandle SYSTEM = new UserHandle();
}
