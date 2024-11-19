#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "RoomIdSet.h"
#include "roomid.h"

#include <cassert>
#include <memory>
#include <optional>
#include <set>
#include <utility>
#include <variant>

template<typename Type_, typename Set_, typename IdHelper_>
struct NODISCARD TinySet
{
private:
    using Type = Type_;
    using Set = Set_;
    using PtrToSet = std::unique_ptr<Set>;

    // REVISIT: if the type can spare a bit, then they can be equal.
    /*
    static_assert(sizeof(Type) < sizeof(uintptr_t));
    static_assert(sizeof(uintptr_t) >= sizeof(PtrToSet));
    */

private:
    uintptr_t m_storage = 0;

private:
    NODISCARD bool isEmpty() const noexcept { return m_storage == 0u; }
    NODISCARD bool hasOne() const noexcept { return (m_storage & 0x1u) == 0x1u; }
    NODISCARD bool hasBig() const noexcept { return !isEmpty() && (m_storage & 0x1u) == 0x0u; }

private:
    NODISCARD Set &getBig()
    {
        if (!hasBig()) {
            throw std::runtime_error("bad type");
        }
        return **reinterpret_cast<PtrToSet *>(&m_storage);
    }
    NODISCARD const Set &getBig() const
    {
        if (!hasBig()) {
            throw std::runtime_error("bad type");
        }
        return **reinterpret_cast<const PtrToSet *>(&m_storage);
    }
    NODISCARD Type getOnly() const
    {
        if (!hasOne()) {
            throw std::runtime_error("bad type");
        }

        // NOTE: Only allows 31-bit roomids on 32-bit platforms.
        return IdHelper_::fromBits(static_cast<size_t>(m_storage) >> 1u);
    }

private:
    void assign(PtrToSet newValue)
    {
        if (!hasBig()) {
            m_storage = 0;
        }

        const bool wasNullptr = newValue == nullptr;
        PtrToSet *const ptr = reinterpret_cast<PtrToSet *>(&m_storage);
        using std::swap;
        swap(*ptr, newValue);

        if (wasNullptr) {
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
        m_storage = (IdHelper_::getBits(value) << 1u) | 1u;
        assert(hasOne());
    }

    void clear() noexcept
    {
        if (hasBig()) {
            assign(nullptr);
        } else {
            m_storage = 0;
        }
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
    TinySet(TinySet &&other) noexcept { std::swap(m_storage, other.m_storage); }

    TinySet(const TinySet &other)
    {
        if (other.hasBig()) {
            assign(std::make_unique<Set>(other.getBig()));
        } else {
            clear();
            m_storage = other.m_storage;
        }
    }

    TinySet &operator=(TinySet &&other) noexcept
    {
        if (std::addressof(other) != this) {
            std::swap(m_storage, other.m_storage);
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

    explicit TinySet(Set &&other)
    {
        if (other.empty()) {
            /* nop */
        } else if (other.size() == 1) {
            *this = TinySet(other.first());
        } else {
            set(std::make_unique<Set>(std::move(other)));
        }
    }

public:
    struct NODISCARD ConstIterator final
    {
    private:
        using SetConstIt = typename Set::ConstIterator;
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
        if (isEmpty()) {
            return ConstIterator{};
        } else if (hasOne()) {
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
        if (m_storage == rhs.m_storage) {
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

        // convert to Big
        PtrToSet ptrToBig = std::make_unique<Set>();
        auto &big = *ptrToBig;
        big.insert(getOnly());
        big.insert(id);

        assert(hasOne());
        assign(std::exchange(ptrToBig, {})); // conversion happens here
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
