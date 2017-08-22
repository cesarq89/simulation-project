# Define the machine object files for your program
OBJECTS = p4skk956.o
# Define your include file
INCLUDES = cs2123p4.h

# make for the executable
p4: ${OBJECTS}
	gcc -g -o p4 ${OBJECTS}

# Simple suffix rules for the .o
%.o: %.c ${INCLUDES}
	gcc -g -c $<

# Clean the .o files
clean:
	rm -f ${OBJECTS}
