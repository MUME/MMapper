#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "RoomIdSet.h"
#include "roomid.h"

#include <cassert>
#include <memory>
#include <new>
#include <optional>
#include <set>
#include <utility>
#include <variant>

template<typename Type_, typename Set_, typename IdHelper_>
struct NODISCARD TinySet
{
private:
    using Type = Type_;
    using BigSet = Set_; // big as opposed to tiny
    using UniqueBigSet = std::unique_ptr<BigSet>;

private:
    // This Variant class is basically just a "compiler approved" union of Type and UniqueBig,
    // where the least significant bit decides which type it is.
    //
    // Note that Storage does not control the lifetime of the pointed-to-set,
    // but TinySet does control the lifetime of that object.
    struct NODISCARD Variant final
    {
    private:
        static inline constexpr size_t BUF_SIZE = sizeof(uintptr_t);
        static inline constexpr size_t BUF_ALIGN = alignof(uintptr_t);
        static_assert(BUF_SIZE == sizeof(UniqueBigSet));
        static_assert(BUF_SIZE >= sizeof(size_t));
        static_assert(sizeof(size_t) >= sizeof(Type)); /* note: must be at least one bit larger */
        alignas(BUF_ALIGN) char m_buf[BUF_SIZE]{0};

    public:
        NODISCARD bool operator==(const Variant &other) const noexcept
        {
            return std::memcmp(m_buf, other.m_buf, BUF_SIZE) == 0;
        }

    private:
        // holds_xxx() functions are based on this.
        //
        // Note: The copy is made to avoid the following dreaded sequence of events:
        // 1) auto *ptr = std::launder(reinterpret_cast<UniqueBigSet*>(buf));
        // 2) auto tmp = *std::launder(reinterpret_cast<uintptr_t*>(buf)); // invalidates ptr
        // 3) use(ptr); // UB
        //
        // The reason this is UB is because of strict pointer aliasing rule that essentially
        // allows the compiler to assume there can only be only one laundered "object"
        // (or primitive type) at a given address, with the notable exception that it's also
        // legal to have a pointer to a type that provides storage (e.g. char).
        //
        // So this means if the compiler can detect the start of the lifetime of a new object
        // at the same address, then the old object's lifetime has ended. This means we cannot
        // simultaneously maintain a pointer in the storage -and- also examine it as a uintptr_t.
        // (Hopefully this rule will be relaxed for primitive types in a future c++ standard.)
        //
        // Another option would be to detect if the platform is big or little endian,
        // and then just read the buffer's high or low byte to know the type.
        // (I'm too lazy to check the assembly to see if either option is actually better,
        // but I assume that the compiler will figure out both, so I'd expect both
        // to yield the same assembly code.)
        NODISCARD uintptr_t get_uintptrt() const noexcept
        {
            const Variant copy = *this; // compiler performs a memcpy
            return *std::launder(reinterpret_cast<const uintptr_t *>(copy.m_buf));
        }

    public:
        // empty is effectively just holding "(UniqueBig*)nullptr"
        NODISCARD bool holds_nothing() const noexcept { return get_uintptrt() == 0u; }
        NODISCARD bool holds_single_value() const noexcept { return (get_uintptrt() & 1u) == 1u; }
        NODISCARD bool holds_unique_bigset() const noexcept
        {
            return !holds_nothing() && !holds_single_value();
        }

    public:
        void clear() noexcept
        {
            if (holds_unique_bigset()) {
                assign_unique_bigset(nullptr);
            } else {
                // would it be okay to write "*this = {};" here?
                *std::launder(reinterpret_cast<uintptr_t *>(m_buf)) = 0u;
            }
            assert(holds_nothing());
        }

    public:
        NODISCARD Type get_single_value() const noexcept
        {
            assert(holds_single_value());
            const auto ui = get_uintptrt() >> 1u;
            const auto size = static_cast<size_t>(ui);
            return IdHelper_::fromBits(size);
        }
        void set_single_value(const Type value) noexcept
        {
            if (holds_unique_bigset()) {
                assert(!holds_unique_bigset());
                std::abort(); // crash
            }

            constexpr size_t MASK = ~size_t{0} >> 1u;
            const auto raw_val = static_cast<size_t>(IdHelper_::getBits(value));
            const auto masked_val = static_cast<uintptr_t>(raw_val & MASK);
            assert(raw_val == masked_val);
            const auto ui = (masked_val << 1u) | 1u;
            assert(holds_nothing() || holds_single_value());
            *std::launder(reinterpret_cast<uintptr_t *>(m_buf)) = ui;
            assert(holds_single_value());
            assert(get_single_value() == value);
        }

    private:
        NODISCARD UniqueBigSet &getUniqueBigSet() noexcept
        {
            assert(holds_nothing() || holds_unique_bigset()); // used in assignment
            return *std::launder(reinterpret_cast<UniqueBigSet *>(m_buf));
        }
        NODISCARD const UniqueBigSet &getUniqueBigSet() const noexcept
        {
            assert(holds_unique_bigset());
            return *std::launder(reinterpret_cast<const UniqueBigSet *>(m_buf));
        }

    public:
        NODISCARD BigSet &get_big() noexcept
        {
            assert(holds_unique_bigset());
            return *getUniqueBigSet();
        }
        NODISCARD const BigSet &get_big() const noexcept
        {
            assert(holds_unique_bigset());
            return *getUniqueBigSet();
        }
        // newValue is allowed to be nullptr
        void assign_unique_bigset(UniqueBigSet newValue) noexcept
        {
            assert(holds_nothing() || holds_unique_bigset());
            UniqueBigSet &ref = getUniqueBigSet();
            using std::swap;
            swap(ref, newValue);
            assert(holds_nothing() || holds_unique_bigset());
        }
    };
    Variant m_var;

private:
    NODISCARD bool isEmpty() const noexcept { return m_var.holds_nothing(); }
    NODISCARD bool hasOne() const noexcept { return m_var.holds_single_value(); }
    NODISCARD bool hasBig() const noexcept { return m_var.holds_unique_bigset(); }

private:
    NODISCARD BigSet &getBig()
    {
        if (!hasBig()) {
            throw std::runtime_error("bad type");
        }
        return m_var.get_big();
    }
    NODISCARD const BigSet &getBig() const
    {
        if (!hasBig()) {
            throw std::runtime_error("bad type");
        }
        return m_var.get_big();
    }
    NODISCARD Type getOnly() const
    {
        if (!hasOne()) {
            throw std::runtime_error("bad type");
        }

        return m_var.get_single_value();
    }

private:
    void assign(UniqueBigSet newValue)
    {
        if (!hasBig()) {
            m_var.clear();
        }

        const bool should_expect_empty = newValue == nullptr;
        m_var.assign_unique_bigset(std::exchange(newValue, {}));

        if (should_expect_empty) {
            assert(isEmpty());
        } else {
            assert(hasBig());
        }
    }

    void assign(Type value) noexcept
    {
        if (hasBig()) {
            assign(nullptr);
            assert(isEmpty());
        }

        // NOTE: Only allows 31-bit roomids on 32-bit platforms.
        m_var.set_single_value(value);
        assert(hasOne());
    }

    void clear() noexcept
    {
        m_var.clear();
        assert(isEmpty());
    }

public:
    TinySet() noexcept = default;
    explicit TinySet(const RoomId one)
    {
        assert(isEmpty());
        assign(one);
        assert(hasOne());
    }

    ~TinySet() { clear(); }
    TinySet(TinySet &&other) noexcept { std::swap(m_var, other.m_var); }

    TinySet(const TinySet &other)
    {
        if (other.hasBig()) {
            assign(std::make_unique<BigSet>(other.getBig()));
        } else {
            clear();
            m_var = other.m_var;
        }
    }

    TinySet &operator=(TinySet &&other) noexcept
    {
        if (std::addressof(other) != this) {
            std::swap(m_var, other.m_var);
        }
        return *this;
    }

    TinySet &operator=(const TinySet &other)
    {
        if (std::addressof(other) != this) {
            *this = TinySet(other);
        }
        return *this;
    }

    explicit TinySet(BigSet &&other) = delete;

public:
    struct NODISCARD ConstIterator final
    {
    private:
        using SetConstIt = typename BigSet::ConstIterator;
        SetConstIt m_setIt{};
        Type m_val{};
        enum class NODISCARD StateEnum : uint8_t { Empty, One, Big };
        StateEnum m_state = StateEnum::Empty;

    private:
        friend TinySet;
        explicit ConstIterator() = default;
        explicit ConstIterator(const Type val)
            : m_val{val}
            , m_state{StateEnum::One}
        {}
        explicit ConstIterator(const SetConstIt setIt)
            : m_setIt{setIt}
            , m_state{StateEnum::Big}
        {}

    public:
        NODISCARD Type operator*() const
        {
            switch (m_state) {
            case StateEnum::Empty:
                assert(false);
                return Type{};
            case StateEnum::One:
                return m_val;
            case StateEnum::Big:
                return *m_setIt;
            }
            assert(false);
            std::abort();
            return Type{};
        }
        ALLOW_DISCARD ConstIterator &operator++()
        {
            switch (m_state) {
            case StateEnum::Empty:
                assert(false);
                break;
            case StateEnum::One:
                m_state = StateEnum::Empty;
                break;
            case StateEnum::Big:
                ++m_setIt;
                break;
            }
            return *this;
        }
        void operator++(int) = delete;
        NODISCARD bool operator==(const ConstIterator &other) const
        {
            if (other.m_state != m_state) {
                return false;
            }

            switch (m_state) {
            case StateEnum::Empty:
                return true;
            case StateEnum::One:
                return m_val == other.m_val;
            case StateEnum::Big:
                return m_setIt == other.m_setIt;
            }
            assert(false);
            std::abort();
            return false;
        }
        NODISCARD bool operator!=(const ConstIterator &other) const { return !operator==(other); }
    };

public:
    NODISCARD ConstIterator begin() const
    {
        if (isEmpty()) {
            return ConstIterator{};
        } else if (hasOne()) {
            return ConstIterator{getOnly()};
        } else {
            return ConstIterator{getBig().begin()};
        }
    }
    NODISCARD ConstIterator end() const
    {
        if (isEmpty() || hasOne()) {
            return ConstIterator{};
        } else {
            return ConstIterator{getBig().end()};
        }
    }
    NODISCARD size_t size() const
    {
        if (isEmpty()) {
            return 0;
        } else if (hasOne()) {
            return 1;
        } else {
            return getBig().size();
        }
    }

public:
    NODISCARD bool empty() const noexcept { return isEmpty(); }
    NODISCARD bool contains(const Type id) const
    {
        if (isEmpty()) {
            return false;
        } else if (hasOne()) {
            return getOnly() == id;
        } else {
            const auto &big = getBig();
            return big.contains(id);
        }
    }

    NODISCARD bool operator==(const TinySet &rhs) const
    {
        if (m_var == rhs.m_var) {
            return true;
        }

        return hasBig() && rhs.hasBig() && getBig() == rhs.getBig();
    }

    NODISCARD bool operator!=(const TinySet &rhs) const { return !operator==(rhs); }

public:
    void erase(const Type id)
    {
        assert(!IdHelper_::isInvalid(id));

        if (isEmpty()) {
            return;
        }

        if (hasOne()) {
            if (getOnly() == id) {
                clear();
            }
            return;
        }

        auto &big = getBig();
        big.erase(id);

        if (big.empty()) {
            clear();
            return;
        }

        if (big.size() > 1) {
            return;
        }

        // shrink since we just have one element
        const auto first = *big.begin();

        assert(hasBig());
        assign(first); // conversion happens here
        assert(hasOne());
    }

    void insert(const Type id)
    {
        assert(!IdHelper_::isInvalid(id));

        if (contains(id)) {
            return;
        }

        if (isEmpty()) {
            assign(id);
            assert(hasOne());
            return;
        }

        if (hasBig()) {
            auto &big = getBig();
            big.insert(id);
            return;
        }

        if (getOnly() == id) {
            return;
        }

        // convert to BigSet
        UniqueBigSet uniqueBigSet = std::make_unique<BigSet>();
        auto &big = *uniqueBigSet;
        big.insert(getOnly());
        big.insert(id);

        assert(hasOne());
        assign(std::exchange(uniqueBigSet, {})); // conversion to BigSet happens here
        assert(hasBig());
    }

public:
    void insertAll(const TinySet &other)
    {
        for (const Type x : other) {
            insert(x);
        }
    }

public:
    NODISCARD Type first() const
    {
        if (isEmpty()) {
            throw std::out_of_range("set is empty");
        } else if (hasOne()) {
            return getOnly();
        } else {
            return *getBig().begin();
        }
    }
};

namespace detail {

template<typename Tag_>
struct NODISCARD InvalidHelper;

template<>
struct NODISCARD InvalidHelper<RoomId> final
{
    static constexpr const RoomId INVALID = INVALID_ROOMID;
};

template<>
struct NODISCARD InvalidHelper<ExternalRoomId> final
{
    static constexpr const ExternalRoomId INVALID = INVALID_EXTERNAL_ROOMID;
};

template<typename IdType_>
struct NODISCARD RoomIdHelper final
{
    using IdType = IdType_;
    NODISCARD static bool isInvalid(const IdType id)
    {
        return id == InvalidHelper<IdType>::INVALID;
    }
    NODISCARD static size_t getBits(const IdType id) { return static_cast<size_t>(id.value()); }
    NODISCARD static IdType fromBits(const size_t bits)
    {
        return IdType{static_cast<uint32_t>(bits)};
    }
};

template<typename IdType_>
using TinyHelper = TinySet<IdType_, BasicRoomIdSet<IdType_>, RoomIdHelper<IdType_>>;

} // namespace detail

using TinyRoomIdSet = detail::TinyHelper<RoomId>;
using TinyExternalRoomIdSet = detail::TinyHelper<ExternalRoomId>;

NODISCARD extern RoomIdSet toRoomIdSet(const TinyRoomIdSet &set);
NODISCARD extern TinyRoomIdSet toTinyRoomIdSet(const RoomIdSet &set);

namespace test {
extern void testTinyRoomIdSet();
} // namespace test
