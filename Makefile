BINDIR=bin

all : $(BINDIR)/asciz

clean :
	git clean -fdX

distclean :
	git clean -fdx

$(BINDIR)/asciz : asciz.c
	@test -d $(BINDIR) || mkdir $(BINDIR)
	rm -f $(BINDIR)/asciz
	gcc $(CFLAGS) -o $(BINDIR)/asciz asciz.c
