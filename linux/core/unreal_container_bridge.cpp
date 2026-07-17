#include "unreal_container_bridge.hpp"

#include <Unreal/CoreUObject/UObject/UnrealType.hpp>

#include <exception>

namespace ue4ss::linux::core
{
    bool calculate_script_set_layout(std::int32_t element_size,
                                     std::int32_t element_alignment,
                                     UnrealScriptSetLayout& output,
                                     std::string& error) noexcept
    {
        output = {};
        error.clear();
        if (element_size <= 0 || element_alignment <= 0)
        {
            error = "positive set element size and alignment are required";
            return false;
        }
        try
        {
            const RC::Unreal::FScriptSetLayout layout =
                    RC::Unreal::FScriptSet::GetScriptLayout(element_size, element_alignment);
            output = {
                    .hash_next_id_offset = layout.HashNextIdOffset,
                    .hash_index_offset = layout.HashIndexOffset,
                    .element_stride = layout.Size,
                    .sparse_alignment = layout.SparseArrayLayout.Alignment,
                    .sparse_stride = layout.SparseArrayLayout.Size,
            };
            return true;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while calculating FScriptSet layout";
            return false;
        }
    }

    bool calculate_script_map_layout(std::int32_t key_size,
                                     std::int32_t key_alignment,
                                     std::int32_t value_size,
                                     std::int32_t value_alignment,
                                     UnrealScriptMapLayout& output,
                                     std::string& error) noexcept
    {
        output = {};
        error.clear();
        if (key_size <= 0 || key_alignment <= 0 || value_size <= 0 || value_alignment <= 0)
        {
            error = "positive map key/value sizes and alignments are required";
            return false;
        }
        try
        {
            const RC::Unreal::FScriptMapLayout layout = RC::Unreal::FScriptMap::GetScriptLayout(
                    key_size, key_alignment, value_size, value_alignment);
            output.value_offset = layout.ValueOffset;
            output.set = {
                    .hash_next_id_offset = layout.SetLayout.HashNextIdOffset,
                    .hash_index_offset = layout.SetLayout.HashIndexOffset,
                    .element_stride = layout.SetLayout.Size,
                    .sparse_alignment = layout.SetLayout.SparseArrayLayout.Alignment,
                    .sparse_stride = layout.SetLayout.SparseArrayLayout.Size,
            };
            return true;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while calculating FScriptMap layout";
            return false;
        }
    }

    std::uint32_t script_set_storage_size() noexcept
    {
        return sizeof(RC::Unreal::FScriptSet);
    }

    std::uint32_t script_map_storage_size() noexcept
    {
        return sizeof(RC::Unreal::FScriptMap);
    }

    std::uint32_t script_set_hash_size(std::uint32_t element_count) noexcept
    {
        return RC::Unreal::FDefaultSetAllocator::GetNumberOfHashBuckets(element_count);
    }
}
