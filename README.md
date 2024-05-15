sudo ifconfig enx000ec6838bb0 10.10.7.2 netmask 255.0.0.0

scp -r  package/sd_card/* root@10.10.7.1:/media/sd-mmcblk0p1/

cd /media/sd-mmcblk0p1

export XILINX_XRT=/usr

./app.exe v

scp root@10.10.7.1:/media/sd-mmcblk0p1/xclbin.run_summary

scp root@10.10.7.1:/media/sd-mmcblk0p1/opencl_trace.csv

scp root@10.10.7.1:/media/sd-mmcblk0p1/opencl_summary.csv

vitis_analyzer xclbin.run_summary

