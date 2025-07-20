 x86_64-w64-mingw32-windres "Visual Icon Maker/icon.rc" -O coff -o icon.res.o

x86_64-w64-mingw32-g++ main.cpp tinyfiledialogs.c icon.res.o \
  -o IconMaker.exe \
  -Iraylib/include -Lraylib/lib \
  -lraylib -lopengl32 -lgdi32 -lwinmm -lole32 -luuid -lcomdlg32 -static


cd "Visual Icon Maker"
zip -r "Visual Icon Maker.zip" ./*