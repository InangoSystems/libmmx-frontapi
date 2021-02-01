#!/usr/bin/lua
--[[
#
# mmx-frontapi.lua
#
# Copyright (c) 2013-2021 Inango Systems LTD.
#
# Author: Inango Systems LTD. <support@inango-systems.com>
# Creation Date: Sep 2013
#
# The author may be reached at support@inango-systems.com
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# Subject to the terms and conditions of this license, each copyright holder
# and contributor hereby grants to those receiving rights under this license
# a perpetual, worldwide, non-exclusive, no-charge, royalty-free, irrevocable
# (except for failure to satisfy the conditions of this license) patent license
# to make, have made, use, offer to sell, sell, import, and otherwise transfer
# this software, where such license applies only to those patent claims, already
# acquired or hereafter acquired, licensable by such copyright holder or contributor
# that are necessarily infringed by:
#
# (a) their Contribution(s) (the licensed copyrights of copyright holders and
# non-copyrightable additions of contributors, in source or binary form) alone;
# or
#
# (b) combination of their Contribution(s) with the work of authorship to which
# such Contribution(s) was added by such copyright holder or contributor, if,
# at the time the Contribution is added, such addition causes such combination
# to be necessarily infringed. The patent license shall not apply to any other
# combinations which include the Contribution.
#
# Except as expressly stated above, no rights or licenses from any copyright
# holder or contributor is granted under this license, whether expressly, by
# implication, estoppel or otherwise.
#
# DISCLAIMER
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
# USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# NOTE
#
# This is part of a management middleware software package called MMX that was developed by Inango Systems Ltd.
#
# This version of MMX provides web and command-line management interfaces.
#
# Please contact us at Inango at support@inango-systems.com if you would like to hear more about
# - other management packages, such as SNMP, TR-069 or Netconf
# - how we can extend the data model to support all parts of your system
# - professional sub-contract and customization services
#
--]]

-- Creation date: Dec 19, 2013

--[[ Description:
       File contains Lua functions allowing to send management request from
       MMX front-ends to the MMX Entry-Point.
--]]

--[[

   Bellow is Lua explanations and examples of Lua tables containing
   requests sent from a front-ends to Entry-Point and 
   responses sent from the Entry-Point to the front-ends.

--====================================================================
--   Lua Request to the Entry-point 
--====================================================================

request = {
    hdr =  { ......},
    body   =  { . . . .}
}

-- -----------------------------------------------------------------------------
   Header of request sent by a front-end to the Entry-Point:
-- ------------------------------------------------------------------------------
request = {
    hdr = { 
        callerId = '',  -- Id of a caller component (not 0 value)
        txaId    = '',  -- Transaction ID - number supplyed by the caller.
                           The same value will be returned in the response.                       
        respMode = '',  -- Response mode: sync (0), async (1), noresp (2)
                        -- Currently supported:  0 - sync or 2 - response is not needed  
        msgType  = ''   -- Type of EP request/response
                           "GetParamValue", "SetParamValue", "GetParamNames",
                           "AddObject", "DelObject", "Reboot", "Factoryreset"
        dbType   = ''   -- Optional parameter specifying type of MMX data base.
                           Possible values: "running", "startup", "candidate".
                           In case this node is absent, 
                           dbtype will be thought as "running".
    }, 

    body={  } -- Body of request - see the below examples
}

-- ----------------------------------------
--   2  examples of the body GetParamValue
-- -----------------------------------------
    body = {
        nextLevel = true,
        paramNames = {
            {name = "Device.Bridging.Bridge.1.Port.2.Stats.BytesSent"}, 
            {name = "Device.Bridging.Bridge.1.Port.2.Stats.BytesReceived"} 
        }
    }
    
--  We ask here only configurable (i.e. writable) parameters of the specified
--   subtree

    body = {
        configOnly = true,
        paramNames = {
            {name = "Device.Ethernet.Interface.1"}, 
        }
    }

-- ---------------------------------------------
--    Example of the body GetParamNextValue
-- ---------------------------------------------
    body = { 
        paramNames = {
            {name = "Device.Bridging.Bridge.1.Port.2.Stats.BytesSent"}, 
            {name = "Device.Bridging.Bridge.1.Port.2.Stats.BytesReceived"} 
        }
    }

-- --------------------------------------------
--      Example of the body SetParamValue
--      (setType == 1 means Apply
--       setType == 3 means Apply + Save)
-- --------------------------------------------
    body={
        setType = "3",
        paramNameValuePairs = {
           { name = "Device.Bridging.Bridge.1.Port.1.Enable", value = "true"},
           { name = "Device.Bridging.Bridge.1.Port.2.Enable", value = "false"}
        }
    }

-- ------------------------------------------
--    Example of the body GetParamNames
-- -----------------------------------------
    body = { 
        pathName  = "Device.Bridging",
        nextLevel = true
    }

-- -----------------------------------------
--   Example of the body AddObject
-- -----------------------------------------
    body = { 
        objName = "Device.Bridging.Bridge",
        paramNameValuePairs = {
          { name = "Alias",    value = "mainbridge"},
          { name = "Enable",   value = "true"},
          { name = "Standard", value = "802.1Q-2005"}
        }
    }

-- ----------------------------------------
--    Example of the body DelObject
-- ----------------------------------------
    body={
        objects = { 
            { objName = "Device.Bridging.Bridge.1.Port.1"},
            { objName = "Device.Bridging.Bridge.1.Port.2"}
        }
    }

-- -----------------------------------
--  Example of the body Reboot
-- -----------------------------------
     body = {delaySeconds = "15"}

-- ------------------------------------
-- Example of the body FactoryReset
-- ------------------------------------
    body = { resetType    = "1", 
             delaySeconds = "15"}

-- ------------------------------------
--    Example of the body Copy
-- ------------------------------------
     body={srcProto = "tftp", 
           srcHostType = "host",
           srcHostName = "192.168.1.32",
           srcFileType = "",
           srcFileName = "MMX_image_Rev12",
           dstProto = "local",
           dstHostType = "",
           dstFileName = "",
           dstFileType = "image",
           userName = "Inango",
           password = "xyz",
           delaySeconds = "15"
          }

--==================================================================
--  Entry-point Lua table response
-- =================================================================

response = {
    hdr =  { ......},
    body   =  { . . . .}
}

-- -------------------------------------------------------------------------
   Header of the Entry-point response that is sent to a front-end
-- -------------------------------------------------------------------------
response = {
    hdr = { 
        callerId = '', -- Id of a caller component. 0 is used for EP itself 
        txaId    = '', -- Transaction ID - the same as in the request       
        moreFlag = '', -- 0 – this is the last resp msg, 1 – more resp will be sent
        msgType  = '', -- Type of EP request/response]
        resCode  = '', -- Operation result code; 0 – OK, otherwise error 
	flags    = '',
    },
    body = { <body> }   -- Body of request - see the below examples
}

-- -----------------------------------------------
--   Example of the body GetParamValueResponse     
-- -----------------------------------------------    
    body = {
        paramNameValuePairs = {
            {name = "Device.Bridging.Bridge.1.Port.2.Stats.BytesSent",      value = "123456"},
            {name = "Device.Bridging.Bridge.1.Port.2.Stats.BytesReceived",  value = "654321"}
        }
    }

-- ------------------------------------------------------
--    Example of the body GetParamNextValueResponse
-- ------------------------------------------------------
   body = {
       paramNameValuePairs={
           {name = "Device.Bridging.Bridge.1.Port.3.Stats.BytesSent",     value = "123456"},
           {name = "Device.Bridging.Bridge.1.Port.3.Stats.BytesReceived", value = "654321"}
       }
   }

-- --------------------------------------------------
--      Example of the body SetParamValueResponse
-- ---------------------------------------------------

    if resCode = 0
    body = {
        status = "0"
    }

    else
    body = {
        paramFaults={
            {name = "Device.Bridging.Bridge.1.Port.3.Stats.BytesSent",     faultcode = "9003"},
            {name = "Device.Bridging.Bridge.1.Port.3.Stats.BytesReceived", faultcode = "9003"}
        }
    }         

-- -------------------------------------------------
--     Example of the body GetParamNamesResponse
-- -------------------------------------------------
    body = {
        paramListArray = {
            {name = "Device.Bridging.MaxBridgeEntries",  writable = "false"},
            {name = "Device.Bridging.MaxVLANEntries",    writable = "false"}
        }
    }

-- ------------------------------------------------
--      Example of the body AddObjectRespons
-- ------------------------------------------------
    body = { objInstanceNumber = "1",
             status = "0"}

-- -----------------------------------------------
--     Example of the body DelObjectResponse
-- -----------------------------------------------
    body = {status = "0"} 

--  ---------------------------------------------
--    Example of the body CopyResponse
--  ---------------------------------------------
    body = {
        status = "0",
	startTime = "Tue May 28 04:25:37 UTC 2013",
        completeTime = "Tue May 28 04:26:11 UTC 2013",
        resText = "Copy of file  MMX_image_Rev12 successfully completed" 
    }   
 
-- ---------------------------------------------
--    Example of the body DiscoverConfig
--  ---------------------------------------------
    body = {
        backendName = "gnh_be", -- it's "optional" parameter, it must present, but can be ""
        objName     = "Device.Ghn.Interface.*."  -- optional parameter
    }

--]]


--==================================================================

require "mmx.ing_utils"
socklib = require"socket"
xml = require("LuaXml")

local MMX_ERROR_NO_ERROR          = 0
local MMX_ERROR_SOCKET_CREATE     = 11
local MMX_ERROR_XML_PARSE         = 12
local MMX_ERROR_SENDTO_ERROR      = 13
local MMX_ERROR_RECEIVEFROM_ERROR = 15
local MMX_ERROR_FEREQUEST_ERROR   = 16

local MMX_ERROR_INTERNAL_ERROR    = 9002

-- Log level (TODO: Should be moved to environment variable)
local loglevel = 1

--Response mode: 0 - sync mode, 1 - async mode, 2 - response is not needed
local MMX_RESMODE_NO_RESP = 2

-- MAX and MIN values of EP response receiving timeout (in secs)
local MAX_EP_RESP_TIMEOUT = 15
local MIN_EP_RESP_TIMEOUT = 12

local serveraddr = '127.0.0.1'
local serverport_send = 10100    -- EP port

local clientAddr = '127.0.0.1'   -- 
local clientport_receive = 4321  -- our port just fo testing !!! We need to think 

--[[ ------------------------------------
--   Function for printing logs
--    Input params:
--       compName      - Name of the backend and set file name
--	 (...)            - Log message
--    Returns:
--       Print log message in the file
--  TODO: Move the function from this file 
-- ------------------------------------------]]
function logMessage(compName,  ...)	

    local logpath = "/var/log/mmx"	
    if (loglevel == 1) and compName and(type(compName) == "string") then
        for i=1,#arg do arg[i]=tostring(arg[i]) end

        local file = io.open(logpath.."/"..compName..".log","a")
        if not file then 
            file = io.open(logpath.."/"..compName..".log","a")
        end
        if file then
            file:write(os.date("[%Y-%m-%d %X] ")..(table.concat(arg," ")).."\n")
            file:close()
        end
    end
end

local function mmx_frontapi_send(clientsock, fe_request_xml)

    local rc, errmsg = clientsock:sendto(fe_request_xml, serveraddr, serverport_send)
    --logMessage("mmx-frontapi", "mmx_frontapi_send: After sendto EP: rc = ",
    --           (rc or "nil"),", errmsg = ",(errmsg or "nil"))
        
    if errmsg ~= nil then  -- send request to EP failed
        return MMX_ERROR_SENDTO_ERROR

    else  -- send request to EP successed
        logMessage("mmx-frontapi", "mmx_frontapi_send:", string.format(
                    "Request (%d bytes) to EntryPoint is sent to %s:%s", rc, serveraddr, serverport_send)) 
        return MMX_ERROR_NO_ERROR 
    end
end

local function mmx_frontapi_receive(clientsock)

    local ep_response, remoteaddr, remoteport = clientsock:receivefrom()

    --Validate response status
    if (ep_response == nil) and (remoteaddr == 'timeout') then 
        return MMX_ERROR_RECEIVEFROM_ERROR , {}
    else
        return MMX_ERROR_NO_ERROR, ep_response, remoteaddr, remoteport
    end

end

local function mmx_frontapi_message_build(fe_request, port)
    local reqMsgType = fe_request.header.msgType

    --Create root node
    local root = xml.new("EP_ApiMsg")
    --Create header node
    local hdr = xml.new("hdr")

    --Fill in header node values 
    hdr.callerId = xml.new("callerId")
    table.insert(hdr.callerId, fe_request.header.callerId)
    hdr.txaId = xml.new("txaId")
    table.insert(hdr.txaId, fe_request.header.txaId)
    hdr.respFlag = xml.new("respFlag")
    table.insert(hdr.respFlag, "0")
    hdr.respMode = xml.new("respMode")
    table.insert(hdr.respMode, fe_request.header.respMode)
    hdr.respPort = xml.new("respPort")
    table.insert(hdr.respPort, port)
    hdr.respIpAddr = xml.new("respIpAddr")
    table.insert(hdr.respIpAddr, clientAddr)
    hdr.msgType = xml.new("msgType")
    table.insert(hdr.msgType, fe_request.header.msgType)
    
    if (fe_request.header.dbType) then
        hdr.dbType = xml.new("dbType")
        table.insert(hdr.dbType, fe_request.header.dbType)
    end
    root:append(hdr)

    --Fill in body node values 
    local body = xml.new("body")
    --Get type of request
    local bodyType = xml.new(fe_request.header.msgType)
    body:append(bodyType)

    if reqMsgType == "GetParamValue" or reqMsgType == "GetParamNextValue" then
          
        if fe_request.body.nextLevel then
            bodyType.nextLevel = xml.new("nextLevel")
            if fe_request.body.nextLevel == true then
                table.insert(bodyType.nextLevel, "true")
            else   
                table.insert(bodyType.nextLevel, "false")
            end
        end
        
        if fe_request.body.configOnly then
            bodyType.configOnly = xml.new("configOnly")
            if fe_request.body.configOnly == true then
                table.insert(bodyType.configOnly, "true")
            else   
                table.insert(bodyType.configOnly, "false")
            end
        end

        local paramNames = xml.new("paramNames")
        for key, val in pairs (fe_request.body.paramNames or {}) do
            name = xml.new("name")
            table.insert(name, val.name)
            paramNames:append(name)
        end

        bodyType:append(paramNames)
        root:append(body)

        local xparamNames = root:find("paramNames")
        xparamNames["arraySize"] = #fe_request.body.paramNames

    elseif reqMsgType == "SetParamValue" or reqMsgType == "AddObject" then

        if reqMsgType == "SetParamValue" then
            bodyType.setType = xml.new("setType")
            table.insert(bodyType.setType, fe_request.body.setType)

        elseif reqMsgType == "AddObject" then
            bodyType.objName = xml.new("objName")
            table.insert(bodyType.objName, fe_request.body.objName)
        end

        local paramValues = xml.new("paramValues")
        for key, val in pairs (fe_request.body.paramNameValuePairs or {}) do
            nameValuePair = xml.new("nameValuePair")
            name = xml.new("name")
            table.insert(name, val.name)
            value = xml.new("value")
            table.insert(value, val.value)
            nameValuePair:append(name)
            nameValuePair:append(value)
            paramValues:append(nameValuePair)
        end

        bodyType:append(paramValues)
        root:append(body)

        local xparamValues = root:find("paramValues")
        if fe_request.body.paramNameValuePairs ~= nil then
            xparamValues["arraySize"] = #fe_request.body.paramNameValuePairs
        else
            xparamValues["arraySize"] = 0;
        end        

    elseif reqMsgType == "GetParamNames" then
        bodyType.pathName = xml.new("pathName")
        table.insert(bodyType.pathName, fe_request.body.pathName)

        bodyType.nextLevel = xml.new("nextLevel")
        table.insert(bodyType.nextLevel, fe_request.body.nextLevel)
        root:append(body)

    elseif reqMsgType == "DelObject" then
        local objects = xml.new("objects")
        for key, val in pairs (fe_request.body.objects or {}) do
            objName = xml.new("objName")
            table.insert(objName, val.objName)
            objects:append(objName)
        end

        bodyType:append(objects)
        root:append(body)

        local xobjects = root:find("objects")
        xobjects["arraySize"] = #fe_request.body.objects

    elseif reqMsgType == "Reboot" then
        bodyType.delaySeconds = xml.new("delaySeconds")
        table.insert(bodyType.delaySeconds, fe_request.body.delaySeconds)
        root:append(body)

    elseif reqMsgType == "FactoryReset" then
        bodyType.resetType = xml.new("resetType")
        table.insert(bodyType.resetType, fe_request.body.resetType)

        bodyType.delaySeconds = xml.new("delaySeconds")
        table.insert(bodyType.delaySeconds, fe_request.body.delaySeconds)
        root:append(body)
    elseif reqMsgType == "Copy" then
        bodyType.srcProto = xml.new("srcProto")
        table.insert(bodyType.srcProto, fe_request.body.srcProto)

        bodyType.srcHostType = xml.new("srcHostType")
        table.insert(bodyType.srcHostType, fe_request.body.srcHostType)

        bodyType.srcHostName = xml.new("srcHostName")
        table.insert(bodyType.srcHostName, fe_request.body.srcHostName)

        bodyType.srcFileType = xml.new("srcFileType")
        table.insert(bodyType.srcFileType, fe_request.body.srcFileType)

        bodyType.srcFileName = xml.new("srcFileName")
        table.insert(bodyType.srcFileName, fe_request.body.srcFileName)

        bodyType.dstProto = xml.new("dstProto")
        table.insert(bodyType.dstProto, fe_request.body.dstProto)

        bodyType.dstHostType = xml.new("dstHostType")
        table.insert(bodyType.dstHostType, fe_request.body.dstHostType)

        bodyType.dstHostName = xml.new("dstHostName")
        table.insert(bodyType.dstHostName, fe_request.body.dstHostName)

        bodyType.dstFileType = xml.new("dstFileType")
        table.insert(bodyType.dstFileType, fe_request.body.dstFileType)

        bodyType.dstFileType = xml.new("dstFileType")
        table.insert(bodyType.dstFileType, fe_request.body.dstFileType)

        bodyType.userName = xml.new("userName")
        table.insert(bodyType.userName, fe_request.body.userName)

        bodyType.password = xml.new("password")
        table.insert(bodyType.password, fe_request.body.password)

        bodyType.delaySeconds = xml.new("delaySeconds")
        table.insert(bodyType.delaySeconds, fe_request.body.delaySeconds)
        root:append(body)

        elseif reqMsgType == "DiscoverConfig" then
            bodyType.backendName = xml.new("backendName")
            if fe_request.body.backendName ~= nil then
                table.insert(bodyType.backendName, fe_request.body.backendName)
            else
                table.insert(bodyType.backendName, "")
            end
               
            bodyType.objName = xml.new("objName")
            if  fe_request.body.objName ~= nil then
                table.insert(bodyType.objName, fe_request.body.objName)
            else
                table.insert(bodyType.objName, "") 
            end

        root:append(body)
    end

        -- Convert XML object to XML string and return it
        return xml.str(root)
end

local function mmx_frontapi_message_parse(ep_response)

    local resTab, res_header, res_body = {}, {}, {}
    local xname, xvalue, xstatus = nil, nil, nil
    local xList = {}

    --converts an XML string to a Lua table for work with LuaXml lib
    local respXml = xml.eval(ep_response);

    --Parse XML header and fill header of the resulting table
    xvalue = respXml:find("callerId") or {}
    res_header.callerId = xvalue[1]

    xvalue = respXml:find("txaId") or {}
    res_header.txaId = xvalue[1]

    xvalue = respXml:find("resCode") or {}
    res_header.resCode = xvalue[1]

    xvalue = respXml:find("moreFlag") or {}
    res_header.moreFlag = xvalue[1]

    xvalue = respXml:find("msgType") or {}
    res_header.msgType  = xvalue[1]
    
    -- DB Type is an "optional" parameter
    xvalue = respXml:find("dbType")
    if xvalue ~= nil then res_header.dbType  = xvalue[1] end

    xvalue = respXml:find("flags") or {}
    res_header.flags = xvalue[1]
    
     if tonumber(res_header.resCode) ~= MMX_ERROR_NO_ERROR and 
        res_header.msgType ~= "SetParamValueResponse" then
        resTab={hdr = res_header, body = res_body}
        logMessage("mmx-frontapi", "message_parse: response with bad resCode: \n", ing.utils.tableToString(resTab)) 
        return MMX_ERROR_NO_ERROR, resTab
     end 
 
     -- Parse body of XML response message
     if res_header.msgType == "GetParamValueResponse" or 
       res_header.msgType == "GetParamNextValueResponse" then

        res_body = {paramNameValuePairs={}}
        xList = respXml:find("paramValues")
        --logMessage("mmx-frontapi", "message_parse: Debug print - paramValues: \n", ing.utils.tableToString(xList)) 

        for i, elem in ipairs(xList or {}) do
            -- process the nameValuePair elements, extract names and values
            if (elem:tag() == "nameValuePair") then
                xname  = elem:find("name")
                xvalue = elem:find("value")
                if xname and xvalue  then
                    table.insert(res_body.paramNameValuePairs, {name = xname[1], value = xvalue[1]})
                else
                   logMessage("mmx-frontapi", string.format("message_parse: ERROR - wrong format of EP response name-value pair (%s)", i))
                end
            end
        end
        --logMessage("mmx-frontapi", "message_parse: response body for GetParamValues: \n", ing.utils.tableToString(res_body))  

     elseif res_header.msgType == "DelObjectResponse" then
         xstatus = respXml:find("status")
         if xstatus and xstatus[1] then
             res_body = {status = xstatus[1]}
         else
             res_header.resCode = MMX_ERROR_INTERNAL_ERROR 
             logMessage("mmx-frontapi", string.format("message_parse: ERROR - msg %s does not contain XML tag 'status'", res_header.msgType))             
         end

     elseif res_header.msgType == "SetParamValueResponse" then
         if tonumber(res_header.resCode) == MMX_ERROR_NO_ERROR then
             --Response message in case of success
             xstatus = respXml:find("status")
             if xstatus and xstatus[1] then
                 res_body = {status = xstatus[1]}
             else
                 res_header.resCode = MMX_ERROR_INTERNAL_ERROR 
                 logMessage("mmx-frontapi", string.format(
                            "message_parse: ERROR - successful msg %s does not contain XML tag 'status'",
                            res_header.msgType))             
             end
         else
             --Response message with faults
             res_body = {paramFaults={}}
             xList = respXml:find("paramFaults")
             for i, elem in ipairs(xList or {}) do
                 -- process paramFault; extract the "name" and "faultcode" of parameter
                 if (elem:tag() == "paramFault") then
                     xname = elem:find("name")
                     xfaultcode = elem:find("faultcode")
                     if xname and xfaultcode  then
                         table.insert(res_body.paramFaults, {name = xname[1], faultcode = xfaultcode[1]})
                     end
                 end
             end
         end
    
     elseif res_header.msgType == "GetParamNamesResponse" then
         res_body = {paramListArray={}}
         xList = respXml:find("paramList")
         for i, elem in ipairs(xList or {}) do
             -- process paramInfo; extract the "name" and "writable" of parameter
             if (elem:tag() == "paramInfo") then
                 xname = elem:find("name")
                 xwritable = elem:find("writable")
                if xname and xwritable  then
                    table.insert(res_body.paramListArray, {name = xname[1], writable = xwritable[1]})
                end
             end
         end
    
     elseif res_header.msgType == "AddObjectResponse" then
         local xobjInstanceNumber = respXml:find("objInstanceNumber")
         xstatus = respXml:find("status")
         if xobjInstanceNumber and xstatus then
             res_body={objInstanceNumber = xobjInstanceNumber[1], status = xstatus[1]}
         end
     elseif res_header.msgType == "CopyResponse" then

         xstatus = respXml:find("status")
         if xstatus and xstatus[1] then res_body.status = xstatus[1] end
        
         xvalue = respXml:find("startTime")
         if xvalue and xvalue[1] then res_body.startTime = xvalue[1] end

         xvalue = respXml:find("completeTime")
         if xvalue and xvalue[1] then res_body.completeTime = xvalue[1] end

         xvalue = respXml:find("resText")
         if xvalue and xvalue[1] then res_body.resText = xvalue[1] end		
     end

     --Make full result table
     resTab={hdr = res_header, body = res_body}
     --logMessage("mmx-frontapi", string.format("message_parse: full response for %s message:\n %s",
     --           res_header.msgType, ing.utils.tableToString(resTab))  
     return MMX_ERROR_NO_ERROR, resTab
end

local function is_array(x)
    local count = 0
    for _ in pairs(x) do
        count = count + 1
    end
    return count == #x
end

--[[--------------------------------------------------------------------
  Function name: mmx_frontapi_message_merge
  Description:
     This function is used for collecting fragmented response from EP and performs updating 
     content of buffered response (orig_resp) with content of single response fragment (add_resp).
                    
  Input parameters: 
     orig_resp - original parsed response, will be extended with data from add_resp
     add_resp  - additional parsed response, its body will extend original response's body 
                 and its header will overwrite header of original response 
  Output: 
     No output
-------------------------------------------------------------------------]]
function mmx_frontapi_message_merge(orig_tbl, add_tbl)
    for key, elem in pairs(add_tbl or {}) do
        local orig_elem = orig_tbl[key]
        if type(orig_elem) == "table" and type(elem) == "table" then
            local is_orig_array = is_array(orig_elem)
            local is_elem_array = is_array(elem)
            if is_orig_array and is_elem_array then
                for _, subelem in ipairs(elem) do
                    table.insert(orig_elem, subelem)
                end
            else
                -- Associative tables merged using this function recursively,
                -- because they act like content of messages from EP.
                mmx_frontapi_message_merge(orig_elem, elem)
            end
        else
            orig_tbl[key] = elem
        end
    end
end

-- =============================================
--      API functions
-- =============================================

--[[--------------------------------------------------------------------
  Function name: mmx_frontapi_epexecute_lua
  Description:
     This is one of the main API functions responsible of executing 
     front-end requests. It performs the following actions:
       - converts frontend request from Lua-table to XML format,
       - sends this request to the Entry-Point,
       - if response is needed:
             waits response from the Entry-Point,
             converts it from XML format to Lua-table and returns
               res-code 0 and the resulting Lua-table to the calling front-end
        - if response is not needed, returns res-code and empty table.                
  Input parameters: 
     fe_request - request from frontend to EP in Lua table format
     timeout    - value of timeout for response 
  Output: 
     res_code    - integer result code: 0 - in case of success, 
                                        otherwise - failure
     ep_response - if EP response is needed, this parameter contains response
                    from EP in Lua table format
                   if response is not needed (i.e response mode of the 
                   request header is "2"), this parameter comtains empty table.
-------------------------------------------------------------------------]]
function mmx_frontapi_epexecute_lua(fe_request,  timeout, udp_port)

    local func = "mmx_frontapi_epexecute_lua:"
    local res, ep_response_tab, msgType, awaitTxId = MMX_ERROR_NO_ERROR, {body={}}, "", nil
    local ep_response_xml
    local wait_for_response = false
    local datagramLenMax = 32768
    --Validate input
    if next(fe_request or {}, nil) == nil then
        logMessage("mmx-frontapi",func, "Empty request from frontend")
        return MMX_ERROR_FEREQUEST_ERROR, {}
    end
    if udp_port == nil then
        udp_port = clientport_receive
    end
    logMessage("mmx-frontapi","========== New request", fe_request.header.msgType,
                                         "(timeout:", timeout, ") ==========");
    --logMessage("mmx-frontapi", func, "Frontend request:\n", ing.utils.tableToString(fe_request)) 

    --Create socket and set name (EP Address and port) and timeout
    local clientsock = socklib.udp(datagramLenMax);
    if (clientsock == nil) then
        logMessage("mmx-frontapi",func, "Failed to create socket")
        return MMX_ERROR_SOCKET_CREATE, {}
    end
    for i = 1, 5 do
        local result, error = clientsock:setsockname(clientAddr, udp_port)
        if result ~= nil then
            break
        end
        os.execute("sleep 1")
    end
    timeout = tonumber(timeout) 
    if (timeout == nil) or (timeout < MIN_EP_RESP_TIMEOUT) then 
        timeout = MIN_EP_RESP_TIMEOUT
    else
        if timeout > MAX_EP_RESP_TIMEOUT then timeout = MAX_EP_RESP_TIMEOUT end
    end
    clientsock:settimeout(timeout);

    -- Convert lua table to xml
    msgType = fe_request.header.msgType
    awaitTxId = fe_request.header.txaId
    fe_request_xml = mmx_frontapi_message_build(fe_request, udp_port)
    logMessage("mmx-frontapi",func, "Built XML for", msgType,"request:\n", fe_request_xml) 

    --Add 8 bites in beginning of the XML string
    fe_request_xml="00000000"..fe_request_xml

    --Send xml to EP
    res = mmx_frontapi_send(clientsock, fe_request_xml)	
    if res ~= MMX_ERROR_NO_ERROR then
        logMessage("mmx-frontapi",func,"Sending request",msgType,"to EP failed:",res)
    else
        if tonumber(fe_request.header.respMode) == MMX_RESMODE_NO_RESP then
            logMessage("mmx-frontapi", func, "Response is not needed for",msgType,"request")	
        else
            wait_for_response = true
        end
    end

    local start_time = os.time()
    while wait_for_response and res == MMX_ERROR_NO_ERROR do
        res, ep_response_xml = mmx_frontapi_receive(clientsock)			
        if res ~= MMX_ERROR_NO_ERROR then
            logMessage("mmx-frontapi", func, " Failed to receive response from EP:",res)
            break 
        end

        logMessage("mmx-frontapi", func, "EP XML response successfully received") 
        --logMessage("XML response:\n",ep_response_xml)   

        local parsed_response_tab
        res, parsed_response_tab = mmx_frontapi_message_parse(ep_response_xml)
        logMessage("mmx-frontapi", func, "Response parsing results (", res, "):\n", 
                   ing.utils.tableToString(parsed_response_tab))	           
        if res ~= MMX_ERROR_NO_ERROR then
            logMessage("mmx-frontapi", func, " Failed to parse response from EP:",res)
            break
        end

        if parsed_response_tab["hdr"]["txaId"] == awaitTxId then
            ep_response_tab["hdr"] = parsed_response_tab["hdr"]
            mmx_frontapi_message_merge(ep_response_tab["body"], parsed_response_tab["body"])
            if parsed_response_tab["hdr"]["moreFlag"] == "0" then
                -- Successfully finished receiving response fragments
                break
            end
        else
            -- Just ignore response packets with wrong txaId
            logMessage("mmx-frontapi", func, " Received response from EP with wrong txaId: ",
                       parsed_response_tab["hdr"]["txaId"], ", expected: ", awaitTxId)
        end
        -- Check guard timeout for full response
        if wait_for_response and (os.time() - start_time) > timeout then
            -- Timeout exceeded and response not completely received, so force stop waiting and return error
            logMessage("mmx-frontapi", func, " Global timeout during waiting response from EP")
            break
        end
    end
    
    -- Free socket!
    clientsock:close()
    
    logMessage("mmx-frontapi","========== End of", msgType,"request processing ( res:",
                res,") ==========\n")
    return res, ep_response_tab
end

--[[-------------------------------------------------------------------------
    Function name: mmx_frontapi_epexecute_xml
    This function is similar to the previous one (mmx_frontapi_epexecute_lua),
    but the input fe_request and the output ep_response are XML-strings, 
    not Lua-tables.
    Input parameters:
       fe_request_xml - request from frontend to EP in xml format
       timeout    - value of timeout for response 
    Output parameters:
       res_code - integer result code: 0 - in case of success, 
                                       otherwise - failure 
       ep_response - if EP response is needed, this parameter contains response
                       from EP in XML format
                     if response is not needed (i.e response mode of the 
                        request header is "2"), this parameter is nil .
-----------------------------------------------------------------------------]]
function mmx_frontapi_epexecute_xml(fe_request_xml,  timeout, udp_port)

    local func = "mmx_frontapi_epexecute_xml:"
    local res, ep_response_xml, msgType = MMX_ERROR_NO_ERROR, "", ""
    local ep_response_xml
    local datagramLenMax = 32768
    --Validate input
    if fe_request_xml == nil then
        logMessage("mmx-frontapi",func, "Empty request from frontend")
        return MMX_ERROR_FEREQUEST_ERROR, {}
    end
    if udp_port == nil then
        udp_port = clientport_receive
    end
    logMessage("mmx-frontapi","========== New request (timeout:", timeout, ") ==========");
    logMessage("mmx-frontapi", func, "Frontend request:\n", fe_request_xml) 

    --Create socket and set name (EP Address and port) and timeout
    local clientsock = socklib.udp(datagramLenMax);
    if (clientsock == nil) then
        logMessage("mmx-frontapi",func, "Failed to create socket")
        return MMX_ERROR_SOCKET_CREATE, {}
    end

    for i = 1, 5 do
        local result, error = clientsock:setsockname(clientAddr, udp_port)
        if result ~= nil then
            break
        end
        os.execute("sleep 1")
    end

    timeout = tonumber(timeout) 
    if (timeout == nil) or (timeout < MIN_EP_RESP_TIMEOUT) then 
        timeout = MIN_EP_RESP_TIMEOUT
    else
        if timeout > MAX_EP_RESP_TIMEOUT then timeout = MAX_EP_RESP_TIMEOUT end
    end
    clientsock:settimeout(timeout);


    --Retrieve respMode and msgType fields from the request header.
    local respMode, msgType
    local xvalue = nil
    local respXml = xml.eval(fe_request_xml);
    xvalue = respXml:find("respMode") or {}
    respMode = xvalue[1]
    xvalue = respXml:find("msgType") or {}
    msgType = xvalue[1]

    --Add 8 bites in beginning of the XML string
    fe_request_xml="00000000"..fe_request_xml

    --Send xml to EP
    res = mmx_frontapi_send(clientsock, fe_request_xml)	
    if res == MMX_ERROR_NO_ERROR then
        if tonumber(respMode) ~= MMX_RESMODE_NO_RESP then		
            res, ep_response_xml = mmx_frontapi_receive(clientsock)			
            if res == MMX_ERROR_NO_ERROR then
                logMessage("mmx-frontapi", func, "Receiving response from EP succeeded")  
                logMessage("mmx-frontapi", func, "Received XML response: \n",ep_response_xml)    
            else
                logMessage("mmx-frontapi", func, " Failed to receive response from EP:",res)    
            end
        else -- Response is not needed
            logMessage("mmx-frontapi", func, "Response is not needed for",msgType,"request")	
        end
    else  -- Request sending failed
        logMessage("mmx-frontapi",func,"Sending request",msgType,"to EP failed:",res) 
    end

    -- Free socket!
    clientsock:close()

    logMessage("mmx-frontapi","========== End of", msgType,"request processing ( res:",
                res,") ==========\n")

    return res, ep_response_xml

end


