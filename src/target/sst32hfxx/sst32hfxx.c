/***************************************************************************
 *   Copyright (C) 2009 - 2010 by Simon Qian <SimonQian@SimonQian.com>     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>

#include "app_cfg.h"
#if TARGET_SST32HFXX_EN

#include "app_type.h"
#include "app_io.h"
#include "app_err.h"
#include "app_log.h"

#include "vsprog.h"
#include "interfaces.h"
#include "target.h"
#include "scripts.h"
#include "app_scripts.h"

#include "dal/mal/mal.h"
#include "dal/sst32hfxx/sst32hfxx_drv.h"

#include "sst32hfxx.h"

#define CUR_TARGET_STRING			SST32HFXX_STRING

struct program_area_map_t sst32hfxx_program_area_map[] =
{
	{APPLICATION_CHAR, 1, 0, 0, 0, AREA_ATTR_EWR | AREA_ATTR_EP},
	{0, 0, 0, 0, 0, AREA_ATTR_NONE}
};

const struct program_mode_t sst32hfxx_program_mode[] =
{
	{'*', "", IFS_EBI},
	{0, NULL, 0}
};

ENTER_PROGRAM_MODE_HANDLER(sst32hfxx);
LEAVE_PROGRAM_MODE_HANDLER(sst32hfxx);
ERASE_TARGET_HANDLER(sst32hfxx);
WRITE_TARGET_HANDLER(sst32hfxx);
READ_TARGET_HANDLER(sst32hfxx);
const struct program_functions_t sst32hfxx_program_functions =
{
	NULL,			// execute
	ENTER_PROGRAM_MODE_FUNCNAME(sst32hfxx),
	LEAVE_PROGRAM_MODE_FUNCNAME(sst32hfxx),
	ERASE_TARGET_FUNCNAME(sst32hfxx),
	WRITE_TARGET_FUNCNAME(sst32hfxx),
	READ_TARGET_FUNCNAME(sst32hfxx)
};

VSS_HANDLER(sst32hfxx_help)
{
	VSS_CHECK_ARGC(1);
	PRINTF("Usage of %s:"LOG_LINE_END, CUR_TARGET_STRING);
	PRINTF(LOG_LINE_END);
	return VSFERR_NONE;
}

const struct vss_cmd_t sst32hfxx_notifier[] =
{
	VSS_CMD(	"help",
				"print help information of current target for internal call",
				sst32hfxx_help,
				NULL),
	VSS_CMD_END
};






static struct sst32hfxx_drv_info_t sst32hfxx_drv_info;
static struct sst32hfxx_drv_param_t sst32hfxx_drv_param;
static struct sst32hfxx_drv_interface_t sst32hfxx_drv_ifs;
static struct mal_info_t sst32hfxx_mal_info =
{
	{0, 0}, NULL, 0, 0, 0, &sst32hfxx_nor_drv
};
static struct dal_info_t sst32hfxx_dal_info =
{
	&sst32hfxx_drv_ifs,
	&sst32hfxx_drv_param,
	&sst32hfxx_drv_info,
	&sst32hfxx_mal_info,
};

ENTER_PROGRAM_MODE_HANDLER(sst32hfxx)
{
	struct chip_param_t *param = context->param;
	struct chip_area_info_t *flash_info = NULL;
	struct program_info_t *pi = context->pi;
	uint8_t data_setup;
	
	if (pi->ifs_indexes != NULL)
	{
		if (dal_config_interface(SST32HFXX_STRING, pi->ifs_indexes, &sst32hfxx_dal_info))
		{
			return VSFERR_FAIL;
		}
	}
	else
	{
		sst32hfxx_drv_ifs.ebi_port = 0;
		sst32hfxx_drv_ifs.nor_index = 1;
	}
	
	sst32hfxx_drv_param.nor_info.common_info.data_width = 16;
	sst32hfxx_drv_param.nor_info.common_info.wait_signal = EBI_WAIT_NONE;
	sst32hfxx_drv_param.nor_info.param.addr_multiplex = false;
	sst32hfxx_drv_param.nor_info.param.timing.clock_hz_r = 
		sst32hfxx_drv_param.nor_info.param.timing.clock_hz_w = 0;
	sst32hfxx_drv_param.nor_info.param.timing.address_setup_cycle_r = 
		sst32hfxx_drv_param.nor_info.param.timing.address_setup_cycle_w = 2;
	sst32hfxx_drv_param.nor_info.param.timing.address_hold_cycle_r = 
		sst32hfxx_drv_param.nor_info.param.timing.address_hold_cycle_w = 0;
	// parameter 0 is the us delay of word programming
	sst32hfxx_drv_param.delayus = (uint16_t)param->param[0];
	sst32hfxx_drv_param.nor_info.common_info.mux_addr_mask = pi->chip_address;
	
	data_setup = 255;
	if (pi->param != NULL)
	{
		data_setup = (uint8_t)strtoul(pi->param, NULL, 0);
	}
	sst32hfxx_drv_param.nor_info.param.timing.data_setup_cycle_r = 
		sst32hfxx_drv_param.nor_info.param.timing.data_setup_cycle_w = data_setup;
	
	flash_info = target_get_chip_area(context->param, APPLICATION_IDX);
	if ((!sst32hfxx_mal_info.capacity.block_number ||
			!sst32hfxx_mal_info.capacity.block_size) &&
		(flash_info != NULL))
	{
		sst32hfxx_mal_info.capacity.block_size = flash_info->page_size;
		sst32hfxx_mal_info.capacity.block_number = flash_info->page_num;
	}
	if (mal.init(&sst32hfxx_dal_info) || 
		mal.getinfo(&sst32hfxx_dal_info))
	{
		return VSFERR_FAIL;
	}
	return dal_commit();
}

LEAVE_PROGRAM_MODE_HANDLER(sst32hfxx)
{
	REFERENCE_PARAMETER(context);
	REFERENCE_PARAMETER(success);
	
	mal.fini(&sst32hfxx_dal_info);
	return dal_commit();
}

ERASE_TARGET_HANDLER(sst32hfxx)
{
	struct chip_area_info_t *flash_info = NULL;
	
	switch (area)
	{
	case APPLICATION_CHAR:
		flash_info = target_get_chip_area(context->param, APPLICATION_IDX);
		if ((NULL == flash_info) || (size % flash_info->page_size))
		{
			return VSFERR_FAIL;
		}
		if (sst32hfxx_mal_info.erase_page_size)
		{
			size /= sst32hfxx_mal_info.erase_page_size;
		}
		else
		{
			size /= flash_info->page_size;
		}
		
		if (mal.eraseblock(&sst32hfxx_dal_info, addr, size))
		{
			return VSFERR_FAIL;
		}
		return dal_commit();
	default:
		return VSFERR_FAIL;
	}
}

WRITE_TARGET_HANDLER(sst32hfxx)
{
	struct chip_area_info_t *flash_info = NULL;
	
	switch (area)
	{
	case APPLICATION_CHAR:
		flash_info = target_get_chip_area(context->param, APPLICATION_IDX);
		if ((NULL == flash_info) || (size % flash_info->page_size))
		{
			return VSFERR_FAIL;
		}
		if (sst32hfxx_mal_info.write_page_size)
		{
			size /= sst32hfxx_mal_info.write_page_size;
		}
		else
		{
			size /= flash_info->page_size;
		}
		
		if (mal.writeblock(&sst32hfxx_dal_info,addr, buff, size))
		{
			return VSFERR_FAIL;
		}
		return dal_commit();
		break;
	default:
		return VSFERR_FAIL;
	}
}

READ_TARGET_HANDLER(sst32hfxx)
{
	struct chip_area_info_t *flash_info = NULL;
	
	switch (area)
	{
	case CHIPID_CHAR:
		if (mal.getinfo(&sst32hfxx_dal_info))
		{
			return VSFERR_FAIL;
		}
		buff[3] = 0;
		buff[2] = sst32hfxx_drv_info.manufacturer_id;
		buff[1] = (sst32hfxx_drv_info.device_id >> 8) & 0xFF;
		buff[0] = (sst32hfxx_drv_info.device_id >> 0) & 0xFF;
		return VSFERR_NONE;
		break;
	case APPLICATION_CHAR:
		flash_info = target_get_chip_area(context->param, APPLICATION_IDX);
		if ((NULL == flash_info) || (size % flash_info->page_size))
		{
			return VSFERR_FAIL;
		}
		if (sst32hfxx_mal_info.read_page_size)
		{
			size /= sst32hfxx_mal_info.read_page_size;
		}
		else
		{
			size /= flash_info->page_size;
		}
		
		if (mal.readblock(&sst32hfxx_dal_info, addr, buff, size))
		{
			return VSFERR_FAIL;
		}
		return VSFERR_NONE;
		break;
	default:
		return VSFERR_FAIL;
	}
}

#endif
