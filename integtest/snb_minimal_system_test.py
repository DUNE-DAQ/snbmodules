import pytest
import urllib.request
import json
from os.path import exists

 # Checks and tests functions
import integrationtest.log_file_checks as log_file_checks

# Values that help determine the running conditions
run_duration=5
snb_clients_number = 3 # number of clients
host="localhost"

check_for_logfile_errors=True
dro_map_required=False
ignored_logfile_problems={"connectionservice": ["Searching for connections matching uid_regex<errored_frames_q> and data_type Unknown"]}

# The next three variable declarations *must* be present as globals in the test
# file. They're read by the "fixtures" in conftest.py to determine how
# to run the config generation and nanorc

# The name of the python module for the config generation
confgen_name="snbmodules_gen"
# The arguments to pass to the config generator, excluding the json
# output directory (the test framework handles that)


# default config
conf_dict = {}
conf_dict["detector"] = {}
conf_dict["boot"] = {}

# SNBmodules config
conf_dict["snbmodules"] = {}

conf_dict["snbmodules"]["snb_connections_prefix"] = "snbmodules"
conf_dict["snbmodules"]["snb_timeout_notification_send"] = 10
conf_dict["snbmodules"]["snb_timeout_notification_receive"] = 100

# Adding bookkeeper
conf_dict["snbmodules"]["have_bookkeeper"] = True
conf_dict["snbmodules"]["host_bookkeeper"] = host
conf_dict["snbmodules"]["bookkeeper_name"] = "snbbookkeeper"
conf_dict["snbmodules"]["bookkeeper_log_path"] = "./"
conf_dict["snbmodules"]["bookkeeper_port"] = 5000
conf_dict["snbmodules"]["bookkeeper_refresh_rate"] = 1

# Adding clients app, one per host (can contain multiple clients per host)
conf_dict["snbmodules"]["apps"] = []

for i in range(snb_clients_number):
    client_conf = {}
    client_conf["host"] = host
    client_conf["interface"] = "0.0.0.0"
    client_conf["client_name"] = f"snbclient{i}"
    client_conf["client_num"] = 1
    client_conf["client_starting_port"] = 5001 + i
    client_conf["clients_root_dir"] = "./"
    conf_dict["snbmodules"]["apps"].append(client_conf)

confgen_arguments={"MinimalSystem": conf_dict}

# The commands to run in nanorc, as a list
nanorc_command_list = "integtest-partition boot conf start 111 wait 1 enable_triggers ".split() 
nanorc_command_list += ["wait"] + [str(run_duration)]
nanorc_command_list += "stop_run wait 2 scrap terminate".split()

# The tests themselves
def test_nanorc_success(run_nanorc):
    print(run_nanorc.json_dir)
    print(run_nanorc.log_files)
    print(run_nanorc.run_dir)
    # Check that nanorc completed correctly
    assert run_nanorc.completed_process.returncode==0

def test_log_files(run_nanorc):
    if check_for_logfile_errors:
        # Check that there are no warnings or errors in the log files
        assert log_file_checks.logs_are_error_free(run_nanorc.log_files, True, True, ignored_logfile_problems)

def test_local_transfer_snbmodules(run_nanorc):
    # Check that the transfer was successful
    for i in range(snb_clients_number):
        assert exists(run_nanorc.run_dir / f"{host}snbclient{i}0")

def test_bookkeeper_snbmodules(run_nanorc):
    assert exists(run_nanorc.run_dir / f"{host}{conf_dict['snbmodules']['bookkeeper_name']}.log")
    