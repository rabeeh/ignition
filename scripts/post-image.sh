#!/bin/sh
# Start installer post-build
set -x
BOARD_DIR="$(dirname $0)"

# find fdisk (also look in places that may not be on a regular users path)
FDISK=$(PATH=$PATH:/sbin:/usr/sbin:/usr/local/sbin which fdisk)
[ $? != 0 ] && exit 1

\rm -rf ${BINARIES_DIR}/fs
mkdir -p ${BINARIES_DIR}/fs
cp ${BINARIES_DIR}/zImage ${BINARIES_DIR}/*.dtb ${BINARIES_DIR}/fs

# Signature file
touch ${BINARIES_DIR}/fs/ignition.sig

# Repository configuration
# Following is a base configuration of the default repository.
# First line is the actual URL and second one is a text to be shown on the GUI.
# On boot the repo.url file is copied to /tmp
echo "http://www.github.com/SolidRun/ignition-imx6/tarball/master" > ${BINARIES_DIR}/fs/repo.url
echo "SolidRun GitHub - http://www.github.com/SolidRun/ignition-imx6/" >> ${BINARIES_DIR}/fs/repo.url

genext2fs -m 1 -i 4096 -b 36000 -d ${BINARIES_DIR}/fs/ ${BINARIES_DIR}/ignition.ext2.part
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
cat /tmp/fdisk.script | $FDISK ${BINARIES_DIR}/ignition.img
