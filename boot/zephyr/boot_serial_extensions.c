/*
 * Copyright (c) 2021-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "bootutil/bootutil_log.h"
#include "../boot_serial/src/boot_serial_priv.h"
#include <zcbor_encode.h>
#include <boot_serial/boot_serial_extensions.h>

#ifdef CONFIG_FW_INFO
#include <fw_info.h>
#endif

BOOT_LOG_MODULE_DECLARE(mcuboot);

int bs_peruser_system_specific(const struct nmgr_hdr *hdr, const char *buffer,
                               int len, zcbor_state_t *cs)
{
    int mgmt_rc = MGMT_ERR_ENOTSUP;

    STRUCT_SECTION_FOREACH(mcuboot_bs_custom_handlers, function) {
        if (function->handler) {
            mgmt_rc = function->handler(hdr, buffer, len, cs);

            if (mgmt_rc != MGMT_ERR_ENOTSUP) {
                break;
            }
        }
    }

    if (mgmt_rc == MGMT_ERR_ENOTSUP) {
        zcbor_map_start_encode(cs, 10);
        zcbor_tstr_put_lit(cs, "rc");
        zcbor_uint32_put(cs, mgmt_rc);
        zcbor_map_end_encode(cs, 10);
    }

    return MGMT_ERR_OK;
}

static int bs_custom_mcuboot_image_get(const struct nmgr_hdr *hdr,
                                       const char *buffer, int len,
                                       zcbor_state_t *cs)
{
    if(hdr->nh_id == IMGMGR_NMGR_ID_STATE)
    {
        bool s0_valid, s1_valid, s0_active;

        const struct fw_info *s0 = fw_info_find(PM_S0_IMAGE_ADDRESS);
        s0_valid = s0->valid == CONFIG_FW_INFO_VALID_VAL;

        const struct fw_info *s1 = fw_info_find(PM_S1_IMAGE_ADDRESS);
        s1_valid = s1->valid == CONFIG_FW_INFO_VALID_VAL;

        if(!s1_valid && !s0_valid)
        {
            zcbor_map_start_encode(cs, 10);
            zcbor_tstr_put_lit(cs, "rc");
            zcbor_uint32_put(cs, MGMT_ERR_EINVAL);
            zcbor_map_end_encode(cs, 10);
            return MGMT_ERR_EINVAL;
        }
        else if(!s1_valid)
        {
            s0_active = true;
        }
        else if(!s0_valid)
        {
            s0_active = false;
        }
        else
        {
            s0_active = s0->version >= s1->version;
        }

        uint32_t mcuboot_version;
        uint32_t mcuboot_slot;
        if(s0_active)
        {
            mcuboot_version = s0->version;
            mcuboot_slot = 0;
        }
        else
        {
            mcuboot_version = s1->version;
            mcuboot_slot = 1;
        }

        zcbor_map_start_encode(cs, 10);
        zcbor_tstr_put_lit(cs, "version");
        zcbor_uint32_put(cs, mcuboot_version);
        zcbor_tstr_put_lit(cs, "slot");
        zcbor_uint32_put(cs, mcuboot_slot);
        zcbor_map_end_encode(cs, 10);
    }

    return MGMT_ERR_OK;
}

MCUMGR_HANDLER_DEFINE(mcuboot_image_get, bs_custom_mcuboot_image_get);
