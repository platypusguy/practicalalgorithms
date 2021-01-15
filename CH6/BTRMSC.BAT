e:\msvc\bin\cl /c /Od /I"e:\msvc\include" /Zi /AS user.c
e:\msvc\bin\cl /c /Od /I"e:\msvc\include" /Zi /AS bt_new.c
e:\msvc\bin\cl /c /Od /I"e:\msvc\include" /Zi /AS bt_open.c
e:\msvc\bin\cl /c /Od /I"e:\msvc\include" /Zi /AS bt_data.c
e:\msvc\bin\cl /c /Od /I"e:\msvc\include" /Zi /AS bt_disk.c
e:\msvc\bin\link user.obj+bt_new.obj+bt_open.obj+bt_data.obj+bt_disk.obj,user.exe,,e:\msvc\lib\slibce.lib+e:\msvc\lib\oldnames.lib,;
