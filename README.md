# UMD Hub
Application to route UMD and tally data between systems.

This application offers a simple text based protocol to allow connection to switchers
or broadcast software and translates the status updates from/to other protocols.

## Dependencies
Currently you need to checkout umd_tools in the same workspace folder as this repo.
Will add a submodule later. 

## Protocol support

### Currently supported output protocols:
 * TSL over Serial (UDP and TCP soon to come)

### Currently supported input protocols:


## Outlook

### Features planned
 * TSL input over UDP/TCP/serial
 * Web frontend

### Plugins and connectors planned
 * OBS integration
 * BMD ATEM Protocol support (tally and UMD from ATEM)

## License

This software is provided under GPL v3 (see LICENSE.md)
