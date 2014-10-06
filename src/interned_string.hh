#ifndef interned_string_hh_INCLUDED
#define interned_string_hh_INCLUDED

#include "string.hh"
#include "utils.hh"

#include <unordered_map>

namespace Kakoune
{

class InternedString;

class StringRegistry : public Singleton<StringRegistry>
{
private:
    friend class InternedString;

    InternedString acquire(StringView str);
    void acquire(size_t slot);
    void release(size_t slot);

    std::unordered_map<StringView, size_t> m_slot_map;
    std::vector<size_t> m_free_slots;
    using DataAndRefCount = std::pair<std::vector<char>, int>;
    std::vector<DataAndRefCount> m_storage;
};

class InternedString : public StringView
{
public:
    InternedString() = default;

    InternedString(const InternedString& str) { acquire_ifn(str); }

    InternedString(InternedString&& str) : StringView(str)
    {
        m_slot = str.m_slot;
        str.m_slot = -1;
    }

    InternedString(const char* str) : StringView() { acquire_ifn(str); }
    InternedString(StringView str) : StringView() { acquire_ifn(str); }

    InternedString& operator=(const InternedString& str)
    {
        if (str.data() == data() && str.length() == length())
            return *this;
        static_cast<StringView&>(*this) = str;
        if (str.m_slot != m_slot)
        {
            release_ifn();
            m_slot = str.m_slot;
            if (str.m_slot != -1)
                StringRegistry::instance().acquire(str.m_slot);
        }

        return *this;
    }

    InternedString& operator=(InternedString&& str)
    {
        static_cast<StringView&>(*this) = str;
        m_slot = str.m_slot;
        str.m_slot = -1;
        return *this;
    }

    ~InternedString()
    {
        release_ifn();
    }

    bool operator==(const InternedString& str) const
    { return data() == str.data() && length() == str.length(); }
    bool operator!=(const InternedString& str) const
    { return !(*this == str); }

    using StringView::operator==;
    using StringView::operator!=;

private:
    friend class StringRegistry;

    InternedString(StringView str, size_t slot)
        : StringView(str), m_slot(slot) {}

    void acquire_ifn(StringView str)
    {
        if (str.empty())
        {
            static_cast<StringView&>(*this) = StringView{};
            m_slot = -1;
        }
        else
            *this = StringRegistry::instance().acquire(str);
    }

    void release_ifn()
    {
        if (m_slot != -1)
            StringRegistry::instance().release(m_slot);
    }

    size_t m_slot = -1;
};

}

namespace std
{
    template<>
    struct hash<Kakoune::InternedString>
    {
        size_t operator()(const Kakoune::InternedString& str) const
        {
            return hash<const char*>{}(str.data()) ^
                   hash<int>{}((int)str.length());
        }
    };
}

#endif // interned_string_hh_INCLUDED
