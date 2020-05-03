mkdir build\msvc2017-x86
pushd build\msvc2017-x86
cmake -G "Visual Studio 15 2017" %* ../..
popd