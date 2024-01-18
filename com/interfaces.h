#ifndef _PORT_COM_INTERFACES_H_
#define _PORT_COM_INTERFACES_H_

#include <stdbool.h>
#include "udp/udpip.h"
#include "tcp/client.h"
#include "tcp/server.h"

/**
 * @brief Set of callbacks to enable communication between client and server.
 * Can be used to define APIs for communuicating via arbitrary protocols, such as
 * CAN, SPI, UART, TCP/IP ...
 */
typedef struct rlog_ifc_s
{
    /**
     * @brief Function pointer to intialize communication interface. This can be a blocking function
     * if needs be.
     * 
     * @param arg Interface context data
     * @return true if succesfully initialzied the communication interface. 
     * @return false if failed.
     */
    bool (*init)(void* arg);

    /**
     * @brief Optional pointer to de-initialization function to be called at
     * thread termination
     * 
     * @param arg Interface context data
     */
    void (*deinit)(void* arg);

    /**
     * @brief Pointer to a non-blocking function to check if there is at leat 1 client waiting
     * to be served. 
     * Example: If interface is a connection oriented protocol (i.e TCP), it could be 
     * used to accept new connections on a non-blocking socket, or if is connectionless 
     * (i.e UDP, UART etc.) it could be used to wait for a client request or to send a ping.
     * 
     * @param arg Interface context data
     * @return true if found at least one client to send logs to
     * @return false if there are no clients to send logs to
     */
    bool (*poll)(void* arg);

    /**
     * @brief Pointer to a non-blocking function to send log messages to the client.
     * 
     * @param arg Interface context data
     * @param buf Buffer holding the message to be sent
     * @param len Length of the message in bytes
     * @return true if successfully sent the log to at least one client
     */
    bool (*send)(void* arg, const void* buf, int len);

    /**
     * @brief Pointer to arbitrary context data. Can be used as a "this" pointer
     * for using multiple instances of the same interface
     */
    void* ctx;

}rlog_ifc_t;

/**
 * @brief Default TCP Server interface
 */
extern rlog_ifc_t rlog_tcp_server_ifc;
#define RLOG_TCP_SERVER rlog_tcp_server_ifc

/**
 * @brief Default TCP Client interface
 * 
 */
extern rlog_ifc_t rlog_tcpcli_ifc;
#define RLOG_TCP_CLIENT rlog_tcpcli_ifc

/**
 * @brief STDOUT interface, use this if connected to the device
 * through some serial monitor or SSH, etc.
 */
extern rlog_ifc_t rlog_stdout;
#define RLOG_STDOUT rlog_stdout

/**
 * @brief Default UDP interface
 */
extern rlog_ifc_t rlog_udp_ifc;
#define RLOG_DEFAULT_UDP rlog_udp_ifc
 
/*
TODO: Add more interfaces..

example:

extern rlog_ifc_t rlog_default_uart_ifc;
#define RLOG_UART_IFC rlog_default_uart_ifc

*/

/**
 * @brief Default interface to be used for server and client communication.
 */
#define RLOG_DEFAULT_IFC RLOG_DEFAULT_TCPIP

#endif //_PORT_COM_INTERFACES_H_