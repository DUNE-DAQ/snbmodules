/**
 * @file transfer_interface_RClone.hpp TransferInterfaceRClone protocol class for a RClone transfer
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef SNBMODULES_INCLUDE_SNBMODULES_INTERFACES_TRANSFER_INTERFACE_RCLONE_HPP_
#define SNBMODULES_INCLUDE_SNBMODULES_INTERFACES_TRANSFER_INTERFACE_RCLONE_HPP_

#include "snbmodules/interfaces/transfer_interface_abstract.hpp"
#include "snbmodules/common/status_enum.hpp"

#include "appfwk/cmd/Nljs.hpp"

#include <librclone.h>

#include <chrono>
#include <iostream>
#include <cstdio>
#include <fstream>

#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <utility>

namespace dunedaq::snbmodules
{

    class TransferInterfaceRClone : public TransferInterfaceAbstract
    {

    public:
        TransferInterfaceRClone(GroupMetadata &config, const std::filesystem::path &work_dir)
            : TransferInterfaceAbstract(config),
              m_work_dir(work_dir),
              m_thread([&](std::atomic<bool> &running)
                       { this->do_work(running); })
        {
            RcloneInitialize();
            m_thread.start_working_thread();

            // protocol parameters

            if (config.get_protocol_options().contains("protocol"))
            {
                m_params.protocol = config.get_protocol_options()["protocol"].get<std::string>();
            }

            if (m_params.protocol == "sftp")
            {
                if (config.get_protocol_options().contains("user"))
                {
                    m_params.user = config.get_protocol_options()["user"].get<std::string>();
                }
            }

            if (config.get_protocol_options().contains("rate_limit"))
            {
                m_params.bwlimit = config.get_protocol_options()["rate_limit"].get<std::string>();
            }
            if (config.get_protocol_options().contains("refresh_rate"))
            {
                m_params.refresh_rate = config.get_protocol_options()["refresh_rate"].get<int>();
            }
            if (config.get_protocol_options().contains("port"))
            {
                m_params.port = config.get_protocol_options()["port"].get<int>();
            }

            // config parameters
            if (config.get_protocol_options().contains("simult_transfers"))
            {
                m_params.simult_transfers = config.get_protocol_options()["simult_transfers"].get<int>();
            }
            if (config.get_protocol_options().contains("transfer_threads"))
            {
                m_params.transfer_threads = config.get_protocol_options()["transfer_threads"].get<int>();
            }
            if (config.get_protocol_options().contains("checkers_threads"))
            {
                m_params.checkers_threads = config.get_protocol_options()["checkers_threads"].get<int>();
            }

            if (config.get_protocol_options().contains("chunk_size"))
            {
                m_params.chunk_size = config.get_protocol_options()["chunk_size"].get<std::string>();
            }
            if (config.get_protocol_options().contains("buffer_size"))
            {
                m_params.buffer_size = config.get_protocol_options()["buffer_size"].get<std::string>();
            }
            if (config.get_protocol_options().contains("use_mmap"))
            {
                m_params.use_mmap = config.get_protocol_options()["use_mmap"].get<bool>();
            }
            if (config.get_protocol_options().contains("checksum"))
            {
                m_params.checksum = config.get_protocol_options()["checksum"].get<bool>();
            }
            if (config.get_protocol_options().contains("root_folder"))
            {
                m_params.root_folder = std::filesystem::absolute(config.get_protocol_options()["root_folder"].get<std::string>());
            }

            char *input_request = new char[100];
            sprintf(input_request, "{"
                                   "\"rate\": \"%s\""
                                   "}",
                    m_params.bwlimit.c_str());

            requestRPC("core/bwlimit", input_request);
            delete[] input_request;

            requestRPC("options/set", "{"
                                      "\"vfs\": "
                                      "{"
                                      //   "\"ChunkSize\": 8388608,"
                                      "\"CacheMaxSize\": 0"
                                      "}"
                                      "}");

            // all global options
            requestRPC("options/get", "{}");
        }

        virtual ~TransferInterfaceRClone()
        {
            m_thread.stop_working_thread();

            RcloneFinalize();
        }

        bool upload_file(TransferMetadata &f_meta) override
        {
            TLOG() << "debug : RClone : Uploading file " << f_meta.get_file_name();

            // char exec[300];
            // sprintf(exec,
            //         "rclone serve http / --addr %s:%d --buffer-size '%s' --no-modtime --transfers %d -vv --vfs-cache-mode 'off' --vfs-cache-max-size 'off'",
            //         f_meta.get_src().get_ip().c_str(),
            //         m_params.port,
            //         m_params.buffer_size,
            //         m_params.simult_transfers);

            // TLOG() << "debug : executing " << exec;
            // if (system(exec) == 0)
            // {
            //     TLOG() << "debug : RClone : Sucess Starting server";
            // }
            // else
            // {
            //     // TODO: error
            //     return false;
            // }

            f_meta.set_status(status_type::e_status::FINISHED);

            return true;
        }
        bool download_file(TransferMetadata &f_meta, std::filesystem::path dest) override
        {
            TLOG() << "debug : RClone : Downloading file " << f_meta.get_file_name();

            char *input_request = new char[1000];

            if (m_params.protocol == "http")
            {
                std::string file_relative_path = std::filesystem::relative(f_meta.get_file_path(), m_params.root_folder).generic_string();

                // check if file path is relative to root folder
                if (file_relative_path.find("..") != std::string::npos)
                {
                    TLOG() << "debug : RClone : File path is not relative to root folder";
                    f_meta.set_status(status_type::e_status::ERROR);
                    f_meta.set_error_code("File path is not relative to root folder !");
                    return false;
                }

                sprintf(input_request, "{"
                                       "\"srcFs\": "
                                       "{"
                                       "\"type\": \"http\"," // HTTP/WebDAV/FTP/SFTP/DLNA/DOCKER
                                       "\"url\": \"http://%s:%d\""
                                       "},"
                                       "\"srcRemote\": \"%s\","

                                       "\"dstFs\": "
                                       "\"/\","
                                       "\"dstRemote\": \"%s\","

                                       "\"_config\": {"

                                       "\"BindAddr\": \"\","

                                       "\"MultiThreadSet\": true,"
                                       "\"Transfers\": %d,"
                                       "\"Checkers\": %d,"
                                       "\"MultiThreadStreams\": %d,"
                                       "\"MultiThreadCutoff\": \"%s\","
                                       "\"StreamingUploadCutoff\": \"%s\","

                                       "\"UseMmap\": %s,"
                                       "\"CheckSum\": %s,"
                                       "\"BufferSize\": \"%s\","
                                       "\"ErrorOnNoTransfer\": true"
                                       "},"

                                       "\"_async\": true"
                                       "}",

                        // source
                        f_meta.get_src().get_ip().c_str(),
                        m_params.port,
                        file_relative_path.c_str(),

                        // destination
                        dest.append(f_meta.get_file_name()).string().c_str(),

                        // config
                        m_params.simult_transfers,
                        m_params.transfer_threads,
                        m_params.checkers_threads,
                        m_params.chunk_size.c_str(),
                        m_params.chunk_size.c_str(),
                        m_params.use_mmap ? "true" : "false",
                        m_params.checksum ? "true" : "false",
                        m_params.buffer_size.c_str());
            }
            else if (m_params.protocol == "sftp")
            {
                sprintf(input_request, "{"
                                       "\"srcFs\": "
                                       //    "\":%s,host=\'%s\',user=\'%s\',key_file=\'/home/ljoly/.ssh/id_rsa\':/\","

                                       //    "{"
                                       //    "\"type\": \"http\"," // HTTP/WebDAV/FTP/SFTP/DLNA/DOCKER
                                       //    "\"url\": \"http://127.0.0.1:8000\""
                                       //    //    "\"no-head\": \"true\","
                                       //    "},"

                                       "{"
                                       "\"type\": \"sftp\"," // HTTP/WebDAV/FTP/SFTP/DLNA/DOCKER
                                       "\"host\": \"%s\","
                                       "\"user\": \"%s\","
                                       "\"port\": \"%d\","
                                       "\"key_file\": \"/home/ljoly/.ssh/id_rsa\","
                                       "\"disable_concurrent_writes\": \"false\","
                                       "\"concurrency\": \"%d\""
                                       // "\"pubkey_file\": \"/home/ljoly/.ssh/id_rsa.pub\","
                                       //   "\"known_hosts_file\": \"/home/ljoly/.ssh/known_hosts\","
                                       //    "\"_path\": \"/home/ljoly/N23-04-17\""
                                       //   "\"_name\": \"distant\""
                                       "},"
                                       "\"srcRemote\": \"../..%s\","

                                       //    "{"                  // not supported enought
                                       //    "\"type\": \"ftp\"," // HTTP/WebDAV/FTP/SFTP/DLNA/DOCKER
                                       //    "\"host\": \"%s\","
                                       //    "\"user\": \"anonymous\","
                                       //    "\"port\": \"2121\","
                                       //    "\"pass\": \"LXOcWWr5yMievDLHrJ2IapoJZB_AQhinb9V0\""
                                       //    "},"

                                       //    "{"
                                       //    "\"type\": \"chunker\","
                                       //    "\"remote\": \":%s,host=\'%s\',user=\'%s\',key_file=\'/home/ljoly/.ssh/id_rsa\':%s\","
                                       //    "\"chunk_size\": \"1Gi\","
                                       //    "\"hash_type\": \"md5\""
                                       //    "},"

                                       "\"dstFs\": "
                                       "\"/\","
                                       "\"dstRemote\": \"%s\","

                                       "\"_config\": {"

                                       "\"BindAddr\": \"\","

                                       "\"MultiThreadSet\": true,"
                                       "\"Transfers\": %d,"
                                       "\"Checkers\": %d,"
                                       "\"MultiThreadStreams\": %d,"
                                       "\"MultiThreadCutoff\": \"%s\","

                                       "\"UseMmap\": %s,"
                                       "\"CheckSum\": %s,"
                                       "\"BufferSize\": \"%s\","
                                       "\"ErrorOnNoTransfer\": true"
                                       "},"

                                       "\"_async\": true"
                                       "}",

                        // source
                        // m_params.type.c_str(),
                        f_meta.get_src().get_ip().c_str(),
                        m_params.user.c_str(),
                        m_params.port,
                        m_params.simult_transfers,

                        // f_meta.get_file_path().remove_filename().string().c_str(),
                        f_meta.get_file_path().string().c_str(),
                        // destination
                        // dest.string().c_str(),
                        dest.append(f_meta.get_file_name()).string().c_str(),

                        // config
                        m_params.simult_transfers,
                        m_params.transfer_threads,
                        m_params.checkers_threads,
                        m_params.chunk_size.c_str(),
                        m_params.use_mmap ? "true" : "false",
                        m_params.checksum ? "true" : "false",
                        m_params.buffer_size.c_str());
            }

            auto res = requestRPC("operations/copyfile", input_request);
            TLOG() << "Requested copyfile operation with parameters : " << input_request;

            // sprintf(input_request, "{"
            //                        "\"url\": \"http://%s:8080%s\","

            //                        "\"fs\": \"/\","
            //                        "\"remote\": \"%s\","

            //                        "\"autoFilename\": true,"

            //                        //    "\"_config\": {"
            //                        //    // for this file only
            //                        //    //    "\"Progress\": true,"
            //                        //    // general config to move
            //                        //    "\"CheckSum\": true,"
            //                        //    "\"BindAddr\": \"\","
            //                        //    "\"BufferSize\": 67108864,"
            //                        //    "\"CheckSum\": true,"
            //                        //    "\"ErrorOnNoTransfer\": true,"
            //                        //    "\"MultiThreadSet\": true,"
            //                        //    "\"MultiThreadStreams\": 4,"
            //                        //    "\"Transfers\": 200,"
            //                        //    "\"UseMmap\": true"
            //                        //    "},"

            //                        "\"_async\": true"
            //                        "}",

            //         // source
            //         f_meta.get_src().get_ip().c_str(),
            //         f_meta.get_file_path().string().c_str(),

            //         // destination
            //         dest.string().c_str());

            // auto res = requestRPC("operations/copyurl", input_request);

            TLOG() << input_request;

            delete[] input_request;
            if (res.has_value())
            {
                m_jobs_id[&f_meta] = res.value()["jobid"];
            }
            else
            {
                return false;
            }

            // print local options of the transfer
            requestRPC("options/local", "");

            return true;
        }

        bool pause_file(TransferMetadata &f_meta) override
        {
            TLOG() << "debug : RClone : Pausing file " << f_meta.get_file_name();
            ers::warning(RCloneNotSupportError(ERS_HERE, "pausing a single file. Pausing everything."));

            requestRPC("core/bwlimit", "{\"bytesPerSecond\": 0}");

            return true;
        }

        bool resume_file(TransferMetadata &f_meta) override
        {
            TLOG() << "debug : RClone : Resuming file " << f_meta.get_file_name();
            ers::warning(RCloneNotSupportError(ERS_HERE, "resuming a single file. Resuming everything."));

            char *input_request = new char[100];
            sprintf(input_request, "{"
                                   "\"bytesPerSecond\": %s"
                                   "}",
                    m_params.bwlimit.c_str());

            requestRPC("core/bwlimit", input_request);
            delete[] input_request;

            return true;
        }

        bool hash_file(TransferMetadata &f_meta) override
        {
            TLOG() << "debug : RClone : Hashing file " << f_meta.get_file_name();

            return true;
        }

        bool cancel_file(TransferMetadata &f_meta) override
        {
            TLOG() << "debug : RClone : Cancelling file " << f_meta.get_file_name();

            // find job id
            int job_id = 0;
            if (m_jobs_id.find(&f_meta) != m_jobs_id.end())
            {
                job_id = m_jobs_id[&f_meta];
            }
            else
            {
                TLOG() << "debug : RClone : Job id not found";
                return false;
            }

            char *input_request = new char[100];
            sprintf(input_request, "{"
                                   "\"jobid\": %d"
                                   "}",
                    job_id);

            requestRPC("job/stop", input_request);
            delete[] input_request;

            return true;
        }

    private:
        struct parameters
        {
            std::string protocol = "http";
            std::string user = "anonymous";
            int port = 8080;
            std::string bwlimit = "off";
            int refresh_rate = 10;
            std::filesystem::path root_folder = "/";

            // config
            int simult_transfers = 200;
            int transfer_threads = 4;
            int checkers_threads = 1;

            std::string chunk_size = "10G";
            std::string buffer_size = "100G";
            bool use_mmap = false;
            bool checksum = true;

        } m_params;

        // job id to transfer metadata to keep track of the transfer and update the status
        std::map<TransferMetadata *, int> m_jobs_id;
        std::filesystem::path m_work_dir;

        std::optional<nlohmann::json> requestRPC(const std::string &method, const std::string &input)
        {
            char *m = strdup(method.c_str());
            char *in = strdup(input.c_str());

            struct RcloneRPCResult out = RcloneRPC(m, in);
            TLOG_DEBUG(2) << "debug : RClone : result status: " << out.Status;
            TLOG_DEBUG(2) << "debug : RClone : result output: " << out.Output;
            nlohmann::json j = nlohmann::json::parse(out.Output);
            free(out.Output); // NOLINT

            if (out.Status != 200)
            {
                return std::nullopt;
            }
            return j;
        }

        // Threading
        dunedaq::utilities::WorkerThread m_thread;
        void do_work(std::atomic<bool> &running)
        {
            TLOG() << "debug : running thread ";

            while (running.load())
            {
                // requestRPC("cache/stats", "{}");
                // requestRPC("core/memstats", "{}");
                auto stats = requestRPC("core/stats", "{}");

                if (stats.has_value())
                {
                    if (stats.value()["transferring"] != nullptr)
                    {
                        auto current_transfers = stats.value()["transferring"].get<std::vector<nlohmann::json>>();

                        for (const auto &t : current_transfers)
                        {
                            auto grp = t["group"].get<std::string>();
                            int job_id = std::stoi(grp.substr(grp.find("/") + 1));

                            for (auto &[meta, id] : m_jobs_id)
                            {
                                if (id == job_id)
                                {
                                    meta->set_progress(t["percentage"].get<int>());
                                    meta->set_transmission_speed(t["speedAvg"].get<int32_t>());
                                    break;
                                }
                            }
                        }
                    }
                }

                // Update information about ongoing transfers
                for (auto &[meta, id] : m_jobs_id)
                {
                    // get refreshed infos
                    char *input_request = new char[100];
                    sprintf(input_request, "{"
                                           "\"jobid\": %d"
                                           "}",
                            id);

                    auto res = requestRPC("job/status", input_request);
                    delete[] input_request;

                    if (res.has_value())
                    {
                        if (res.value()["finished"] != nullptr && res.value()["finished"].get<bool>())
                        {
                            if (res.value()["success"].get<bool>())
                            {
                                meta->set_status(status_type::e_status::FINISHED);
                                meta->set_progress(100);
                            }
                            else
                            {
                                meta->set_status(status_type::e_status::ERROR);
                                meta->set_error_code(res.value()["error"].get<std::string>());
                            }
                            meta->set_transmission_speed(0);
                        }
                    }
                }

                // wait
                std::this_thread::sleep_for(std::chrono::seconds(m_params.refresh_rate));
            }

            // TODO Leo joly 11/09/2023 : segmentation fault
            // for (auto &meta : get_transfer_options().get_transfers_meta())
            // {
            //     if (meta->get_status() == status_type::e_status::UPLOADING)
            //     {
            //         meta->set_status(status_type::e_status::FINISHED);
            //     }
            //     if (meta->get_status() == status_type::e_status::DOWNLOADING)
            //     {
            //         meta->set_status(status_type::e_status::ERROR);
            //         meta->set_error_code("Transfer interrupted");
            //     }
            // }
        }
    };
} // namespace dunedaq::snbmodules
#endif // SNBMODULES_INCLUDE_SNBMODULES_INTERFACES_TRANSFER_INTERFACE_RCLONE_HPP_