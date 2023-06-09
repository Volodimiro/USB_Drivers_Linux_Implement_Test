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


//#############################################################################  OUR DRIVER DESCRIPTION STRUCTURE  #######################################################
struct usb_iterrupt_driver_t
{
    struct usb_device *usb_dev;           
    struct usb_interface *intf;
    char *commandBuffer; //= NULL; //Buffer for send of commands 
    char *bufferRead; //= NULL;    //Buffer for receive of commands
    struct usb_endpoint_descriptor *interrupt_in_endpoint;
    struct usb_endpoint_descriptor *interrupt_out_endpoint; 
    int interrupt_endpoint_IN_address;
    int interrupt_endpoint_OUT_address;
    int interrupt_Interval_In;
    int interrupt_Interval_Out;
    int usb_interrupt_in_size;
    struct urb *urb;
    int usb_interrupt_out_size; 
    struct usb_host_interface *iface_desc; 
    int open_count;
};




//#############################################################################  OPEN  #######################################################
static int usbtest_open(struct inode *i, struct file *f)
{
   struct usb_iterrupt_driver_t *dev;
    struct usb_interface *intf;

    intf = usb_find_interface(&usb_hid_driver, iminor(i));
    dev =  usb_get_intfdata(intf);
    f->private_data = dev;

    return 0;
}
//#############################################################################  CLOSE  #######################################################
static int usbtest_close(struct inode *i, struct file *f)
{
    return 0;
}

//#############################################################################  CALLBACK  #######################################################
static void urb_read_int_callback(struct urb *urb)
{
  complete((struct completion *)urb->context);
 
  printk(KERN_INFO "urb_read_int_callback");
}

static void urb_write_int_callback(struct urb *urb)
{
   struct usb_iterrupt_driver_t *dev = NULL;
   dev = (struct usb_iterrupt_driver_t*)urb->context;
   printk(KERN_INFO "urb_write_int_callback" );
}

//#############################################################################  READ  #######################################################
static ssize_t usbtest_read(struct file *f, char __user *buf, size_t count, loff_t *off)
{
    printk("Reading from Driver\n");
   
    int status, size, toCopy, sizeNotCopiedData;
    struct usb_iterrupt_driver_t *dev = NULL;
    struct urb *urb = NULL;
    struct completion dev_config_done;
    void *dev_ctrl_context;

    dev = (struct usb_iterrupt_driver_t*)f->private_data; //Gets our driver structure from file structure which was safed \
                                                           in usbtest_open() metods 
   
   
   
    if(dev->bufferRead == NULL)
    {
      printk(KERN_ERR "Cannot to allocate memory for reciceive buffer!" );
      return -1;
    }
   
   
    urb = usb_alloc_urb(0, GFP_KERNEL);  //Allocating kernel memory for urb structure
     

    if(!urb)
    {
      printk(KERN_ERR "< Reading function > Cennot to allocate urb" );
      return -ENOMEM;
    } 
    
    init_completion(&dev_config_done);
    dev_ctrl_context = (void*)&dev_config_done;

    usb_fill_int_urb(urb, dev->usb_dev, usb_rcvintpipe(dev->usb_dev, 0x81),dev->bufferRead,\
    dev->usb_interrupt_in_size, urb_read_int_callback, dev_ctrl_context, dev->interrupt_Interval_In); //filling urb structure
        
    printk(KERN_ERR "< Reading function > dev->interrupt_endpoint_IN_address 0x%x", dev->interrupt_endpoint_IN_address );
    
    memset(dev->bufferRead, 0, dev->usb_interrupt_out_size + 1); //clearing the buffer

    status = usb_submit_urb(urb, GFP_KERNEL); // Send urb requests to the kernel
    
    wait_for_completion(&dev_config_done); //Wait for comletation is done
    
    usb_free_urb(urb); //free urb

        
    printk(KERN_ERR "status = urb_interrupt_rcv(): %d\n", status);
    
    if(status)
    {
        printk(KERN_ERR "Not reciave messages by usb_interrupt_msg");
       
        return status;
    }
    
    
    sizeNotCopiedData = copy_to_user(buf, dev->bufferRead, MAX_PKT_SIZE); //Copy from the kernel buffer to the user buffer
  
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
     
   return 8;
}
 
//#############################################################################  WRITE  #######################################################
static ssize_t usbtest_write(struct file *f, const char __user *buf, size_t count, loff_t *ppos)
{
    printk("< Writing function > Writing in Driver\n");
    
    int status;
    int _count = MIN(count, MAX_PKT_SIZE);
    struct usb_iterrupt_driver_t *dev = NULL;
    struct urb *urb = NULL;

    dev = (struct usb_iterrupt_driver_t*)f->private_data;//Gets our driver  structure from file structure which was safed \
                                                           in usbtest_open() metods 
    
    urb = usb_alloc_urb(0, GFP_KERNEL);   //Allocating kernel memory for urb structure 
    
    urb->number_of_packets = 1;

    if(!urb)
    {
       printk(KERN_ERR "< Writing function > Cennot to allocate urb" );
       return -ENOMEM;
    }
    
   
    if(dev->commandBuffer == NULL)
    {
       printk(KERN_ERR "< Writing function > Cannot allocate memory for commands buffer!" );
       dev->commandBuffer = NULL;
       return -2;
    }
  
    memset(dev->commandBuffer, 0, _count); //clearing the buffer
    
    if (copy_from_user(dev->commandBuffer, buf, _count))
    {
        printk(KERN_ERR "< Writing function > Cannot  copy memory for commands buffer!" );
        return -EFAULT;
    }
    
    printk("< Writing function > Writing in Driver command %c \n", *dev->commandBuffer);
    printk("< Writing function > Writing in Driver _count %d \n", _count);
    printk("< Writing function > dev->interrupt_endpoint_IN_address 0x%x \n", dev->interrupt_endpoint_OUT_address);
  
    usb_fill_int_urb(urb, dev->usb_dev, usb_sndintpipe(dev->usb_dev, dev->interrupt_endpoint_OUT_address), dev->commandBuffer, _count, \
    urb_write_int_callback, dev, dev->interrupt_Interval_Out);  //filling the urb structure
   
    printk(KERN_ERR "< Writing function >  urb->number_of_packets %d", urb->number_of_packets ); 
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
    //int _ret;
    struct usb_iterrupt_driver_t *dev;
    struct usb_endpoint_descriptor *endpoint;
    int retval = -ENOMEM; 
   
    usb_cd.name = "usbTest%d";
    usb_cd.fops = &fops;
    usb_cd.minor_base = MYUSB_MINOR_BASE;
       
   
   
//******************************************************** Initialization our structure dev
 
  dev = kzalloc(sizeof(struct usb_iterrupt_driver_t), GFP_KERNEL); //Allocation kernel memory for our driver description structure
  
  //dev->interrupt_endpoint_IN_address = ENDPOINT_IN_ADDRESS;
  //dev->interrupt_endpoint_OUT_address = ENDPOINT_OUT_ADDRESS;
  dev->intf = intf;
  dev->usb_dev = interface_to_usbdev(intf);
  dev->iface_desc = intf->cur_altsetting;
  dev->interrupt_Interval_In = 100;
  dev->interrupt_Interval_Out = 100;
  dev->usb_interrupt_out_size = 65;
  dev->usb_interrupt_in_size = 65;
  dev->urb = usb_alloc_urb(0, GFP_KERNEL);
  dev->bufferRead = kzalloc(MAX_PKT_SIZE, GFP_KERNEL);
  dev->commandBuffer = kzalloc(MAX_PKT_SIZE, GFP_KERNEL);
  

  DUMP_INTERFACE_DESCRIPTORS(dev->iface_desc->desc); //Showing our interface desriptor in dmesg
   
  //**************************************************Researching endpoints and filling our driver sructure 
  for( int i = 0; i < dev->iface_desc->desc.bNumEndpoints; ++i ) 
  {
    DUMP_USB_ENDPOINT_DESCRIPTOR(dev->iface_desc->endpoint[i].desc);
    endpoint = &dev->iface_desc->endpoint[i].desc;
    if(usb_endpoint_is_int_in(endpoint)) //If enpoind is IN
    {
      dev->usb_interrupt_in_size  = endpoint->wMaxPacketSize;
      dev->interrupt_endpoint_IN_address = endpoint->bEndpointAddress;
      printk(KERN_INFO "USB_DIR_IN: 0x%x\n", endpoint->bEndpointAddress);
      printk(KERN_INFO "IN: endpoint->wMaxPacketSize %d\n\n", endpoint->wMaxPacketSize);
    }
    if(usb_endpoint_is_int_out(endpoint)) //If ebdpoint is out
    {
       dev->usb_interrupt_out_size = endpoint->wMaxPacketSize;
       dev->interrupt_endpoint_OUT_address =  endpoint->bEndpointAddress;
       printk(KERN_INFO "USB_DIR_OUT 0x%x\n", endpoint->bEndpointAddress);
       printk(KERN_INFO "OUT: endpoint->wMaxPacketSize %d\n\n", endpoint->wMaxPacketSize);
    }
  }
  
  if(!(dev->interrupt_endpoint_IN_address && dev->interrupt_endpoint_OUT_address))
  {
    printk(KERN_ERR "Cannot get endpoint address.");
    goto error; 
  }
  
  printk(KERN_INFO "Device's structure has initialized");
   
  usb_set_intfdata(intf, dev); //Safe this device structure with this interface structure for have oportuniti get this structure other functions
  
  retval = usb_register_dev(intf, &usb_cd); //Register this driver

  if (retval)
  {
    printk(KERN_ERR "Not able to get a minor for this device.");
    goto error;
  }
 
  printk(KERN_INFO "Minor obtained: %d\n", intf->minor);
  printk(KERN_INFO"The usb device now attached to /dev\n");
  return 0;

  error: //free all kernel memory what allocated in the probe function if error

    usb_set_intfdata(intf, NULL);//Safe NUUL with this interface structure 
    kfree(dev->bufferRead); // free kernel memory for buffer read
    kfree(dev->commandBuffer); // free kernel memory for buffer commands
    printk("Probe function is error \n");
    usb_deregister_dev(intf, &usb_cd); //deregister device structure
    dev->commandBuffer = NULL;
    dev->bufferRead = NULL;
    dev->intf = NULL;
    kfree(dev);
    return  retval;
}

//#############################################################################  DISCONNECT  #######################################################
static void my_usb_disconnect(struct usb_interface *intf)
{
    struct usb_iterrupt_driver_t *dev;
    dev = usb_get_intfdata(intf); //Get device interface what has saving in the probe function by   usb_set_intfdata(intf, dev); 
    usb_set_intfdata(intf, NULL);
    kfree(dev->bufferRead);
    kfree(dev->commandBuffer);
    printk("my_usb_devdrv - Disconnect Function\n");
    usb_deregister_dev(intf, &usb_cd);
    dev->commandBuffer = NULL;
    dev->bufferRead = NULL;
    dev->intf = NULL;
    kfree(dev);
    
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