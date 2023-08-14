

#ifndef SNBMODULES_INCLUDE_SNBMODULES_TRANSFERINTERFACERCLONE_HPP_
#define SNBMODULES_INCLUDE_SNBMODULES_TRANSFERINTERFACERCLONE_HPP_

#include "snbmodules/interfaces/transfer_interface_abstract.hpp"
#include "snbmodules/common/status_enum.hpp"

#include "librclone.h"
#include "appfwk/cmd/Nljs.hpp"

#include <chrono>
#include <iostream>
#include <stdio.h>
#include <fstream>

namespace dunedaq::snbmodules
{

    class TransferInterfaceRClone : public TransferInterfaceAbstract
    {
    private:
        struct parameters
        {
            std::string protocol = "http";
            std::string user;
            int port = 8080;
            std::string bwlimit = "off";
            int refresh_rate = 10;
            std::filesystem::path root_folder = "/";

            // config
            int simult_transfers = 16;
            int transfer_threads = 16;
            int checkers_threads = 16;

            std::string chunk_size = "10G";
            std::string buffer_size = "100G";
            bool use_mmap = false;
            bool checksum = true;

        } m_params;

        // job id to transfer metadata to keep track of the transfer and update the status
        std::map<TransferMetadata *, int> m_jobs_id;
        std::filesystem::path m_work_dir;

        std::optional<nlohmann::json> requestRPC(std::string const method, std::string const input)
        {
            char *m = const_cast<char *>(method.c_str());
            char *in = const_cast<char *>(input.c_str());

            struct RcloneRPCResult out = RcloneRPC(m, in);
            printf("debug : RClone : result status: %d\n", out.Status);
            printf("debug : RClone : result output: %s\n", out.Output);
            nlohmann::json j = nlohmann::json::parse(out.Output);
            free(out.Output);

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

                        for (auto t : current_transfers)
                        {
                            std::string grp = t["group"].get<std::string>();
                            int job_id = std::stoi(grp.substr(grp.find("/") + 1));

                            for (auto [meta, id] : m_jobs_id)
                            {
                                if (id == job_id)
                                {
                                    meta->set_progress(t["percentage"].get<int>());
                                    meta->set_transmission_speed(t["speedAvg"].get<unsigned long>());
                                    break;
                                }
                            }
                        }
                    }
                }

                // Update information about ongoing transfers
                for (auto [meta, id] : m_jobs_id)
                {
                    // get refreshed infos
                    char *input_request = (char *)malloc(100);
                    sprintf(input_request, "{"
                                           "\"jobid\": %d"
                                           "}",
                            id);

                    auto res = requestRPC("job/status", input_request);
                    free(input_request);

                    if (res.has_value())
                    {
                        if (res.value()["finished"] != nullptr && res.value()["finished"].get<bool>())
                        {
                            if (res.value()["success"].get<bool>())
                            {
                                meta->set_status(e_status::FINISHED);
                                meta->set_progress(100);
                            }
                            else
                            {
                                meta->set_status(e_status::ERROR);
                                meta->set_error_code(res.value()["error"].get<std::string>());
                            }
                            meta->set_transmission_speed(0);
                        }
                    }
                }

                // wait
                std::this_thread::sleep_for(std::chrono::seconds(m_params.refresh_rate));
            }

            for (auto meta : get_transfer_options()->get_transfers_meta())
            {
                if (meta->get_status() == e_status::UPLOADING)
                {
                    meta->set_status(e_status::FINISHED);
                }
                if (meta->get_status() == e_status::DOWNLOADING)
                {
                    meta->set_status(e_status::ERROR);
                    meta->set_error_code("Transfer interrupted");
                }
            }
        }

    public:
        TransferInterfaceRClone(GroupMetadata *config, std::filesystem::path work_dir)
            : TransferInterfaceAbstract(config),
              m_work_dir(work_dir),
              m_thread(std::bind(&TransferInterfaceRClone::do_work, this, std::placeholders::_1))
        {
            RcloneInitialize();
            m_thread.start_working_thread();

            m_params.protocol = config->get_protocol_options()["protocol"].get<std::string>();

            if (m_params.protocol == "sftp")
            {
                m_params.user = config->get_protocol_options()["user"].get<std::string>();
            }

            m_params.bwlimit = config->get_protocol_options()["rate_limit"].get<std::string>();
            m_params.refresh_rate = config->get_protocol_options()["refresh_rate"].get<int>();
            m_params.port = config->get_protocol_options()["port"].get<int>();

            // config parameters
            m_params.simult_transfers = config->get_protocol_options()["simult_transfers"].get<int>();
            m_params.transfer_threads = config->get_protocol_options()["transfer_threads"].get<int>();
            m_params.checkers_threads = config->get_protocol_options()["checkers_threads"].get<int>();

            m_params.chunk_size = config->get_protocol_options()["chunk_size"].get<std::string>();
            m_params.buffer_size = config->get_protocol_options()["buffer_size"].get<std::string>();
            m_params.use_mmap = config->get_protocol_options()["use_mmap"].get<bool>();
            m_params.checksum = config->get_protocol_options()["checksum"].get<bool>();
            m_params.root_folder = std::filesystem::absolute(config->get_protocol_options()["root_folder"].get<std::string>());

            char *input_request = (char *)malloc(200);
            sprintf(input_request, "{"
                                   "\"rate\": \"%s\""
                                   "}",
                    m_params.bwlimit.c_str());

            requestRPC("core/bwlimit", input_request);
            free(input_request);

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
        };

        virtual bool upload_file(TransferMetadata *f_meta) override
        {
            TLOG() << "debug : RClone : Uploading file " << f_meta->get_file_name();

            // char exec[300];
            // sprintf(exec,
            //         "rclone serve http / --addr %s:%d --buffer-size '%s' --no-modtime --transfers %d -vv --vfs-cache-mode 'off' --vfs-cache-max-size 'off'",
            //         f_meta->get_src().get_ip().c_str(),
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

            return true;
        }
        virtual bool download_file(TransferMetadata *f_meta, std::filesystem::path dest) override
        {
            TLOG() << "debug : RClone : Downloading file " << f_meta->get_file_name();

            char *input_request = (char *)malloc(1000);

            if (m_params.protocol == "http")
            {
                std::string file_relative_path = std::filesystem::relative(f_meta->get_file_path(), m_params.root_folder).generic_string();

                // check if file path is relative to root folder
                if (file_relative_path.find("..") != std::string::npos)
                {
                    TLOG() << "debug : RClone : File path is not relative to root folder";
                    f_meta->set_status(e_status::ERROR);
                    f_meta->set_error_code("File path is not relative to root folder !");
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
                        f_meta->get_src().get_ip().c_str(),
                        m_params.port,
                        file_relative_path.c_str(),

                        // destination
                        dest.append(f_meta->get_file_name()).string().c_str(),

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
                        f_meta->get_src().get_ip().c_str(),
                        m_params.user.c_str(),
                        m_params.port,
                        m_params.simult_transfers,

                        // f_meta->get_file_path().remove_filename().string().c_str(),
                        f_meta->get_file_path().string().c_str(),
                        // destination
                        // dest.string().c_str(),
                        dest.append(f_meta->get_file_name()).string().c_str(),

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
            //         f_meta->get_src().get_ip().c_str(),
            //         f_meta->get_file_path().string().c_str(),

            //         // destination
            //         dest.string().c_str());

            // auto res = requestRPC("operations/copyurl", input_request);

            std::cout << input_request << std::endl;
            free(input_request);
            if (res.has_value())
            {
                m_jobs_id[f_meta] = res.value()["jobid"];
            }
            else
            {
                return false;
            }

            // print local options of the transfer
            requestRPC("options/local", "");

            return true;
        }

        virtual bool pause_file(TransferMetadata *f_meta) override
        {
            TLOG() << "debug : RClone : Pausing file " << f_meta->get_file_name();
            ers::warning(RCloneNotSupportError(ERS_HERE, "pausing a single file. Pausing everything."));

            requestRPC("core/bwlimit", "{\"bytesPerSecond\": 0}");

            return true;
        }

        virtual bool resume_file(TransferMetadata *f_meta) override
        {
            TLOG() << "debug : RClone : Resuming file " << f_meta->get_file_name();
            ers::warning(RCloneNotSupportError(ERS_HERE, "resuming a single file. Resuming everything."));

            char *input_request = (char *)malloc(200);
            sprintf(input_request, "{"
                                   "\"bytesPerSecond\": %s"
                                   "}",
                    m_params.bwlimit.c_str());

            requestRPC("core/bwlimit", input_request);
            free(input_request);

            return true;
        }

        virtual bool hash_file(TransferMetadata *f_meta) override
        {
            TLOG() << "debug : RClone : Hashing file " << f_meta->get_file_name();

            return true;
        }

        virtual bool cancel_file(TransferMetadata *f_meta) override
        {
            TLOG() << "debug : RClone : Cancelling file " << f_meta->get_file_name();

            // find job id
            int job_id;
            if (m_jobs_id.find(f_meta) != m_jobs_id.end())
            {
                job_id = m_jobs_id[f_meta];
            }
            else
            {
                TLOG() << "debug : RClone : Job id not found";
                return false;
            }

            char *input_request = (char *)malloc(100);
            sprintf(input_request, "{"
                                   "\"jobid\": %d"
                                   "}",
                    job_id);

            requestRPC("job/stop", input_request);
            free(input_request);

            return true;
        }
    };
} // namespace dunedaq::snbmodules
#endif // SNBMODULES_SRC_RCLONE_HPP_