# IRIX SMDI Tools

**Version 1.0**  
**Copyright B) 2025 Mike Wolak (mikewolak@gmail.com)**  
**Licensed under GNU GPL 3.0**

## Overview

IRIX SMDI Tools is a comprehensive suite of utilities designed for IRIX 5.3 workstations to communicate with SCSI musical samplers using the SCSI Musical Data Interchange (SMDI) protocol. The software provides both low-level SCSI ASPI access for diagnostics and a high-level SMDI implementation for sample transfer and management.

This implementation is specifically designed for IRIX 5.3 on MIPS platforms and is written in ANSI C90 for maximum compatibility with older systems.

## Features

### ASPI Tester (`aspi_test`)

- Interactive shell for raw SCSI command testing
- Scan SCSI bus for devices
- Send arbitrary SCSI commands
- Receive and display responses
- Detailed debugging with hex dumps
- File transfer capabilities for raw data

### SMDI Utility (`smdi_test`)

- Interactive shell for SMDI operations
- List samples on connected samplers
- Transfer samples between IRIX and samplers
- View detailed sample information
- Delete samples from devices
- AIF (AIFF/AIFC) file format support
- Native sample format for maximum transfer efficiency

### Key Capabilities

- **Direct SCSI Access**: Communicates directly with devices using IRIX SCSI drivers
- **Transfer Progress**: Visual feedback during sample transfers
- **Comprehensive Error Handling**: Detailed error reporting and diagnostics
- **Audio Interchange**: Integration with IRIX Audio File Library for AIF support
- **Efficient Sample Format**: Uses an internal big-endian optimized format by default, with support for standard AIFF/AIFC files
- **Sampler Compatibility**: Designed with focus on the Yamaha A4000 sampler

## Requirements

- IRIX 5.3 (or compatible)
- MIPS processor (big-endian)
- C compiler with ANSI C90 support
- SCSI controller
- Audio File Library (for AIF support)

## Building
