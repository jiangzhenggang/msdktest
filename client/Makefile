.PHONY=msdkt
CC := gcc
Q = @
RM := rm
CFLAG := -Wall -Werror -Wno-portability 
C_OBJ = $(wildcard *.c)
O_OBJ = $(patsubst %.c, %.o, ${C_OBJ})

DBG = 0

ifeq (1, $(DBG))
	CFLAG += -DDBG -g
else
	CFLAG += -O2
endif
msdkt:
	$Q $(CC) -c $(CFLAG) $(C_OBJ)
	$Q $(CC) $(CFLAG) $(O_OBJ) -o $@
	$Q $(RM) $(O_OBJ)

clean:
	$Q $(RM) -f msdkt
