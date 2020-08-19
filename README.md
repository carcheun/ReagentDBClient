Notes:
 

 
Cpprestsdk is statically linked
 
instructions from these links
[Source1]https://stackoverflow.com/questions/44905708/statically-linking-casablanca-cpprest-sdk
[Source2]https://stackoverflow.com/questions/56097412/how-to-statically-link-cpprest-without-dll-files-in-vs-project/57177759

Adjust and follow instructions for 32 bit versions, instead of 64 bit!

download vcpkg (https://github.com/microsoft/vcpkg). This is a package C and C++ library package manager. You will need to 'bootstrap' it. See the Quick Start.
install necessary dependencies for cpprestsdk: `vcpkg install --triplet x86-windows zlib openssl boost-system boost-date-time boost-regex boost-interprocess websocketpp brotli
download cpprestsdk from github (https://github.com/Microsoft/cpprestsdk)
generate a Visual Studio solution file (https://github.com/microsoft/cpprestsdk/wiki/How-to-build-for-Windows). I wanted to generate an x86 version, so I had to use the following command cmake ../Release -A win32 -DCMAKE_TOOLCHAIN_F  ILE=d:\jw\git\vcpkg\scripts\buildsystems\vcpkg.cmake.
open cpprestsdk.sln solution and do the following for Release and Debug configurations in the cpprest project:
change the configuration type to Static library
change the target file extension to .lib.
build debug and release versions.
I could then use the generated libraries in my solution.

add the cpprestsdk include directory to my project
add the cpprestsdk libraries to the linker Input
add the zlib and openssl libraries from the cpprestsdk packages directory.
add the libraries bcrypt.lib, winhttp.lib and crypt32.lib to the linker Input too (Statically linking Casablanca/CPPREST SDK)
you also need to add the preprocessor flag _NO_ASYNCRTIMP to the project where you use cpprestsdk.