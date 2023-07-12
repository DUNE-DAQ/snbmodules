# This module facilitates the generation of snbmodules DAQModules within snbmodules apps


# Set moo schema search path                                                                              
from dunedaq.env import get_moo_model_path
import moo.io
moo.io.default_load_path = get_moo_model_path()

# Load configuration types                                                                                
import moo.otypes
moo.otypes.load_types("snbmodules/snbfiletransfer.jsonnet")

import dunedaq.snbmodules.snbfiletransfer as snbfiletransfer

from daqconf.core.app import App, ModuleGraph
from daqconf.core.daqmodule import DAQModule
#from daqconf.core.conf_utils import Endpoint, Direction

def get_snbmodules_app(nickname, num_snbfiletransfers, some_configured_value, host="localhost"):
    """
    Here the configuration for an entire daq_application instance using DAQModules from snbmodules is generated.
    """

    modules = []

    for i in range(num_snbfiletransfers):
        modules += [DAQModule(name = f"nickname{i}", 
                              plugin = "SNBFileTransfer", 
                              conf = snbfiletransfer.Conf(some_configured_value = some_configured_value
                                )
                    )]

    mgraph = ModuleGraph(modules)
    snbmodules_app = App(modulegraph = mgraph, host = host, name = nickname)

    return snbmodules_app
