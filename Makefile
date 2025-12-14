UNAME_S := $(shell uname -s)

INCLUDE_DIR = include
BUILD_DIR = build

ifeq ($(UNAME_S),Linux)
    CC = gcc
    TARGET = fttf
    SRC_DIR = src/Linux
    SOURCES = $(SRC_DIR)/main.c $(SRC_DIR)/args.c $(SRC_DIR)/drives.c $(SRC_DIR)/format.c
    OBJECTS = $(BUILD_DIR)/main.o $(BUILD_DIR)/args.o $(BUILD_DIR)/drives.o $(BUILD_DIR)/format.o
    CFLAGS = -I$(INCLUDE_DIR) -O2 -Wall -std=c99
    LDFLAGS = 
else
    CC = clang
    TARGET = Fat32Format.exe
    SRC_DIR = src
    RES_DIR = res
    SOURCES = $(SRC_DIR)/main.c $(SRC_DIR)/ui.c $(SRC_DIR)/drives.c $(SRC_DIR)/format.c
    OBJECTS = $(BUILD_DIR)/main.o $(BUILD_DIR)/ui.o $(BUILD_DIR)/drives.o $(BUILD_DIR)/format.o
    CFLAGS = -I$(INCLUDE_DIR) -D_UNICODE -DUNICODE -O2 -Wall
    LDFLAGS = -lkernel32 -luser32 -lgdi32 -lshell32 -lcomctl32 -lole32 -Wl,/SUBSYSTEM:WINDOWS -Wl,/ENTRY:wWinMainCRTStartup
endif

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
ifeq ($(UNAME_S),Linux)
	mkdir -p $(BUILD_DIR)
else
	if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)
endif

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

clean:
ifeq ($(UNAME_S),Linux)
	rm -rf $(BUILD_DIR)
	rm -f $(TARGET)
else
	if exist $(BUILD_DIR) rmdir /s /q $(BUILD_DIR)
	if exist $(TARGET) del $(TARGET)
endif

rebuild: clean all

install:
ifeq ($(UNAME_S),Linux)
	@if [ -w /usr/local/bin ]; then \
		cp $(TARGET) /usr/local/bin/$(TARGET); \
		chmod 755 /usr/local/bin/$(TARGET); \
	else \
		sudo cp $(TARGET) /usr/local/bin/$(TARGET); \
		sudo chmod 755 /usr/local/bin/$(TARGET); \
	fi
	@echo "Installed $(TARGET) to /usr/local/bin"
else
	@echo "Install target is only available on Linux"
endif

uninstall:
ifeq ($(UNAME_S),Linux)
	@if [ -w /usr/local/bin/$(TARGET) ]; then \
		rm -f /usr/local/bin/$(TARGET); \
	else \
		sudo rm -f /usr/local/bin/$(TARGET); \
	fi
	@echo "Uninstalled $(TARGET) from /usr/local/bin"
else
	@echo "Uninstall target is only available on Linux"
endif

.PHONY: all clean rebuild install uninstall