SRCDIR=src
BUILDIR=build

SOURCES= $(wildcard $(SRCDIR)/*.c) 
HEADERS= $(wildcard $(SRCDIR)/*.h)

CC=gcc
CFLAGS=-fno-builtin -static -Wall -Werror -MMD -Wundef -pthread -D _FILE_OFFSET_BITS=64
CFLAGS+=-O2 -DNDEBUG
#CFLAGS+=-g -O0 -fno-inline 

OBJDIR=$(BUILDIR) $(BUILDIR)/$(SRCDIR)

OBJS=$(addprefix $(BUILDIR)/, $(SOURCES:.c=.o))
DEPS=$(OBJS:.o=.d)

word_count: $(OBJDIR) $(OBJS) $(HEADERS)
	$(CC) $(CFLAGS) $(OBJS) -o $@

$(BUILDIR)/%.o: %.c 
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(OBJDIR)

$(OBJDIR):
	mkdir -p $@ 

print:
	echo $(SOURCES) $(HEADERS) $(OBJDIR)


-include $(DEPS)

