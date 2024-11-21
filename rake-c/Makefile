
PROJECT = rake-c
HEADERS = $(PROJECT).h strsplit.h protocol.h
OBJ = rake-c.o \
        strsplit.o \
        parsing.o \
        networking.o \
        packet.o

C11 = cc -std=c99
CFLAGS = -Wall -pedantic -Werror

$(PROJECT) : $(OBJ)
	$(C11) $(CFLAGS) -o $(PROJECT) $(OBJ)
    
%.o : %.c $(HEADERS)
	$(C11) $(CFLAGS) -c $<

clean:
	rm -f $(PROJECT) $(OBJ)