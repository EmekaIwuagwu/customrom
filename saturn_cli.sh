# Service Call Script (Client)
#!/system/bin/sh

PKG=$1
DEST="/sdcard/backup_${PKG}.tar.gz"

# OpCode 1 = BACKUP
# s16 = UTF-16 string (standard for AIDL/Binder command line serialization via `service call`)
echo "Requesting backup for $PKG..."
service call device_lifecycle 1 s16 "$PKG" s16 "$DEST"
