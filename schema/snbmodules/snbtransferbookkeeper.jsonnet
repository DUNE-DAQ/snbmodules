local moo = import "moo.jsonnet";
local ns = "dunedaq.snbmodules.snbtransferbookkeeper";
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

    conf: s.record("ConfParams", [
                            s.field("bookkeeper_ip", self.string, "0.0.0.0:0",
                                           doc="IPV4:PORT address of the bookkeeper"),
                            s.field("bookkeeper_log_path", self.string, "",
                                           doc="Path to the directory where the log files are stored, leave empty for standard output"),
                            s.field("refresh_rate", self.uint8, 5,
                                           doc="Refresh of transfer data by the bookkeeper in seconds"),

                            s.field("connection_prefix", self.string, "snbmodules",
                                           doc="Prefix of the connections name, for the plugin to find others connections"),
                            s.field("timeout_send", self.uint8, "10",
                                           doc="Max time in ms passed sending a notification"),
                            s.field("timeout_receive", self.uint8, "100",
                                           doc="Max time in ms listening for a notification"),
                           ],
                   doc="Configuration of a new SNB file transfer client"),

};

moo.oschema.sort_select(types, ns)
