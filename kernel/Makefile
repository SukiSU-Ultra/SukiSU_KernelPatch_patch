ifndef TARGET_COMPILE
    TARGET_COMPILE=$(ANDROID_NDK_HOME)/toolchains/llvm/prebuilt/linux-x86_64/bin
endif

TARGET=kpimg

CC = $(TARGET_COMPILE)/aarch64-linux-android35-clang
LD = $(TARGET_COMPILE)/ld.lld
AS = $(TARGET_COMPILE)/llvm-as
OBJCOPY = $(TARGET_COMPILE)/llvm-objcopy

CFLAGS += -Wall -fno-builtin -std=gnu11 -nostdinc -mgeneral-regs-only
CFLAGS += -g
CFLAGS += -Wno-unused-function -Wno-return-type -Wno-c23-extensions -Wno-uninitialized
CFLAGS += -fno-PIC -fno-asynchronous-unwind-tables -fno-stack-protector -fno-unwind-tables -fno-semantic-interposition -U_FORTIFY_SOURCE -fno-common

ifdef DEBUG
	CFLAGS += -DDEBUG -DMAP_DEBUG -g
endif

# ifdef ANDROID
	CFLAGS += -DANDROID
# endif

INCLUDE := -I. -Iinclude -Ipatch/include -Ilinux -Ilinux/include -Ilinux/arch/arm64/include -Ilinux/tools/arch/arm64/include -Ikernel-source -I$(TARGET_COMPILE)/../sysroot/usr/include -I$(TARGET_COMPILE)/../sysroot/usr/include/aarch64-linux-android

BASE_SRCS += base/setup.c 
BASE_SRCS += base/setup1.S
BASE_SRCS += base/cache.S
BASE_SRCS += base/tlsf.c
BASE_SRCS += base/start.c 
BASE_SRCS += base/map.c 
BASE_SRCS += base/map1.S 
BASE_SRCS += base/hook.c 
BASE_SRCS += base/fphook.c 
BASE_SRCS += base/hotpatch.c 
BASE_SRCS += base/hmem.c 
BASE_SRCS += base/predata.c 
BASE_SRCS += base/puff/puff.c
BASE_SRCS += base/puff/setjmp_aarch64.S
BASE_SRCS += base/symbol.c 
BASE_SRCS += base/baselib.c 
BASE_SRCS += base/sha256.c 

BASE_SRCS += $(wildcard patch/*.c)
BASE_SRCS += $(wildcard patch/common/*.c)
BASE_SRCS += $(wildcard patch/module/*.c)
BASE_SRCS += $(wildcard patch/ksyms/*.c)

BASE_SRCS += patch/android/sepolicy_flags.c

BASE_SRCS += $(wildcard patch/sukisu/*.c)


SRCS += $(BASE_SRCS)
SRCS += $(LINUX_SRCS)

OBJS := $(SRCS:.c=.o)
OBJS := $(OBJS:.S=.o)


LDFLAGS += -Tkpimg.lds --no-relax

all: ${TARGET}

${TARGET}: ${TARGET}.elf
	${OBJCOPY} -O binary -S $^ $@

${TARGET}.elf: ${OBJS}
	${LD} -nostdlib -static -no-pie -Tkpimg.lds -e start -o $@ $^

%.o: %.c
	${CC} $(CFLAGS) $(INCLUDE) -c -O2 -o $@ $<

base/sha256.o: base/sha256.c
	${CC} $(CFLAGS) $(INCLUDE) -c -O0 -o $@ $<

base/puff/puff.o: base/puff/puff.c
	${CC} $(CFLAGS) $(INCLUDE) -c -O2 -DINFLATE_ALLOW_INVALID_DISTANCE_TOOFAR_ARRR -o $@ $<

%.o: %.S
	${CC} $(CFLAGS) $(INCLUDE) -c -o $@ $<

.PHONY: clean
clean:
	rm -rf *.elf
	rm -rf kpimg
	find . -name *.o | xargs rm -f
