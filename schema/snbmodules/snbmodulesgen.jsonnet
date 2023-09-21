// This is the configuration schema for daqconf_multiru_gen
//

local moo = import "moo.jsonnet";

local stypes = import "daqconf/types.jsonnet";
local types = moo.oschema.hier(stypes).dunedaq.daqconf.types;

local s = moo.oschema.schema("dunedaq.snbmodules.snbmodulesgen");
local nc = moo.oschema.numeric_constraints;

local cs = {
  snbmodules: s.record("snbmodules", [
    s.field( "host_interface", types.host, default="localhost", doc="Set the IP address interface used by the snbmodules to transfer data" ),
    s.field( "client_num", types.uint4, default=1, doc="Set how many Clients need to be started on the host" ),
    s.field( "client_name", types.string, default="client", doc="Set the base name of the client (number will be added with his number)" ),
    s.field( "client_starting_port", types.port, default=0, doc="Set the First port used by the SNBmodules Clients to transfer data" ),
    s.field( "clients_root_dir", types.path, default="./", doc="Set the root directory where clients files are saved" ),
    s.field( "have_bookkeeper", types.flag, default=false, doc="Set if the bookkeeper is started here or not" ),
    s.field( "bookkeeper_port", types.port, default=0, doc="Set the port used by the Bookkeeper to transfer data" ),
    s.field( "bookkeeper_refresh_rate", types.uint8, default=1, doc="Set the update rate of the Bookkeeper" ),
  ]),
};


stypes + moo.oschema.sort_select(cs)
