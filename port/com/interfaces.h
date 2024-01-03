#ifndef _PORT_COM_INTERFACES_H_
#define _PORT_COM_INTERFACES_H_

/**
 * @brief API was successful
 */
#define RLOG_COM_OK         1

/**
 * @brief API was successful and a new client has connected
 */
#define RLOG_COM_NEW_CLIENT 1

/**
 * @brief API was successful but no client has connected
 */
#define RLOG_COM_NO_CLIENT  0

/**
 * @brief Set of callbacks to enable communication between client and server.
 * Can be used to define APIs for communuicating via arbitrary protocols, such as
 * CAN, SPI, UART, TCP/IP ...
 */
typedef struct rlog_ifc_s
{
    /**
     * @brief Function pointer to intialize communication interface
     * @return RLOG_COM_OK on success. 
     * @return User defined error code on failure.
     */
    int (*init)(void);

    /**
     * @brief Pointer to a non-blocking function to check for client connection. 
     * @return RLOG_NEW_CLIENT if successfully connected to client, 
     * @return RLOG_NO_CLIENT if API is succesfull but no client is connected at the moment
     * @return User defined error code on API failure.
     */
    int (*connect)(void);

    /**
     * @brief Pointer to a non-blocking function to send data to the client.
     * 
     * @param buf Buffer holding the message to be sent
     * @param len Length of the message in bytes
     * @return RLOG_COM_OK on success. 
     * @return User defined error code on failure.
     */
    int (*send)(const void* buf, int len);

    /**
     * @brief Function pointer to get a string with client information
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