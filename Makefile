
SRC := $(wildcard src/*.c)
OBJ := $(SRC:.c=.o)
DEP := $(SRC:.c=.d)
COLIB := colib/colib.so

CC := gcc -std=c99
# setting include path for lua
INC :=
# INC := -I../lua/lua-5.4.2/src
CFLAGS := -Wall -O2 -fPIC $(INC)
LDFLAGS := -shared
LDLIBS := -lm

all: $(COLIB)
	@echo "done"

$(COLIB): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

ifneq ($(MAKECMDGOALS),clean)
-include $(DEP)
endif

%.d: %.c
	@echo "make depend: $@"
	@set -e; rm -f $@; \
	$(CC) $(CFLAGS) -MM $< -MT "$*.o" | sed -E 's,($*).o[: ]*,\1.o $@: ,g' > $@

.PHONY : clean
clean :
	rm -f $(COLIB) $(OBJ) $(DEP)