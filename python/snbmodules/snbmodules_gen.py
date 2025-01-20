
# Set moo schema search path
from dunedaq.env import get_moo_model_path
import moo.io
moo.io.default_load_path = get_moo_model_path()

# Load configuration types
import moo.otypes
moo.otypes.load_types('rcif/cmd.jsonnet')
moo.otypes.load_types('appfwk/cmd.jsonnet')
moo.otypes.load_types('appfwk/app.jsonnet')

# Import new types
moo.otypes.load_types('snbmodules/snbfiletransfer.jsonnet')
import dunedaq.snbmodules.snbfiletransfer as snbfiletransfer
moo.otypes.load_types('snbmodules/snbtransferbookkeeper.jsonnet')
import dunedaq.snbmodules.snbtransferbookkeeper as snbtransferbookkeeper

from daqconf.core.app import App, ModuleGraph
from daqconf.core.daqmodule import DAQModule
from daqconf.core.conf_utils import Direction

import socket

# Time to wait on pop()
QUEUE_POP_WAIT_MS = 100

def get_snbmodules_client_app(
        snbmodules,
        SNB_CONNECTION_PREFIX="snbmodules",
        SNB_TIMEOUT_SEND=10,
        SNB_TIMEOUT_RECEIVE=100,
        DEBUG=False
    ):
    """
    Here the configuration for an entire daq_application instance using DAQModules from snbmodules is generated.
    """
    HOST=snbmodules.host
    HOST2 = HOST.replace("-", "")
    INTERFACE=snbmodules.interface
    CLIENT_NUM=snbmodules.client_num
    CLIENT_NAME=snbmodules.client_name
    CLIENT_STARTING_PORT=snbmodules.client_starting_port
    CLIENTS_ROOT_DIR = snbmodules.clients_root_dir
    
    modules = []
    
    # Adding the modules
    for i in range(CLIENT_NUM):
        RESOLVED_INTERFACE = INTERFACE
        if RESOLVED_INTERFACE == "0.0.0.0":
            RESOLVED_INTERFACE = socket.gethostbyname(HOST)

        if CLIENT_STARTING_PORT == 0:
            client_ip = RESOLVED_INTERFACE
        else:
            client_ip = RESOLVED_INTERFACE + ":" + str(CLIENT_STARTING_PORT + i)

        modules += [DAQModule(name = HOST2+CLIENT_NAME + str(i), # Adding host in the client name to avoid name clashes
                          plugin = "SNBFileTransfer", 
                            conf = snbfiletransfer.ConfParams(
                            client_ip = client_ip, 
                            work_dir = CLIENTS_ROOT_DIR + HOST2 + CLIENT_NAME + str(i) + "/", 
                            connection_prefix = SNB_CONNECTION_PREFIX, 
                            timeout_send = SNB_TIMEOUT_SEND, 
                            timeout_receive = SNB_TIMEOUT_RECEIVE, 
                            )
                )]
        
    # Adding the endpoints
    mgraph = ModuleGraph(modules)

    for i in range(CLIENT_NUM):
        mgraph.add_endpoint(SNB_CONNECTION_PREFIX+"_client_" + HOST2 + CLIENT_NAME + str(i) + "_notifications", HOST2 + CLIENT_NAME + str(i) + ".notifications", "notification_t", Direction.IN, check_endpoints=False)
        
    snbmodules_app = App(modulegraph = mgraph, host = HOST, name = HOST+CLIENT_NAME)

    return snbmodules_app

def get_snbmodules_bookkeeper_app(
        HOST,
        BOOKKEEPER_PORT=0,
        BOOKKEEPER_REFRESH_RATE=1,
        BOOKKEEPER_NAME="bookkeeper",
        BOOKKEEPER_LOG_PATH="./",
        SNB_CONNECTION_PREFIX="snbmodules",
        SNB_TIMEOUT_SEND=10,
        SNB_TIMEOUT_RECEIVE=100,
        DEBUG=False
    ):
    """
    Here the configuration for an entire daq_application instance using DAQModules from snbmodules is generated.
    """

    modules = []

    if BOOKKEEPER_PORT == 0:
        bookkeeper_ip = socket.gethostbyname(HOST)
    else:
        bookkeeper_ip = socket.gethostbyname(HOST) + ":" + str(BOOKKEEPER_PORT)

    HOST2 = HOST.replace("-", "")
    modules += [DAQModule(name = HOST2+BOOKKEEPER_NAME, 
                            plugin = "SNBTransferBookkeeper", 
                            conf = snbtransferbookkeeper.ConfParams(
                            bookkeeper_ip = bookkeeper_ip, 
                            bookkeeper_log_path = BOOKKEEPER_LOG_PATH, 
                            refresh_rate = BOOKKEEPER_REFRESH_RATE,
                            connection_prefix = SNB_CONNECTION_PREFIX, 
                            timeout_send = SNB_TIMEOUT_SEND, 
                            timeout_receive = SNB_TIMEOUT_RECEIVE, 
                            )
                )]
        
    # Adding the endpoints
    mgraph = ModuleGraph(modules)
    
    mgraph.add_endpoint(SNB_CONNECTION_PREFIX + "_bookkeeper_" + HOST2 + BOOKKEEPER_NAME+"_notifications", HOST2 + BOOKKEEPER_NAME + ".notifications", "notification_t", Direction.IN, check_endpoints=False)
        
    snbmodules_app = App(modulegraph = mgraph, host = HOST, name = HOST+BOOKKEEPER_NAME)

    return snbmodules_app
