#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/string.h>
#include <linux/device.h>
#include <linux/hashtable.h>
#include <linux/mutex.h>

#include<linux/init.h>
#include<linux/moduleparam.h>

#define DEVICE_NAME                     "ht438_dev"

#define NUM_DEVICES                     2

static dev_t ht438_dev_number;
struct class *ht438_dev_class;

typedef struct {
    // struct mutex ht438_mutex;
    struct cdev cdev;
    char name[20];
    DECLARE_HASHTABLE(ht438_tbl, 5);
} ht438_dev_t;

typedef struct ht_object {
    int key;
    char data[4]; 
} ht_object_t; 

struct ht438_node {
    ht_object_t obj;
    struct hlist_node ht_node;
};

ht438_dev_t *ht438_dev[NUM_DEVICES];

static int ht438_drv_open(struct inode *inode, struct file *file)
{
    ht438_dev_t *ht438_devp;
    
    ht438_devp = container_of(inode->i_cdev, ht438_dev_t, cdev);

    // mutex_init(&ht438_devp->ht438_mutex);

    file->private_data = ht438_devp;
    
    printk("Device Opened: %s", ht438_devp->name);

	return 0;
}

static int ht438_drv_release(struct inode *inode, struct file *file)
{
    ht438_dev_t *ht438_devp = file->private_data;

    // mutex_unlock(&ht438_devp->ht438_mutex);

    printk("Device Closed: %s", ht438_devp->name);
	
    return 0;
}

ssize_t ht438_drv_read(struct file *file, char *buf,
           size_t count, loff_t *ppos)
{
    int i = 0;
    int data_found_flag = 0;

    ht438_dev_t *ht438_devp = file->private_data;

    // mutex_lock(&ht438_devp->ht438_mutex);

    struct ht438_node *temp_node;
    ht_object_t temp_ht_object;

    if(copy_from_user(&temp_ht_object, (ht_object_t *)buf, sizeof(ht_object_t))) {
        printk("Cannot copy from user\n");
        return -1;
    }

    printk("Reading Data from key: %d\n", temp_ht_object.key);

    hash_for_each_possible(ht438_devp->ht438_tbl, temp_node, ht_node, temp_ht_object.key)
    {
        if(temp_node->obj.key == temp_ht_object.key) {
            // strcpy(temp_ht_object.data, temp_node->obj.data);
            for(i=0; i<4; i++) {
                temp_ht_object.data[i] = temp_node->obj.data[i];
            }
            data_found_flag = 1;
        }
    }

    if(data_found_flag) {
        printk("Found Data at Key: %d\n", temp_ht_object.key);
        printk("Data is: %s", temp_ht_object.data);
        if(copy_to_user((ht_object_t *)buf, &temp_ht_object, sizeof(ht_object_t))) {
            printk("Cannot copy to user\n");
            return -1;
        }
    }
    else {
        printk("Could not find data at given key\n");
        return -1;
    }

    // mutex_unlock(&ht438_devp->ht438_mutex);

	return 0;
}

ssize_t ht438_drv_write(struct file *file, const char *buf,
           size_t count, loff_t *ppos)
{
    int i = 0;
    int data_update_flag = 0;

    ht438_dev_t *ht438_devp = file->private_data;

    // mutex_lock(&ht438_devp->ht438_mutex);

    struct ht438_node *node = kmalloc(sizeof(struct ht438_node), GFP_KERNEL);
    struct ht438_node *temp_node;
    
    ht_object_t temp_ht_object;

    memset(node, 0, sizeof(struct ht438_node));

    if(copy_from_user(&temp_ht_object, (ht_object_t *)buf, sizeof(ht_object_t))) {
        printk("Cannot copy from user\n");
        return -1;
    }

    node->obj.key = temp_ht_object.key;
    for(i=0; i<4; i++) {
        node->obj.data[i] = temp_ht_object.data[i];
    }

    if(!strcmp(node->obj.data, ""))
    {
        printk("Data at Key: %d is 0, deleting node\n", node->obj.key);
        hash_for_each_possible(ht438_devp->ht438_tbl, temp_node, ht_node, node->obj.key)
        {
            if(node->obj.key == temp_node->obj.key) {
                hash_del(&temp_node->ht_node);
            }
        }
    }
    else
    {
        printk("Writing Data: %s, at Key: %d", node->obj.data, node->obj.key);
        hash_for_each_possible(ht438_devp->ht438_tbl, temp_node, ht_node, node->obj.key)
        {
            if(node->obj.key == temp_node->obj.key) {
                // strcpy(temp_node->obj.data, node->obj.data);
                while((i < 4) || (node->obj.data[i] != 0)) {
                    temp_node->obj.data[i] = node->obj.data[i];
                }
                data_update_flag = 1;
            }
        }
        if(!data_update_flag) {
            hash_add(ht438_devp->ht438_tbl, &node->ht_node, node->obj.key);
        }
    }
    
    // mutex_unlock(&ht438_devp->ht438_mutex);

	return 1;
}

struct file_operations ht438_fops = {
	.owner = THIS_MODULE,
	.open = ht438_drv_open,
	.release = ht438_drv_release,
	.read = ht438_drv_read,
	.write = ht438_drv_write
};

/*
 * Driver Initialization
 */
int __init ht438_driver_init(void)
{
    int i=0;
    int ret;

    if(alloc_chrdev_region(&ht438_dev_number, 0, 2, DEVICE_NAME)) {
        printk(KERN_DEBUG "Can't register device\n"); return -1;
    }

    ht438_dev[0] = kmalloc(sizeof(ht438_dev_t), GFP_KERNEL);
	if (!ht438_dev[0]) {
		printk("Bad Kmalloc\n"); return -ENOMEM;
	}

    ht438_dev[1] = kmalloc(sizeof(ht438_dev_t), GFP_KERNEL);
	if (!ht438_dev[1]) {
		printk("Bad Kmalloc\n"); return -ENOMEM;
	}

	ht438_dev_class = class_create(THIS_MODULE, DEVICE_NAME);

    sprintf(ht438_dev[0]->name, "ht438_dev_0");
    sprintf(ht438_dev[1]->name, "ht438_dev_1");

    for(i=0; i<NUM_DEVICES; i++)
    {
        cdev_init(&ht438_dev[i]->cdev, &ht438_fops);
        ht438_dev[i]->cdev.owner = THIS_MODULE;
        ret = cdev_add(&ht438_dev[i]->cdev, MKDEV(MAJOR(ht438_dev_number), i), 1);
        if (ret) {
            printk("Bad cdev\n");
            return ret;
        }

        device_create(ht438_dev_class, NULL, MKDEV(MAJOR(ht438_dev_number), i), NULL, ht438_dev[i]->name);
        // mutex_init(&ht438_dev[i]->ht438_mutex);
        hash_init(ht438_dev[i]->ht438_tbl);

        printk("Device %s created\n", ht438_dev[i]->name);
    }

    return ret;
}
void __exit ht438_driver_exit(void)
{
    int i=0, bucket;
    struct ht438_node *temp_node;

	unregister_chrdev_region(ht438_dev_number, NUM_DEVICES);
    for(i=0; i<NUM_DEVICES; i++)
    {
        hash_for_each(ht438_dev[i]->ht438_tbl, bucket, temp_node, ht_node) {
            hash_del(&temp_node->ht_node);
            kfree(temp_node);
        }
        device_destroy(ht438_dev_class, MKDEV(MAJOR(ht438_dev_number), i));
        cdev_del(&ht438_dev[i]->cdev);

        printk("Device %s deleted\n", ht438_dev[i]->name);
        kfree(ht438_dev[i]);
    }
    class_destroy(ht438_dev_class);
}

module_init(ht438_driver_init);
module_exit(ht438_driver_exit);
