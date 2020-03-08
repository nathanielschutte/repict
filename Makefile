CC=gcc
CFLAGS=-c -Wall -g -I src
SDIR=src
USESUPER=n # 'n' bin and obj left alone. 'y' put bin and obj in super dir
SUPERDIR=build
BDIR=bin
ODIR=obj
EXC=repict
MKDIR=mkdir -p

all: main rename $(BDIR) $(ODIR)
	mv *.o $(ODIR)
	mv $(EXC) $(BDIR)

rename:
ifeq ($(USESUPER),y)
	BDIR=$(SUPERDIR)/$(BDIR)
	ODIR=$(SUPERDIR)/$(ODIR)
endif

$(BDIR):
	$(MKDIR) $@

$(ODIR):
	$(MKDIR) $@

main: *.o
	$(CC) *.o -o $(EXC)
	
*.o: $(SDIR)/*.c
	$(CC) $(CFLAGS) $(SDIR)/*.c

clean:
ifeq ($(USESUPER),y)
	rm -r $(SUPERDIR)
else
	rm -r $(BDIR)
	rm -r $(ODIR)
endif

dirty:
	rm *.o
	rm *.exe