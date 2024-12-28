# threedeestuff
Voxel engine playground with a decently fast mesher. Please dont compare me to ethan gore I know his mesher is faster

# How to run (requires cmake and gcc)
git clone <repo url>
cmake -S . -B build -G "Unix Makefiles" # if this errors just run `cmake -S . -B build`
cmake --build build -- -j <max thread count>
cd build
./threedeestuff # or .\threedeestuff.exe if on windows
