#!/usr/bin/lua
--[[
#
# mmx_api_wrapper.lua
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


--[[
This file contains class declaration for wrapper for mmx-frontapi
]]--

require "mmx.ing_utils"
require "mmx.mmx-frontapi"

MMXAPIWrapper = {}
MMXAPIWrapper.__index = MMXAPIWrapper

-- max number of object instances, which can be retrieved with one GET request
local MAX_GET_INSTANCE_SIZE = 8

---
-- Constructor
--
-- @todo: TODO: add support "address" and "port" arguments in front-api and here
-- @param caller_id Id of caller for requests header
-- @param address IP address of MMX EntryPoint
-- @param port Port which MMX EntryPoint listen
--
function MMXAPIWrapper.create(caller_id, address, port)
    local inst = {}
    setmetatable(inst, MMXAPIWrapper)
    inst.callerId = caller_id
    inst.SET_TYPE_APPLY = 1
    inst.SET_TYPE_SAVE = 2
    inst.SET_TYPE_TEST = 4

    math.randomseed(os.time())
    -- Random function maximum argument value is 2^31 - 1 == 2147483646
    inst.nextTxaId = math.random(2147483646)
    return inst
end

function MMXAPIWrapper:generateTxaId()
    local retTxaId = self.nextTxaId
    self.nextTxaId = self.nextTxaId + 1
    return tostring(retTxaId)
end

---
-- Splits full path to management object parameter into partial path (ending with ".") and parameter name
--
-- @param path string full path to management object
-- @return string   partial path to object (ending with ".")
-- @return string   parameter name
--
-- @usage MMXAPIWrapper::splitMngObjPath("Device.Ethernet.Interface.{i}.Name") --> "Device.Ethernet.Interface.{i}.", "Name"
-- @usage MMXAPIWrapper::splitMngObjPath("Device.DeviceInfo.") --> "Device.DeviceInfo.", ""
--
function MMXAPIWrapper:splitMngObjPath(mngObjPath)
    local leaf = string.match(mngObjPath, ".*%.([%w_]*)")

    local partialPathLength = string.len(mngObjPath) - string.len(leaf)
    local partialPath = string.sub(mngObjPath, 1, partialPathLength)

    return partialPath, leaf
end

--[[---
-- Perfroms GET request to MMX Entrypoint for each parameters listed in {params} on object {path}
--
-- @todo: TODO: should be removed on next commit
-- @param path string Partial path to object (must ends with ".")
-- @param params table Array of parameters in object by provided partial path
-- @return number Non-zero value indicates whether any error returned from Entrypoint
-- @return table Array of error codes for each parameter returned from Entrypoint (keys are full-paths to parameters)
-- @return table Array of returned values for each parameter (keys are full-paths to parameters)
--
function MMXAPIWrapper:get(path, params)
	require "mmx.mmx-frontapi"
	local retRows = {}
	local errRows = {}
	for _, name in pairs(params) do
		local normalizedParams = {}
		normalizedParams[#normalizedParams + 1] = {["name"] = path..name}
		request = {
			header={
				callerId = self.callerId,
				txaId = 123,
				respMode = 0,
				msgType = "GetParamValue"
			},
			body={
				paramNames = normalizedParams
			}
		}

		local errcode, response = mmx_frontapi_epexecute_lua(request, 3)
		local assocParams = {}
		if errcode == 0 and response["body"]["paramNameValuePairs"] ~= nil then
			for _, paramPair in pairs(response["body"]["paramNameValuePairs"]) do
				assocParams[paramPair["name"] ] = paramPair["value"]
			end
		end
		for _, paramNameHolder in pairs(normalizedParams) do
			local paramBigName = paramNameHolder["name"]
			local paramSmallName = string.sub(paramBigName, string.len(path) + 1)
			retRows[paramSmallName] = assocParams[paramBigName]
			errRows[paramSmallName] = errcode
		end
	end

	local allError = 0
	for idx, val in pairs(errRows) do
		if val ~= 0 and allError == 0 then
			allError = val
		end
	end

--	print("<pre>get request: "..ing.utils.tableToString(request).."</pre>")
--	print("<pre>get errs: "..ing.utils.tableToString(errRows).."</pre>")
--      print("<pre>get rets: "..ing.utils.tableToString(retRows).."</pre>")

	return allError, errRows, retRows
end
]]--

---
-- Filters all given {data}, leaving only entries, which match given {paramPath}
--
-- @param data       table  Associatiative array of data entries
-- @param paramPath  string Path with possible placeholders ("{i}" and/or "*") against which all data entries must be filtered
-- @return table    Subset of indirect data entries, which match given {paramPath} or empty table if no such entries were found
--
-- @usage   MMXWebEngine:filterAssocParams({["Device.IP.Interface.1.Name"] = "eth0", ["Device.Bridging.Bridge.1.VLAN.2.VLANID"] = "20"}, "Device.IP.Interface.{i}.Name")
--              --> {["Device.IP.Interface.1.Name"] = "eth0"}
-- @usage   MMXWebEngine:filterAssocParams({["Device.Bridging.Bridge.1.VLAN.1.VLANID"] = "10", ["Device.Bridging.Bridge.2.VLAN.1.VLANID"] = "20"}, "Device.Bridging.Bridge.1.VLAN.*.VLANID")
--              --> {["Device.Bridging.Bridge.1.VLAN.1.VLANID"] = "10"}
--
local function filterAssocParams(data, paramPath)
    local filteredData = {}
    local paramPathParts = ing.utils.split(paramPath, ".")
    local pathPartCount = #paramPathParts

    for dataPath, data in pairs(data) do
        local dataPathParts = ing.utils.split(dataPath, ".")

        -- compare both paths length
        if pathPartCount <= #dataPathParts then
            -- compare both paths part by part
            local similarParts = 0
            for i = 1, pathPartCount do
                local isPlaceholder = paramPathParts[i] == "{i}" or paramPathParts[i] == "*"
                if paramPathParts[i] == dataPathParts[i] or (isPlaceholder and tonumber(dataPathParts[i])) then
                    similarParts = similarParts + 1
                else
                    break
                end
            end

            -- if all paths parts are equal - add this data entry into filtered data
            if similarParts == pathPartCount then
                filteredData[dataPath] = data
            end
        end
    end

    return filteredData
end

---
-- Performs one GET request to MMX Entrypoint to retreive all parameters on object {path} and returns only required
-- parameters listed on {params}
--
-- @param path string Partial path to instance (must ends with ".")
-- @param params table Array of needed parameters in object by provided partial path
-- @return number Non-zero value indicates whether any error returned from Entrypoint
-- @return table Array of error codes for each parameter returned from Entrypoint (keys are full-paths to parameters)
-- @return table Array of returned values for each parameter (keys are full-paths to parameters)
--
function MMXAPIWrapper:getInstance(path, params)
        local retRows = {}
        local errRows = {}
        local request = {
                header={
                    callerId = self.callerId,
                    txaId = self:generateTxaId(),
                    respMode = 0,
                    msgType = "GetParamValue"
                },
                body={
                nextLevel = true,
                    paramNames = {
                        {["name"] = path}
                    }
                }
             }
    local errcode, response = mmx_frontapi_epexecute_lua(request, 3)
        --print("<pre> Received response from front API"..ing.utils.tableToString(response).."</pre>")
    if errcode == 0 then
        -- if request was successfully sent to EP and response was received and parsed without problems - check response header for errors
        errcode = tonumber(response["hdr"]["resCode"])
    end

    local assocParams = {}
    if errcode == 0 and response["body"]["paramNameValuePairs"] ~= nil then
        for _, paramPair in pairs(response["body"]["paramNameValuePairs"]) do
            assocParams[paramPair["name"]] = paramPair["value"]
        end
    end
        --print("<pre>path: Assoc params table"..ing.utils.tableToString(assocParams).."</pre>")

    for _, paramSmallName in pairs(params) do
        local paramBigName = path..paramSmallName
        retRows[paramSmallName] = assocParams[paramBigName] or ""
        errRows[paramSmallName] = errcode
    end
--	print("<pre>path: "..path..", request: "..ing.utils.tableToString(request).."</pre>")
--	print("<pre>params: "..ing.utils.tableToString(assocParams).."</pre>")
--	print("<pre>rets: "..ing.utils.tableToString(retRows).."</pre>")
	return errcode, errRows, retRows
end

---
-- Performs one compound GET request to MMX Entrypoint to retrieve all parameters on given objects and returns only required
-- parameters for each object
--
-- @param  pathParamTable   table   Table of partial paths (must ends with ".") of objects to get as keys
--                                  and list of parameters to return as values.
--                                  Paths with placeholders ("*") are also supported
-- @param  instanceMaxCount number  Max number of instances to retrieve in one request.
--                                  If isn't specied - MAX_GET_INSTANCE_SIZE is used
-- Input table example:
-- {
--      [Device.Ethernet.Interface.*.] = { "Name" },
--      [Device.IP.Interface.1.IPv4Address.1.] = { "InterfaceIndex", "IPv4AddressIndex", "IPAddress" }
-- }
--
-- @return number Non-zero value indicates whether any error returned from Entrypoint
-- @return table Array of error codes for each parameter returned from Entrypoint (keys are full-paths to parameters)
-- @return table Table of returned values for each path (keys are partial paths to objects and values are tables of returned values)
-- Output table example:
--  {
--      [Device.Ethernet.Interface.1.] = { ["Name"] = "eth0" },
--      [Device.Ethernet.Interface.2.] = { ["Name"] = "eth1" },
--      [Device.IP.Interface.1.IPv4Address.1.] = { ["InterfaceIndex"] = "1", ["IPv4AddressIndex"] = "2", ["IPAddress"] = "10.0.0.20" }
-- }
--
function MMXAPIWrapper:getMultipleInstances(pathParamTable, instanceMaxCount)
    local retRows = {}
    local combinedErrorCode = 0
    local errRows = {}

    -- max number of instances to retrieve via one request
    local getInstanceCount
    if instanceMaxCount and instanceMaxCount > 0 then
        -- shouldn't exceed max allowed value
        getInstanceCount = math.min(instanceMaxCount, MAX_GET_INSTANCE_SIZE)
    else
        -- if wasn't specified - use max allowed value
        getInstanceCount = MAX_GET_INSTANCE_SIZE
    end

    -- total number of instances to retrieve
    local pathParamSize = ing.utils.getTableSize(pathParamTable)

    -- table with names of parameters to get in current request
    local requestParamNames = {}

    -- table to store parsed responsed from EP
    local assocParams = {}

    -- number of paths, which were already processed (added (and, maybe, sent) to request)
    local processedPaths = 0
    for path, askedParams in pairs(pathParamTable) do
        local nameEntry
        -- if only one parameter is asked - get it directly
        if ing.utils.getTableSize(askedParams) == 1 then
            local askedParam = askedParams[1] or ""
            nameEntry = { ["name"] = path..askedParam }
        else
            nameEntry = { ["name"] = path }
        end

        -- add this path to requested
        table.insert(requestParamNames, nameEntry)
        processedPaths = processedPaths + 1

        -- if limit of asked instances for one request is achieved
        -- or last asked path was added to requested one - sent request immediately
        if #requestParamNames == getInstanceCount or processedPaths == pathParamSize then
            local request = {
                -- request header
                header = {
                    callerId = self.callerId,
                    txaId = self:generateTxaId(),
                    respMode = 0,
                    msgType = "GetParamValue"
                },
                -- request body
                body = {
                    nextLevel = true,
                    paramNames = requestParamNames
                }
            } -- request end

            local errCode, response = mmx_frontapi_epexecute_lua(request, 3)
            -- print("<pre> Received response from front API"..ing.utils.tableToString(response).."</pre>")
            if errCode == 0 then
                -- if request was successfully sent to EP
                -- and response was received and parsed without problems - check response header for errors
                errCode = tonumber(response["hdr"]["resCode"])
            end

            combinedErrorCode = math.max(combinedErrorCode, errCode)

            if errCode == 0 and response["body"]["paramNameValuePairs"] ~= nil then
                for _, paramPair in pairs(response["body"]["paramNameValuePairs"]) do
                    assocParams[paramPair["name"]] = paramPair["value"]
                end
            end
            -- print("<pre>path: Assoc params table"..ing.utils.tableToString(assocParams).."</pre>")

            -- clear list of paths to request
            requestParamNames = {}
        end
    end

    for path, wantedParams in pairs(pathParamTable) do
        if string.find(path, "*", 1, true) then
            -- requested path contains placeholders - get all params, which match given placeholder
            local filteredAssocParams = filterAssocParams(assocParams, path)

            for paramFullName, paramValue in pairs(filteredAssocParams) do
                local paramPath, paramName = self:splitMngObjPath(paramFullName)

                -- check if given param was asked
                if ing.utils.tableContainsValue(wantedParams, paramName) then
                    -- get table with name-values pairs for given instance path
                    local pathFields = retRows[paramPath] or {}

                    pathFields[paramName] = paramValue
                    errRows[paramName] = combinedErrorCode

                    -- override existing param table with updated values
                    retRows[paramPath] = pathFields
                end
            end
        else
            local singlePathRetRows = {}
            for _, paramSmallName in pairs(wantedParams) do
                local paramBigName = path..paramSmallName
                singlePathRetRows[paramSmallName] = assocParams[paramBigName] or ""
                errRows[paramSmallName] = combinedErrorCode
            end

            retRows[path] = singlePathRetRows
        end
    end

    -- print("<pre>params: "..ing.utils.tableToString(assocParams).."</pre>")
    -- print("<pre>rets: "..ing.utils.tableToString(retRows).."</pre>")

    return combinedErrorCode, errRows, retRows
end

---
-- Get index by object string
--
-- @param object string Partial path to object (must end with "." and have only one item '{i}')
-- @param value string Value witch we use to find the index
-- @return index number Founded index for this object or nil
-- @usage For example we have next query parameter 'routerid=a323b336-9177-4b32-a93f-5a4edeef2226'
--        We found next path 'Device.Routers.1.Device' for this router-id and resulting index=1
--
function MMXAPIWrapper:getIndexByParamValue(path, value, ep_timeout)

    local _, count = string.gsub(path, "{i}", "{i}")
    local ep_timeout = ep_timeout or 30

    if count == 0 or count > 1  then
        return nil
    end

    local requestObj = string.gsub(path, "{i}", "*", 1)

    local fe_request = {
        header = {
            callerId = self.callerId,
            txaId = self:generateTxaId(),
            respMode = 0,
            msgType = "GetParamValue"
        },
        body={
            paramNames = {
                {["name"] = requestObj}
            }
        }
    }

    local errcode, response = mmx_frontapi_epexecute_lua(fe_request, ep_timeout)

    -- Exit if we have error and return nil
    if errcode ~= 0 then
        return nil
    end

    local index = nil
    if (response["body"]["paramNameValuePairs"]) then
        for key, responseTable in pairs(response["body"]["paramNameValuePairs"]) do
            if value == responseTable["value"] then
                index = (string.match (responseTable["name"], "^.+%.(%d+)%..+$"))
                break
            end
        end
    else
        return nil
    end

    return index
end

---
-- Modify path for specified {section}
--
-- @param section table Section information from mmx_web_info
-- @return path string Partial path to object (must ends with ".").
--
function MMXAPIWrapper:modifyPath(section)

    local prevIndex = nil
    local path = section["mmgModObjName"]

    -- Obtaining query parameters from the URL.
    local queryTable = luci.http.formvalue()

    for k, v in pairs (section["mngModParams"]) do

        if queryTable[k] then

            -- Replacing previous index "{i}" for next subrequest if needed
            if prevIndex ~= nil then
                v = string.gsub(v, "{i}", prevIndex, 1)
            end

            local index = self:getIndexByParamValue(v, queryTable[k])

            if index ~= nil then
                prevIndex = index

                -- Replacing "{i}" in object name
                path = string.gsub(path, "{i}", index, 1)
            else
                path = nil
            end

        end
    end

    return path
end

---
-- Performs request to MMX Entrypoint to enumerate rows stored in object {path}
--
-- @param path string Partial path to object (must ends with "."), and last part of path must contain enumerable name.
-- @return number  Non-zero value indicates whether any error returned from Entrypoint
-- @return table   Array of rows in object (keys are ordinal number, values are full path to row-object with last index)
-- @usage Example for enumerable (multi-instances) object "Device.Ghn.Interface.{i}.":
--        Example of usage: MMXAPIWrapper:countRow("Device.Ghn.Interface.{i}.")
--
function MMXAPIWrapper:countRow(path)
    --replace all {i} placeholder to "*"
    path = string.gsub(path, "{i}", "*")
    -- split full path to two subpath: starting from beginning to  last "*"(path_core) , from last "*"  to "*"(path_tail)
    -- For ex: Full path = Device.IP.Interface.*.stats
    -- path_core = Device.IP.Interface.*, path_tail = stats
    local path_core, path_tail = string.match(path, "([%w_%d%.%*]+%.[%*%d%[%]%-]+%.)([%w_%d%.]*)$")
    local retRows = {}

    -- Determine name of the last index parameter of the object:
    -- get the last node in the path (i.e. pattern in path between "." and ".*." )
    -- and add to it the "Index" string.
    -- The result string is the last index parameter.
    -- For ex: path = Device.Ghn.Interface. last node = Interface
    --         the last index parameter name is InterfaceIndex
    local lastNodeName = string.match (path_core, "^.+%.([%w_%d]+)%.[%*%d%[%]%-]+%.$")

    --build full-path parameter name needed for retrieving all existing indexes as follows:
    --Device.Ghn.Interface.*.InterfaceIndex
    local countPath = path_core .. lastNodeName .."Index"
    local request = {
        header={
            callerId = self.callerId,
            txaId = self:generateTxaId(),
            respMode = 0,
            msgType = "GetParamValue"
        },
        body={
            paramNames = {
                {["name"] = countPath}
            }
        }
    }
    local errcode, response = mmx_frontapi_epexecute_lua(request, self.apiTimeout)
    if errcode == 0 then
        -- if request was successfully sent to EP and response was received and parsed without problems - check response header for errors
        errcode = tonumber(response["hdr"]["resCode"])
    end

    if errcode == 0 and response["body"]["paramNameValuePairs"] ~= nil then
        for _, valuePair in pairs(response["body"]["paramNameValuePairs"]) do
            -- cut lastNodeName from response path and concatenate path tail if need
            -- For ex: response path = Device.IP.Interface.1.InterfaceIndex
            --         retRows[i] = Device.IP.Interface.1.stats
            retRows[#retRows + 1] = string.match(valuePair["name"], "^(.+%.[%w_%d]+%.)" .. lastNodeName .. "Index") .. (path_tail or "")
        end
    end
--  print("<pre>req: "..ing.utils.tableToString(request).."</pre>")
--  print("<pre>resp: "..ing.utils.tableToString(response).."</pre>")
--  print("<pre>rets: "..ing.utils.tableToString(retRows).."</pre>")
    return errcode, retRows
end

---
-- Perfroms request to MMX Entrypoint to "discover config" (update rows/parameters values)
--
-- @param path string Partial path to object (must ends with ".")
-- @return number Non-zero value indicates whether any error returned from Entrypoint
-- @return table Parsed XML-response from Entrypoint
--
function MMXAPIWrapper:discoverConfig(path)
    local request = {
            header={
                    callerId = self.callerId,
                    txaId = self:generateTxaId(),
                    respMode = 2, -- need to be "0" for sync mode
                    msgType = "DiscoverConfig"
                    },
            body={
                    ["objName"] = path
            }
    }
    local errcode, response = mmx_frontapi_epexecute_lua(request, self.apiTimeout)
    os.execute("sleep 2") -- need to be removed after enable blocking mode in Entrypoint
    return errcode, response
end


--[[
--   Function sends SetParamValue request to EP with specified params
--    Input params:
--       objInstName   - object instance name (ending by ".")
--       setType       - bitmask, possible values: apply - 1, save - 2, test - 4 (example for apply & save is "3")
--       setValues     - table of "name = value" pairs
--                       for each parameter that should be set.
--                       name is full-path parameter name;
-- example of objInstName: Device.Ethernet.Interface.1.

-- example of setValues:
--   {
--     Enable = "true",
--     MaxBitRate = "100"
--   }
--]]
function MMXAPIWrapper:setParamValue (objInstName, setType, setValues)
    -- build paramNameValuePairs table for frontapi request
    local setParamNameValuePairs = {}
    for paramName,paramVal in pairs(setValues or {}) do
        table.insert(setParamNameValuePairs, {name = objInstName..paramName, value = paramVal})
    end

    local request = {
        header = { callerId = self.callerId,
                   txaId = self:generateTxaId(),
                   respMode = '0',
                   msgType = 'SetParamValue'
        },
        body = {
                 setType = setType,
                 paramNameValuePairs = setParamNameValuePairs
       }
    }

    --print("request:\n"..ing.utils.tableToString(request))
    -- sent request to entry point
    local errcode, response = mmx_frontapi_epexecute_lua(request, 6)
    if errcode == 0 then
        -- if request was successfully sent to EP and response was received and parsed without problems - check response header for errors
        errcode = tonumber(response["hdr"]["resCode"])
    end

    return errcode, response
end

---
-- Performs request to create new row in specified {objectPath} with specified parameters.
--
-- @usage MMXAPIWrapper:addInstance("Device.Ethernet.Interface.", {Name="eth1", Enable="true"}) for adding
--        new row to object "Device.Ethernet.Interface.{i}"
--
-- @usage MMXAPIWrapper:addInstance("Device.Ghn.Interface.2.AssociatedDevice.", {DeviceId="123456", Active="true"}) for adding
--        new row to object "Device.Ghn.Interface.{i}.AssociatedDevice.{i}."
--
-- @param objectPath string Object name without last index placeholder
-- @param params table Associative array of parameters, which Key => Value pair is parameter's name and value
-- @return number Non-zero value indicates whether any error returned from Entrypoint
-- @return table Parsed XML-response from Entrypoint
--
function MMXAPIWrapper:addInstance(objectPath, params)
    paramPairs = {}
    for paramName, paramValue in pairs(params or {}) do
        table.insert(paramPairs, {name = paramName, value = paramValue})
    end
    local request = {
        header = {
            callerId = self.callerId,
            txaId = self:generateTxaId(),
            respMode = '0',
            msgType = 'AddObject'
        },
        body = {
            objName = objectPath,
            paramNameValuePairs = paramPairs
        }
    }
    local errcode, response = mmx_frontapi_epexecute_lua(request, 6)
    if errcode == 0 then
        -- if request was successfully sent to EP and response was received and parsed without problems - check response header for errors
        errcode = tonumber(response["hdr"]["resCode"])
    end

    return errcode, response
end

---
-- Performs request to delete specified row from table object.
--
-- @usage MMXAPIWrapper:deleteInstance("Device.Ethernet.Interface.1.") for deletion
--        row from "Device.Ethernet.Interface.{i}"
--
-- @usage MMXAPIWrapper:deleteInstance("Device.Ghn.Interface.3.AssociatedDevice.2.") for deletion
--        row from "Device.Ghn.Interface.{i}.AssociatedDevice.{i}."
--
-- @param objectInstName string Full object name
-- @return number Non-zero value indicates whether any error returned from Entrypoint
-- @return table Parsed XML-response from Entrypoint
--
function MMXAPIWrapper:deleteInstance(objInstName)
    local request = {
        header = {
            callerId = self.callerId,
            txaId = self:generateTxaId(),
            respMode = '0',
            msgType = 'DelObject'
        },
        body = {
            objects = {
                { objName = objInstName }
            }
        }
    }
    local errcode, response = mmx_frontapi_epexecute_lua(request, 6)
    if errcode == 0 then
        -- if request was successfully sent to EP and response was received and parsed without problems - check response header for errors
        errcode = tonumber(response["hdr"]["resCode"])
    end

    return errcode, response
end
