# Block Storage Library Release Notes
The block storage library provides an API enabling use of non-volatile storage to store, retrieve, and erase data .

### What's Included?
APIs for storing, retrieving and erasing data in non-volatile storage.

This library provides a convenient way to store, retrieve and erase data in non volatile storage.
It defines APIs that can be used, but does not make any assumptions about the underlying memory type.
This allows the interface to be used by a wide range of middleware libraries and to be expanded to support different memory architectures.
Some memory abstractions are already supported such as:
* using NVM HAL APIs
* relying directly on mtb-pdl-cat2.
Others are expected to be added in future releases, or can be supported by the application itself.
To implement a custom interface see the mtb_block_storage.h file for what is expected and the mtb_block_storage_*.c files for how the existing protocols are supported.

#### v1.3.0
* Added support for serial memory
* Added support for serial flash
* NVM Create function renamed to mtb_block_storage_create_hal_nvm
* NVM Create function prototype updated to accept the HAL NVM object
* CAT2 Create function renamed to mtb_block_storage_create_pdl

#### v1.2.1
* Fixed issue on ECT flash

#### v1.2.0
* Support MTB-HAL version

#### v1.1.0
* Fixed support for older version of HAL

#### v1.0.1
* Patch version update
* ECT flash handling updates

#### v1.0.0
* Initial release

### Supported Software and Tools
This version of the Block Storage Library was validated for compatibility with the following Software and Tools:

| Software and Tools                        | Version |
| :---                                      | :----:  |
| ModusToolbox™ Software Environment        | 3.1.0   |
| GCC Compiler                              | 11.3.1  |

Minimum required ModusToolbox™ Software Environment: v3.0

### More information
Use the following links for more information, as needed:
* [API Reference Guide](https://infineon.github.io/block-storage/html/modules.html)
* [Cypress Semiconductor, an Infineon Technologies Company](http://www.infineon.com)
* [Infineon GitHub](https://github.com/infineon)
* [ModusToolbox™](https://www.cypress.com/products/modustoolbox-software-environment)

---
© Cypress Semiconductor Corporation (an Infineon company) or an affiliate of Cypress Semiconductor Corporation, 2023.
