#ifndef _PORT_COM_INTERFACES_H_
#define _PORT_COM_INTERFACES_H_

#include <stdbool.h>

/**
 * @brief Set of callbacks to enable communication between client and server.
 * Can be used to define APIs for communuicating via arbitrary protocols, such as
 * CAN, SPI, UART, TCP/IP ...
 */
typedef struct rlog_ifc_s
{
    /**
     * @brief Function pointer to intialize communication interface
     * @return true if succesfully initialzied the communication interface. 
     * @return false if failed.
     */
    bool (*init)(void);

    /**
     * @brief Pointer to a non-blocking function to check if there is a link with a client. 
     * Example: If interface is a connection oriented protocol (i.e TCP), it could be 
     * used to accept new connections on a non-blocking socket, or if is connectionless 
     * (i.e UDP, UART etc.) it could be used to wait for a client request or to send a ping.
     * @return true if found a client to send logs to
     * @return false if there is no client to send logs to
     */
    bool (*poll)(void);

    /**
     * @brief Pointer to a non-blocking function to send log messages to the client.
     * 
     * @param buf Buffer holding the message to be sent
     * @param len Length of the message in bytes
     * @return true if successfully sent the log
     * @return false if failed to send the log
     */
    bool (*send)(const void* buf, int len);

}rlog_ifc_t;

/**
 * @brief Default TCP/IP interface implementation
 */
extern rlog_ifc_t rlog_default_tcpip_ifc;
#define RLOG_DEFAULT_TCPIP rlog_default_tcpip_ifc
 
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