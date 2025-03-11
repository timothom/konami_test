CC = gcc
CFLAGS = -Wall -Iinclude -O3
DEBUGFLAGS = -DDEBUG -O0 -g
SRC_DIR = src
BUILD_DIR = build
TARGET1 = server
TARGET2 = client
SOURCES1 = $(SRC_DIR)/server.c
SOURCES2 = $(SRC_DIR)/client.c
OBJECTS1 = $(SOURCES1:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
OBJECTS2 = $(SOURCES2:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

all: $(BUILD_DIR)/$(TARGET1) $(BUILD_DIR)/$(TARGET2)

$(BUILD_DIR)/$(TARGET1): $(OBJECTS1)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD_DIR)/$(TARGET2): $(OBJECTS2)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

debug: CFLAGS += $(DEBUGFLAGS)
debug: all

clean:
	rm -rf $(BUILD_DIR)