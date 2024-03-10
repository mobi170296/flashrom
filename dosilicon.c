#include <string.h>
#include "flash.h"
#include "chipdrivers.h"
#include "programmer.h"
#include "spi.h"
#include <unistd.h>

#define PAGE_SIZE 2048 //bytes

/* COMMANDS */
#define PAGE_READ 0x13
#define GET_FEATURE 0x0F
#define SET_FEATURE 0x1F
#define READ_ROM_CACHE 0x0B
#define PROGRAM_LOAD 0x02
#define BLOCK_ERASE 0xD8

/* REGISTER */
#define BLOCK_LOCK_REGISTER 0xA0
#define OTP_REGISTER 0xB0
#define STATUS_REGISTER 0xC0
#define DRIVER_STRENGTH 0xD0

/* STATUS REGISTER BITS */
#define STATUS_OIP_BIT 0x01
#define STATUS_WEL_BIT 0x02
#define STATUS_ERASE_FAIL 0x04
#define STATUS_PROGRAM_FAIL 0x08
#define STATUS_ECC_STATUS 0x30

/* STATUS REGISTER OIP VALUES */
#define STATUS_OIP_READY 0x00
#define STATUS_OIP_BUSY 0x01

/* STATUS REGISTER ECC VALUES */
#define STATUS_ECC_NO_ERROR 0x00
#define STATUS_ECC_CORRECTED_ERROR 0x10
#define STATUS_ECC_NO_CORRECTED_ERROR 0x20
#define UNUSED __attribute__((unused))

static int UNUSED dosilicon_get_feature(struct flashctx *flash, uint8_t reg, uint8_t *value)
{
	uint8_t cmd[] = { GET_FEATURE, reg };
	int ret = spi_send_command(flash, sizeof(cmd), sizeof(*value), cmd, value);
	if (ret)
	{
		msg_gerr("Failed to get feature\n");
	}
	return ret;
}

static int UNUSED dosilicon_set_feature(struct flashctx *flash, uint8_t reg, uint8_t value)
{
	uint8_t cmd[] = { SET_FEATURE, reg, value };
	int ret = spi_send_command(flash, sizeof(cmd), 0, cmd, NULL);
	if (ret)
	{
		msg_gerr("Failed to set feature\n");
	}
	return ret;
}

/* Here is the function which read contents from the Dosilicon flash */
/*
	1. PAGE READ to cache
		ADDRESS: 8 dummy bits + 16 block/page bits
	2. GET FEATURE to read the status
		Check STATUS_OIP_READY/STATUS_OIP_BUSY on STATUS_OIP_BIT
	3. Random data read from cache
		ADDRESS: 3 dummy bits + 12 bits (0-2111)
*/
int spi_read_dosilicon(struct flashctx *flash, uint8_t *buf, unsigned int addr, unsigned int len)
{
	msg_ginfo("Reading flash rom %u %u...\n", addr, len);
	
	uint8_t cmd_read_page[] = {PAGE_READ, 0x00, 0x00, 0x00};
	uint8_t cmd_read_from_cache[] = {READ_ROM_CACHE, 0x00, 0x00, 0x00};
	uint8_t status;
	uint16_t page = 0x00;
	uint32_t bi = 0;
	uint8_t data[2112];
	uint32_t i;

	for (; page < 0xffff; page++)
	{
		if (spi_send_command(flash, sizeof cmd_read_page, 0, cmd_read_page, NULL))
		{
			printf("Cannot read page\n");
			return 1;
		}

		usleep(10);

		while (1)
		{
			if (dosilicon_get_feature(flash, STATUS_REGISTER, &status))
			{
				printf("Cannot get feature\n");
				return 1;
			}

			printf("S: %02X\n", status);

			if ((status & STATUS_OIP_BIT) == STATUS_OIP_READY)
			{
				break;
			}
		}

		memset(data, 0, sizeof data);

		if (spi_send_command(flash, sizeof cmd_read_from_cache, sizeof data, cmd_read_from_cache, data))
		{
			printf("Cannot read from cache\n");
			return 1;
		}

		for (i = 0; i < sizeof data; i++)
		{
			buf[bi++] = data[i];
		}

		printf("READ DONE PAGE: %04X\n", page);
	}

	return 0;
}
