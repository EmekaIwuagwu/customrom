package android.app;

import android.util.Log;
import java.io.File;
import java.nio.file.Files;
import java.security.MessageDigest;
import java.util.HexFormat;
import org.json.JSONObject;

/** @hide */
public class InstrumentationLoader {
    private static final String TAG = "InstrLoader";
    private static final String ALLOWLIST_PATH = "/vendor/etc/frd_allowlist.json";
    private static final String BASE_VENDOR_PATH = "/vendor/framework/frd/";

    public static void attemptLoad(String packageName, String apkPath) {
        File allowFile = new File(ALLOWLIST_PATH);
        if (!allowFile.exists()) return;

        try {
            // 1. Parse Allowlist
            // Note: In strict AOSP, use android.util.JsonReader or similar if org.json isn't available in core
            String jsonRaw = Files.readString(allowFile.toPath());
            JSONObject whitelist = new JSONObject(jsonRaw);
            
            if (!whitelist.has(packageName)) return;
            JSONObject config = whitelist.getJSONObject(packageName);

            if (!config.getBoolean("enabled")) return;

            String libName = config.getString("lib_name"); // e.g., "libfrd_agent.so"
            String expectedHash = config.getString("sha256");

            // 2. Validate Path (Prevent traversal)
            File libFile = new File(BASE_VENDOR_PATH + packageName + "/" + libName);
            if (!libFile.getCanonicalPath().startsWith(BASE_VENDOR_PATH)) {
                Log.e(TAG, "AUDIT: Path traversal attempt detected: " + packageName);
                return;
            }

            // 3. Verify Checksum
            byte[] fileBytes = Files.readAllBytes(libFile.toPath());
            byte[] hash = MessageDigest.getInstance("SHA-256").digest(fileBytes);
            String actualHash = HexFormat.of().formatHex(hash);

            if (!actualHash.equalsIgnoreCase(expectedHash)) {
                Log.e(TAG, "AUDIT: Checksum mismatch for " + packageName + ". Loading blocked.");
                return;
            }

            // 4. Load
            System.load(libFile.getAbsolutePath());
            Log.i(TAG, "AUDIT: Loaded authorized instrumentation for " + packageName);

        } catch (Exception e) {
            Log.w(TAG, "Error checking instrumentation: " + e.getMessage());
        }
    }
}
