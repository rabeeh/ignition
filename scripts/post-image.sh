#!/bin/sh
# Start installer post-build
set -x
BOARD_DIR="$(dirname $0)"

echo "BOARD_DIR = $BOARD_DIR"
echo "TARGET_DIR = $TARGET_DIR"
echo "BINARIES_DIR = $BINARIES_DIR"
\rm -rf ${BINARIES_DIR}/fs
mkdir -p ${BINARIES_DIR}/fs
cp ${BINARIES_DIR}/zImage ${BINARIES_DIR}/*.dtb ${BINARIES_DIR}/fs

# Signature file
touch ${BINARIES_DIR}/fs/ignition.sig

# Local repo bypass
# The following if uncommented creates a file called repo.url that is used
# as a base url for ignition (instead of the tarball 

#echo "http://192.168.15.2/content/ignition-imx6.tar.gz" > ${BINARIES_DIR}/fs/repo.url

genext2fs -m 1 -i 4096 -B 4096 -b 9000 -d ${BINARIES_DIR}/fs/ ${BINARIES_DIR}/ignition.ext2.part
dd if=/dev/zero of=${BINARIES_DIR}/ignition.img bs=512 count=2048
dd if=${BINARIES_DIR}/SPL of=${BINARIES_DIR}/ignition.img bs=1K seek=1 conv=notrunc
dd if=${BINARIES_DIR}/u-boot.img of=${BINARIES_DIR}/ignition.img bs=1K seek=42 conv=notrunc
cat $BINARIES_DIR/ignition.ext2.part >> ${BINARIES_DIR}/ignition.img
cat > /tmp/fdisk.script << EOF
n
p
1
2048

w
EOF
cat /tmp/fdisk.script | fdisk ${BINARIES_DIR}/ignition.img
