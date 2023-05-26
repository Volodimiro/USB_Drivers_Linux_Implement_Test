#include <linux/usb.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/kernel.h>



MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("TEST");
MODULE_DESCRIPTION("TEST UDB DRIVER");

#define VENDOR_ID 0x0483
#define PRODUCT_ID 0x5762
#define MYUSB_MINOR_BASE    250
#define MAX_PKT_SIZE 65
#define MIN(a,b) (((a) <= (b)) ? (a) : (b))

#define DUMP_INTERFACE_DESCRIPTORS(i) \
{\
    printk("USB INTERFACE DESCRIPTOR:\n"); \
    printk("-----------------------------------\n"); \
    printk("dLength: 0x%\n", i.bLength); \
    printk("bDescriptorType: 0x%x\n", i.bDescriptorType); \
    printk("bInterfaceNumber: 0x%x\n", i.bInterfaceNumber); \
    printk("bAlternateSetting: 0x%x\n", i.bAlternateSetting); \
    printk("bNumEndpoints: 0x%x\n", i.bNumEndpoints); \
    printk("bInterfaceClass: 0x%x\n", i.bInterfaceClass); \
    printk("bInterfaceSubClass: 0x%x\n", i.bInterfaceSubClass); \
    printk("bInterfaceProtocol: 0x%x\n", i.bInterfaceProtocol); \
    printk("iInterface: 0x%x\n", i.iInterface); \
    printk("\n"); \
}

#define DUMP_USB_ENDPOINT_DESCRIPTOR( e ) \
{\
        printk("USB_ENDPOINT_DESCRIPTOR:\n"); \
        printk("------------------------\n"); \
        printk("bLength: 0x%x\n", e.bLength); \
        printk("bDescriptorType: 0x%x\n", e.bDescriptorType); \
        printk("bEndPointAddress: 0x%x\n", e.bEndpointAddress); \
        printk("bmAttributes: 0x%x\n", e.bmAttributes); \
        printk("wMaxPacketSize: 0x%x\n", e.wMaxPacketSize); \
        printk("bInterval: 0x%x\n", e.bInterval); \
        printk("\n"); \
}


char *commandBuffer = NULL; //Buffer for send of commands 
char *bufferRead = NULL; //Buffer for receive of commands

struct usb_device_id usb_hid_devece_id[] = {
    {USB_DEVICE(VENDOR_ID, PRODUCT_ID)},
    {},
};

MODULE_DEVICE_TABLE(usb, usb_hid_devece_id);

static struct usb_device *usb_dev;
struct usb_class_driver usb_cd;
struct usb_host_interface *iface_desc;
struct usb_endpoint_descriptor *endpoint;


static int usbtest_open(struct inode *i, struct file *f)
{
    return 0;
}

static int usbtest_close(struct inode *i, struct file *f)
{
    return 0;
}

 
static ssize_t usbtest_read(struct file *f, char __user *buf, size_t count, loff_t *off)
{
    printk("Reading from Driver\n");
   
    int status, size, toCopy;
    unsigned long sizeNotCopiedData;
    

   
   
    if(bufferRead == NULL)
    {
      bufferRead = kzalloc(MAX_PKT_SIZE, GFP_KERNEL);
      if(bufferRead == NULL)
      {
        printk(KERN_ERR "Cannot to allocate memory for reciceive buffer!" );
        return -1;
      }
    }
    
    memset(bufferRead, 0, MAX_PKT_SIZE);
               
    status = usb_interrupt_msg(usb_dev, usb_rcvintpipe(usb_dev, 0x81), bufferRead, MAX_PKT_SIZE, &size, HZ*100);
    
    printk(KERN_ERR "status: %d\n", status);
        
    if(status)
    {
        printk(KERN_ERR "Not reciave messages by usb_interrupt_msg");
        return status;
    }
    
    toCopy = min(count, size);
     
    for(int i=0; i < MAX_PKT_SIZE; i++ )
    {
        if (bufferRead[i] != 0)
        {
            printk(KERN_INFO "bufferRead: 0x%x\n", bufferRead[i]);
        }
    }
  
  
   sizeNotCopiedData = copy_to_user(buf, bufferRead, size);
  
   printk(KERN_INFO "sizeNotCopiedData: %d", sizeNotCopiedData);
   
   if(sizeNotCopiedData < 0)
    {
        printk(KERN_ERR "copy_to_user ERROR:");
        return -EFAULT;
    }
    if(sizeNotCopiedData > 0)
    {
        printk(KERN_ERR "copy_to_user Not copyed %d bytes:", sizeNotCopiedData);
        toCopy = size - sizeNotCopiedData;
    } 

    printk(KERN_INFO "copy_to_user success:");
    return toCopy;
}
 

static ssize_t usbtest_write(struct file *f, const char __user *buf, size_t count, loff_t *ppos)
{
    printk("Writing in Driver\n");
    
    int status;
    int _count = MIN(count, MAX_PKT_SIZE);
   
    if(commandBuffer == NULL)
    {
        commandBuffer = kzalloc(sizeof(buf), GFP_KERNEL);
       if(commandBuffer == NULL)
       {
          printk(KERN_ERR "Cannot to allocate memory for commands buffer!" );
          return -2;
       }
    }
   
    memset(commandBuffer, 0, MIN(count, MAX_PKT_SIZE)); 
    
    if (copy_from_user(commandBuffer, buf, MIN(count, MAX_PKT_SIZE)))
    {
        return -EFAULT;
    }
    
    printk("Writing in Driver command %c \n", *commandBuffer);
    printk("Writing in Driver _count %d \n", _count);
    status = usb_interrupt_msg(usb_dev, usb_sndintpipe(usb_dev, 0x01), commandBuffer,  MIN(count, MAX_PKT_SIZE), &_count, HZ*100);
    if(status)
    {
        printk(KERN_ERR "Not send messages by usb_interrupt_msg");
        return status;
    }
    return count;
}

static struct file_operations fops =
{
    .owner      = THIS_MODULE,
    .open       = usbtest_open,
    .release    = usbtest_close,
    .read       = usbtest_read,
    .write      = usbtest_write
};



static int my_usb_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
    printk("my_usb_devdrv - Probe Function\n");
    int _ret;
   
    usb_cd.name = "usbTest%d";
    usb_cd.fops = &fops;
    usb_cd.minor_base = MYUSB_MINOR_BASE;

    _ret = usb_register_dev(intf, &usb_cd);

    if (_ret)
    {
        // Something prevented us from registering this driver 
        printk(KERN_ERR "Not able to get a minor for this device.");
        return _ret;
    }
    else
    {
        printk(KERN_INFO "Minor obtained: %d\n", intf->minor);
    }
   
    iface_desc = intf->cur_altsetting;
    usb_dev = interface_to_usbdev(intf);
    
    DUMP_INTERFACE_DESCRIPTORS(iface_desc->desc);
    
    
    for( int i = 0; i < iface_desc->desc.bNumEndpoints; ++i )
    {
        endpoint = &iface_desc->endpoint[i].desc;
        DUMP_USB_ENDPOINT_DESCRIPTOR(iface_desc->endpoint[i].desc);
        if(endpoint->bEndpointAddress & USB_DIR_IN)
        {
          printk(KERN_INFO "USB_DIR_IN: 0x%x\n", endpoint->bEndpointAddress);
        }
        if(endpoint->bEndpointAddress & USB_DIR_OUT)
        {
          printk(KERN_INFO "USB_DIR_OUT 0x%x\n", endpoint->bEndpointAddress);
        }
    }
 
    return 0;
}

static void my_usb_disconnect(struct usb_interface *init)
{
    printk("my_usb_devdrv - Disconnect Function\n");
    usb_deregister_dev(init, &usb_cd);
}


static struct usb_driver usb_hid_driver = {
    //.owner  = THIS_MODULE,
    .name = "hid_stm32_usb",
    .id_table = usb_hid_devece_id,
    .probe = my_usb_probe,
    .disconnect = my_usb_disconnect,
};



static int __init my_usb_init(void)
{
    printk(KERN_ALERT "Hello, world and register function\n");
    int result;
    result = usb_register(&usb_hid_driver);
    if(result)
    {
      printk("my_usb_devdrv - Error during register!\n");
      return -result;
    }
   return 0;
}

static void  __exit exitDriver(void)
{
   printk(KERN_ALERT "Goodbye, cruel world and deregister function\n");
   kfree(commandBuffer);
   kfree(bufferRead);
   commandBuffer = NULL;
   bufferRead = NULL;
   usb_deregister(&usb_hid_driver);
}

module_init(my_usb_init);
module_exit(exitDriver);