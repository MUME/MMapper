#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "macros.h"

#include <cassert>
#include <cmath>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <vector>

namespace diff {

namespace detail {

// clang-format off
template<typename Token>
struct NODISCARD DefaultGetScore final
{
    NODISCARD float operator()(const Token a, const Token b) const
    {
        return static_cast<float>(a == b);
    }
};
// clang-format on

// REVISIT: Consider using a tagged int to make sure we know these are reversed indices.
struct NODISCARD Pair final
{
    std::shared_ptr<const Pair> next;
    size_t aidx = 0;
    size_t bidx = 0;
    float score = 0;

    Pair(const size_t a, const size_t b, const float sc)
        : aidx{a}
        , bidx{b}
        , score{sc}
    {}
    Pair(const size_t a, const size_t b, const float sc, std::shared_ptr<const Pair> moved_next)
        : next{std::move(moved_next)}
        , aidx{a}
        , bidx{b}
        , score{sc}
    {
        if (next != nullptr) {
            // NOTE: construction order is increasing,
            // and these are actually "reverse index."
            assert(aidx > next->aidx);
            assert(bidx > next->bidx);
        }
    }
};

} // namespace detail

template<typename Token_>
struct NODISCARD Range final
{
public:
    using Token = Token_;
    static constexpr const size_t NPOS = ~static_cast<size_t>(0);

private:
    const Token *m_begin = nullptr;
    const Token *m_end = nullptr;

public:
    // explicit Range() = default;
    explicit Range(const Token *const begin, const Token *const end)
        : m_begin{begin}
        , m_end{end}
    {
        if ((begin == nullptr) != (end == nullptr)) {
            throw std::invalid_argument("end");
        }

        const auto b = reinterpret_cast<uintptr_t>(begin);
        const auto e = reinterpret_cast<uintptr_t>(end);
        assert(b <= e);
    }

    // This one obviously only makes sense if Token is char.
    NODISCARD static Range fromStringView(const std::string_view sv)
    {
        static_assert(std::is_same_v<char, std::decay_t<Token>>);
        const Token *const beg = sv.data();
        return Range{beg, beg + static_cast<ptrdiff_t>(sv.size())};
    }
    NODISCARD static Range fromVector(const std::vector<Token> &v)
    {
        // if (v.empty()) {
        //     return Range{};
        // }

        // Note: Using data/size avoids lack of conversion to const T* for STL debug iterators.
        const Token *const beg = v.data();
        return Range{beg, beg + static_cast<ptrdiff_t>(v.size())};
    }

    NODISCARD const Token *begin() const { return m_begin; }
    NODISCARD const Token *end() const { return m_end; }
    NODISCARD size_t size() const { return static_cast<size_t>(end() - begin()); }
    NODISCARD bool empty() const { return size() == 0; }

    NODISCARD const Token &front() const
    {
        assert(!empty());
        return *begin();
    }
    NODISCARD const Token &back() const
    {
        assert(!empty());
        return end()[-1];
    }

    void remove_prefix(size_t n)
    {
        assert(n <= size());
        m_begin += std::min(size(), n);
    }
    void remove_suffix(size_t n)
    {
        assert(n <= size());
        m_end -= std::min(size(), n);
    }

    NODISCARD const Token &at(const size_t pos) const
    {
        assert(pos < size());
        return begin()[pos];
    }

    NODISCARD const Token &operator[](const size_t pos) const { return at(pos); }

private:
    NODISCARD static Range static_substr(Range r, size_t start, size_t len = NPOS)
    {
        if (start != 0) {
            r.remove_prefix(start);
        }

        if (len != NPOS) {
            assert(len <= r.size());
            r.remove_suffix(r.size() - len);
        }

        return r;
    }

public:
    NODISCARD Range substr(size_t start, size_t len = NPOS) const
    {
        return static_substr(*this, start, len);
    }
};

enum class NODISCARD SideEnum { A, B, Common };

// Scorer needs to have either:
// float Scorer::operator()(const Token &, const Token &) const;
//   -or-
// float Scorer::operator()(Token, Token) const;
//
// The returned score must be finite; scores less than or equal to zero all count as zero.
//
// This doesn't support lambdas, and results are undefined if the function ever returns
// a different value for the same two inputs. Ideally you should write a pure function
// that only looks at the values of the tokens passed.
//
// However, if you're trying to be clever, keep in mind that although the implementation might
// present the arguments in the same order as the compared ranges (to help with debugging),
// that feature may not always exist, and the actual token given may not be part of the array
// you passed to compare(), so never try to compare the address to your array's address range,
// since that could be undefined behavior:
//
//     float operator()(const Token &a, const Token& b) const {
//        if (&a >= some_pointer) {} // example of possible undefined behavior
//        return static_cast<float>(a == b);
//     }
//
// If you need to check which array a token pointer belongs to, compare via uintptr_t.
template<typename Token_, typename GetScore_ = detail::DefaultGetScore<Token_>>
class NODISCARD Diff
{
public:
    using Token = Token_;
    using Range = diff::Range<Token_>;

private:
    using Pair = detail::Pair;
    using SharedPair = std::shared_ptr<const Pair>;
    struct NODISCARD SharedPairVector final : std::vector<SharedPair>
    {};

private:
    SharedPairVector m_vec;
    bool m_swapped = false;

public:
    virtual ~Diff() = default;

public:
    // This function will result in calls to virt_report().
    void compare(Range ra, Range rb)
    {
        const bool swapped = (ra.size() < rb.size());
        if (swapped) {
            std::swap(ra, rb);
        }
        compare_untrimmed(ra, rb, swapped);
    }

private:
    virtual void virt_report(SideEnum side, const Range &r) = 0;
    void report(SideEnum side, const Range &r)
    {
        if (m_swapped) {
            switch (side) {
            case SideEnum::A:
                side = SideEnum::B;
                break;
            case SideEnum::B:
                side = SideEnum::A;
                break;
            case SideEnum::Common:
                break;
            }
        }
        virt_report(side, r);
    }

private:
    void compare_untrimmed(const Range &input_ra, const Range &input_rb, const bool swapped)
    {
        assert(input_ra.size() >= input_rb.size());

        m_swapped = swapped;

        auto ra = input_ra;
        auto rb = input_rb;

        size_t start_offset = 0;
        size_t removed_from_end = 0;
        while (!ra.empty() && !rb.empty() && ra.front() == rb.front()) {
            ra.remove_prefix(1);
            rb.remove_prefix(1);
            ++start_offset;
        }
        while (!ra.empty() && !rb.empty() && ra.back() == rb.back()) {
            ra.remove_suffix(1);
            rb.remove_suffix(1);
            ++removed_from_end;
        }

        if (start_offset != 0) {
            report(SideEnum::Common, input_ra.substr(0, start_offset));
        }

        compare_trimmed(ra, rb);

        if (removed_from_end != 0) {
            report(SideEnum::Common, input_ra.substr(start_offset + ra.size()));
        }
    }

    NODISCARD static bool isRepeat(const SharedPair &p, const size_t apos, const size_t bpos)
    {
        return p != nullptr && (p->aidx == apos || p->bidx == bpos);
    }

    NODISCARD static size_t reverseIndex(const Range &r, const size_t idx)
    {
        assert(idx < r.size());
        return r.size() - idx - 1;
    }

    NODISCARD static float maybeScore(const SharedPair &p) { return !p ? 0.f : p->score; }

    class NODISCARD DeferredCommonOutput final
    {
    private:
        // R stands for Range, but it's a pair of indices.
        struct NODISCARD R final
        {
            size_t begin = 0;
            size_t end = 0;
        };

        R a;
        R b;

    public:
        explicit DeferredCommonOutput(const size_t aidx, const size_t bidx)
            : a{aidx, aidx + 1}
            , b{bidx, bidx + 1}
        {}

        NODISCARD bool canExtend(const size_t aidx, size_t bidx) const
        {
            return a.end == aidx && b.end == bidx;
        }

        void extend(const size_t aidx, const size_t bidx)
        {
            if (!canExtend(aidx, bidx)) {
                throw std::invalid_argument("aidx or bidx");
            }
            ++a.end;
            ++b.end;
        }

        NODISCARD Range getA(const Range &ra) const { return ra.substr(a.begin, a.end - a.begin); }
    };

    void compare_trimmed(const Range &ra, const Range &rb)
    {
        assert(ra.size() >= rb.size());
        if (rb.empty()) {
            report(SideEnum::A, ra);
            return;
        }

        auto scoreAb = [this, &ra, &rb, getScore = GetScore_{}](const size_t apos,
                                                                const size_t bpos) -> float {
            assert(apos < ra.size());
            assert(bpos < rb.size());
            // make sure it has the right return type and can accept pass by const ref
            // (note: this will also allow by-value)
            static_assert(
                std::is_invocable_r_v<float, const GetScore_ &, const Token &, const Token &>);

            Token a = ra[reverseIndex(ra, apos)];
            Token b = rb[reverseIndex(rb, bpos)];

            // This helps with debugging by presenting the arguments
            // in the same order that the caller passed them to compare().
            if (m_swapped) {
                std::swap(a, b);
            }

            const auto score = getScore(a, b);
            assert(std::isfinite(score));
            return score;
        };

        auto update = [&scoreAb](SharedPair &here,
                                 const size_t apos,
                                 const size_t bpos,
                                 const SharedPair defaultValue) {
            here = defaultValue;
            const float score = scoreAb(apos, bpos);
            if (score <= 0.f) {
                return;
            }

            if (!isRepeat(defaultValue, apos, bpos)) {
                // extend the sequence (common)
                here = std::make_shared<const Pair>(apos,
                                                    bpos,
                                                    score + maybeScore(defaultValue),
                                                    defaultValue);

            } else if (defaultValue->score < score) {
                // override the previous match of the same token with a better score (rare)
                here = std::make_shared<const Pair>(apos, bpos, score);
            }
        };

        // O(M*N) operations, with O(N) fixed storage,
        // but it might grow to O(M*N) via linked lists of pairs.
        const auto &vec = std::invoke(
            [this, &ra, &rb, &scoreAb, &update]() -> const SharedPairVector & {
                const auto asize = ra.size();
                const auto bsize = rb.size();
                assert(asize >= bsize);

                SharedPairVector &v = m_vec;
                v.clear();
                v.resize(bsize);

                // first pass initializes
                {
                    const size_t apos = 0;

                    // first element is handled differently
                    {
                        const size_t bpos = 0;
                        const auto score = scoreAb(apos, bpos);
                        v[bpos] = (score > 0) ? std::make_shared<Pair>(apos, bpos, score) : nullptr;
                    }

                    for (size_t bpos = 1; bpos < bsize; ++bpos) {
                        const SharedPair before = v[bpos - 1];
                        update(v[bpos], apos, bpos, before);
                    }
                }

                // loop over the remaining elements of the longer input
                {
                    for (size_t apos = 1; apos < asize; ++apos) {
                        // first element is handled differently
                        {
                            const size_t bpos = 0;
                            const SharedPair before = v[bpos];
                            update(v[bpos], apos, bpos, before);
                        }

                        // this loop accounts for the majority of the (asize * bsize) operations
                        // actually it's (asize * bsize + 1 - asize - bsize)
                        for (size_t bpos = 1; bpos < bsize; ++bpos) {
                            // merge from two sources;
                            const SharedPair best = std::invoke([bpos, &v]() -> SharedPair {
                                const SharedPair &one = v[bpos - 1]; // from the b-chain
                                const SharedPair &two = v[bpos];     // from the a-chain
                                return (maybeScore(one) > maybeScore(two)) ? one : two;
                            });
                            update(v[bpos], apos, bpos, best);
                        }
                    }
                }
                return v;
            });

        auto advanceAndReport =
            [this, &ra, &rb](const SideEnum side, size_t &pos, const size_t idx) {
                assert(side != SideEnum::Common);
                if (pos >= idx) {
                    return;
                }

                const auto &r = (side == SideEnum::A) ? ra : rb;
                const auto only = r.substr(pos, idx - pos);
                report(side, only);

                pos = idx;
            };

        size_t apos = 0;
        size_t bpos = 0;

        auto showA = [&apos, &advanceAndReport](const size_t aidx) {
            if (aidx > 0) {
                advanceAndReport(SideEnum::A, apos, aidx);
            }
        };
        auto showB = [&bpos, &advanceAndReport](const size_t bidx) {
            if (bidx > 0) {
                advanceAndReport(SideEnum::B, bpos, bidx);
            }
        };

        auto showAB = [this, &showA, &showB](size_t aidx, size_t bidx) {
            if (m_swapped) {
                showB(bidx);
                showA(aidx);
            } else {
                showA(aidx);
                showB(bidx);
            }
        };

        std::optional<DeferredCommonOutput> deferredCommon;
        auto flush_deferred = [this, &ra, &deferredCommon]() {
            if (!deferredCommon) {
                return;
            }
            report(SideEnum::Common, deferredCommon->getA(ra));
            deferredCommon.reset();
        };

        auto defer_common = [&apos, &bpos, &deferredCommon, &flush_deferred](const size_t aidx,
                                                                             const size_t bidx) {
            assert(apos == aidx);
            assert(bpos == bidx);
            apos = aidx + 1;
            bpos = bidx + 1;

            if (deferredCommon) {
                if (deferredCommon->canExtend(aidx, bidx)) {
                    deferredCommon->extend(aidx, bidx);
                    return;
                }
                flush_deferred();
            }

            deferredCommon.emplace(aidx, bidx);
        };

        auto report_next_common =
            [&deferredCommon, &defer_common, &flush_deferred, &showAB, &ra, &rb](const size_t aidx,
                                                                                 const size_t bidx) {
                assert(ra[aidx] == rb[bidx]);

                if (deferredCommon && !deferredCommon->canExtend(aidx, bidx)) {
                    flush_deferred();
                }

                showAB(aidx, bidx);
                defer_common(aidx, bidx);
            };

        // Reporting begins here
        {
            for (SharedPair pPair = vec.back(); pPair != nullptr; pPair = pPair->next) {
                const Pair &p = *pPair;
                const auto aidx = reverseIndex(ra, p.aidx);
                const auto bidx = reverseIndex(rb, p.bidx);
                report_next_common(aidx, bidx);
            }

            flush_deferred();
            showAB(ra.size(), rb.size());
        }

        // Note: This frees the shared pointers, but it preserves the allocated array
        // for future invocations.
        m_vec.clear();
    }
};
} // namespace diff

namespace test {
extern void testDiff();
} // namespace test
