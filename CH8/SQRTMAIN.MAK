# make file for Newton's square root (full version)
# Borland C v. 4.0
#
# to use your files you need only put your
# path to your Borland libraries in the LIBPATH below

FILE1=sqrtmain
FILE2=longmath
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
#       -D      #define for MAX_SIZE
#
.c.obj:
     bcc -c -K -M -Od -v -ml -y -DMAX_SIZE=50 $<

$(FILE1).exe: $(FILE1).obj  $(FILE2).obj
#    tlink /v /s $(LIBPATH)\c0s $* $(FILE2), $*, , $(LIBPATH)\emu.lib+$(LIBPATH)\maths.lib+$(LIBPATH)\cs.lib
     tlink /v /s $(LIBPATH)\c0l $* $(FILE2), $*, , $(LIBPATH)\emu.lib+$(LIBPATH)\mathl.lib+$(LIBPATH)\cl.lib

$(FILE1).obj: $(FILE1).c $(FILE2).h

$(FILE2).obj: $(FILE2).c $(FILE2).h
