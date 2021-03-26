# Debug / Release版
#CONFIG = Debug

ifndef CONFIG
CONFIG = Release
endif

LINK = fan

#CROSS_COMPILE = aarch64-linux-gnu-
CC = $(CROSS_COMPILE)g++

# 编译标志
#CFLAGS 	+= -Wall -Werror
#CFLAGS += -g -MD -O3 -std=c++11 -static
# 程序的默认名称
TARGET = $(CONFIG)/$(LINK)
CFLAGS += -std=c++11
ifeq ($(CONFIG), Debug)
CFLAGS += -Wall -g -MD
else
CFLAGS += -O3 -MD
endif

DIRS = $(shell find . -type d)

# 找出所有的.cpp .h .a .so文件及目录
CPPFILES = $(notdir $(shell find . -type f -name "*.cpp"))
CFILES = $(notdir $(shell find . -type f -name "*.c"))
#
#HFILES_DIR 	= $(shell find . -type f -name "*.h")
#HFILES 		= $(notdir $(HFILES_DIR))
#HDIRS		= $(sort $(dir $(HFILES_DIR)))
#
#AFILES_DIR 	= $(shell find . -type f -name "lib*.a")
#AFILES		= $(notdir $(AFILES_DIR))
#ADIRS		= $(sort $(dir $(AFILES_DIR)))
#
#SOFILES_DIR = $(shell find . -type f -name "lib*.so")
#SOFILES		= $(notdir $(SOFILES_DIR))
#SODIRS		= $(sort $(dir $(SOFILES_DIR)))
#RASPI = /home/oblind/raspi
RASPI = /home/oblind/cpp
#
#     # 包含所有含有.h文件的目录
#INCLUDES += $(HDIRS:%=-I%)
INCLUDES += -Iinclude -I/usr/local/include -I$(RASPI)/include

#$(warning INCLUDES : $(INCLUDES))

CFLAGS	+= $(INCLUDES)

#  # 含有.a .so文件的目录
LDFLAGS += $(ADIRS:%=-L%)
LDFLAGS += $(SODIRS:%=-L%)

# 引用库文件
#LDFLAGS += $(AFILES:lib%.a=-l%)
#LDFLAGS += $(SOFILES:lib%.so=-l%)
#LDFLAGS += -L${RASPI}/lib	#交叉编译

#ifeq ($(CONFIG), Debug)
#LDFLAGS += -lwiringPiDev
#else
LDFLAGS += -lwiringPi
#endif
LDFLAGS += -lcrypt -lpthread -lrt

#
#   $(warning LDFLAGS : [ $(LDFLAGS) ])
#
#    # 包含所有的目录
VPATH = $(DIRS)
#
#  $(warning SOURCES : [ $(SOURCES) ])
#
#   # 目标及依赖
OBJS = $(CPPFILES:%.cpp=$(CONFIG)/obj/%.o) $(CFILES:%.c=$(CONFIG)/obj/%.o)
DEPS = $(CPPFILES:%.cpp=$(CONFIG)/obj/%.d) $(CFILES:%.c=$(CONFIG)/obj/%.d)

$(info config: $(CONFIG))
###########################################################

.PHONY: all clean cleanall

all: $(LINK)

$(LINK): $(TARGET)
	$(shell if [ -f $(LINK) ]; then rm $(LINK); fi;)
	@ln -s $(TARGET) $(LINK)

$(TARGET): $(OBJS)
	@echo
	@echo "Linking..."
	@echo
	$(CC) $^ -o $@ $(LDFLAGS)
	@echo
	@echo "build $(TARGET) success"
	@echo

$(CONFIG)/obj/%.o: %.cpp
	@mkdir -p $(CONFIG)/obj
	$(CC) $(CFLAGS) -c $< -o $@

$(CONFIG)/obj/%.o: %.c
	@mkdir -p $(CONFIG)/obj
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(TARGET) $(OBJS) $(DEPS)

sinclude $(DEPS)
