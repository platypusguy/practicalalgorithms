e:\msvc\bin\cl /Od            /I"e:\msvc\include" /Zi /Feb.exe bintree.c /link e:\msvc\lib\slibce.lib e:\msvc\lib\oldnames.lib
e:\msvc\bin\cl /Od /DREDBLACK /I"e:\msvc\include" /Zi /Fer.exe bintree.c /link e:\msvc\lib\slibce.lib e:\msvc\lib\oldnames.lib
e:\msvc\bin\cl /Od /DSPLAY    /I"e:\msvc\include" /Zi /Fes.exe bintree.c /link e:\msvc\lib\slibce.lib e:\msvc\lib\oldnames.lib
ren b.exe binary.exe
ren r.exe redblk.exe
ren s.exe splay.exe
