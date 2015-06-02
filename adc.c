#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
// kmalloc
#include <linux/slab.h>
// cdev
#include <linux/cdev.h>
#include <linux/fs.h>
//class
#include <linux/device.h>
//resource 
#include <linux/ioport.h>
#include <linux/io.h>
//自旋锁
#include <linux/spinlock.h>
//copy_to_user/copy_from_user
#include <linux/uaccess.h>
//ioctl
#include <linux/ioctl.h>
#include "adc.h"
// clk
#include <linux/clk.h>
// interrupt
#include <linux/interrupt.h>


// hardware : 定义寄存器的偏移 
#define ADCCON 0x0
#define ADCDAT 0xC
#define ADCCLRINT 0x18
#define ADCMUX 0x1c
// char : 定义一个结构体描述adc
struct adc_cdev{
    struct cdev cdev;
    dev_t devno;
    void __iomem *regs; // resource ：映射后虚拟地址
    struct resource *res;// resource : 申请的物理地址
    struct clk *clk; //hardware : 时钟
    int irqno;
};
// mknod : adc全局类
struct class *adc_class;

/*// char : 定义存放主设备号参数的变量*/
/*dev_t adc_major = 250;*/
/*module_param(adc_major, int, 0400);*/
/*MODULE_PARM_DESC(adc_major, "adc device major");*/

// hardware : adc_dev_init
void adc_dev_init(struct adc_cdev *adc, int minor)
{
    unsigned long ADC_CON;    
    unsigned long ADC_DAT;
    unsigned long ADC_MUX;
    unsigned long ADC_CLRINT;

    ADC_CON = readl(adc->regs + ADCCON);    
    ADC_CON = (1 << 16) | (1 << 14) | (65 << 6);
    writel(ADC_CON, adc->regs + ADCCON);

    ADC_DAT = readl(adc->regs + ADCDAT);
    ADC_DAT = 0;
    writel(ADC_DAT, adc->regs + ADCDAT);

    ADC_MUX = readl(adc->regs + ADCMUX);
    ADC_MUX &= ~(0xf);
    writel(ADC_MUX, adc->regs + ADCMUX);

    ADC_CLRINT = readl(adc->regs + ADC_CLRINT);
    ADC_CLRINT = 0; 
    writel(ADC_CLRINT, adc->regs + ADCCLRINT);

    
}

// char : adc_open
int adc_open(struct inode *inode, struct file *filp)
{
    struct adc_cdev *adc = container_of(inode->i_cdev, struct adc_cdev, cdev);
    filp->private_data = adc;
    printk("%s\n", __func__);
    return 0;
}
// char : adc_release
int adc_release(struct inode *inode, struct file *filp)
{
    printk("%s\n", __func__);
    return 0;
}
// char : adc_read
ssize_t adc_read(struct file *filp, char __user *buf, size_t count, loff_t *f_ops)
{
    ssize_t ret;
    printk("%s\n", __func__);
    ret = copy_to_user(buf, "Hello world\n", 13);
    if (0 != ret){
        ret = -EFAULT;
    }
    return ret;
}
// char : adc_ioctl
long adc_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    printk("%s\n", __func__);

    switch (cmd){
    case IOCTL_SET_RESOLUTION:
        break;
    default:
        ret = -EINVAL;
        break;
    }
    return ret;
}
// char : 供系统调用的函数
const struct file_operations fops = {
    .open = adc_open,
    .release = adc_release,
    .read = adc_read,
    .unlocked_ioctl = adc_unlocked_ioctl,
};
// hardware : 中断处理程序
irqreturn_t adc_irq_handler(int irq, void *dev_id)
{
    return 0;
}


// bus : adc_probe 
int adc_probe(struct platform_device *pdev) 
{
    int ret = 0; 
    struct adc_cdev *adc;
    struct device *device;
    struct resource *res;

    printk("%s\n", __func__);
    // char : 分配内存
    adc = (struct adc_cdev *)kmalloc(sizeof(struct adc_cdev), GFP_KERNEL);
    if (NULL == adc){
        ret = -ENOMEM;
        goto exit;
    }

    // char : 将驱动数据放入平台设备
    platform_set_drvdata(pdev, adc);

    // resource : 获取寄存器资源
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (res == NULL){
        ret = -ENOENT;
        printk("err_platform_get_resource\n");
        goto err_platform_get_resource;
    }
    printk("res = <%08x %d>\n", res->start, resource_size(res));

    adc->res = res;
    // resource : 申请资源
    res = request_mem_region(res->start, resource_size(res), "adc");
    if (NULL == res){
        ret = -EBUSY;
        printk("err_request_mem_region\n");
        goto err_request_mem_region;
    }
    // resource : 虚拟内存映射
    adc->regs = ioremap(res->start, resource_size(res));
    if (NULL == adc->regs){
        ret = -EINVAL;
        goto err_ioremap;
    }
    printk("adc->regs = <%p>\n", adc->regs);

    // hardware : adc_dev_init
    adc_dev_init(adc, 0);

    // char : 初始化(描述)字符设备(属性与操作)
    cdev_init(&adc->cdev, &fops);
    adc->cdev.owner = THIS_MODULE;

    // char : 分配编号
    ret = alloc_chrdev_region(&adc->devno, 0, 1,"adc");
    if (ret < 0){
        goto err_alloc_chrdev_region;
    }

    // char : 添加字符设备到内核
    ret = cdev_add(&adc->cdev, adc->devno, 1);
    if (ret < 0){
        goto err_cdev_add;
    }

    // mknod : 建立设备并注册到sysfs中
    device = device_create(adc_class, NULL, adc->devno, NULL, "adc%d", pdev->id);
    if (IS_ERR(device)){
        ret = PTR_ERR(device);
        goto err_device_create;
    }

    // hardware : 获取时钟
    adc->clk = clk_get(&pdev->dev, "adc");
    if (IS_ERR(adc->clk)){
        ret = PTR_ERR(adc->clk);
        goto err_clk_get;
    }

    // hardware : 使能时钟
    clk_enable(adc->clk);
    printk("clk is OK\n");

    // hardware : 获得中断号
    ret = platform_get_irq(pdev, 0);
    if (ret < 0){
        printk("err_platform_get_irq\n");
        goto err_platform_get_irq;
    }
    printk("%s : irqno = %d\n", __func__, ret);
    adc->irqno = ret;
    // hardware : 注册中断程序
    ret = request_irq(adc->irqno, adc_irq_handler, IRQF_TRIGGER_FALLING, "adc", pdev);
    if (ret < 0){
        printk("err_request_irq\n");
        goto err_request_irq;
    }
    // 重要
    goto exit;

err_request_irq:
    free_irq(adc->irqno, pdev);
err_platform_get_irq:
err_clk_get:
err_device_create:
    cdev_del(&adc->cdev);

err_cdev_add:
    unregister_chrdev_region(adc->cdev.dev, 1);

err_alloc_chrdev_region:
    iounmap(adc->regs);
err_ioremap:
    release_mem_region(res->start, resource_size(res));

err_request_mem_region:
err_platform_get_resource:
    kfree(adc);

exit:
    return ret;
}
// bus : adc_remove 
int adc_remove(struct platform_device *pdev)
{
    int ret = 0;

    struct adc_cdev *adc;
    struct resource *res;
    printk("%s\n", __func__);

    // char : 从平台设备获得驱动数据
    adc = platform_get_drvdata(pdev);

    // hardware : 禁止时钟
    clk_disable(adc->clk);

    // hardware : 释放时钟
    clk_put(adc->clk);

    // mknod : 删除设备文件
    device_destroy(adc_class, adc->cdev.dev);

    // char : 删除一个字符设备
    cdev_del(&adc->cdev);

    // char : 回收设备编号
    unregister_chrdev_region(adc->devno, 1);

    // resource : 取消映射
    iounmap(adc->regs);

    // resource : 释放资源
    res = adc->res;
    release_mem_region(res->start, resource_size(res));

    // char : 释放内存
    kfree(adc);

    return ret;
}

// bus : 设备列表
struct platform_device_id adc_ids[] = {
    [0] = {
        .name = "adc",
    },

    {/*NULL*/}
};
MODULE_DEVICE_TABLE(platform, adc_ids);

// bus : 平台设备驱动
struct platform_driver adc_driver = {
    .probe = adc_probe,
    .remove = adc_remove,

    // 与BSP代码中的设备匹配
    .id_table = adc_ids,
    .driver = {
        .name = "adc",
        .owner = THIS_MODULE,
    }
};

//module : insmod /lib/modules/... 
int __init adc_init(void)
{
    int ret = 0;
    printk("%s\n", __func__);

    // mknod : 创建adc_class
    adc_class = class_create(THIS_MODULE, "adc");
    if (IS_ERR(adc_class)){
        ret = PTR_ERR(adc_class);
        goto exit;
    }
    // bus : 添加平台驱动->adc_probe
    ret = platform_driver_register(&adc_driver);
    if (ret < 0){
        class_destroy(adc_class);
    }


exit:
    return ret;
}

//module : rmmod adc
void __exit adc_exit(void)
{
    printk("%s\n", __func__);

    // bus : 移除平台驱动->adc_remove
    platform_driver_unregister(&adc_driver);

    // mknod : 销毁adc_class
    class_destroy(adc_class);
}

module_init(adc_init);
module_exit(adc_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("farsight");
MODULE_DESCRIPTION("a adc driver.");
MODULE_ALIAS("adc driver");
