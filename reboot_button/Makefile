MODULE=rbutton
 
CCPREFIX=/usr/bin/arm-linux-gnueabihf-
KERNEL_SRC=/usr/src/linux
 
obj-m += ${MODULE}.o
 
module_file=${MODULE}.ko
 
all: clean compile
 
compile:
	${MAKE} ARCH=arm CROSS_COMPILE=${CCPREFIX} -C ${KERNEL_SRC} M=$(PWD) modules
 
clean:
	${MAKE} -C ${KERNEL_SRC} M=$(PWD) clean
 
info:
	modinfo ${module_file}

