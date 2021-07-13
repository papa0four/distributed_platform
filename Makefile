CC				:= gcc

TARGET			:= scheduler

#The directories, src, includes, objects, binary, and resources
SRCDIR			:= src/scheduler
INCDIR			:= src/includes
TARGETDIR		:= bin
BUILDDIR		:= objects
SRCEXT			:= c
HDREXT			:= h
DEPEXT			:= d
OBJEXT			:= o

#flags and libraries
CFLAGS			:= -g -Wall -Wextra -Werror -Wpedantic -std=c11
LIB				:= -lm -lpthread -pthread

SOURCES			:= $(shell find $(SRCDIR) -type f -name *.$(SRCEXT))
OBJECTS			:= $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.$(OBJEXT)))

all: directories $(TARGET)

directories:
	@mkdir -p $(TARGETDIR)
	@mkdir -p $(BUILDDIR)

-include $(OBJECTS:.$(OBJEXT)=.$(DEPEXT))

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(CFLAGS) $(LIB) -o $(TARGETDIR)/$(TARGET)

clean:
	-rm objects/*
	-rm bin/*

lint:
	clang-tidy src/*.c src/include/*.h

$(BUILDDIR)/%.$(OBJEXT): $(SRCDIR)/%.$(SRCEXT)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<
	@$(CC) $(CFLAGS) $(INCDEP) -MM $(SRCDIR)/$*.$(SRCEXT) > $(BUILDDIR)/$*.$(DEPEXT)
	@cp -f $(BUILDDIR)/$*.$(DEPEXT) $(BUILDDIR)/$*.$(DEPEXT).tmp
	@cp -f $(BUILDDIR)/$*.$(OBJEXT) $(BUILDDIR)/$*.$(OBJEXT).tmp
	@sed -e 's|.*:|$(BUILDDIR)/$*.$(OBJEXT):|' < $(BUILDDIR)/$*.(DEPEXT).tmp > $(BUILDDIR)/$*.$(DEPEXT)
	@sed -e 's/.*://' -e 's/\\$$//' < $(BUILDDIR)/$*.$(DEPEXT).tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $(BUILDDIR)/$*.$(DEPEXT)
	@rm -f $(BUILDDIR)/$*.$(DEPEXT).tmp

.PHONY: all clean directories