#!/bin/sh
# Start installer post-build

set -x
BOARD_DIR="$(dirname $0)"

echo "BOARD_DIR = $BOARD_DIR"
echo "TARGET_DIR = $TARGET_DIR"
echo "BINARIES_DIR = $BINARIES_DIR"

mkdir -p ${TARGET_DIR}/etc/systemd/system/multi-user.target.wants
ln -fs /usr/lib/systemd/system/connman.service ${TARGET_DIR}/etc/systemd/system/multi-user.target.wants

# Remove /bin/tar from the root filesystem. We use /usr/bin/tar instead
rm -rf ${TARGET_DIR}/bin/tar

# Some scripts rely on mkfs.vfat
ln -fs /sbin/mkfs.fat ${TARGET_DIR}/bin/mkfs.vfat

cat > ${TARGET_DIR}/etc/systemd/system/multi-user.target.wants/ignition.service << EOF
[Unit]
Description=Ignition installer script
After=connman.service ignition-url.service

[Service]
ExecStart=/usr/bin/ignition -qws -fn null -display linuxfb:mmWidth=500:mmHeight=300

[Install]
WantedBy=multi-user.target
EOF

cat > ${TARGET_DIR}/etc/systemd/system/multi-user.target.wants/ignition-url.service << EOF
[Unit]
Description=Ignition URL resolver

[Service]
ExecStart=/usr/bin/ignition_copy_repo.sh

[Install]
WantedBy=multi-user.target
EOF

cat > ${TARGET_DIR}/usr/bin/ignition_copy_repo.sh << EOF
#!/bin/bash

set +e
# The following might fail; set +e should protect it from bailing out
mount /dev/mmcblk0p1 /mnt -o ro
cp /mnt/repo.url /tmp
umount /mnt
EOF
chmod +x ${TARGET_DIR}/usr/bin/ignition_copy_repo.sh

# Copy over fdisk,cfdisk,sfdisk. Those are binaries from Debian armhf and
# and will be removed once util-linux build under buildroot is fixed
rm -rf ${TARGET_DIR}/sbin/fdisk
rm -rf ${TARGET_DIR}/sbin/cfdisk
cp ../scripts/fdisk ../scripts/cfdisk ${TARGET_DIR}/sbin

# Copy over brcm4329 and brcm4330 firmwares
mkdir -p ${TARGET_DIR}/lib/firmware/brcm
cp ../scripts/brcmfmac* ${TARGET_DIR}/lib/firmware/brcm
