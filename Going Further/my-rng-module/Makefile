# Nom du module
obj-m += my-rng-module.o

# Chemin vers les sources du kernel
KERNEL_SRC := /root/virt-101-exercise/linux-6.6

# Répertoire actuel
PWD := $(shell pwd)

# Compilation
all:
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) modules

# Nettoyage
clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) clean

# Aide
help:
	@echo "Commandes disponibles:"
	@echo "  make       - Compiler le module"
	@echo "  make clean - Nettoyer les fichiers"
	@echo ""
	@echo "Pour charger le module:"
	@echo "  insmod my-rng-module.ko"
	@echo ""
	@echo "Pour décharger le module:"
	@echo "  rmmod my_rng_module"

.PHONY: all clean help
