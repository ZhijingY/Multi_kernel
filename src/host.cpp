/**********
Copyright (c) 2018, Xilinx, Inc.
All rights reserved.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**********/

#define CL_HPP_CL_1_2_DEFAULT_BUILD
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#define CL_HPP_ENABLE_PROGRAM_CONSTRUCTION_FROM_ARRAY_COMPATIBILITY 1
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS

#define DATA_SIZE 4096

#include <vector>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <CL/cl2.hpp>

#include "xcl2.hpp"

size_t offset = 0;
size_t global = 1;
size_t local = 1;

// Forward declaration of utility functions included at the end of this file
std::vector<cl::Device> get_xilinx_devices();
char *read_binary_file(const std::string &xclbin_file_name, unsigned &nb);

// ------------------------------------------------------------------------------------
// Main program
// ------------------------------------------------------------------------------------

void event_cb(cl_event event1, cl_int cmd_status, void* data) {
    cl_int err;
    cl_command_type command;
    cl::Event event(event1, true);
    event.getInfo<cl_command_type>(CL_EVENT_COMMAND_TYPE, &command);
    cl_int status;
    event.getInfo<cl_int>(CL_EVENT_COMMAND_EXECUTION_STATUS, &status);

    const char* command_str;
    const char* status_str;
    switch (command) {
        case CL_COMMAND_READ_BUFFER:
            command_str = "buffer read";
            break;
        case CL_COMMAND_WRITE_BUFFER:
            command_str = "buffer write";
            break;
        case CL_COMMAND_NDRANGE_KERNEL:
            command_str = "kernel";
            break;
    }
    switch (status) {
        case CL_QUEUED:
            status_str = "Queued";
            break;
        case CL_SUBMITTED:
            status_str = "Submitted";
            break;
        case CL_RUNNING:
            status_str = "Executing";
            break;
        case CL_COMPLETE:
            status_str = "Completed";
            break;
    }
    printf("%s %s %s\n", status_str, reinterpret_cast<char*>(data), command_str);
    fflush(stdout);
}

void set_callback(cl::Event event, const char* queue_name) {
    cl_int err;
    event.setCallback(CL_COMPLETE, event_cb, (void*)queue_name);
}

int main(int argc, char **argv)
{
    // ------------------------------------------------------------------------------------
    // Step 1: Initialize the OpenCL environment
    // ------------------------------------------------------------------------------------
    cl_int err;
    std::string binaryFile = (argc != 2) ? "vadd.xclbin" : argv[1];
    unsigned fileBufSize;
    std::vector<cl::Device> devices = get_xilinx_devices();
    devices.resize(1);
    cl::Device device = devices[0];
    cl::Context context(device, NULL, NULL, NULL, &err);
    char *fileBuf = read_binary_file(binaryFile, fileBufSize);
    cl::Program::Binaries bins{{fileBuf, fileBufSize}};
    cl::Program program(context, devices, bins, NULL, &err);
    cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE, &err);
    cl::Kernel krnl_vector_add(program, "vadd", &err);
    cl::Kernel krnl_vector_mult(program, "vmult", &err);
    cl::Kernel krnl_vector_mult2(program, "vmult2", &err);

    // ------------------------------------------------------------------------------------
    // Step 2: Create buffers and initialize test values
    // ------------------------------------------------------------------------------------
    const size_t array_size = DATA_SIZE;
    std::vector<int, aligned_allocator<int> > in1(array_size);
    std::vector<int, aligned_allocator<int> > in2(array_size);
    std::vector<int, aligned_allocator<int> > in3(array_size);
    std::vector<int, aligned_allocator<int> > in4(array_size);
    std::vector<int, aligned_allocator<int> > out(array_size);

    // Initialize the vectors used in the test
    printf("Start assigning values\n");
    for (int i = 0; i < DATA_SIZE; i++)
    {
        in1.at(i) = rand() % DATA_SIZE;
        in2.at(i) = rand() % DATA_SIZE;
        in3.at(i) = 0;
        in4.at(i) = 0;
        out.at(i) = 0;
    }

    // Create the buffers and allocate memory
    printf("Start creating buffers\n");
    cl::Buffer in1_buf(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, sizeof(int) * DATA_SIZE, in1.data(), &err);
    cl::Buffer in2_buf(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, sizeof(int) * DATA_SIZE, in2.data(), &err);
    cl::Buffer in3_buf(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, sizeof(int) * DATA_SIZE, in3.data(), &err);
    cl::Buffer in4_buf(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, sizeof(int) * DATA_SIZE, in4.data(), &err);
    cl::Buffer out_buf(context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, sizeof(int) * DATA_SIZE, out.data(), &err);

    // Multi-kernel coding
    std::vector<cl::Event> kernel_events(3);

    // ------------------------------------------------------------------------------------
    // Step 3: Run the kernel
    // ------------------------------------------------------------------------------------
    // Set kernel arguments
    printf("Start setting args\n");
    krnl_vector_add.setArg(0, in1_buf);
    krnl_vector_add.setArg(1, in2_buf);
    krnl_vector_add.setArg(2, in3_buf);
    krnl_vector_add.setArg(3, DATA_SIZE);

    q.enqueueMigrateMemObjects({in1_buf, in2_buf}, 0 /* 0 means from host*/);
    q.enqueueNDRangeKernel(krnl_vector_add, offset, global, local, nullptr,
                                                             &kernel_events[0]);
    set_callback(kernel_events[0], "add");
    q.enqueueMigrateMemObjects({in3_buf}, CL_MIGRATE_MEM_OBJECT_HOST);

    krnl_vector_mult.setArg(0, in3_buf);
    krnl_vector_mult.setArg(1, in2_buf);
    krnl_vector_mult.setArg(2, in4_buf);
    krnl_vector_mult.setArg(3, DATA_SIZE);

    q.enqueueMigrateMemObjects({in3_buf, in2_buf}, 0 /* 0 means from host*/);
    q.enqueueNDRangeKernel(krnl_vector_mult, offset, global, local, nullptr,
                                                             &kernel_events[1]);
    set_callback(kernel_events[1], "mult");
    q.enqueueMigrateMemObjects({in4_buf}, CL_MIGRATE_MEM_OBJECT_HOST);

    krnl_vector_mult2.setArg(0, in4_buf);
    krnl_vector_mult2.setArg(1, in1_buf);
    krnl_vector_mult2.setArg(2, out_buf);
    krnl_vector_mult2.setArg(3, DATA_SIZE);

    q.enqueueMigrateMemObjects({in4_buf, in1_buf}, 0 /* 0 means from host*/);
    q.enqueueNDRangeKernel(krnl_vector_mult2, offset, global, local, nullptr,
                                                             &kernel_events[2]);
    set_callback(kernel_events[2], "mult2");
    q.enqueueMigrateMemObjects({out_buf}, CL_MIGRATE_MEM_OBJECT_HOST);

    // Wait for all scheduled operations to finish
    q.finish();

    // ------------------------------------------------------------------------------------
    // Step 4: Check Results and Release Allocated Resources
    // ------------------------------------------------------------------------------------
    bool match = true;
    for (int i = 0; i < DATA_SIZE; i++)
    {
        int expected = (in1[i] + in2[i]) * in2[i] * in1[i];
        if (out[i] != expected)
        {
            std::cout << "Error: Result mismatch" << std::endl;
            std::cout << "i = " << i << " CPU result = " << expected << " Device result = " << out[i] << std::endl;
            match = false;
            break;
        }
    }

    delete[] fileBuf;

    std::cout << "TEST " << (match ? "PASSED" : "FAILED") << std::endl;
    return (match ? EXIT_SUCCESS : EXIT_FAILURE);
}

// ------------------------------------------------------------------------------------
// Utility functions
// ------------------------------------------------------------------------------------
std::vector<cl::Device> get_xilinx_devices()
{
    size_t i;
    cl_int err;
    std::vector<cl::Platform> platforms;
    err = cl::Platform::get(&platforms);
    cl::Platform platform;
    for (i = 0; i < platforms.size(); i++)
    {
        platform = platforms[i];
        std::string platformName = platform.getInfo<CL_PLATFORM_NAME>(&err);
        if (platformName == "Xilinx")
        {
            std::cout << "INFO: Found Xilinx Platform" << std::endl;
            break;
        }
    }
    if (i == platforms.size())
    {
        std::cout << "ERROR: Failed to find Xilinx platform" << std::endl;
        exit(EXIT_FAILURE);
    }

    //Getting ACCELERATOR Devices and selecting 1st such device
    std::vector<cl::Device> devices;
    err = platform.getDevices(CL_DEVICE_TYPE_ACCELERATOR, &devices);
    return devices;
}

char *read_binary_file(const std::string &xclbin_file_name, unsigned &nb)
{
    if (access(xclbin_file_name.c_str(), R_OK) != 0)
    {
        printf("ERROR: %s xclbin not available please build\n", xclbin_file_name.c_str());
        exit(EXIT_FAILURE);
    }
    //Loading XCL Bin into char buffer
    std::cout << "INFO: Loading '" << xclbin_file_name << "'\n";
    std::ifstream bin_file(xclbin_file_name.c_str(), std::ifstream::binary);
    bin_file.seekg(0, bin_file.end);
    nb = bin_file.tellg();
    bin_file.seekg(0, bin_file.beg);
    char *buf = new char[nb];
    bin_file.read(buf, nb);
    return buf;
}
