CC 		:= gcc
LD 		:= ld
NASM 	:= nasm

BASEDIR 	:= .
BUILDDIR 	:= $(BASEDIR)/build

FLOPPY_IMAGE 		:= $(BUILDDIR)/floppy.img
DISK_IMAGE 			:= $(BUILDDIR)/disk.img
DISK_FS_OFFSET 		:= 1048576

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

all: disk

clean:
	rm -rf $(BUILDDIR)

$(OBJDIRS):
	@mkdir -p $@

$(CONFIG_H):
	[ -f $@ ] || cp $(BASEDIR)/config.sample.h $@

$(BUILDDIR)/data.o: $(BUILDDIR)/data.c
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

always_rebuild:

$(BUILDDIR)/data.c: always_rebuild | $(OBJDIRS)
	./tools/mkdata.pl

$(BUILDDIR)/%.o: %.c | $(OBJDIRS) $(CONFIG_H)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILDDIR)/%.o: %.s | $(OBJDIRS)
	$(NASM) $(ASFLAGS) -f elf32 $< -o $@

kernel: $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $(BUILDDIR)/gentleos.elf

disk: kernel
	zcat $(BASEDIR)/misc/empty-disk.img > $(DISK_IMAGE)
	mcopy -D o -i $(DISK_IMAGE)@@$(DISK_FS_OFFSET) $(BUILDDIR)/gentleos.elf ::
	mcopy -D o -i $(DISK_IMAGE)@@$(DISK_FS_OFFSET) $(BASEDIR)/misc/grub.cfg ::boot/grub

	cp $(BASEDIR)/misc/grub-floppy.img $(FLOPPY_IMAGE)
	mcopy -D o -i $(FLOPPY_IMAGE) $(BUILDDIR)/gentleos.elf ::
	mcopy -D o -i $(FLOPPY_IMAGE) $(BASEDIR)/misc/menu.lst ::boot

print:
	@echo "SUBDIRS=$(SUBDIRS)"
	@echo "SRCS=$(SRCS)"
	@echo "OBJS=$(OBJS)"

.PHONY: all clean kernel disk print

# Include auto-generated dependency files if they exist
-include $(DEPS)
