@rem: Watcom 10.0 using built-in Rational DOS Extender
@rem
wcl386            -d2 -i=D:\wat10\h -4r -l=dos4g -fe=binary.exe bintree.c
wcl386 -dREDBLACK -d2 -i=D:\wat10\h -4r -l=dos4g -fe=redblk.exe bintree.c
wcl386 -dSPLAY    -d2 -i=D:\wat10\h -4r -l=dos4g -fe=splay.exe  bintree.c
