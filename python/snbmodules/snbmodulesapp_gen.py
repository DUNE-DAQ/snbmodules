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

def get_snbmodules_app(nickname, port=0, host="localhost"):
    """
    Here the configuration for an entire daq_application instance using DAQModules from snbmodules is generated.
    """
    
    SNB_WORK_DIR = "./"
    SNB_CONNECTION_PREFIX = "snbmodules"
    SNB_TIMEOUT_SEND = 10
    SNB_TIMEOUT_RECEIVE = 100
  
    modules = []

    modules += [DAQModule(name = f"nickname", 
                          plugin = "SNBFileTransfer", 
                          conf = snbfiletransfer.ConfParams(
                            client_ip = host + ":" + str(port), 
                            work_dir = SNB_WORK_DIR, 
                            connection_prefix = SNB_CONNECTION_PREFIX, 
                            timeout_send = SNB_TIMEOUT_SEND, 
                            timeout_receive = SNB_TIMEOUT_RECEIVE, 
                            )
                )]

    mgraph = ModuleGraph(modules)
    snbmodules_app = App(modulegraph = mgraph, host = host, name = nickname)

    return snbmodules_app
