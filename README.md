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
    
    // Initialize target HW...
    
    // Configure and install TCP-Client interface
    // here we need to set the Syslog Server IP and Port number
    rlog_tcpcli_config("192.168.178.174", 514);
    rlog_install_interface(RLOG_TCP_CLIENT);
      
    // Set my device name
    rlog_set_hostname("my_esp32_devkit");    

    // Set log format to the old BSD Syslog format
    rlog_set_format(RLOG_RFC3164);
    
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
|-- os/ (Operating system and middleware APIs) 
|   |-- osal.h (Operating systems abstraction layer) 
|   |-- FreeRTOS (FreeRTOS implementation)
|   '-- POSIX (POSIX implementation, future!)

```

## Roadmap
    - Improve API documentation.
    - CRC check for persistence file.
    - Port to POSIX.