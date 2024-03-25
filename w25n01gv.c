#include "chipdrivers.h"
#include <spi.h>

#define UNUSED __attribute__((unused))

/* COMMANDS */
#define CMD_READ_ID 0x9F
#define CMD_READ_PAGE 0x13
#define CMD_READ_DATA 0x03

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

static UNUSED int read_status(struct flashctx *flash, uint8_t reg)
{
    return 0;
}

static UNUSED int read_page(struct flashctx *flash, uint16_t page, uint8_t *data, uint32_t size)
{
    return 0;
}

int spi_w25n01gv_read(struct flashctx *flash, uint8_t *buf, unsigned int start, unsigned int len)
{
    

    return 0;
}
