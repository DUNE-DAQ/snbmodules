# SNB-Modules

This library uses external libraries such as Bittorent or Rclone that need to be intalled.

1. Clone the this repo into sourcecode and add the library name to the dbt-build-order.cmake

2. Source the environment and install external libraries

3. Add to env.sh file the export of the libraries and source again

4. Configure the test environment

5. Build and enjoy~

## TODO

- Have default values for parameters
- See important parameters for RClone
- Script for json generation
- Auto start RClone HTTP server on source client (Uploader)
    - For now, we can start the server with rclone serve, see [Install local server section](#run-local-server)

## Install RClone
### Install GO
```
sudo dnf install golang
```
### Clone RClone lib
```
git clone https://github.com/rclone/rclone.git
```
### Build lib in rclone
Better to Build with the evironment already sourced.
```
go build --buildmode=c-shared -o librclone.so github.com/rclone/rclone/librclone
```
### Export var (add to the env.sh file)
```
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/path/to/lib/folder
export RCLIB_INC=/path/to/lib/folder
export RCLIB_LIB=/path/to/lib/folder
```
### Run local server
```
cd rclone/
curl -O https://downloads.rclone.org/rclone-current-linux-amd64.zip
unzip rclone-current-linux-amd64.zip
rm rclone-current-linux-amd64.zip
cd rclone-current-linux-amd64
./rclone serve http / --addr IP:PORT(8080) --buffer-size '0' --no-modtime --transfers 200 -v --multi-thread-cutoff=50G --multi-thread-streams=16
```

## Install libtorrent
### Clone lib
```
git clone --recurse-submodules https://github.com/arvidn/libtorrent.git
```
### Build lib in rclone
```
mkdir build;cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=17 -G Ninja ..
ninja
```
### Export var (add to the env.sh file)
```
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/path/to/lib/folder/build
export LIBTORRENT_INC=/path/to/lib/folder/include
export LIBTORRENT_LIB=/path/to/lib/folder/build
```

## Configuration files
### boot.json
Add the apps/package in the booting configuration. One should exist for every hoststarting a client or a bookkeeper.

<details>
  <summary>Show example Code</summary>

```
"apps": {
        "snbmodules0": {
            "exec": "daq_application_ssh",
            "host": "snbmodules0",
            "port": 3333
        },
        "snbmodules1": {
            "exec": "daq_application_ssh",
            "host": "snbmodules1",
            "port": 3334
        }
    },
    ...
    "hosts-ctrl": {
        "snbmodules0": "hostname",
        "snbmodules1": "hostname"
    },
    "hosts-data": {
        "snbmodules0": "hostname",
        "snbmodules1": "hostname"
    },
```

</details>

### config file

#### Bookkeeper params
- "bookkeeper_ip" : string (mandatory) The IP address used by the Bookkeeper to receive/send notifications
- "bookkeeper_log_path" : string (default:"./") Path where record transfer data in csv file and graphical bookkeeper.log interface for monitoring. leave empty "" to disable
- "refresh_rate" : int (default:5) Refresh rate of bookkeeper logs in seconds

#### Client params
- "client_ip" : string format IPV4:PORT (mandatory) The IP address used by the client, you can precise the interface used here
- "work_dir" : string (default:"") Directory where the client is gonna watch for files to share with Bookkeeper and where files are Downloaded by default (uploaded files don't have to be in here)

#### Global params
- "connection_prefix" : string (default:"snbmodules") prefix of the connections name, for the plugin to find others connections
- "timeout_send" : int (default:10) max time in ms passed sending a notification
- "timeout_receive" : int (default:100) max time in ms listening for a notification

<details>
  <summary>Show example Code</summary>

```
{
    "modules": [
        {
            "data": {
                "bookkeeper_ip": "192.168.0.106:0",
                "bookkeeper_log_path": "./",
                "connection_prefix": "snbmodules",
                "timeout_send": 100,
                "timeout_receive": 100,
                "refresh_rate": 5
            },
            "match": "bookkeeper"
        },
        {
            "data": {
                "client_ip": "192.168.0.106:5010",
                "work_dir": "/mnt/md1/client0",
                "connection_prefix": "snbmodules",
                "timeout_send": 100,
                "timeout_receive": 100
            },
            "match": "client0"
        },
        {
            "data": {
                "client_ip": "192.168.0.105:5011",
                "work_dir": "/mnt/md1/client1",
                "connection_prefix": "snbmodules",
                "timeout_send": 100,
                "timeout_receive": 100
            },
            "match": "client1"
        }
    ]
}
```

</details>

### init file

Connection names must start with 'snbmodules' or  then client or bookkeeper :
- snbmodules_bookkeeper_notification_0x00000000
- snbmodules_client0_notification_0x00000000

<details>
  <summary>Show example Code</summary>

```
{
    "connections": [
        {
            "connection_type": "kSendRecv",
            "id": {
                "data_type": "notification_t",
                "session": "",
                "uid": "snbmodules_bookkeeper_notification_0x00000000"
            },
            "uri": "tcp://{snbmodules0}:12649"
        },
        {
            "connection_type": "kSendRecv",
            "id": {
                "data_type": "notification_t",
                "session": "",
                "uid": "snbmodules_client0_notification_0x00000000"
            },
            "uri": "tcp://{snbmodules0}:12648"
        },
        {
            "connection_type": "kSendRecv",
            "id": {
                "data_type": "notification_t",
                "session": "",
                "uid": "snbmodules_client1_notification_0x00000001"
            },
            "uri": "tcp://{snbmodules0}:12647"
        },
        {
            "connection_type": "kSendRecv",
            "id": {
                "data_type": "notification_t",
                "session": "",
                "uid": "snbmodules_client2_notification_0x00000002"
            },
            "uri": "tcp://{snbmodules1}:12646"
        }
    ],
    "connectivity_service_interval_ms": 1000,
    "modules": [
        {
            "inst": "bookkeeper",
            "plugin": "SNBTransferBookkeeper"
        },
        {
            "inst": "client0",
            "plugin": "SNBFileTransfer"
        },
        {
            "inst": "client1",
            "plugin": "SNBFileTransfer"
        }
    ],
    "queues": [],
    "use_connectivity_service": false
}
```

</details>

## Custom commands
### Bookkeeper

- Collect data from clients

<details>
  <summary>Show example Code</summary>

```
{
    "_comment_post_command": "curl --header 'Content-Type: application/json' --header 'X-Answer-Port: 56789' --request POST --data @test1/commands/bk-info.json http://localhost:3333/command",
    "data": {
        "modules": [
            {
                "data": {},
                "match": ""
            }
        ]
    },
    "id": "info"
}
```

</details>

### Client

- New group transfer : only for the Uploader Client of the transfer
    - Only one source (Uploder) to multiple clients (Dowloaders) per group transfer
    - Parameters : 
        - "transfer_id" : string (mandatory) Name of the transfer
        - "files" : array<string> (mandatory) List of files full path to send
        - "src" : string (mandatory) Name of the source (Uploader) Client
        - "dests" : array<string> (mandatory) Names of the destinations Clients (Downloaders)
        - "protocol" : enum (mandatory) Choice of the protocol used for the transfer
            - SCP
            - BITTORRENT
            - RCLONE
        - "protocol_args" : json (optional/mandatory) Json of parameters for the protocol, they change depending of the protocol
            - SCP params
                - "user" : String (mandatory) Name of the username to use for the transfer
                - "use_password" : bool (default:false) Request password to the user (only for stand alone application)
            - BITTORRENT params
                - "port": int (mandatory) Listening port of the bittorent client
                - "rate_limit": int (default:-1) rate limit of the transfer in bytes/second, -1 for unlimited 
            - RCLONE params
                - "protocol": string (default:"http") RClone param to select protocol used, supported : "http", "sftp"
                - "user": string (mandatory for sftp only) username if using sftp
                - "rate_limit": string (default:"off") Rate limiter for the transfer, for 1 GiB put "1GiB" and "off" for unlimited
                - "port": int (default:8080) Port of the HTTP server in the source Client (Uploader) if using http
                - "refresh_rate": int (default:5) Seconds before RClone request transfer informations of transfering files
                - "simult_transfers": int (default:200) Number of allowed concurrent connection for a transfer
                - "transfer_threads": int (default:1) Number of threads that will write the file per file transfered
                - "checkers_threads": int (default:2) Number of threads that will Hash and check the file per file transfered
                - "chunk_size": string (default:"8GiB") Chunk to split each file, this will create a new connection for each chunk
                - "buffer_size": string  (default:"0") Buffer size allocated, for 1 GiB put "1GiB"
                - "use_mmap": bool (default:false) Use memory map
                - "checksum": bool (default:true) Check sum of the file once downloaded (or on flight)
    - "match": string (mandatory) The match must be equal to src parameter.

        
        - "rate_limit" : Integer, .

<details>
  <summary>Show example Code</summary>

```
{
    "_comment_post_command": "curl --header 'Content-Type: application/json' --header 'X-Answer-Port: 56789' --request POST --data @test1/commands/new-RClone-transfer.json http://localhost:3333/command",
    "data": {
        "modules": [
            {
                "data": {
                    "transfer_id": "transfer2",
                    "files": [
                        "/mnt/md1/small_32_block_0",
                        "/mnt/md1/small_32_block_1",
                        "/mnt/md1/small_32_block_2",
                        "/mnt/md1/small_32_block_3",
                        "/mnt/md1/small_32_block_4",
                        "/mnt/md1/small_32_block_5",
                        "/mnt/md1/small_32_block_6",
                        "/mnt/md1/small_32_block_7",
                        "/mnt/md1/small_32_block_8",
                        "/mnt/md1/small_32_block_9"
                    ],
                    "src": "client0",
                    "dests": [
                        "client2"
                    ],
                    "protocol": "RCLONE",
                    "protocol_args": {
                        "protocol": "http",
                        "rate_limit": "off",
                        "port": 8080,
                        "refresh_rate": 5,
                        "user": "username",
                        "simult_transfers": 200,
                        "transfer_threads": 1,
                        "checkers_threads": 2,
                        "chunk_size": "8GiB",
                        "buffer_size": "0",
                        "use_mmap": false,
                        "checksum": true
                    }
                },
                "match": "client0"
            }
        ]
    },
    "id": "new-transfer"
}
```

</details>

- Start transfer

Simply indicate the name of the transfer to apply the command to with "transfer_id" parameter and the Uploader client in the "match" option.

<details>
  <summary>Show example Code</summary>

```
{
    "_comment_post_command": "curl --header 'Content-Type: application/json' --header 'X-Answer-Port: 56789' --request POST --data @test1/commands/start-transfer.json http://localhost:3333/command",
    "data": {
        "modules": [
            {
                "data": {
                    "transfer_id": "transfer0"
                },
                "match": "client0"
            }
        ]
    },
    "id": "start-transfer"
}
```

</details>

- Pause transfer

<details>
  <summary>Show example Code</summary>

```
{
    "_comment_post_command": "curl --header 'Content-Type: application/json' --header 'X-Answer-Port: 56789' --request POST --data @test1/commands/pause-transfer.json http://localhost:3333/command",
    "data": {
        "modules": [
            {
                "data": {
                    "transfer_id": "transfer0"
                },
                "match": "client0"
            }
        ]
    },
    "id": "pause-transfer"
}
```

</details>

- Resume transfer

<details>
  <summary>Show example Code</summary>

```
{
    "_comment_post_command": "curl --header 'Content-Type: application/json' --header 'X-Answer-Port: 56789' --request POST --data @test1/commands/resume-transfer.json http://localhost:3333/command",
    "data": {
        "modules": [
            {
                "data": {
                    "transfer_id": "transfer0"
                },
                "match": "client0"
            }
        ]
    },
    "id": "resume-transfer"
}
```

</details>

- Cancel transfer

<details>
  <summary>Show example Code</summary>

```
{
    "_comment_post_command": "curl --header 'Content-Type: application/json' --header 'X-Answer-Port: 56789' --request POST --data @test1/commands/cancel-transfer.json http://localhost:3333/command",
    "data": {
        "modules": [
            {
                "data": {
                    "transfer_id": "transfer0"
                },
                "match": "client0"
            }
        ]
    },
    "id": "cancel-transfer"
}
```

</details>