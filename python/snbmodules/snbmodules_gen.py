
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

# Time to wait on pop()
QUEUE_POP_WAIT_MS = 100

def get_snbmodules_app(
        snbmodules,
        DEBUG=False
    ):
    """
    Here the configuration for an entire daq_application instance using DAQModules from snbmodules is generated.
    """
    HOST=snbmodules.host_interface
    CLIENT_NUM=snbmodules.client_num
    CLIENT_NAME=snbmodules.client_name
    CLIENT_STARTING_PORT=snbmodules.client_starting_port
    CLIENTS_ROOT_DIR = snbmodules.clients_root_dir
    HAVE_BOOKKEEPER=snbmodules.have_bookkeeper
    BOOKKEEPER_PORT=snbmodules.bookkeeper_port
    BOOKKEEPER_REFRESH_RATE=snbmodules.bookkeeper_refresh_rate
    SNB_CONNECTION_PREFIX = "snbmodules"
    SNB_TIMEOUT_SEND = 10
    SNB_TIMEOUT_RECEIVE = 100
    BOOKKEEPER_NAME = "bookkeeper"
    BOOKKEEPER_LOG_PATH = "./"
    
    modules = []
    
    # Adding the modules
    for i in range(CLIENT_NUM):
        if CLIENT_STARTING_PORT == 0:
            client_ip = HOST
        else:
            client_ip = HOST + ":" + str(CLIENT_STARTING_PORT + i)
        
        modules += [DAQModule(name = HOST+CLIENT_NAME + str(i), # Adding host in the client name to avoid name clashes
                          plugin = "SNBFileTransfer", 
                            conf = snbfiletransfer.ConfParams(
                            client_ip = client_ip, 
                            work_dir = CLIENTS_ROOT_DIR + HOST + CLIENT_NAME + str(i) + "/", 
                            connection_prefix = SNB_CONNECTION_PREFIX, 
                            timeout_send = SNB_TIMEOUT_SEND, 
                            timeout_receive = SNB_TIMEOUT_RECEIVE, 
                            )
                )]

    if HAVE_BOOKKEEPER:
        
        if BOOKKEEPER_PORT == 0:
            bookkeeper_ip = HOST
        else:
            bookkeeper_ip = HOST + ":" + str(BOOKKEEPER_PORT)
        
        modules += [DAQModule(name = HOST+BOOKKEEPER_NAME, 
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
    
    for i in range(CLIENT_NUM):
        mgraph.add_endpoint(SNB_CONNECTION_PREFIX+"_" + HOST + CLIENT_NAME + str(i) + "_notifications", HOST + CLIENT_NAME + str(i) + ".notifications", "notification_t", Direction.IN, check_endpoints=False)
        
    if HAVE_BOOKKEEPER:
        mgraph.add_endpoint(SNB_CONNECTION_PREFIX+"_" + HOST + BOOKKEEPER_NAME+"_notifications", HOST + BOOKKEEPER_NAME + ".notifications", "notification_t", Direction.IN, check_endpoints=False)
        
    snbmodules_app = App(modulegraph = mgraph, host = HOST, name = "snbmodules")

    return snbmodules_app
