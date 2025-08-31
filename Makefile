CC:=gcc


FLAGS:=  -std=c2x -Wall -Wextra 

SRC:=$(wildcard *.c)

OBJECTS:=$(SRC:.c=.o)


BUILD_DIR:=build
OBJECTS:=$(addprefix $(BUILD_DIR)/,$(OBJECTS))

TARGET:=main
 
all: clean $(TARGET)

$(TARGET):$(OBJECTS)
			$(CC) -o $@ $^

$(BUILD_DIR)/%.o:%.c
			mkdir -p $(BUILD_DIR)
			$(CC) $(FLAGS) -c $< -o $@


clean:
	rm -rf $(BUILD_DIR) $(TARGET)