#pragma once

#pragma warning(disable: 4068)

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"

#ifdef LINUX
#define fopen_s(pFile, filename, mode) ((*(pFile)) = fopen((filename), (mode))) == NULL
#endif

class IBufferWriter
{
public:

    virtual FORCEINLINE void WriteString(std::string String) = 0;
    virtual FORCEINLINE void WriteString(std::string_view String) = 0;
    virtual FORCEINLINE void Write(void* Input, size_t Size) = 0;
    virtual FORCEINLINE void Seek(int Pos, int Origin = SEEK_CUR) = 0;
    virtual uint32_t Size() = 0;
};

class StreamWriter : IBufferWriter
{
    std::stringstream m_Stream;

public:

    virtual ~StreamWriter()
    {
        m_Stream.flush();
    }

    FORCEINLINE std::stringstream& GetBuffer()
    {
        return m_Stream;
    }

    FORCEINLINE void WriteString(std::string String) override
    {
        m_Stream.write(String.c_str(), String.size());
    }

    FORCEINLINE void WriteString(std::string_view String) override
    {
        m_Stream.write(String.data(), String.size());
    }

    FORCEINLINE void Write(void* Input, size_t Size) override
    {
        m_Stream.write((char*)Input, Size);
    }

    FORCEINLINE void Seek(int Pos, int Origin = SEEK_CUR) override
    {
        m_Stream.seekp((std::streamoff) Pos, (std::ios_base::seekdir) Origin);
    }

    uint32_t Size() override
    {
        auto pos = m_Stream.tellp();
        this->Seek(0, SEEK_END);
        auto ret = m_Stream.tellp();
        this->Seek(pos, SEEK_SET);

        return ret;
    }

    template <typename T>
    FORCEINLINE void Write(T Input)
    {
        Write(&Input, sizeof(T));
    }
};

class FileWriter : IBufferWriter
{
    FILE* m_File;

public:

    FileWriter(const char* FileName)
    {
        auto fopen_r = fopen_s(&m_File, FileName, "wb");
        printf("");
    }

    virtual ~FileWriter()
    {
        std::fclose(m_File);
    }

    FORCEINLINE void WriteString(std::string String) override
    {
        std::fwrite(String.c_str(), String.length(), 1, m_File);
    }

    FORCEINLINE void WriteString(std::string_view String) override
    {
        std::fwrite(String.data(), String.size(), 1, m_File);
    }

    FORCEINLINE void Write(void* Input, size_t Size) override
    {
        std::fwrite(Input, Size, 1, m_File);
    }

    FORCEINLINE void Seek(int Pos, int Origin = SEEK_CUR) override
    {
        std::fseek(m_File, Pos, Origin);
    }

    uint32_t Size() override
    {
        auto pos = std::ftell(m_File);
        std::fseek(m_File, 0, SEEK_END);
        auto ret = std::ftell(m_File);
        std::fseek(m_File, pos, SEEK_SET);
        return ret;
    }

    template <typename T>
    FORCEINLINE void Write(T Input)
    {
        Write(&Input, sizeof(T));
    }
};

#pragma clang diagnostic pop

#pragma warning(default: 4068)
