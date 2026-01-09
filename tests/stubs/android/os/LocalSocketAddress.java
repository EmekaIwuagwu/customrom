package android.os;

public class LocalSocketAddress {
    public enum Namespace {
        ABSTRACT, RESERVED, FILESYSTEM
    }
    public LocalSocketAddress(String name, Namespace namespace) {}
    public LocalSocketAddress(String name) {}
}
