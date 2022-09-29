# RLOG Server
Small and portable server to send log messages to a [remote client](https://github.com/eduardodsp/rlogcli).

## Features
I wrote this to help me monitor the status of devices on my other projects. Some of the features include:

    - Simple and clean APIs
    - Timestamped messages.
    - Easy to filter by type and by context.
    - Asynchronous write.
    - Thread safety. Multiple threads will write to the same log buffer
    - Portable, since I will be using on other projects.

## Supported Platforms
|   OS          | Target          | Status          |
| ------------- | -------------   | -------------   |
| FreeRTOS      | ESP32           | Supported       |
| TBD           | TBD             | TBD             |

## How To Use
```
    #include "rlog.h"
    
    // Initialize target HW...
    
    // Initialize network interface...
    
    // Initialize RLOG server
    int t_err = rlog_init();
    if(t_err == RLOG_OK) {
        // Log an INFO message, with tag "TEST"
        rlog("TEST", RLOG_INFO,"HELLO WORLD!");
    }
    else {
        printf("RLOG init failed, error: %d \n", t_err);
    }
```
## Portability layer

The header files on [platform directory](https://github.com/eduardodsp/rlog/tree/main/platform) define the APIs that must be implemented for each target system. 
```
platform
|
|-- env (Operating system and middleware APIs)  
|   |-- rlog_net.h (Network API)
|   |-- rlog_osal.h (Operating systems abstraction layer)
|   |-- FreeRTOS (FreeRTOS implementation)
|   '-- POSIX (POSIX implementation, future!)
|
|-- target   (Hardware dependent APIs)
|   |-- rlog_rtc.h (Real Time Clock APIs)
|   '-- ESP32 (Generic ESP32 implementation)

```


## Roadmap
    - Improve API documentation
    - Add multiclient support. This is a must.
    - Port to other popular platforms depending on my needs
    - Add support for other protocols (UART, SPI, CAN etc..) depending on my needs.