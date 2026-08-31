#ifndef UE4SS_REWRITTEN_FILEDEVICE_HPP
#define UE4SS_REWRITTEN_FILEDEVICE_HPP

#include <filesystem>
#include <memory>

#include <DynamicOutput/Common.hpp>
#include <DynamicOutput/Macros.hpp>
#include <DynamicOutput/OutputDevice.hpp>
#include <File/File.hpp>

namespace RC::Output
{
    // Note: For FileDevice, 'Output::sends()' must only be called after 'FileDevice::set_file_name_and_path()' has been called

    // Less simple class that outputs to a file on a drive
    // Behavior defined as:
    // Create all necessary directories
    // Create file if it doesn't exist
    // Open a file in append mode and keep it open until ~FileDevice
    // Whether to allow the file to be opened by other applications is not defined
    // Write one std::wstring to the file
    class FileDevice : public OutputDevice
    {
      private:
        mutable File::Handle m_file;
        std::filesystem::path m_file_name_and_path;

      protected:
        bool m_always_create_file{};

      public:
#if ENABLE_OUTPUT_DEVICE_DEBUG_MODE
        FileDevice::FileDevice() FileDevice fmt::print("FileDevice opening...\n");
    }

    FileDevice::~FileDevice() override
    {
        fmt::print("FileDevice closing...\n");
    }
#else
        ~FileDevice() override
        {
            // Do nothing if the file was never actually constructed
            // That can happen if there was an error during the call to 'FileType::open_file()'
            if (!m_file.is_valid())
            {
                return;
            }

            m_file.close();
        }
#endif

  private:
    auto start_device() const -> void
    {
        if (m_always_create_file)
        {
            m_file = File::open(m_file_name_and_path, File::OpenFor::Appending, File::OverwriteExistingFile::Yes, File::CreateIfNonExistent::Yes);
        }
        else
        {
            m_file = File::open(m_file_name_and_path, File::OpenFor::Appending, File::OverwriteExistingFile::No, File::CreateIfNonExistent::Yes);
        }

        m_is_device_ready = true;
    }

  public:
    // OutputDevice Interface -> START
    // Due to the design of the Output system the opening of the file is done in receive instead of in the constructor
    // It's opened only once and stays open until the Output object (not the device) leaves scope
    // The destructor is responsible for closing the file
    auto receive(File::StringViewType fmt) const -> void override
    {
        if (!m_is_device_ready)
        {
            start_device();
        }

        // Do file output stuff here
        // File should already be open & be ready for writing (happens in constructor)

        m_file.write_string_to_file(m_formatter(fmt));
    }
    // OutputDevice Interface -> END

    auto set_file_name_and_path(const File::StringType& file_name_and_path) -> void
    {
        m_file_name_and_path = file_name_and_path;
    }
};
} // namespace RC::Output

#endif // UE4SS_REWRITTEN_FILEDEVICE_HPP
