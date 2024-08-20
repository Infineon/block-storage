/***********************************************************************************************//**
 * \file mtb_block_storage_nvm.c
 *
 * \brief
 * Utility library for defining storage to NVM using cyhal_nvm or cyhal_flash library depending
 * on HAL version.
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
#include "cyhal.h"
#if (CYHAL_DRIVER_AVAILABLE_NVM)
#include "cyhal_nvm.h"
#else // if (CYHAL_DRIVER_AVAILABLE_NVM)
#include "cyhal_flash.h"
#endif // if (CYHAL_DRIVER_AVAILABLE_NVM)

#if (CYHAL_DRIVER_AVAILABLE_NVM)
typedef cyhal_nvm_region_info_t mtb_block_storage_nvm_region_info_t;
typedef cyhal_nvm_t             mtb_block_storage_nvm_t;
typedef cyhal_nvm_info_t        mtb_block_storage_nvm_info_t;
#define MTB_BLOCK_STORAGE_NVM_SUPPORT
#else // (CYHAL_DRIVER_AVAILABLE_NVM)
typedef cyhal_flash_block_info_t mtb_block_storage_nvm_region_info_t;
typedef cyhal_flash_t            mtb_block_storage_nvm_t;
typedef cyhal_flash_info_t       mtb_block_storage_nvm_info_t;
#define MTB_BLOCK_STORAGE_FLASH_SUPPORT
#endif // (CYHAL_DRIVER_AVAILABLE_NVM)


/* The following API is implemented in block storage as a fallback for older HAL
   versions that use the now deprecated cyhal_flash.h or do not have access to the
   cyhal_nvm.h API cyhal_nvm_get_region_for_address. */

/** Find the nvm region based on given address and length.
 * If "length is zero and address is not in any nvm region" or
 * if "length is not zero and address is not in any nvm region" or
 * if "length is not zero and address is one nvm region but address + length goes into another
 * nvm region", the function will return Null.
 *
 * @param[in] obj The nvm object
 * @param[in] addr The start address of the region to check
 * @param[in] length The length to check
 * @return pointer, mtb_block_storage_nvm_region_info_t, to region info
 */
const mtb_block_storage_nvm_region_info_t* mtb_block_storage_nvm_get_region_for_address(
    mtb_block_storage_nvm_t* obj, uint32_t addr,
    uint32_t length)
{
    #if !defined(CYHAL_API_AVAILABLE_NVM_GET_REGION_FOR_ADDRESS)
    mtb_block_storage_nvm_info_t nvm_info;
    #if defined(MTB_BLOCK_STORAGE_FLASH_SUPPORT)
    cyhal_flash_get_info(obj, &nvm_info);

    for (uint32_t block = 0; block < nvm_info.block_count; block++)
    {
        if (((addr >= nvm_info.blocks[block].start_address) &&
             (addr < (nvm_info.blocks[block].start_address + nvm_info.blocks[block].size)) &&
             (addr + length <=
              (nvm_info.blocks[block].start_address + nvm_info.blocks[block].size)))
            )
        {
            return (mtb_block_storage_nvm_region_info_t*)&nvm_info.blocks[block];
        }
    }

    #else // if defined(MTB_BLOCK_STORAGE_FLASH_SUPPORT)
    cyhal_nvm_get_info(obj, &nvm_info);

    for (uint32_t region = 0; region < nvm_info.region_count; region++)
    {
        if (((addr >= nvm_info.regions[region].start_address) &&
             (addr < (nvm_info.regions[region].start_address + nvm_info.regions[region].size)) &&
             (addr + length <=
              (nvm_info.regions[region].start_address + nvm_info.regions[region].size)))
            )
        {
            return (mtb_block_storage_nvm_region_info_t*)&nvm_info.regions[region];
        }
    }
    #endif // if defined(MTB_BLOCK_STORAGE_FLASH_SUPPORT)
    return NULL;
    #else // if !defined(CYHAL_API_AVAILABLE_NVM_GET_REGION_FOR_ADDRESS)
    return (mtb_block_storage_nvm_region_info_t*)cyhal_nvm_get_region_for_address((cyhal_nvm_t*)obj,
                                                                                  addr, length);
    #endif // if !defined(CYHAL_API_AVAILABLE_NVM_GET_REGION_FOR_ADDRESS)
}


mtb_block_storage_nvm_t obj;

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
    uint32_t programSize = 0;

    const mtb_block_storage_nvm_region_info_t* region_info =
        mtb_block_storage_nvm_get_region_for_address(
            (mtb_block_storage_nvm_t*)context, addr, 0);
    if (NULL != region_info)
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
        #if defined(MTB_BLOCK_STORAGE_NVM_SUPPORT)
        if (region_info->nvm_type == CYHAL_NVM_TYPE_RRAM)
        {
            programSize = (region_info->block_size);
        }
        else
        #endif // defined(MTB_BLOCK_STORAGE_NVM_SUPPORT)
        {
            programSize = region_info->sector_size;
        }
    }
    return programSize;
}


//--------------------------------------------------------------------------------------------------
// mtb_block_storage_nvm_erase_size
//--------------------------------------------------------------------------------------------------
static uint32_t mtb_block_storage_nvm_erase_size(void* context, uint32_t addr)
{
    uint32_t eraseSize = 0;

    const mtb_block_storage_nvm_region_info_t* region_info =
        mtb_block_storage_nvm_get_region_for_address(
            (mtb_block_storage_nvm_t*)context, addr, 0);
    if (NULL != region_info)
    {
        //For Flash NVM block size represents the minimum programmable size and sector size
        //represents the minimum erasable size. However for RRAM NVM where we don't
        //have an erase operation, block size represents the actual minimum programmable size
        //and sector size represents the overal physical sectorization of the memory.
        //For this reason we need to select different measures for this function based on the
        //memory type.
        #if defined(MTB_BLOCK_STORAGE_NVM_SUPPORT)
        if (region_info->nvm_type == CYHAL_NVM_TYPE_RRAM)
        {
            eraseSize = (region_info->block_size);
        }
        else
        #endif // defined (MTB_BLOCK_STORAGE_NVM_SUPPORT)
        {
            eraseSize = region_info->sector_size;
        }
    }

    return eraseSize;
}


//--------------------------------------------------------------------------------------------------
// mtb_block_storage_nvm_erase_value
//--------------------------------------------------------------------------------------------------
static uint8_t mtb_block_storage_nvm_erase_value(void* context, uint32_t addr)
{
    uint8_t eraseValue = 0;

    const mtb_block_storage_nvm_region_info_t* region_info =
        mtb_block_storage_nvm_get_region_for_address(
            (mtb_block_storage_nvm_t*)context, addr, 0);

    if (NULL != region_info)
    {
        eraseValue = region_info->erase_value;
    }

    return eraseValue;
}


//--------------------------------------------------------------------------------------------------
// mtb_block_storage_nvm_read
//--------------------------------------------------------------------------------------------------
static cy_rslt_t mtb_block_storage_nvm_read(void* context, uint32_t addr, uint32_t length,
                                            uint8_t* buf)
{
    #if (CYHAL_DRIVER_AVAILABLE_NVM)
    return cyhal_nvm_read((cyhal_nvm_t*)context, addr, buf, length);
    #else // if (CYHAL_DRIVER_AVAILABLE_NVM)
    return cyhal_flash_read((cyhal_flash_t*)context, addr, buf, length);
    #endif // if (CYHAL_DRIVER_AVAILABLE_NVM)
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

    if (result == CY_RSLT_SUCCESS)
    {
        for (uint32_t loc = addr; result == CY_RSLT_SUCCESS && loc < addr + length;
             loc += prog_size, buf += prog_size)
        {
            #if (CPUSS_FLASHC_ECT == 1)
            result = WorkFlashProgramRow((uint32_t*)addr, (const uint32_t*)buf, prog_size);
            #else // if (CPUSS_FLASHC_ECT == 1)
            #if (CYHAL_DRIVER_AVAILABLE_NVM)
            result = cyhal_nvm_program((cyhal_nvm_t*)context, loc, (const uint32_t*)buf);
            #else // if (CYHAL_DRIVER_AVAILABLE_NVM)
            result = cyhal_flash_program((cyhal_flash_t*)context, loc, (const uint32_t*)buf);
            #endif // if (CYHAL_DRIVER_AVAILABLE_NVM)
            #endif // if (CPUSS_FLASHC_ECT == 1)
        }
    }
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
    cy_en_flashdrv_status_t status = CY_FLASH_DRV_SUCCESS;
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
    bool isEraseRequired = true;

    #if !defined(MTB_BLOCK_STORAGE_FLASH_SUPPORT)
    const mtb_block_storage_nvm_region_info_t* region_info =
        mtb_block_storage_nvm_get_region_for_address(
            (mtb_block_storage_nvm_t*)context, addr, length);
    if (NULL != region_info)
    {
        isEraseRequired = region_info->is_erase_required;
    }
    #else // if !defined(MTB_BLOCK_STORAGE_FLASH_SUPPORT)
    CY_UNUSED_PARAMETER(context);
    CY_UNUSED_PARAMETER(addr);
    CY_UNUSED_PARAMETER(length);
    #endif // if !defined(MTB_BLOCK_STORAGE_FLASH_SUPPORT)

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
            #if (CYHAL_DRIVER_AVAILABLE_NVM)
            result = cyhal_nvm_erase((cyhal_nvm_t*)context, loc);
            #else // if (CYHAL_DRIVER_AVAILABLE_NVM)
            result = cyhal_flash_erase((cyhal_flash_t*)context, loc);
            #endif // if (CYHAL_DRIVER_AVAILABLE_NVM)
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
    #else // if !defined(MTB_BLOCK_STORAGE_NON_BLOCKING_SUPPORTED)

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
            #if (CYHAL_DRIVER_AVAILABLE_NVM)
            result = cyhal_nvm_start_program((cyhal_nvm_t*)context, loc, (const uint32_t*)buf);
            while (!cyhal_nvm_is_operation_complete((cyhal_nvm_t*)context))
            #else // if (CYHAL_DRIVER_AVAILABLE_NVM)
            result = cyhal_flash_start_program((cyhal_flash_t*)context, loc, (const uint32_t*)buf);
            while (!cyhal_flash_is_operation_complete((cyhal_flash_t*)context))
            #endif // if (CYHAL_DRIVER_AVAILABLE_NVM)
            {
            }
        }
    }
    return result;
    #endif // if !defined(MTB_BLOCK_STORAGE_NON_BLOCKING_SUPPORTED)
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
    #else // if !defined(MTB_BLOCK_STORAGE_NON_BLOCKING_SUPPORTED)
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
            #if (CYHAL_DRIVER_AVAILABLE_NVM)
            result = cyhal_nvm_start_erase((cyhal_nvm_t*)context, loc);
            while (!cyhal_nvm_is_operation_complete((cyhal_nvm_t*)context))
            #else // if (CYHAL_DRIVER_AVAILABLE_NVM)
            result = cyhal_flash_start_erase((cyhal_flash_t*)context, loc);
            while (!cyhal_flash_is_operation_complete((cyhal_flash_t*)context))
            #endif // if (CYHAL_DRIVER_AVAILABLE_NVM)
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
    bool isInRange = false;

    const mtb_block_storage_nvm_region_info_t* region_info =
        mtb_block_storage_nvm_get_region_for_address(
            (mtb_block_storage_nvm_t*)context, addr, length);
    if (NULL != region_info)
    {
        isInRange = true;
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
        #if (CYHAL_DRIVER_AVAILABLE_NVM)
        result = cyhal_nvm_init(&obj);
        #else // (CYHAL_DRIVER_AVAILABLE_NVM)
        result = cyhal_flash_init(&obj);
        #endif // (CYHAL_DRIVER_AVAILABLE_NVM)
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
        #if (CYHAL_DRIVER_AVAILABLE_NVM)
        cyhal_nvm_free(&obj);
        #else
        cyhal_flash_free(&obj);
        #endif
    }

    return result;
}


#endif // if !defined(COMPONENT_CAT2)
