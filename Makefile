CC              := gcc
LD              := ld
NASM            := nasm
OBJCOPY         := objcopy

BASEDIR         := .
BUILDDIR        := $(BASEDIR)/build

KERNEL_ELF      := $(BUILDDIR)/gentleos.elf
KERNEL_BIN      := gentleos.bin

FLOPPY_IMAGE    := gentleos32-floppy.img
DISK_IMAGE      := gentleos32-disk.img
DISK_FS_OFFSET  := 1048576

CONFIG_H        := $(BASEDIR)/config.h
KERNEL_LD       := $(BASEDIR)/misc/kernel.ld

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

OBJDIRS := $(addprefix $(BUILDDIR)/,$(KERNEL_SUBDIRS))

all: disks
	./tools/chkcfg.pl

disks: $(KERNEL_BIN)
	zcat $(BASEDIR)/misc/empty-disk.img > $(DISK_IMAGE)
	mcopy -D o -i $(DISK_IMAGE)@@$(DISK_FS_OFFSET) $(KERNEL_BIN) ::
	mcopy -D o -i $(DISK_IMAGE)@@$(DISK_FS_OFFSET) $(BASEDIR)/misc/grub.sample.cfg ::boot/grub/grub.cfg
	[ -f $(BASEDIR)/misc/grub.cfg ] && mcopy -D o -i $(DISK_IMAGE)@@$(DISK_FS_OFFSET) $(BASEDIR)/misc/grub.cfg ::boot/grub/grub.cfg || true

	cp $(BASEDIR)/misc/grub-floppy.img $(FLOPPY_IMAGE)
	mcopy -D o -i $(FLOPPY_IMAGE) $(KERNEL_BIN) ::
	mcopy -D o -i $(FLOPPY_IMAGE) $(BASEDIR)/misc/menu.sample.lst ::boot/menu.lst
	[ -f $(BASEDIR)/misc/menu.lst ] && mcopy -D o -i $(FLOPPY_IMAGE) $(BASEDIR)/misc/menu.lst ::boot/menu.lst || true

clean:
	rm -rf $(BUILDDIR) $(KERNEL_BIN) $(DISK_IMAGE) $(FLOPPY_IMAGE)

$(OBJDIRS):
	@mkdir -p $@

$(CONFIG_H):
	[ -f $@ ] || cp $(BASEDIR)/config.sample.h $@

$(KERNEL_ELF): $(KERNEL_OBJS) $(KERNEL_LD)
	$(LD) $(KERNEL_LDFLAGS) -T$(KERNEL_LD) $(KERNEL_OBJS) -o $@

$(KERNEL_BIN): $(KERNEL_ELF)
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

print:
	@echo "KERNEL_SUBDIRS=$(KERNEL_SUBDIRS)"
	@echo "KERNEL_SRCS=$(KERNEL_SRCS)"
	@echo "KERNEL_OBJS=$(KERNEL_OBJS)"

.PHONY: all clean kernel print check-config

# Include auto-generated dependency files if they exist
-include $(KERNEL_DEPS)
