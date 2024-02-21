#include <format>
#include <future>
#include <regex>

#include <cstring>

#ifdef WIN32
#define NOMINMAX
#include <Windows.h>
#include <Psapi.h>
#endif


#include <Profiler/Profiler.hpp>
#include <SigScanner/SinglePassSigScanner.hpp>

namespace RC
{
    
    auto SinglePassScanner::get_system_info() -> SystemInfo
    {
        SYSTEM_INFO info{};
        GetSystemInfo(&info);
        return info;
    }

    namespace Platform {
        // Get the start address of the system
        auto get_start_address(SystemInfo &info) -> uint8_t* {
            return static_cast<uint8_t*>(info.lpMinimumApplicationAddress);
        }

        // Get the end address of the system
        auto get_end_address(SystemInfo &info) -> uint8_t* {
            return static_cast<uint8_t*>(info.lpMaximumApplicationAddress);
        }

        // Get the size of the module
        auto get_module_size(ModuleOS &info) -> size_t {
            return info.SizeOfImage;
        }

        // Get the base address of the module
        auto get_module_base(ModuleOS &info) -> uint8_t* {
            return static_cast<uint8_t*>(info.lpBaseOfDll);
        }
    }; // namespace Platform
    
    auto WIN_MODULEINFO::operator=(MODULEINFO other) -> WIN_MODULEINFO&
    {
        lpBaseOfDll = other.lpBaseOfDll;
        SizeOfImage = other.SizeOfImage;
        EntryPoint = other.EntryPoint;
        return *this;
    }

    auto SinglePassScanner::string_scan(std::wstring_view string_to_scan_for, ScanTarget scan_target) -> void*
    {
        auto module = SigScannerStaticData::m_modules_info[scan_target];

        auto start_address = static_cast<uint8_t*>(module.lpBaseOfDll);
        auto end_address = static_cast<uint8_t*>(module.lpBaseOfDll) + module.SizeOfImage;

        MEMORY_BASIC_INFORMATION memory_info{};
        DWORD protect_flags = PAGE_GUARD | PAGE_NOCACHE | PAGE_NOACCESS;

        void* address_found{};

        for (uint8_t* i = start_address; i < end_address; ++i)
        {
            if (VirtualQuery(i, &memory_info, sizeof(memory_info)))
            {
                if (memory_info.Protect & protect_flags || !(memory_info.State & MEM_COMMIT))
                {
                    i += memory_info.RegionSize;
                    continue;
                }

                uint8_t* region_end = static_cast<uint8_t*>(memory_info.BaseAddress) + memory_info.RegionSize;

                for (uint8_t* region_start = static_cast<uint8_t*>(memory_info.BaseAddress); region_start < region_end; ++region_start)
                {
                    if (region_start > end_address || region_start + string_to_scan_for.size() > end_address)
                    {
                        break;
                    }

                    auto maybe_string = std::wstring_view((const wchar_t*)region_start, string_to_scan_for.size());
                    if (maybe_string == string_to_scan_for)
                    {
                        address_found = region_start;
                        break;
                    }
                }

                if (address_found)
                {
                    break;
                }
                i = static_cast<uint8_t*>(memory_info.BaseAddress) + memory_info.RegionSize;
            }
        }

        return address_found;
    }

    auto SinglePassScanner::scanner_work_thread_scalar(uint8_t* start_address,
                                                       uint8_t* end_address,
                                                       SYSTEM_INFO& info,
                                                       std::vector<SignatureContainer>& signature_containers) -> void
    {
        ProfilerScope();
        if (!start_address)
        {
            start_address = static_cast<uint8_t*>(info.lpMinimumApplicationAddress);
        }
        if (!end_address)
        {
            start_address = static_cast<uint8_t*>(info.lpMaximumApplicationAddress);
        }

        MEMORY_BASIC_INFORMATION memory_info{};
        DWORD protect_flags = PAGE_GUARD | PAGE_NOCACHE | PAGE_NOACCESS;

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
            // Populate memory_info if VirtualQuery doesn't fail
            if (VirtualQuery(i, &memory_info, sizeof(memory_info)))
            {
                // If the "protect flags" or state are undesired for this region then skip to the next iteration of the loop
                if (memory_info.Protect & protect_flags || !(memory_info.State & MEM_COMMIT))
                {
                    i += memory_info.RegionSize;
                    continue;
                }

                uint8_t* region_end = static_cast<uint8_t*>(memory_info.BaseAddress) + memory_info.RegionSize;

                for (uint8_t* region_start = static_cast<uint8_t*>(memory_info.BaseAddress); region_start < region_end; ++region_start)
                {
                    if (region_start > end_address)
                    {
                        break;
                    }

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
                                if (sig.at(sig_i) != -1 && sig.at(sig_i) != HI_NIBBLE(*(byte*)(region_start + (sig_i / 2))) ||
                                    sig.at(sig_i + 1) != -1 && sig.at(sig_i + 1) != LO_NIBBLE(*(byte*)(region_start + (sig_i / 2))))
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

                i = static_cast<uint8_t*>(memory_info.BaseAddress) + memory_info.RegionSize;
            }
        }
    }
} // namespace RC