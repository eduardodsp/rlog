#ifndef _PORT_COM_INTERFACES_H_
#define _PORT_COM_INTERFACES_H_

#define RLOG_COM_OK 0

/**
 * @brief Set of callbacks to enable communication between client and server.
 * Can be used to define APIs for communuicating via arbitrary protocols, such as
 * CAN, SPI, UART, TCP/IP ...
 * 
 */
typedef struct rlog_ifc_s
{
    /**
     * @brief Function pointer to intialize communication interface
     * @return RLOG_COM_OK on success, user defined error code on failure.
     */
    int (*init)(void);

    /**
     * @brief Function pointer wait for client connection. This can be a blocking function
     * waiting for a client connection if using a connection oriented protocol. If using a 
     * connection-less protocol this function can return immediately.
     * @return RLOG_COM_OK if successfully connected to client, user defined error code on failure.
     */
    int (*connect)(void);

    /**
     * @brief Send data to the client.
     * 
     * @param buf Buffer holding the message to be sent
     * @param len Length of the message in bytes
     * @return RLOG_COM_OK on success, user defined error code on failure.
     */
    int (*send)(const void* buf, int len);

    /**
     * @brief Get a string with client information
     * @return Pointer to constant string containing information about the client. It could be an IP address
     * or any other meaningful string.
     * May return NULL if no information is to be provided.
     */
    const char* (*get_cli)(void);

}rlog_ifc_t;

/**
 * @brief Default TCP/IP interface implementation
 */
extern rlog_ifc_t rlog_default_tcpip_ifc;
#define RLOG_TCPIP_IFC rlog_default_tcpip_ifc
 
/*
TODO: Add more interfaces..

example:

extern rlog_ifc_t rlog_default_uart_ifc;
#define RLOG_UART_IFC rlog_default_uart_ifc

*/

/**
 * @brief Default interface to be used for server and client communication.
 */
#define RLOG_DEFAULT_IFC RLOG_TCPIP_IFC

#endif //_PORT_COM_INTERFACES_H_