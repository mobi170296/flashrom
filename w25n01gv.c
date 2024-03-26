#include <string.h>
#include "chipdrivers.h"
#include <spi.h>

#define UNUSED __attribute__((unused))

#define PAGE_SIZE 2048

/* COMMANDS */
#define CMD_READ_ID 0x9F
#define CMD_READ_PAGE 0x13
#define CMD_READ_DATA 0x03
#define CMD_READ_STATUS 0x0F
#define CMD_WRITE_STATUS 0x1F
#define CMD_RESET 0xFF
#define CMD_WRITE_ENABLE 0x06
#define CMD_WRITE_DISABLE 0x04

/* REGISTERS */
#define REG_PROTECTION 0xA0 // SR1: SRP0 | BP3 | BP2 | BP1 | BP0 | TB | WP_E | SRP1
#define REG_CONFIGURATION 0xB0 // SR2: OTP_L | OTP_E | SR1_L | ECC_E | BUF | R | R | R
#define REG_STATUS 0xC0 // SR3: R | LUT_F | ECC_1 | ECC_0 | P_FAIL | E_FAIL | WEL | BUSY

/* MASK */
#define MASK_STATUS_BUSY_BIT 0x01

static UNUSED void dump_hex(uint8_t *mem, uint32_t size, uint32_t width)
{
    uint32_t i;
    for (i = 0; i < size; i++)
    {
        printf("%02X ", mem[i]);
        if (((i + 1) % width) == 0)
        {
            printf("\n");
        }
    }
}

int probe_spi_w25n01gv(struct flashctx *flash)
{
	uint8_t cmd[] = { CMD_READ_ID, 0x00 };
    uint8_t ids[3];
    uint32_t device_id;

    if (spi_send_command(flash, sizeof cmd, sizeof ids, cmd, ids))
    {
        printf("Cannot read ID\n");
        return 1;
    }

    device_id = (ids[1] << 8) | ids[2];

    if (flash->chip->manufacture_id == ids[0] && flash->chip->model_id == device_id)
    {
        return 1;
    }

	return 0;
}

static UNUSED int read_status(struct flashctx *flash, uint8_t reg, uint8_t *status)
{
    uint8_t cmd[] = { CMD_READ_STATUS, reg };

    if (spi_send_command(flash, sizeof cmd, sizeof *status, cmd, status))
    {
        printf("Cannot read status\n");
        return 1;
    }

    return 0;
}

static UNUSED int write_status(struct flashctx *flash, uint8_t reg, uint8_t status)
{
    uint8_t cmd[] = { CMD_WRITE_STATUS, reg, status };

    if (spi_send_command(flash, sizeof cmd, 0, cmd, NULL))
    {
        printf("Cannot write status\n");
        return 1;
    }

    return 0;
}

static UNUSED int enable_write(struct flashctx *flash)
{
    uint8_t cmd[] = { CMD_WRITE_ENABLE };

    if (spi_send_command(flash, sizeof cmd, 0, cmd, NULL))
    {
        return 1;
    }

    return 0;
}

static UNUSED int disable_write(struct flashctx *flash)
{
    uint8_t cmd[] = { CMD_WRITE_DISABLE };
    if (spi_send_command(flash, sizeof cmd, 0, cmd, NULL))
    {
        return 1;
    }

    return 0;
}

static UNUSED int reset_device(struct flashctx *flash)
{
    uint8_t cmd[] = { CMD_RESET };
    uint8_t status;

    if (spi_send_command(flash, sizeof cmd, 0, cmd, NULL))
    {
        printf("Cannot reset the device\n");
        return 1;
    }

    while (1)
    {
        if (read_status(flash, REG_STATUS, &status))
        {
            printf("Cannot read status\n");
            return 1;
        }

        if ((status & MASK_STATUS_BUSY_BIT) == 0x00)
        {
            break;
        }
    }

    return 0;
}

static UNUSED int read_page(struct flashctx *flash, uint16_t page, uint8_t *data, uint32_t size)
{
    uint8_t in_data[2112], status;
    uint8_t cmd_read_page[] = { CMD_READ_PAGE, 0x00, (page >> 8) & 0xff, page & 0xff };
    uint8_t cmd_read_data[] = { CMD_READ_DATA, 0x00, 0x00, 0x00 };

    memset(data, 0, size);
    memset(in_data, 0, sizeof in_data);

    if (spi_send_command(flash, sizeof cmd_read_page, 0, cmd_read_page, NULL))
    {
        printf("Cannot read page\n");
        return 1;
    }

    while (1)
    {
        if (read_status(flash, REG_STATUS, &status))
        {
            printf("Cannot read status\n");
            return 1;
        }

        if ((status & MASK_STATUS_BUSY_BIT) == 0x00)
        {
            break;
        }
    }

    if (spi_send_command(flash, sizeof cmd_read_data, sizeof in_data, cmd_read_data, in_data))
    {
        printf("Cannot read data\n");
        return 1;
    }

    memcpy(data, in_data, min(size, sizeof in_data));

    return 0;
}

int spi_w25n01gv_read(struct flashctx *flash, uint8_t *buf, unsigned int start, unsigned int len)
{
    msg_ginfo("\nReading flash rom %u %u...\n", start, len);
    uint32_t page;
    uint8_t data[PAGE_SIZE];

    for (page = 0; page <= 0xffff; page++)
    {
        if (read_page(flash, page, data, sizeof data))
        {
            printf("Cannot read page\n");
            return 1;
        }

        memcpy(buf, data, sizeof data);
        buf += sizeof data;

        // printf("Read page done: %04X\n", page);
    }

    return 0;
}
