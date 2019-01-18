# Makefile for the ttftps program
CC = g++
CFLAGS = -g -Wall
CCLINK = $(CC)
OBJS =  main.o ttftp.o
RM = rm -f
# Creating the  executable
ttftps: $(OBJS)
	$(CCLINK) -o ttftps $(OBJS)
# Creating the object files
main.o: main.cpp ttftp.h
ttftp.o: ttftp.cpp ttftp.h

# Cleaning old files before new make
clean:
	$(RM) $(TARGET) *.o *~ "#"* core.*
