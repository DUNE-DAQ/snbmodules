#!/bin/bash

echo "Welcome to the command generator"
echo "This script will help you to generate and send commands to snbmodules"
echo "Please refer to the wiki page for more information about options https://github.com/DUNE-DAQ/snbmodules/wiki/Transfers"
echo ""

read -p "Enter destination application ip:port (ex:'localhost:3333'): " dest_port
read -p "Enter answer port (ex:'56789'): " answer_port
echo ""

while [ true ]
do

    echo "Choose what you want to do: "3
    echo "0. Run previously generated command"
    echo "1. Create a new transfer"
    echo "2. Start a transfer (created before)"
    echo "3. Pause a transfer"
    echo "4. Resume a transfer"
    echo "5. Cancel a transfer"
    echo "6. Force update Bookkeeper"
    echo "7. Exit"
    echo ""

    read -p "Enter your choice: " choice
    echo ""

    command="{\n\t\"data\": {\n\t\t\"modules\": [\n\t\t\t{\n\t\t\t\t\"data\": {\n"

    case $choice in
        0)
            ls *.json
            echo ""
            read -p "Enter the command file name to execute: " file_name

            echo ""
            echo ""
            echo "Sending command to ${dest_port}..."
            echo "curl --header 'Content-Type: application/json' --header 'X-Answer-Port: ${answer_port}' --request POST --data @${file_name} http://${dest_port}/command"
            curl --header 'Content-Type: application/json' --header 'X-Answer-Port: ${answer_port}' --request POST --data @${file_name} http://${dest_port}/command
            echo ""
            continue
            ;;
        1) 
            read -p "Enter the transfer id (ex:'transfer0'): " transfer_id
            read -p "Enter the destination client id (ex:'client0'): " client
            command_id="new_transfer"
            command="${command}\t\t\t\t\t\"transfer_id\": \"${transfer_id}\",\n"

            # Add files
            command="${command}\t\t\t\t\t\"files\": [\n"
            while [ true ]
            do
                read -p "Enter the file path to transfer (ex:'/home/ljoly/N23-07-15/Makefile'): " file_path
                command="${command}\t\t\t\t\t\t\"${file_path}\""
                read -p "Do you want to add another file? (y/n): " add_file
                if [ $add_file == "n" ]
                then
                    command="${command}\n"
                    break
                else
                    command="${command},\n"
                fi
            done
            command="${command}\t\t\t\t\t],\n"

            # Add src
            command="${command}\t\t\t\t\t\"src\": \"${client}\",\n"

            # Add destination clients
            command="${command}\t\t\t\t\t\"dests\": [\n"
            while [ true ]
            do
                read -p "Enter the destination client id (ex:'client0'): " dest_client
                command="${command}\t\t\t\t\t\t\"${dest_client}\""
                read -p "Do you want to add another destination client ? (y/n): " add_client
                if [ $add_client == "n" ]
                then
                    command="${command}\n"
                    break
                else
                    command="${command},\n"
                fi
            done
            command="${command}\t\t\t\t\t],\n"

            # Add protocol
            read -p "Enter the protocol ('dummy' | 'SCP' | 'RCLONE' | 'BITTORRENT'): " protocol
            command="${command}\t\t\t\t\t\"protocol\": \"${protocol}\",\n"

            # Add protocol args
            command="${command}\t\t\t\t\t\"protocol_args\": {\n"
            case "$protocol" in
                "dummy")
                    ;;
                "SCP")
                    read -p "Enter User to use (ex:'ljoky'): " user
                    read -p "Do you want to use password identification (ex:'true/false'): " use_password
                    command="${command}\t\t\t\t\t\t\"user\": \"${user}\",\n"
                    command="${command}\t\t\t\t\t\t\"use_password\": ${use_password}\n"
                    ;;
                "RCLONE")

                    read -p "Enter protocol to use (ex:'http'): " rclone_protocol
                    read -p "Enter Rate limit (ex:'off'): " rate_limit
                    read -p "Enter http server Port to use (ex:'8080'): " port
                    read -p "Enter Refresh rate (ex:'5'): " refresh_rate
                    if [ $rclone_protocol == "sftp" ]
                    then
                        read -p "Enter User to use (ex:'ljoky'): " user
                        command="${command}\t\t\t\t\t\t\"user\": \"${user}\",\n"
                    fi
                    read -p "Enter simultaneous transfers (ex:'200'): " simult_transfers
                    read -p "Enter transfer threads (ex:'1'): " transfer_threads
                    read -p "Enter checkers threads (ex:'2'): " checkers_threads
                    read -p "Enter chunk size (ex:'8GiB'): " chunk_size
                    read -p "Enter buffer size (ex:'0'): " buffer_size
                    read -p "Enter use mmap (ex:'false'): " use_mmap
                    read -p "Enter checksum (ex:'true'): " checksum
                    read -p "Enter root folder of http sharing (ex:'/'): " root_folder

                    command="${command}\t\t\t\t\t\t\"protocol\": \"${rclone_protocol}\",\n"
                    command="${command}\t\t\t\t\t\t\"rate_limit\": \"${rate_limit}\",\n"
                    command="${command}\t\t\t\t\t\t\"port\": ${port},\n"
                    command="${command}\t\t\t\t\t\t\"refresh_rate\": ${refresh_rate},\n"
                    command="${command}\t\t\t\t\t\t\"simult_transfers\": ${simult_transfers},\n"
                    command="${command}\t\t\t\t\t\t\"transfer_threads\": ${transfer_threads},\n"
                    command="${command}\t\t\t\t\t\t\"checkers_threads\": ${checkers_threads},\n"
                    command="${command}\t\t\t\t\t\t\"chunk_size\": \"${chunk_size}\",\n"
                    command="${command}\t\t\t\t\t\t\"buffer_size\": \"${buffer_size}\",\n"
                    command="${command}\t\t\t\t\t\t\"use_mmap\": ${use_mmap},\n"
                    command="${command}\t\t\t\t\t\t\"checksum\": ${checksum},\n"
                    command="${command}\t\t\t\t\t\t\"root_folder\": ${root_folder}\n"
                    

                    ;;
                "BITTORRENT")

                    read -p "Enter http server Port to use (ex:'8080'): " port
                    read -p "Enter Rate limit (ex:'-1'): " rate_limit

                    command="${command}\t\t\t\t\t\t\"port\": \"${port}\",\n"
                    command="${command}\t\t\t\t\t\t\"rate_limit\": ${rate_limit}\n"
                    ;;
                *)
                    echo "Invalid protocol"
                    continue
                    ;;
            esac
            command="${command}\t\t\t\t\t}\n"

            ;;
        2) 
            read -p "Enter the transfer id (ex:'transfer0'): " transfer_id
            read -p "Enter the destination client id (ex:'client0'): " client
            command_id="start_transfer"
            command="${command}\t\t\t\t\t\"transfer_id\": \"${transfer_id}\"\n"
            ;;
        3) 
            read -p "Enter the transfer id (ex:'transfer0'): " transfer_id
            read -p "Enter the destination client id (ex:'client0'): " client
            command_id="pause_transfer"
            command="${command}\t\t\t\t\t\"transfer_id\": \"${transfer_id}\"\n"
            ;;
        4) 
            read -p "Enter the transfer id (ex:'transfer0'): " transfer_id
            read -p "Enter the destination client id (ex:'client0'): " client
            command_id="resume_transfer"
            command="${command}\t\t\t\t\t\"transfer_id\": \"${transfer_id}\"\n"
            ;;
        5) 
            read -p "Enter the transfer id (ex:'transfer0'): " transfer_id
            read -p "Enter the destination client id (ex:'client0'): " client
            command_id="cancel_transfer"
            command="${command}\t\t\t\t\t\"transfer_id\": \"${transfer_id}\"\n"
            ;;
        6) 
            read -p "Enter the transfer id (ex:'transfer0'): " transfer_id
            read -p "Enter the destination client id (ex:'client0'): " client
            command_id="info"
            ;;
        7)
            exit 0
            ;;
        *)
            echo "Invalid choice"
            continue
            ;;
    esac

    file_name="generated_${command_id}_${client}_${transfer_id}.json"
    command="${command}\t\t\t\t},\n\t\t\t\t\"match\": \"${client}\"\n\t\t\t}\n\t\t]\n\t},\n\t\"id\": \"${command_id}\"\n}\n"

    echo ""
    echo ""
    echo "Saving command to ${file_name}"
    echo -e ${command} > ${file_name}

    echo "Sending command to ${dest_port}..."
    echo "curl --header 'Content-Type: application/json' --header 'X-Answer-Port: ${answer_port}' --request POST --data @${file_name} http://${dest_port}/command"
    curl --header 'Content-Type: application/json' --header 'X-Answer-Port: ${answer_port}' --request POST --data @${file_name} http://${dest_port}/command
    echo ""
done
