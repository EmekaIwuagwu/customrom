package android.app;

import static org.junit.Assert.*;
import static org.mockito.Mockito.*;

import android.util.Log;
import java.io.File;
import java.nio.file.Files;
import java.security.MessageDigest;
import java.util.HexFormat;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

@RunWith(MockitoJUnitRunner.class)
public class InstrumentationLoaderTest {

    @Rule
    public TemporaryFolder tempFolder = new TemporaryFolder();

    @Mock
    InstrumentationLoader.NativeLoader mockLoader;

    private File mAllowListFile;
    private File mVendorDir;

    @Before
    public void setUp() throws Exception {
        // Create mock paths
        mAllowListFile = tempFolder.newFile("frd_allowlist.json");
        mVendorDir = tempFolder.newFolder("vendor_libs");
        
        // Inject paths
        InstrumentationLoader.setPathsForTesting(
            mAllowListFile.getAbsolutePath(),
            mVendorDir.getAbsolutePath() + "/"
        );
        InstrumentationLoader.setLoaderForTesting(mockLoader);
    }

    private String calculateHash(byte[] content) throws Exception {
        byte[] hash = MessageDigest.getInstance("SHA-256").digest(content);
        return HexFormat.of().formatHex(hash);
    }

    @Test
    public void testLoad_Success() throws Exception {
        String pkg = "com.example.secure";
        String libName = "libsecure.so";
        // Cast to byte to avoid lossy conversion error
        byte[] libContent = new byte[] {(byte)0xCA, (byte)0xFE, (byte)0xBA, (byte)0xBE};
        String hash = calculateHash(libContent);

        // Setup filesystem
        File pkgDir = new File(mVendorDir, pkg);
        pkgDir.mkdirs();
        File libFile = new File(pkgDir, libName);
        Files.write(libFile.toPath(), libContent);

        // Setup Allowlist
        String json = String.format("{\"%s\": {\"enabled\": true, \"lib_name\": \"%s\", \"sha256\": \"%s\"}}", 
                                    pkg, libName, hash);
        Files.writeString(mAllowListFile.toPath(), json);

        // Run
        InstrumentationLoader.attemptLoad(pkg, "/data/app/some.apk");

        // Verify loaded
        verify(mockLoader, times(1)).load(libFile.getAbsolutePath());
    }

    @Test
    public void testLoad_BadHash_Blocked() throws Exception {
        String pkg = "com.example.hacked";
        String libName = "libhacked.so";
        byte[] libContent = new byte[] {(byte)0xDE, (byte)0xAD, (byte)0xBE, (byte)0xEF}; // Malicious
        String goodHash = calculateHash(new byte[] {0x00}); // allowlist expects different content

        // Setup filesystem
        File pkgDir = new File(mVendorDir, pkg);
        pkgDir.mkdirs();
        File libFile = new File(pkgDir, libName);
        Files.write(libFile.toPath(), libContent);

        // Setup Allowlist
        String json = String.format("{\"%s\": {\"enabled\": true, \"lib_name\": \"%s\", \"sha256\": \"%s\"}}", 
                                    pkg, libName, goodHash);
        Files.writeString(mAllowListFile.toPath(), json);

        // Run
        InstrumentationLoader.attemptLoad(pkg, "/data/app/some.apk");

        // Verify NOT loaded
        verify(mockLoader, never()).load(anyString());
    }

    @Test
    public void testLoad_PathTraversal_Blocked() throws Exception {
        String pkg = "com.example.traverse";
        // Attempt to step out of vendor dir
        String libName = "../../../../etc/passwd"; 
        // Hash doesn't matter as path check comes first usually, but let's provide one
        String hash = "0000";

        // Setup Allowlist
        String json = String.format("{\"%s\": {\"enabled\": true, \"lib_name\": \"%s\", \"sha256\": \"%s\"}}", 
                                    pkg, libName, hash);
        Files.writeString(mAllowListFile.toPath(), json);

        // Run
        InstrumentationLoader.attemptLoad(pkg, "/data/app/some.apk");

        // Verify NOT loaded
        verify(mockLoader, never()).load(anyString());
    }
}
