# RLOG (Remote Logger)
Small and portable syslog (both RFC 5424 and RFC 3164) compatible logger, with support for multiple communication protocols and circular backup file
to save messages when no one is listening! 

## Features
I wrote this to help me monitor the status of devices on my other projects. Some of the features include:

    - Simple and clean APIs.
    - Timestamped messages.
    - Support RFC 5424 and RFC 3164.
    - Asynchronous write.
    - Thread safety.
    - Portable, since I will be using on other projects.
    - Support for persistence file when no client/server is connected.
    - Support for multiple communication protocols including user defined protocols.
    - Support for multiple active protocols. The server will write messages to all connected
    interfaces.
    - Support for stdout. This can be used when connected to the device through some serial monitor
    or SSH.

## Supported Platforms
|   OS          | Target          | Status          |
| ------------- | -------------   | -------------   |
| FreeRTOS      | ESP32           | Supported       |
| TBD           | TBD             | TBD             |

## How To Use
```
    #include "rlog.h"
              
    rlog_cfg_t cfg = {
        .name = "my esp32 devkit",      // Set my device name
        .filepath = "/spiffs/rlog.log", // Set backup file
        .nlogs    = 40,                 // Set max number of logs in backup file
        .priority = 3,                  // Set service priority
        .format = RLOG_RFC3164,         // Set log format to the old BSD Syslog format
        .level = RLOG_WARNING           // Set log level
    };

    // Configure and start RLOG server
    assert(rlog_init(cfg));

    // From this point onwards you can already start logging
    // even if there is no communication interface installed.
    // In this case rlog will just store it in the backup file

    // Log an INFO message
    rlog(RLOG_INFO,"HELLO WORLD!");

    // At any moment we can configure and install a new interface
    rlog_tcpcli_config("192.168.178.174", 514);
    rlog_install_interface(RLOG_TCP_CLIENT);

    // Now all logs will be dumped to the interface
    // as soon as it's available. In the case of this example, all 
    // logs will be dumped as soon as the tcp client connects to the server
    // at 192.168.178.174. Note: you can use an URL too!

```
## Portability layer

The header files on [port directory](https://github.com/eduardodsp/rlog/tree/main/port) define the APIs that must be implemented for each target system. 
```
port/
|
|-- os/ (Operating system and middleware APIs) 
|   |-- osal.h (Operating systems abstraction layer) 
|   |-- FreeRTOS (FreeRTOS implementation)
|   '-- POSIX (POSIX implementation, future!)

```

## Roadmap
    - Improve API documentation.
    - CRC check for persistence file.
    - Port to POSIX.