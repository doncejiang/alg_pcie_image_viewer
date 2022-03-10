
#include <Windows.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <strsafe.h>
#include <stdint.h>
#include <SetupAPI.h>
#include <INITGUID.H>
#include <WinIoCtl.h>
//#include <AtlBase.h>
#include <io.h>
#include "xdma_public.h"
#include "xdma_drv.h"

#pragma comment(lib, "setupapi.lib")

#define C2H0_NAME "c2h_0"
#define H2C0_NAME "h2c_0"
#define USER_NAME "user"
#define EVENT0_NAME "event_0"
#define EVENT1_NAME "event_1"

#define MAX_BYTES_PER_TRANSFER 0x800000


static void* posix_memalign(void** buffer, size_t size, size_t alignment) {

    if(size == 0) {
        size = 4096;
    }

    if (alignment == 0) {
        SYSTEM_INFO sys_info;
        GetSystemInfo(&sys_info);
        alignment = sys_info.dwPageSize;
    }

    *buffer =  (BYTE*)_aligned_malloc(size, alignment);
    return *buffer;
}


static int get_devices(GUID guid, char* devpath, size_t len_devpath)
{

    SP_DEVICE_INTERFACE_DATA device_interface;
    PSP_DEVICE_INTERFACE_DETAIL_DATA dev_detail;
    DWORD index;
    HDEVINFO device_info;
    wchar_t tmp[256];
    device_info = SetupDiGetClassDevs((LPGUID)&guid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (device_info == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "GetDevices INVALID_HANDLE_VALUE\n");
        exit(-1);
    }

    device_interface.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    // enumerate through devices

    for (index = 0; SetupDiEnumDeviceInterfaces(device_info, NULL, &guid, index, &device_interface); ++index) {

        // get required buffer size
        ULONG detailLength = 0;
        if (!SetupDiGetDeviceInterfaceDetail(device_info, &device_interface, NULL, 0, &detailLength, NULL) && GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            fprintf(stderr, "SetupDiGetDeviceInterfaceDetail - get length failed\n");
            break;
        }

        // allocate space for device interface detail
        dev_detail = (PSP_DEVICE_INTERFACE_DETAIL_DATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, detailLength);
        if (!dev_detail) {
            fprintf(stderr, "HeapAlloc failed\n");
            break;
        }
        dev_detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        // get device interface detail
        if (!SetupDiGetDeviceInterfaceDetail(device_info, &device_interface, dev_detail, detailLength, NULL, NULL)) {
            fprintf(stderr, "SetupDiGetDeviceInterfaceDetail - get detail failed\n");
            HeapFree(GetProcessHeap(), 0, dev_detail);
            break;
        }

        StringCchCopy(tmp, len_devpath, dev_detail->DevicePath);
        wcstombs(devpath, tmp, 256);
        HeapFree(GetProcessHeap(), 0, dev_detail);
    }

    SetupDiDestroyDeviceInfoList(device_info);

    return index;
}


int xdma_open_device(struct pcie_trans* trans, int dev_id)
{

    memset(trans, 0, sizeof(struct pcie_trans));

    char device_path[MAX_PATH + 1] = "";
    char device_base_path[MAX_PATH + 1] = "";
    wchar_t device_path_w[MAX_PATH + 1];
    DWORD num_devices = get_devices(GUID_DEVINTERFACE_XDMA, device_base_path, sizeof(device_base_path));
    printf("Devices found: %d\n", num_devices);
    if (num_devices < 1)
    {
        printf("error\n");
    }
    // extend device path to include target device node (xdma_control, xdma_user etc)
    printf("Device base path: %s\n", device_base_path);
    strcpy_s(device_path, sizeof device_path, device_base_path);
    strcat_s(device_path, sizeof device_path, "\\");
    strcat_s(device_path, sizeof device_path, C2H0_NAME);
    printf("Device node: %s\n", C2H0_NAME);
    // open device file
    mbstowcs(device_path_w, device_path, sizeof(device_path));
    trans->c2h0_device = CreateFile(device_path_w, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (trans->c2h0_device == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "Error opening device, win32 error code: %ld\n", GetLastError());
        //
    }
    memset(device_path, 0, sizeof(device_path));
    strcpy_s(device_path, sizeof device_path, device_base_path);
    strcat_s(device_path, sizeof device_path, "\\");
    strcat_s(device_path, sizeof device_path, H2C0_NAME);
    printf("Device node: %s\n", H2C0_NAME);
    // open device file
    mbstowcs(device_path_w, device_path, sizeof(device_path));
    trans->h2c0_device = CreateFile(device_path_w, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (trans->h2c0_device == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "Error opening device, win32 error code: %ld\n", GetLastError());
        //
    }
    memset(device_path, 0, sizeof(device_path));
    strcpy_s(device_path, sizeof device_path, device_base_path);
    strcat_s(device_path, sizeof device_path, "\\");
    strcat_s(device_path, sizeof device_path, USER_NAME);
    printf("Device node: %s\n", USER_NAME);
    // open device file
    mbstowcs(device_path_w, device_path, sizeof(device_path));
    trans->user_device = CreateFile(device_path_w, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (trans->user_device == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "Error opening device, win32 error code: %ld\n", GetLastError());
        return -1;
    }
    memset(device_path, 0, sizeof(device_path));
    strcpy_s(device_path, sizeof device_path, device_base_path);
    strcat_s(device_path, sizeof device_path, "\\");
    strcat_s(device_path, sizeof device_path, EVENT0_NAME);
    printf("Device node: %s\n", EVENT0_NAME);
    // open device file
    mbstowcs(device_path_w, device_path, sizeof(device_path));
    trans->img_event_device[0] = CreateFile(device_path_w, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (trans->img_event_device[0] == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "Error opening device, win32 error code: %ld\n", GetLastError());
        return -1;
    }
    memset(device_path, 0, sizeof(device_path));
    strcpy_s(device_path, sizeof device_path, device_base_path);
    strcat_s(device_path, sizeof device_path, "\\");
    strcat_s(device_path, sizeof device_path, EVENT1_NAME);
    printf("Device node: %s\n", EVENT1_NAME);
    // open device file
    mbstowcs(device_path_w, device_path, sizeof(device_path));
    trans->img_event_device[1] = CreateFile(device_path_w, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (trans->img_event_device[1] == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "Error opening device, win32 error code: %ld\n", GetLastError());
        return -1;
    }

    for (int i = 0; i < VDMA_NUM; ++i) {
        posix_memalign((void **)&trans->read_img_buffer[i], 4096 , MAX_BYTES_PER_TRANSFER);
    }
    posix_memalign((void **)&trans->write_dev_buffer, 4096 , MAX_BYTES_PER_TRANSFER);
    posix_memalign((void **)&trans->read_dev_buffer, 4096 , MAX_BYTES_PER_TRANSFER);
    return 1;
}


int xdma_close_device(struct pcie_trans* trans, int dev_id)
{
    if (trans->user_device)
        CloseHandle(trans->user_device);
    for (int i = 0; i < VDMA_NUM; ++i) {
        if (trans->img_event_device[i])
            CloseHandle(trans->img_event_device[i]);
    }
    if (trans->c2h0_device)
        CloseHandle(trans->c2h0_device);
    if (trans->h2c0_device)
        CloseHandle(trans->h2c0_device);

    for (int i = 0; i < VDMA_NUM; ++i) {
        if (trans->read_img_buffer[i])
            _aligned_free(trans->read_img_buffer[i]);
    }
    if (trans->write_dev_buffer)
        _aligned_free(trans->write_dev_buffer);
    if (trans->read_dev_buffer)
        _aligned_free(trans->read_dev_buffer);
    return 0;
}


int xdma_read_device(HANDLE device, long address, DWORD size, BYTE* buffer)
{
    DWORD rd_size = 0;

    unsigned int transfers;
    int i;
    if (INVALID_SET_FILE_POINTER == SetFilePointer(device, address, NULL, FILE_BEGIN)) {
        fprintf(stderr, "Error setting file pointer, win32 error code: %ld\n", GetLastError());
        return -3;
    }
    transfers = (unsigned int)(size / MAX_BYTES_PER_TRANSFER);
    for (i = 0; i < transfers; i++)
    {
        if (!ReadFile(device, (void*)(buffer + i * MAX_BYTES_PER_TRANSFER), (DWORD)MAX_BYTES_PER_TRANSFER, &rd_size, NULL))
        {
            return -1;
        }
        if (rd_size != MAX_BYTES_PER_TRANSFER)
        {
            return -2;
        }
    }
    if (!ReadFile(device, (void*)(buffer + i * MAX_BYTES_PER_TRANSFER), (DWORD)(size - i * MAX_BYTES_PER_TRANSFER), &rd_size, NULL))
    {
        return -1;
    }
    if (rd_size != (size - i * MAX_BYTES_PER_TRANSFER))
    {
        return -2;
    }

    return size;
}

int xdma_write_device(HANDLE device, long address, DWORD size, BYTE* buffer)
{
    DWORD wr_size = 0;
    unsigned int transfers;
    int i;
    transfers = (unsigned int)(size / MAX_BYTES_PER_TRANSFER);
    //printf("transfers = %d\n",transfers);
    if (INVALID_SET_FILE_POINTER == SetFilePointer(device, address, NULL, FILE_BEGIN)) {
        fprintf(stderr, "Error setting file pointer, win32 error code: %ld\n", GetLastError());
        return -3;
    }
    for (i = 0; i < transfers; i++)
    {
        if (!WriteFile(device, (void*)(buffer + i * MAX_BYTES_PER_TRANSFER), MAX_BYTES_PER_TRANSFER, &wr_size, NULL))
        {
            return -1;
        }
        if (wr_size != MAX_BYTES_PER_TRANSFER)
        {
            return -2;
        }
    }
    if (!WriteFile(device, (void*)(buffer + i * MAX_BYTES_PER_TRANSFER), (DWORD)(size - i * MAX_BYTES_PER_TRANSFER), &wr_size, NULL))
    {
        return -1;
    }
    if (wr_size != (size - i * MAX_BYTES_PER_TRANSFER))
    {
        return -2;
    }
    return size;
}

static int seek_device(HANDLE device, long address)
{
    if (INVALID_SET_FILE_POINTER == SetFilePointer(device, address, NULL, FILE_BEGIN)) {
        fprintf(stderr, "Error setting file pointer, win32 error code: %ld\n", GetLastError());
        return -1;
    }
    return 0;
}


static void write_user(HANDLE device, long address, unsigned int val)
{
    xdma_write_device(device, address, 4, (BYTE*)&val);
}


