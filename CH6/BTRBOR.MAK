# make file for b-tree routines
# Borland C v. 4.x, large model
#
# to use your files you need only put your
# path to the Borland libraries in the LP below

LP=c:\bc4\lib

# Meaning of compiler switches:
#       -c      compile only
#       -K      characters default to unsigned
#       -M      have the linker generate a map file
#       -Od     disable all optimizations
#       -v      source debugging on
#       -ml     large memory model
#
.c.obj:
     bcc -c -K -Od -v -ml $<

user.exe: user.obj bt_new.obj bt_open.obj bt_data.obj bt_disk.obj
  tlink /v $(LP)\c0l $* bt_new bt_open bt_data bt_disk,$*,,$(LP)\emu.lib+$(LP)\mathl.lib+$(LP)\cl.lib

user.obj:   user.c bt_hdr.h user.h

bt_new.obj: bt_new.c bt_hdr.h user.h

bt_open.obj: bt_open.c bt_hdr.h user.h

bt_data.obj: bt_data.c bt_hdr.h user.h

bt_disk.obj: bt_disk.c bt_hdr.h user.h

