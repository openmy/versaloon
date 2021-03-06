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

#include "compiler.h"

#include "interfaces.h"
#include "usbtoxxx.h"
#include "usbtoxxx_internal.h"

vsf_err_t usbtobdm_init(uint8_t index)
{
	return usbtoxxx_init_command(USB_TO_BDM, index);
}

vsf_err_t usbtobdm_fini(uint8_t index)
{
	return usbtoxxx_fini_command(USB_TO_BDM, index);
}

vsf_err_t usbtobdm_sync(uint8_t index, uint16_t *khz)
{
#if PARAM_CHECK
	if (index > 7)
	{
		LOG_BUG(ERRMSG_INVALID_INTERFACE_NUM, index);
		return VSFERR_FAIL;
	}
#endif
	
	return usbtoxxx_sync_command(USB_TO_BDM, index, NULL, 0, 2,
									(uint8_t *)khz);
}

vsf_err_t usbtobdm_transact(uint8_t index, uint8_t *out,
	uint8_t outlen, uint8_t *in, uint8_t inlen, uint8_t delay, uint8_t ack)
{
	uint16_t token;
	
#if PARAM_CHECK
	if (index > 7)
	{
		LOG_BUG(ERRMSG_INVALID_INTERFACE_NUM, index);
		return VSFERR_FAIL;
	}
	if ((outlen > 0x0F) || (inlen > 0x0F) || (NULL == out) || (delay > 3))
	{
		return VSFERR_FAIL;
	}
#endif
	
	token = outlen | (inlen << 8) | (delay << 6) | (ack ? 0x8000 : 0x0000);
	SET_LE_U16(&usbtoxxx_info->cmd_buff[0], token);
	memcpy(&usbtoxxx_info->cmd_buff[2], out, outlen);
	
	if (NULL == in)
	{
		return usbtoxxx_inout_command(USB_TO_BDM, index,
				usbtoxxx_info->cmd_buff, 2 + outlen, inlen, NULL, 0, 0, 1);
	}
	else
	{
		return usbtoxxx_inout_command(USB_TO_BDM, index,
				usbtoxxx_info->cmd_buff, 2 + outlen, inlen, in, 0, inlen, 1);
	}
}

