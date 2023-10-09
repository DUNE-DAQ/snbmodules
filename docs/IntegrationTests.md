# Integration tests of SNBmodules

The integration tests of `snbmodules` are located in the integtest folder.

## Nomenclature

* Bookkeeper : Monitor file transfers status and registers/logs metadata about these transfers
* TransferClient : Client object that contains transfer implenetation (e.g.: torrent, rclone, etc.) sessions (upload, download)
* Transfer metadata : Contains informations about file transfers and their states
* Group metadata : Contains multiple Transfer metadata uploaded by the same transfer client

## snb_minimal_system_test.py

This very simple integration test contains 4 daq_applications (processes), each with a single module.

| daq_application(s) |          module(s)         |
|:------------------:|:--------------------------:|
|    snbbookkeeper   |  1 x SNBTransferBookkeeper |
|     snbclient0     |     1 x SNBFileTransfer    |
|     snbclient1     |     1 x SNBFileTransfer    |
|     snbclient2     |     1 x SNBFileTransfer    |

**Aim of the test**: Verify that the different SNB file transfer modules respect the state machine, the control interfaces are
working as expected and can be communicated with via the run control. It also ensures that the SNB plugin configuration generators are working 
as expected. This test doesn't test actual transfers! There are individual tests for testing transfer implementations of different protocols.

**Test steps**:
1. Generate default configs for `snbmodules`.
2. Modify `snbmodules` configurations.
3. Add `bookkeeper` to `snbmodules` configuration.
4. Add `client` apps, in this case three.
5. Populate nanorc command list: 

```
nanorc_command_list = "integtest-partition boot conf start 111 wait 1 enable_triggers ".split()
nanorc_command_list += ["wait"] + [str(run_duration)]
nanorc_command_list += "stop_run wait 2 scrap terminate".split()
```

**Pass criteria**:
1. test_nanorc_success: nanorc completed processes return code is 0
2. test_log_files: log files are error free
3. test_local_transfer_snbmodules: snb client process spawned and controlled successfully
4. test_bookkeeper_snbmodules: snb bookkeeper process spawned and controlled succesffully

## snb_1node_1app system quick tests

