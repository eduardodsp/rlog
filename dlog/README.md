# Dlog

Stupid simple log file manager written entirely in ANSI C.

Features:

    - Circular log with user defined size.

    - Overwrite old logs if file is full. This option can be disabled at runtime.

    - Save messages to a human readable file.
    
    - Multiple instances.

Warning: NOT THREAD SAFE!

All APIs are documented in dlog.h

### Usage:
```

    // first we create an instance of dlog_t
    dlog_t log;

    // then we initialize the instance and open the file
    if( dlog_open(&log, "mylog", 4) !=  DLOG_OK ) {
        printf("dlog_open FAILED \n");
        return 0;
    }

    // optionally we can also set or clear some of the log options
    if ( dlog_set_opt(&log, DLOG_OPT_OVERWRITE, 1) !=  DLOG_OK ){
        printf("dlog_set_opt FAILED \n");
        return 0;
    }   

    // now we can write a message into the file
    if ( dlog_put(&log, "Sample Info message") !=  DLOG_OK ){
        printf("dlog_put FAILED \n");
        return 0;
    }

    // and read the message
    char readbuf[DLOG_LINE_MAX_SIZE] = "";
    if( dlog_get(&log, readbuf, sizeof(readbuf)) == DLOG_OK )
        printf("%s\n", readbuf);
    
```

