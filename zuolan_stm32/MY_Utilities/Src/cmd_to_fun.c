/**
 * @file cmd_to_fun.c
 * @brief Low-level FPGA control register helpers.
 */
#include "cmd_to_fun.h"

static u16 fpga_ctrl_shadow = 0;

static void CTRL_WRITE_SHADOW(void)
{
    CTRL_DATA = fpga_ctrl_shadow;
}

void CTRL_INIT(void)
{
    fpga_ctrl_shadow = 0;
    CTRL_WRITE_SHADOW();
}

void DA_FPGA_START(void)
{
    fpga_ctrl_shadow |= DA_FREQ_EN;
    CTRL_WRITE_SHADOW();
}

void DA_FPGA_STOP(void)
{
    fpga_ctrl_shadow &= (u16)(~DA_FREQ_EN);
    CTRL_WRITE_SHADOW();
}

void AD_FREQ_CLR_ENABLE(int ch)
{
    switch (ch) {
    case 1:
        fpga_ctrl_shadow &= (u16)(~AD1_FREQ_CLR);
        break;
    case 2:
        fpga_ctrl_shadow &= (u16)(~AD2_FREQ_CLR);
        break;
    default:
        break;
    }
    CTRL_WRITE_SHADOW();
}

void AD_FREQ_CLR_DISABLE(int ch)
{
    switch (ch) {
    case 1:
        fpga_ctrl_shadow |= AD1_FREQ_CLR;
        break;
    case 2:
        fpga_ctrl_shadow |= AD2_FREQ_CLR;
        break;
    default:
        break;
    }
    CTRL_WRITE_SHADOW();
}

void AD_FREQ_START(int ch)
{
    switch (ch) {
    case 1:
        fpga_ctrl_shadow |= AD1_FREQ_START;
        break;
    case 2:
        fpga_ctrl_shadow |= AD2_FREQ_START;
        break;
    default:
        break;
    }
    CTRL_WRITE_SHADOW();
}

void AD_FREQ_STOP(int ch)
{
    switch (ch) {
    case 1:
        fpga_ctrl_shadow &= (u16)(~AD1_FREQ_START);
        break;
    case 2:
        fpga_ctrl_shadow &= (u16)(~AD2_FREQ_START);
        break;
    default:
        break;
    }
    CTRL_WRITE_SHADOW();
}

void AD_FREQ_SET(int ch)
{
    switch (ch) {
    case 1:
        fpga_ctrl_shadow |= AD1_FREQ_EN;
        break;
    case 2:
        fpga_ctrl_shadow |= AD2_FREQ_EN;
        break;
    case 3:
        fpga_ctrl_shadow |= AD1_FREQ_EN | AD2_FREQ_EN;
        break;
    default:
        break;
    }
    CTRL_WRITE_SHADOW();
}

void AD_FIFO_WRITE_ENABLE(int ch)
{
    switch (ch) {
    case 1:
        fpga_ctrl_shadow |= AD1_FIFO_WR;
        break;
    case 2:
        fpga_ctrl_shadow |= AD2_FIFO_WR;
        break;
    case 3:
        fpga_ctrl_shadow |= AD1_FIFO_WR | AD2_FIFO_WR;
        break;
    default:
        break;
    }
    CTRL_WRITE_SHADOW();
}

void AD_FIFO_WRITE_DISABLE(int ch)
{
    switch (ch) {
    case 1:
        fpga_ctrl_shadow &= (u16)(~AD1_FIFO_WR);
        break;
    case 2:
        fpga_ctrl_shadow &= (u16)(~AD2_FIFO_WR);
        break;
    default:
        break;
    }
    CTRL_WRITE_SHADOW();
}

void AD_FIFO_READ_ENABLE(int ch)
{
    switch (ch) {
    case 1:
        fpga_ctrl_shadow |= AD1_FIFO_RD;
        break;
    case 2:
        fpga_ctrl_shadow |= AD2_FIFO_RD;
        break;
    default:
        break;
    }
    CTRL_WRITE_SHADOW();
}

void AD_FIFO_READ_DISABLE(int ch)
{
    switch (ch) {
    case 1:
        fpga_ctrl_shadow &= (u16)(~AD1_FIFO_RD);
        break;
    case 2:
        fpga_ctrl_shadow &= (u16)(~AD2_FIFO_RD);
        break;
    default:
        break;
    }
    CTRL_WRITE_SHADOW();
}
