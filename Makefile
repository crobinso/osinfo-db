CC = gcc
LD = ld
SRCDIR = ./src
BINDIR = ./bin
INCDIR = ./inc
TESTDIR = ./test
DATADIR = ./data
LD_LIBRARY_PATH = $(shell echo $LD_LIBRARY_PATH):$(shell pwd)/$(BINDIR)
DATA_PATH = $(shell pwd)/$(DATADIR)

INCLUDES = -I ./$(SRCDIR) -I ./$(INCDIR) \
	`xml2-config --cflags --libs`
FLAGS = -g3 -fPIC
SONAME = libosinfo.so
OBJS = osi_hv.o osi_common.o osi_dataread.o osi_device.o osi_filter.o \
	osi_lib.o osi_os.o
OUT_OBJS = $(addprefix $(BINDIR)/,$(OBJS))
TESTS = sample test-filter test-get_devices test-get_os \
	test-manipulate_devices test-manipulate_hypervisor \
	test-manipulate_library test-manipulate_os test-set_hypervisor \
	test-skeleton
IN_TESTS = $(addsuffix .c, $(addprefix $(SRCDIR)/,$(TESTS)))
OUT_TESTS = $(addprefix $(BINDIR)/,$(TESTS))

lib: $(BINDIR)/$(SONAME)

$(BINDIR)/$(SONAME) : $(OUT_OBJS)
	mv $(OBJS) $(BINDIR)/.
	$(CC) -shared -Wl,-soname,$(SONAME) -o $(BINDIR)/$(SONAME) $(OUT_OBJS)

$(BINDIR)/%.o : $(SRCDIR)/%.c
	$(CC) $(FLAGS) $(INCLUDES) -c $^

clean:
	rm -f $(BINDIR)/*

test: lib $(OUT_TESTS)

$(BINDIR)/% : $(TESTDIR)/%.c
	$(CC) $(FLAGS) $(INCLUDES) -L $(BINDIR) -losinfo $^ -o $@
