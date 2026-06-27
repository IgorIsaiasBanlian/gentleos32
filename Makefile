CC 		:= gcc
LD 		:= ld
NASM 	:= nasm

BASEDIR 	:= .
BUILDDIR 	:= $(BASEDIR)/build

KERNEL_ELF		:= gentleos.elf
FLOPPY_IMAGE 	:= gentleos32-floppy.img
DISK_IMAGE 		:= gentleos32-disk.img
DISK_FS_OFFSET 	:= 1048576

CFLAGS 	:=  -std=c11 -m32 -march=i386 -O2 \
			-ffreestanding -fno-stack-protector \
			-Wall -Wextra -pedantic \
			-I$(BASEDIR)/include

ASFLAGS :=

LDFLAGS := 	-m elf_i386 -nostdlib -z nodefaultlib \
			-z noexecstack --no-warn-rwx-segments \
			-T$(BASEDIR)/misc/kernel.ld

SUBDIRS := gui apps lib kernel
CONFIG_H := $(BASEDIR)/config.h
C_SRCS  := $(foreach d,$(SUBDIRS),$(wildcard $(d)/*.c))
S_SRCS  := $(foreach d,$(SUBDIRS),$(wildcard $(d)/*.s))
SRCS    := $(C_SRCS) $(S_SRCS)
OBJS    := $(patsubst %.c,$(BUILDDIR)/%.o,$(C_SRCS)) \
           $(patsubst %.s,$(BUILDDIR)/%.o,$(S_SRCS)) \
		   $(BUILDDIR)/data.o
DEPS    := $(OBJS:.o=.d)
OBJDIRS := $(addprefix $(BUILDDIR)/,$(SUBDIRS))

all: disks
	./tools/chkcfg.pl

disks: $(KERNEL_ELF)
	zcat $(BASEDIR)/misc/empty-disk.img > $(DISK_IMAGE)
	mcopy -D o -i $(DISK_IMAGE)@@$(DISK_FS_OFFSET) $(KERNEL_ELF) ::
	mcopy -D o -i $(DISK_IMAGE)@@$(DISK_FS_OFFSET) $(BASEDIR)/misc/grub.sample.cfg ::boot/grub/grub.cfg
	[ -f $(BASEDIR)/misc/grub.cfg ] && mcopy -D o -i $(DISK_IMAGE)@@$(DISK_FS_OFFSET) $(BASEDIR)/misc/grub.cfg ::boot/grub/grub.cfg || true

	cp $(BASEDIR)/misc/grub-floppy.img $(FLOPPY_IMAGE)
	mcopy -D o -i $(FLOPPY_IMAGE) $(KERNEL_ELF) ::
	mcopy -D o -i $(FLOPPY_IMAGE) $(BASEDIR)/misc/menu.sample.lst ::boot/menu.lst
	[ -f $(BASEDIR)/misc/menu.lst ] && mcopy -D o -i $(FLOPPY_IMAGE) $(BASEDIR)/misc/menu.lst ::boot/menu.lst || true

clean:
	rm -rf $(BUILDDIR) $(KERNEL_ELF) $(DISK_IMAGE) $(FLOPPY_IMAGE)

$(OBJDIRS):
	@mkdir -p $@

$(CONFIG_H):
	[ -f $@ ] || cp $(BASEDIR)/config.sample.h $@

$(KERNEL_ELF): $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $@

$(BUILDDIR)/data.o: $(BUILDDIR)/data.c
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

always_rebuild:

$(BUILDDIR)/data.c: always_rebuild | $(OBJDIRS)
	./tools/mkdata.pl

$(BUILDDIR)/%.o: %.c | $(OBJDIRS) $(CONFIG_H)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILDDIR)/%.o: %.s | $(OBJDIRS)
	$(NASM) $(ASFLAGS) -f elf32 $< -o $@

print:
	@echo "SUBDIRS=$(SUBDIRS)"
	@echo "SRCS=$(SRCS)"
	@echo "OBJS=$(OBJS)"

.PHONY: all clean kernel print check-config

# Include auto-generated dependency files if they exist
-include $(DEPS)
