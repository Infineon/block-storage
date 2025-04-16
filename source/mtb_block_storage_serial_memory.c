/***********************************************************************************************//**
 * \file mtb_block_storage_serial_memory.c
 *
 * \brief
 * Utility library for defining storage to Serial Memory using the mtb-serial-memory library
 *
 ***************************************************************************************************
 * \copyright
 * Copyright 2024 Cypress Semiconductor Corporation (an Infineon company) or
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
#if defined(COMPONENT_MW_SERIAL_MEMORY)

#include "mtb_block_storage.h"

/*******************************************************************************
*                       Private Function Definitions
*******************************************************************************/
//--------------------------------------------------------------------------------------------------
// _mtb_block_storage_serial_memory_read_size
//--------------------------------------------------------------------------------------------------
static uint32_t _mtb_block_storage_serial_memory_read_size(void* context, uint32_t addr)
{
    CY_UNUSED_PARAMETER(context);
    CY_UNUSED_PARAMETER(addr);
    return 1;
}


//--------------------------------------------------------------------------------------------------
// _mtb_block_storage_serial_memory_program_size
//--------------------------------------------------------------------------------------------------
static uint32_t _mtb_block_storage_serial_memory_program_size(void* context, uint32_t addr)
{
    return mtb_serial_memory_get_prog_size((mtb_serial_memory_t*)context, addr);
}


//--------------------------------------------------------------------------------------------------
// _mtb_block_storage_serial_memory_erase_size
//--------------------------------------------------------------------------------------------------
static uint32_t _mtb_block_storage_serial_memory_erase_size(void* context, uint32_t addr)
{
    return mtb_serial_memory_get_erase_size((mtb_serial_memory_t*)context, addr);
}


//--------------------------------------------------------------------------------------------------
// _mtb_block_storage_serial_memory_erase_value
//--------------------------------------------------------------------------------------------------
static uint8_t _mtb_block_storage_serial_memory_erase_value(void* context, uint32_t addr)
{
    CY_UNUSED_PARAMETER(context);
    CY_UNUSED_PARAMETER(addr);
    /* Common erase value for flash based chips is 0xFF */
    return 0XFF;
}


//--------------------------------------------------------------------------------------------------
// _mtb_block_storage_serial_memory_read
//--------------------------------------------------------------------------------------------------
static cy_rslt_t _mtb_block_storage_serial_memory_read(void* context, uint32_t addr,
                                                       uint32_t length,
                                                       uint8_t* buf)
{
    return mtb_serial_memory_read((mtb_serial_memory_t*)context, addr, length, buf);
}


//--------------------------------------------------------------------------------------------------
// _mtb_block_storage_serial_memory_program
//--------------------------------------------------------------------------------------------------
static cy_rslt_t _mtb_block_storage_serial_memory_program(void* context, uint32_t addr,
                                                          uint32_t length,
                                                          const uint8_t* buf)
{
    return mtb_serial_memory_write((mtb_serial_memory_t*)context, addr, length, buf);
}


//--------------------------------------------------------------------------------------------------
// _mtb_block_storage_serial_memory_is_erase_required
//--------------------------------------------------------------------------------------------------
static bool _mtb_block_storage_serial_memory_is_erase_required(void* context, uint32_t addr,
                                                               uint32_t length)
{
    CY_UNUSED_PARAMETER(context);
    CY_UNUSED_PARAMETER(addr);
    CY_UNUSED_PARAMETER(length);
    /* There is no way to detect the type of the external memory, so be safe
       and assume it is a flash memory that needs to be erased before writing */
    return true;
}


//--------------------------------------------------------------------------------------------------
// _mtb_block_storage_serial_memory_erase
//--------------------------------------------------------------------------------------------------
static cy_rslt_t _mtb_block_storage_serial_memory_erase(void* context, uint32_t addr,
                                                        uint32_t length)
{
    return mtb_serial_memory_erase((mtb_serial_memory_t*)context, addr, length);
}


/*******************************************************************************
*                        Public Function Definitions
*******************************************************************************/

/** Creates and sets up the block storage object for serial memory */
cy_rslt_t mtb_block_storage_create_serial_memory(mtb_block_storage_t* bsd, mtb_serial_memory_t* obj)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;

    if ((NULL == bsd) || (NULL == obj))
    {
        result = MTB_BLOCK_STORAGE_INVALID_INPUT_ERROR;
    }
    if (result == CY_RSLT_SUCCESS)
    {
        bsd->read = _mtb_block_storage_serial_memory_read;
        bsd->program = _mtb_block_storage_serial_memory_program;
        bsd->erase = _mtb_block_storage_serial_memory_erase;
        bsd->get_read_size = _mtb_block_storage_serial_memory_read_size;
        bsd->get_program_size = _mtb_block_storage_serial_memory_program_size;
        bsd->get_erase_size = _mtb_block_storage_serial_memory_erase_size;
        bsd->get_erase_value = _mtb_block_storage_serial_memory_erase_value;
        bsd->is_erase_required = _mtb_block_storage_serial_memory_is_erase_required;
        bsd->program_nb = NULL; //Setting NULL, as program_nb is not supported
        bsd->erase_nb = NULL; //Setting NULL, as erase_nb is not supported
        bsd->is_in_range = NULL; //Setting NULL, as is_in_range is not supported
        bsd->context = obj;
    }
    return result;
}


#endif // defined(COMPONENT_MW_SERIAL_MEMORY)
