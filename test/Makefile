# This is a project made within Seoul National University of Science and Technology
# Embedded Systems Laboratory 2018 - Raimarius Tolentino Delgado
# #####################################################################################################

CUR_DIR = .
TOP_DIR = ..

INC_POSIX=$(TOP_DIR)/include
LIB_POSIX=$(TOP_DIR)/lib

CFLAGS=-c -Wall -I$(INC_POSIX) -O0 --coverage
LDFLAGS=-lgtest -lpthread -L$(LIB_POSIX) -lrtposix

OBJECTS = UnitTest.o
TARGET = UnitTest

START	= start

CC=g++

all: $(START)
	@echo BUILD DONE.
	@chmod +x $(START).sh

$(START): $(TARGET)
	@printf "#!/bin/bash \n" > $(START).sh
	@printf "## This is a project made within Seoul National University of Science and Technology \n" >> $(START).sh
	@printf "## Embedded Systems Laboratory 2018 - Raimarius Tolentino Delgado \n\n" >> $(START).sh
	@printf "## Start-up for dynamically linked executable file \n\n" >> $(START).sh
ifeq ($(RT_DOMAIN),xenomai)
	@printf "export LD_LIBRARY_PATH=$$LD_LIBRARY_PATH:$(XENOMAI_PATH)/lib:$(LIB_AIDE) \n" >> $(START).sh
else
	@printf "export LD_LIBRARY_PATH=$$LD_LIBRARY_PATH:$(LIB_POSIX) \n" >> $(START).sh
endif
	@printf "cur_dir=.\n\n" >> $(START).sh
	@printf "if [[ -x \$$1 ]]\n" >> $(START).sh
	@printf "then\n\t \$${cur_dir}/\$$1 \$$2 \$$3\n" >> $(START).sh
	@printf "else\n\t echo run with executable file\n" >> $(START).sh
	@printf "fi\n" >> $(START).sh


$(TARGET): $(OBJECTS)
	$(CC) --coverage -o $(TARGET) $(OBJECTS) $(LDFLAGS)

%.o : %.cpp
	$(CC) $(CFLAGS) $<

clean:
	rm -f $(TARGET) $(OBJECTS) $(CUR_DIR)/*.gcno $(CUR_DIR)/*.gcda *.sh

.PHONY: all clean
