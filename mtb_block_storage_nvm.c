/***********************************************************************************************//**
 * \file mtb_block_storage_nvm.c
 *
 * \brief
 * Utility library for for defining storage to NVM using cyhal_nvm library.
 *
 ***************************************************************************************************
 * \copyright
 * Copyright 2023 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 **************************************************************************************************/
#if !defined(COMPONENT_CAT2)
#include "mtb_block_storage.h"
#include "cyhal_nvm.h"


cyhal_nvm_t obj;
#if (CPUSS_FLASHC_ECT == 1)
static cy_rslt_t WorkFlashProgramRow(
    const uint32_t* addr,
    const uint32_t* data,
    uint32_t prog_size);
#endif
//--------------------------------------------------------------------------------------------------
// mtb_block_storage_nvm_read_size
//--------------------------------------------------------------------------------------------------
static uint32_t mtb_block_storage_nvm_read_size(void* context, uint32_t addr)
{
    CY_UNUSED_PARAMETER(context);
    CY_UNUSED_PARAMETER(addr);
    //Read operation is based on direct memory access so read size is always one 32-bit word.
    return 1;
}


//--------------------------------------------------------------------------------------------------
// mtb_block_storage_nvm_program_size
//--------------------------------------------------------------------------------------------------
static uint32_t mtb_block_storage_nvm_program_size(void* context, uint32_t addr)
{
    cyhal_nvm_info_t nvm_info;
    cyhal_nvm_get_info((cyhal_nvm_t*)context, &nvm_info);
    uint32_t programSize = 0;

    for (uint32_t region = 0; region < nvm_info.region_count; region++)
    {
        if ((addr >= nvm_info.regions[region].start_address) &&
            (addr <
             (nvm_info.regions[region].start_address + nvm_info.regions[region].size)))
        {
            //For Flash NVM block size represents the minimum programmable size and sector size
            //represents the minimum erasable size. As they do not always match, it is necessary to
            // use the bigger
            //metric so that program and erase are performed on the same area.
            //However for RRAM NVM where we don't have an erase operation, block size represents the
            // actual
            //minimum programmable size and sector size represents the overall physical
            // sectorization of the memory.
            //For this reason we need to select different measures for this function based on the
            // memory type.
            if (nvm_info.regions[region].nvm_type == CYHAL_NVM_TYPE_RRAM)
            {
                programSize = (nvm_info.regions[region].block_size);
            }
            else
            {
                programSize = nvm_info.regions[region].sector_size;
            }
            break;
        }
    }
    return programSize;
}


//--------------------------------------------------------------------------------------------------
// mtb_block_storage_nvm_erase_size
//--------------------------------------------------------------------------------------------------
static uint32_t mtb_block_storage_nvm_erase_size(void* context, uint32_t addr)
{
    cyhal_nvm_info_t nvm_info;
    cyhal_nvm_get_info((cyhal_nvm_t*)context, &nvm_info);
    uint32_t eraseSize = 0;

    for (uint32_t region = 0; region < nvm_info.region_count; region++)
    {
        if ((addr >= nvm_info.regions[region].start_address) &&
            (addr <
             (nvm_info.regions[region].start_address + nvm_info.regions[region].size)))
        {
            //For Flash NVM block size represents the minimum programmable size and sector size
            //represents the minimum erasable size. However for RRAM NVM where we don't
            //have an erase operation, block size represents the actual minimum programmable size
            //and sector size represents the overal physical sectorization ov the memory.
            //For this reason we need to select different measures for this function based on the
            //memory type.
            if (nvm_info.regions[region].nvm_type == CYHAL_NVM_TYPE_RRAM)
            {
                eraseSize = (nvm_info.regions[region].block_size);
            }
            else
            {
                eraseSize = nvm_info.regions[region].sector_size;
            }
            break;
        }
    }
    return eraseSize;
}


//--------------------------------------------------------------------------------------------------
// mtb_block_storage_nvm_erase_value
//--------------------------------------------------------------------------------------------------
static uint8_t mtb_block_storage_nvm_erase_value(void* context, uint32_t addr)
{
    cyhal_nvm_info_t nvm_info;
    cyhal_nvm_get_info((cyhal_nvm_t*)context, &nvm_info);
    uint8_t eraseValue = 0;

    for (uint32_t region = 0; region < nvm_info.region_count; region++)
    {
        if ((addr >= nvm_info.regions[region].start_address) &&
            (addr <
             (nvm_info.regions[region].start_address + nvm_info.regions[region].size)))
        {
            eraseValue = nvm_info.regions[region].erase_value;
            break;
        }
    }
    return eraseValue;
}


//--------------------------------------------------------------------------------------------------
// mtb_block_storage_nvm_read
//--------------------------------------------------------------------------------------------------
static cy_rslt_t mtb_block_storage_nvm_read(void* context, uint32_t addr, uint32_t length,
                                            uint8_t* buf)
{
    return cyhal_nvm_read((cyhal_nvm_t*)context, addr, buf, length);
}


//--------------------------------------------------------------------------------------------------
// mtb_block_storage_nvm_program
//--------------------------------------------------------------------------------------------------
static cy_rslt_t mtb_block_storage_nvm_program(void* context, uint32_t addr, uint32_t length,
                                               const uint8_t* buf)
{
    uint32_t prog_size = mtb_block_storage_nvm_program_size(context, addr);
    cy_rslt_t result = CY_RSLT_SUCCESS;

    if ((0 != (length % prog_size)))
    {
        result = MTB_BLOCK_STORAGE_INVALID_SIZE_ERROR;
    }

    #if (CPUSS_FLASHC_ECT == 1)
    if (result == CY_RSLT_SUCCESS)
    {
        for (uint32_t loc = addr; result == CY_RSLT_SUCCESS && loc < addr + length;
             loc += prog_size, buf += prog_size)
        {
            result = WorkFlashProgramRow((uint32_t*)addr, (const uint32_t*)buf, prog_size);
        }
    }
    #else
    if (result == CY_RSLT_SUCCESS)
    {
        for (uint32_t loc = addr; result == CY_RSLT_SUCCESS && loc < addr + length;
             loc += prog_size, buf += prog_size)
        {
            result = cyhal_nvm_program((cyhal_nvm_t*)context, loc, (const uint32_t*)buf);
        }
    }
    #endif // if (CPUSS_FLASHC_ECT == 1)
    return result;
}


#if (CPUSS_FLASHC_ECT == 1)
/*******************************************************************************
* Function Name: WorkFlashProgramRow
****************************************************************************//**
*
* Programs XMC7xxx Work Flash Sector from address provided
*
* \param addr
* The XMC7xxx Work Flash Sector from addr
*
* \param data
* The data to be programmed into the Work Flash Sector
*
* \param prog_size
* The device's program size, needed to determine program row data size in bits
*
*******************************************************************************/


static cy_rslt_t WorkFlashProgramRow(const uint32_t* addr, const uint32_t* data, uint32_t prog_size)
{
    cy_rslt_t result = CYHAL_NVM_RSLT_ERR_ADDRESS;
    cy_stc_flash_programrow_config_t config;
    cy_en_flashdrv_status_t status;
    const uint32_t* cur_addr = (uint32_t*)addr;
    const uint32_t* cur_data = (uint32_t*)data;

    cy_en_flash_programrow_datasize_t prog_data_size;
    uint32_t page_increase = 0;
    bool found_prog_data_size = false;
    switch (prog_size)
    {
        case 128:
            prog_data_size = CY_FLASH_PROGRAMROW_DATA_SIZE_1024BIT;
            found_prog_data_size = true;
            page_increase = 128;
            break;

        case 2048:
            prog_data_size = CY_FLASH_PROGRAMROW_DATA_SIZE_4096BIT;
            found_prog_data_size = true;
            page_increase = 512;
            break;

        default:
            found_prog_data_size = false;
            break;
    }

    if (found_prog_data_size)
    {
        config.blocking  =   CY_FLASH_PROGRAMROW_BLOCKING;
        config.skipBC    =   CY_FLASH_PROGRAMROW_SKIP_BLANK_CHECK;
        config.dataSize  =   prog_data_size;
        config.dataLoc   =   CY_FLASH_PROGRAMROW_DATA_LOCATION_SRAM;
        config.intrMask  =   CY_FLASH_PROGRAMROW_SET_INTR_MASK;

        for (uint32_t cur_pg_size = 0; cur_pg_size < prog_size; cur_pg_size += page_increase)
        {
            config.destAddr  =   cur_addr;
            config.dataAddr  =   cur_data;
            status = Cy_Flash_Program_WorkFlash(&config);

            if (status != CY_FLASH_DRV_SUCCESS)
            {
                break;
            }
            cur_addr += page_increase /4;
            cur_data += (page_increase / 4U);
        }
        result = (cy_rslt_t)status;
    }

    return (result);
}


#endif // if (CPUSS_FLASHC_ECT == 1)

//--------------------------------------------------------------------------------------------------
// mtb_block_storage_nvm_is_erase_required
//--------------------------------------------------------------------------------------------------
static bool mtb_block_storage_nvm_is_erase_required(void* context, uint32_t addr, uint32_t length)
{
    cyhal_nvm_info_t nvm_info;
    cyhal_nvm_get_info((cyhal_nvm_t*)context, &nvm_info);
    bool isEraseRequired = true;

    for (uint32_t region = 0; region < nvm_info.region_count; region++)
    {
        if ((addr >= nvm_info.regions[region].start_address) &&
            (addr+length <
             (nvm_info.regions[region].start_address + nvm_info.regions[region].size)))
        {
            isEraseRequired = nvm_info.regions[region].is_erase_required;
            break;
        }
    }
    return isEraseRequired;
}


//--------------------------------------------------------------------------------------------------
// mtb_block_storage_nvm_erase
//--------------------------------------------------------------------------------------------------
static cy_rslt_t mtb_block_storage_nvm_erase(void* context, uint32_t addr, uint32_t length)
{
    uint32_t erase_size = mtb_block_storage_nvm_erase_size(context, addr);
    cy_rslt_t result = CY_RSLT_SUCCESS;

    if ((0 != (length % erase_size)))
    {
        result = MTB_BLOCK_STORAGE_INVALID_SIZE_ERROR;
    }

    if (result == CY_RSLT_SUCCESS)
    {
        for (uint32_t loc = addr; result == CY_RSLT_SUCCESS && loc < addr + length;
             loc += erase_size)
        {
            result = cyhal_nvm_erase((cyhal_nvm_t*)context, loc);
        }
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
// mtb_block_storage_nvm_program_nb
//--------------------------------------------------------------------------------------------------
static cy_rslt_t mtb_block_storage_nvm_program_nb(void* context, uint32_t addr, uint32_t length,
                                                  const uint8_t* buf)
{
    #if !defined(MTB_BLOCK_STORAGE_NON_BLOCKING_SUPPORTED)
    CY_UNUSED_PARAMETER(context);
    CY_UNUSED_PARAMETER(addr);
    CY_UNUSED_PARAMETER(length);
    CY_UNUSED_PARAMETER(buf);
    return MTB_BLOCK_STORAGE_NOT_SUPPORTED_ERROR;
    #else

    uint32_t prog_size = mtb_block_storage_nvm_program_size(context, addr);
    cy_rslt_t result = CY_RSLT_SUCCESS;

    if ((0 != (length % prog_size)))
    {
        result = MTB_BLOCK_STORAGE_INVALID_SIZE_ERROR;
    }

    if (result == CY_RSLT_SUCCESS)
    {
        for (uint32_t loc = addr; result == CY_RSLT_SUCCESS && loc < addr + length;
             loc += prog_size, buf += prog_size)
        {
            result = cyhal_nvm_start_program((cyhal_nvm_t*)context, loc, (const uint32_t*)buf);
            while (!cyhal_nvm_is_operation_complete((cyhal_nvm_t*)context))
            {
            }
        }
    }
    return result;
    #endif // !defined(MTB_BLOCK_STORAGE_NON_BLOCKING_SUPPORTED)
}


//--------------------------------------------------------------------------------------------------
// mtb_block_storage_nvm_erase_nb
//--------------------------------------------------------------------------------------------------
static cy_rslt_t mtb_block_storage_nvm_erase_nb(void* context, uint32_t addr, uint32_t length)
{
    #if !defined(MTB_BLOCK_STORAGE_NON_BLOCKING_SUPPORTED)
    CY_UNUSED_PARAMETER(context);
    CY_UNUSED_PARAMETER(addr);
    CY_UNUSED_PARAMETER(length);
    return MTB_BLOCK_STORAGE_NOT_SUPPORTED_ERROR;
    #else
    uint32_t erase_size = mtb_block_storage_nvm_erase_size(context, addr);
    cy_rslt_t result = CY_RSLT_SUCCESS;

    if ((0 != (length % erase_size)))
    {
        result = MTB_BLOCK_STORAGE_INVALID_SIZE_ERROR;
    }

    if (result == CY_RSLT_SUCCESS)
    {
        for (uint32_t loc = addr; result == CY_RSLT_SUCCESS && loc < addr + length;
             loc += erase_size)
        {
            result = cyhal_nvm_start_erase((cyhal_nvm_t*)context, loc);
            while (!cyhal_nvm_is_operation_complete((cyhal_nvm_t*)context))
            {
            }
        }
    }
    return result;
    #endif // if !defined(MTB_BLOCK_STORAGE_NON_BLOCKING_SUPPORTED)
}


//--------------------------------------------------------------------------------------------------
// mtb_block_storage_nvm_is_in_range
//--------------------------------------------------------------------------------------------------
static bool mtb_block_storage_nvm_is_in_range(void* context, uint32_t addr, uint32_t length)
{
    cyhal_nvm_info_t nvm_info;
    cyhal_nvm_get_info((cyhal_nvm_t*)context, &nvm_info);
    bool isInRange = false;

    for (uint32_t region = 0; region < nvm_info.region_count; region++)
    {
        if ((addr >= nvm_info.regions[region].start_address) &&
            (addr+length <=
             (nvm_info.regions[region].start_address + nvm_info.regions[region].size)))
        {
            isInRange = true;
            break;
        }
    }
    return isInRange;
}


//--------------------------------------------------------------------------------------------------
// mtb_block_storage_nvm_create
//--------------------------------------------------------------------------------------------------
cy_rslt_t mtb_block_storage_nvm_create(mtb_block_storage_t* bsd)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;

    if (NULL == bsd)
    {
        result = MTB_BLOCK_STORAGE_INVALID_INPUT_ERROR;
    }

    if (result == CY_RSLT_SUCCESS)
    {
        result = cyhal_nvm_init(&obj);
    }

    if (result == CY_RSLT_SUCCESS)
    {
        bsd->read = mtb_block_storage_nvm_read;
        bsd->program = mtb_block_storage_nvm_program;
        bsd->erase = mtb_block_storage_nvm_erase;
        bsd->program_nb = mtb_block_storage_nvm_program_nb;
        bsd->erase_nb = mtb_block_storage_nvm_erase_nb;
        bsd->get_read_size = mtb_block_storage_nvm_read_size;
        bsd->get_program_size = mtb_block_storage_nvm_program_size;
        bsd->get_erase_size = mtb_block_storage_nvm_erase_size;
        bsd->get_erase_value = mtb_block_storage_nvm_erase_value;
        bsd->is_in_range = mtb_block_storage_nvm_is_in_range;
        bsd->is_erase_required = mtb_block_storage_nvm_is_erase_required;
        bsd->context = &obj;
    }
    else
    {
        cyhal_nvm_free(&obj);
    }

    return result;
}


#endif // if !defined(COMPONENT_CAT2)
