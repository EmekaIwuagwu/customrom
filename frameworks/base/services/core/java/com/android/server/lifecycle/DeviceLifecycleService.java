package com.android.server.lifecycle;

import android.content.Context;
import android.os.Binder;
import android.os.IDeviceLifecycle;
import android.os.LocalSocket;
import android.os.LocalSocketAddress;
import android.os.RemoteException;
import android.util.Slog;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.Map;

/**
 * Orchestrates privileged device operations via the constrained 'satord' native daemon.
 */
public class DeviceLifecycleService extends IDeviceLifecycle.Stub {
    private static final String TAG = "DeviceLifecycleService";
    private static final String SOCKET_PATH = "/dev/socket/satord";
    private static final String PERMISSION = "android.permission.MANAGE_DEVICE_LIFECYCLE";
    
    // Command Opcodes matching satord.cpp
    private static final byte CMD_BACKUP = 1;
    private static final byte CMD_RESTORE = 2;
    private static final byte CMD_RESET = 3;
    private static final byte CMD_SET_FLAGS = 4;
    private static final byte CMD_SKIP_SETUP = 5;

    private final Context mContext;

    public DeviceLifecycleService(Context context) {
        mContext = context;
    }

    private void enforceAuth() {
        int uid = Binder.getCallingUid();
        // Allow System (1000) or Root (0)
        // In strict mode, we might also allow specific Device Owner UIDs if managed.
        if (uid != android.os.Process.SYSTEM_UID && uid != 0) {
            mContext.enforceCallingOrSelfPermission(PERMISSION, "Unauthorized caller");
        }
    }

    @Override
    public void backupAppData(String packageName, String destPath) throws RemoteException {
        enforceAuth();
        Slog.i(TAG, "AUDIT: Backup requested for " + packageName + " by UID " + Binder.getCallingUid());
        
        if (packageName == null || destPath == null) throw new IllegalArgumentException("Null args");
        if (packageName.contains("..") || destPath.contains("..")) throw new IllegalArgumentException("Path traversal");

        int userId = android.os.UserHandle.getCallingUserId();
        // Protocol: userId\0packageName\0destPath
        String payload = userId + "\0" + packageName + "\0" + destPath;
        sendNativeCommand(CMD_BACKUP, payload);
    }

    @Override
    public void restoreAppData(String packageName, String srcPath) throws RemoteException {
        enforceAuth();
        Slog.i(TAG, "AUDIT: Restore requested for " + packageName + " by UID " + Binder.getCallingUid());
        
        if (packageName == null || srcPath == null) throw new IllegalArgumentException("Null args");
        if (packageName.contains("..") || srcPath.contains("..")) throw new IllegalArgumentException("Path traversal");

        try {
            // Stop the app before restoring data to prevent corruption
            android.app.ActivityManager.getService().forceStopPackage(packageName, android.os.UserHandle.getCallingUserId());
        } catch (Exception e) {
            Slog.w(TAG, "Failed to stop package " + packageName, e);
            // We proceed, as the native side might still succeed, but it's risky.
        }
        
        int userId = android.os.UserHandle.getCallingUserId();
        // Protocol: userId\0packageName\0srcPath
        String payload = userId + "\0" + packageName + "\0" + srcPath;
        sendNativeCommand(CMD_RESTORE, payload);
    }

    @Override
    public void requestFactoryReset(boolean wipeEsim) {
        enforceAuth();
        Slog.w(TAG, "AUDIT: Factory Reset requested by UID " + Binder.getCallingUid());
        sendNativeCommand(CMD_RESET, wipeEsim ? "1" : "0");
    }

    @Override
    public void setProvisioningFlags(Map flags) {
        enforceAuth();
        Slog.i(TAG, "Setting provisioning flags");
        
        if (flags == null || flags.isEmpty()) return;

        // Serialize map to simple "key=value\n" format for the native helper to write
        StringBuilder sb = new StringBuilder();
        for (Object key : flags.keySet()) {
            Object value = flags.get(key);
            if (key instanceof String && value instanceof String) {
                sb.append(key).append("=").append(value).append("\n");
            }
        }
        
        sendNativeCommand(CMD_SET_FLAGS, sb.toString());
    }

    /**
     * Skip Android setup wizard.
     * Useful after factory reset to directly access the device for phone farming.
     * @hide
     */
    public void skipSetupWizard() {
        enforceAuth();
        Slog.i(TAG, "AUDIT: Skip Setup Wizard requested by UID " + Binder.getCallingUid());
        sendNativeCommand(CMD_SKIP_SETUP, "");
    }


    private synchronized void sendNativeCommand(byte opcode, String payload) {
        try (LocalSocket socket = new LocalSocket()) {
            socket.connect(new LocalSocketAddress(SOCKET_PATH, LocalSocketAddress.Namespace.FILESYSTEM));
            
            DataOutputStream out = new DataOutputStream(socket.getOutputStream());
            DataInputStream in = new DataInputStream(socket.getInputStream());

            byte[] payloadBytes = payload.getBytes(StandardCharsets.UTF_8);
            out.writeByte(opcode);
            out.writeInt(payloadBytes.length);
            out.write(payloadBytes);
            out.flush();

            // Wait for simple ACK: 0 = OK, >0 = Error Code, -1 = Error
            int result = in.readInt();
            if (result != 0) {
                throw new IllegalStateException("Native helper failed with error code: " + result);
            }
        } catch (IOException e) {
            Slog.e(TAG, "Failed to communicate with satord", e);
            throw new RuntimeException("Service unavailable", e);
        }
    }
}
