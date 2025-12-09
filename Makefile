CC = clang
TARGET = Fat32Format.exe
SRC_DIR = src
INCLUDE_DIR = include
RES_DIR = res
BUILD_DIR = build

SOURCES = $(SRC_DIR)/main.c $(SRC_DIR)/ui.c $(SRC_DIR)/drives.c $(SRC_DIR)/format.c
OBJECTS = $(BUILD_DIR)/main.o $(BUILD_DIR)/ui.o $(BUILD_DIR)/drives.o $(BUILD_DIR)/format.o

CFLAGS = -I$(INCLUDE_DIR) -D_UNICODE -DUNICODE -O2 -Wall
LDFLAGS = -lkernel32 -luser32 -lgdi32 -lshell32 -lcomctl32 -lole32 -Wl,/SUBSYSTEM:WINDOWS -Wl,/ENTRY:wWinMainCRTStartup

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

clean:
	if exist $(BUILD_DIR) rmdir /s /q $(BUILD_DIR)
	if exist $(TARGET) del $(TARGET)

rebuild: clean all

.PHONY: all clean rebuild