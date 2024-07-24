BIN=\
  config-test\
  api-test\
  confedit-api\

all: $(BIN) schema.pdf

UTILDIR=../../idanlib/src
UTILSLIB=$(UTILDIR)/utils.a

LUADIR := $(shell $(UTILDIR)/lua-dir.sh)
LUALIB := $(shell $(UTILDIR)/lua-ver.sh)

INCLUDES=$(LUADIR) -I$(UTILDIR)
LIBS=$(UTILSLIB) -lm -lpthread -luuid $(LUALIB) -lcurl

CC=gcc
CFLAGS=-Wall -O2 -g $(INCLUDES)
# CFLAGS=-Wall -g -rdynamic -fsanitize=address $(INCLUDES)

SHAREDOBJ=\
  api.o\
  api-methods.o\
  run.o\
  document.o\
  config.o\
  variable.o\
  value.o\
  user.o\

ALLOBJ=\
  $(SHAREDOBJ)\

%.o: %.c $(wildcard *.h)
	$(CC) $(CFLAGS) -c $<

$(BIN): % : %.o $(ALLOBJ) $(UTILSLIB)
	$(CC) $(CFLAGS) -o $@ $< $(ALLOBJ) $(LIBS)

schema.pdf: schema.tex
	pdflatex schema

schema.tex: config-test config.ini
	./config-test -o schema.tex -doc

clean: NULL
	rm -f $(SHAREDOBJ) $(BIN)
	rm -f *.o

NULL:
