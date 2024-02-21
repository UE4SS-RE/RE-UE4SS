#include <format>
#include <future>
#include <regex>
#include <cmath>
#include <algorithm>

#include <cstring>

#include <Profiler/Profiler.hpp>
#include <SigScanner/SinglePassSigScanner.hpp>

#include <elf.h>

namespace RC
{

    auto ScanTargetArray::operator[](ScanTarget index) -> ModuleOS&
    {
        return array[static_cast<size_t>(index)];
    }

    namespace Platform
    {
        auto get_system_info() -> SystemInfo*
        {
            return &SigScannerStaticData::m_modules_info[ScanTarget::MainExe];
        }

        auto get_start_address(DLData *info) -> uint8_t* {
            return info->base_address;
        }

        auto get_end_address(DLData *info) -> uint8_t* {
            return info->base_address + info->size;
        }

        auto get_module_size(DLData *info) -> uint32_t {
            return info->size;
        }

        auto get_module_base(DLData *info) -> uint8_t* {
            return info->base_address;
        }
    }; // namespace Platform

    auto SinglePassScanner::string_scan(std::wstring_view string_to_scan_for, ScanTarget scan_target) -> void* {
        throw std::runtime_error{"[SinglePassSigScanner::string_scan] Not implemented for Linux"};
    }


    // This function may have problem becasue we're not checking the memory region's permissions
    auto SinglePassScanner::scanner_work_thread_scalar(uint8_t* start_address,
                                                       uint8_t* end_address,
                                                       DLData* info,
                                                       std::vector<SignatureContainer>& signature_containers) -> void
    {
        ProfilerScope();
        if (!start_address)
        {
            start_address = Platform::get_start_address(info);
        }
        if (!end_address)
        {
            end_address = Platform::get_end_address(info);
        }

        // TODO: Nasty nasty nasty. Come up with a better solution... wtf
        // It should ideally be able to work with the char* directly instead of converting to to vectors of ints
        // The reason why working directly with the char* is a problem is that it's expensive to convert a hex char to an int
        // Also look at the comments below
        std::vector<std::vector<std::vector<int>>> vector_of_sigs;

        // const std::regex signature_validity_regex(R"(^([0-9A-F\?] )([0-9A-F\?]\/[0-9A-F\?] )*([0-9A-F\?])$)");

        // Making a vector here to be identical to the SignatureContainer vector
        // The difference is that it stores the sigs converted from char* to std::vector<int>
        // This makes it easier to work with even if it's wasteful
        // This should not be done in the nested loops below because this operation is very slow
        vector_of_sigs.reserve(signature_containers.size());
        for (const auto& container : signature_containers)
        {
            // Only continue if the signature is properly formatted
            // Bring this code back when both:
            // A. The regex has been updated to take into consideration.
            // B. The threads have been synced before the scan to verify that all threads are scanning for valid signatures.
            // for (const auto& signature_data : container.signatures)
            //{
            //    if (!std::regex_search(signature_data.signature, signature_validity_regex))
            //    {
            //        throw std::runtime_error{std::format("[SinglePassSigScanner::start_scan] A signature is improperly formatted. Signature: {}", signature_data.signature)};
            //    }
            //}

            // Signatures for this container
            vector_of_sigs.emplace_back(string_to_vector(container.signatures));
        }

        // Loop everything
        for (uint8_t* i = start_address; i < end_address; ++i)
        {
            auto addr = info->find_phdr(i);
            if (addr == std::make_tuple(nullptr, 0, 0))
            {
                continue;
            }
            if (std::get<0>(addr) > i) {
                i = std::get<0>(addr);
            }
            uint8_t* region_end = static_cast<uint8_t*>(std::get<0>(addr)) + std::get<1>(addr);
            if ((std::get<2>(addr) & PF_R) == 0) {
                i = region_end;
                continue;
            }

            uint8_t* scan_end = std::min(end_address, region_end);
            for (uint8_t* region_start = std::get<0>(addr); region_start < scan_end; ++region_start)
            {
                bool skip_to_next_container{};

                for (size_t container_index = 0; const auto& int_container : vector_of_sigs)
                {
                    for (size_t signature_index = 0; const auto& sig : int_container)
                    {
                        // If the container is refusing more calls then skip to the next container
                        if (signature_containers[container_index].ignore)
                        {
                            break;
                        }

                        // Skip if we're about to dereference uninitialized memory
                        if (region_start + sig.size() > region_end)
                        {
                            break;
                        }

                        for (size_t sig_i = 0; sig_i < sig.size(); sig_i += 2)
                        {
                            if (sig.at(sig_i) != -1 && sig.at(sig_i) != HI_NIBBLE(*(uint8_t*)(region_start + (sig_i / 2))) ||
                                sig.at(sig_i + 1) != -1 && sig.at(sig_i + 1) != LO_NIBBLE(*(uint8_t*)(region_start + (sig_i / 2))))
                            {
                                break;
                            }

                            if (sig_i + 2 == sig.size())
                            {
                                {
                                    std::lock_guard<std::mutex> safe_scope(m_scanner_mutex);

                                    // Checking for the second time if the container is refusing more calls
                                    // This is required when multi-threading is enabled
                                    if (signature_containers[container_index].ignore)
                                    {
                                        skip_to_next_container = true;
                                        break;
                                    }

                                    // One of the signatures have found a full match so lets forward the details to the callable
                                    signature_containers[container_index].index_into_signatures = signature_index;
                                    signature_containers[container_index].match_address = region_start;
                                    signature_containers[container_index].match_signature_size = sig.size() / 2;

                                    skip_to_next_container = signature_containers[container_index].on_match_found(signature_containers[container_index]);
                                    signature_containers[container_index].ignore = skip_to_next_container;

                                    // Store results if the container at the containers request
                                    if (signature_containers[container_index].store_results)
                                    {
                                        signature_containers[container_index].result_store.emplace_back(
                                                SignatureContainerLight{.index_into_signatures = signature_index, .match_address = region_start});
                                    }
                                }

                                break;
                            }
                        }

                        if (skip_to_next_container)
                        {
                            // A match was found and signaled to skip to the next container
                            break;
                        }

                        ++signature_index;
                    }

                    ++container_index;
                }
            }

            i = scan_end;
        
        }
    }
} // namespace RC