CC=arm-none-eabi-gcc
AR=arm-none-eabi-ar

vpath %.c real

CFLAGS  = -ggdb3 -O3 -Wall
CFLAGS += -mlittle-endian -mthumb -mthumb-interwork -mcpu=cortex-m4		// Set the CPU to Cortex-M4 and enables Thumb mode
CFLAGS += -fsingle-precision-constant -Wdouble-promotion
CFLAGS += -mfpu=fpv4-sp-d16 -mfloat-abi=hard							// Enables hardware floating point (helix is fixed point, so unused)
CFLAGS += -ffreestanding -nostdlib
CFLAGS += -Ireal -Ipub						// Includes real/ and pub/ to the search path
	
SRCS = mp3dec.c mp3tabs.c bitstream.c buffers.c dct32.c dequant.c dqchan.c		
SRCS += huffman.c hufftabs.c imdct.c polyphase.c scalfact.c
SRCS += stproc.c subband.c trigtabs_fixpt.c

OBJS = $(SRCS:.c=.o)

.PHONY: libhelix.a

all: libhelix.a

%.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $^

libhelix.a: $(OBJS)
	$(AR) -r $@ $(OBJS)

clean:
	rm -f $(OBJS) libhelix.a