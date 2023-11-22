#![allow(unused)]

use std::{error::Error, sync::Arc, time::Instant};

use patternsleuth::resolvers::{
    futures::join,
    impl_collector,
    unreal::{
        engine_version::EngineVersion,
        fname::{FNameCtorWchar, FNameToString},
        ftext::FTextFString,
        gmalloc::GMalloc,
        guobject_array::GUObjectArray,
        static_construct_object::StaticConstructObjectInternal,
    },
    ResolveError,
};

impl_collector! {
    #[derive(Debug, PartialEq)]
    struct UE4SSResolution {
        guobject_array: GUObjectArray,
        fname_tostring: FNameToString,
        fname_ctor_wchar: FNameCtorWchar,
        gmalloc: GMalloc,
        static_construct_object_internal: StaticConstructObjectInternal,
        ftext_fstring: FTextFString,
        engine_version: EngineVersion,
    }
}

#[repr(C)]
pub struct LogLevel(extern "C" fn(*const u16));
impl LogLevel {
    fn log(&self, msg: impl AsRef<str>) {
        let wchar: Vec<u16> = msg.as_ref().encode_utf16().chain([0]).collect();
        (self.0)(wchar.as_ptr())
    }
}

#[repr(C)]
pub struct PsCtx {
    default: LogLevel,
    normal: LogLevel,
    verbose: LogLevel,
    warning: LogLevel,
    error: LogLevel,
    config: PsScanConfig,
}

macro_rules! _log_level {
    ($level:ident, $ctx:ident) => { $ctx.$level.log("") };
    ($level:ident, $ctx:ident, $($arg:tt)*) => { $ctx.$level.log(format!($($arg)*)) };
}
macro_rules! default { ($ctx:ident $($arg:tt)*) => { _log_level!(default, $ctx $($arg)*) }; }
macro_rules! normal { ($ctx:ident $($arg:tt)*) => { _log_level!(normal, $ctx $($arg)*) }; }
macro_rules! verbose { ($ctx:ident $($arg:tt)*) => { _log_level!(verbose, $ctx $($arg)*) }; }
macro_rules! warning { ($ctx:ident $($arg:tt)*) => { _log_level!(warning, $ctx $($arg)*) }; }
macro_rules! error { ($ctx:ident $($arg:tt)*) => { _log_level!(error, $ctx $($arg)*) }; }

#[repr(C)]
pub struct PsEngineVersion {
    major: u16,
    minor: u16,
}

#[repr(C)]
pub struct PsScanConfig {
    guobject_array: bool,
    fname_tostring: bool,
    fname_ctor_wchar: bool,
    gmalloc: bool,
    static_construct_object_internal: bool,
    ftext_fstring: bool,
    engine_version: bool,
}

#[repr(C)]
pub struct PsScanResults {
    guobject_array: usize,
    fname_tostring: usize,
    fname_ctor_wchar: usize,
    gmalloc: usize,
    static_construct_object_internal: usize,
    ftext_fstring: usize,
    engine_version: PsEngineVersion,
}

#[derive(Debug, Default)]
struct ScanErrors(Vec<Box<dyn Error>>);
impl std::error::Error for ScanErrors {}
impl std::fmt::Display for ScanErrors {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "ScanError")
    }
}

pub fn ps_scan_internal(ctx: &PsCtx, results: &mut PsScanResults) -> Result<(), Box<dyn Error>> {
    default!(ctx, "Reading image");

    let exe = patternsleuth::process::internal::read_image()?;

    default!(ctx, "Starting scan");
    let before = Instant::now();
    let resolution = exe.resolve(UE4SSResolution::resolver())?;
    default!(ctx, "Scan finished in {:?}", before.elapsed());

    let mut errors = ScanErrors::default();

    macro_rules! handle {
        ($member:ident, $name:literal, $lua:literal) => {
            if ctx.config.$member {
                match resolution.$member {
                    Ok(res) => {
                        default!(ctx, "Found {}: 0x{:x?}", $name, res.0);
                        results.$member = res.0;
                    }
                    Err(err) => {
                        warning!(ctx, "Failed to find {}: {err}", $name);
                        warning!(
                            ctx,
                            "You can supply your own AOB in 'UE4SS_Signatures/{}'",
                            $lua
                        );
                        errors.0.push(Box::new(err));
                    }
                }
            }
        };
    }
    if ctx.config.engine_version {
        match resolution.engine_version {
            Ok(res) => {
                default!(ctx, "Found EngineVersion: {}", res);
                results.engine_version.major = res.major;
                results.engine_version.minor = res.minor;
            }
            Err(err) => {
                warning!(ctx, "Failed to find EngineVersion: {err}");
                warning!(
                    ctx,
                    "You need to override the engine version in 'UE4SS-settings.ini'."
                );
                errors.0.push(Box::new(err));
            }
        }
    }
    handle!(guobject_array, "GUObjectArray", "GUObjectArray.lua");
    handle!(gmalloc, "GMalloc", "GMalloc.lua");
    handle!(fname_tostring, "FName::ToString", "FName_ToString.lua");
    handle!(
        fname_ctor_wchar,
        "FName::FName(wchar_t*)",
        "FName_Constructor.lua"
    );
    handle!(
        static_construct_object_internal,
        "StaticConstructObject_Internal",
        "StaticConstructObject.lua"
    );
    handle!(
        ftext_fstring,
        "FText::FText(FString&&)",
        "FText_Constructor.lua"
    );

    if errors.0.is_empty() {
        Ok(())
    } else {
        Err(Box::new(errors))
    }
}

#[no_mangle]
pub extern "C" fn ps_scan(ctx: &PsCtx, results: &mut PsScanResults) -> bool {
    if let Err(_err) = ps_scan_internal(ctx, results) {
        warning!(ctx, "Scan failed\n");
        false
    } else {
        true
    }
}
