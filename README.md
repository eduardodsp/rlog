# RLOG Server
Small and portable server to send log messages to a [remote client](https://github.com/eduardodsp/rlogcli).

## Features
I wrote this to help me monitor the status of devices on my other projects. Some of the features include:

    - Simple and clean APIs
    - Timestamped messages.
    - Asynchronous write.
    - Thread safety. Multiple threads will write to the same log buffer
    - Portable, since I will be using on other projects.
    - Support for persistence file when no client is connected.
    - Support for multiple communication protocols including user defined protocols.
    - Support for multiple TCP clients when using the RLOG_DEFAULT_TCPIP interface.
    - Support for multiple active interfaces. The server will write messages to all connected
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
    
    // Initialize target HW...
    
    // Install and initialize communication interface...
    rlog_install_interface(RLOG_STDOUT);
    rlog_install_interface(RLOG_DEFAULT_TCPIP);

    // Initialize RLOG server and the backup file
    if( rlog_init("/spiffs/rlog.log", 40) ) {
        // Log an INFO message
        rlog(RLOG_INFO,"HELLO WORLD!");
    }
```
## Portability layer

The header files on [port directory](https://github.com/eduardodsp/rlog/tree/main/port) define the APIs that must be implemented for each target system. 
```
port/
|
|-- com/ (Communication APIs)
|   |-- interfaces.h (Supported communication protocols)
|   '-- tcpip (Default TCP/IP APIs)
|
|-- os/ (Operating system and middleware APIs) 
|   |-- osal.h (Operating systems abstraction layer) 
|   |-- FreeRTOS (FreeRTOS implementation)
|   '-- POSIX (POSIX implementation, future!)
|

```

## Roadmap
    - Improve API documentation.
    - Support for RFC-6587 (Syslog over TCP).
    - CRC check for persistence file.
    - Port to other platforms depending on my needs.