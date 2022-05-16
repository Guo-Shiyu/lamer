#ifndef TPDEC_H
#define TPEDC_H

#include <array>
#include <string_view>
#include <atomic>
#include <iostream>

namespace lamer
{
    constexpr bool _kStrictMemoryAlign =
#ifdef
        true;
#undef STRICT_MEMORY_ALIGN
#else
        false;
#endif

    template <typename TimePoint, typename Mark = size_t>
    struct Piece
    {
        TimePoint timestamp;
        std::string_view comment;
        Mark mark;

        static_assert((!_kStrictMemoryAlign ||
                       (sizeof(timestamp) + sizeof(comment) + sizeof(mark)) == 32 or (sizeof(timestamp) + sizeof(comment) + sizeof(mark)) == 64),
                      "For performance, the struct layout needs to be aligned to 32 or 64 bytes");

        constexpr static Mark kUnmarked = Mark();
    };

    template <typename TimePoint>
    using Detector = TimePoint (*)(void);

    template <typename TimePoint, size_t _Size, typename Mark = size_t>
    using Cache = std::array<Piece<TimePoint, Mark>, _Size>;

    template <typename TimePoint, size_t _Size, typename Mark = size_t>
    using Explainer = void (*)(std::ostream &, const Cache<TimePoint, _Size, Mark> &, size_t len);

    template <typename TimePoint, size_t _Size, typename Mark = size_t>
    using Dumper = Explainer<TimePoint, _Size, Mark>;

    template <typename TimePoint, typename Mark = size_t>
    using TableBodyFormatter = void (*)(std::ostream &, const Piece<TimePoint, Mark> &, size_t);

    template <typename TimePoint>
    Detector<TimePoint> _default_detector = []() -> auto
    {
        return TimePoint();
    };

    template <typename TimePoint>
    constexpr const char *_default_precision_stmt = nullptr;

    // append nothing to Overview in default
    template <typename TimePoint, size_t _Size, typename Mark = size_t>
    Explainer<TimePoint, _Size, Mark> _default_explainer = [](std::ostream &, const Cache<TimePoint, _Size, Mark> &, size_t) {};

    // dump none detail infomation in default
    template <typename TimePoint>
    constexpr bool _with_table = false;

    // with a empty head of table
    template <typename TimePoint>
    constexpr const char *_default_table_head_stmt = nullptr;

    // dump each line in default
    template <typename TimePoint, typename Mark = size_t>
    TableBodyFormatter<TimePoint, Mark> _default_table_body_formatter = [](std::ostream &out, const Piece<TimePoint, Mark> &piece, size_t idx) -> void
    {
        out << piece.timestamp << '\n';
    };

    // lame's default formatter for a Measurer
    template <typename TimePoint, size_t _Size, typename Mark = size_t>
    Dumper<TimePoint, _Size, Mark> _default_dumper = [](std::ostream &out, const Cache<TimePoint, _Size, Mark> &cache, size_t len)
    {
        // dump base infomation
        constexpr std::string_view precision = (_default_precision_stmt<TimePoint> == nullptr ? "undesigned" : _default_precision_stmt<TimePoint>);
        out << "lame info (default dumper):\n"
            << "precision: " << precision << '\t' << "sample: " << len << '\n';

        // dump appendix to Overview
        _default_explainer<TimePoint, _Size, Mark>(out, cache, len);
        out << '\n'
            << '\n';

        if constexpr (_with_table<TimePoint>)
        {
            // dump detial info as a table
            constexpr std::string_view table_header = (_default_table_head_stmt<TimePoint> == nullptr ? "" : _default_table_head_stmt<TimePoint>);
            out << table_header << '\n'
                << "------------------------------------" << '\n';

            for (size_t i = 0; i < len; i++)
                _default_table_body_formatter<TimePoint, Mark>(out, cache[i], i), out << '\n';
        }
    };

    constexpr bool _is_power_of_2(size_t number)
    {
        if (number == 0 || number == 1)
            return false;
        else
            return 1 + _is_power_of_2(number >> 1);
    };

    template <typename TimePoint, size_t Capability,
              typename _Mark = size_t,
              typename _Dector = Detector<TimePoint>,
              typename _Cache = Cache<TimePoint, Capability, _Mark>,
              typename _Dumper = Dumper<TimePoint, Capability, _Mark>>
    struct Measurer
    {
        static_assert(
            _is_power_of_2(Capability),
            "Measurer's capability must be power of 2 and has a minimum of 2");

        using _TP = TimePoint;
        using _MK = _Mark;
        using _DC = _Dector;
        using _CA = _Cache;
        using _DP = _Dumper;
        using _PE = typename _Cache::value_type;

        _DC detector;
        _CA cache;
        std::atomic_size_t len;
        _DP dumper;

        Measurer() : detector(_default_detector<_TP>), cache(), len(0U), dumper(_default_dumper<_TP, Capability, _MK>) {}

        size_t record(std::string_view comment = "", _MK mark = _PE::kUnmarked)
        {
            _TP tp = detector();
            size_t usable_index = len.fetch_add(1U);

            _PE &piece = cache.at(usable_index);
            piece.timestamp = tp;
            piece.comment = comment;
            piece.mark = mark;

            return usable_index;
        }

        void dump(std::ostream &out)
        {
            dumper(out, cache, len);
        }
    };

    constexpr size_t _default_capability = 256U;

    using NanoSec = timespec;

    template <>
    Detector<NanoSec> _default_detector<NanoSec> = []() -> NanoSec {
        timespec ts = {0, 0};
        clock_gettime(CLOCK_REALTIME, &ts);
        return ts;
    };

    template <>
    constexpr bool _with_table<NanoSec> = true;

    template <>
    constexpr const char *_default_precision_stmt<NanoSec> = "nanosec";

    template <>
    constexpr const char *_default_table_head_stmt<NanoSec> = "stamp(s.ns)\t\tcomment";

    template <>
    TableBodyFormatter<NanoSec> _default_table_body_formatter<NanoSec> = [](std::ostream &out, const Piece<NanoSec> &pe, size_t idx) -> void
    {
        int thri_digit_sec = pe.timestamp.tv_sec % 100;
        out << thri_digit_sec << '.' << pe.timestamp.tv_nsec << '\t' << '\t' << pe.comment;
    };

    template <size_t Capability = _default_capability>
    using NanoMeasurer = Measurer<NanoSec, Capability>;

    using ClockCycle = size_t;

    template <>
    Detector<ClockCycle> _default_detector<ClockCycle> = []() -> ClockCycle {
        uint32_t lo, hi;
        uint64_t o;
        __asm__ __volatile__("rdtscp"
                             : "=a"(lo), "=d"(hi)
                             :
                             : "%ecx");
        o = hi;
        o <<= 32;
        return (o | lo);
    };

    template <>
    constexpr const char *_default_precision_stmt<ClockCycle> = "ClockCycle";

    template <size_t _Size, typename _Mark>
    Explainer<ClockCycle, _Size, _Mark> _default_explainer<ClockCycle, _Size, _Mark> = [](std::ostream &out, const Cache<ClockCycle, _Size, _Mark> &, size_t)
    {
        out << "CPU frequency: "
            << "todo-get-cpu-frequency";
    };

    template <>
    constexpr bool _with_table<ClockCycle> = true;

    template <>
    constexpr const char *_default_table_head_stmt<ClockCycle> = "stamp(cycle)\t\tcomment";

    template <>
    TableBodyFormatter<ClockCycle> _default_table_body_formatter<ClockCycle> = [](std::ostream &out, const Piece<ClockCycle> &pe, size_t idx) -> void
    {
        out << pe.timestamp << '\t' << '\t' << pe.mark;
    };

    template <size_t Capability = _default_capability>
    using CpuClockCycleMeasurer = Measurer<ClockCycle, Capability>;

}

#endif // TPEDC_H