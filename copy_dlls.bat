rem Run this file to copy all required DLLs into the install\ folder after successful compilation.

copy ..\libxml2\bin\libxml2.dll install\
copy ..\gtk2\lib\iconv.dll install\
copy ..\gtk2\bin\libglib-2.0-0.dll install\
copy ..\gtk2\bin\libgobject-2.0-0.dll install\
copy ..\gtk2\bin\libgdk-win32-2.0-0.dll install\
copy ..\gtk2\bin\libgdk_pixbuf-2.0-0.dll install\
copy ..\gtk2\bin\libgmodule-2.0-0.dll install\
copy ..\gtk2\bin\intl.dll install\
copy ..\gtk2\bin\libcairo-2.dll install\
copy ..\gtk2\bin\libpng13.dll install\
copy ..\gtk2\bin\libpango-1.0-0.dll install\
copy ..\gtk2\bin\libpangocairo-1.0-0.dll install\
copy ..\gtk2\bin\libpangowin32-1.0-0.dll install\
copy ..\gtk2\bin\libgtk-win32-2.0-0.dll install\
copy ..\gtk2\bin\libatk-1.0-0.dll install\
copy ..\gtk2\lib\libgtkglext-win32-1.0-0.dll install\
copy ..\gtk2\lib\libgdkglext-win32-1.0-0.dll install\
copy ..\gtk2\bin\libpangoft2-1.0-0.dll install\
copy ..\freetype-dev_2.4.2-1_win32\bin\freetype6.dll install\
copy ..\fontconfig-dev_2.8.0-2_win32\bin\libfontconfig-1.dll install\
copy ..\expat_2.0.1-1_win32\bin\libexpat-1.dll install\

rem No idea yet how to handle this in a smart way, so be warned, could need manual adjusting ...
copy "C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\redist\x86\Microsoft.VC90.CRT\msvcr90.dll" install\
