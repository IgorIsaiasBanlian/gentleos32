CC              := gcc
LD              := ld
NASM            := nasm
OBJCOPY         := objcopy

BASEDIR         := .
BUILDDIR        := $(BASEDIR)/build

KERNEL_HIMEM_ELF    := $(BUILDDIR)/kernel-himem.elf
KERNEL_HIMEM_BIN    := gentleos.bin
KERNEL_LOMEM_ELF    := $(BUILDDIR)/kernel-lomem.elf
KERNEL_LOMEM_BIN    := $(BUILDDIR)/kernel-lomem.bin

FLOPPY_IMAGE    := gentleos32-floppy.img
DISK_IMAGE      := gentleos32-disk.img
DISK_FS_OFFSET  := 1048576

CONFIG_H        := $(BASEDIR)/config.h
KERNEL_HIMEM_LD := $(BASEDIR)/misc/kernel-himem.ld
KERNEL_LOMEM_LD := $(BASEDIR)/misc/kernel-lomem.ld

KERNEL_ASFLAGS  :=

KERNEL_CFLAGS   := -std=c11 -m32 -march=i386 -O2 \
                   -ffreestanding -fno-stack-protector -fno-pic \
                   -Wall -Wextra -pedantic \
                   -I$(BASEDIR)/include

KERNEL_LDFLAGS  := -m elf_i386 -nostdlib -z nodefaultlib \
                   -z noexecstack --no-warn-rwx-segments \
                   --orphan-handling=warn

KERNEL_SUBDIRS  := gui apps lib kernel
KERNEL_C_SRCS   := $(foreach d,$(KERNEL_SUBDIRS),$(wildcard $(d)/*.c))
KERNEL_S_SRCS   := $(foreach d,$(KERNEL_SUBDIRS),$(wildcard $(d)/*.s))
KERNEL_SRCS     := $(KERNEL_C_SRCS) $(KERNEL_S_SRCS)
KERNEL_OBJS     := $(patsubst %.c,$(BUILDDIR)/%.o,$(KERNEL_C_SRCS)) \
                   $(patsubst %.s,$(BUILDDIR)/%.o,$(KERNEL_S_SRCS)) \
                   $(BUILDDIR)/data.o
KERNEL_DEPS     := $(KERNEL_OBJS:.o=.d)


BOOT_CFLAGS     := -std=c11 -m16 -march=i386 -Os \
                   -ffreestanding -fno-stack-protector -fno-pic \
                   -fno-asynchronous-unwind-tables \
                   -Wall -Wextra -pedantic \
                   -I$(BASEDIR)/include

BOOT_LDFLAGS    := -m elf_i386 -nostdlib -z nodefaultlib \
                   -z noexecstack --no-warn-rwx-segments \
                   --orphan-handling=warn

BOOT_SUBDIRS    := boot
BOOT_LD         := misc/boot.ld
BOOT_OBJS       := $(BUILDDIR)/boot/boot_a.o $(BUILDDIR)/boot/boot_c.o
BOOT_DEPS       := $(BOOT_OBJS:.o=.d)
BOOT_ELF        := $(BUILDDIR)/boot/boot.elf
BOOT_BIN        := $(BUILDDIR)/boot.bin

OBJDIRS := $(addprefix $(BUILDDIR)/,$(KERNEL_SUBDIRS)) \
           $(addprefix $(BUILDDIR)/,$(BOOT_SUBDIRS))

all: disks
	./tools/chkcfg.pl

disks: $(KERNEL_HIMEM_BIN) $(KERNEL_LOMEM_BIN) $(BOOT_BIN)
	zcat $(BASEDIR)/misc/empty-disk.img > $(DISK_IMAGE)
	mcopy -D o -i $(DISK_IMAGE)@@$(DISK_FS_OFFSET) $(KERNEL_HIMEM_BIN) ::
	mcopy -D o -i $(DISK_IMAGE)@@$(DISK_FS_OFFSET) $(BASEDIR)/misc/grub.sample.cfg ::boot/grub/grub.cfg
	[ -f $(BASEDIR)/misc/grub.cfg ] && mcopy -D o -i $(DISK_IMAGE)@@$(DISK_FS_OFFSET) $(BASEDIR)/misc/grub.cfg ::boot/grub/grub.cfg || true

	cp $(BASEDIR)/misc/grub-floppy.img $(FLOPPY_IMAGE)
	mcopy -D o -i $(FLOPPY_IMAGE) $(KERNEL_HIMEM_BIN) ::
	mcopy -D o -i $(FLOPPY_IMAGE) $(BASEDIR)/misc/menu.sample.lst ::boot/menu.lst
	[ -f $(BASEDIR)/misc/menu.lst ] && mcopy -D o -i $(FLOPPY_IMAGE) $(BASEDIR)/misc/menu.lst ::boot/menu.lst || true

	dd if=$(BOOT_BIN) of=gentleos32-lomem.img bs=512 count=1
	cat $(BOOT_BIN) $(KERNEL_LOMEM_BIN) >> gentleos32-lomem.img

clean:
	rm -rf $(BUILDDIR) $(KERNEL_HIMEM_BIN) $(DISK_IMAGE) $(FLOPPY_IMAGE)

$(OBJDIRS):
	@mkdir -p $@

$(CONFIG_H):
	[ -f $@ ] || cp $(BASEDIR)/config.sample.h $@

$(KERNEL_HIMEM_ELF): $(KERNEL_OBJS) $(KERNEL_HIMEM_LD)
	$(LD) $(KERNEL_LDFLAGS) -T$(KERNEL_HIMEM_LD) $(KERNEL_OBJS) -o $@

$(KERNEL_LOMEM_ELF): $(KERNEL_OBJS) $(KERNEL_LOMEM_LD)
	$(LD) $(KERNEL_LDFLAGS) -T$(KERNEL_LOMEM_LD) $(KERNEL_OBJS) -o $@

$(KERNEL_HIMEM_BIN): $(KERNEL_HIMEM_ELF)
	$(OBJCOPY) -O binary $< $@

$(KERNEL_LOMEM_BIN): $(KERNEL_LOMEM_ELF)
	$(OBJCOPY) -O binary $< $@

$(BUILDDIR)/data.o: $(BUILDDIR)/data.c
	$(CC) $(KERNEL_CFLAGS) -MMD -MP -c $< -o $@

ALWAYS_REBUILD:

$(BUILDDIR)/data.c: ALWAYS_REBUILD | $(OBJDIRS)
	./tools/mkdata.pl

$(BUILDDIR)/%.o: %.c | $(OBJDIRS) $(CONFIG_H)
	$(CC) $(KERNEL_CFLAGS) -MMD -MP -c $< -o $@

$(BUILDDIR)/%.o: %.s | $(OBJDIRS)
	$(NASM) $(KERNEL_ASFLAGS) -f elf32 $< -o $@

$(BUILDDIR)/boot/boot_a.o: boot/boot_a.s $(KERNEL_LOMEM_BIN) | $(OBJDIRS)
	$(NASM) -f elf32 -o $@ $< -DKERNEL_SECTORS=$(shell ./tools/sectors.pl $(KERNEL_LOMEM_BIN))

$(BUILDDIR)/boot/boot_c.o: boot/boot_c.c | $(OBJDIRS) $(CONFIG_H)
	$(CC) $(BOOT_CFLAGS) -MMD -MP -c $< -o $@

$(BOOT_ELF): $(BOOT_OBJS) $(BOOT_LD)
	$(LD) $(BOOT_LDFLAGS) -T$(BOOT_LD) $(BOOT_OBJS) -o $@

$(BOOT_BIN): $(BOOT_ELF)
	$(OBJCOPY) -O binary $< $@
	test $$(wc -c < $@) -eq $$((512 * 5))

print:
	@echo "KERNEL_SUBDIRS=$(KERNEL_SUBDIRS)"
	@echo "KERNEL_SRCS=$(KERNEL_SRCS)"
	@echo "KERNEL_OBJS=$(KERNEL_OBJS)"

.PHONY: all clean kernel print check-config

# Include auto-generated dependency files if they exist
-include $(KERNEL_DEPS) $(BOOT_DEPS)
