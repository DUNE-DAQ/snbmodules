local moo = import "moo.jsonnet";
local ns = "dunedaq.snbmodules.snbfiletransfer";
local s = moo.oschema.schema(ns);

local types = {

    int4 :    s.number(  "int4",    "i4",          doc="A signed integer of 4 bytes"),
    uint4 :   s.number(  "uint4",   "u4",          doc="An unsigned integer of 4 bytes"),
    int8 :    s.number(  "int8",    "i8",          doc="A signed integer of 8 bytes"),
    uint8 :   s.number(  "uint8",   "u8",          doc="An unsigned integer of 8 bytes"),
    float4 :  s.number(  "float4",  "f4",          doc="A float of 4 bytes"),
    double8 : s.number(  "double8", "f8",          doc="A double of 8 bytes"),
    boolean :  s.boolean( "Boolean",                doc="A boolean"),
    string :   s.string(  "String",   		   doc="A string"),
    // string_array :   s.sequence(  "String Array", s.string(  "String", doc="A string"), doc="A string array"),  
    // json :   s.any(  "nlohmann::json",   		   doc="A json object"),   
    // protocol : s.enum("Protocols", ["BITTORRENT", "RCLONE", "SCP", "dummy"], doc="Protocols for file transfer"),

    conf: s.record("ConfParams", [
                                s.field("client_ip", self.string,
                                           doc="IPV4:PORT The IP address used by the client, you can precise the interface used here"),
                                s.field("work_dir", self.string, "./",
                                           doc="Directory where the client is gonna watch for files to share with Bookkeeper and where files are Downloaded by default (uploaded files don't have to be in here)"),
                                s.field("connection_prefix", self.string, "snbmodules",
                                           doc="Prefix of the connections name, for the plugin to find others connections"),
                                s.field("timeout_send", self.uint8, "10",
                                           doc="Max time in ms passed sending a notification"),
                                s.field("timeout_receive", self.uint8, "100",
                                           doc="Max time in ms listening for a notification"),
                           ],
                   doc="Configuration of a new SNB file transfer client"),

//     conf: s.record("Command", [
//                            s.field("transfer_id", self.string, "transfer0",
//                                            doc="ID of the transfer to be paused/resumed/cancelled"),
//                            ],
//                    doc="Configuration for standard commands (pause, resume, cancel)"),

//     conf: s.record("NewTransferCommand", [
//                            s.field("transfer_id", self.string, "transfer0",
//                                            doc="ID of the transfer to be created"),
//                            s.field("source", self.string, "client0",
//                                            doc="Client ID of the source of the transfer"),
//                            s.field("dests", self.string_array, [],
//                                            doc="List of the destination client_IDs of the transfer"),
//                            s.field("files", self.string_array, [],
//                                            doc="List of path to files to be transferred in source client"),
//                            s.field("protocol", self.protocol, "dummy",
//                                            doc="Communication protocol to be used for the transfer"),
//                            s.field("protocol_args", self.json, "{}",
//                                            doc="Specific arguments for the protocol, they may vary depending on the protocol used"),
//                            ],
//                    doc="Configuration for the creation of a new transfer"),

};

moo.oschema.sort_select(types, ns)
