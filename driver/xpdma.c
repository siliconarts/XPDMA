//
// Created by user on 8/3/15.
//

#include <stddef.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>

#include "xpdma.h"
#include <stdio.h>
#include "../driver/xpdma_driver.h"

#define CTR_REG_OFFSET 0x00004000 // TODO: temporary. For tests only
#define CTR_REG_SIZE   (4<<10)    // 4 kB configuration memory

struct xpdma_t {
    int fd;
    int id;
};

static int gfd = -1; // global device file escriptor
static int gOpenCount = 0; // counter for opened devices


xpdma_t *xpdma_open(int id) 
{
    xpdma_t * device;
    if (id >= XPDMA_NUM_MAX)
        return NULL;

    device = (xpdma_t *)malloc(sizeof(xpdma_t));
    if (device == NULL)
        return NULL;

    if (gfd < 0) 
        gfd = open("/dev/" DEVICE_NAME, O_RDWR | O_SYNC);
    
    if (gfd < 0) {
        free(device);
        return NULL;
    }

    device->id = id;
    device->fd = gfd;
    gOpenCount++;

//    if (CRIT_ERR == ioctl(device->fd, IOCTL_RESET, &(device->id))) {
//        close(gfd);
//        gfd = -1;
//        free(device);
//        return NULL;
//    }

    return device;
}

void xpdma_close(xpdma_t * device) {
    gOpenCount--;
    //printf ("free DEVICE\n");
    if (device != NULL)
        free(device);

    if (gOpenCount == 0) {
        close(gfd);
        gfd = -1;
    }
    //printf ("end free DEVICE\n");
}

int xpdma_send(xpdma_t *fpga, void *data, unsigned int count, unsigned int addr)
{
    if ( addr % 4 )
        return -1;

    cdmaBuffer_t buffer = {fpga->id, data, count, addr};
    ioctl(fpga->fd, IOCTL_SEND, &buffer);
    return 0;
}

int xpdma_recv(xpdma_t *fpga, void *data, unsigned int count, unsigned int addr)
{
    if ( addr % 4 )
        return -1;

    cdmaBuffer_t buffer = {fpga->id, data, count, addr};
    ioctl(fpga->fd, IOCTL_RECV, &buffer);
    return 0;
}

void xpdma_writeReg(xpdma_t *fpga, uint32_t addr, uint32_t value)
{
    cdmaReg_t data;
    data.id = fpga->id;
    data.reg = addr;
    data.value = value;
    ioctl(fpga->fd, IOCTL_WRCDMAREG, &data);
}

uint32_t xpdma_readReg(xpdma_t *fpga, uint32_t addr)
{
    cdmaReg_t data;
    data.id = fpga->id;
    data.reg = addr;
    data.value = 0;
    ioctl(fpga->fd, IOCTL_RDCDMAREG, &data);
    return data.value;
}

/*void xpdma_read(xpdma_t *fpga, void *data, unsigned int count)
{
    read(fpga->fd, data, count);
}

void xpdma_write(xpdma_t *fpga, void *data, unsigned int count)
{
    write(fpga->fd, data, count);
}*/

void xpdma_test_sg(xpdma_t *fpga, void *data, unsigned int count)
{
    cdmaBuffer_t buffer;
    buffer.id = fpga->id;
    buffer.data = data;
    buffer.count = count;
    buffer.addr = 0x1;

    ioctl(fpga->fd, IOCTL_SEND, &buffer);
    ioctl(fpga->fd, IOCTL_RECV, &buffer);
}

void xpdma_info(xpdma_t *fpga)
{
    ioctl(fpga->fd, IOCTL_INFO, fpga->id);
}


void xpdma_setCfgReg(xpdma_t *fpga, uint32_t regNumber, uint32_t data)
{
    if (regNumber > CTR_REG_SIZE) {
        printf("setCfgReg: Wrong reg number :%08X!\n", regNumber);
        return;
    }
    xpdma_writeReg(fpga, regNumber*4 + CTR_REG_OFFSET, data);
}

uint32_t xpdma_getCfgReg(xpdma_t *fpga, uint32_t regNumber)
{
    if (regNumber > CTR_REG_SIZE) {
        printf("getCfgReg: Wrong reg number :%08X!\n", regNumber);
        return 0;
    }
    return xpdma_readReg(fpga, regNumber*4 + CTR_REG_OFFSET);
}
