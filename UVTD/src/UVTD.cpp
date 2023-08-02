#define NOMINMAX

#include <format>
#include <numeric>
#include <stdexcept>
#include <thread>
#include <unordered_set>

#include <DynamicOutput/DynamicOutput.hpp>
#include <Helpers/String.hpp>
#include <Input/Handler.hpp>
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

namespace RC::UVTD
{
    bool processing_events{false};
    Input::Handler input_handler{L"ConsoleWindowClass", L"UnrealWindow"};

    auto static event_loop_update() -> void
    {
        for (processing_events = true; processing_events;)
        {
            input_handler.process_event();
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    auto main(DumpSettings dump_settings) -> void
    {
        static std::vector<std::filesystem::path> pdbs_to_dump{
                "PDBs/4_11.pdb",
                "PDBs/4_12.pdb",
                "PDBs/4_13.pdb",
                "PDBs/4_14.pdb",
                "PDBs/4_15.pdb",
                "PDBs/4_16.pdb",
                "PDBs/4_17.pdb",
                "PDBs/4_18.pdb",
                "PDBs/4_19.pdb",
                "PDBs/4_20.pdb",
                "PDBs/4_21.pdb",
                "PDBs/4_22.pdb",
                "PDBs/4_23.pdb",
                "PDBs/4_24.pdb",
                "PDBs/4_25.pdb",
                "PDBs/4_26.pdb",
                "PDBs/4_27.pdb",
                // WITH_CASE_PRESERVING_NAMES
                "PDBs/4_27_CasePreserving.pdb",
                "PDBs/5_00.pdb",
                "PDBs/5_01.pdb",
                "PDBs/5_02.pdb",
        };

        UnrealVirtualGenerator::output_cleanup();

        if (dump_settings.should_dump_vtable) VTableDumper::output_cleanup();
        if (dump_settings.should_dump_member_vars) MemberVarsDumper::output_cleanup();
        if (dump_settings.should_dump_sol_bindings) SolBindingsGenerator::output_cleanup();

        TypeContainer shared_container{};

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
        }

        CoUninitialize();
    }
} // namespace RC::UVTD
