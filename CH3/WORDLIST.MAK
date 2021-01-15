# make file for worlist.c (listing 3-4) using elfhash (listing 3-3)
# Borland C v. 4.0, large model
#
# to use your files you need only put your
# path to the Borland libraries in the LIBPATH below

FILE1=wordlist
FILE2=llgen
FILE3=elfhash
FILE4=llapp
LIBPATH=c:\bc4\lib

# Meaning of compiler switches:
#       -c      compile only
#       -K      characters default to unsigned
#       -M      have the linker generate a map file
#       -Od     disable all optimizations
#       -v      source debugging on
#       -ms     small memory model or
#       -ml     large memory model
#       -y      line numbers on
#
.c.obj:
     bcc -c -K -M -Od -v -ml -y $<

$(FILE1).exe: $(FILE1).obj  $(FILE2).obj  $(FILE3).obj   $(FILE4).obj
     tlink /v $(LIBPATH)\c0l $* $(FILE2) $(FILE3) $(FILE4), $*, , $(LIBPATH)\emu.lib+$(LIBPATH)\mathl.lib+$(LIBPATH)\cl.lib

$(FILE1).obj: $(FILE1).c llgen.h llapp.h

$(FILE2).obj: $(FILE2).c llgen.h

$(FILE3).obj: $(FILE3).c

$(FILE4).obj: $(FILE4).c llgen.h llapp.h
