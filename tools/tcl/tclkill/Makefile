ifeq ($(findstring Cygwin,$(shell uname -a)),Cygwin)
all: kill-windows-x86.dll
else
ifeq ($(findstring Linux,$(shell uname -a)),Linux)
all: kill-linux-x86.so
else
all:
endif
endif

kill-windows-x86.dll: kill-windows-x86.o
	gcc -shared -o $@ $^ -L. -ltclstub85

kill-windows-x86.o: kill.c
	gcc -c -o $@ -Ic:/Software/Tcl/include -DUSE_TCL_STUBS $<

kill-linux-x86.so: kill-linux-x86.o
	gcc -shared -o $@ $^ -L. -ltclstub8.4

kill-linux-x86.o: kill.c
	gcc -c -o $@ -I/h/fpilhofe/soft/build-linux/include -DUSE_TCL_STUBS $<

clean:
	rm -f *~ *.o core core.*

distclean: clean
	rm -f kill-windows-x86.dll kill-linux-x86.so
