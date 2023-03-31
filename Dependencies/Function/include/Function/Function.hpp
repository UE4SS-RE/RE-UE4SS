#ifndef RC_FUNCTION_HPP
#define RC_FUNCTION_HPP

#include <bit>
#include <stdexcept>

namespace RC
{
    template<typename>
    class Function;

    template<typename ReturnType, typename ...Params>
    class Function<ReturnType(Params...)>
    {
    private:
        // Make non-copyable & non-movable
        Function(const Function&) = delete;
        Function& operator=(const Function&) = delete;
        Function(Function&&) = delete;
        Function& operator=(Function&&) = delete;

        using FunctionSignature = ReturnType(*)(Params...);
        FunctionSignature m_active_func;
        FunctionSignature m_stored_original_func;
        void* m_function_address;
        bool m_is_ready = false;

    public:
        // Default constructor, make sure to call 'assign_address' before calling operator()
        Function() = default;

        // Constructor for assigning a proper function pointer, no need to call 'assign_address'
        Function(FunctionSignature func) : m_active_func(func), m_function_address(func), m_is_ready(true) {}

        // Constructor for when you don't have a signature pointer outside this Function class
        Function(void* func_address) : m_active_func(std::bit_cast<FunctionSignature>(func_address)), m_function_address(func_address), m_is_ready(true) {}

        // Assign a function pointer to this Function object
        auto assign_address(FunctionSignature func_ptr) -> void
        {
            m_active_func = func_ptr;
            m_function_address = func_ptr;
            m_stored_original_func = m_active_func;
            m_is_ready = true;
        }

        // Assign a void* to this Function object as the function address
        // Used as a helper when you only have a void* and it needs to be converted to a real function pointer
        auto assign_address(void* func_address) -> void
        {
            if (!func_address) { throw std::runtime_error{"Tried to set the function pointer of a Function object to nullptr"}; }
            m_active_func = std::bit_cast<FunctionSignature>(func_address);
            m_stored_original_func = m_active_func;
            m_function_address = m_active_func;
            m_is_ready = true;
        }

        // Assign a new function pointer (same type) to this Function object and store the old one
        auto assign_temp_address(FunctionSignature func_ptr) -> void
        {
            m_active_func = func_ptr;
            m_function_address = func_ptr;
            m_is_ready = true;
        }

        // Assign a new void* to this Function object and store the old function pointer
        // Used as a helper when you only have a void* and it needs to be converted to a real function pointer
        auto assign_temp_address(void* address) -> void
        {
            if (!address) { return; }
            m_active_func = std::bit_cast<FunctionSignature>(address);
            m_function_address = address;
            m_is_ready = true;
        }

        // Reset the function pointer back to the original one
        auto reset_address() -> void
        {
            m_active_func = m_stored_original_func;
            m_function_address = m_stored_original_func;
        }

        // Returns the currently active function pointer
        auto get_function_pointer() -> FunctionSignature
        {
            return m_active_func;
        }

        auto get_function_address() -> void*
        {
            return m_function_address;
        }

        auto is_ready() -> bool
        {
            return m_is_ready;
        }

        auto operator()(Params... params) -> ReturnType
        {
            if (!this->m_is_ready) { throw std::runtime_error{"Function object not ready but was called anyway"}; }
            return this->m_active_func(params...);
        }
    };
}

#endif //RC_FUNCTION_HPP
