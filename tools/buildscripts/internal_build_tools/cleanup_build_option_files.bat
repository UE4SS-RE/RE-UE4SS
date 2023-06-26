cd generated_src
if exist %enable_with_case_preserving_file% (
    del "%enable_with_case_preserving_file%"
)
if exist %enable_build_as_xinput_file% (
    del "%enable_build_as_xinput_file%"
)
if exist %disable_with_case_preserving_file% (
    del "%disable_with_case_preserving_file%"
)
if exist %disable_build_as_xinput_file% (
    del "%disable_build_as_xinput_file%"
)
cd ..