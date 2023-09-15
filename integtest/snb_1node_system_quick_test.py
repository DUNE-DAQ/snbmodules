import pytest
import urllib.request
import json
from os.path import exists

 # Checks and tests functions
import integrationtest.data_file_checks as data_file_checks
import integrationtest.log_file_checks as log_file_checks
import integrationtest.config_file_gen as config_file_gen
import integrationtest.dro_map_gen as dro_map_gen
import raw_file_check as raw_file_check
import transfer_check as transfer_check

# Values that help determine the running conditions
number_of_data_producers=2
data_rate_slowdown_factor=10 # 10 for ProtoWIB/DuneWIB
run_duration=5
send_duration=5
record_duration=5
readout_window_time_before=1000
readout_window_time_after=1001

# Default values for validation parameters
expected_number_of_data_files=1
check_for_logfile_errors=True
expected_event_count=run_duration+send_duration+record_duration
expected_event_count_tolerance=2
wib1_frag_hsi_trig_params={"fragment_type_description": "WIB",
                           "fragment_type": "ProtoWIB",
                           "hdf5_source_subsystem": "Detector_Readout",
                           "expected_fragment_count": number_of_data_producers,
                           "min_size_bytes": 37656, "max_size_bytes": 37656}
wib2_frag_params={"fragment_type_description": "WIB2",
                  "fragment_type": "WIB",
                  "hdf5_source_subsystem": "Detector_Readout",
                  "expected_fragment_count": number_of_data_producers,
                  "min_size_bytes": 29808, "max_size_bytes": 30280}
wibeth_frag_params={"fragment_type_description": "WIBEth",
                  "fragment_type": "WIBEth",
                  "hdf5_source_subsystem": "Detector_Readout",
                  "expected_fragment_count": number_of_data_producers,
                  "min_size_bytes": 7272, "max_size_bytes": 14472}
triggercandidate_frag_params={"fragment_type_description": "Trigger Candidate",
                              "fragment_type": "Trigger_Candidate",
                              "hdf5_source_subsystem": "Trigger",
                              "expected_fragment_count": 1,
                              "min_size_bytes": 72, "max_size_bytes": 216}
hsi_frag_params ={"fragment_type_description": "HSI",
                             "fragment_type": "Hardware_Signal",
                             "hdf5_source_subsystem": "HW_Signals_Interface",
                             "expected_fragment_count": 1,
                             "min_size_bytes": 72, "max_size_bytes": 100}
ignored_logfile_problems={"connectionservice": ["Searching for connections matching uid_regex<errored_frames_q> and data_type Unknown"]}

# The next three variable declarations *must* be present as globals in the test
# file. They're read by the "fixtures" in conftest.py to determine how
# to run the config generation and nanorc

# The name of the python module for the config generation
confgen_name="daqconf_multiru_gen"
# The arguments to pass to the config generator, excluding the json
# output directory (the test framework handles that)


# default config
conf_dict = {}
conf_dict["detector"] = {}
conf_dict["daq_common"] = {}
conf_dict["boot"] = {}
conf_dict["hsi"] = {}
conf_dict["timing"] = {}
conf_dict["trigger"] = {}
conf_dict["dataflow"] = {
    "apps": [{
        "app_name": "dataflow0"
    }]
}
conf_dict["dqm"] = {}
conf_dict["detector"]["op_env"] = "integtest"
conf_dict["detector"]["clock_speed_hz"] = 62500000 # DuneWIB/WIBEth
conf_dict["daq_common"]["data_rate_slowdown_factor"] = data_rate_slowdown_factor
conf_dict["trigger"]["trigger_window_before_ticks"] = readout_window_time_before
conf_dict["trigger"]["trigger_window_after_ticks"] = readout_window_time_after

# Readout config
dro_map_contents = dro_map_gen.generate_dromap_contents(n_streams=number_of_data_producers, n_apps=1, det_id = 3) 
conf_dict["readout"] = {}
conf_dict["readout"]["dro_map"] = dro_map_contents
conf_dict["readout"]["use_fake_cards"] = True
conf_dict["readout"]["default_data_file"] = "asset://?checksum=e96fd6efd3f98a9a3bfaba32975b476e"
conf_dict["readout"]["enable_raw_recording"] = True # readout raw recording enabled
# conf_dict["readout"]["raw_recording_output_dir"] = "."
interface_name = "localhosteth0" # interface name, need to be changed for different machine testing

# SNBmodules config
# for now, majority of config is by default in daqconf generator script
conf_dict["snbmodules"] = {}
conf_dict["snbmodules"]["port"] = 5009
conf_dict["snbmodules"]["host"] = "epdtdi105"
conf_dict["snbmodules"]["client_name"] = "client"

# create text file containing simple text
# with open("test.txt", "w") as f:
#     f.write("Hello World")


confgen_arguments={"MinimalSystem": conf_dict}

# Modify new transfer expert command to transfer every raw data files
with open('new-RClone-transfer.json', 'r+') as f:
    data = json.load(f)
    
    file_names = []
    for i in range(number_of_data_producers):
        file_names.append("./output_" + interface_name + "_" + str(i) + ".out")
        
    data['data']['modules'][0]['data']['files'] = file_names # <--- add `id` value.
    f.seek(0)        # <--- should reset file position to the beginning.
    json.dump(data, f, indent=4)
    f.truncate()     # remove remaining part
    
# The commands to run in nanorc, as a list
nanorc_command_list="integtest-partition boot conf start 111 wait 1 enable_triggers wait ".split() + [str(run_duration)] + \
("expert_command /json0/json0/ru" + interface_name + " /home/ljoly/NFD23-08-23/sourcecode/snbmodules/integtest/record-cmd.json ").split() + \
["wait"] + [str(record_duration)] + \
"expert_command /json0/json0/snbmodules /home/ljoly/NFD23-08-23/sourcecode/snbmodules/integtest/new-RClone-transfer.json ".split() + \
"expert_command /json0/json0/snbmodules /home/ljoly/NFD23-08-23/sourcecode/snbmodules/integtest/start-transfer.json ".split() + \
["wait"] + [str(send_duration)] + "stop_run wait 2 scrap terminate".split()

# The tests themselves
def test_nanorc_success(run_nanorc):
    print(run_nanorc.json_dir)
    print(run_nanorc.log_files)
    # Check that nanorc completed correctly
    assert run_nanorc.completed_process.returncode==0

def test_log_files(run_nanorc):
    if check_for_logfile_errors:
        # Check that there are no warnings or errors in the log files
        assert log_file_checks.logs_are_error_free(run_nanorc.log_files, True, True, ignored_logfile_problems)

def test_data_files(run_nanorc):
    # Run some tests on the output data file
    assert len(run_nanorc.data_files)==expected_number_of_data_files

    fragment_check_list=[triggercandidate_frag_params, hsi_frag_params]
    #fragment_check_list.append(wib1_frag_hsi_trig_params) # ProtoWIB
    #fragment_check_list.append(wib2_frag_params) # DuneWIB
    fragment_check_list.append(wibeth_frag_params) # WIBEth

    for idx in range(len(run_nanorc.data_files)):
        data_file=data_file_checks.DataFile(run_nanorc.data_files[idx])
        assert data_file_checks.sanity_check(data_file)
        assert data_file_checks.check_file_attributes(data_file)
        assert data_file_checks.check_event_count(data_file, expected_event_count, expected_event_count_tolerance)
        for jdx in range(len(fragment_check_list)):
            assert data_file_checks.check_fragment_count(data_file, fragment_check_list[jdx])
            assert data_file_checks.check_fragment_sizes(data_file, fragment_check_list[jdx])
            
def test_local_transfer_snbmodules(run_nanorc):
    # Check that the transfer was successful
    print(run_nanorc.run_dir)
    assert exists(run_nanorc.run_dir / "client0")
    assert exists(run_nanorc.run_dir / "client1")
    
    for i in range(number_of_data_producers):
        file_name = "output_" + interface_name + "_" + str(i) + ".out"
        assert exists(run_nanorc.run_dir / "client1/transfer0/" / file_name)
    
        # Compare file size and content
        assert raw_file_check.compare_raw_size(run_nanorc.run_dir / file_name, run_nanorc.run_dir / "client1/transfer0/" / file_name)
        assert raw_file_check.compare_raw_content(run_nanorc.run_dir / file_name, run_nanorc.run_dir / "client1/transfer0/" / file_name)
        
def test_bookkeeper_snbmodules(run_nanorc):
    assert exists(run_nanorc.run_dir / "bookkeeper.log")
    
    assert transfer_check.check_contain_no_errors(run_nanorc.run_dir / "bookkeeper.log")
    for i in range(number_of_data_producers):
        file_name = "output_" + interface_name + "_" + str(i) + ".out"
        assert transfer_check.check_transfer_finished(run_nanorc.run_dir / "bookkeeper.log", file_name)