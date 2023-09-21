// This is the configuration schema for daqconf_multiru_gen
//

local moo = import "moo.jsonnet";

local stypes = import "daqconf/types.jsonnet";
local types = moo.oschema.hier(stypes).dunedaq.daqconf.types;

local s = moo.oschema.schema("dunedaq.snbmodules.snbmodulesgen");
local nc = moo.oschema.numeric_constraints;

local cs = {
  snbmodulesapp: s.record("snbmodulesapp", [
    s.field( "host", types.host, default="localhost", doc="Set the host starting the clients" ),
    s.field( "interface", types.ipv4, default="0.0.0.0", doc="Set the IP address interface used by the snbmodules to transfer data" ),
    s.field( "client_num", types.uint4, default=1, doc="Set how many Clients need to be started on the host" ),
    s.field( "client_name", types.string, default="snbclient", doc="Set the base name of the client (number will be added with his number)" ),
    s.field( "client_starting_port", types.port, default=0, doc="Set the First port used by the SNBmodules Clients to transfer data" ),
    s.field( "clients_root_dir", types.path, default="./", doc="Set the root directory where clients files are saved" ),
  ]),

  snbmodulesapps: s.sequence("snbmodulesapps", self.snbmodulesapp, doc="List of snbmodulesapp instances"),

  snbmodules: s.record("snbmodules", [
    s.field( "apps", self.snbmodulesapps, default=[], doc="Configuration for the Clients snbmodules apps (see snbmodulesapp for options)"),
    s.field( "have_bookkeeper", types.flag, default=false, doc="Set if the bookkeeper is started here or not" ),
    s.field( "host_bookkeeper", types.host, default='localhost', doc="Sets the host for the Bookkeeper app"),
    s.field( "bookkeeper_name", types.string, default="snbbookkeeper", doc="Set the name of the Bookkeeper module" ),
    s.field( "bookkeeper_log_path", types.string, default="./", doc="Set the output log path of the Bookkeeper, empty for standard log output" ),
    s.field( "bookkeeper_port", types.port, default=0, doc="Set the port used by the Bookkeeper to communicate" ),
    s.field( "bookkeeper_refresh_rate", types.uint8, default=1, doc="Set the update rate of the Bookkeeper" ),
    s.field( "snb_connections_prefix", types.string, default="snbmodules", doc="Set the prefix string to the connections names" ),
    s.field( "snb_timeout_notification_send", types.uint8, default=10, doc="Set the timeout time (ms) for sending notifications" ),
    s.field( "snb_timeout_notification_receive", types.uint8, default=100, doc="Set the timeout time (ms) for receiving notifications" ),
  ]),
};


stypes + moo.oschema.sort_select(cs)
