# Specify compiler 
CC = gcc

# FFmpeg library and include directories
FFMPEG_DIR = /opt/homebrew/Cellar/ffmpeg/5.1
INCLUDES = -I$(FFMPEG_DIR)/include
LIBS = -L$(FFMPEG_DIR)/lib -lavcodec -lavformat -lswscale -lavutil

# The converter source file
SRC = video_converter.c

# Compiler flags
CFLAGS = -Wall $(INCLUDES)

# The executable to generate
TARGET = converter

all: $(TARGET) 

$(TARGET): $(SRC) 
	$(CC) $(CFLAGS) $(SRC) $(LIBS) -o $(TARGET)

.PHONY: clean
clean:
	rm -f $(TARGET)

.PHONY: run
run:
	./$(TARGET) <input> <output> <in_fmt> <out_fmt>
