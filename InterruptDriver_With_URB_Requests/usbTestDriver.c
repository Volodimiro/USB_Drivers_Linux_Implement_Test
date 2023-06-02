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
#define ENDPOINT_OUT_ADDRESS 0x01
#define ENDPOINT_IN_ADDRESS 0x81
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





struct usb_device_id usb_hid_devece_id[] = {
    {USB_DEVICE(VENDOR_ID, PRODUCT_ID)},
    {},
};

MODULE_DEVICE_TABLE(usb, usb_hid_devece_id);

static struct usb_driver usb_hid_driver;
struct usb_class_driver usb_cd;             
/*struct usb_host_interface *iface_desc        = NULL;
struct usb_endpoint_descriptor *endpoint     = NULL;
struct urb *urbInterrupt                     = NULL;
struct urb *interrupt_out_urb                = NULL;
char *commandBuffer                          = NULL; //Buffer for send of commands 
char *bufferRead                             = NULL; //Buffer for receive of commands
struct usb_device *usb_dev                   = NULL;  
void *context;
*/

//#############################################################################  OUR DRIVER DESCRIPTION STRUCTURE  #######################################################
typedef struct 
{
    struct usb_device *usb_dev;           
    struct usb_interface *intf;
    char *commandBuffer; //= NULL; //Buffer for send of commands 
    char *bufferRead; //= NULL;    //Buffer for receive of commands
    struct usb_endpoint_descriptor *interrupt_in_endpoint;
    struct usb_endpoint_descriptor *interrupt_out_endpoint; 
    int interrupt_endpoint_IN_address;
    int interrupt_endpoint_OUT_address;
    struct urb *interrupt_urb;
    int interrupt_Interval_In;
    int interrupt_Interval_Out;
    int usb_interrupt_in_size;
    int usb_interrupt_out_size; 
    struct usb_host_interface *iface_desc; 
    int open_count;
}usb_iterrupt_driver_t;




//#############################################################################  CLOSE  #######################################################
static int usbtest_open(struct inode *i, struct file *f)
{
    usb_iterrupt_driver_t *dev;
    struct usb_interface *intf;

    intf = usb_find_interface(&usb_hid_driver, iminor(i));
    dev =  usb_get_intfdata(intf);
    f->private_data = dev;

    return 0;
}
//#############################################################################  OPEN  #######################################################
static int usbtest_close(struct inode *i, struct file *f)
{
    return 0;
}

//#############################################################################  FREE RESURSE  #######################################################

void freeRssurse(void)
{
   /* if(interrupt_out_urb)
    {
       usb_free_urb(interrupt_out_urb);
       printk( KERN_INFO "Call free resource URB_Interrupt \n");
    }
    if(commandBuffer)
    {
       kfree(commandBuffer);
       printk( KERN_INFO "Call free resource commandBuffer \n");
    }
    if(bufferRead)
    {
       kfree(bufferRead);
       printk( KERN_INFO "Call free resource commandBuffer \n");
    }  */
    printk( KERN_ALERT "Call free resource bufferRead \n");            
}

static void urb_read_int_callback(struct urb *urb)
{
   usb_iterrupt_driver_t *dev = NULL;
   dev = (usb_iterrupt_driver_t*)urb->context;
  
   kfree(dev->bufferRead);
   dev->bufferRead = NULL;
   printk(KERN_INFO "urb_write_int_callback" );
}

static void urb_write_int_callback(struct urb *urb)
{
   usb_iterrupt_driver_t *dev = NULL;
   dev = (usb_iterrupt_driver_t*)urb->context;
   kfree(dev->commandBuffer);
   dev->commandBuffer = NULL;
   printk(KERN_INFO "urb_write_int_callback" );
}

//#############################################################################  READ  #######################################################
 
static ssize_t usbtest_read(struct file *f, char __user *buf, size_t count, loff_t *off)
{
    printk("Reading from Driver\n");
   
    int status, size, toCopy, sizeNotCopiedData;
    struct urb *urb = NULL;
    usb_iterrupt_driver_t *dev = NULL;
    
    dev = (usb_iterrupt_driver_t*)f->private_data;
    
    dev->bufferRead = kzalloc(MAX_PKT_SIZE, GFP_KERNEL);
    if(dev->bufferRead == NULL)
    {
      printk(KERN_ERR "Cannot to allocate memory for reciceive buffer!" );
      return -1;
    }

    urb = usb_alloc_urb(0, GFP_KERNEL);
    if(!urb)
    {
       printk(KERN_ERR "< Reading function > Cennot to allocate urb" );
       return -ENOMEM;
    }
    
    
    memset(dev->bufferRead, 0, dev->usb_interrupt_out_size + 1);
               
    usb_fill_int_urb(urb, dev->usb_dev, usb_rcvintpipe(dev->usb_dev, dev->interrupt_endpoint_IN_address), dev->bufferRead,\
    dev->usb_interrupt_out_size, urb_read_int_callback, dev, dev->interrupt_Interval_Out);
   
    status = usb_submit_urb(urb, GFP_KERNEL);
   // status = usb_interrupt_msg(usb_dev, usb_rcvintpipe(usb_dev, 0x81), bufferRead, MAX_PKT_SIZE, &size, HZ*100);
    
    printk(KERN_ERR "status = usb_interrupt_msg(): %d\n", status);
    
    usb_free_urb(urb);
   

    if(status)
    {
        printk(KERN_ERR "Not reciave messages by usb_interrupt_msg");
        return status;
    }
    
   // toCopy = min(count, size);
     
    for(int i=0; i < MAX_PKT_SIZE; i++ )
    {
        if (dev->bufferRead[i] != 0)
        {
            printk(KERN_INFO "bufferRead: 0x%x\n", dev->bufferRead[i]);
        }
    }
    
   sizeNotCopiedData = copy_to_user(buf, dev->bufferRead, size);
  
   printk(KERN_INFO "sizeNotCopiedData: %d", sizeNotCopiedData);
   
   if(sizeNotCopiedData < 0)
    {
        printk(KERN_ERR "copy_to_user ERROR:");
        return -EFAULT;
    }
    if(sizeNotCopiedData > 0)
    {
        printk(KERN_ERR "copy_to_user Not copyed %d bytes:", sizeNotCopiedData);
        toCopy = dev->usb_interrupt_out_size - sizeNotCopiedData;
    } 

    printk(KERN_INFO "copy_to_user success:");
   
   return toCopy;
}
 
//#############################################################################  WRITE  #######################################################
static ssize_t usbtest_write(struct file *f, const char __user *buf, size_t count, loff_t *ppos)
{
    printk("< Writing function > Writing in Driver\n");
    
    int status;
    int _count = MIN(count, MAX_PKT_SIZE);
    usb_iterrupt_driver_t *dev = NULL;
    struct urb *urb = NULL;

    dev = (usb_iterrupt_driver_t*)f->private_data;
       
    urb = usb_alloc_urb(0, GFP_KERNEL);
    if(!urb)
    {
       printk(KERN_ERR "< Writing function > Cennot to allocate urb" );
       return -ENOMEM;
    }
    
    dev->commandBuffer = kzalloc(MAX_PKT_SIZE, GFP_KERNEL);
    if(dev->commandBuffer == NULL)
    {
       printk(KERN_ERR "< Writing function > Cannot allocate memory for commands buffer!" );
       dev->commandBuffer = NULL;
       return -2;
    }
  
    memset(dev->commandBuffer, 0, _count); 
    
    if (copy_from_user(dev->commandBuffer, buf, _count))
    {
        printk(KERN_ERR "< Writing function > Cannot  copy memory for commands buffer!" );
        return -EFAULT;
    }
    
    printk("< Writing function > Writing in Driver command %c \n", *dev->commandBuffer);
    printk("< Writing function > Writing in Driver _count %d \n", _count);
  
    printk("< Writing function > sb_sndintpipe(dev->usb_dev, 0x01) %d \n", usb_sndintpipe(dev->usb_dev, 0x01));
  
    usb_fill_int_urb(urb, dev->usb_dev, usb_sndintpipe(dev->usb_dev, dev->interrupt_endpoint_IN_address), dev->commandBuffer, _count, \
    urb_write_int_callback, dev, dev->interrupt_Interval_In);
   
    printk(KERN_ERR "< Writing function >  urb->number_of_packets %d", urb->number_of_packets ); 
    urb->number_of_packets = 1;
 
    printk("< Writing function > 1\n", _count);
  
    status = usb_submit_urb(urb, GFP_KERNEL);

  
    printk("< Writing function > 2\n", _count);
    usb_free_urb(urb);
  
    if(status)
    {
       printk(KERN_ERR "< Writing function > Not send messages by usb_interrupt_msg! Stasus: %d", status);
       
       return status;
    }
    return count;
}


//#############################################################################  FILE STRUCTURE  #######################################################
static struct file_operations fops =
{
    .owner      = THIS_MODULE,
    .open       = usbtest_open,
    .release    = usbtest_close,
    .read       = usbtest_read,
    .write      = usbtest_write
};

//#############################################################################  PROBE  ####################################################################

static int my_usb_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
    printk("my_usb_devdrv - Probe Function\n");
    int _ret;
    usb_iterrupt_driver_t *dev;
    struct usb_endpoint_descriptor *endpoint;
    int retval = -ENOMEM; 
   
    usb_cd.name = "usbTest%d";
    usb_cd.fops = &fops;
    usb_cd.minor_base = MYUSB_MINOR_BASE;

   
   
    //iface_desc = intf->cur_altsetting;
   // usb_dev = interface_to_usbdev(intf);
//############################################## Initialization our structure dev
 
  dev = kzalloc(sizeof(usb_iterrupt_driver_t), GFP_KERNEL);
    
  
 
  //dev->interrupt_endpioint_IN_address = ENDPOINT_IN_ADDRESS;
  //dev->interrupt_endpoint_OUT_address = ENDPOINT_OUT_ADDRESS;
  dev->intf = intf;
  dev->usb_dev = interface_to_usbdev(intf);
  dev->iface_desc = intf->cur_altsetting;
  dev->interrupt_Interval_In = 100;
  dev->interrupt_Interval_Out = 100;
  dev->usb_interrupt_out_size = 65;
  dev->usb_interrupt_in_size = 65;

  DUMP_INTERFACE_DESCRIPTORS(dev->iface_desc->desc);
    
 
  for( int i = 0; i < dev->iface_desc->desc.bNumEndpoints; ++i )
  {
    DUMP_USB_ENDPOINT_DESCRIPTOR(dev->iface_desc->endpoint[i].desc);
    endpoint = &dev->iface_desc->endpoint[i].desc;
    if(!(dev->interrupt_endpoint_IN_address && usb_endpoint_is_int_in(endpoint)))
    {
      dev->usb_interrupt_in_size  = endpoint->wMaxPacketSize;
      dev->interrupt_endpoint_IN_address = endpoint->bEndpointAddress;
      dev->bufferRead    = kzalloc(MAX_PKT_SIZE, GFP_KERNEL);
      printk(KERN_INFO "USB_DIR_IN: 0x%x\n", endpoint->bEndpointAddress);
      printk(KERN_INFO "USB_DIR_IN: endpoint->wMaxPacketSize %d\n", endpoint->wMaxPacketSize);

    }
    if(!(dev->interrupt_endpoint_OUT_address && usb_endpoint_is_int_out(endpoint)))
    {
       dev->usb_interrupt_out_size = endpoint->wMaxPacketSize;
       dev->interrupt_endpoint_OUT_address =  endpoint->bEndpointAddress;
       dev->commandBuffer = kzalloc(MAX_PKT_SIZE, GFP_KERNEL);
       printk(KERN_INFO "USB_DIR_OUT 0x%x\n", endpoint->bEndpointAddress);
       printk(KERN_INFO "USB_DIR_OUT: endpoint->wMaxPacketSize %d\n", endpoint->wMaxPacketSize);
    }
  }
  
  if(!(dev->interrupt_endpoint_IN_address && dev->interrupt_endpoint_OUT_address))
  {
    printk(KERN_ERR "Cannot get endpoint address.");
    return retval; 
  }
  
  printk(KERN_INFO "Device's structure has initialized");
   
  usb_set_intfdata(intf, dev);
  
   _ret = usb_register_dev(intf, &usb_cd);

  if (_ret)
  {
     // Something prevented us from registering this driver 
     printk(KERN_ERR "Not able to get a minor for this device.");
     usb_set_intfdata(intf, NULL);
     return _ret;
  }
  
  printk(KERN_INFO "Minor obtained: %d\n", intf->minor);
  printk(KERN_INFO"The usb device now attached to /dev\n");
  return 0;
}

//#############################################################################  DISCONNECT  #######################################################
static void my_usb_disconnect(struct usb_interface *intf)
{
    usb_iterrupt_driver_t *dev;
    dev = usb_get_intfdata(intf);
    usb_set_intfdata(intf, NULL);

    printk("my_usb_devdrv - Disconnect Function\n");
    usb_deregister_dev(intf, &usb_cd);
    dev->intf = NULL;
} 


//#############################################################################  HID_DRIVER_STRUCTURE  #######################################################
static struct usb_driver usb_hid_driver = {
    //.owner  = THIS_MODULE,
    .name = "hid_stm32_usb",
    .id_table = usb_hid_devece_id,
    .probe = my_usb_probe,
    .disconnect = my_usb_disconnect,
}; 

//#############################################################################  INIT  #######################################################
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

//#############################################################################  EXIT  #######################################################
static void  __exit exitDriver(void)
{
   printk(KERN_ALERT "Goodbye, cruel world and deregister function\n");
   //kfree(commandBuffer);
  // kfree(bufferRead);
  // commandBuffer = NULL;
   //bufferRead = NULL;
   usb_deregister(&usb_hid_driver);
}

//#############################################################################  REGISTRATION FUNCTIONS  #######################################################
module_init(my_usb_init);
module_exit(exitDriver);