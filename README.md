# Block Storage Library
## Overview
This library provides a convenient way to store information in non-volatile storage.

## Features
* Supports any storage which can be modeled as a block device, including internal nvm or external nvm
(e.g. via QSPI).

## Design details
### Block storage object
The three main operations needed to interact with a memory are read, write and erase.
However more information is needed to conduct these operations and another useful function to package in the object is checking whether the area selected (made up of startAddress + size) is in range of the memory selected and whether the area selected requires erase operations and what the erased value is.
Therefore the memory object is made up of:

* context : Context object that can be used in the block device implementation
* get_read_size: Function to get read size for an address
* get_program_size: Function to get program size for an address
* get_erase_size: Function to get erase size for an address
* get_erase_value: Function to get the erase value for the current memory
* read: Function to read from the memory
* program: Function to program data to the memory
* erase: Function to erase the memory area selected
* program_nb: Function to program to device in a non blocking way for the devices that support it
* erase_nb: Function to erase the memory in a non blocking way for the devices that support it
* is_in_range: Function to check whether address selected is in the memory range
* is_erase_required: Function to check whether erase is necessary for the current memory

The two create functions populate the block storage object with the required functions as explained in the following sections.

#### HAL implementation
This is the first of the actual implementations that are currently supported by the block storage solution.

It is built on top of the HAL NVM library and it allows the to abstract all devices that are supported within the library.

The block storage object is as follows:

* context : a pointer to an cyhal_nvm_t object
* get_read_size: returns 1 as the read is done by direct access to the specified address
* get_program_size: uses cyhal_nvm_info_t object to determine the region to which the address belongs and returns its sector size, i.e. the minimum programmable unit
* get_erase_size: uses cyhal_nvm_info_t object to determine the region to which the address belongs and returns its sector size, i.e. the minimum erasable unit
* get_erase_value: uses cyhal_nvm_info_t object to determine the the erase value for the current memory
* read: calls directly cyhal_nvm_read with the correct parameters
* program: determines how many program operations are needed based on program size and repeatedly calls cyhal_nvm_program
* erase: determines how many erase operations are needed based on erase size and checks whether erase is required on the memory selected. Then it repeatedly calls cyhal_nvm_erase id erase is required or it calls program with an all zero buffer to simulate an erased state for devices that do not require erase.
* program_nb: determines how many program operations are needed based on program size and repeatedly calls cyhal_nvm_start_program
* erase_nb: determines how many erase operations are needed based on erase size and checks whether erase is required on the memory selected. Then it repeatedly calls cyhal_nvm_start_erase id erase is required or it calls program_nb with an all zero buffer to simulate an erased state for devices that do not require erase.
* is_in_range: uses cyhal_nvm_info_t object to determine the region to which the address belongs and checks whether the end address (made up of start address + size) is still within the range of the memory region
* is_erase_required: uses cyhal_nvm_info_t object to determine whether erase is necessary for the current memory


#### PSoC4 implementation
This is the second of the actual implementations currently supported by the block storage solution.

It is built specifically for PSoC4 on top of the PDL library.

The block storage object is as follows:

* context : NULL, no supporting context is needed
* get_read_size: returns 1 as the read is done by direct access to the specified address
* get_program_size: returns a define CY_FLASH_SIZEOF_ROW
* get_erase_size: returns a define CY_FLASH_SIZEOF_ROW
* get_erase_value: returns 0x00 which is the erased value for PSoC4 devices
* read: calls directly memcpy with the correct parameters
* program: determines how many program operations are needed based on program size and repeatedly calls Cy_Flash_WriteRow
* erase: determines how many erase operations are needed based on erase size and repeatedly calls Cy_Flash_WriteRow with an empty buffer
* program_nb: not supported
* erase_nb: not supported
* is_in_range: checks that address is greater than CY_FLASH_BASE and that address + length is smaller than CY_FLASH_BASE + CY_FLASH_SIZE
* is_erase_required: returns true

## Dependencies
* [mtb-hal-cat1](https://github.com/infineon/mtb-hal-cat1)
* [mtb-pdl-cat2](https://github.com/infineon/mtb-pdl-cat2)

## More information
* [API Reference Guide](https://infineon.github.io/block-storage/html/index.html)
* [Cypress Semiconductor, an Infineon Technologies Company](http://www.infineon.com)
* [Infineon GitHub](https://github.com/infineon)
* [ModusToolbox™](https://www.cypress.com/products/modustoolbox-software-environment)

---
© Cypress Semiconductor Corporation (an Infineon company) or an affiliate of Cypress Semiconductor Corporation, 2023.
