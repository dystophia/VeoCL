# VeoCL
Amoveo OpenCL miner

for AMD & Nvidia GPU´s

Donations:
BONJmlU2FUqYgUY60LTIumsYrW/c6MHte64y5KlDzXk5toyEMaBzWm8dHmdMfJmXnqvbYmlwim0hiFmYCCn3Rm0=

# Usage
Compile with "make"

run 
./clientnvidia <address> <poolip:poolport/work>

or
./clientamd <address> <poolip:poolport/work>

Make sure amoveo.cl is in current directory

# Compile errors

client.c:18:19: fatal error: CL/cl.h: No such file or directory

sudo apt install opencl-headers


Could not find libOpenCL.so

sudo ln -s /usr/lib/x86_64-linux-gnu/libOpenCL.so.1 /usr/lib/x86_64-linux-gnu/libOpenCL.so


