# Makefile for efi-hello-world-bootloader

ARCH            = $(shell uname -m | sed s,i[3456789]86,ia32,)

OBJS            = main.o
TARGET          = hello.efi

EFIINC          = /usr/include/efi
EFILIB          = /usr/lib
EFI_CRT_OBJS    = $(EFILIB)/crt0-efi-$(ARCH).o
EFI_LDS         = $(EFILIB)/elf_$(ARCH)_efi.lds

CFLAGS          = -nostdlib \
                  -fno-stack-protector \
                  -fno-strict-aliasing \
                  -fno-builtin \
                  -fpic \
                  -fshort-wchar \
                  -mno-red-zone \
                  -Wall

ifeq ($(ARCH),x86_64)
  CFLAGS        += -DEFI_FUNCTION_WRAPPER
endif

CFLAGS          += -I$(EFIINC) \
                   -I$(EFIINC)/$(ARCH) \
                   -I$(EFIINC)/protocol

LDFLAGS         = -nostdlib \
                  -znocombreloc \
                  -shared \
                  -no-undefined \
                  -Bsymbolic

LDFLAGS         += -T $(EFI_LDS) \
                   -L$(EFILIB) \
                   $(EFI_CRT_OBJS)

LIBS            = -lefi \
                  -lgnuefi

OBJCOPYFLAGS    = -j .text \
                  -j .sdata \
                  -j .data \
                  -j .dynamic \
                  -j .dynsym \
                  -j .rel \
                  -j .rela \
                  -j .reloc \
                  --target=efi-app-$(ARCH)

all: $(TARGET)

install: $(TARGET)
	cp $(TARGET) /boot/efi/$(TARGET)

hello.so: $(OBJS)
	ld $(LDFLAGS) $(OBJS) -o $@ $(LIBS)

%.efi: %.so
	objcopy $(OBJCOPYFLAGS) $^ $@

.PHONY:    clean tags

tags:
	ctags -R .

clean:
	rm -f $(OBJS) $(TARGET) hello.so tags
