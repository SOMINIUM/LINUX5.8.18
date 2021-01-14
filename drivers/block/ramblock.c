#include <linux/major.h>
#include <linux/vmalloc.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/blkdev.h>
#include <linux/hdreg.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("DRAAPHO");

#define DEVICE_NAME "RAMDISK"
#define RAMBLOCK_SIZE (1024*1024)

static int major;
static struct gendisk *ramblock_disk;
static request_queue_t *ramblock_queue;
static DEFINE_SPINLOCK(ramblock_lock);
static unsigned char *ramblock_buf;

// 分区需要知道"硬盘"的几何结构(geometry), 这里虚拟一下即可.
static int ramblock_getgeo(struct block_device *bdev, struct hd_geometry *geo)
{
    geo->heads     = 2;                                     // 磁头数=盘面数*2
    geo->cylinders = 32;                                    // 柱面数
    geo->sectors   = RAMBLOCK_SIZE/2/32/512;                // 扇区数. 利用公式: 存储容量=磁头数x柱面数x扇区数x512
    return 0;
}

static struct block_device_operations ramblock_fops = {
    .owner  = THIS_MODULE,
    .getgeo = ramblock_getgeo,
};

// 实现扇区的读写操作
static void do_ramblock_request(request_queue_t * q)
{
    static int r_cnt = 0;
    static int w_cnt = 0;
    struct request *req;

    while ((req = elv_next_request(q)) != NULL) {           // 取出要处理的数据(连续的扇区数据, 即簇)
        unsigned long offset = req->sector*512;             // 读写的目标地址
        unsigned long len = req->current_nr_sectors*512;    // 长度

        if (rq_data_dir(req) == READ) {                     // 读操作
            printk("do_ramblock_request read %d\n", ++r_cnt);
            memcpy(req->buffer, ramblock_buf+offset, len);  // 直接读 ramblock_buf
        } else {                                            // 写操作
            printk("do_ramblock_request write %d\n", ++w_cnt);
            memcpy(ramblock_buf+offset, req->buffer, len);  // 直接写 ramblock_buf
        }
        end_request(req, 1);                                // 告知操作完成
    }
}

static int ramblock_init(void)
{
    /* 1. 分配一个gendisk结构体 */
    ramblock_disk = alloc_disk(16);                         // 次设备号个数, 也是允许的最大分区个数

    /* 2. 设置 */
    /* 2.1 分配/设置缓冲队列 */
    ramblock_queue = blk_init_queue(do_ramblock_request, &ramblock_lock);
    ramblock_disk->queue = ramblock_queue;

    /* 2.2 设置其他属性: 比如容量 */
    major = register_blkdev(0, DEVICE_NAME);                // cat /proc/devices 查看块设备
    ramblock_disk->major       = major;                     // 主设备号
    ramblock_disk->first_minor = 0;                         // 次设备号起始值
    sprintf(ramblock_disk->disk_name, "ramblock");
    ramblock_disk->fops        = &ramblock_fops;
    set_capacity(ramblock_disk, RAMBLOCK_SIZE / 512);       // 设置扇区的数量, 不是字节数

    /* 3. 硬件初始化操作 */
    ramblock_buf = kzalloc(RAMBLOCK_SIZE, GFP_KERNEL);

    /* 4. 注册 */
    add_disk(ramblock_disk);
    return 0;
}

static void ramblock_exit(void)
{
    del_gendisk(ramblock_disk);                     // 对应 add_disk
    put_disk(ramblock_disk);                        // 对应 blk_init_queue
    blk_cleanup_queue(ramblock_queue);              // 对应 blk_init_queue
    unregister_blkdev(major, DEVICE_NAME);          // 对应 register_blkdev
    kfree(ramblock_buf);                            // 安全起见, 最后释放buf
}

module_init(ramblock_init);
module_exit(ramblock_exit);
