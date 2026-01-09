package android.os;

import java.io.IOException;

public class LocalSocket implements java.io.Closeable {
    public LocalSocket() {}
    public void connect(LocalSocketAddress endpoint) throws IOException {}
    public java.io.InputStream getInputStream() throws IOException { return null; }
    public java.io.OutputStream getOutputStream() throws IOException { return null; }
    public void close() throws IOException {}
    public void shutdownInput() throws IOException {}
    public void shutdownOutput() throws IOException {}
}
