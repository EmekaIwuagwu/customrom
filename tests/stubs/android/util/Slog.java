package android.util;

public final class Slog {
    public static int v(String tag, String msg) { System.out.println("V/" + tag + ": " + msg); return 0; }
    public static int d(String tag, String msg) { System.out.println("D/" + tag + ": " + msg); return 0; }
    public static int i(String tag, String msg) { System.out.println("I/" + tag + ": " + msg); return 0; }
    public static int w(String tag, String msg) { System.out.println("W/" + tag + ": " + msg); return 0; }
    public static int w(String tag, String msg, Throwable tr) { System.out.println("W/" + tag + ": " + msg); tr.printStackTrace(); return 0; }
    public static int e(String tag, String msg) { System.out.println("E/" + tag + ": " + msg); return 0; }
    public static int e(String tag, String msg, Throwable tr) { System.out.println("E/" + tag + ": " + msg); tr.printStackTrace(); return 0; }
}
