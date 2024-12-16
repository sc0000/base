COMP_FLAGS = -Wall -Wextra -Wpedantic -Wconversion -Wno-unused-parameter

SRC_DIR = src/base
BUILD_DIR = base
INCLUDE_DIR = $(BUILD_DIR)/include/base
LIB_DIR = $(BUILD_DIR)/lib
LIB_FILE = libbase.a
O_FILES = allocators.o fileio.o log.o mem_utils.o

test:
	gcc $(COMP_FLAGS) -I src \
	test.c \
	$(SRC_DIR)/*.c \
	-o test

lib:
	gcc $(COMP_FLAGS) -O3 \
	-I src -c \
	$(SRC_DIR)/*.c &&\
	ar rcs $(LIB_FILE) $(O_FILES) && \
	mkdir -p $(LIB_DIR) && \
	mv libbase.a $(LIB_DIR) && \
	mkdir -p $(INCLUDE_DIR) && \
	cp src/base/*.h $(INCLUDE_DIR) && \
	make clean
	
clean:
	-rm *.o *.exe
	-rm -r base_logs

clean-build:
	-rm -r $(BUILD_DIR)
