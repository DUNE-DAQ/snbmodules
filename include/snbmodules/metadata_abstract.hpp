/**
 * @file metadata_abstract.hpp MetadataAbstract class, abstract class to declare metadata objects
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef SNBMODULES_INCLUDE_SNBMODULES_METADATA_ABSTRACT_HPP_
#define SNBMODULES_INCLUDE_SNBMODULES_METADATA_ABSTRACT_HPP_

// errors handling
#include "snbmodules/common/errors_declaration.hpp"
#include "logging/Logging.hpp"

// json
#include "appfwk/cmd/Nljs.hpp"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>

namespace dunedaq::snbmodules
{
    /// @brief Abstract class for metadata classes, they must be able to generate and load metadata files
    class MetadataAbstract
    {
    public:
        virtual ~MetadataAbstract() = default;

        /// @brief Export metadata to string (json format)
        /// @return Metadata in string format
        virtual std::string export_to_string() = 0;

        /// @brief Import metadata from string (json format)
        virtual void from_string(const std::string &s) = 0;

        /// @brief Generaete metadata file to dest
        /// @param dest
        virtual void generate_metadata_file(std::filesystem::path dest) = 0;

        /// @brief Load metadata file from src
        /// @param src
        virtual void load_metadata_from_meta_file(std::filesystem::path src) = 0;

        /// @brief Operator ==
        virtual bool operator==(MetadataAbstract const &other) const = 0;

        /// @brief Operator <
        virtual bool operator<(MetadataAbstract const &other) const = 0;
    };
} // namespace dunedaq::snbmodules
#endif // SNBMODULES_INCLUDE_SNBMODULES_METADATA_ABSTRACT_HPP_