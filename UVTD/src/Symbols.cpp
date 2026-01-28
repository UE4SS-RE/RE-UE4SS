#include "Helpers/String.hpp"

#include <algorithm>
#include <format>

#include <DynamicOutput/DynamicOutput.hpp>
#include <File/File.hpp>
#include <UVTD/ConfigUtil.hpp>
#include <UVTD/Helpers.hpp>
#include <UVTD/Symbols.hpp>
#include <UVTD/TemplateClassParser.hpp>

#include <PDB.h>
#include <set>

namespace RC::UVTD
{
// ============= Constants =============
namespace Constants {
    // PDB Format Constants
    constexpr uint16_t PADDING_MARKER = 0xF0;
    constexpr uint16_t PADDING_SIZE_MASK = 0x0F;
    constexpr uint16_t PDB_TYPE_RECORD_HEADER_SIZE = sizeof(uint16_t);  // Type record header is 2 bytes
    constexpr uint32_t NUMERIC_LEAF_PREFIX_SIZE = sizeof(uint16_t);
    constexpr uint32_t VTABLE_OFFSET_SIZE = sizeof(uint32_t);
    constexpr uint32_t INVALID_RECORD_SIZE = 0;
    constexpr size_t MAX_STRING_LENGTH = 4096;  // Sanity check for strings
    
    // Pointer Type Values (from PDB spec)
    enum class PointerTypeValue : uint32_t {
        Near16 = 0x00,
        Far16  = 0x01,
        Huge16 = 0x02,
        Near32 = 0x0A,
        Far32  = 0x0B,
        Near64 = 0x0C
    };
}

// ============= Validation Helpers =============
inline bool is_valid_pointer_range(const uint8_t* current, const uint8_t* end, size_t required_size) {
    return current && end && current + required_size <= end && required_size <= Constants::MAX_STRING_LENGTH;
}

inline bool is_padding_record(uint16_t kind_value) {
    return kind_value >= Constants::PADDING_MARKER && kind_value < 0xF10;
}

inline uint32_t get_padding_size(uint16_t kind_value) {
    return kind_value & Constants::PADDING_SIZE_MASK;
}

// ============= Safe String Operations =============
inline uint32_t safe_strlen(const char* str, size_t max_len = Constants::MAX_STRING_LENGTH) {
    if (!str) return 0;
    
    // Use strnlen for safety
    size_t len = strnlen(str, max_len);
    
    // If we hit max_len, the string is likely corrupted
    if (len >= max_len) {
        // Log warning if you have logging
        return 0;
    }
    
    return static_cast<uint32_t>(len + 1); // +1 for null terminator
}

// ============= Method Property Helpers =============
inline bool is_virtual_method(uint16_t mprop) {
    return mprop == static_cast<uint16_t>(PDB::CodeView::TPI::MethodProperty::Intro) ||
           mprop == static_cast<uint16_t>(PDB::CodeView::TPI::MethodProperty::PureIntro) ||
           mprop == static_cast<uint16_t>(PDB::CodeView::TPI::MethodProperty::Virtual);
}

inline bool is_static_method(uint16_t mprop) {
    return mprop == static_cast<uint16_t>(PDB::CodeView::TPI::MethodProperty::Static);
}

// ============= Type Classification Helpers =============
inline bool is_class_or_struct(PDB::CodeView::TPI::TypeRecordKind kind) {
    return kind == PDB::CodeView::TPI::TypeRecordKind::LF_CLASS ||
           kind == PDB::CodeView::TPI::TypeRecordKind::LF_STRUCTURE;
}

inline bool is_pointer_type(PDB::CodeView::TPI::TypeIndexKind type) {
    // Check if the type index is in any of the pointer ranges
    uint32_t val = static_cast<uint32_t>(type);
    
    // Near pointers (0x100-0x1FF)
    // Far pointers (0x200-0x2FF)
    // Huge pointers (0x300-0x3FF)
    // 32-bit pointers (0x400-0x4FF)
    // 64-bit pointers (0x600-0x6FF)
    if (val >= 0x100 && val < 0x700) return true;
    
    return false;
}

// ============= Numeric Leaf Helpers =============
struct NumericValue {
    uint64_t value;
    uint32_t size_consumed;  // How many bytes were consumed reading this value
    bool is_valid;
};

inline NumericValue read_numeric_safe(const uint8_t* data, const uint8_t* data_end) {
    if (!data || !data_end || data + sizeof(uint16_t) > data_end) {
        return NumericValue{0, 0, false};
    }
    
    const auto leaf = *reinterpret_cast<const uint16_t*>(data);
    const uint8_t* current = data + sizeof(uint16_t);
    
    // If the leaf value is less than LF_NUMERIC, it's the actual value
    if (leaf < static_cast<uint16_t>(PDB::CodeView::TPI::TypeRecordKind::LF_NUMERIC)) {
        return NumericValue{leaf, 0, true};  // No additional bytes consumed
    }
    
    // Otherwise, it's a numeric leaf that follows
    switch (static_cast<PDB::CodeView::TPI::TypeRecordKind>(leaf)) {
        case PDB::CodeView::TPI::TypeRecordKind::LF_CHAR:
        {
            if (current + sizeof(int8_t) > data_end) return NumericValue{0, 0, false};
            // Explicit cast to avoid narrowing conversion
            int8_t val = *reinterpret_cast<const int8_t*>(current);
            return NumericValue{static_cast<uint64_t>(val), sizeof(int8_t), true};
        }
            
        case PDB::CodeView::TPI::TypeRecordKind::LF_SHORT:
        {
            if (current + sizeof(int16_t) > data_end) return NumericValue{0, 0, false};
            // Explicit cast to avoid narrowing conversion
            int16_t val = *reinterpret_cast<const int16_t*>(current);
            return NumericValue{static_cast<uint64_t>(val), sizeof(int16_t), true};
        }
            
        case PDB::CodeView::TPI::TypeRecordKind::LF_USHORT:
        {
            if (current + sizeof(uint16_t) > data_end) return NumericValue{0, 0, false};
            uint16_t val = *reinterpret_cast<const uint16_t*>(current);
            return NumericValue{static_cast<uint64_t>(val), sizeof(uint16_t), true};
        }
            
        case PDB::CodeView::TPI::TypeRecordKind::LF_LONG:
        {
            if (current + sizeof(int32_t) > data_end) return NumericValue{0, 0, false};
            // Explicit cast to avoid narrowing conversion
            int32_t val = *reinterpret_cast<const int32_t*>(current);
            return NumericValue{static_cast<uint64_t>(val), sizeof(int32_t), true};
        }
            
        case PDB::CodeView::TPI::TypeRecordKind::LF_ULONG:
        {
            if (current + sizeof(uint32_t) > data_end) return NumericValue{0, 0, false};
            uint32_t val = *reinterpret_cast<const uint32_t*>(current);
            return NumericValue{static_cast<uint64_t>(val), sizeof(uint32_t), true};
        }
            
        case PDB::CodeView::TPI::TypeRecordKind::LF_QUADWORD:
        {
            if (current + sizeof(int64_t) > data_end) return NumericValue{0, 0, false};
            // Explicit cast to avoid narrowing conversion
            int64_t val = *reinterpret_cast<const int64_t*>(current);
            return NumericValue{static_cast<uint64_t>(val), sizeof(int64_t), true};
        }
            
        case PDB::CodeView::TPI::TypeRecordKind::LF_UQUADWORD:
        {
            if (current + sizeof(uint64_t) > data_end) return NumericValue{0, 0, false};
            uint64_t val = *reinterpret_cast<const uint64_t*>(current);
            return NumericValue{val, sizeof(uint64_t), true};
        }
            
        default:
            return NumericValue{0, 0, false};  // Unknown numeric leaf type
    }
}
    
    
    Symbols::Symbols(std::filesystem::path pdb_file_path)
        : pdb_file_path(pdb_file_path), pdb_file_handle(std::move(File::open(pdb_file_path))), pdb_file_map(std::move(pdb_file_handle.memory_map())),
          pdb_file(pdb_file_map.data())
    {
        // Parse PDB name using standardized format: Major_Minor[-Suffix1][-Suffix2]
        auto pdb_stem = this->pdb_file_path.filename().stem().wstring();
        auto name_info = PDBNameInfo::parse(pdb_stem);

        if (name_info.has_value())
        {
            m_pdb_name_info = *name_info;
            is_425_plus = m_pdb_name_info.is_at_least(4, 25);
        }
        else
        {
            Output::send(STR("Warning: PDB name '{}' does not match expected format Major_Minor[-Suffix]. Using defaults.\n"), pdb_stem);
            is_425_plus = false;
        }

        if (!std::filesystem::exists(pdb_file_path))
        {
            throw std::runtime_error{std::format("PDB '{}' not found", pdb_file_path.string())};
        }

        if (PDB::HasValidDBIStream(pdb_file) != PDB::ErrorCode::Success)
        {
            throw std::runtime_error{std::format("PDB '{}' doesn't contain a valid DBI stream", pdb_file_path.string())};
        }

        if (PDB::HasValidTPIStream(pdb_file) != PDB::ErrorCode::Success)
        {
            throw std::runtime_error{std::format("PDB '{}' doesn't contain a valid TPI stream", pdb_file_path.string())};
        }

        dbi_stream = PDB::CreateDBIStream(pdb_file);

        // The DBI stream is typically at index 3
        uint32_t dbiStreamIndex = 3;
        PDB::DirectMSFStream dbiDirectStream = pdb_file.CreateMSFStream<PDB::DirectMSFStream>(dbiStreamIndex);
    
        if (dbiDirectStream.GetSize() >= sizeof(PDB::DBI::StreamHeader))
        {
            // Read the header directly from the scattered blocks into a local variable.
            PDB::DBI::StreamHeader header{};
            dbiDirectStream.ReadAtOffset(&header, sizeof(PDB::DBI::StreamHeader), 0);
    
            m_machine_type = static_cast<PDB::CodeView::DBI::CPUType>(header.machine);
        }
        else
        {
            throw std::runtime_error{std::format("Failed to read DBI Stream Header for PDB '{}'", pdb_file_path.string())};
        }
    }

    Symbols::Symbols(const Symbols& other)
        : pdb_file_path(other.pdb_file_path), pdb_file_handle(std::move(File::open(pdb_file_path))), pdb_file_map(std::move(pdb_file_handle.memory_map())),
          pdb_file(pdb_file_map.data()), is_425_plus(other.is_425_plus), m_machine_type(other.m_machine_type), m_pdb_name_info(other.m_pdb_name_info)
    {
        dbi_stream = PDB::CreateDBIStream(pdb_file);
    }

    Symbols& Symbols::operator=(const Symbols& other)
    {
        pdb_file_path = other.pdb_file_path;
        pdb_file_handle = std::move(File::open(pdb_file_path));
        pdb_file_map = std::move(pdb_file_handle.memory_map());
        pdb_file = PDB::RawFile(pdb_file_map.data());
        is_425_plus = other.is_425_plus;
        m_machine_type = other.m_machine_type;
        m_pdb_name_info = other.m_pdb_name_info;
        dbi_stream = PDB::CreateDBIStream(pdb_file);

        return *this;
    }

    MethodQualifiers extract_method_qualifiers(const PDB::TPIStream& tpi_stream,
                                               const PDB::CodeView::TPI::Record* function_record)
    {
        MethodQualifiers quals;

        auto this_pointer = tpi_stream.GetTypeRecord(function_record->data.LF_MFUNCTION.thistype);
        if (!this_pointer) return quals;

        // Check if underlying type has modifiers
        auto underlying = tpi_stream.GetTypeRecord(this_pointer->data.LF_POINTER.utype);
        if (underlying && underlying->header.kind == PDB::CodeView::TPI::TypeRecordKind::LF_MODIFIER)
        {
            quals.is_const = underlying->data.LF_MODIFIER.attr.MOD_const;
            quals.is_volatile = underlying->data.LF_MODIFIER.attr.MOD_volatile;
        }

        // Check ref-qualifiers on the this pointer
        quals.is_lvalue_ref = this_pointer->data.LF_POINTER.attr.islref;
        quals.is_rvalue_ref = this_pointer->data.LF_POINTER.attr.isrref;

        return quals;
    }

    auto Symbols::generate_method_signature(const PDB::TPIStream& tpi_stream,
                                            const PDB::CodeView::TPI::Record* function_record,
                                            File::StringType method_name)
        -> MethodSignature
    {
        MethodSignature signature{};
        signature.name = method_name;

        // Get all qualifiers
        signature.qualifiers = extract_method_qualifiers(tpi_stream, function_record);

        // Get parameters
        auto arg_list = tpi_stream.GetTypeRecord(function_record->data.LF_MFUNCTION.arglist);
        for (size_t i = 0; i < function_record->data.LF_MFUNCTION.parmcount; i++)
        {
            auto argument = Symbols::get_type_name(tpi_stream, arg_list->data.LF_ARGLIST.arg[i]);
            signature.params.push_back(FunctionParam{.type = argument});
        }

        // Get return type
        if (function_record->data.LF_MFUNCTION.rvtype)
        {
            signature.return_type = Symbols::get_type_name(tpi_stream, function_record->data.LF_MFUNCTION.rvtype);
        }

        return signature;
    }

    
    auto Symbols::is_x64() const -> bool
    {
        // Use the CPUType enum for clarity and correctness
        return m_machine_type == PDB::CodeView::DBI::CPUType::X64 ||
               m_machine_type == PDB::CodeView::DBI::CPUType::AMD64;
    }

    auto Symbols::is_x86() const -> bool
    {
        // Use the CPUType enum
        return m_machine_type == PDB::CodeView::DBI::CPUType::Intel80386;
    }

    auto Symbols::read_numeric(const uint8_t*& data) -> uint64_t
    {
        const auto leaf = *reinterpret_cast<const uint16_t*>(data);
        data += sizeof(uint16_t);

        if (leaf < static_cast<uint16_t>(PDB::CodeView::TPI::TypeRecordKind::LF_NUMERIC))
        {
            return leaf;
        }

        switch (static_cast<PDB::CodeView::TPI::TypeRecordKind>(leaf))
        {
        case PDB::CodeView::TPI::TypeRecordKind::LF_CHAR:
        {
            const auto value = *reinterpret_cast<const int8_t*>(data);
            data += sizeof(int8_t);
            return value;
        }
        case PDB::CodeView::TPI::TypeRecordKind::LF_SHORT:
        {
            const auto value = *reinterpret_cast<const int16_t*>(data);
            data += sizeof(int16_t);
            return value;
        }
        case PDB::CodeView::TPI::TypeRecordKind::LF_USHORT:
        {
            const auto value = *reinterpret_cast<const uint16_t*>(data);
            data += sizeof(uint16_t);
            return value;
        }
        case PDB::CodeView::TPI::TypeRecordKind::LF_LONG:
        {
            const auto value = *reinterpret_cast<const int32_t*>(data);
            data += sizeof(int32_t);
            return value;
        }
        case PDB::CodeView::TPI::TypeRecordKind::LF_ULONG:
        {
            const auto value = *reinterpret_cast<const uint32_t*>(data);
            data += sizeof(uint32_t);
            return value;
        }
        case PDB::CodeView::TPI::TypeRecordKind::LF_QUADWORD:
        {
            const auto value = *reinterpret_cast<const int64_t*>(data);
            data += sizeof(int64_t);
            return value;
        }
        case PDB::CodeView::TPI::TypeRecordKind::LF_UQUADWORD:
        {
            const auto value = *reinterpret_cast<const uint64_t*>(data);
            data += sizeof(uint64_t);
            return value;
        }
        default:
            // This type is not a numeric leaf we can decode.
                return 0;
        }
    }

    auto Symbols::get_numeric_leaf_size(const uint8_t* data) -> uint32_t
    {
        const auto leaf = *reinterpret_cast<const uint16_t*>(data);
        if (leaf < static_cast<uint16_t>(PDB::CodeView::TPI::TypeRecordKind::LF_NUMERIC))
        {
            return 0; // The value is the size, so the leaf itself takes no extra space.
        }

        switch (static_cast<PDB::CodeView::TPI::TypeRecordKind>(leaf))
        {
        case PDB::CodeView::TPI::TypeRecordKind::LF_CHAR: return sizeof(int8_t);
        case PDB::CodeView::TPI::TypeRecordKind::LF_SHORT: return sizeof(int16_t);
        case PDB::CodeView::TPI::TypeRecordKind::LF_USHORT: return sizeof(uint16_t);
        case PDB::CodeView::TPI::TypeRecordKind::LF_LONG: return sizeof(int32_t);
        case PDB::CodeView::TPI::TypeRecordKind::LF_ULONG: return sizeof(uint32_t);
        case PDB::CodeView::TPI::TypeRecordKind::LF_QUADWORD: return sizeof(int64_t);
        case PDB::CodeView::TPI::TypeRecordKind::LF_UQUADWORD: return sizeof(uint64_t);
        default: return 0;
        }
    }

auto Symbols::get_field_record_size(const PDB::CodeView::TPI::FieldList* field) -> uint32_t
{
    if (!field) return Constants::INVALID_RECORD_SIZE;
    
    switch (field->kind)
    {
        case PDB::CodeView::TPI::TypeRecordKind::LF_BCLASS:
        {
            const auto& record = field->data.LF_BCLASS;
            const size_t fixed_size = reinterpret_cast<const uint8_t*>(&record.offset) - 
                                     reinterpret_cast<const uint8_t*>(&record);
            
            const uint8_t* offset_ptr = reinterpret_cast<const uint8_t*>(record.offset);
            const uint32_t offset_size = get_numeric_leaf_size(offset_ptr) + Constants::NUMERIC_LEAF_PREFIX_SIZE;
            
            return static_cast<uint32_t>(fixed_size) + offset_size;
        }
        
        case PDB::CodeView::TPI::TypeRecordKind::LF_VFUNCTAB:
            return sizeof(field->data.LF_VFUNCTAB);
            
        case PDB::CodeView::TPI::TypeRecordKind::LF_ENUMERATE:
        {
            const auto& record = field->data.LF_ENUMERATE;
            const size_t fixed_size = reinterpret_cast<const uint8_t*>(&record.value) - 
                                     reinterpret_cast<const uint8_t*>(&record);
            
            const uint8_t* numeric_ptr = reinterpret_cast<const uint8_t*>(record.value);
            const uint32_t value_size = get_numeric_leaf_size(numeric_ptr) + Constants::NUMERIC_LEAF_PREFIX_SIZE;
            
            // Name follows the numeric value
            const char* name_ptr = reinterpret_cast<const char*>(numeric_ptr + value_size);
            const uint32_t name_len = safe_strlen(name_ptr);
            
            if (name_len == 0) return Constants::INVALID_RECORD_SIZE; // Indicates truncated/invalid data
            
            return static_cast<uint32_t>(fixed_size) + value_size + name_len;
        }
        
        case PDB::CodeView::TPI::TypeRecordKind::LF_NESTTYPE:
        {
            const auto& record = field->data.LF_NESTTYPE;
            const size_t fixed_size = reinterpret_cast<const uint8_t*>(&record.name) - 
                                     reinterpret_cast<const uint8_t*>(&record);
            const uint32_t name_len = safe_strlen(record.name);
            
            if (name_len == 0) return Constants::INVALID_RECORD_SIZE;
            
            return static_cast<uint32_t>(fixed_size) + name_len;
        }
        
        case PDB::CodeView::TPI::TypeRecordKind::LF_METHOD:
        {
            const auto& record = field->data.LF_METHOD;
            const size_t fixed_size = reinterpret_cast<const uint8_t*>(&record.name) - 
                                     reinterpret_cast<const uint8_t*>(&record);
            const uint32_t name_len = safe_strlen(record.name);
            
            if (name_len == 0) return Constants::INVALID_RECORD_SIZE;
            
            return static_cast<uint32_t>(fixed_size) + name_len;
        }
        
        case PDB::CodeView::TPI::TypeRecordKind::LF_ONEMETHOD:
        {
            const auto& record = field->data.LF_ONEMETHOD;
            const size_t fixed_size = reinterpret_cast<const uint8_t*>(&record.vbaseoff) - 
                                     reinterpret_cast<const uint8_t*>(&record);
            
            // Check if method is virtual (has vtable offset)
            const uint32_t vtable_offset_size = is_virtual_method(record.attributes.mprop) ? 
                                                Constants::VTABLE_OFFSET_SIZE : 0;
            
            // Name follows the optional vtable offset
            const uint8_t* name_ptr = reinterpret_cast<const uint8_t*>(record.vbaseoff) + vtable_offset_size;
            const uint32_t name_len = safe_strlen(reinterpret_cast<const char*>(name_ptr));
            
            if (name_len == 0) return Constants::INVALID_RECORD_SIZE;
            
            return static_cast<uint32_t>(fixed_size) + vtable_offset_size + name_len;
        }
        
        case PDB::CodeView::TPI::TypeRecordKind::LF_MEMBER:
        {
            const auto& record = field->data.LF_MEMBER;
            const size_t fixed_size = reinterpret_cast<const uint8_t*>(&record.offset) - 
                                     reinterpret_cast<const uint8_t*>(&record);
            
            const uint8_t* numeric_ptr = reinterpret_cast<const uint8_t*>(record.offset);
            const uint32_t offset_size = get_numeric_leaf_size(numeric_ptr) + Constants::NUMERIC_LEAF_PREFIX_SIZE;
            
            // Name follows the numeric offset
            const char* name_ptr = reinterpret_cast<const char*>(numeric_ptr + offset_size);
            const uint32_t name_len = safe_strlen(name_ptr);
            
            if (name_len == 0) return Constants::INVALID_RECORD_SIZE;
            
            return static_cast<uint32_t>(fixed_size) + offset_size + name_len;
        }
        
        case PDB::CodeView::TPI::TypeRecordKind::LF_STMEMBER:
        {
            const auto& record = field->data.LF_STMEMBER;
            const size_t fixed_size = reinterpret_cast<const uint8_t*>(&record.name) - 
                                     reinterpret_cast<const uint8_t*>(&record);
            const uint32_t name_len = safe_strlen(record.name);
            
            if (name_len == 0) return Constants::INVALID_RECORD_SIZE;
            
            return static_cast<uint32_t>(fixed_size) + name_len;
        }
        
        default:
            // Unknown field type - cannot calculate size
            return Constants::INVALID_RECORD_SIZE;
    }
}

auto Symbols::get_class_inheritance_model(const PDB::TPIStream& tpi_stream, uint32_t class_type_index) 
    -> ClassInheritanceModel
{
    // Validate and retrieve the class record
    const PDB::CodeView::TPI::Record* class_record = tpi_stream.GetTypeRecord(class_type_index);
    if (!class_record) {
        return ClassInheritanceModel::Single; // Default for invalid records
    }
    
    // Check if this is actually a class or structure record
    const bool is_class_or_struct = 
        class_record->header.kind == PDB::CodeView::TPI::TypeRecordKind::LF_CLASS ||
        class_record->header.kind == PDB::CodeView::TPI::TypeRecordKind::LF_STRUCTURE;
    
    if (!is_class_or_struct) {
        return ClassInheritanceModel::Single; // Not a class type
    }
    
    // Forward declarations don't have field lists
    if (class_record->data.LF_CLASS.property.fwdref) {
        return ClassInheritanceModel::Single;
    }

    // Retrieve the field list that contains base class information
    const PDB::CodeView::TPI::Record* field_list_record = 
        tpi_stream.GetTypeRecord(class_record->data.LF_CLASS.field);
    
    if (!field_list_record || field_list_record->header.kind != PDB::CodeView::TPI::TypeRecordKind::LF_FIELDLIST) {
        return ClassInheritanceModel::Single; // No field list or invalid field list
    }

    // Parse the field list to count base classes and check for virtual inheritance
    int base_class_count = 0;
    
    const uint8_t* list_data = reinterpret_cast<const uint8_t*>(&field_list_record->data.LF_FIELD.list);
    const uint8_t* list_end = list_data + field_list_record->header.size - Constants::PDB_TYPE_RECORD_HEADER_SIZE;

    // Iterate through all fields in the field list
    while (list_data < list_end)
    {
        // Ensure we have at least enough space for the kind field
        if (!is_valid_pointer_range(list_data, list_end, sizeof(uint16_t))) {
            break; // Corrupted data, stop parsing
        }
        
        const auto* field = reinterpret_cast<const PDB::CodeView::TPI::FieldList*>(list_data);
        const auto kind_val = static_cast<uint16_t>(field->kind);

        // Handle padding records (used for alignment in the PDB format)
        if (is_padding_record(kind_val)) {
            const uint32_t padding_size = get_padding_size(kind_val);
            
            // Validate that we won't go out of bounds
            if (!is_valid_pointer_range(list_data, list_end, padding_size)) {
                break; // Invalid padding, stop parsing
            }
            
            list_data += padding_size;
            continue;
        }

        // Check for virtual base classes (immediately determines Virtual inheritance)
        if (field->kind == PDB::CodeView::TPI::TypeRecordKind::LF_VBCLASS || 
            field->kind == PDB::CodeView::TPI::TypeRecordKind::LF_IVBCLASS) {

            // We can return immediately since virtual inheritance is the "highest" model
            return ClassInheritanceModel::Virtual;
        }
        
        // Count direct base classes
        if (field->kind == PDB::CodeView::TPI::TypeRecordKind::LF_BCLASS) {
            base_class_count++;
        }

        // Calculate the size of this field record to advance to the next one
        const uint32_t record_size = get_field_record_size(field);
        if (record_size == 0) {
            // Unknown field type or error in parsing - stop processing
            // This is safer than potentially reading garbage data
            break;
        }
        
        // Move to the next field record
        const size_t total_field_size = sizeof(field->kind) + record_size;
        
        // Validate before advancing
        if (!is_valid_pointer_range(list_data, list_end, total_field_size)) {
            break; // Would go out of bounds
        }
        
        list_data += total_field_size;
    }

    // Determine inheritance model based on what we found
    // Note: We've already returned Virtual if we found virtual bases
    if (base_class_count > 1) {
        return ClassInheritanceModel::Multiple;
    }
    
    return ClassInheritanceModel::Single;
}

// Helper to determine pointer size based on architecture and pointer type
inline uint32_t get_pointer_size(PDB::CodeView::TPI::TypeIndexKind type, bool is_64bit) {
    using T = PDB::CodeView::TPI::TypeIndexKind;
    
    // Extract the base type value to determine pointer category
    uint32_t type_val = static_cast<uint32_t>(type);
    
    // Near pointers (0x100-0x1FF) - architecture dependent
    if (type_val >= 0x100 && type_val < 0x200) {
        return 2; // 16-bit near pointers
    }
    
    // Far pointers (0x200-0x2FF) - seg:offset
    if (type_val >= 0x200 && type_val < 0x300) {
        return 4; // 16:16 far pointers
    }
    
    // Huge pointers (0x300-0x3FF)
    if (type_val >= 0x300 && type_val < 0x400) {
        return 4; // Huge pointers
    }
    
    // 32-bit near pointers (0x400-0x4FF)
    if (type_val >= 0x400 && type_val < 0x500) {
        return 4;
    }
    
    // 32-bit far pointers (0x500-0x5FF)
    if (type_val >= 0x500 && type_val < 0x600) {
        return 6; // 16:32 far pointers
    }
    
    // 64-bit pointers (0x600-0x6FF)
    if (type_val >= 0x600 && type_val < 0x700) {
        return 8;
    }
    
    // Unknown pointer type - use architecture default
    return is_64bit ? 8 : 4;
}

    static auto find_complete_type_definition(const PDB::TPIStream& tpi_stream, uint32_t forward_decl_index) -> uint32_t
{
    const PDB::CodeView::TPI::Record* forward_record = tpi_stream.GetTypeRecord(forward_decl_index);
    if (!forward_record) return forward_decl_index;
    
    // Only search for class/struct types
    if (forward_record->header.kind != PDB::CodeView::TPI::TypeRecordKind::LF_CLASS &&
        forward_record->header.kind != PDB::CodeView::TPI::TypeRecordKind::LF_STRUCTURE) {
        return forward_decl_index;
    }
    
    // If it's not a forward declaration, return as-is
    if (!forward_record->data.LF_CLASS.property.fwdref) {
        return forward_decl_index;
    }
    
    // Get the name of the forward declaration
    const uint8_t* data = reinterpret_cast<const uint8_t*>(forward_record->data.LF_CLASS.data);
    
    // Skip the numeric leaf (size, which is 0 for forward decl)
    const uint8_t* temp_data = data;
    Symbols::read_numeric(temp_data);  // This advances temp_data
    
    // Now temp_data points to the name
    std::string forward_name(reinterpret_cast<const char*>(temp_data));
    
    // Search through all type records
    uint32_t first_index = tpi_stream.GetFirstTypeIndex();
    uint32_t last_index = tpi_stream.GetLastTypeIndex();
    
    for (uint32_t i = first_index; i <= last_index; ++i) {
        // Skip the forward declaration itself
        if (i == forward_decl_index) continue;
        
        const PDB::CodeView::TPI::Record* candidate = tpi_stream.GetTypeRecord(i);
        if (!candidate) continue;
        
        // Must be the same kind (class or struct)
        if (candidate->header.kind != forward_record->header.kind) continue;
        
        // Check if this is NOT a forward declaration
        if (candidate->data.LF_CLASS.property.fwdref) continue;
        
        // Get the candidate's name
        const uint8_t* candidate_data = reinterpret_cast<const uint8_t*>(candidate->data.LF_CLASS.data);
        
        // Skip the size numeric leaf
        const uint8_t* temp_candidate_data = candidate_data;
        Symbols::read_numeric(temp_candidate_data);
        
        // Compare names
        std::string candidate_name(reinterpret_cast<const char*>(temp_candidate_data));
        if (candidate_name == forward_name) {
            // Found the complete definition!
            return i;
        }
    }
    
    // No complete definition found, return the original
    return forward_decl_index;
}

auto Symbols::get_type_size_impl(const PDB::TPIStream& tpi_stream, uint32_t record_index, bool is_64bit) -> uint32_t
{
    // Handle built-in types
    if (record_index < tpi_stream.GetFirstTypeIndex())
    {
        auto type = static_cast<PDB::CodeView::TPI::TypeIndexKind>(record_index);
        
        // Check if this is a pointer type first (they need special handling)
        uint32_t type_val = static_cast<uint32_t>(type);
        if (type_val >= 0x100) {
            return get_pointer_size(type, is_64bit);
        }
        
        // Non-pointer built-in types
        using T = PDB::CodeView::TPI::TypeIndexKind;
        switch (type)
        {
            // No type/void
            case T::T_NOTYPE:
            case T::T_ABS:
            case T::T_SEGMENT:
            case T::T_VOID:
                return 0;

            // 1-byte types
            case T::T_INT1:
            case T::T_UINT1:
            case T::T_CHAR:
            case T::T_UCHAR:
            case T::T_RCHAR:
            case T::T_BOOL08:
            case T::T_CHAR8:
                return 1;

            // 2-byte types
            case T::T_INT2:
            case T::T_UINT2:
            case T::T_SHORT:
            case T::T_USHORT:
            case T::T_WCHAR:
            case T::T_CHAR16:
            case T::T_BOOL16:
                return 2;

            // 4-byte types
            case T::T_INT4:
            case T::T_UINT4:
            case T::T_LONG:
            case T::T_ULONG:
            case T::T_REAL32:
            case T::T_BOOL32:
            case T::T_BOOL32FF:
            case T::T_CHAR32:
            case T::T_HRESULT:
                return 4;

            // 6-byte types
            case T::T_REAL48:
                return 6;

            // 8-byte types
            case T::T_INT8:
            case T::T_UINT8:
            case T::T_QUAD:
            case T::T_UQUAD:
            case T::T_REAL64:
            case T::T_BOOL64:
            case T::T_CURRENCY:
                return 8;

            // 10-byte types
            case T::T_REAL80:
                return 10;

            // 16-byte types
            case T::T_INT16:
            case T::T_UINT16:
            case T::T_OCT:
            case T::T_UOCT:
            case T::T_REAL128:
                return 16;

            // Complex types (real + imaginary parts)
            case T::T_CPLX32:  return 8;   // 2 * float
            case T::T_CPLX64:  return 16;  // 2 * double
            case T::T_CPLX80:  return 20;  // 2 * extended
            case T::T_CPLX128: return 32;  // 2 * quad

            default:
                // Unknown built-in type
                return 0;
        }
    }

    // Handle complex types from user-defined type records
    const PDB::CodeView::TPI::Record* record = tpi_stream.GetTypeRecord(record_index);
    if (!record) return 0;

    switch (record->header.kind)
    {
    case PDB::CodeView::TPI::TypeRecordKind::LF_CLASS:
    case PDB::CodeView::TPI::TypeRecordKind::LF_STRUCTURE:
    {
        const auto& class_data = record->data.LF_CLASS;
            
        // Get the size from the numeric leaf
        const uint8_t* data = reinterpret_cast<const uint8_t*>(class_data.data);
        const uint8_t* data_copy = data;
        uint32_t size = static_cast<uint32_t>(read_numeric(data_copy));
            
        // If size is 0 or it's a forward declaration, try to find the complete type
        if (size == 0 || class_data.property.fwdref) {
            uint32_t complete_type_index = find_complete_type_definition(tpi_stream, record_index);
                
            if (complete_type_index != record_index) {
                // Found a complete definition, get its size recursively
                const PDB::CodeView::TPI::Record* complete_record = tpi_stream.GetTypeRecord(complete_type_index);
                if (complete_record) {
                    const uint8_t* complete_data = reinterpret_cast<const uint8_t*>(complete_record->data.LF_CLASS.data);
                    const uint8_t* complete_data_copy = complete_data;
                    size = static_cast<uint32_t>(read_numeric(complete_data_copy));
                }
            }
        }
            
        return size;
    }

        case PDB::CodeView::TPI::TypeRecordKind::LF_UNION:
        {
            const uint8_t* data = reinterpret_cast<const uint8_t*>(record->data.LF_UNION.data);
            return static_cast<uint32_t>(read_numeric(data));
        }

        case PDB::CodeView::TPI::TypeRecordKind::LF_POINTER:
        {
            // The pointer attributes contain the size
            uint32_t ptr_size = record->data.LF_POINTER.attr.size;
        
            // If size is 0, fall back to type-based detection
            if (ptr_size == 0) {
                const uint32_t ptr_type = record->data.LF_POINTER.attr.ptrtype;
            
                switch (ptr_type) {
                case 0x00: return 2;  // Near16
                case 0x01: return 4;  // Far16
                case 0x02: return 4;  // Huge16
                case 0x0A: return 4;  // Near32
                case 0x0B: return 6;  // Far32
                case 0x0C: return 8;  // Near64
                default:
                    return is_64bit ? 8 : 4;
                }
            }
        
            return ptr_size;
        }

        case PDB::CodeView::TPI::TypeRecordKind::LF_ARRAY:
        {
        const uint8_t* data = reinterpret_cast<const uint8_t*>(record->data.LF_ARRAY.data);
        const uint8_t* data_copy = data;
        return static_cast<uint32_t>(read_numeric(data_copy));
        }

        case PDB::CodeView::TPI::TypeRecordKind::LF_ENUM:
            return get_type_size(tpi_stream, record->data.LF_ENUM.utype);

        case PDB::CodeView::TPI::TypeRecordKind::LF_MODIFIER:
            return get_type_size(tpi_stream, record->data.LF_MODIFIER.type);

        case PDB::CodeView::TPI::TypeRecordKind::LF_BITFIELD:
            return (record->data.LF_BITFIELD.length + 7) / 8;

        case PDB::CodeView::TPI::TypeRecordKind::LF_PROCEDURE:
            // Function pointers
            return is_64bit ? 8 : 4;

        case PDB::CodeView::TPI::TypeRecordKind::LF_MFUNCTION:
        {
            // Member function pointer size varies by inheritance model
            const uint32_t ptr_size = is_64bit ? 8 : 4;
            const uint32_t class_type_index = record->data.LF_MFUNCTION.classtype;
            
            const ClassInheritanceModel model = get_class_inheritance_model(tpi_stream, class_type_index);

            switch (model) {
                case ClassInheritanceModel::Single:   return ptr_size;
                case ClassInheritanceModel::Multiple: return ptr_size * 2;
                case ClassInheritanceModel::Virtual:  return ptr_size * 3;
                default:                              return ptr_size;
            }
        }

        default:
            return 0;
    }
}

    auto Symbols::get_type_size(const PDB::TPIStream& tpi_stream, uint32_t record_index, bool is_64bit) -> uint32_t
{
    auto cache_key = (record_index << 1) | (is_64bit ? 1 : 0);
        
    auto it = type_size_cache.find(cache_key);
    if (it != type_size_cache.end()) {
        return it->second;
    }
        
    uint32_t size = get_type_size_impl(tpi_stream, record_index, is_64bit);
    type_size_cache[cache_key] = size;
    return size;
}
    

    auto Symbols::get_type_name(const PDB::TPIStream& tpi_stream, uint32_t record_index, bool check_valid, bool is_64bit) -> File::StringType
{
    if (record_index < tpi_stream.GetFirstTypeIndex())
    {
        auto type = static_cast<PDB::CodeView::TPI::TypeIndexKind>(record_index);
        switch (type)
        {
        case PDB::CodeView::TPI::TypeIndexKind::T_NOTYPE:
            return STR("<NO TYPE>");
        case PDB::CodeView::TPI::TypeIndexKind::T_HRESULT:
            return STR("HRESULT");
        case PDB::CodeView::TPI::TypeIndexKind::T_32PHRESULT:
        case PDB::CodeView::TPI::TypeIndexKind::T_64PHRESULT:
            return STR("PHRESULT");
        case PDB::CodeView::TPI::TypeIndexKind::T_VOID:
            return STR("void");
        case PDB::CodeView::TPI::TypeIndexKind::T_32PVOID:
        case PDB::CodeView::TPI::TypeIndexKind::T_64PVOID:
        case PDB::CodeView::TPI::TypeIndexKind::T_PVOID:
            return STR("void*");

        case PDB::CodeView::TPI::TypeIndexKind::T_32PBOOL08:
        case PDB::CodeView::TPI::TypeIndexKind::T_32PBOOL16:
        case PDB::CodeView::TPI::TypeIndexKind::T_32PBOOL32:
        case PDB::CodeView::TPI::TypeIndexKind::T_32PBOOL64:
        case PDB::CodeView::TPI::TypeIndexKind::T_64PBOOL08:
        case PDB::CodeView::TPI::TypeIndexKind::T_64PBOOL16:
        case PDB::CodeView::TPI::TypeIndexKind::T_64PBOOL32:
        case PDB::CodeView::TPI::TypeIndexKind::T_64PBOOL64:
            return STR("bool*");

        case PDB::CodeView::TPI::TypeIndexKind::T_BOOL08:
        case PDB::CodeView::TPI::TypeIndexKind::T_BOOL16:
        case PDB::CodeView::TPI::TypeIndexKind::T_BOOL32:
            return STR("bool");

        case PDB::CodeView::TPI::TypeIndexKind::T_RCHAR:
        case PDB::CodeView::TPI::TypeIndexKind::T_CHAR:
            return STR("char");
        case PDB::CodeView::TPI::TypeIndexKind::T_32PRCHAR:
        case PDB::CodeView::TPI::TypeIndexKind::T_32PCHAR:
        case PDB::CodeView::TPI::TypeIndexKind::T_64PRCHAR:
        case PDB::CodeView::TPI::TypeIndexKind::T_64PCHAR:
        case PDB::CodeView::TPI::TypeIndexKind::T_PRCHAR:
        case PDB::CodeView::TPI::TypeIndexKind::T_PCHAR:
            return STR("char*");

        case PDB::CodeView::TPI::TypeIndexKind::T_UCHAR:
            return STR("uint8");
        case PDB::CodeView::TPI::TypeIndexKind::T_32PUCHAR:
        case PDB::CodeView::TPI::TypeIndexKind::T_64PUCHAR:
        case PDB::CodeView::TPI::TypeIndexKind::T_PUCHAR:
            return STR("uint8*");
        case PDB::CodeView::TPI::TypeIndexKind::T_WCHAR:
            return STR("wchar_t");
        case PDB::CodeView::TPI::TypeIndexKind::T_32PWCHAR:
        case PDB::CodeView::TPI::TypeIndexKind::T_64PWCHAR:
        case PDB::CodeView::TPI::TypeIndexKind::T_PWCHAR:
            return STR("wchar_t*");
        case PDB::CodeView::TPI::TypeIndexKind::T_SHORT:
            return STR("int16");
        case PDB::CodeView::TPI::TypeIndexKind::T_32PSHORT:
        case PDB::CodeView::TPI::TypeIndexKind::T_64PSHORT:
        case PDB::CodeView::TPI::TypeIndexKind::T_PSHORT:
            return STR("int16*");
        case PDB::CodeView::TPI::TypeIndexKind::T_USHORT:
            return STR("uint16");
        case PDB::CodeView::TPI::TypeIndexKind::T_32PUSHORT:
        case PDB::CodeView::TPI::TypeIndexKind::T_64PUSHORT:
        case PDB::CodeView::TPI::TypeIndexKind::T_PUSHORT:
            return STR("uint16*");
        case PDB::CodeView::TPI::TypeIndexKind::T_LONG:
            return STR("int32");
        case PDB::CodeView::TPI::TypeIndexKind::T_32PLONG:
        case PDB::CodeView::TPI::TypeIndexKind::T_64PLONG:
        case PDB::CodeView::TPI::TypeIndexKind::T_PLONG:
            return STR("int32*");
        case PDB::CodeView::TPI::TypeIndexKind::T_ULONG:
            return STR("uint32");
        case PDB::CodeView::TPI::TypeIndexKind::T_32PULONG:
        case PDB::CodeView::TPI::TypeIndexKind::T_64PULONG:
        case PDB::CodeView::TPI::TypeIndexKind::T_PULONG:
            return STR("uint32*");
        case PDB::CodeView::TPI::TypeIndexKind::T_REAL32:
            return STR("float");
        case PDB::CodeView::TPI::TypeIndexKind::T_32PREAL32:
        case PDB::CodeView::TPI::TypeIndexKind::T_64PREAL32:
        case PDB::CodeView::TPI::TypeIndexKind::T_PREAL32:
            return STR("float*");
        case PDB::CodeView::TPI::TypeIndexKind::T_REAL64:
            return STR("double");
        case PDB::CodeView::TPI::TypeIndexKind::T_32PREAL64:
        case PDB::CodeView::TPI::TypeIndexKind::T_64PREAL64:
        case PDB::CodeView::TPI::TypeIndexKind::T_PREAL64:
            return STR("double*");
        case PDB::CodeView::TPI::TypeIndexKind::T_QUAD:
            return STR("int64");
        case PDB::CodeView::TPI::TypeIndexKind::T_32PQUAD:
        case PDB::CodeView::TPI::TypeIndexKind::T_64PQUAD:
        case PDB::CodeView::TPI::TypeIndexKind::T_PQUAD:
            return STR("int64*");
        case PDB::CodeView::TPI::TypeIndexKind::T_UQUAD:
            return STR("uint64");
        case PDB::CodeView::TPI::TypeIndexKind::T_32PUQUAD:
        case PDB::CodeView::TPI::TypeIndexKind::T_64PUQUAD:
        case PDB::CodeView::TPI::TypeIndexKind::T_PUQUAD:
            return STR("uint64*");
        case PDB::CodeView::TPI::TypeIndexKind::T_INT4:
            return STR("int32");
        case PDB::CodeView::TPI::TypeIndexKind::T_32PINT4:
        case PDB::CodeView::TPI::TypeIndexKind::T_64PINT4:
        case PDB::CodeView::TPI::TypeIndexKind::T_PINT4:
            return STR("int32*");
        case PDB::CodeView::TPI::TypeIndexKind::T_UINT4:
            return STR("uint32");
        case PDB::CodeView::TPI::TypeIndexKind::T_32PUINT4:
        case PDB::CodeView::TPI::TypeIndexKind::T_64PUINT4:
        case PDB::CodeView::TPI::TypeIndexKind::T_PUINT4:
            return STR("uint32*");
        default:
            __debugbreak();
            return STR("<UNKNOWN TYPE>");
            break;
        }
    }

    auto* record = tpi_stream.GetTypeRecord(record_index);
    if (!record) return STR("<NONE>");

    switch (record->header.kind)
    {
    case PDB::CodeView::TPI::TypeRecordKind::LF_CLASS:
    case PDB::CodeView::TPI::TypeRecordKind::LF_STRUCTURE: {
        File::StringType name = get_leaf_name(record->data.LF_CLASS.data, record->data.LF_CLASS.lfEasy.kind);
        ParsedTemplateClass parsed = TemplateClassParser::Parse(name);

        if (parsed.class_name == STR("TMap"))
        {
            name = STR("TMap<") + parsed.template_args[0] + STR(", ") + parsed.template_args[1] + STR(">");
        }
        
        // Use ConfigUtil instead of hardcoded list
        if (check_valid && !ConfigUtil::GetValidUDTNames().contains(name)) return STR("void");
        return name;
    }
    case PDB::CodeView::TPI::TypeRecordKind::LF_ENUM:
        return to_string_type(record->data.LF_ENUM.name);
    case PDB::CodeView::TPI::TypeRecordKind::LF_MODIFIER: {
        const auto modifier_attr = record->data.LF_MODIFIER.attr;
        std::vector<File::StringType> modifiers{};

        if (modifier_attr.MOD_const) modifiers.push_back(STR("const"));
        if (modifier_attr.MOD_volatile) modifiers.push_back(STR("volatile"));
        // Note: MOD_unaligned is typically not shown in signatures

        File::StringType modifier_string{};
        for (const auto& modifier : modifiers) {
            modifier_string += modifier + STR(" ");
        }

        return modifier_string + get_type_name(tpi_stream, record->data.LF_MODIFIER.type, check_valid, is_64bit);
    }
    case PDB::CodeView::TPI::TypeRecordKind::LF_POINTER:
    {
        auto underlying_type = get_type_name(tpi_stream, record->data.LF_POINTER.utype, check_valid, is_64bit);
    
        const uint32_t ptr_mode = record->data.LF_POINTER.attr.ptrmode;
    
        // Check ptrmode first (this is what works in your PDB files)
        if (ptr_mode == 0x01) {  // CV_PTR_MODE_REF / CV_PTR_MODE_LVREF
            return underlying_type + STR("&");
        }
        else if (ptr_mode == 0x04) {  // CV_PTR_MODE_RVREF
            return underlying_type + STR("&&");
        }
        else if (ptr_mode == 0x02) {  // CV_PTR_MODE_PMEM
            return underlying_type + STR("::*");
        }
        else if (ptr_mode == 0x03) {  // CV_PTR_MODE_PMFUNC
            return underlying_type + STR("::*");
        }
    
        // Fallback: Check islref/isrref flags (for compatibility with other PDB versions)
        // Only check these if ptrmode is 0 (CV_PTR_MODE_PTR)
        if (ptr_mode == 0x00) {
            if (record->data.LF_POINTER.attr.islref) {
                return underlying_type + STR("&");
            }
            else if (record->data.LF_POINTER.attr.isrref) {
                return underlying_type + STR("&&");
            }
        }
    
        // Handle regular pointers
        File::StringType result = underlying_type + STR("*");
    
        // Check if the pointer itself is const (T* const)
        if (record->data.LF_POINTER.attr.isconst) {
            result += STR(" const");
        }
    
        if (record->data.LF_POINTER.attr.isvolatile) {
            result += STR(" volatile");
        }

        return result;
    }
    case PDB::CodeView::TPI::TypeRecordKind::LF_MFUNCTION:
    case PDB::CodeView::TPI::TypeRecordKind::LF_PROCEDURE: {
        File::StringType return_type = get_type_name(tpi_stream, record->data.LF_PROCEDURE.rvtype, true, is_64bit);
        File::StringType args = get_type_name(tpi_stream, record->data.LF_PROCEDURE.arglist, check_valid, is_64bit);
        return std::format(STR("std::function<{}({})>"), return_type, args);
    }
    case PDB::CodeView::TPI::TypeRecordKind::LF_ARGLIST: {
        File::StringType args{};

        for (size_t i = 0; i < record->data.LF_ARGLIST.count; i++)
        {
            bool should_add_comma = i < record->data.LF_ARGLIST.count - 1;
            args.append(std::format(STR("{}{}"), get_type_name(tpi_stream, record->data.LF_ARGLIST.arg[i], true, is_64bit), should_add_comma ? STR(", ") : STR("")));
        }

        return args;
    }
    case PDB::CodeView::TPI::TypeRecordKind::LF_BITFIELD:
        return get_type_name(tpi_stream, record->data.LF_BITFIELD.type, check_valid, is_64bit);
    case PDB::CodeView::TPI::TypeRecordKind::LF_ARRAY:
    {
        // Get the element type name
        File::StringType element_type = get_type_name(tpi_stream, record->data.LF_ARRAY.elemtype, check_valid, is_64bit);
    
        // Get the total array size in bytes from the numeric leaf
        // Note: read_numeric takes a reference to the pointer and advances it
        const uint8_t* data_ptr = reinterpret_cast<const uint8_t*>(record->data.LF_ARRAY.data);
        uint64_t array_size_bytes = read_numeric(data_ptr);
    
        // Get the size of a single element
        uint32_t element_size = get_type_size(tpi_stream, record->data.LF_ARRAY.elemtype, is_64bit);
    
        // Calculate the number of elements
        if (element_size > 0 && array_size_bytes > 0) {
            uint64_t element_count = array_size_bytes / element_size;
        
            // Return formatted array type with dimensions
            return std::format(STR("{}[{}]"), element_type, element_count);
        } else {
            if (element_size == 0) {
                printf("  WARNING: Element size is 0 - type might be incomplete or forward declaration\n");
            }
            if (array_size_bytes == 0) {
                printf("  WARNING: Array size is 0 - numeric leaf might not be read correctly\n");
            }
            // If we can't determine the size, just indicate it's an array
            return element_type + STR("[]");
        }
    }
    default:
        __debugbreak();
        return STR("<UNKNOWN TYPE>");
    }
}

    auto Symbols::get_method_name(const PDB::CodeView::TPI::FieldList* method_record) -> File::StringType
    {
        auto methodAttributes = static_cast<PDB::CodeView::TPI::MethodProperty>(method_record->data.LF_ONEMETHOD.attributes.mprop);
        switch (methodAttributes)
        {
        case PDB::CodeView::TPI::MethodProperty::Intro:
        case PDB::CodeView::TPI::MethodProperty::PureIntro:
            return to_string_type(&reinterpret_cast<const char*>(method_record->data.LF_ONEMETHOD.vbaseoff)[sizeof(uint32_t)]);
        default:
            break;
        }

        return to_string_type(&reinterpret_cast<const char*>(method_record->data.LF_ONEMETHOD.vbaseoff)[0]);
    }

    auto static get_leaf_size(PDB::CodeView::TPI::TypeRecordKind kind) -> size_t
    {
        if (kind < PDB::CodeView::TPI::TypeRecordKind::LF_NUMERIC)
        {
            // No leaf can have an index less than LF_NUMERIC (0x8000)
            // so word is the value...
            return sizeof(PDB::CodeView::TPI::TypeRecordKind);
        }

        switch (kind)
        {
        case PDB::CodeView::TPI::TypeRecordKind::LF_CHAR:
            return sizeof(PDB::CodeView::TPI::TypeRecordKind) + sizeof(uint8_t);

        case PDB::CodeView::TPI::TypeRecordKind::LF_USHORT:
        case PDB::CodeView::TPI::TypeRecordKind::LF_SHORT:
            return sizeof(PDB::CodeView::TPI::TypeRecordKind) + sizeof(uint16_t);

        case PDB::CodeView::TPI::TypeRecordKind::LF_LONG:
        case PDB::CodeView::TPI::TypeRecordKind::LF_ULONG:
            return sizeof(PDB::CodeView::TPI::TypeRecordKind) + sizeof(uint32_t);

        case PDB::CodeView::TPI::TypeRecordKind::LF_QUADWORD:
        case PDB::CodeView::TPI::TypeRecordKind::LF_UQUADWORD:
            return sizeof(PDB::CodeView::TPI::TypeRecordKind) + sizeof(uint64_t);
        }
        return 0;
    }

    auto Symbols::get_leaf_name(const char* data, PDB::CodeView::TPI::TypeRecordKind kind) -> File::StringType
    {
        auto name = to_string_type(&data[get_leaf_size(kind)]);
        return name;
    }

    auto Symbols::get_leaf_name(const File::StringType& class_name, const char* data, PDB::CodeView::TPI::TypeRecordKind kind) -> File::StringType
    {
        auto original_name = to_string_type(&data[get_leaf_size(kind)]);
    
        // Check if there is a special mapping for this member in this class
        auto info = ConfigUtil::GetMemberRenameInfo(class_name, original_name);
        if (info.has_value()) {
            return info->mapped_name;
        }
    
        return original_name;
    }

    auto Symbols::clean_name(File::StringType name) -> File::StringType
    {
        std::replace(name.begin(), name.end(), ':', '_');
        std::replace(name.begin(), name.end(), '~', '$');

        return name;
    }

    auto Symbols::is_virtual(PDB::CodeView::TPI::MemberAttributes attributes) -> bool
    {
        return attributes.mprop == (uint16_t)PDB::CodeView::TPI::MethodProperty::Intro ||
               attributes.mprop == (uint16_t)PDB::CodeView::TPI::MethodProperty::PureIntro;
    }

    auto Symbols::get_bitfield_info(const PDB::TPIStream& tpi_stream, uint32_t record_index) -> BitfieldInfo
    {
        BitfieldInfo info{};

        // Built-in types are never bitfields
        if (record_index < tpi_stream.GetFirstTypeIndex())
        {
            return info;
        }

        const auto* record = tpi_stream.GetTypeRecord(record_index);
        if (!record)
        {
            return info;
        }

        if (record->header.kind == PDB::CodeView::TPI::TypeRecordKind::LF_BITFIELD)
        {
            info.is_bitfield = true;
            info.bit_length = record->data.LF_BITFIELD.length;
            info.bit_position = record->data.LF_BITFIELD.position;
        }

        return info;
    }
} // namespace RC::UVTD