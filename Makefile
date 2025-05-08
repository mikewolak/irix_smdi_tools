# Makefile for IRIX ASPI and SMDI Implementation with AIF support
# Targets IRIX 5.3 on MIPS 32-bit systems

# Directory structure
SRCDIR = src
INCDIR = include
OBJDIR = obj
BINDIR = bin

# Tools
CC = cc
RM = rm -f

# ANSI C Compilation flags
# -ansi: Ensures ANSI C compliance
# -fullwarn: Enable full warnings 
# -32: Generate 32-bit code
# -mips2: Use MIPS ISA 2 instructions
CFLAGS = -ansi -32 -mips2
INCLUDES = -I$(INCDIR)
LDFLAGS = -L/usr/lib32
LIBS = -lds -laudiofile -laudioutil -lm

# Output binaries
ASPI_TEST = $(BINDIR)/aspi_test
SMDI_TEST = $(BINDIR)/smdi_test

# Object files
ASPI_OBJS = $(OBJDIR)/scsi_debug.o $(OBJDIR)/aspi_irix.o $(OBJDIR)/aspi_test.o
SMDI_OBJS = $(OBJDIR)/scsi_debug.o $(OBJDIR)/aspi_irix.o \
            $(OBJDIR)/smdi_util.o $(OBJDIR)/smdi_core.o $(OBJDIR)/smdi_sample.o \
            $(OBJDIR)/smdi_aif.o $(OBJDIR)/smdi_test.o

# Default target
all: directories $(ASPI_TEST) $(SMDI_TEST)

# Create directories if they don't exist
directories:
	@if [ ! -d $(OBJDIR) ]; then mkdir -p $(OBJDIR); fi
	@if [ ! -d $(BINDIR) ]; then mkdir -p $(BINDIR); fi

# Compile ASPI source files
$(OBJDIR)/scsi_debug.o: $(SRCDIR)/scsi_debug.c $(INCDIR)/scsi_debug.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $(SRCDIR)/scsi_debug.c -o $(OBJDIR)/scsi_debug.o

$(OBJDIR)/aspi_irix.o: $(SRCDIR)/aspi_irix.c $(INCDIR)/aspi_irix.h $(INCDIR)/scsi_debug.h $(INCDIR)/aspi_defs.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $(SRCDIR)/aspi_irix.c -o $(OBJDIR)/aspi_irix.o

$(OBJDIR)/aspi_test.o: $(SRCDIR)/aspi_test.c $(INCDIR)/aspi_test.h $(INCDIR)/aspi_irix.h $(INCDIR)/scsi_debug.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $(SRCDIR)/aspi_test.c -o $(OBJDIR)/aspi_test.o

# Compile SMDI source files
$(OBJDIR)/smdi_util.o: $(SRCDIR)/smdi_util.c $(INCDIR)/smdi.h $(INCDIR)/aspi_irix.h $(INCDIR)/scsi_debug.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $(SRCDIR)/smdi_util.c -o $(OBJDIR)/smdi_util.o

$(OBJDIR)/smdi_core.o: $(SRCDIR)/smdi_core.c $(INCDIR)/smdi.h $(INCDIR)/aspi_irix.h $(INCDIR)/scsi_debug.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $(SRCDIR)/smdi_core.c -o $(OBJDIR)/smdi_core.o

$(OBJDIR)/smdi_sample.o: $(SRCDIR)/smdi_sample.c $(INCDIR)/smdi.h $(INCDIR)/smdi_sample.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $(SRCDIR)/smdi_sample.c -o $(OBJDIR)/smdi_sample.o

$(OBJDIR)/smdi_aif.o: $(SRCDIR)/smdi_aif.c $(INCDIR)/smdi.h $(INCDIR)/smdi_sample.h $(INCDIR)/smdi_aif.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $(SRCDIR)/smdi_aif.c -o $(OBJDIR)/smdi_aif.o

$(OBJDIR)/smdi_test.o: $(SRCDIR)/smdi_test.c $(INCDIR)/smdi.h $(INCDIR)/smdi_sample.h $(INCDIR)/smdi_aif.h $(INCDIR)/scsi_debug.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $(SRCDIR)/smdi_test.c -o $(OBJDIR)/smdi_test.o

# Link the executables
$(ASPI_TEST): $(ASPI_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(ASPI_OBJS) $(LIBS)

$(SMDI_TEST): $(SMDI_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SMDI_OBJS) $(LIBS)

# Single-step build (alternative for debugging)
aspi_single_build:
	@if [ ! -d $(BINDIR) ]; then mkdir -p $(BINDIR); fi
	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) -o $(ASPI_TEST) \
		$(SRCDIR)/scsi_debug.c $(SRCDIR)/aspi_irix.c $(SRCDIR)/aspi_test.c $(LIBS)

smdi_single_build:
	@if [ ! -d $(BINDIR) ]; then mkdir -p $(BINDIR); fi
	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) -o $(SMDI_TEST) \
		$(SRCDIR)/scsi_debug.c $(SRCDIR)/aspi_irix.c \
		$(SRCDIR)/smdi_util.c $(SRCDIR)/smdi_core.c $(SRCDIR)/smdi_sample.c \
		$(SRCDIR)/smdi_aif.c $(SRCDIR)/smdi_test.c $(LIBS)

# Clean up
clean:
	$(RM) $(OBJDIR)/*.o $(ASPI_TEST) $(SMDI_TEST)

# Install to /usr/local/bin
install: $(ASPI_TEST) $(SMDI_TEST)
	cp $(ASPI_TEST) /usr/local/bin/
	cp $(SMDI_TEST) /usr/local/bin/

.PHONY: all clean directories install aspi_single_build smdi_single_build
