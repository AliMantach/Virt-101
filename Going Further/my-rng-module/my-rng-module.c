/*
 * Module kernel pour le générateur de nombres aléatoires virtuel
 * Version avec découverte automatique PCI
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/ioctl.h>
#include <linux/pci.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Virt-101 Exercise");
MODULE_DESCRIPTION("Driver PCI pour générateur de nombres aléatoires virtuel");
MODULE_VERSION("3.0");

/* Define ioctl commands */
#define MY_RNG_IOCTL_RAND    _IOR('q', 1, unsigned int)
#define MY_RNG_IOCTL_SEED    _IOW('q', 1, unsigned int)
#define MY_RNG_IOCTL_RAND64  _IOR('q', 2, unsigned long long)

/* PCI IDs du périphérique */
#define MY_RNG_VENDOR_ID 0x1234
#define MY_RNG_DEVICE_ID 0xcafe

/* Pointeur vers la région MMIO */
static void __iomem *devmem = NULL;

/* Numéro de device (major number) alloué dynamiquement */
static int major_num = 0;

/* Pointeur vers le périphérique PCI */
static struct pci_dev *my_rng_pci_dev = NULL;

/* Fonction ioctl */
static long my_rng_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    unsigned int value;
    unsigned long long value64;
    unsigned int low, high;
    int ret;

    if (!devmem) {
        pr_err("my_rng: Device not mapped\n");
        return -ENODEV;
    }

    switch (cmd) {
    case MY_RNG_IOCTL_RAND:
        /* Lire un nombre aléatoire 32-bit depuis le registre à l'offset 0 */
        value = ioread32(devmem);
        
        /* Copier vers l'espace utilisateur */
        ret = copy_to_user((unsigned int __user *)arg, &value, sizeof(unsigned int));
        if (ret != 0) {
            return -EFAULT;
        }
        break;

    case MY_RNG_IOCTL_RAND64:
        /* Lire un nombre aléatoire 64-bit depuis les registres à l'offset 0x8 et 0xC */
        low = ioread32(devmem + 0x8);   /* 32 bits bas */
        high = ioread32(devmem + 0xC);  /* 32 bits haut */
        value64 = ((unsigned long long)high << 32) | low;
        
        /* Copier vers l'espace utilisateur */
        ret = copy_to_user((unsigned long long __user *)arg, &value64, sizeof(unsigned long long));
        if (ret != 0) {
            return -EFAULT;
        }
        break;

    case MY_RNG_IOCTL_SEED:
        /* Copier la valeur de seed depuis l'espace utilisateur */
        ret = copy_from_user(&value, (unsigned int __user *)arg, sizeof(unsigned int));
        if (ret != 0) {
            return -EFAULT;
        }
        
        /* Écrire la valeur de seed dans le registre à l'offset 4 */
        iowrite32(value, devmem + 4);
        pr_info("my_rng: RNG seeded with 0x%x\n", value);
        break;

    default:
        return -ENOTTY;
    }

    return 0;
}

/* File operations */
static struct file_operations my_rng_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = my_rng_ioctl,
};

/* Table des IDs PCI supportés */
static struct pci_device_id my_rng_pci_ids[] = {
    { PCI_DEVICE(MY_RNG_VENDOR_ID, MY_RNG_DEVICE_ID) },
    { 0, }
};
MODULE_DEVICE_TABLE(pci, my_rng_pci_ids);

/*
 * Fonction appelée quand le périphérique PCI est détecté
 */
static int my_rng_pci_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
    unsigned long mmio_start;
    unsigned long mmio_len;

    pr_info("my_rng: PCI device found (vendor=0x%x, device=0x%x)\n",
            pdev->vendor, pdev->device);

    /* Activer le périphérique PCI */
    if (pci_enable_device(pdev)) {
        pr_err("my_rng: Cannot enable PCI device\n");
        return -EIO;
    }

    /* Réserver les régions MMIO */
    if (pci_request_regions(pdev, "my_rng")) {
        pr_err("my_rng: Cannot request PCI regions\n");
        pci_disable_device(pdev);
        return -EIO;
    }

    /* Obtenir l'adresse de base MMIO depuis BAR0 */
    mmio_start = pci_resource_start(pdev, 0);
    mmio_len = pci_resource_len(pdev, 0);

    pr_info("my_rng: MMIO region at 0x%lx (size: %lu bytes)\n", mmio_start, mmio_len);

    /* Mapper la région MMIO */
    devmem = pci_iomap(pdev, 0, mmio_len);
    if (!devmem) {
        pr_err("my_rng: Cannot map MMIO region\n");
        pci_release_regions(pdev);
        pci_disable_device(pdev);
        return -ENOMEM;
    }

    /* Sauvegarder le pointeur vers le périphérique */
    my_rng_pci_dev = pdev;
    pci_set_drvdata(pdev, devmem);

    /* Enregistrer le character device */
    major_num = register_chrdev(0, "my_rng_driver", &my_rng_fops);
    if (major_num < 0) {
        pr_err("my_rng: Failed to register character device\n");
        pci_iounmap(pdev, devmem);
        pci_release_regions(pdev);
        pci_disable_device(pdev);
        return major_num;
    }

    pr_info("my_rng: Character device registered with major number %d\n", major_num);
    pr_info("my_rng: Create device node with: mknod /dev/my_rng_driver c %d 0\n", major_num);
    pr_info("my_rng: Registered ioctls:\n");
    pr_info("my_rng:   0x%lx (RAND - 32-bit random number)\n", (unsigned long)MY_RNG_IOCTL_RAND);
    pr_info("my_rng:   0x%lx (RAND64 - 64-bit random number)\n", (unsigned long)MY_RNG_IOCTL_RAND64);
    pr_info("my_rng:   0x%lx (SEED - set seed)\n", (unsigned long)MY_RNG_IOCTL_SEED);

    return 0;
}

/*
 * Fonction appelée quand le périphérique PCI est retiré
 */
static void my_rng_pci_remove(struct pci_dev *pdev)
{
    pr_info("my_rng: Removing PCI device\n");

    /* Désenregistrer le character device */
    if (major_num > 0) {
        unregister_chrdev(major_num, "my_rng_driver");
    }

    /* Unmapper la région MMIO */
    if (devmem) {
        pci_iounmap(pdev, devmem);
        devmem = NULL;
    }

    /* Libérer les ressources PCI */
    pci_release_regions(pdev);
    pci_disable_device(pdev);

    my_rng_pci_dev = NULL;

    pr_info("my_rng: PCI device removed\n");
}

/* PCI driver structure */
static struct pci_driver my_rng_pci_driver = {
    .name     = "my_rng_pci",
    .id_table = my_rng_pci_ids,
    .probe    = my_rng_pci_probe,
    .remove   = my_rng_pci_remove,
};

/* Fonction d'initialisation du module */
static int __init my_rng_init(void)
{
    int ret;

    pr_info("my_rng: Loading PCI driver module\n");

    /* Enregistrer le PCI driver */
    ret = pci_register_driver(&my_rng_pci_driver);
    if (ret < 0) {
        pr_err("my_rng: Failed to register PCI driver\n");
        return ret;
    }

    pr_info("my_rng: PCI driver registered successfully\n");
    return 0;
}

/* Fonction de nettoyage du module */
static void __exit my_rng_exit(void)
{
    pr_info("my_rng: Unloading PCI driver module\n");

    /* Désenregistrer le PCI driver */
    pci_unregister_driver(&my_rng_pci_driver);

    pr_info("my_rng: PCI driver unloaded\n");
}

module_init(my_rng_init);
module_exit(my_rng_exit);
