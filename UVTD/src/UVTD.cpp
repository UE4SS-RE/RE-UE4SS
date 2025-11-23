#define NOMINMAX

#include <format>
#include <numeric>
#include <stdexcept>
#include <thread>
#include <unordered_set>

#include <DynamicOutput/DynamicOutput.hpp>
#include <Helpers/String.hpp>
#include <Input/Handler.hpp>
#include <UVTD/Config.hpp>
#include <UVTD/ConfigUtil.hpp>
#include <UVTD/ExceptionHandling.hpp>
#include <UVTD/Helpers.hpp>
#include <UVTD/MemberVarsDumper.hpp>
#include <UVTD/MemberVarsWrapperGenerator.hpp>
#include <UVTD/SolBindingsGenerator.hpp>
#include <UVTD/UVTD.hpp>
#include <UVTD/UnrealVirtualGenerator.hpp>
#include <UVTD/VTableDumper.hpp>

#include <Psapi.h>
#include <Windows.h>
#include <dbghelp.h>

#include <String/StringType.hpp>

namespace RC::UVTD
{
    bool processing_events{false};

    auto main(DumpSettings dump_settings) -> void
    {
        UnrealVirtualGenerator::output_cleanup();

        if (dump_settings.should_dump_vtable) VTableDumper::output_cleanup();
        if (dump_settings.should_dump_member_vars) MemberVarsDumper::output_cleanup();
        if (dump_settings.should_dump_sol_bindings) SolBindingsGenerator::output_cleanup();

        TypeContainer shared_container{};

        // Use ConfigUtil instead of hardcoded list
        const auto& pdbs_to_dump = ConfigUtil::GetPDBsToDump();
        if (pdbs_to_dump.empty()) 
        {
            Output::send(STR("No PDBs configured to dump. Please check your pdbs_to_dump.json configuration.\n"));
            return;
        }

        for (const std::filesystem::path& pdb : pdbs_to_dump)
        {
            TRY([&] {
                {
                    TypeContainer run_container{};

                    if (dump_settings.should_dump_vtable)
                    {
                        Symbols symbols{pdb};

                        VTableDumper dumper{std::move(symbols)};
                        dumper.generate_code();
                        dumper.generate_files();

                        run_container.join(dumper.get_type_container());
                    }

                    if (dump_settings.should_dump_member_vars)
                    {
                        Symbols symbols{pdb};

                        MemberVarsDumper dumper{std::move(symbols)};
                        dumper.generate_code();
                        dumper.generate_files();

                        run_container.join(dumper.get_type_container());
                    }

                    if (dump_settings.should_dump_sol_bindings)
                    {
                        Symbols symbols{pdb};

                        SolBindingsGenerator generator{std::move(symbols)};
                        generator.generate_code();
                        generator.generate_files();
                    }

                    File::StringType pdb_name = pdb.filename().stem();
                    UnrealVirtualGenerator virtual_generator(pdb_name, run_container);
                    virtual_generator.generate_files();

                    shared_container.join(run_container);

                    Output::send(STR("Code generated.\n"));
                }
            });
        }

        if (dump_settings.should_dump_member_vars)
        {
            MemberVarsWrapperGenerator::output_cleanup();
            MemberVarsWrapperGenerator wrapper_generator{std::move(shared_container)};
            wrapper_generator.generate_files();

            Output::send(STR("Code generated.\n"));
        }

        Output::send(STR("All done.\n"));
    }
} // namespace RC::UVTD