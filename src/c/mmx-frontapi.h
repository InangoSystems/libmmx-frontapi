/*  mmx-frontapi.h
 *
 * Copyright (c) 2013-2021 Inango Systems LTD.
 *
 * Author: Inango Systems LTD. <support@inango-systems.com>
 * Creation Date: 20 Sep 2013
 *
 * The author may be reached at support@inango-systems.com
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * Subject to the terms and conditions of this license, each copyright holder
 * and contributor hereby grants to those receiving rights under this license
 * a perpetual, worldwide, non-exclusive, no-charge, royalty-free, irrevocable
 * (except for failure to satisfy the conditions of this license) patent license
 * to make, have made, use, offer to sell, sell, import, and otherwise transfer
 * this software, where such license applies only to those patent claims, already
 * acquired or hereafter acquired, licensable by such copyright holder or contributor
 * that are necessarily infringed by:
 *
 * (a) their Contribution(s) (the licensed copyrights of copyright holders and
 * non-copyrightable additions of contributors, in source or binary form) alone;
 * or
 *
 * (b) combination of their Contribution(s) with the work of authorship to which
 * such Contribution(s) was added by such copyright holder or contributor, if,
 * at the time the Contribution is added, such addition causes such combination
 * to be necessarily infringed. The patent license shall not apply to any other
 * combinations which include the Contribution.
 *
 * Except as expressly stated above, no rights or licenses from any copyright
 * holder or contributor is granted under this license, whether expressly, by
 * implication, estoppel or otherwise.
 *
 * DISCLAIMER
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * NOTE
 *
 * This is part of a management middleware software package called MMX that was developed by Inango Systems Ltd.
 *
 * This version of MMX provides web and command-line management interfaces.
 *
 * Please contact us at Inango at support@inango-systems.com if you would like to hear more about
 * - other management packages, such as SNMP, TR-069 or Netconf
 * - how we can extend the data model to support all parts of your system
 * - professional sub-contract and customization services
 *
 */

/*
 * These functions are used by worker thread to parse and write XML data
 */

#ifndef MMX_FRONTAPI_H_
#define MMX_FRONTAPI_H_

#include <netinet/in.h> /* in_addr_t */
#include <syslog.h> /* log levels */

#include <microxml.h>

#include <ing_gen_utils.h>
#include <mmx-backapi-config.h>

/*
 * Socket configuration
 */
#define MMX_EP_ADDR       INADDR_LOOPBACK
#define MMX_EP_BE_ADDR    INADDR_ANY
#define MMX_EP_PORT       10100

/* Error messages */
#define FA_OK                  0
#define FA_GENERAL_ERROR       1
#define FA_INVALID_FORMAT      2
#define FA_BAD_INPUT_PARAMS    3
#define FA_NOT_ENOUGH_MEMORY   4

#define MMXFA_MAX_NUMBER_OF_ANY_OP_PARAMS 16
#define MMXFA_MAX_NUMBER_OF_MSG_STR_SETTYPE 64
#define MMXFA_MAX_NUMBER_OF_MMX_API_RC 64
#define MMXFA_MAX_NUMBER_OF_DELAY_SEC 8

/* Standard TR-069 fault codes 
   (used as respcode value in the front-api response header)  */
#define MMX_API_RC_OK                   0
  
#define MMX_API_RC_NOT_SUPPORTED        9000 //Method not supported
#define MMX_API_RC_REQUEST_DENIED       9001 //Request denied (no reason specified)
#define MMX_API_RC_INTERNAL_ERROR       9002 //Internal error
#define MMX_API_RC_INVALID_ARGUMENT     9003 //Invalid arguments 
#define MMX_API_RC_RESOURCES_EXCEEDED   9004 //Resources exceeded
#define MMX_API_RC_INVALID_PARAM_NAME   9005 //Invalid Parameter name
#define MMX_API_RC_INVALID_PARAM_TYPE   9006 //Invalid Parameter type
#define MMX_API_RC_INVALID_PARAM_VALUE  9007 //Invalid Parameter value (in SetValue request)
#define MMX_API_RC_NOT_WRITABLE         9008 //Attempt to set a non-writable parameter

/* Vendor-specific codes (9800 - 9899) */
#define MMX_API_RC_DB_ACCESS_ERROR      9810  //Cannot open DB file or DB does not exist
#define MMX_API_RC_DB_QUERY_ERROR       9811  //DB query failure
#define MMX_API_RC_INCORRECT_DB_TYPE    9812  //Incorrect DB type for the operation


#define MSG_SHORT_STR_LEN   8
#define MSG_MAX_STR_LEN     128
#define MSG_MAX_NUMBER_OF_GET_PARAMS     MMXBA_MAX_NUMBER_OF_GET_PARAMS
#define MSG_MAX_NUMBER_OF_SET_PARAMS     MMXBA_MAX_NUMBER_OF_SET_PARAMS
#define MSG_MAX_NUMBER_OF_ADDOBJ_PARAMS  MMXBA_MAX_NUMBER_OF_SET_PARAMS
#define MSG_MAX_NUMBER_OF_DELOBJ_PARAMS  16

#define MAX_NUMBER_OF_RESPONSE_VALUES      96
#define MAX_NUMBER_OF_GPN_RESPONSE_VALUES  128

/* Caller ID - type of frontend sending requests to MMX Entry-Point */
#define MMX_API_CALLERID_WEB      1
#define MMX_API_CALLERID_CLI      2
#define MMX_API_CALLERID_NETCONF  4
#define MMX_API_CALLERID_TR069    8
#define MMX_API_CALLERID_SNMP    16

/* Response mode */
#define MMX_API_RESPMODE_SYNC    0   // Blocking call of EP API request
#define MMX_API_RESPMODE_NOSYNC  1   // Non-blocking call of EP API request
#define MMX_API_RESPMODE_NORESP  2   // Response is not needed

/* Reset type */
#define MMX_API_RESETTYPE_FACTORY  0 // Full factory reset
#define MMX_API_RESETTYPE_KEEPIP   1 // Factory reset keeping device IP connectivity info
#define MMX_API_RESETTYPE_GHN      2 // Reset of Ghn configuration info only


//#define MAX_NUMBER_OF_PARAM_INDICES    5 /* Indices are numeric values inside param name */

#define MSG_TEMPLATE_RESPONSE \
    "<EP_ApiMsg>" \
        "<hdr>" \
            "<callerId></callerId>" \
            "<txaId></txaId>" \
            "<respFlag></respFlag>" \
            "<respMode></respMode>" \
            "<respPort></respPort>" \
            "<respIpAddr></respIpAddr>" \
            "<resCode></resCode>" \
            "<moreFlag></moreFlag>" \
            "<msgType></msgType>" \
            "<dbType></dbType>" \
        "</hdr>" \
        "<body>" \
        "</body>" \
    "</EP_ApiMsg>"

/* Request template
    <EP_ApiMsg>
        <hdr>
            <callerId></callerId>
            <txaId></txaId>
            <respFlag></respFlag>
            <respMode></respMode>
            <respIpAddr></respIpAddr>
            <respPort></respPort>
            <msgType></msgType>
            <flags></flags>
        </hdr>
        <body>
        </body>
    </EP_ApiMsg>
*/


/* Message tags */
#define MSG_STR_ROOT_NAME   "EP_ApiMsg"

/* Header */
#define MSG_STR_HEADER      "hdr"
#define MSG_STR_CALLERID    "callerId"
#define MSG_STR_TXAID       "txaId"
#define MSG_STR_RESPFLAG    "respFlag"
#define MSG_STR_RESPMODE    "respMode"
#define MSG_STR_RESPADDR    "respIpAddr"
#define MSG_STR_RESPPORT    "respPort"
#define MSG_STR_TYPE        "msgType"
#define MSG_STR_DBTYPE      "dbType"
#define MSG_STR_RESPCODE    "resCode"
#define MSG_STR_MOREFLAG    "moreFlag"

/* Body */
#define MSG_STR_BODY                "body"
#define MSG_STR_GETPARAMVALUE       "GetParamValue"
#define MSG_STR_GETPARAMVALUE_RESP  "GetParamValueResponse"
#define MSG_STR_SETPARAMVALUE       "SetParamValue"
#define MSG_STR_SETPARAMVALUE_RESP  "SetParamValueResponse"
#define MSG_STR_GETPARAMNAMES       "GetParamNames"
#define MSG_STR_GETPARAMNAMES_RESP  "GetParamNamesResponse"
#define MSG_STR_ADDOBJECT           "AddObject"
#define MSG_STR_ADDOBJECT_RESP      "AddObjectResponse"
#define MSG_STR_DELOBJECT           "DelObject"
#define MSG_STR_DELOBJECT_RESP      "DelObjectResponse"
#define MSG_STR_DISCOVERCONFIG      "DiscoverConfig"
#define MSG_STR_DISCOVERCONFIG_RESP "DiscoverConfigResponse"
#define MSG_STR_INITACTIONS         "InitActions"
#define MSG_STR_REBOOT              "Reboot"
#define MSG_STR_RESET               "FactoryReset"

#define MSG_STR_BACKENDNAME         "backendName"

#define MSG_STR_PARAMNAMES      "paramNames"
#define MSG_STR_PARAMVALUES     "paramValues"
#define MSG_STR_PARAMFAULTS     "paramFaults"
#define MSG_STR_PARAMFAULT      "paramFault"
#define MSG_STR_NAME            "name"
#define MSG_STR_NAMEVALUEPAIR   "nameValuePair"
#define MSG_STR_VALUE           "value"
#define MSG_STR_FAULTCODE       "faultcode"
#define MSG_STR_PATHNAME        "pathName"
#define MSG_STR_NEXTLEVEL       "nextLevel"
#define MSG_STR_CONFIGONLY      "configOnly"
#define MSG_STR_OBJNAME         "objName"
#define MSG_STR_OBJECTS         "objects"
#define MSG_STR_STATUS          "status"
#define MSG_STR_PARAMLIST       "paramList"
#define MSG_STR_PARAMINFO       "paramInfo"
#define MSG_STR_WRITABLE        "writable"
#define MSG_STR_DELAY_SEC       "delaySeconds"
#define MSG_STR_RESETTYPE       "resetType"
#define MSG_STR_INST_NUMBER     "objInstanceNumber"
#define MSG_STR_SETTYPE         "setType"

#define MSG_STR_ATTR_ARRAYSIZE  "arraySize"

typedef enum msgtype_e {
    MSGTYPE_ERR = -1,
    MSGTYPE_GETVALUE,
    MSGTYPE_GETVALUE_RESP,
    MSGTYPE_SETVALUE,
    MSGTYPE_SETVALUE_RESP,
    MSGTYPE_GETPARAMNAMES,
    MSGTYPE_GETPARAMNAMES_RESP,
    MSGTYPE_ADDOBJECT,
    MSGTYPE_ADDOBJECT_RESP,
    MSGTYPE_DELOBJECT,
    MSGTYPE_DELOBJECT_RESP,
    MSGTYPE_DISCOVERCONFIG,
    MSGTYPE_DISCOVERCONFIG_RESP,
    MSGTYPE_INITACTIONS,
    MSGTYPE_REBOOT,
    MSGTYPE_RESET,

    MSGTYPE_LAST
} msgtype_t;


typedef enum mmx_dstype {
    MMXDBTYPE_RUNNING = 0,
    MMXDBTYPE_STARTUP,
    MMXDBTYPE_CANDIDATE
} mmx_dbtype_t;

#define MSG_STR_DB_RUNNING     "running"
#define MSG_STR_DB_STARTUP     "startup"
#define MSG_STR_DB_CANDIDATE   "candidate"

typedef enum rsttype_e {
    RSTTYPE_FACTORY_RESET = 0,
    RSTTYPE_FACTORY_RESET_KEEPCONN
} rsttype_t;


#define MMX_SETTYPE_APPLY            1
#define MMX_SETTYPE_APPLY_SAVE       3
#define MMX_SETTYPE_TEST             4
#define MMX_SETTYPE_TEST_APPLY       5
#define MMX_SETTYPE_TEST_APPLY_SAVE  7

#define MMX_SETTYPE_FLAG_APPLY       0x1
#define MMX_SETTYPE_FLAG_SAVE        0x2
#define MMX_SETTYPE_FLAG_TEST        0x4

typedef struct ep_packet_s {
    char flags[8];
    char msg[0];    /* xml message */
} ep_packet_t;

/*
 * Type declaration for an element of ParameterList in GetParamNamesResponse
 */
typedef struct name_writable_pair_s {
    char name[MSG_MAX_STR_LEN];
    char writable[MSG_SHORT_STR_LEN];
} name_writable_pair_t;

/*
 * Type declaration for an element of ParameterFaults in setParamValueFaultResponse body
 */
typedef struct namefaultpair_s {
    char name[MSG_MAX_STR_LEN];
    int  faultcode;
} namefaultpair_t;

/*
 * Type declaration for an element of paramInfo array in getParamNamesResponse body
 */
typedef struct param_info_resp_s {
    char name[NVP_MAX_NAME_LEN];
    char writable;
} param_info_resp_t;

/* -----------------------------------------------
 * Entry-point request/response message structures
 * -------------------------------------------------
 */

/* Structures of Entry-point requests and responses */
typedef struct ep_getParamValue_req_s {
    char nextLevel;
    char configOnly;
    uint32_t arraySize;
    char paramNames[MSG_MAX_NUMBER_OF_GET_PARAMS][NVP_MAX_NAME_LEN];
} ep_getParamValue_req_t;

typedef struct ep_getParamValue_resp_s {
    uint32_t arraySize;
    uint32_t totalNVSize;
    nvpair_t paramValues[MAX_NUMBER_OF_RESPONSE_VALUES];
} ep_getParamValue_resp_t;

typedef struct ep_setParamValue_req_s {
    int setType;
    uint32_t arraySize;
    nvpair_t paramValues[MSG_MAX_NUMBER_OF_SET_PARAMS];
} ep_setParamValue_req_t;

typedef struct ep_setParamValue_resp_s {
    uint32_t status;
} ep_setParamValue_resp_t;

typedef struct ep_setParamValueFault_s {
    uint32_t arraySize;
    namefaultpair_t paramFaults[MSG_MAX_NUMBER_OF_SET_PARAMS];
} ep_setParamValueFault_t;

typedef struct  ep_getParamNames_req_s {
    char pathName[MSG_MAX_STR_LEN];
    char nextLevel;
} ep_getParamNames_req_t;

typedef struct ep_getParamNames_resp_s {
    uint32_t arraySize;
    param_info_resp_t paramInfo[MAX_NUMBER_OF_GPN_RESPONSE_VALUES];
} ep_getParamNames_resp_t;

typedef struct ep_addObject_req_s {
    char objName[MSG_MAX_STR_LEN];
    uint32_t arraySize;
    nvpair_t paramValues[MSG_MAX_NUMBER_OF_ADDOBJ_PARAMS];
} ep_addObject_req_t;

typedef struct ep_addObject_resp_s {
    uint32_t instanceNumber;
    uint32_t status;
} ep_addObject_resp_t;

typedef struct ep_delObject_req_s{
    uint32_t arraySize;
    char objects[MSG_MAX_NUMBER_OF_DELOBJ_PARAMS][MSG_MAX_STR_LEN];
} ep_delObject_req_t;

typedef struct ep_delObject_resp_s {
    uint32_t status;
} ep_delObject_resp_t;

typedef struct ep_discoverConfig_req_s {
    char backendName[MSG_MAX_STR_LEN];
    char objName[MSG_MAX_STR_LEN];
} ep_discoverConfig_req_t;

typedef struct ep_reboot_req_s {
    uint32_t delaySeconds;
} ep_reboot_req_t;

typedef struct ep_reset_req_s {
    uint32_t delaySeconds;
    uint32_t resetType;
} ep_reset_req_t;

/* Entry-point message header  */
typedef struct ep_msg_header_s {
    int callerId;
    int txaId;
    int respFlag;
    int respMode;
    in_addr_t respIpAddr;
    int respPort;
    msgtype_t msgType;
    mmx_dbtype_t mmxDbType;
    int respCode;
    char moreFlag;
} ep_msg_header_t;

/* Entry-point message body */
typedef union ep_msg_body_s {
    ep_getParamValue_req_t getParamValue;
    ep_getParamValue_resp_t getParamValueResponse;

    ep_setParamValue_req_t  setParamValue;
    ep_setParamValue_resp_t setParamValueResponse;
    ep_setParamValueFault_t setParamValueFaultResponse;

    ep_getParamNames_req_t getParamNames;
    ep_getParamNames_resp_t getParamNamesResponse;

    ep_addObject_req_t addObject;
    ep_addObject_resp_t addObjectResponse;

    ep_delObject_req_t delObject;
    ep_delObject_resp_t delObjectResponse;

    ep_discoverConfig_req_t discoverConfig;

    ep_reboot_req_t reboot;
    ep_reset_req_t  reset;

} ep_msg_body_t;


/* The structure defines the memory pool used for keeping values of
   name-value pairs of parameters passed in the ep message structure */
typedef struct ep_msg_mempool_s {
    int             initialized;
    unsigned short  size_bytes;
    unsigned short  curr_offset;
    char            *pool;
} ep_msg_mempool_t;

typedef struct ep_message_s {
    ep_msg_header_t    header;
    ep_msg_body_t      body;
    ep_msg_mempool_t   mem_pool;
} ep_message_t;

/*
 * Entry-point connection structure
 */
typedef struct mmx_ep_connection_s {
    int sock;
    struct sockaddr_in dest;
    unsigned sock_timeout;
} mmx_ep_connection_t;

/*
 * Opens connection to MMX Entry point
 */
int mmx_frontapi_connect(mmx_ep_connection_t *conn, in_port_t own_port, unsigned timeout);

/*
 * Sends request pkt to MMX Entry point
 */
int mmx_frontapi_send_req(mmx_ep_connection_t *conn, ep_packet_t *pkt);

/*
 * Receives response from MMX Entry point for previously sent request
 * with specified transaction Id (more reliable function)
 */
int mmx_frontapi_receive_resp(mmx_ep_connection_t *conn, int txaId,
                              char *buf, size_t buf_size, size_t *rcvd);

/*
 * Receives request's response from MMX Entry point
 */
int mmx_frontapi_receive(mmx_ep_connection_t *conn, char *buf, size_t buf_size, size_t *rcvd);

/*
 * Closes connection to MMX Entry point
 */
int mmx_frontapi_close(mmx_ep_connection_t *conn);

/*
 * Parses xml_string and fills message
 */
int mmx_frontapi_message_parse(const char *xml_string, ep_message_t *message);

/*
 * Parses xml_string and fills message header
 */
int mmx_frontapi_msg_header_parse(const char *xml_string, ep_msg_header_t *msg_header);

/*
 * Creates xml_string from the specified message
 */
int mmx_frontapi_message_build(ep_message_t *message, char *xml_string, size_t xml_string_size);


/* ********************************************************************* */
/*                            Shorthand API                              */
/* ********************************************************************* */

/*
 * Performs full sequence of actions for transmitting 'msg' to Entry point
 * and receiving answer from it (the answer is written into the same structure)
 * Output argument 'more' is set to value of moreFlag in response message
 * more = 0 means no more response messages are expected, this response is the last one
 * more = 1 - this response is not the last, more packets are expected
 */
int mmx_frontapi_make_request(mmx_ep_connection_t *conn, ep_message_t *msg, int *more);

/*
 * Sends pre-formed XML string to Entry point and receives XML answer which is
 * written into the same buffer
 */
int mmx_frontapi_make_xml_request(mmx_ep_connection_t *conn, char *xml_str,
                                  size_t xml_str_size, int *more);

/* ********************************************************************* */
/*                            Helpers                                    */
/* ********************************************************************* */

/*
 * Convert API message type string used in XML message header to number
 */
msgtype_t msgtype2num(const char *str);

/*
 * Convert API message type from number to string used in XML message header
 */
const char *msgtype2str(msgtype_t t);


/*
 * Convert MMX DB type string used in XML message header to number
 */
msgtype_t mmxdbtype_str2num(const char *str);

/*
 * Convert MMX DB type from number to string used in XML message header
 */
char *mmxdbtype_num2str(mmx_dbtype_t dbtype);


/* ******************************************************************** */
/*    API for processing memory pool of the frontend API message        */
/* ******************************************************************** */

/*
 *  Initialize front-api message structure (ep_message_t)
 *  The caller must supply memory buffer that will be used for keeping
 *  parameters values
 */
int mmx_frontapi_msg_struct_init (ep_message_t *message, char *mem_buff,
                                  unsigned short mem_buff_size);

 /*
  * Release front-api message structure (ep_message_t)
  */
int mmx_frontapi_msg_struct_release (ep_message_t *message);


/*
 * Insert value to the specified name-value pair of the message
 */
int mmx_frontapi_msg_struct_insert_value (ep_message_t *message,
                                          nvpair_t *nvPair, char *value);

/*
 * Insert name and value strings to the specified name-value pair
 * of the specified message structure
 */
int mmx_frontapi_msgstruct_insert_nvpair(ep_message_t *message,
                                         nvpair_t *nvPair,
                                         char *name, char *value);

#endif /* MMX_FRONTAPI_H_ */
