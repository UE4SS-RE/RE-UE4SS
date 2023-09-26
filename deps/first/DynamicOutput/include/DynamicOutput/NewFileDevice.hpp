#ifndef UE4SS_REWRITTEN_NEWFILEDEVICE_HPP
#define UE4SS_REWRITTEN_NEWFILEDEVICE_HPP

namespace RC::Output
{

    // Behavior defined as:
    // Identical to FileDevice except it deletes the file & re-creates it before outputting anything
    class NewFileDevice : public FileDevice
    {
      public:
        NewFileDevice()
        {
            this->m_always_create_file = true;
        }
    };

} // namespace RC::Output

#endif // UE4SS_REWRITTEN_NEWFILEDEVICE_HPP
