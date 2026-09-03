#ifndef __CHDK_PTP_H
#define __CHDK_PTP_H

// CHDK PTP protocol interface (can also be used in client PTP programs)

// Note: used in modules and platform independent code. 
// Do not add platform dependent stuff in here (#ifdef/#endif compile options or camera dependent values)

#define PTP_CHDK_VERSION_MAJOR 2  // increase only with backwards incompatible changes (and reset minor)
#define PTP_CHDK_VERSION_MINOR 6  // increase with extensions of functionality
                                  // minor > 1000 for development versions

/*
protocol version history
0.1 - initial proposal from mweerden, + luar
0.2 - Added ScriptStatus and ScriptSupport, based on work by ultimA
1.0 - removed old script result code (luar), replace with message system
2.0 - return PTP_CHDK_TYPE_TABLE for tables instead of TYPE_STRING, allow return of empty strings
2.1 - experimental live view, not formally released
2.2 - live view (work in progress)
2.3 - live view - released in 1.1
2.4 - live view protocol 2.1
2.5 - remote capture
2.6 - script execution flags
*/

#define PTP_OC_CHDK 0x9999

// N.B.: unused parameters should be set to 0
//enum ptp_chdk_command {
enum PTP_CHDK_Command {
  PTP_CHDK_Version = 0,     // return param1 is major version number
                            // return param2 is minor version number
  PTP_CHDK_GetMemory,       // param2 is base address (not NULL; circumvent by taking 0xFFFFFFFF and size+1)
                            // param3 is size (in bytes)
                            // return data is memory block
  PTP_CHDK_SetMemory,       // param2 is address
                            // param3 is size (in bytes)
                            // data is new memory block
  PTP_CHDK_CallFunction,    // data is array of function pointer and 32 bit int arguments (max: 10 args prior to protocol 2.5)
                            // return param1 is return value
  PTP_CHDK_TempData,        // data is data to be stored for later
                            // param2 is for the TD flags below
  PTP_CHDK_UploadFile,      // data is 4-byte length of filename, followed by filename and contents
  PTP_CHDK_DownloadFile,    // preceded by PTP_CHDK_TempData with filename
                            // return data are file contents
  PTP_CHDK_ExecuteScript,   // data is script to be executed
                            // param2 is language of script
                            //  in proto 2.6 and later, language is the lower byte, rest is used for PTP_CHDK_SCRIPT_FL* flags
                            // return param1 is script id, like a process id
                            // return param2 is status from ptp_chdk_script_error_type
  PTP_CHDK_ScriptStatus,    // Script execution status
                            // return param1 bits
                            // PTP_CHDK_SCRIPT_STATUS_RUN is set if a script running, cleared if not
                            // PTP_CHDK_SCRIPT_STATUS_MSG is set if script messages from script waiting to be read
                            // all other bits and params are reserved for future use
  PTP_CHDK_ScriptSupport,   // Which scripting interfaces are supported in this build
                            // param1 CHDK_PTP_SUPPORT_LUA is set if lua is supported, cleared if not
                            // all other bits and params are reserved for future use
  PTP_CHDK_ReadScriptMsg,   // read next message from camera script system
                            // return param1 is chdk_ptp_s_msg_type
                            // return param2 is message subtype:
                            //   for script return and users this is ptp_chdk_script_data_type
                            //   for error ptp_chdk_script_error_type
                            // return param3 is script id of script that generated the message
                            // return param4 is length of the message data. 
                            // return data is message.
                            // A minimum of 1 bytes of zeros is returned if the message has no data (empty string or type NONE)
  PTP_CHDK_WriteScriptMsg,  // write a message for scripts running on camera
                            // input param2 is target script id, 0=don't care. Messages for a non-running script will be discarded
                            // data length is handled by ptp data phase
                            // input messages do not have type or subtype, they are always a string destined for the script (similar to USER/string)
                            // output param1 is ptp_chdk_script_msg_status
  PTP_CHDK_GetDisplayData,  // Return camera display data
                            // This is defined as separate sub protocol in live_view.h
                            // Changes to the sub-protocol will always be considered a minor change to the main protocol
                            //  param2 bitmask of data
                            //  output param1 = total size of data
                            //  return data is protocol information, frame buffer descriptions and selected display data
                            //  Currently a data phase is always returned. Future versions may define other behavior 
                            //  for values in currently unused parameters.
  // Direct image capture over USB.
  // Use lua get_usb_capture_support for available data types, lua init_usb_capture for setup
  PTP_CHDK_RemoteCaptureIsReady, // Check if data is available
                                 // return param1 is status 
                                 //  0 = not ready
                                 //  0x10000000 = remote capture not initialized
                                 //  otherwise bitmask of PTP_CHDK_CAPTURE_* datatypes
                                 // return param2 is image number
  PTP_CHDK_RemoteCaptureGetData  // retrieve data
                                 // param2 is bit indicating data type to get
                                 // return param1 is length
                                 // return param2 more chunks available?
                                 //  0 = no more chunks of selected format
                                 // return param3 seek required to pos (-1 = no seek)
};

// data types as used by ReadScriptMessage
enum ptp_chdk_script_data_type {
  PTP_CHDK_TYPE_UNSUPPORTED = 0, // type name will be returned in data
  PTP_CHDK_TYPE_NIL,
  PTP_CHDK_TYPE_BOOLEAN,
  PTP_CHDK_TYPE_INTEGER,
  PTP_CHDK_TYPE_STRING, // Empty strings are returned with length=0
  PTP_CHDK_TYPE_TABLE,  // tables are converted to a string by usb_msg_table_to_string, 
                        // this function can be overridden in lua to change the format
                        // the string may be empty for an empty table
};

// TempData flags
#define PTP_CHDK_TD_DOWNLOAD  0x1  // download data instead of upload
#define PTP_CHDK_TD_CLEAR     0x2  // clear the stored data; with DOWNLOAD this
                                   // means first download, then clear and
                                   // without DOWNLOAD this means no uploading,
                                   // just clear

// Script Languages - for execution only lua is supported for now
#define PTP_CHDK_SL_LUA    0
#define PTP_CHDK_SL_UBASIC 1
#define PTP_CHDK_SL_MASK 0xFF

/* standard message chdkptp sends */
#define PTP_CHDK_LUA_SERIALIZE "\n\
serialize_r = function(v,opts,r,seen,depth)\n\
	local vt = type(v)\n\
	if vt == 'nil' or  vt == 'boolean' or vt == 'number' then\n\
		table.insert(r,tostring(v))\n\
		return\n\
	end\n\
	if vt == 'string' then\n\
		table.insert(r,string.format('%%q',v))\n\
		return\n\
	end\n\
	if vt == 'table' then\n\
		if not depth then\n\
			depth = 1\n\
		end\n\
		if depth >= opts.maxdepth then\n\
			error('serialize: max depth')\n\
		end\n\
		if not seen then\n\
			seen={}\n\
		elseif seen[v] then\n\
			if opts.err_cycle then\n\
				error('serialize: cycle')\n\
			else\n\
				table.insert(r,'\"cycle:'..tostring(v)..'\"')\n\
				return\n\
			end\n\
		end\n\
		seen[v] = true;\n\
		table.insert(r,'{')\n\
		for k,v1 in pairs(v) do\n\
			if opts.pretty then\n\
				table.insert(r,'\\n'..string.rep(' ',depth))\n\
			end\n\
			if type(k) == 'string' and string.match(k,'^[_%%a][%%a%%d_]*$') then\n\
				table.insert(r,k)\n\
			else\n\
				table.insert(r,'[')\n\
				serialize_r(k,opts,r,seen,depth+1)\n\
				table.insert(r,']')\n\
			end\n\
			table.insert(r,'=')\n\
			serialize_r(v1,opts,r,seen,depth+1)\n\
			table.insert(r,',')\n\
		end\n\
		if opts.pretty then\n\
			table.insert(r,'\\n'..string.rep(' ',depth-1))\n\
		end\n\
		table.insert(r,'}')\n\
		return\n\
	end\n\
	if opts.err_type then\n\
		error('serialize: unsupported type ' .. vt, 2)\n\
	else\n\
		table.insert(r,'\"'..tostring(v)..'\"')\n\
	end\n\
end\n\
serialize_defaults = {\n\
	maxdepth=10,\n\
	err_type=true,\n\
	err_cycle=true,\n\
	pretty=false,\n\
}\n\
function serialize(v,opts)\n\
	if opts then\n\
		for k,v in pairs(serialize_defaults) do\n\
			if not opts[k] then\n\
				opts[k]=v\n\
			end\n\
		end\n\
	else\n\
		opts=serialize_defaults\n\
	end\n\
	local r={}\n\
	serialize_r(v,opts,r)\n\
	return table.concat(r)\n\
end\n"

#define PTP_CHDK_LUA_SERIALIZE_SIMPLEQUOTE "\n\
serialize_r = function(v,opts,r,seen,depth)\n\
	local vt = type(v)\n\
	if vt == 'nil' or  vt == 'boolean' or vt == 'number' then\n\
		table.insert(r,tostring(v))\n\
		return\n\
	end\n\
	if vt == 'string' then\n\
		table.insert(r,string.format('%q',v))\n\
		return\n\
	end\n\
	if vt == 'table' then\n\
		if not depth then\n\
			depth = 1\n\
		end\n\
		if depth >= opts.maxdepth then\n\
			error('serialize: max depth')\n\
		end\n\
		if not seen then\n\
			seen={}\n\
		elseif seen[v] then\n\
			if opts.err_cycle then\n\
				error('serialize: cycle')\n\
			else\n\
				table.insert(r,'\"cycle:'..tostring(v)..'\"')\n\
				return\n\
			end\n\
		end\n\
		seen[v] = true;\n\
		table.insert(r,'{')\n\
		for k,v1 in pairs(v) do\n\
			if opts.pretty then\n\
				table.insert(r,'\\n'..string.rep(' ',depth))\n\
			end\n\
			if type(k) == 'string' and string.match(k,'^[_%a][%a%d_]*$') then\n\
				table.insert(r,k)\n\
			else\n\
				table.insert(r,'[')\n\
				serialize_r(k,opts,r,seen,depth+1)\n\
				table.insert(r,']')\n\
			end\n\
			table.insert(r,'=')\n\
			serialize_r(v1,opts,r,seen,depth+1)\n\
			table.insert(r,',')\n\
		end\n\
		if opts.pretty then\n\
			table.insert(r,'\\n'..string.rep(' ',depth-1))\n\
		end\n\
		table.insert(r,'}')\n\
		return\n\
	end\n\
	if opts.err_type then\n\
		error('serialize: unsupported type ' .. vt, 2)\n\
	else\n\
		table.insert(r,'\"'..tostring(v)..'\"')\n\
	end\n\
end\n\
serialize_defaults = {\n\
	maxdepth=10,\n\
	err_type=true,\n\
	err_cycle=true,\n\
	pretty=false,\n\
}\n\
function serialize(v,opts)\n\
	if opts then\n\
		for k,v in pairs(serialize_defaults) do\n\
			if not opts[k] then\n\
				opts[k]=v\n\
			end\n\
		end\n\
	else\n\
		opts=serialize_defaults\n\
	end\n\
	local r={}\n\
	serialize_r(v,opts,r)\n\
	return table.concat(r)\n\
end\n"

#define PTP_CHDK_LUA_SERIALIZE_MSGS \
PTP_CHDK_LUA_SERIALIZE\
"usb_msg_table_to_string=serialize\n"

#define PTP_CHDK_LUA_SERIALIZE_MSGS_SIMPLEQUOTE \
PTP_CHDK_LUA_SERIALIZE_SIMPLEQUOTE\
"usb_msg_table_to_string=serialize\n"

#define PTP_CHDK_LUA_EXTEND_TABLE \
"function extend_table(target,source,deep)\n\
	if type(target) ~= 'table' then\n\
		error('extend_table: target not table')\n\
	end\n\
	if source == nil then\n\
		return target\n\
	end\n\
	if type(source) ~= 'table' then \n\
		error('extend_table: source not table')\n\
	end\n\
	if source == target then\n\
		error('extend_table: source == target')\n\
	end\n\
	if deep then\n\
		return extend_table_r(target, source)\n\
	else \n\
		for k,v in pairs(source) do\n\
			target[k]=v\n\
		end\n\
		return target\n\
	end\n\
end\n"

#define PTP_CHDK_LUA_MSG_BATCHER	\
PTP_CHDK_LUA_SERIALIZE_MSGS \
PTP_CHDK_LUA_EXTEND_TABLE \
"function msg_batcher(opts)\n\
	local t = extend_table({\n\
		batchsize=50,\n\
		batchgc='step',\n\
		timeout=100000,\n\
	},opts)\n\
	t.data={}\n\
	t.n=0\n\
	if t.dbgmem then\n\
		t.init_free = get_meminfo().free_block_max_size\n\
		t.init_count = collectgarbage('count')\n\
	end\n\
	t.write=function(self,val)\n\
		self.n = self.n+1\n\
		self.data[self.n]=val\n\
		if self.n >= self.batchsize then\n\
			return self:flush()\n\
		end\n\
		return true\n\
	end\n\
	t.flush = function(self)\n\
		if self.n > 0 then\n\
			if self.dbgmem then\n\
				local count=collectgarbage('count')\n\
				local free=get_meminfo().free_block_max_size\n\
				self.data._dbg=string.format(\"count %%d (%%d) free %%d (%%d)\",\n\
					count, count - self.init_count, free, self.init_free-free)\n\
			end\n\
			if not write_usb_msg(self.data,self.timeout) then\n\
				return false\n\
			end\n\
			self.data={}\n\
			self.n=0\n\
			if self.batchgc then\n\
				collectgarbage(self.batchgc)\n\
			end\n\
			if self.batchpause then\n\
				sleep(self.batchpause)\n\
			end\n\
		end\n\
		return true\n\
	end\n\
	return t\n\
end\n"

#define PTP_CHDK_LUA_LS_SIMPLE \
PTP_CHDK_LUA_MSG_BATCHER  \
"function ls_simple(path)\n\
	local b=msg_batcher()\n\
	local t,err=os.listdir(path)\n\
	if not t then\n\
		return false,err\n\
	end\n\
	for i,v in ipairs(t) do\n\
		if not b:write(v) then\n\
			return false\n\
		end\n\
	end\n\
	return b:flush()\n\
end\n"

#define PTP_CHDK_LUA_JOINPATH \
"function joinpath(...)\n\
	local parts={...}\n\
	if #parts < 2 then\n\
		error('joinpath requires at least 2 parts',2)\n\
	end\n\
	local r=parts[1]\n\
	for i = 2, #parts do\n\
		local v = string.gsub(parts[i],'^/','')\n\
		if not string.match(r,'/$') then\n\
			r=r..'/'\n\
		end\n\
		r=r..v\n\
	end\n\
	return r\n\
end\n"

#define PTP_CHDK_LUA_LS \
PTP_CHDK_LUA_MSG_BATCHER \
PTP_CHDK_LUA_JOINPATH \
"function ls_single(opts,b,path,v)\n\
	if not opts.match or string.match(v,opts.match) then\n\
		if opts.stat then\n\
			local st,msg=os.stat(joinpath(path,v))\n\
			if not st then\n\
				return false,msg\n\
			end\n\
			if opts.stat == '/' then\n\
				if st.is_dir then\n\
					b:write(v .. '/')\n\
				else\n\
					b:write(v)\n\
				end\n\
			elseif opts.stat == '*' then\n\
				st.name=v\n\
				b:write(st)\n\
			end\n\
		else\n\
			b:write(v)\n\
		end\n\
	end\n\
	return true\n\
end\n\
\n\
function ls(path,opts_in)\n\
	local opts={\n\
		msglimit=50,\n\
		msgtimeout=100000,\n\
		dirsonly=true\n\
	}\n\
	if opts_in then\n\
		for k,v in pairs(opts_in) do\n\
			opts[k]=v\n\
		end\n\
	end\n\
	local st, err = os.stat(path)\n\
	if not st then\n\
		return false, err\n\
	end\n\
	\n\
	local b=msg_batcher{\n\
		batchsize=opts.msglimit,\n\
		timeout=opts.msgtimeout\n\
	}\n\
	\n\
	if not st.is_dir then\n\
		if opts.dirsonly then\n\
			return false, 'not a directory'\n\
		end\n\
		if opts.stat == '*' then\n\
			st.name=path\n\
			b:write(st)\n\
		else\n\
			b:write(path)\n\
		end\n\
		b:flush()\n\
		return true\n\
	end\n\
	\n\
	if os.idir then\n\
		for v in os.idir(path,opts.listall) do\n\
			local status,err=ls_single(opts,b,path,v)\n\
			if not status then\n\
				return false, err\n\
			end\n\
		end\n\
	else\n\
		local t,msg=os.listdir(path,opts.listall)\n\
		if not t then\n\
			return false,msg\n\
		end\n\
		for i,v in ipairs(t) do\n\
			local status,err=ls_single(opts,b,path,v)\n\
			if not status then\n\
				return false, err\n\
			end\n\
		end\n\
	end\n\
	b:flush()\n\
	return true\n\
end\n"

#define PTP_CHDK_LUA_RLIB_SHOOT_COMMON \
"function rlib_shoot_init_exp(opts)	\n\
	if opts.tv then\n\
		set_tv96_direct(opts.tv)\n\
	end\n\
	if opts.sv then\n\
		set_sv96(opts.sv)\n\
	end\n\
	if opts.svm then\n\
		if type(sv96_market_to_real) ~= 'function' then\n\
			error('svm not supported')\n\
		end\n\
		set_sv96(sv96_market_to_real(opts.svm))\n\
	end\n\
	if opts.isomode then\n\
		set_iso_mode(opts.isomode)\n\
	end\n\
	if opts.av then\n\
		set_av96_direct(opts.av)\n\
	end\n\
	if opts.nd then\n\
		set_nd_filter(opts.nd)\n\
	end\n\
	if opts.sd then\n\
		set_focus(opts.sd)\n\
	end\n\
end\n"

#define PTP_CHDK_LUA_RLIB_SHOOT \
PTP_CHDK_LUA_RLIB_SHOOT_COMMON \
"function rlib_shoot(opts)\n\
	local rec,vid = get_mode()\n\
	if not rec then\n\
		return false,'not in rec mode'\n\
	end\n\
\n\
	rlib_shoot_init_exp(opts)\n\
\n\
	local save_raw\n\
	if opts.raw then\n\
		save_raw=get_raw()\n\
		set_raw(opts.raw)\n\
	end\n\
	local save_dng\n\
	if opts.dng then\n\
		save_dng=get_config_value(226)\n\
		set_config_value(226,opts.dng)\n\
	end\n\
	shoot()\n\
	local r\n\
	if opts.info then\n\
		r = {\n\
			dir=get_image_dir(),\n\
			exp=get_exp_count(),\n\
			raw=(get_raw() == 1),\n\
		}\n\
		if r.raw then\n\
			r.raw_in_dir = (get_config_value(35) == 1)\n\
			r.raw_pfx = get_config_value(36)\n\
			r.raw_ext = get_config_value(37)\n\
			r.dng = (get_config_value(226) == 1)\n\
			if r.dng then\n\
				r.use_dng_ext = (get_config_value(234) == 1)\n\
			end\n\
		end\n\
	else\n\
		r=true\n\
	end\n\
	if save_raw then\n\
		set_raw(save_raw)\n\
	end\n\
	if save_dng then\n\
		set_config_value(226,save_dng)\n\
	end\n\
	return r\n\
end\n"



// bit flags for script start
#define PTP_CHDK_SCRIPT_FL_NOKILL           0x100 // if script is running return error instead of killing
#define PTP_CHDK_SCRIPT_FL_FLUSH_CAM_MSGS   0x200 // discard existing cam->host messages before starting
#define PTP_CHDK_SCRIPT_FL_FLUSH_HOST_MSGS  0x400 // discard existing host->cam messages before starting

// bit flags for script status
#define PTP_CHDK_SCRIPT_STATUS_RUN   0x1 // script running
#define PTP_CHDK_SCRIPT_STATUS_MSG   0x2 // messages waiting
// bit flags for scripting support
#define PTP_CHDK_SCRIPT_SUPPORT_LUA  0x1


// bit flags for remote capture
// used to select and also to indicate available data in PTP_CHDK_RemoteCaptureIsReady
/*
Full jpeg file. Note supported on all cameras, use Lua get_usb_capture_support to check
*/
#define PTP_CHDK_CAPTURE_JPG    0x1 

/*
Raw framebuffer data, in camera native format.
A subset of rows may be requested in init_usb_capture.
*/
#define PTP_CHDK_CAPTURE_RAW    0x2

/*
DNG header. 
The header will be DNG version 1.3
Does not include image data, clients wanting to create a DNG file should also request RAW
Raw data for all known cameras will be packed, little endian. Client is responsible for
reversing the byte order if creating a DNG.
Can requested without RAW to get sensor dimensions, exif values etc.

ifd 0 specifies a 128x96 RGB thumbnail, 4 byte aligned following the header
client is responsible for generating thumbnail data.

ifd 0 subifd 0 specifies the main image
The image dimensions always contain the full sensor dimensions, if a sub-image was requested
with init_usb_capture, the client is responsible for padding the data to the full image or
adjusting dimensions.

Bad pixels will not be patched, but DNG opcodes will specify how to patch them
*/
#define PTP_CHDK_CAPTURE_DNGHDR 0x4  

// status from PTP_CHDK_RemoteCaptureIsReady if capture not enabled
#define PTP_CHDK_CAPTURE_NOTSET 0x10000000

// message types
enum ptp_chdk_script_msg_type {
    PTP_CHDK_S_MSGTYPE_NONE = 0, // no messages waiting
    PTP_CHDK_S_MSGTYPE_ERR,      // error message
    PTP_CHDK_S_MSGTYPE_RET,      // script return value
    PTP_CHDK_S_MSGTYPE_USER,     // message queued by script
// TODO chdk console data ?
};

// error subtypes for PTP_CHDK_S_MSGTYPE_ERR and script startup status
enum ptp_chdk_script_error_type {
    PTP_CHDK_S_ERRTYPE_NONE = 0,
    PTP_CHDK_S_ERRTYPE_COMPILE,
    PTP_CHDK_S_ERRTYPE_RUN,
    // the following are for ExecuteScript status only, not message types
    PTP_CHDK_S_ERR_SCRIPTRUNNING = 0x1000, // script already running with NOKILL
};

// message status
enum ptp_chdk_script_msg_status {
    PTP_CHDK_S_MSGSTATUS_OK = 0, // queued ok
    PTP_CHDK_S_MSGSTATUS_NOTRUN, // no script is running
    PTP_CHDK_S_MSGSTATUS_QFULL,  // queue is full
    PTP_CHDK_S_MSGSTATUS_BADID,  // specified ID is not running
};

#endif // __CHDK_PTP_H
