mkfs.vfat -F 32 -C udisk.img 51200
losetup /dev/loop20 udisk.img
mkdir a
mount /dev/loop20 a
cp -r $1 a
umount a
losetup -d /dev/loop20
rmdir a