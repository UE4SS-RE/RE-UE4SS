-- Satisfactory 1.0, UE 5.3
--return LoadExport("?GMalloc@@3PEAVFMalloc@@EA")

-- Satisfactory 1.2, UE 5.6
-- class FMalloc * __ptr64 __ptr64 UE::Private::GMalloc
return LoadExport("?GMalloc@Private@UE@@3PEAVFMalloc@@EA")
