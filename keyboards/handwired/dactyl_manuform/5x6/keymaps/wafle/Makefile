GTEST_DIR = $(shell git rev-parse --show-toplevel)/lib/googletest/googletest

G++ = g++
G++_FLAGS = -c -Wall -I $(GOOGLE_TEST_INCLUDE) -I tests/ -I .
LD_FLAGS = -L /usr/local/lib -l $(GOOGLE_TEST_LIB) -l pthread

TESTS = vim_test

CPPFLAGS += -isystem $(GTEST_DIR)/include

# Flags passed to the C++ compiler.
CXXFLAGS += -g -Wall -Wextra -pthread

GTEST_HEADERS = $(GTEST_DIR)/include/gtest/*.h \
                $(GTEST_DIR)/include/gtest/internal/*.h

GTEST_SRCS_ = $(GTEST_DIR)/src/*.cc $(GTEST_DIR)/src/*.h $(GTEST_HEADERS)

all : $(TESTS)

clean :
	rm -f $(TESTS) gtest.a gtest_main.a *.o

gtest-all.o : $(GTEST_SRCS_)
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) -c \
            $(GTEST_DIR)/src/gtest-all.cc

gtest_main.o : $(GTEST_SRCS_)
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) -c \
            $(GTEST_DIR)/src/gtest_main.cc

gtest.a : gtest-all.o
	$(AR) $(ARFLAGS) $@ $^

gtest_main.a : gtest-all.o gtest_main.o
	$(AR) $(ARFLAGS) $@ $^

vim_test.o : tests/vim_test.cpp $(GTEST_HEADERS) vim.h tests/quantum.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c tests/vim_test.cpp -I . -I tests/


vim_test : vim_test.o gtest_main.a
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -lpthread $^ -o $@
.PHONY: all clean
