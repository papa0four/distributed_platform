
CC 			:= gcc

TARGET 		:= scheduler

#The Directories, Source, Includes, Objects, Binary and Resources
SRCDIR		:= src/scheduler/
INCDIR		:= src/scheduler/include
TARGETDIR	:= bin
BUILDDIR    := objects
SRCEXT      := c
HDREXT		:= h
DEPEXT      := d
OBJEXT      := o

#Flag and libraries
CFLAGS		:= -g -Wall -std=c11
LIB			:= -lm -lpthread -pthread

SOURCES     := $(shell find $(SRCDIR) -type f -name *.$(SRCEXT))
OBJECTS     := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.$(OBJEXT)))

all: directories $(TARGET)

directories:
	@mkdir -p $(TARGETDIR)
	@mkdir -p $(BUILDDIR)

-include $(OBJECTS:.$(OBJEXT)=.$(DEPEXT))

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(CFLAGS) $(LIB) -o $(TARGETDIR)/$(TARGET)

clean:
	-rm $(SRCDIR)*.o
	-rm $(TARGETDIR)/*
	-rm $(BUILDDIR)/*

lint:
	clang-tidy src/*.c src/include/*.h

$(BUILDDIR)/%.$(OBJEXT): $(SRCDIR)/%.$(SRCEXT)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<
	@$(CC) $(CFLAGS) $(INCDEP) -MM $(SRCDIR)/$*.$(SRCEXT) > $(BUILDDIR)/$*.$(DEPEXT)
	@cp -f $(BUILDDIR)/$*.$(DEPEXT) $(BUILDDIR)/$*.$(DEPEXT).tmp
	@sed -e 's|.*:|$(BUILDDIR)/$*.$(OBJEXT):|' < $(BUILDDIR)/$*.$(DEPEXT).tmp > $(BUILDDIR)/$*.$(DEPEXT)
	@sed -e 's/.*://' -e 's/\\$$//' < $(BUILDDIR)/$*.$(DEPEXT).tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $(BUILDDIR)/$*.$(DEPEXT)
	@rm -f $(BUILDDIR)/$*.$(DEPEXT).tmp

.PHONY: all clean directories