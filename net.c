#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

#include "platform.h"

#include "util.h"
#include "net.h"

/* NOTE: if you want to add/delete the entries after net_run(), you need to protect these lists with a mutex. */
static struct net_device *devices;

// net_device型分のメモリを確保する
struct net_device *
    net_device_alloc(void)
{
    struct  net_device *dev;
    dev = memory_alloc(sizeof(*dev));
    if (!dev) {
        errorf("memory_alloc() failed");
        return NULL;
    }
    return dev;
}

/* NOTE: must not be call after net_run() */
int
net_device_register(struct net_device *dev)
{
    static unsigned int index = 0;

    dev->index = index++; // Goだと dev.index = index; index++;
    sprintf(dev->name, sizeof(dev->name), "net%d", dev->index);
    dev->next = devices; // 前のデバイスをnextに入れる
    devices = dev; // 今のデバイスをデバイスリストの先頭に入れる
    infof("registered, dev=%s, type=ox%04x", dev->name, dev->type);
    return 0;
}

static int
net_device_open(struct net_device *dev)
{
    if (NET_DEVICE_IS_UP(dev)) {
        errorf("already oepened, dev=%s", dev->name);
        return -1;
    }
    if (dev->ops->open) { // dev->ops->openがNULLじゃない場合
        if (dev->ops->open(dev) == -1) {
            errorf("failed, dev=%s", dev->name);
            return -1;
        }
    }
    dev->flags |= NET_DEVICE_FLAG_UP;
    infof("dev=%s, state=%s", dev->name, NET_DEVICE_STATE(dev));
    return 0;
}

static int
net_device_close(struct net_device *dev)
{
    if (!NET_DEVICE_IS_UP(dev)) {
        errorf("not opend, dev=%s", dev->name);
        return -1;
    }
    if (dev->ops->close) { // dev->ops->closeがNULLじゃない場合
        if (dev->ops->close(dev) == -1) {
            errorf("failed, dev=%s", dev->name);
            return -1;
        }
    }
    dev->flags &= ~NET_DEVICE_FLAG_UP;
    infof("dev=%s, state=%s", dev->name, NET_DEVICE_STATE(dev));
    return 0;
}

int
net_device_output(struct net_device *dev, uint16_t type, const uint8_t *data, size_t len, const void *dst)
{
    if (!NET_DEVICE_IS_UP(dev)) { // デバイス状態がUPでないと送信できない
        errorf("not opend, dev=%s", dev->name);
        return -1;
    }
    if (len > dev->mtu) { // 送信データのサイズをチェック, MTUより大きいと送信できないのでエラー
        errorf("too long, dev=%s, mtu=%u, len=%zu", dev->name, dev->mtu, len);
        return -1;
    }
    debugf("dev=%s, type0x%04x, len=%zu", dev->name, type, len);
    debugdump(data, len);
    if (dev->ops->transmit(dev, type, data, len, dst) == -1) { // デバイスドライバの出力(送信)関数を呼び出す
        errorf("device transmit failed, dev=%s, len=%zu", dev->name, len);
        return -1;
    }
}

int
net_input_handler(uint16_t type, const uint8_t *data, size_t len, struct net_device *dev)
{
    debugf("dev=%s, type=0x%04x, len=%zu", dev->name, type, len);
    debugdump(data, len);
    return 0;
}

int
net_run(void)
{
    struct net_device *dev;

    debugf("open all devices...");
    for (dev = devices; dev; dev = dev->next) { // 登録済みのデバイスを全て開く
        net_device_open(dev);
    }
    debugf("running...");
    return 0;
}

void
net_shutdown(void)
{
    struct net_device *dev;

    debugf("close all devices...");
    for (dev = devices; dev; dev = dev->next) { // 登録済みのデバイスを全て閉じる
        net_device_close(dev);
    }
    debugf("shutting down");
}

void
net_device_dump(void)
{
}

int
net_init(void)
{
    infof("initialized");
    return 0;
}

//exapmle
// int
// main(void)
// {
//     if (net_init() == -1) {
//         return -1;
//     }

//     /* デバイスの登録 */

//     if (net_run() == -1) {
//         return -1;
//     }

//     /* アプリケーションの処理 */

//     net_shutdown();

//     return 0;
// }
