STATIC_LINKING := 1
AR             := ar

ifeq ($(platform),)
platform = unix
endif

system_platform = unix

CORE_DIR    := .
TARGET_NAME := openlara
LIBM        := -lm

ifeq ($(platform), sf2000)
   TARGET := _libretro_$(platform).a
   MIPS := /opt/mips32-mti-elf/2019.09-03-2/bin/mips-mti-elf-
   CC = $(MIPS)gcc
   CXX = $(MIPS)g++
   AR = $(MIPS)ar
   CFLAGS = -EL -march=mips32 -mtune=mips32 -msoft-float -G0 -mno-abicalls -fno-pic
   CFLAGS += -O3 -ffast-math -fomit-frame-pointer -ffunction-sections -fdata-sections
   CFLAGS += -fno-math-errno -fno-trapping-math -finline-functions -fmerge-all-constants
   CFLAGS += -DSF2000 -DNDEBUG -I$(CORE_DIR)/src -I$(CORE_DIR) -I../../libs/libretro-common/include
   CXXFLAGS := $(CFLAGS) -std=c++11 -fno-exceptions -fno-rtti -fno-threadsafe-statics
   STATIC_LINKING = 1
else
   TARGET := $(TARGET_NAME)_libretro.so
   fpic := -fPIC
   SHARED := -shared -Wl,--version-script=link.T -Wl,--no-undefined
   CFLAGS += -O3 -fPIC -I$(CORE_DIR)/src -I$(CORE_DIR) -I../../libs/libretro-common/include
   CXXFLAGS += $(CFLAGS) -std=c++11
endif

SOURCES_CXX := \
	src/platform/sf2000/libretro.cpp \
	src/libs/minimp3/minimp3.cpp

SOURCES_C := \
	src/libs/tinf/tinflate.c \
	src/libs/stb_vorbis/stb_vorbis.c

OBJECTS := $(SOURCES_CXX:.cpp=.o) $(SOURCES_C:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
ifeq ($(STATIC_LINKING), 1)
	$(AR) rcs $@ $(OBJECTS)
else
	$(CXX) $(OBJECTS) $(SHARED) $(LDFLAGS) $(LIBM) -o $@
endif

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET) _libretro_*.a *.so

.PHONY: all clean
