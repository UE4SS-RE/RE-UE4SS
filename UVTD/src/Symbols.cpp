#include <format>
#include <algorithm>

#include <UVTD/Symbols.hpp>
#include <UVTD/Helpers.hpp>
#include <UVTD/TemplateClassParser.hpp>
#include <File/File.hpp>

#include <PDB.h>

namespace RC::UVTD
{
    Symbols::Symbols(std::filesystem::path pdb_file_path) :
        pdb_file_path(pdb_file_path),
        pdb_file_handle(std::move(File::open(pdb_file_path))),
        pdb_file_map(std::move(pdb_file_handle.memory_map())),
        pdb_file(pdb_file_map.data())
    {
        auto version_string = this->pdb_file_path.filename().stem().string();
        auto major_version = std::atoi(version_string.substr(0, 1).c_str());
        auto minor_version = std::atoi(version_string.substr(2).c_str());
        is_425_plus = (major_version > 4) || (major_version == 4 && minor_version >= 25);

        if (!std::filesystem::exists(pdb_file_path))
        {
            throw std::runtime_error{ std::format("PDB '{}' not found", pdb_file_path.string()) };
        }

        if (PDB::HasValidDBIStream(pdb_file) != PDB::ErrorCode::Success)
        {
            throw std::runtime_error{ std::format("PDB '{}' doesn't contain a valid DBI stream", pdb_file_path.string()) };
        }

        if (PDB::HasValidTPIStream(pdb_file) != PDB::ErrorCode::Success)
        {
            throw std::runtime_error{ std::format("PDB '{}' doesn't contain a valid TPI stream", pdb_file_path.string()) };
        }

        dbi_stream = PDB::CreateDBIStream(pdb_file);
    }

    Symbols::Symbols(const Symbols& other) :
        pdb_file_path(other.pdb_file_path),
        pdb_file_handle(std::move(File::open(pdb_file_path))),
        pdb_file_map(std::move(pdb_file_handle.memory_map())),
        pdb_file(pdb_file_map.data()),
        is_425_plus(other.is_425_plus)
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
        dbi_stream = PDB::CreateDBIStream(pdb_file);

        return *this;
    }

    auto Symbols::generate_method_signature(const PDB::TPIStream& tpi_stream, const PDB::CodeView::TPI::Record* function_record, File::StringType method_name) -> MethodSignature
    {
        MethodSignature signature{};

        auto this_pointer = tpi_stream.GetTypeRecord(function_record->data.LF_MFUNCTION.thistype);

        signature.name = method_name;
        signature.const_qualifier = this_pointer != nullptr ? this_pointer->data.LF_POINTER.attr.isconst : false;

        auto arg_list = tpi_stream.GetTypeRecord(function_record->data.LF_MFUNCTION.arglist);
        for (size_t i = 0; i < function_record->data.LF_MFUNCTION.parmcount; i++)
        {
            auto argument = Symbols::get_type_name(tpi_stream, arg_list->data.LF_ARGLIST.arg[i]);
            signature.params.push_back(FunctionParam{
                .type = argument
            });
        }

        if (function_record->data.LF_MFUNCTION.rvtype)
        {
            signature.return_type = Symbols::get_type_name(tpi_stream, function_record->data.LF_MFUNCTION.rvtype);
        }

        return signature;
    }

    auto Symbols::get_type_name(const PDB::TPIStream& tpi_stream, uint32_t record_index, bool check_valid) -> File::StringType
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
        case PDB::CodeView::TPI::TypeRecordKind::LF_STRUCTURE:
        {
            File::StringType name = get_leaf_name(record->data.LF_CLASS.data, record->data.LF_CLASS.lfEasy.kind);
            ParsedTemplateClass parsed = TemplateClassParser::Parse(name);

            if (parsed.class_name == STR("TMap"))
            {
                name = STR("TMap<") + parsed.template_args[0] + STR(", ") + parsed.template_args[1] + STR(">");
            }
            if (check_valid && !valid_udt_names.contains(name)) return STR("void");
            return name;
        }
        case PDB::CodeView::TPI::TypeRecordKind::LF_ENUM:
            return to_string_type(record->data.LF_ENUM.name);
        case PDB::CodeView::TPI::TypeRecordKind::LF_MODIFIER:
        {
            const auto modifier_attr = record->data.LF_MODIFIER.attr;
            std::vector<File::StringType> modifiers{};

            if (modifier_attr.MOD_volatile)
                modifiers.push_back(STR("volatile"));

            File::StringType modifier_string{};
            for (const auto& modifier : modifiers)
            {
                modifier_string += modifier + STR(" ");
            }

            return modifier_string + get_type_name(tpi_stream, record->data.LF_MODIFIER.type, check_valid);
        }
        case PDB::CodeView::TPI::TypeRecordKind::LF_POINTER: 
            return get_type_name(tpi_stream, record->data.LF_POINTER.utype, check_valid) + STR("*");
        case PDB::CodeView::TPI::TypeRecordKind::LF_MFUNCTION:
        case PDB::CodeView::TPI::TypeRecordKind::LF_PROCEDURE:
        {
            File::StringType return_type = get_type_name(tpi_stream, record->data.LF_PROCEDURE.rvtype, true);
            File::StringType args = get_type_name(tpi_stream, record->data.LF_PROCEDURE.arglist, check_valid);
            return std::format(STR("std::function<{}({})>"), return_type, args);
        }
        case PDB::CodeView::TPI::TypeRecordKind::LF_ARGLIST:
        {
            File::StringType args{};

            for (size_t i = 0; i < record->data.LF_ARGLIST.count; i++)
            {
                bool should_add_comma = i < record->data.LF_ARGLIST.count - 1;
                args.append(std::format(STR("{}{}"), get_type_name(tpi_stream, record->data.LF_ARGLIST.arg[i], true), should_add_comma ? STR(", ") : STR("")));
            }

            return args;
        }
        case PDB::CodeView::TPI::TypeRecordKind::LF_BITFIELD:
            return get_type_name(tpi_stream, record->data.LF_BITFIELD.type, check_valid);
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
        return to_string_type(&data[get_leaf_size(kind)]);
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
}