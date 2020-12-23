#ifndef OC_TIMEPOINT
#define OC_TIMEPOINT

// #include <chrono>
// #include <functional>
// #include <iterator>
#include <cmath>
#include <string>
// #include <tuple>
// #include <unordered_map>
// #include <vector>

#include "oc/LeapSecondDatabase.hpp"

#include <boost/date_time/gregorian_calendar.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace oc {
    /*!
        \class oc::TimePoint

        \brief Representation of a single point in time, with clock conversions.

        The internal representation is a boost::posix_time::ptime, meaning that
        leap seconds are compensated for manually within this class and there
        are no such things as epochs.  ptime's are stored as a Gregorian date
        and the time duration that has passed within that date, wrapping to a
        new date when hitting 23:59:60; this also means that leap seconds can't
        be represented in UTC (time will "freeze").  For clocks with epochs,
        such as TAI93, GPS, etc, seconds_since* functions are provided.

        Conversions to and from local time are not supported, largely due to DST.

        \section Example
        \snippet tests/time_point.cpp oc::TimePoint example
    */
    class TimePoint {
        public:
            /*! \enum TimePoint::clock_type

                \brief Type of clock being represented
            */
            enum clock_type {
                local, utc, tai, tai93, gps, tt
            };
            /*!
                \brief Construct a clock with the current time in the given clock type.

                \param clock_type what kind of clock
            */
            explicit TimePoint(const TimePoint::clock_type clock_type=local);

            /*!
                \brief Construct a new TimePoint given date, time, and optional input clock_type

                \param date date
                \param time time
                \param clock_type clock type of input
            */
            TimePoint(const boost::gregorian::date& date, const boost::posix_time::time_duration& time, TimePoint::clock_type clock_type=utc) : ptime_{date, time}, clock_type_{clock_type} {}

            /*!
                \brief Construct a new TimePoint given date, time, and optional input clock_type

                \param date date
                \param time time
                \param milli milliseconds
                \param clock_type clock type of input
            */
            TimePoint(const boost::gregorian::date date, const boost::posix_time::time_duration& time, const int milli, const TimePoint::clock_type clock_type=utc) : ptime_{date, time + boost::posix_time::milliseconds{milli}}, clock_type_{clock_type} {}

            /*!
                \brief Construct a new TimePoint given a boost ptime and optional input clock_type

                \param ptime time point to represent
                \param clock_type clock type of input
            */
            explicit TimePoint(const boost::posix_time::ptime& ptime, const TimePoint::clock_type clock_type=utc) : ptime_{ptime}, clock_type_(clock_type) {}

            /*!
                \brief Construct a new TimePoint given a boost ptime and optional input clock_type

                \param datetime string representation of date (see example section for acceptable strings)
                \param clock_type clock type of input
            */
            explicit TimePoint(const std::string& datetime, const TimePoint::clock_type clock_type=utc);

            /*!
                \brief Construct a clock from a Unix timestamp (truncated to milliseconds)

                \param seconds_since_1970 seconds since Jan 1, 1970, 00:00:00 (standard Unix epoch)
                \param clock_type what kind of clock
            */
            explicit TimePoint(const double seconds_since_1970, const TimePoint::clock_type clock_type=utc);

            // TimePoint(const TimePoint& other){ptime_ = other.ptime_; clock_type_ = other.clock_type_;}

            /*!
                \brief Return time point as string, with the given format.

                The format is as described in the following link, with one addition.  \%t is also available to insert the clock type as returned by clock().

                https://www.boost.org/doc/libs/1_69_0/doc/html/date_time/date_time_io.html

                \param format format string

                \return string representing this time point
            */
            // %t = clock()
            std::string to_string(const std::string &format="%Y-%m-%dT%H:%M:%S%F") const;

            /*!
                \brief Get the clock type being represented

                \return the current clock type
            */
            TimePoint::clock_type get_clock_type() const {return clock_type_;}
            /*!
                \brief Get the native representation of this date time.

                This shouldn't really be used, as the internals might change to
                not even use a ptime if a limitation is discovered.

                \return the raw, underlying ptime used by this object
            */
            const boost::posix_time::ptime& native() const {return ptime_;} // shouldn't be relied on, but I won't wrap everything

            /*!
                \brief Convert this clock to another type

                \todo The return value of this might change.  Does it make more sense to treat time as immutable and return a new instance?

                \param clock_type new type of clock

                \return pointer to this
            */
            const TimePoint convert_to(const TimePoint::clock_type clock_type) const;

            /*!
                \brief Get the clock type as a string.

                This is used during formatting for the to_string method.

                \return string representation of TimePoint::clock_type
            */
            const std::string clock() const {
                switch (clock_type_){
                    case local:
                        return "Local";
                    case utc:
                        return "UTC";
                    case tai:
                        return "TAI";
                    case tai93:
                        return "TAI93";
                    case gps:
                        return "GPS";
                    case tt:
                        return "TT";
                }
                return "Unknown"; // not possible
            }

            /*!
                \brief Output time point using the to_string() function with default parameters.

                \param out output stream to which to print
                \param t TimePoint object to print

                \return output stream passed in, for chaining
            */
            inline friend std::ostream& operator<<(std::ostream &out, const TimePoint& t){return out << t.to_string();}


            /*!
                \brief Return duration since given time.

                \param time epoch time to check

                \return duration since given epoch
            */
            boost::posix_time::time_duration duration_since(const boost::posix_time::ptime& time) const;

            /*! \brief Return duration since 1970-01-01 00:00:00.
                \return duration since 1970-01-01 00:00:00.  */
            boost::posix_time::time_duration duration_since_1970() const {return duration_since(boost::posix_time::ptime{{1970,1,1}});}
            /*! \brief Return seconds since 1970-01-01 00:00:00.
                \return seconds since 1970-01-01 00:00:00.  */
            auto seconds_since_1970() const {return duration_since_1970().total_seconds();}
            /*! \brief Return duration since 1993-01-01 00:00:00.  
                \return duration since 1993-01-01 00:00:00.  */
            boost::posix_time::time_duration duration_since_1993() const {return duration_since(boost::posix_time::ptime{{1993,1,1}});}
            /*! \brief Return seconds since 1993-01-01 00:00:00.  
                \return seconds since 1993-01-01 00:00:00.  */
            auto seconds_since_1993() const {return duration_since_1993().total_seconds();}
            /*! \brief Return duration since 1958-01-01 00:00:00.  
                \return duration since 1958-01-01 00:00:00.  */
            boost::posix_time::time_duration duration_since_1958() const {return duration_since(boost::posix_time::ptime{{1958,1,1}});}
            /*! \brief Return seconds since 1958-01-01 00:00:00.  
                \return seconds since 1958-01-01 00:00:00.  */
            auto seconds_since_1958() const {return duration_since_1958().total_seconds();}

            /*! \brief Get Julian date
                \return Julian date  */
            double julian_date() const {return (seconds_since_1970() / 86400.) + 2440587.5;}
            /*! \brief Get modified Julian date
                \return modified Julian date  */
            double modified_julian_date() const {return julian_date() - 2400000.5;}
            /*! \brief Get reduced Julian date
                \return reduced Julian date  */
            double reduced_julian_date() const {return julian_date() - 2400000.;}
            /*! \brief ReGet Julian date
                \return truncated Julian date  */
            double truncated_julian_date() const {return std::floor(julian_date() - 2440000.5);}

            /*!
                \brief Check if two time points are equal (clock type and time must be equal)

                \param lhs Left hand side argument
                \param rhs Right hand side argument

                \return true if the two given objects are equal
            */
            constexpr friend inline bool operator==(const TimePoint& lhs, const TimePoint& rhs){
                return lhs.clock_type_ == rhs.clock_type_ && lhs.ptime_ == rhs.ptime_;
            }
            /*!
                \brief Check if two time points are not equal (clock type or time differ)

                \param lhs Left hand side argument
                \param rhs Right hand side argument

                \return true if the two given objects are not equal
            */
            constexpr friend inline bool operator!=(const TimePoint& lhs, const TimePoint& rhs){ return !(lhs == rhs); }

            /*!
                \brief Return true if the left TimePoint is less than the right

                This is fairly nonsensical and only checks the time.  If leap
                seconds and/or time conversion would change the order, this
                function doesn't notice.

                \param lhs Left hand side argument
                \param rhs Right hand side argument

                \return true if the left hand side is less
            */
            friend inline bool operator<(const TimePoint& lhs, const TimePoint& rhs){
                return lhs.ptime_ < rhs.ptime_;
            }
            /*!
                \brief Return true if the left TimePoint is greater than the right

                This is fairly nonsensical and only checks the time.  If leap
                seconds and/or time conversion would change the order, this
                function doesn't notice.

                \param lhs Left hand side argument
                \param rhs Right hand side argument

                \return true if the left hand side is greater
            */
            friend inline bool operator> (const TimePoint& lhs, const TimePoint& rhs){ return rhs < lhs; }
            /*!
                \brief Return true if the left TimePoint is less than or equal to the right

                This is fairly nonsensical and only checks the time.  If leap
                seconds and/or time conversion would change the order, this
                function doesn't notice.

                \param lhs Left hand side argument
                \param rhs Right hand side argument

                \return true if the left hand side is less than or equal to the right
            */
            friend inline bool operator<=(const TimePoint& lhs, const TimePoint& rhs){ return !(lhs > rhs); }
            /*!
                \brief Return true if the left TimePoint is greater than or equal to the right

                This is fairly nonsensical and only checks the time.  If leap
                seconds and/or time conversion would change the order, this
                function doesn't notice.

                \param lhs Left hand side argument
                \param rhs Right hand side argument

                \return true if the left hand side is greater than or equal to the right
            */
            friend inline bool operator>=(const TimePoint& lhs, const TimePoint& rhs){ return !(lhs < rhs); }
        private:
            boost::posix_time::ptime ptime_;
            clock_type clock_type_{utc};
    };
} // namespace oc

#endif // OC_TIMEPOINT

