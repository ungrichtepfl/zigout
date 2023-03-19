SRC_DIR := .
INC_DIR := .
LIB_DIR := lib
OBJ_DIR := obj
BIN_DIR := bin

EXE := $(BIN_DIR)/cout
SRC := $(wildcard $(SRC_DIR)/*.c)
OBJ := $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# If RELEASE environment var is set to 1
ifeq ($(RELEASE),1)
	OPTFLAG := -O3
else 
	OPTFLAG := -DDEBUG 
endif
CPPFLAGS := -I$(INC_DIR) -MMD -MP
CFLAGS   := -Wall -Wextra -Wpedantic -Werror $(OPTFLAG)
LDFLAGS  := -L$(LIB_DIR) $(OPTFLAG)
SDL2LIB  := `sdl2-config --cflags --libs` -lSDL2_ttf
LDLIBS   := -lm $(SDL2LIB)


.PHONY: all
all: $(EXE)
	@echo 'Run "$^" to start game.'

.PHONY: run
run: $(EXE)
	@./$^

.PHONY: clean
clean:
	@$(RM) -rv $(BIN_DIR) $(OBJ_DIR)

# Linking:
$(EXE): $(OBJ) | $(BIN_DIR)
	@echo "Linking..."
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@
	

# Compiling:
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@echo "Compiling..."
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# Creat dirs:
$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

-include $(OBJ:.o=.d)
