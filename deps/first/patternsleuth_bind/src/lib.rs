#![allow(unused)]

use std::{error::Error, sync::Arc, time::Instant};

use patternsleuth::resolvers::{
    futures::join,
    impl_collector,
    unreal::{
        engine_version::EngineVersion,
        fname::{FNameCtorWchar, FNameToString},
        ftext::FTextFString,
        fuobject_hash_tables::FUObjectHashTablesGet,
        game_loop::UGameEngineTick,
        gmalloc::GMalloc,
        guobject_array::GUObjectArray,
        kismet::GNatives,
        static_construct_object::StaticConstructObjectInternal,
        ConsoleManagerSingleton,
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
        fuobject_hash_tables_get: FUObjectHashTablesGet,
        gnatives: GNatives,
        console_manager_singleton: ConsoleManagerSingleton,
        gameengine_tick: UGameEngineTick,
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
    fuobject_hash_tables_get: bool,
    gnatives: bool,
    console_manager_singleton: bool,
    gameengine_tick: bool,
}

#[repr(C)]
pub struct PsScanResults {
    guobject_array: u64,
    fname_tostring: u64,
    fname_ctor_wchar: u64,
    gmalloc: u64,
    static_construct_object_internal: u64,
    ftext_fstring: u64,
    engine_version: PsEngineVersion,
    fuobject_hash_tables_get: u64,
    gnatives: u64,
    console_manager_singleton: u64,
    gameengine_tick: u64,
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
            handle!($member, $name, $lua, false);
        };
        ($member:ident, $name:literal, $lua:literal, $optional:expr) => {
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
                        results.$member = 0;
                        // Only add to `errors` if it's not optional:
                        if !$optional {
                            errors.0.push(Box::new(err));
                        }
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
    handle!(
        fname_tostring,
        "FName::ToString",
        "FName_ToString.lua",
        true
    );
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
        "FText_Constructor.lua",
        true
    );

    handle!(
        fuobject_hash_tables_get,
        "FUObjectHashTables::Get()",
        "GUObjectHashTables.lua",
        true
    );

    handle!(gnatives, "GNatives", "GNatives.lua", true);

    handle!(
        console_manager_singleton,
        "ConsoleManagerSingleton",
        "ConsoleManager.lua",
        true
    );

    handle!(
        gameengine_tick,
        "GameEngineTick",
        "GameEngineTick.lua",
        true
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

#[cfg(target_os = "linux")]
mod linux_v1 {
    use std::panic::{catch_unwind, AssertUnwindSafe};

    use patternsleuth::image::Image;

    use patternsleuth::resolvers::unreal::{
        blueprint_library::UFunctionBind,
        engine_version::EngineVersion,
        fname::{FNameCtorWchar, FNameToString},
        game_loop::{FEngineLoopInit, UGameEngineTick},
        gmalloc::GMalloc,
        guobject_array::{
            FUObjectArrayAllocateUObjectIndex, FUObjectArrayFreeUObjectIndex, GUObjectArray,
        },
        static_construct_object::StaticConstructObjectInternal,
        static_find_object::StaticFindObjectFast,
    };

    const ABI_VERSION: u32 = 1;
    const INFO: u32 = 0;
    const WARNING: u32 = 1;
    const ERROR: u32 = 2;

    const GUOBJECT_ARRAY: u64 = 1 << 0;
    const FNAME_TO_STRING: u64 = 1 << 1;
    const FNAME_CTOR: u64 = 1 << 2;
    const GMALLOC: u64 = 1 << 3;
    const STATIC_CONSTRUCT_OBJECT: u64 = 1 << 4;
    const STATIC_FIND_OBJECT: u64 = 1 << 5;
    const GAME_ENGINE_TICK: u64 = 1 << 6;
    const ENGINE_LOOP_INIT: u64 = 1 << 7;
    const UFUNCTION_BIND: u64 = 1 << 8;
    const ALLOCATE_UOBJECT_INDEX: u64 = 1 << 9;
    const FREE_UOBJECT_INDEX: u64 = 1 << 10;
    const ENGINE_VERSION: u64 = 1 << 11;

    #[repr(C)]
    pub struct Config {
        struct_size: u32,
        abi_version: u32,
        enabled_mask: u64,
        log: Option<extern "C" fn(u32, *const u8, usize, *mut std::ffi::c_void)>,
        user_data: *mut std::ffi::c_void,
    }

    #[repr(C)]
    pub struct Results {
        struct_size: u32,
        abi_version: u32,
        available_mask: u64,
        failed_mask: u64,
        engine_major: u16,
        engine_minor: u16,
        reserved: u32,
        guobject_array: u64,
        fname_to_string: u64,
        fname_ctor: u64,
        gmalloc: u64,
        static_construct_object: u64,
        static_find_object: u64,
        game_engine_tick: u64,
        engine_loop_init: u64,
        ufunction_bind: u64,
        allocate_uobject_index: u64,
        free_uobject_index: u64,
    }

    fn log(config: &Config, level: u32, message: impl AsRef<str>) {
        if let Some(callback) = config.log {
            let message = message.as_ref().as_bytes();
            callback(level, message.as_ptr(), message.len(), config.user_data);
        }
    }

    unsafe extern "C" fn find_main_image(
        info: *mut libc::dl_phdr_info,
        _size: usize,
        output: *mut std::ffi::c_void,
    ) -> i32 {
        if info.is_null() {
            return 0;
        }
        let name = unsafe { (*info).dlpi_name };
        if name.is_null() || unsafe { *name } == 0 {
            unsafe { *(output as *mut u64) = (*info).dlpi_addr };
            return 1;
        }
        0
    }

    fn read_linux_image<'data>(
        file_data: &'data [u8],
    ) -> Result<(Image<'data>, u64), Box<dyn std::error::Error>> {
        let mut base_address = 0u64;
        unsafe {
            libc::dl_iterate_phdr(
                Some(find_main_image),
                &mut base_address as *mut u64 as *mut std::ffi::c_void,
            );
        }
        let executable = std::path::Path::new("/proc/self/exe");
        let image = Image::read(None, file_data, Some(executable), true)?;
        Ok((image, base_address))
    }

    fn scan_image(
        config: &Config,
        results: &mut Results,
        image: &Image<'_>,
        load_bias: u64,
    ) -> Result<(), Box<dyn std::error::Error>> {
        macro_rules! address_resolver {
            ($mask:ident, $field:ident, $resolver:ty) => {
                if config.enabled_mask & $mask != 0 {
                    match image.resolve(<$resolver>::resolver()) {
                        Ok(value) => {
                            results.$field = value
                                .0
                                .checked_add(load_bias)
                                .ok_or("resolver address overflow")?;
                            results.available_mask |= $mask;
                            log(
                                config,
                                INFO,
                                format!(
                                    "patternsleuth: {}=0x{:x}",
                                    stringify!($field),
                                    results.$field
                                ),
                            );
                        }
                        Err(error) => {
                            results.failed_mask |= $mask;
                            log(
                                config,
                                WARNING,
                                format!(
                                    "patternsleuth: {} unavailable: {error}",
                                    stringify!($field)
                                ),
                            );
                        }
                    }
                }
            };
        }

        address_resolver!(GUOBJECT_ARRAY, guobject_array, GUObjectArray);
        address_resolver!(FNAME_TO_STRING, fname_to_string, FNameToString);
        address_resolver!(FNAME_CTOR, fname_ctor, FNameCtorWchar);
        address_resolver!(GMALLOC, gmalloc, GMalloc);
        address_resolver!(
            STATIC_CONSTRUCT_OBJECT,
            static_construct_object,
            StaticConstructObjectInternal
        );
        address_resolver!(STATIC_FIND_OBJECT, static_find_object, StaticFindObjectFast);
        address_resolver!(GAME_ENGINE_TICK, game_engine_tick, UGameEngineTick);
        address_resolver!(ENGINE_LOOP_INIT, engine_loop_init, FEngineLoopInit);
        address_resolver!(UFUNCTION_BIND, ufunction_bind, UFunctionBind);
        address_resolver!(
            ALLOCATE_UOBJECT_INDEX,
            allocate_uobject_index,
            FUObjectArrayAllocateUObjectIndex
        );
        address_resolver!(
            FREE_UOBJECT_INDEX,
            free_uobject_index,
            FUObjectArrayFreeUObjectIndex
        );

        if config.enabled_mask & ENGINE_VERSION != 0 {
            match image.resolve(EngineVersion::resolver()) {
                Ok(version) => {
                    results.engine_major = version.major;
                    results.engine_minor = version.minor;
                    results.available_mask |= ENGINE_VERSION;
                    log(
                        config,
                        INFO,
                        format!("patternsleuth: engine_version={version}"),
                    );
                }
                Err(error) => {
                    results.failed_mask |= ENGINE_VERSION;
                    log(
                        config,
                        WARNING,
                        format!("patternsleuth: engine_version unavailable: {error}"),
                    );
                }
            }
        }
        Ok(())
    }

    fn scan(config: &Config, results: &mut Results) -> Result<(), Box<dyn std::error::Error>> {
        log(
            config,
            INFO,
            "patternsleuth: reading the in-process ELF image",
        );
        let executable_data = std::fs::read("/proc/self/exe")?;
        let (image, load_bias) = read_linux_image(&executable_data)?;
        scan_image(config, results, &image, load_bias)
    }

    fn validate_and_reset(config: &Config, results: &mut Results) -> Result<(), i32> {
        if config.struct_size < std::mem::size_of::<Config>() as u32
            || results.struct_size < std::mem::size_of::<Results>() as u32
            || config.abi_version != ABI_VERSION
        {
            return Err(libc::EPROTO);
        }
        results.abi_version = ABI_VERSION;
        results.available_mask = 0;
        results.failed_mask = 0;
        Ok(())
    }

    #[no_mangle]
    pub extern "C" fn ps_scan_linux_v1(config: *const Config, results: *mut Results) -> i32 {
        if config.is_null() || results.is_null() {
            return libc::EINVAL;
        }
        let config = unsafe { &*config };
        let results = unsafe { &mut *results };
        if let Err(error) = validate_and_reset(config, results) {
            return error;
        }

        match catch_unwind(AssertUnwindSafe(|| scan(config, results))) {
            Ok(Ok(())) => 0,
            Ok(Err(error)) => {
                log(
                    config,
                    ERROR,
                    format!("patternsleuth: image scan failed: {error}"),
                );
                libc::EIO
            }
            Err(_) => {
                log(
                    config,
                    ERROR,
                    "patternsleuth: panic contained at the C ABI boundary",
                );
                libc::EFAULT
            }
        }
    }

    #[no_mangle]
    pub extern "C" fn ps_scan_linux_file_v1(
        path: *const libc::c_char,
        config: *const Config,
        results: *mut Results,
    ) -> i32 {
        if path.is_null() || config.is_null() || results.is_null() {
            return libc::EINVAL;
        }
        let config = unsafe { &*config };
        let results = unsafe { &mut *results };
        if let Err(error) = validate_and_reset(config, results) {
            return error;
        }

        match catch_unwind(AssertUnwindSafe(
            || -> Result<(), Box<dyn std::error::Error>> {
                let path_bytes = unsafe { std::ffi::CStr::from_ptr(path) }.to_bytes();
                if path_bytes.is_empty() {
                    return Err("ELF path is empty".into());
                }
                use std::os::unix::ffi::OsStrExt;
                let path = std::path::Path::new(std::ffi::OsStr::from_bytes(path_bytes));
                log(
                    config,
                    INFO,
                    format!(
                        "patternsleuth: reading offline ELF image {}",
                        path.display()
                    ),
                );
                let executable_data = std::fs::read(path)?;
                let image = Image::read(None, &executable_data, Some(path), true)?;
                scan_image(config, results, &image, 0)
            },
        )) {
            Ok(Ok(())) => 0,
            Ok(Err(error)) => {
                log(
                    config,
                    ERROR,
                    format!("patternsleuth: offline image scan failed: {error}"),
                );
                libc::EIO
            }
            Err(_) => {
                log(
                    config,
                    ERROR,
                    "patternsleuth: panic contained at the offline C ABI boundary",
                );
                libc::EFAULT
            }
        }
    }
}

#[cfg(target_os = "linux")]
pub use linux_v1::{ps_scan_linux_file_v1, ps_scan_linux_v1};
