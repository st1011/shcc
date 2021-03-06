COMPILER= gcc

CFLAGS=-Wall -std=c11
LDFLAGS	=

LIBS	=
INCLUDE	=

TGTDIR	= ./bin
TARGET	= $(TGTDIR)/$(shell basename `readlink -f .`)

SRCDIR	= ./src
SRCS	= $(wildcard $(SRCDIR)/*.c)

OBJDIR	= ./obj
OBJS	= $(addprefix $(OBJDIR)/, $(notdir $(SRCS:.c=.o)))
DEPENDS	= $(OBJS:.o=.d)

TESTDIR	= ./test

$(TARGET): $(OBJS) $(LIBS)
	mkdir -p $(TGTDIR)
	$(COMPILER) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	-mkdir -p $(OBJDIR)
	$(COMPILER) $(CFLAGS) $(INCLUDE) -o $@ -c $<

all: clean $(TARGET)

clean:
	rm -f $(OBJS) $(DEPENDS) $(TARGET)

test: all
	$(TARGET) -test
	$(TESTDIR)/test.sh

