import pytest
import urllib.request
import os
import copy

import conffwk
from daqconf.assets import resolve_asset_file
import integrationtest.data_file_checks as data_file_checks
import integrationtest.log_file_checks as log_file_checks
import integrationtest.basic_checks as basic_checks
import integrationtest.data_classes as data_classes
from integrationtest.verbosity_helper import IntegtestVerbosityLevels
from hdf5libs import HDF5RawDataFile

import functools
print = functools.partial(print, flush=True)  # always flush print() output

pytest_plugins = "integrationtest.integrationtest_drunc"

# Values that help determine the running conditions
run_duration = 30  # seconds

# Default values for validation parameters
expected_number_of_data_files = 1
check_for_logfile_errors = True
expected_event_count = 1
expected_event_count_tolerance = 0
tr_splitting_overhead = 0 # Bytes for WIBEth splitting (repeated frames)

wibeth_frag_params = {
    "fragment_type_description": "WIBEth",
    "fragment_type": "WIBEth",
    "expected_fragment_count": 1,
    "min_size_bytes": 0,
    "max_size_bytes": 0,
#    "status_bitmask": 0xFFFFFFFC,  # Mask kIncomplete
}
triggercandidate_frag_params = {
    "fragment_type_description": "Trigger Candidate",
    "fragment_type": "Trigger_Candidate",
    "expected_fragment_count": 1,
    "min_size_bytes": 72, # Empty TCs for sequences
    "max_size_bytes": 216,
}
hsi_frag_params = {
    "fragment_type_description": "HSI",
    "fragment_type": "Hardware_Signal",
    "expected_fragment_count": 0,
    "min_size_bytes": 72,
    "max_size_bytes": 100,
}
ignored_logfile_problems = {
    "-controller": [
        "Connection '.*' not found on the application registry",
    ],
    "connectivity-service": [
        "errorlog: -",
    ],
    "ru-det-conn": ["Request timed out for trig/seq_num"],
}

# The next three variable declarations *must* be present as globals in the test
# file. They're read by the "fixtures" in conftest.py to determine how
# to run the config generation and drunc

# The arguments to pass to the config generator, excluding the json
# output directory (the test framework handles that)

# CCM includes FSM, hosts; moduleconfs includes connections
object_databases = [
    "config/daqsystemtest/integrationtest-objects.data.xml",
    "config/snbmodules/simple-transform-test.data.xml",
]


dal = conffwk.dal.module("generated", "schema/appmodel/fdmodules.schema.xml")
db = conffwk.Configuration(
    "oksconflibs:config/snbmodules/simple-transform-test.data.xml"
)

found_file = True
ii = 0
file_size_map = {}
frame_file = ""
while found_file:
    try:
        file_conf = db.get_dal(
            class_name="SNBFileSourceParameters", uid=f"snb-files-det-conn-{ii}"
        )
        frame_file = file_conf.data_files[0]

        frame_file_name = frame_file
        if "asset:" in frame_file_name:
            frame_file_name = resolve_asset_file(frame_file)
        frame_file_size = os.path.getsize(frame_file_name)
        file_size_map[ii] = frame_file_size
        if (
            frame_file_size + 72 < wibeth_frag_params["min_size_bytes"]
            or wibeth_frag_params["min_size_bytes"] == 0
        ):
            wibeth_frag_params["min_size_bytes"] = frame_file_size + 72
        if frame_file_size + 72 > wibeth_frag_params["max_size_bytes"]:
            wibeth_frag_params["max_size_bytes"] = frame_file_size + 72
        ii = ii + 1
    except:
        found_file = False

# Add 10s per GB of input data
run_duration = 30 + wibeth_frag_params["max_size_bytes"] * 10 // (1024 * 1024 * 1024)

wibeth_frag_params["expected_fragment_count"] = ii

ft_conf = db.get_dal(class_name="FixedTimeTCMakerModuleConf", uid="ft-trig-conf")
expected_event_count = len(ft_conf.triggers)
sequence_count = 0
sequence_length = 500000 # intention is 8 msec sequences
for trig_n,trig in enumerate(ft_conf.triggers):
    trig_len = trig.timestamp_end - trig.timestamp_start
    sequence_count = sequence_count + (trig_len // sequence_length) + 1


conf_dict = data_classes.drunc_config()
conf_dict.dro_map_config = None
conf_dict.op_env = "integtest"
conf_dict.config_session_name = "snb-transform-simple"
conf_dict.tpg_enabled = False
conf_dict.frame_file = frame_file

conf_dict.config_substitutions.append(
    data_classes.attribute_substitution(
        obj_class="TRBConf",
        updates={
            "max_sequence_length_ticks": 0,  # Unlimited, no sequences
            "trigger_record_timeout_ms": 1000 * run_duration,
        },
    )
)

window_dict = copy.deepcopy(conf_dict)
window_dict.config_substitutions.append(
    data_classes.attribute_substitution(
        obj_class="TRBConf",
        updates={
            "max_sequence_length_ticks": sequence_length,
            "trigger_record_timeout_ms": 1000 * run_duration,
        },
    )
)
# For testing, allow drunc to manage ConnectivityService (default is False, integrationtest manages Connectivity Service)
# conf_dict.drunc_connsvc = True
# For testing, specify connectivity service port (default is 0, a random port is chosen for the Connectivity Service)
# conf_dict.connsvc_port = 12345

confgen_arguments = {
    "SNBTransformWithSequences": window_dict,
}

if wibeth_frag_params["max_size_bytes"] < 1024 * 1024 * 1024: # 1 GB
    confgen_arguments["SNBTransformSingle"] = conf_dict

# The commands to run in dunerc, as a list
dunerc_command_list = (
    "boot conf start --run-number 101 wait 1 enable-triggers wait ".split()
    + [str(run_duration)]
    + "disable-triggers wait 2 drain-dataflow wait 2 stop-trigger-sources stop scrap terminate".split()
)

# The tests themselves


def test_dunerc_success(run_dunerc, caplog):
    # check for run control success, problems during pytest setup, etc.
    basic_checks.basic_checks(run_dunerc, caplog, print_test_name=False)


def test_log_files(run_dunerc):

    # Check that at least some of the expected log files are present
    assert any(
        f"{run_dunerc.daq_session_name}_df-01" in str(logname)
        for logname in run_dunerc.log_files
    )
    assert any(
        f"{run_dunerc.daq_session_name}_dfo" in str(logname) for logname in run_dunerc.log_files
    )
    assert any(
        f"{run_dunerc.daq_session_name}_mlt" in str(logname) for logname in run_dunerc.log_files
    )
    assert any(
        f"{run_dunerc.daq_session_name}_ru" in str(logname) for logname in run_dunerc.log_files
    )

    if check_for_logfile_errors:
        # Check that there are no warnings or errors in the log files
        assert log_file_checks.logs_are_error_free(
            run_dunerc.log_files, True, True, ignored_logfile_problems,
            verbosity_helper=run_dunerc.verbosity_helper
        )


def test_data_files(run_dunerc):
    # Run some tests on the output data file
    current_test = os.environ.get("PYTEST_CURRENT_TEST")
    all_ok = True
    # Don't care how many files are written with sequences
    if "WithSequences" not in current_test:
        all_ok = len(run_dunerc.data_files) == expected_number_of_data_files
        #print("")  # Clear potential dot from pytest
        if all_ok:
            if run_dunerc.verbosity_helper.compare_level(IntegtestVerbosityLevels.drunc_transitions):
                print(
                    f"\N{WHITE HEAVY CHECK MARK} The correct number of raw data files was found ({expected_number_of_data_files})"
                )
        else:
            print(
                f"\N{POLICE CARS REVOLVING LIGHT} An incorrect number of raw data files was found, expected {expected_number_of_data_files}, found {len(run_dunerc.data_files)} \N{POLICE CARS REVOLVING LIGHT}"
            )

    local_expected_event_count = expected_event_count
    local_wibeth_frag_params = copy.deepcopy(wibeth_frag_params)
    
    if "WithSequences" in current_test:
        local_expected_event_count = sequence_count
        local_wibeth_frag_params["min_size_bytes"] = 73 # One byte of data must be present
        
    fragment_check_list = [triggercandidate_frag_params, hsi_frag_params]
    fragment_check_list.append(local_wibeth_frag_params)
    nontrig_fragment_check_list = [hsi_frag_params, local_wibeth_frag_params]

    total_record_count=0
    fragment_size_by_id={}

    for idx in range(len(run_dunerc.data_files)):
        data_file = data_file_checks.DataFile(run_dunerc.data_files[idx], run_dunerc.verbosity_helper)
        all_ok &= data_file_checks.sanity_check(data_file)
        all_ok &= data_file_checks.check_file_attributes(data_file)
        
        # We'll just check that the _total_ number is correct for sequences
        if "WithSequences" not in current_test:
            all_ok &= data_file_checks.check_event_count(
                data_file, local_expected_event_count, expected_event_count_tolerance
            )
        for jdx in range(len(fragment_check_list)):
            all_ok &= data_file_checks.check_fragment_count(
                data_file, fragment_check_list[jdx]
            )
            all_ok &= data_file_checks.check_fragment_sizes(
                data_file, fragment_check_list[jdx]
            )
        for kdx in range(len(nontrig_fragment_check_list)):
            all_ok &= data_file_checks.check_fragment_error_flags(
                data_file, nontrig_fragment_check_list[kdx]
            )
        if "WithSequences" in current_test:
            h5_file = HDF5RawDataFile(data_file.name)
            records = h5_file.get_all_record_ids()
            total_record_count += len(records)
            for rec in records:
                src_ids = h5_file.get_source_ids_for_fragment_type(rec, "WIBEth")
                for src_id in src_ids:
                    frag=h5_file.get_frag(rec,src_id);
                    size=frag.get_size()
                    if src_id.id in fragment_size_by_id.keys():
                        fragment_size_by_id[src_id.id] += size - 72
                    else:
                        fragment_size_by_id[src_id.id] = size - 72

    if "WithSequences" in current_test:
        correct_count = (total_record_count == local_expected_event_count)
        all_ok &= correct_count
        if correct_count:
            if run_dunerc.verbosity_helper.compare_level(IntegtestVerbosityLevels.drunc_transitions):
                print(f"\N{WHITE HEAVY CHECK MARK} Record count {total_record_count} matches expected count {local_expected_event_count}")
        else:
            print(f"\N{POLICE CARS REVOLVING LIGHT} Record count {total_record_count} DOES NOT match expected count {local_expected_event_count} \N{POLICE CARS REVOLVING LIGHT} ")
        correct_sizes = True
        for src_id,size in fragment_size_by_id.items():
            expected_size = file_size_map[src_id] + ((total_record_count - 1) * tr_splitting_overhead)
            if size != expected_size:
                print(f"\N{POLICE CARS REVOLVING LIGHT} Fragments with source ID {src_id} have total size {size}, expected {expected_size} \N{POLICE CARS REVOLVING LIGHT} ")
                correct_sizes = False
            else:
                if run_dunerc.verbosity_helper.compare_level(IntegtestVerbosityLevels.drunc_transitions):
                    print(f"\N{WHITE HEAVY CHECK MARK} Fragments with source ID {src_id} have total size {size}, expected {expected_size}")
            all_ok &= (size == expected_size)
        if correct_sizes:
            if run_dunerc.verbosity_helper.compare_level(IntegtestVerbosityLevels.drunc_transitions):
                print(f"\N{WHITE HEAVY CHECK MARK} All source IDs had total data size equal to expected")

    assert all_ok
