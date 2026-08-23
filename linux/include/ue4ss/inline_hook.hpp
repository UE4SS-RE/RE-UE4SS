#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

namespace ue4ss::linux
{
    class InlineHook
    {
      public:
        InlineHook() = default;
        InlineHook(const InlineHook&) = delete;
        InlineHook& operator=(const InlineHook&) = delete;
        InlineHook(InlineHook&&) = delete;
        InlineHook& operator=(InlineHook&&) = delete;
        ~InlineHook();

        [[nodiscard]] bool install(void* target, void* replacement, std::string& error);
        [[nodiscard]] bool install(void* target,
                                   void* replacement,
                                   const std::function<void(void*)>& before_commit,
                                   std::string& error);
        // Inline-hook shutdown is deliberately split into two phases for
        // live multithreaded processes. restore() makes future calls enter the
        // original target while retaining the executable trampoline for
        // detours which are already in flight. release() may only be called
        // after the owner has drained those detours.
        [[nodiscard]] bool restore(std::string& error);
        [[nodiscard]] bool release(std::string& error);
        [[nodiscard]] bool remove(std::string& error);
        [[nodiscard]] bool is_installed() const;
        [[nodiscard]] void* target() const;
        [[nodiscard]] void* trampoline() const;

      private:
        [[nodiscard]] bool restore_locked(std::string& error);
        [[nodiscard]] bool release_locked(std::string& error);

        mutable std::mutex m_mutex;
        void* m_target{};
        void* m_replacement{};
        void* m_trampoline{};
        std::size_t m_trampoline_size{};
        std::vector<std::uint8_t> m_original_bytes;
        std::vector<std::uint8_t> m_patch_bytes;
        bool m_installed{};
    };

    class PointerHook
    {
      public:
        PointerHook() = default;
        PointerHook(const PointerHook&) = delete;
        PointerHook& operator=(const PointerHook&) = delete;
        PointerHook(PointerHook&&) = delete;
        PointerHook& operator=(PointerHook&&) = delete;
        ~PointerHook();

        [[nodiscard]] bool install(void** slot, void* replacement, std::string& error);
        [[nodiscard]] bool remove(std::string& error);
        [[nodiscard]] bool is_installed() const;
        [[nodiscard]] void* trampoline() const;

      private:
        mutable std::mutex m_mutex;
        void** m_slot{};
        void* m_original{};
        void* m_replacement{};
        bool m_installed{};
    };
} // namespace ue4ss::linux
