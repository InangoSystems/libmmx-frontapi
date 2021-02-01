/* mmx-frontapi.c
 *
 * Copyright (c) 2013-2021 Inango Systems LTD.
 *
 * Author: Inango Systems LTD. <support@inango-systems.com>
 * Creation Date: Sep 2013
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
#include <arpa/inet.h>

#include "mmx-frontapi.h"
#include "ing_gen_utils.h"
#include <sys/time.h> // for gettimeofday function

#define FA_BUF_SIZE     2048

#define GOTO_RET_WITH_ERROR(err_num, msg, ...)     do { \
    ing_log(LOG_ERR, msg"\n", ##__VA_ARGS__); \
    status = err_num; \
    goto ret; \
} while (0)

#define XML_GET_INT(tree, name, to)     do { \
    mxml_node_t *node = mxmlFindElement(tree, tree, name, NULL, NULL, MXML_DESCEND); \
    if (node == NULL) \
        GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT, "Could not find tag `%s'", name); \
    char *s = (char *)mxmlGetOpaque(node); \
    to = atoi(s ? s : "0"); \
} while (0)

#define XML_GET_POSITIVE_OR_NULL_INT(tree, name, to)     do { \
    mxml_node_t *node = mxmlFindElement(tree, tree, name, NULL, NULL, MXML_DESCEND); \
    if (node == NULL) \
        GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT, "Could not find tag `%s'", name); \
    char *s = (char *)mxmlGetOpaque(node); \
    to = atoi(s ? s : "0");\
    if ((int)to < 0) to = 0; \
} while (0)

#define XML_GET_TEXT(node, tree, name, to, size_to, ignore_err)    do { \
    mxml_node_t *n = mxmlFindElement(node, tree, name, NULL, NULL, MXML_DESCEND); \
    if (n == NULL) { \
        if (ignore_err) \
            memset(to, 0, size_to); \
        else \
            GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT, "Could not find tag `%s'", name); \
    } \
    else { \
        char *s = (char *)mxmlGetOpaque(n); \
        strncpy(to, s ? s : "", size_to-1); \
    }\
} while (0)

#define XML_GET_NODE(node, tree, name, to)    do { \
    to = mxmlFindElement(node, tree, name, NULL, NULL, MXML_DESCEND); \
    if (to == NULL) \
        GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT, "Could not find tag `%s'", name); \
} while (0)

#define XML_WRITE_INT(node, tree, name, value)     do { \
    node = mxmlFindElement(node, tree, name, NULL, NULL, MXML_DESCEND); \
    if (node == NULL) \
        GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT, "Could not find tag `%s'", name); \
    node = mxmlNewInteger(node, value); \
    if (!node) \
        GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT, "Could not write `%s'", name); \
} while (0)

#define XML_WRITE_TEXT(node, tree, name, value)     do { \
    node = mxmlFindElement(node, tree, name, NULL, NULL, MXML_DESCEND); \
    if (node == NULL) \
        GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT, "Could not find tag `%s'", name); \
    node = mxmlNewText(node, 0, value); \
    if (!node) \
        GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT, "Could not write `%s'", name); \
} while (0)

static char bool2num(const char *str)
{
    if (!strcmp(str, "TRUE") || !strcmp(str, "True") ||
        !strcmp(str, "true") || !strcmp(str, "1"))
        return 1;
    else
        return 0;
}

static const char *bool2str(char flag)
{
    return flag ? "true" : "false";
}

static int xml_parse_body_getvalue(ep_message_t *message, mxml_node_t *tree)
{
    int  status = FA_OK;
    int  i = 0;
    char *s = NULL;
    char *arraySizeStr;
    long int arraySize;

    mxml_node_t *node = NULL, *subnode = NULL;
    XML_GET_NODE(tree, tree, MSG_STR_GETPARAMVALUE, node);

    subnode = mxmlFindElement(node, tree, MSG_STR_NEXTLEVEL, NULL, NULL, MXML_DESCEND);
    s = (char *)mxmlGetOpaque(subnode);
    if (s != NULL) 
        message->body.getParamValue.nextLevel = bool2num(s);

    subnode = mxmlFindElement(node, tree, MSG_STR_CONFIGONLY, NULL, NULL, MXML_DESCEND);
    s = (char *)mxmlGetOpaque(subnode);
    if (s != NULL) 
        message->body.getParamValue.configOnly = bool2num(s);
        
    XML_GET_NODE(node, tree, MSG_STR_PARAMNAMES, node);
       
    arraySizeStr = (char *)mxmlElementGetAttrValue(node, MSG_STR_ATTR_ARRAYSIZE);
    if (arraySizeStr == NULL)
        GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT, "Attribute `%s' in not set",
                                                            MSG_STR_ATTR_ARRAYSIZE);

    arraySize = strtol(arraySizeStr, NULL, 10);
    if (arraySize <= 0 || arraySize > MSG_MAX_NUMBER_OF_GET_PARAMS)
        GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT,
          "Incorrect value of attribute %s - %d (max value is %d)",
              MSG_STR_ATTR_ARRAYSIZE, arraySize, MSG_MAX_NUMBER_OF_GET_PARAMS);

    message->body.getParamValue.arraySize = arraySize;

    /* save all names */
    for (node = mxmlFindElement(node, tree, MSG_STR_NAME, NULL, NULL, MXML_DESCEND);
            node != NULL && i < arraySize;
            node = mxmlFindElement(node, tree, MSG_STR_NAME, NULL, NULL, MXML_DESCEND), i++)
    {
        s = (char *)mxmlGetOpaque(node);
        strncpy(message->body.getParamValue.paramNames[i], s ? s : "", NVP_MAX_NAME_LEN);
    }
    if (i != arraySize)
        GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT, "Number of parameters does not match arraySize attribute");

ret:
    return status;
}

msgtype_t msgtype2num(const char *str)
{
    if (!strcmp(str, MSG_STR_GETPARAMVALUE)) return MSGTYPE_GETVALUE;
    else if (!strcmp(str, MSG_STR_GETPARAMVALUE_RESP)) return MSGTYPE_GETVALUE_RESP;
    else if (!strcmp(str, MSG_STR_SETPARAMVALUE)) return MSGTYPE_SETVALUE;
    else if (!strcmp(str, MSG_STR_SETPARAMVALUE_RESP)) return MSGTYPE_SETVALUE_RESP;
    else if (!strcmp(str, MSG_STR_GETPARAMNAMES)) return MSGTYPE_GETPARAMNAMES;
    else if (!strcmp(str, MSG_STR_GETPARAMNAMES_RESP)) return MSGTYPE_GETPARAMNAMES_RESP;
    else if (!strcmp(str, MSG_STR_ADDOBJECT)) return MSGTYPE_ADDOBJECT;
    else if (!strcmp(str, MSG_STR_ADDOBJECT_RESP)) return MSGTYPE_ADDOBJECT_RESP;
    else if (!strcmp(str, MSG_STR_DELOBJECT)) return MSGTYPE_DELOBJECT;
    else if (!strcmp(str, MSG_STR_DELOBJECT_RESP)) return MSGTYPE_DELOBJECT_RESP;
    else if (!strcmp(str, MSG_STR_DISCOVERCONFIG)) return MSGTYPE_DISCOVERCONFIG;
    else if (!strcmp(str, MSG_STR_DISCOVERCONFIG_RESP)) return MSGTYPE_DISCOVERCONFIG_RESP;
    else if (!strcmp(str, MSG_STR_INITACTIONS)) return MSGTYPE_INITACTIONS;
    else if (!strcmp(str, MSG_STR_REBOOT)) return MSGTYPE_REBOOT;
    else if (!strcmp(str, MSG_STR_RESET)) return MSGTYPE_RESET;

    return MSGTYPE_ERR;
}

const char *msgtype2str(msgtype_t t)
{
    switch (t)
    {
    case MSGTYPE_GETVALUE:       return MSG_STR_GETPARAMVALUE;
    case MSGTYPE_GETVALUE_RESP:  return MSG_STR_GETPARAMVALUE_RESP;
    case MSGTYPE_SETVALUE:       return MSG_STR_SETPARAMVALUE;
    case MSGTYPE_SETVALUE_RESP:  return MSG_STR_SETPARAMVALUE_RESP;
    case MSGTYPE_GETPARAMNAMES:  return MSG_STR_GETPARAMNAMES;
    case MSGTYPE_GETPARAMNAMES_RESP: return MSG_STR_GETPARAMNAMES_RESP;
    case MSGTYPE_ADDOBJECT:      return MSG_STR_ADDOBJECT;
    case MSGTYPE_ADDOBJECT_RESP: return MSG_STR_ADDOBJECT_RESP;
    case MSGTYPE_DELOBJECT:      return MSG_STR_DELOBJECT;
    case MSGTYPE_DELOBJECT_RESP: return MSG_STR_DELOBJECT_RESP;
    case MSGTYPE_DISCOVERCONFIG: return MSG_STR_DISCOVERCONFIG;
    case MSGTYPE_DISCOVERCONFIG_RESP: return MSG_STR_DISCOVERCONFIG_RESP;
    case MSGTYPE_INITACTIONS:    return MSG_STR_INITACTIONS;
    case MSGTYPE_REBOOT:         return MSG_STR_REBOOT;
    case MSGTYPE_RESET:          return MSG_STR_RESET;
    /* TODO the rest */
    default: return "Unknown";
    }
}


int mmxdbtype_str2num(const char *str)
{
    mmx_dbtype_t  dbtype = MMXDBTYPE_RUNNING;  //the default is the running db
    
    if (strlen(str))
    {
        if (!strcmp(str, MSG_STR_DB_STARTUP)) 
            dbtype = MMXDBTYPE_STARTUP;
        else if (!strcmp(str, MSG_STR_DB_CANDIDATE)) 
            dbtype =  MMXDBTYPE_CANDIDATE;
    }
    
    return dbtype;
}

char * mmxdbtype_num2str(const mmx_dbtype_t dbtype)
{    
    switch (dbtype)
    {
        case MMXDBTYPE_STARTUP:    return MSG_STR_DB_STARTUP;
        case MMXDBTYPE_CANDIDATE:  return MSG_STR_DB_CANDIDATE;
        default:                   return MSG_STR_DB_RUNNING;
    }
}

static int xml_parse_body_getvalue_resp(ep_message_t *message, mxml_node_t *tree)
{
    int status = FA_OK, res = FA_OK;
    int i = 0;
    long int arraySize;
    const char *arraySizeStr;
    char *s, *name;

    mxml_node_t *node = NULL, *subnode = NULL;
    
    if(!message->mem_pool.initialized)
        GOTO_RET_WITH_ERROR(FA_BAD_INPUT_PARAMS,
                           "Message struct memory pool is not initialized");
    
    XML_GET_NODE(tree, tree, MSG_STR_GETPARAMVALUE_RESP, node);
    XML_GET_NODE(node, tree, MSG_STR_PARAMVALUES, node);

    arraySizeStr = mxmlElementGetAttrValue(node, MSG_STR_ATTR_ARRAYSIZE);
    if (arraySizeStr == NULL)
        GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT, "Attribute `%s' in not set",
                                                        MSG_STR_ATTR_ARRAYSIZE);

    arraySize = strtol(arraySizeStr, NULL, 10);
    if (arraySize < 0 || arraySize > MAX_NUMBER_OF_RESPONSE_VALUES)
        GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT,
        "Incorrect value of attribute %s - %d (max value is %d)",
            MSG_STR_ATTR_ARRAYSIZE, arraySize, MAX_NUMBER_OF_RESPONSE_VALUES);

    message->body.getParamValueResponse.arraySize = arraySize;

    /* save all names */
    i = 0;
    for (node = mxmlFindElement(node, tree, MSG_STR_NAMEVALUEPAIR, NULL, NULL, MXML_DESCEND);
         node != NULL && i < arraySize;
         node = mxmlFindElement(node, tree, MSG_STR_NAMEVALUEPAIR, NULL, NULL, MXML_DESCEND), i++)
    {
        subnode = mxmlFindElement(node, tree, MSG_STR_NAME, NULL, NULL, MXML_DESCEND);
        if (!subnode)
            GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT, "Incorrect syntax: pair name missing");
        name = (char *)mxmlGetOpaque(subnode);

        subnode = mxmlFindElement(node, tree, MSG_STR_VALUE, NULL, NULL, MXML_DESCEND);
        if (!subnode)
            GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT, "Incorrect syntax: pair value missing");
        s = (char*)mxmlGetOpaque(subnode);

        res = mmx_frontapi_msg_struct_insert_value(message, 
                    &(message->body.getParamValueResponse.paramValues[i]),
                     s ? s : "");
        if (res != FA_OK)
            GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT, 
                "Not enough space in the pool for param %s (size %d)",name, s ? s : "");

        strncpy(message->body.getParamValueResponse.paramValues[i].name, 
                                           name ? name : "", NVP_MAX_NAME_LEN);   
    }
    if (i != arraySize)
        GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT, "Number of parameters does not match arraySize attribute");

ret:
    return status;
}

#define TR_069_SET_FAULTCODE_FROM  9000
#define TR_069_SET_FAULTCODE_TO    9008

static int xml_parse_body_setvalue_resp(ep_message_t *message, mxml_node_t *tree)
{
    int status = FA_OK;
    int i = 0, faultcode = 0;
    char *s;
    long int arraySize;
    const char *arraySizeStr;

    mxml_node_t *node = NULL, *subnode = NULL;

    /* If entry-point have processed request successfully we place in response body
       SET operation status, otherwise per parameter fault elements will be added */
    if (message->header.respCode == 0)
    {
        XML_GET_NODE(tree, tree, MSG_STR_SETPARAMVALUE_RESP, node);
        XML_GET_NODE(node, tree, MSG_STR_STATUS, node);

        s = (char *)mxmlGetOpaque(node);
        if ( (!strcmp(trim(s), "0")) || (!strcmp(trim(s), "1")) )
        {
            message->body.setParamValueResponse.status = atoi( trim(s) );
        }
        else
        {
            ing_log(LOG_DEBUG, "Incorrect value of status: %s (expected 0 or 1)\n", s);
            message->body.setParamValueResponse.status = 1;
        }
    }
    else
    {
        XML_GET_NODE(tree, tree, MSG_STR_SETPARAMVALUE_RESP, node);
        XML_GET_NODE(node, tree, MSG_STR_PARAMFAULTS, node);

        arraySizeStr = mxmlElementGetAttrValue(node, MSG_STR_ATTR_ARRAYSIZE);
        if (arraySizeStr == NULL)
            GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT, "Attribute `%s' in not set",
                                                        MSG_STR_ATTR_ARRAYSIZE);

        arraySize = strtol(arraySizeStr, NULL, 10);
        if (arraySize <= 0 || arraySize > MSG_MAX_NUMBER_OF_SET_PARAMS)
            GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT,
            "Incorrect value of attribute %s - %d (max value %d)", 
               MSG_STR_ATTR_ARRAYSIZE,arraySize,MSG_MAX_NUMBER_OF_SET_PARAMS);

        message->body.setParamValueFaultResponse.arraySize = arraySize;

        /* save all names & faultcodes */
        i = 0;
        for (node = mxmlFindElement(node, tree, MSG_STR_PARAMFAULT, NULL, NULL, MXML_DESCEND);
                node != NULL && i < arraySize;
                node = mxmlFindElement(node, tree, MSG_STR_PARAMFAULT, NULL, NULL, MXML_DESCEND), i++)
        {
            subnode = mxmlFindElement(node, tree, MSG_STR_NAME, NULL, NULL, MXML_DESCEND);
            if (!subnode)
                GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT, "Incorrect syntax: pair name missing");
            s = (char *)mxmlGetOpaque(subnode);
            strncpy(message->body.setParamValueFaultResponse.paramFaults[i].name, s ? s : "", NVP_MAX_NAME_LEN);
            
            subnode = mxmlFindElement(node, tree, MSG_STR_FAULTCODE, NULL, NULL, MXML_DESCEND);
            if (!subnode)
                GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT, "Incorrect syntax: pair faultcode missing");
            s = (char *)mxmlGetOpaque(subnode);
            faultcode = strtol(s, NULL, 10);
            if (faultcode <= TR_069_SET_FAULTCODE_FROM || faultcode > TR_069_SET_FAULTCODE_TO)
                ing_log(LOG_DEBUG, "Value of faultcode: %d (expected to be between %d and %d)\n",
                        faultcode, TR_069_SET_FAULTCODE_FROM, TR_069_SET_FAULTCODE_TO);
            message->body.setParamValueFaultResponse.paramFaults[i].faultcode = faultcode;
            // Debug prints
            //ing_log(LOG_DEBUG, "paramFault #%d %s = %d\n", i, message->body.setParamValueFaultResponse.paramFaults[i].name, message->body.setParamValueFaultResponse.paramFaults[i].faultcode);
        }
    }

ret:
    return status;
}

static int xml_parse_body_setvalue(ep_message_t *message, mxml_node_t *tree)
{
    int status = FA_OK, res = FA_OK;
    int i = 0;
    long int arraySize;
    char *s, *name;
    const char *arraySizeStr;

    mxml_node_t *node = NULL, *subnode = NULL;
    
    if(!message->mem_pool.initialized)
        GOTO_RET_WITH_ERROR(FA_BAD_INPUT_PARAMS,
              "Message struct memory pool is not initialized for setValue");
    
    XML_GET_NODE(tree, tree, MSG_STR_SETPARAMVALUE, node);

    XML_GET_INT(tree, MSG_STR_SETTYPE, message->body.setParamValue.setType);

    XML_GET_NODE(node, tree, MSG_STR_PARAMVALUES, node);

    arraySizeStr = mxmlElementGetAttrValue(node, MSG_STR_ATTR_ARRAYSIZE);
    if (arraySizeStr == NULL)
        GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT, "Attribute `%s' in not set",
                                                        MSG_STR_ATTR_ARRAYSIZE);

    arraySize = strtol(arraySizeStr, NULL, 10);
    if (arraySize <= 0 || arraySize > MSG_MAX_NUMBER_OF_SET_PARAMS)
        GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT,
        "Incorrect value of attribute %s - %d (max value is %d)",
            MSG_STR_ATTR_ARRAYSIZE, arraySize, MSG_MAX_NUMBER_OF_SET_PARAMS);

    message->body.setParamValue.arraySize = arraySize;

    /* save all names */
    i = 0;
    for (node = mxmlFindElement(node, tree, MSG_STR_NAMEVALUEPAIR, NULL, NULL, MXML_DESCEND);
            node != NULL && i < arraySize;
            node = mxmlFindElement(node, tree, MSG_STR_NAMEVALUEPAIR, NULL, NULL, MXML_DESCEND), i++)
    {
        subnode = mxmlFindElement(node, tree, MSG_STR_NAME, NULL, NULL, MXML_DESCEND);
        if (!subnode)
            GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT, "Incorrect syntax: pair name missing");
        name = (char *)mxmlGetOpaque(subnode);
        
        subnode = mxmlFindElement(node, tree, MSG_STR_VALUE, NULL, NULL, MXML_DESCEND);
        if (!subnode)
            GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT, "Incorrect syntax: pair value missing");
        s = (char *)mxmlGetOpaque(subnode);
        res = mmx_frontapi_msg_struct_insert_value(message, 
                     &(message->body.setParamValue.paramValues[i]), s ? s : "");
        if (res != FA_OK)
            GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT, 
                "Not enough space in the pool for param %s (size %d)",name, s ? s : "");

        strncpy(message->body.setParamValue.paramValues[i].name, name ? name : "",
                                                               NVP_MAX_NAME_LEN);
    }
    if (i != arraySize)
        GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT, "Number of parameters does not match arraySize attribute");

ret:
    return status;
}

static int xml_parse_body_getparamnames(ep_message_t *message, mxml_node_t *tree)
{
    int status = FA_OK;
    const char *s;
    mxml_node_t *node = NULL, *subnode = NULL;

    XML_GET_NODE(tree, tree, MSG_STR_GETPARAMNAMES, node);
    XML_GET_TEXT(node, tree, MSG_STR_PATHNAME, message->body.getParamNames.pathName,
                                  sizeof(message->body.getParamNames.pathName), FALSE);
    XML_GET_NODE(node, tree, MSG_STR_NEXTLEVEL, subnode);

    s = (char *)mxmlGetOpaque(subnode);
    message->body.getParamNames.nextLevel = bool2num(s);

ret:
    return status;
}

static int xml_parse_body_getparamnames_resp(ep_message_t *message, mxml_node_t *tree)
{
    int status = FA_OK;
    int i = 0;
    long int arraySize;
    char *s;
    const char *arraySizeStr;

    mxml_node_t *node = NULL, *subnode = NULL;
    XML_GET_NODE(tree, tree, MSG_STR_GETPARAMNAMES_RESP, node);
    XML_GET_NODE(tree, tree, MSG_STR_PARAMLIST, node);

    arraySizeStr = mxmlElementGetAttrValue(node, MSG_STR_ATTR_ARRAYSIZE);
    if (arraySizeStr == NULL)
        GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT, "Attribute `%s' in not set",
                                                        MSG_STR_ATTR_ARRAYSIZE);
    arraySize = strtol(arraySizeStr, NULL, 10);
    if (arraySize < 0 || arraySize > MAX_NUMBER_OF_GPN_RESPONSE_VALUES)
        GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT,
        "Incorrect value of attribute %s - %d (max value is %d)", 
           MSG_STR_ATTR_ARRAYSIZE,arraySize,MAX_NUMBER_OF_GPN_RESPONSE_VALUES);

    message->body.getParamNamesResponse.arraySize = arraySize;

    /* save all names */
    i = 0;
    for (node = mxmlFindElement(node, tree, MSG_STR_PARAMINFO, NULL, NULL, MXML_DESCEND);
            node != NULL && i < arraySize;
            node = mxmlFindElement(node, tree, MSG_STR_PARAMINFO, NULL, NULL, MXML_DESCEND), i++)
    {
        subnode = mxmlFindElement(node, tree, MSG_STR_NAME, NULL, NULL, MXML_DESCEND);
        if (!subnode)
            GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT, "Incorrect syntax: pair name missing");
        s = (char *)mxmlGetOpaque(subnode);
        strncpy(message->body.getParamNamesResponse.paramInfo[i].name, s ? s : "",
                                                               NVP_MAX_NAME_LEN);
        subnode = mxmlFindElement(node, tree, MSG_STR_WRITABLE, NULL, NULL, MXML_DESCEND);
        if (!subnode)
            GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT, "Incorrect syntax: pair writable missing");
        s = (char *)mxmlGetOpaque(subnode);
        message->body.getParamNamesResponse.paramInfo[i].writable = bool2num(s);

    }
    if (i != arraySize)
        GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT, "Number of parameters does not match arraySize attribute");

ret:
    return status;
}

static int xml_parse_body_addobject(ep_message_t *message, mxml_node_t *tree)
{
    int status = FA_OK, res = FA_OK;
    int i = 0;
    long int arraySize;
    char *s, *name;
    const char *arraySizeStr;

    mxml_node_t *node = NULL, *subnode = NULL;
    
    if(!message->mem_pool.initialized)
        GOTO_RET_WITH_ERROR(FA_BAD_INPUT_PARAMS,
              "Message struct memory pool is not initialized for addObj");
    
    XML_GET_NODE(tree, tree, MSG_STR_ADDOBJECT, node);
    XML_GET_TEXT(node, tree, MSG_STR_OBJNAME, message->body.addObject.objName,
            sizeof(message->body.addObject.objName), FALSE);

    XML_GET_NODE(node, tree, MSG_STR_PARAMVALUES, node);

    arraySizeStr = mxmlElementGetAttrValue(node, MSG_STR_ATTR_ARRAYSIZE);
    if (arraySizeStr == NULL)
        GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT, "Attribute `%s' in not set",
                                                        MSG_STR_ATTR_ARRAYSIZE);

    arraySize = strtol(arraySizeStr, NULL, 10);
    if (arraySize < 0 || arraySize > MSG_MAX_NUMBER_OF_ADDOBJ_PARAMS)
        GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT,
        "Incorrect value of attribute `%s' - %d (max value is %d)", 
           MSG_STR_ATTR_ARRAYSIZE,arraySize,MSG_MAX_NUMBER_OF_ADDOBJ_PARAMS);

    message->body.addObject.arraySize = arraySize;

    /* save all names */
    i = 0;
    for (node = mxmlFindElement(node, tree, MSG_STR_NAMEVALUEPAIR, NULL, NULL, MXML_DESCEND);
            node != NULL && i < arraySize;
            node = mxmlFindElement(node, tree, MSG_STR_NAMEVALUEPAIR, NULL, NULL, MXML_DESCEND), i++)
    {
        subnode = mxmlFindElement(node, tree, MSG_STR_NAME, NULL, NULL, MXML_DESCEND);
        if (!subnode)
            GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT, "Incorrect syntax: pair name missing");
        name = (char *)mxmlGetOpaque(subnode);
        
        subnode = mxmlFindElement(node, tree, MSG_STR_VALUE, NULL, NULL, MXML_DESCEND);
        if (!subnode)
            GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT, "Incorrect syntax: pair value missing");
        s = (char *)mxmlGetOpaque(subnode);
        res = mmx_frontapi_msg_struct_insert_value(message, 
                      &(message->body.addObject.paramValues[i]), s ? s : "");
        if (res != FA_OK)
            GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT, 
                "Not enough space in the pool for param %s (size %d)",name, s ? s : "");

        strncpy(message->body.addObject.paramValues[i].name, name ? name : "",
                                                               NVP_MAX_NAME_LEN);
    }
    if (i != arraySize)
        GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT, "Number of parameters does not match arraySize attribute");

ret:
    return status;
}

static int xml_parse_body_addobject_resp(ep_message_t *message, mxml_node_t *tree)
{
    int status = FA_OK;
    mxml_node_t *node = NULL;

    XML_GET_NODE(tree, tree, MSG_STR_ADDOBJECT_RESP, node);

    XML_GET_INT(tree, MSG_STR_INST_NUMBER, message->body.addObjectResponse.instanceNumber);
    XML_GET_INT(tree, MSG_STR_STATUS, message->body.addObjectResponse.status);

ret:
    return status;
}

static int xml_parse_body_delobject(ep_message_t *message, mxml_node_t *tree)
{
    int status = FA_OK;
    mxml_node_t *node = NULL;
    XML_GET_NODE(tree, tree, MSG_STR_DELOBJECT, node);
    XML_GET_NODE(node, tree, MSG_STR_OBJECTS, node);

    const char *arraySizeStr = mxmlElementGetAttrValue(node, MSG_STR_ATTR_ARRAYSIZE);
    if (arraySizeStr == NULL)
        GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT, "Attribute `%s' in not set",
                                                            MSG_STR_ATTR_ARRAYSIZE);

    long int arraySize = strtol(arraySizeStr, NULL, 10);
    if (arraySize <= 0 || arraySize > MSG_MAX_NUMBER_OF_DELOBJ_PARAMS)
        GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT,
        "Incorrect value of attribute %s - %d (max value is %d)",
            MSG_STR_ATTR_ARRAYSIZE,arraySize,MSG_MAX_NUMBER_OF_DELOBJ_PARAMS);

    message->body.delObject.arraySize = arraySize;

    /* save all names */
    int i = 0;
    for (node = mxmlFindElement(node, tree, MSG_STR_OBJNAME, NULL, NULL, MXML_DESCEND);
            node != NULL && i < arraySize;
            node = mxmlFindElement(node, tree, MSG_STR_OBJNAME, NULL, NULL, MXML_DESCEND), i++)
    {
        char *s = (char *)mxmlGetOpaque(node);
        strncpy(message->body.delObject.objects[i], s ? s : "", MSG_MAX_STR_LEN);
    }
    if (i != arraySize)
        GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT, "Number of parameters does not match arraySize attribute");

ret:
    return status;
}

static int xml_parse_body_delobject_resp(ep_message_t *message, mxml_node_t *tree)
{
    int status = FA_OK;
    mxml_node_t *node = NULL;

    XML_GET_NODE(tree, tree, MSG_STR_DELOBJECT_RESP, node);

    XML_GET_INT(tree, MSG_STR_STATUS, message->body.delObjectResponse.status);

ret:
    return status;
}

static int xml_parse_body_discoverconfig(ep_message_t *message, mxml_node_t *tree)
{
    int status = FA_OK;
    mxml_node_t *node = NULL;
    XML_GET_NODE(tree, tree, MSG_STR_DISCOVERCONFIG, node);
    XML_GET_TEXT(node, tree, MSG_STR_BACKENDNAME, message->body.discoverConfig.backendName,
            sizeof(message->body.discoverConfig.backendName), FALSE);
    XML_GET_TEXT(node, tree, MSG_STR_OBJNAME, message->body.discoverConfig.objName,
            sizeof(message->body.discoverConfig.objName), FALSE);

ret:
    return status;
}

static int xml_parse_body_reboot(ep_message_t *message, mxml_node_t *tree)
{
    int status = FA_OK;
    mxml_node_t *node = NULL;

    XML_GET_NODE(tree, tree, MSG_STR_REBOOT, node);
    if (mxmlFindElement(node, tree, MSG_STR_DELAY_SEC, NULL, NULL, MXML_DESCEND))
    {
        XML_GET_POSITIVE_OR_NULL_INT(tree, MSG_STR_DELAY_SEC, message->body.reboot.delaySeconds);
    }

ret:
    return status;
}

static int xml_parse_body_reset(ep_message_t *message, mxml_node_t *tree)
{
    int status = FA_OK;
    mxml_node_t *node = NULL;

    XML_GET_NODE(tree, tree, MSG_STR_RESET, node);
    if (mxmlFindElement(node, tree, MSG_STR_RESETTYPE, NULL, NULL, MXML_DESCEND))
    {
        XML_GET_POSITIVE_OR_NULL_INT(tree, MSG_STR_RESETTYPE, message->body.reset.resetType);
    }
    if (mxmlFindElement(node, tree, MSG_STR_DELAY_SEC, NULL, NULL, MXML_DESCEND))
    {
        XML_GET_POSITIVE_OR_NULL_INT(tree, MSG_STR_DELAY_SEC, message->body.reset.delaySeconds);
    }

ret:
    return status;
}

int mmx_frontapi_message_parse(const char *xmlmsg, ep_message_t *message)
{
    int status = FA_OK;
    char buf[MSG_MAX_STR_LEN];

    mxml_node_t *tree = mxmlLoadString(NULL, xmlmsg, MXML_OPAQUE_CALLBACK);

    /* Handle header */
    if (!tree || mxmlGetElement(tree) == NULL || strcmp(mxmlGetElement(tree), MSG_STR_ROOT_NAME))
        GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT, "Incorrect format of the message");

    XML_GET_INT(tree, MSG_STR_CALLERID, message->header.callerId);
    XML_GET_INT(tree, MSG_STR_TXAID, message->header.txaId);
    XML_GET_INT(tree, MSG_STR_RESPFLAG, message->header.respFlag);
    XML_GET_INT(tree, MSG_STR_RESPMODE, message->header.respMode);
    XML_GET_INT(tree, MSG_STR_RESPPORT, message->header.respPort);

    XML_GET_TEXT(tree, tree, MSG_STR_RESPADDR, buf, sizeof(buf), FALSE);
    if (!inet_aton(buf, (struct in_addr *)&(message->header.respIpAddr)))
        GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT, "Could not parse IP address");

    if (mxmlFindElement(tree, tree, MSG_STR_RESPCODE, NULL, NULL, MXML_DESCEND))
    {
        XML_GET_INT(tree, MSG_STR_RESPCODE, message->header.respCode);
    }

    if (mxmlFindElement(tree, tree, MSG_STR_MOREFLAG, NULL, NULL, MXML_DESCEND))
    {
        XML_GET_INT(tree, MSG_STR_MOREFLAG, message->header.moreFlag);
    }

    XML_GET_TEXT(tree, tree, MSG_STR_TYPE, buf, sizeof(buf), FALSE);
    message->header.msgType = msgtype2num(buf);
    
    XML_GET_TEXT(tree, tree, MSG_STR_DBTYPE, buf, sizeof(buf), TRUE);
    message->header.mmxDbType = mmxdbtype_str2num(buf);


    /* Handle body */
    switch (message->header.msgType)
    {
    case MSGTYPE_GETVALUE: status = xml_parse_body_getvalue(message, tree); break;
    case MSGTYPE_GETVALUE_RESP: status = xml_parse_body_getvalue_resp(message, tree); break;
    case MSGTYPE_SETVALUE: status = xml_parse_body_setvalue(message, tree); break;
    case MSGTYPE_SETVALUE_RESP: status = xml_parse_body_setvalue_resp(message, tree); break;
    case MSGTYPE_GETPARAMNAMES: status = xml_parse_body_getparamnames(message, tree); break;
    case MSGTYPE_GETPARAMNAMES_RESP: status = xml_parse_body_getparamnames_resp(message, tree); break;
    case MSGTYPE_ADDOBJECT: status = xml_parse_body_addobject(message, tree); break;
    case MSGTYPE_ADDOBJECT_RESP: status = xml_parse_body_addobject_resp(message, tree); break;
    case MSGTYPE_DELOBJECT: status = xml_parse_body_delobject(message, tree); break;
    case MSGTYPE_DELOBJECT_RESP: status = xml_parse_body_delobject_resp(message, tree); break;
    case MSGTYPE_DISCOVERCONFIG: status = xml_parse_body_discoverconfig(message, tree); break;
    case MSGTYPE_DISCOVERCONFIG_RESP: break;  // Currently this msg has no body node 
    case MSGTYPE_REBOOT: status = xml_parse_body_reboot(message, tree); break;
    case MSGTYPE_RESET: status = xml_parse_body_reset(message, tree); break;
    case MSGTYPE_INITACTIONS: break;  // Currently this msg has no body node 
    default: GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT, "Unknown message type `%s'", buf);
    }

ret:
    mxmlDelete(tree);
    return status;
}

int mmx_frontapi_msg_header_parse(const char *xmlmsg, ep_msg_header_t *msg_header)
{
    int status = FA_OK;
    char buf[MSG_MAX_STR_LEN];

    mxml_node_t *tree = mxmlLoadString(NULL, xmlmsg, MXML_OPAQUE_CALLBACK);
    if (!tree || mxmlGetElement(tree) == NULL || strcmp(mxmlGetElement(tree), MSG_STR_ROOT_NAME))
        GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT, "Incorrect format of the message");

    XML_GET_INT(tree, MSG_STR_CALLERID, msg_header->callerId);
    XML_GET_INT(tree, MSG_STR_TXAID, msg_header->txaId);
    XML_GET_INT(tree, MSG_STR_RESPFLAG, msg_header->respFlag);
    XML_GET_INT(tree, MSG_STR_RESPMODE, msg_header->respMode);
    XML_GET_INT(tree, MSG_STR_RESPPORT, msg_header->respPort);

    XML_GET_TEXT(tree, tree, MSG_STR_RESPADDR, buf, sizeof(buf), FALSE);
    if (!inet_aton(buf, (struct in_addr *)&(msg_header->respIpAddr)))
        GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT, "Could not parse IP address");

    if (mxmlFindElement(tree, tree, MSG_STR_RESPCODE, NULL, NULL, MXML_DESCEND))
    {
        XML_GET_INT(tree, MSG_STR_RESPCODE, msg_header->respCode);
    }

    if (mxmlFindElement(tree, tree, MSG_STR_MOREFLAG, NULL, NULL, MXML_DESCEND))
    {
        XML_GET_INT(tree, MSG_STR_MOREFLAG, msg_header->moreFlag);
    }

    XML_GET_TEXT(tree, tree, MSG_STR_TYPE, buf, sizeof(buf), FALSE);
    msg_header->msgType = msgtype2num(buf);
    
    XML_GET_TEXT(tree, tree, MSG_STR_DBTYPE, buf, sizeof(buf), TRUE);
    msg_header->mmxDbType = mmxdbtype_str2num(buf);

ret:
    mxmlDelete(tree);
    return status;
}

static int xml_write_body_getvalue(ep_message_t *message, mxml_node_t *tree, mxml_node_t *node)
{
    char buf[MMXFA_MAX_NUMBER_OF_ANY_OP_PARAMS];
    int i;
    mxml_node_t *subnode;

    node = mxmlNewElement(node, MSG_STR_GETPARAMVALUE);
    
    subnode = mxmlNewElement(node, MSG_STR_NEXTLEVEL);
    mxmlNewText(subnode, 0, bool2str(message->body.getParamValue.nextLevel));
    
    subnode = mxmlNewElement(node, MSG_STR_CONFIGONLY);
    mxmlNewText(subnode, 0, bool2str(message->body.getParamValue.configOnly));
    
    node = mxmlNewElement(node, MSG_STR_PARAMNAMES);
    sprintf(buf, "%d", message->body.getParamValue.arraySize);
    mxmlElementSetAttr(node, MSG_STR_ATTR_ARRAYSIZE, buf);

    for (i = 0; i < message->body.getParamValue.arraySize; i++)
    {
        subnode = mxmlNewElement(node, MSG_STR_NAME);
        mxmlNewText(subnode, 0, message->body.getParamValue.paramNames[i]);
    }

    return FA_OK;
}

static int xml_write_body_setvalue(ep_message_t *message, mxml_node_t *tree, mxml_node_t *node)
{
    int status = FA_OK;
    char buf[MMXFA_MAX_NUMBER_OF_MSG_STR_SETTYPE];
    int i;
    mxml_node_t *subnode, *subnode1, *subnode2;
    
    if(!message->mem_pool.initialized)
        GOTO_RET_WITH_ERROR(FA_BAD_INPUT_PARAMS,
              "Message struct memory pool is not initialized for setValue");
    
    node = mxmlNewElement(node, MSG_STR_SETPARAMVALUE);

    memset(buf, 0, sizeof(buf));    
    subnode = mxmlNewElement(node, MSG_STR_SETTYPE);
    sprintf(buf, "%d", message->body.setParamValue.setType);
    mxmlNewText(subnode, 0, buf);

    memset(buf, 0, sizeof(buf));
    subnode = mxmlNewElement(node, MSG_STR_PARAMVALUES);
    sprintf(buf, "%d", message->body.setParamValue.arraySize);
    mxmlElementSetAttr(subnode, MSG_STR_ATTR_ARRAYSIZE, buf);

    for (i = 0; i < message->body.setParamValue.arraySize; i++)
    {
        subnode1 = mxmlNewElement(subnode, MSG_STR_NAMEVALUEPAIR);
        subnode2 = mxmlNewElement(subnode1, MSG_STR_NAME);
        
        mxmlNewText(subnode2, 0, message->body.setParamValue.paramValues[i].name);
        subnode2 = mxmlNewElement(subnode1, MSG_STR_VALUE);
        mxmlNewText(subnode2, 0, message->body.setParamValue.paramValues[i].pValue);
    }
ret:
    return status;
}

static int xml_write_body_getvalue_resp(ep_message_t *message, mxml_node_t *tree, mxml_node_t *node)
{
    int status = FA_OK;
    char buf[MMXFA_MAX_NUMBER_OF_ANY_OP_PARAMS];
    int  i;
    mxml_node_t *subnode1, *subnode2;
    
   if(!message->mem_pool.initialized)
        GOTO_RET_WITH_ERROR(FA_BAD_INPUT_PARAMS,
            "Message struct memory pool is not initialized for getValue resp");

    node = mxmlNewElement(node, MSG_STR_GETPARAMVALUE_RESP);
    node = mxmlNewElement(node, MSG_STR_PARAMVALUES);
    sprintf(buf, "%d", message->body.getParamValueResponse.arraySize);
    mxmlElementSetAttr(node, MSG_STR_ATTR_ARRAYSIZE, buf);

    for (i = 0; i < message->body.getParamValueResponse.arraySize; i++)
    {
        subnode1 = mxmlNewElement(node, MSG_STR_NAMEVALUEPAIR);
        subnode2 = mxmlNewElement(subnode1, MSG_STR_NAME);
        mxmlNewText(subnode2, 0, message->body.getParamValueResponse.paramValues[i].name);
        subnode2 = mxmlNewElement(subnode1, MSG_STR_VALUE);
        mxmlNewText(subnode2, 0, message->body.getParamValueResponse.paramValues[i].pValue);
    }

ret:
    return status;
}

static int xml_write_body_setvalue_resp(ep_message_t *message, mxml_node_t *tree, mxml_node_t *node)
{
    char buf[MMXFA_MAX_NUMBER_OF_MMX_API_RC];
    int i;
    mxml_node_t *subnode1, *subnode2 ;
    
    node = mxmlNewElement(node, MSG_STR_SETPARAMVALUE_RESP);
    if (message->header.respCode == FA_OK)
    {
        node = mxmlNewElement(node, MSG_STR_STATUS);
        sprintf(buf, "%d", message->body.setParamValueResponse.status);
        mxmlNewText(node, 0, buf);
        
        return FA_OK;
    }
    
    /* If we get here this is setResponse with faults.*/
    if (message->body.setParamValueFaultResponse.arraySize == 0)
    {
        /* No per-parameter faults, so do nothing. The header respcode is already written */
        return FA_OK;
    }
    else
    {
        node = mxmlNewElement(node, MSG_STR_PARAMFAULTS);
        sprintf(buf, "%d", message->body.setParamValueFaultResponse.arraySize);
        mxmlElementSetAttr(node, MSG_STR_ATTR_ARRAYSIZE, buf);
        
        for (i = 0; i < message->body.setParamValueFaultResponse.arraySize; i++)
        {
            ing_log(LOG_DEBUG, "%s: i=%d, name=%s, faultcode=%d\n", __func__, i, 
                    message->body.setParamValueFaultResponse.paramFaults[i].name,
                    message->body.setParamValueFaultResponse.paramFaults[i].faultcode);
            subnode1 = mxmlNewElement(node, MSG_STR_PARAMFAULT);
            subnode2 = mxmlNewElement(subnode1, MSG_STR_NAME );
            mxmlNewText(subnode2, 0, message->body.setParamValueFaultResponse.paramFaults[i].name);
            subnode2 = mxmlNewElement(subnode1, MSG_STR_FAULTCODE);
            mxmlNewInteger(subnode2, message->body.setParamValueFaultResponse.paramFaults[i].faultcode);		    
        }
    }

    return FA_OK;
}

static int xml_write_body_getparamnames(ep_message_t *message, mxml_node_t *tree, mxml_node_t *node)
{
    mxml_node_t *subnode;

    node = mxmlNewElement(node, MSG_STR_GETPARAMNAMES);
    subnode = mxmlNewElement(node, MSG_STR_PATHNAME);
    mxmlNewText(subnode, 0, message->body.getParamNames.pathName);
    subnode = mxmlNewElement(node, MSG_STR_NEXTLEVEL);
    mxmlNewText(subnode, 0, bool2str(message->body.getParamNames.nextLevel));

    return FA_OK;
}

static int xml_write_body_getparamnames_resp(ep_message_t *message, mxml_node_t *tree, mxml_node_t *node)
{
    char buf[MMXFA_MAX_NUMBER_OF_ANY_OP_PARAMS];
    int i;
    mxml_node_t *subnode1, *subnode2;

    node = mxmlNewElement(node, MSG_STR_GETPARAMNAMES_RESP);
    node = mxmlNewElement(node, MSG_STR_PARAMLIST);
    sprintf(buf, "%d", message->body.getParamNamesResponse.arraySize);
    mxmlElementSetAttr(node, MSG_STR_ATTR_ARRAYSIZE, buf);

    for (i = 0; i < message->body.getParamNamesResponse.arraySize; i++)
    {
        subnode1 = mxmlNewElement(node, MSG_STR_PARAMINFO);
        subnode2 = mxmlNewElement(subnode1, MSG_STR_NAME);
        mxmlNewText(subnode2, 0, message->body.getParamNamesResponse.paramInfo[i].name);
        subnode2 = mxmlNewElement(subnode1, MSG_STR_WRITABLE);
        mxmlNewText(subnode2, 0, bool2str(message->body.getParamNamesResponse.paramInfo[i].writable));
    }

    return FA_OK;
}

static int xml_write_body_addobject(ep_message_t *message, mxml_node_t *tree, mxml_node_t *node)
{
    int status = FA_OK;
    char buf[MMXFA_MAX_NUMBER_OF_ANY_OP_PARAMS];
    int i;
    mxml_node_t *subnode1, *subnode2, *subnode3;
    
    if(!message->mem_pool.initialized)
        GOTO_RET_WITH_ERROR(FA_BAD_INPUT_PARAMS,
              "Message struct memory pool is not initialized for addObj");  
    
    node = mxmlNewElement(node, MSG_STR_ADDOBJECT);
    
    subnode1 = mxmlNewElement(node, MSG_STR_OBJNAME);
    mxmlNewText(subnode1, 0, message->body.addObject.objName);

    subnode1 = mxmlNewElement(node, MSG_STR_PARAMVALUES);
    sprintf(buf, "%d", message->body.addObject.arraySize);
    mxmlElementSetAttr(subnode1, MSG_STR_ATTR_ARRAYSIZE, buf);

    for (i = 0; i < message->body.addObject.arraySize; i++)
    {
        subnode2 = mxmlNewElement(subnode1, MSG_STR_NAMEVALUEPAIR);
        subnode3 = mxmlNewElement(subnode2, MSG_STR_NAME);
        mxmlNewText(subnode3, 0, message->body.addObject.paramValues[i].name);
        subnode3 = mxmlNewElement(subnode2, MSG_STR_VALUE);
        mxmlNewText(subnode3, 0, message->body.addObject.paramValues[i].pValue);
    }
ret:    
    return status;
}

static int xml_write_body_addobject_resp(ep_message_t *message, mxml_node_t *tree, mxml_node_t *node)
{
    char buf[MMXFA_MAX_NUMBER_OF_MMX_API_RC];
    mxml_node_t *subnode;
    
    node = mxmlNewElement(node, MSG_STR_ADDOBJECT_RESP);
    
    subnode = mxmlNewElement(node, MSG_STR_INST_NUMBER);
    sprintf(buf, "%d", message->body.addObjectResponse.instanceNumber);
    mxmlNewText(subnode, 0, buf);
    
    subnode = mxmlNewElement(node, MSG_STR_STATUS);
    sprintf(buf, "%d", message->body.addObjectResponse.status);
    mxmlNewText(subnode, 0, buf);

    return FA_OK;
}

static int xml_write_body_delobject(ep_message_t *message, mxml_node_t *tree, mxml_node_t *node)
{
    char buf[MMXFA_MAX_NUMBER_OF_ANY_OP_PARAMS];
    int i;
    mxml_node_t *subnode1, *subnode2;
    
    node = mxmlNewElement(node, MSG_STR_DELOBJECT);

    subnode1 = mxmlNewElement(node, MSG_STR_OBJECTS);
    sprintf(buf, "%d", message->body.delObject.arraySize);
    mxmlElementSetAttr(subnode1, MSG_STR_ATTR_ARRAYSIZE, buf);

    for (i = 0; i < message->body.delObject.arraySize; i++)
    {
        subnode2 = mxmlNewElement(subnode1, MSG_STR_OBJNAME);
        mxmlNewText(subnode2, 0, message->body.delObject.objects[i]);
    }
    
    return FA_OK;
}

static int xml_write_body_delobject_resp(ep_message_t *message, mxml_node_t *tree, mxml_node_t *node)
{
    char buf[MMXFA_MAX_NUMBER_OF_MMX_API_RC];
    node = mxmlNewElement(node, MSG_STR_DELOBJECT_RESP);
    node = mxmlNewElement(node, MSG_STR_STATUS);
    sprintf(buf, "%d", message->body.delObjectResponse.status);
    mxmlNewText(node, 0, buf);

    return FA_OK;
}

static int xml_write_body_discoverconfig(ep_message_t *message, mxml_node_t *tree, mxml_node_t *node)
{
    char buf[MSG_MAX_STR_LEN];
    mxml_node_t *subnode;

    node = mxmlNewElement(node, MSG_STR_DISCOVERCONFIG);
    subnode = mxmlNewElement(node, MSG_STR_BACKENDNAME);
    sprintf(buf, "%s", message->body.discoverConfig.backendName);
    mxmlNewText(subnode, 0, buf);
    subnode = mxmlNewElement(node, MSG_STR_OBJNAME);
    sprintf(buf, "%s", message->body.discoverConfig.objName);
    mxmlNewText(subnode, 0, buf);

    return FA_OK;
}

static int xml_write_body_reboot(ep_message_t *message, mxml_node_t *tree, mxml_node_t *node)
{
    char buf[MMXFA_MAX_NUMBER_OF_DELAY_SEC] = {0};

    node = mxmlNewElement(node, MSG_STR_REBOOT);
    node = mxmlNewElement(node, MSG_STR_DELAY_SEC);
    sprintf(buf, "%u", message->body.reboot.delaySeconds);
    mxmlNewText(node, 0, buf);

    return FA_OK;
}

static int xml_write_body_reset(ep_message_t *message, mxml_node_t *tree, mxml_node_t *node)
{
    mxml_node_t *subnode;
    char buf[MMXFA_MAX_NUMBER_OF_DELAY_SEC]  = {0};
 
    node = mxmlNewElement(node, MSG_STR_RESET);
    subnode = mxmlNewElement(node, MSG_STR_RESETTYPE);
    sprintf(buf, "%u", message->body.reset.resetType);
    mxmlNewText(subnode, 0, buf);
    subnode = mxmlNewElement(node, MSG_STR_DELAY_SEC);
    sprintf(buf, "%u", message->body.reset.delaySeconds);
    mxmlNewText(subnode, 0, buf);
    memset(buf, 0, sizeof(buf));

    return FA_OK;
}

int mmx_frontapi_message_build(ep_message_t *message, char *resp, size_t resp_size)
{
    int status = FA_OK;
    char buf[MSG_MAX_STR_LEN];

    mxml_node_t *tree, *node;

    tree = mxmlLoadString(NULL, MSG_TEMPLATE_RESPONSE, MXML_OPAQUE_CALLBACK);
    if (!tree)
        GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT, "Could not load reply template");

    /* Fill in the header */
    XML_GET_NODE(tree, tree, MSG_STR_HEADER, node);
    XML_WRITE_INT(node, tree, MSG_STR_CALLERID, message->header.callerId);
    XML_WRITE_INT(node, tree, MSG_STR_TXAID, message->header.txaId);
    XML_WRITE_INT(node, tree, MSG_STR_RESPFLAG, message->header.respFlag);
    XML_WRITE_INT(node, tree, MSG_STR_RESPMODE, message->header.respMode);
    XML_WRITE_INT(node, tree, MSG_STR_RESPPORT, message->header.respPort);
    XML_WRITE_TEXT(node, tree, MSG_STR_RESPADDR, inet_ntop(AF_INET, &message->header.respIpAddr, buf, sizeof(buf)));

    XML_WRITE_INT(node, tree, MSG_STR_RESPCODE, message->header.respCode);
    XML_WRITE_INT(node, tree, MSG_STR_MOREFLAG, (int)message->header.moreFlag);
    XML_WRITE_TEXT(node, tree, MSG_STR_TYPE, msgtype2str(message->header.msgType));
    XML_WRITE_TEXT(node, tree, MSG_STR_DBTYPE, mmxdbtype_num2str(message->header.mmxDbType));

    /* Fill in the body */
    XML_GET_NODE(tree, tree, MSG_STR_BODY, node);
    switch (message->header.msgType)
    {
    case MSGTYPE_GETVALUE: status = xml_write_body_getvalue(message, tree, node); break;
    case MSGTYPE_GETVALUE_RESP: status = xml_write_body_getvalue_resp(message, tree, node); break;
    case MSGTYPE_SETVALUE: status = xml_write_body_setvalue(message, tree, node); break;
    case MSGTYPE_SETVALUE_RESP: status = xml_write_body_setvalue_resp(message, tree, node); break;
    case MSGTYPE_GETPARAMNAMES: status = xml_write_body_getparamnames(message, tree, node); break;
    case MSGTYPE_GETPARAMNAMES_RESP: status = xml_write_body_getparamnames_resp(message, tree, node); break;
    case MSGTYPE_ADDOBJECT: status = xml_write_body_addobject(message, tree, node); break;
    case MSGTYPE_ADDOBJECT_RESP: status = xml_write_body_addobject_resp(message, tree, node); break;
    case MSGTYPE_DELOBJECT: status = xml_write_body_delobject(message, tree, node); break;
    case MSGTYPE_DELOBJECT_RESP: status = xml_write_body_delobject_resp(message, tree, node); break;
    case MSGTYPE_DISCOVERCONFIG: status = xml_write_body_discoverconfig(message, tree, node); break;
    case MSGTYPE_DISCOVERCONFIG_RESP: status = FA_OK; break;  // Currently this msg has no body node 
    case MSGTYPE_REBOOT: status = xml_write_body_reboot(message, tree, node); break;
    case MSGTYPE_RESET: status = xml_write_body_reset(message, tree, node); break;
    default: GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT, "Unknown message type `%d'", message->header.msgType);
    }

    if (status != FA_OK)
        GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT, "Could not write message body");

    if (mxmlSaveString(tree, resp, resp_size, MXML_NO_CALLBACK) <= 0)
        GOTO_RET_WITH_ERROR(FA_INVALID_FORMAT, "Could not save message to string");

ret:
    mxmlDelete(tree);
    return status;
}


int mmx_frontapi_connect(mmx_ep_connection_t *conn, in_port_t own_port, unsigned timeout)
{
    conn->sock = 0;

    if (udp_socket_init(&(conn->sock), MMX_EP_ADDR, own_port))
    {
        perror("Could not initialize socket");
        return 1;
    }

    /* set timeout on the socket */
    struct timeval to;
    to.tv_sec = timeout;
    to.tv_usec = 0;

    if (setsockopt(conn->sock, SOL_SOCKET, SO_RCVTIMEO, (void *)&to, sizeof(to)) < 0)
    {
        perror("Could not set timeout");
        return 2;
    }

    conn->dest.sin_family = AF_INET;
    conn->dest.sin_port = htons(MMX_EP_PORT);
    conn->dest.sin_addr.s_addr = htonl(MMX_EP_ADDR);
    conn->sock_timeout = timeout;

    return 0;
}

int mmx_frontapi_send_req(mmx_ep_connection_t *conn, ep_packet_t *pkt)
{
    int res = sendto(conn->sock, pkt, sizeof(ep_packet_t)+strlen(pkt->msg)+1,
                         0, (struct sockaddr *)&conn->dest, sizeof(conn->dest));
    if (res < 0)
    {
        perror("Could not send packet to Entry point");
        return 1;
    }
    return 0;
}

int mmx_frontapi_receive(mmx_ep_connection_t *conn, char *buf, size_t buf_size, size_t *rcvd)
{
    int res = recv(conn->sock, buf, buf_size, 0);
    if (res < 0)
    {
        perror("Could not receive answer from Entry point");
        return 1;
    }
    buf[res] = '\0';
    *rcvd = res;
    return 0;
}

int mmx_frontapi_receive_resp(mmx_ep_connection_t *conn, int txaId,
                              char *buf, size_t buf_size, size_t *rcvd)
{
    int res = 0, still_waiting = 1;
    ep_msg_header_t msg_header;
    struct timeval begin, now;
    double timediff;

    gettimeofday(&begin , NULL);

    while(still_waiting)
    {
        /* Clear buffer and message */
        memset(buf, 0, buf_size);
        memset(&msg_header, 0, sizeof(ep_msg_header_t));

        if((res = recv(conn->sock, buf, buf_size, 0)) > 0)
        {
            /* Check transaction Id in the received packet */              
            if (mmx_frontapi_msg_header_parse(buf, &msg_header) == 0)
            {
                if (msg_header.txaId == txaId)
                {
                    /* It's correct response */
                    buf[res] = '\0';
                    *rcvd = res;
                    return 0;
                }
            }
        }

        gettimeofday(&now , NULL);
         
        timediff = (now.tv_sec - begin.tv_sec) + 1e-6 * (now.tv_usec - begin.tv_usec);
        if(timediff > conn->sock_timeout)
        {
            still_waiting = 0;
        }
    }

    return 1;
}

int mmx_frontapi_close(mmx_ep_connection_t *conn)
{
    if (conn->sock)
        close(conn->sock);
    return 0;
}

int mmx_frontapi_make_request(mmx_ep_connection_t *conn, ep_message_t *msg, int *more)
{
    int stat = FA_OK;
    size_t rcvd = 0;
    char buf[FA_BUF_SIZE];
    ep_packet_t *packet = (ep_packet_t *)buf;

    memset(packet->flags, 0, sizeof(packet->flags));
    if (more)
        *more = 0;

    if  (stat = mmx_frontapi_message_build(msg, packet->msg, (sizeof(buf) - sizeof(packet->flags) - 1)) != 0)
        return stat;

    if ((stat = mmx_frontapi_send_req(conn, packet)) != 0)
        return stat;

    if ((stat = mmx_frontapi_receive_resp(conn, msg->header.txaId, buf, sizeof(buf), &rcvd)) != 0)
        return stat;

    if ((stat = mmx_frontapi_message_parse(buf, msg)) != 0)
        return stat;

    if (more)
        *more = msg->header.moreFlag;

    return FA_OK;
}

int mmx_frontapi_make_xml_request(mmx_ep_connection_t *conn, char *xml_str,
                                  size_t xml_str_size, int *more)
{
    int stat = FA_OK;
    int txaId = 0;
    char buf[FA_BUF_SIZE];
    size_t rcvd = 0;
    ep_packet_t *packet = (ep_packet_t *)buf;
    ep_msg_header_t msg_header = {0};

    memset(packet->flags, 0, sizeof(packet->flags));
    strcpy_safe(packet->msg, xml_str, sizeof(buf) - sizeof(packet->flags) - 1);
    if (more)
        *more = 0;

    if ((stat = mmx_frontapi_msg_header_parse(xml_str, &msg_header)) != 0)
        return stat;

    txaId = msg_header.txaId;

    if ((stat = mmx_frontapi_send_req(conn, packet)) != 0)
        return stat;
   
    if ((stat = mmx_frontapi_receive_resp(conn, txaId, xml_str, xml_str_size, &rcvd)) != 0)
        return stat;

    if (more)
    {
        memset(&msg_header, 0, sizeof(msg_header));
        if ((stat = mmx_frontapi_msg_header_parse(xml_str, &msg_header)) != 0)
             return stat;

        *more = msg_header.moreFlag;
    }

    return FA_OK;
}


/*
 *  Initialize front-api message structure (of ep_message_t type)
 *  The caller must supply memory buffer that will be used for keeping
 *  parameters values
 */
int mmx_frontapi_msg_struct_init (ep_message_t *message, char *mem_buff, 
                                  unsigned short mem_buff_size)
{
    int status = 0;

    if (message == NULL || mem_buff == NULL || mem_buff_size <= 16)
        GOTO_RET_WITH_ERROR(FA_BAD_INPUT_PARAMS, "Bad input parameters");

    memset(mem_buff, 0, mem_buff_size);

    message->mem_pool.pool = mem_buff;
    message->mem_pool.size_bytes = mem_buff_size;

    message->mem_pool.curr_offset = 0;
    message->mem_pool.initialized = 1;

    ret:
    return status;
}

/*
* Release front-api message structure (ep_message_t).
* All fields of the message memory pool are initialized.
* The functions does not perform memory deallocation of the pool!
*/
int mmx_frontapi_msg_struct_release (ep_message_t *message)
{
    message->mem_pool.pool = 0;
    message->mem_pool.size_bytes = 0;
    message->mem_pool.curr_offset = 0;

    message->mem_pool.initialized = 0;

    return 0;
}


/*
 * Insert the value string to the specified name-value pair that is a part 
 * of the specified frontend-api message.
 * The function checks if there is enough space in the message memory pool.
 */
int mmx_frontapi_msg_struct_insert_value (ep_message_t *message, 
                                          nvpair_t *nvPair, char *value)
{
    int    status = 0;
    int    val_len = 0;
    size_t perm_len = 0;

    if (message == NULL || !message->mem_pool.initialized || !nvPair)
        GOTO_RET_WITH_ERROR(FA_BAD_INPUT_PARAMS, 
            "Bad input params (init flag = %d)", message->mem_pool.initialized);

    perm_len = (size_t)(message->mem_pool.size_bytes - message->mem_pool.curr_offset);

    if (value)
        val_len = strlen(value);

    if(perm_len < val_len + 1)
       GOTO_RET_WITH_ERROR(FA_NOT_ENOUGH_MEMORY, 
         "No space in front-api msg (val len %d, permitted len %d", val_len, perm_len);

    nvPair->pValue = message->mem_pool.pool + message->mem_pool.curr_offset;
    strcpy(nvPair->pValue, value ? value : "");

    message->mem_pool.curr_offset += (val_len + 1);
    /*ing_log(LOG_DEBUG,"Frontend api msg pool: curr offset %d, value %s\n",
               message->mem_pool.curr_offset,nvPair->pValue); */

ret:
    return status;
}

/*
 * Insert the name and value strings to the specified name-value pair 
 * which is a part of the specified frontend-api message.
 * The function checks if there is enough space in the message memory pool.
 */
int mmx_frontapi_msgstruct_insert_nvpair(ep_message_t *message, 
                                         nvpair_t *nvPair, 
                                         char *name, char *value)
{
    int    status = 0;
    int    val_len = 0;
    size_t perm_len = 0;

    if (message == NULL || nvPair == NULL)
        GOTO_RET_WITH_ERROR(FA_BAD_INPUT_PARAMS, 
            "Bad input params (init flag = %d)", message->mem_pool.initialized);

    if ( !message->mem_pool.initialized)
        GOTO_RET_WITH_ERROR(FA_BAD_INPUT_PARAMS, 
            "Bad input params (init flag = %d)", message->mem_pool.initialized);
            
    if ( (name == NULL) || (strlen(name) == 0))
        GOTO_RET_WITH_ERROR(FA_BAD_INPUT_PARAMS, "Bad input params (the name is empty)");
        
    /* Check if there is enough space in the message memory pool */
    perm_len = (size_t)(message->mem_pool.size_bytes - message->mem_pool.curr_offset);

    if (value)
        val_len = strlen(value);

    if(perm_len < val_len + 1)
       GOTO_RET_WITH_ERROR(FA_NOT_ENOUGH_MEMORY, 
         "No space in front-api msg (val len %d, permitted len %d", val_len, perm_len);

    /* Copy the value string to the memory pool and update the pool offset */
    nvPair->pValue = message->mem_pool.pool + message->mem_pool.curr_offset;
    strcpy(nvPair->pValue, value ? value : "");

    message->mem_pool.curr_offset += (val_len + 1);
    
    /* Copy the name string to the nvpair struct */
    if (sizeof(nvPair->name) < strlen(name))
        ing_log(LOG_DEBUG, "MMX front API: param name %s is truncated (permitted len is %d bytes)\n",
                            name, sizeof(nvPair->name));

    strcpy_safe(nvPair->name, name, sizeof(nvPair->name));

    /*ing_log(LOG_DEBUG,"Frontend api msg pool: curr offset %d, value %s\n",
               message->mem_pool.curr_offset,nvPair->pValue); */
    ret:
    return status;
}
