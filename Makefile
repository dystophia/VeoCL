compileOptions = -O3 -lOpenCL -lpthread -Wno-unused-result -Wno-int-conversion -Wno-deprecated-declarations -Wno-incompatible-pointer-types
inputFiles = client.c sha256.c
dependencies = client.c Makefile sha256.c sha256.h

all:	clientnvidia clientamd

clientnvidia:	$(dependencies)
	gcc -o clientnvidia $(inputFiles) -D'CLWAIT=1' $(compileOptions)

clientamd:	$(dependencies)
	gcc -o clientamd $(inputFiles) $(compileOptions)

clean:
	rm clientnvidia clientamd
