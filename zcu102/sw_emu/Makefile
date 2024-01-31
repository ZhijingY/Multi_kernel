ndef = $(if $(value $(1)),,$(error $(1) must be set prior to running))
TARGET=sw_emu
all: package/sd_card.img

app.exe: ../../src/host.cpp
	$(call ndef,SDKTARGETSYSROOT)
	$(CXX) -Wall -g -std=c++11 ../../src/host.cpp -o app.exe \
		-I/usr/include/xrt \
		-lOpenCL -lpthread -lrt -lstdc++

vadd.xo: ../../src/vadd.cpp
	v++ -c -t ${TARGET} --config ../../src/zcu102.cfg -k vadd -I../../src ../../src/vadd.cpp -o vadd.xo 

vmult.xo: ../../src/vmult.cpp
	v++ -c -t ${TARGET} --config ../../src/zcu102.cfg -k vmult -I../../src ../../src/vmult.cpp -o vmult.xo 

vmult2.xo: ../../src/vmult2.cpp
	v++ -c -t ${TARGET} --config ../../src/zcu102.cfg -k vmult2 -I../../src ../../src/vmult2.cpp -o vmult2.xo 

vmulti.xclbin: ./vadd.xo ./vmult.xo ./vmult2.xo
	v++ -l -t ${TARGET} --config ../../src/zcu102.cfg ./vadd.xo ./vmult.xo ./vmult2.xo -o vmulti.xclbin

package/sd_card.img: app.exe emconfig.json vmulti.xclbin xrt.ini run_app.sh
	$(call ndef,ROOTFS)
	v++ -p -t ${TARGET} --config ../../src/zcu102.cfg ./vmulti.xclbin \
		--package.out_dir package \
		--package.rootfs ${ROOTFS}/rootfs.ext4 \
		--package.sd_file vmulti.xclbin \
		--package.sd_file ${ROOTFS}/Image \
		--package.sd_file xrt.ini \
		--package.sd_file emconfig.json \
		--package.sd_file app.exe \
		--package.sd_file run_app.sh

emconfig.json:
	emconfigutil --platform xilinx_zcu102_base_202110_1 --nd 1

clean:
	rm -rf vadd* vmult* vmulti* app.exe *json *csv *log *summary _x package *.json .run .Xil .ipcache *.jou

# Unless specified, use the current directory name as the v++ build target
TARGET ?= $(notdir $(CURDIR))
