set path=e:\tools\Ruby23-x64\bin\;c:\Python27\;%PATH%
set CL=/MP

call configure.bat -static -release -prefix %CD%\qtbase -platform win32-msvc2015 -opengl desktop -qt-zlib -qt-pcre -qt-libpng -qt-sql-sqlite -no-openssl -commercial -confirm-license -make libs -nomake tools -nomake examples -nomake tests -skip qtconnectivity -skip qt3d -skip qtactiveqt -skip qtandroidextras -skip qtcanvas3d -skip qtimageformats -skip qtlocation -skip qtmultimedia -skip qtquickcontrols -skip qtscript -skip qtsensors -skip qtserialbus -skip qtserialport -skip qtsvg -skip qtwayland -skip qtwebchannel -skip qtwebengine -skip qtwebsockets -skip qtx11extras -skip qtxmlpatterns >configure.log 2>&1

nmake>nmake.log 2>&1