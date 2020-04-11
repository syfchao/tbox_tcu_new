
BIN_DIR=$(cd `dirname $0`; pwd)

echo ${BIN_DIR}

TARGET_IP="192.168.1.199"

BIN=tbox

scp ${BIN_DIR}/${BIN} root@$TARGET_IP:/home/root/

ssh root@$TARGET_IP "adb -s 0123456789ABCDEF shell killall -9 monitorTbox"
ssh root@$TARGET_IP "adb -s 0123456789ABCDEF shell killall -9 tbox"

ssh root@$TARGET_IP "adb -s 0123456789ABCDEF push ${BIN} /data/"
ssh root@$TARGET_IP "adb -s 0123456789ABCDEF shell sync"
ssh root@$TARGET_IP "adb -s 0123456789ABCDEF shell chmod +x /data/tbox"
ssh root@$TARGET_IP "adb -s 0123456789ABCDEF shell sync"
