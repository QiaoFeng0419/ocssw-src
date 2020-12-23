
#ifndef OC_LOG
#define OC_LOG

#include "log/Base.hpp"
#include "log/File.hpp"
#include "log/Stream.hpp"
#include "log/FormattedStream.hpp"
#include "log/FormattedFile.hpp"

#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <gsl/gsl>

namespace oc {

/*!
    \class oc::Log

    \brief Multiple output, versatile logger

    NB: log severities are backwards; e.g., debug is numerically larger than emergency,
    but is considered lower severity.  Thus, min_severity and max_severity throughout are
    referring to severity, not numeric representation, which were copied from
    syslog's paradigm.

    \section Example
    \snippet tests/logger.cpp oc::Log test
*/
class Log {
    public:
        /*!  \brief Default constructor with no output facilities */
        Log() : Log(false){ }
        /*!
            \brief Default constructor, optionally creating default facilities
        
            \param default_loggers whether or not to add default facilities
        */
        explicit Log(bool default_loggers){
            if (default_loggers){
                add(std::make_unique<oc::FileStreamLogger>());
            }
        }
        // move constructors (facilities can't easily be moved, needs more work)
        // Log(const Log&) = default;
        // explicit Log(std::vector<std::unique_ptr<LogFacility>> targets) : targets_{std::move(targets)}{}

        /*!
            \brief Obtain a singleton of the default logger

            Thread-safe after C++11's additions of magic statics.
        */
        inline static Log& get_default(){
            static Log _instance{true};
            return _instance;
        }

        /*!
            \brief Add new log facility, using default severity filter (info severity and above)
        
            \param target Pointer to log facility to add
        */
        void add(std::unique_ptr<LogFacility> target){
            targets_.emplace_back(std::move(target));
        }
        /*!
            \brief Add new log facility with a given minimum severity and no maximum
        
            \param min_severity Minimum severity to log
            \param target New log facility to add
        */
        void add(int min_severity, std::unique_ptr<LogFacility> target){
            targets_.emplace_back(min_severity, std::move(target));
        }
        /*!
            \brief Add new log facility with a given minimum and maximum severities
        
            \param min_severity Minimum severity to log
            \param max_severity Maximum severity to log
            \param target New log facility to add
        */
        void add(int min_severity, int max_severity, std::unique_ptr<LogFacility> target){
            targets_.emplace_back(min_severity, max_severity, std::move(target));
        }

        /*!
            \brief Log a message with a given (numeric) severity
        
            Every other logging function is a wrapper around this.
        
            \param severity numeric severity of message
            \param s message to log
        */
        void log(int severity, const std::string& s){
            for (auto& t : targets_){
                if (severity <= t.min_severity() && severity >= t.max_severity()){
                    t.log_facility().log(severity, s);
                }
            }
        }
        /*!
            \brief Log a message with a given severity
        
            \param severity severity of message
            \param s message to log
        */
        void log(LogSeverity severity, const std::string& s){
            return log(static_cast<int>(severity), s);
        }
        /*!
            \brief Log a message with debug severity
        
            \param s message to log
        */
        void debug(const std::string& s){ return log(LogSeverity::debug, s); }
        /*!
            \brief Log a message with info severity
        
            \param s message to log
        */
        void info(const std::string& s){ return log(LogSeverity::info, s); }
        /*!
            \brief Log a message with notice severity
        
            \param s message to log
        */
        void notice(const std::string& s){ return log(LogSeverity::notice, s); }
        /*!
            \brief Log a message with warning severity
        
            \param s message to log
        */
        void warning(const std::string& s){ return log(LogSeverity::warning, s); }
        /*!
            \brief Log a message with error severity
        
            \param s message to log
        */
        void error(const std::string& s){ return log(LogSeverity::error, s); }
        /*!
            \brief Log a message with critical severity
        
            \param s message to log
        */
        void critical(const std::string& s){ return log(LogSeverity::critical, s); }
        /*!
            \brief Log a message with alert severity
        
            \param s message to log
        */
        void alert(const std::string& s){ return log(LogSeverity::alert, s); }
        /*!
            \brief Log a message with emergency severity
        
            \param s message to log
        */
        void emergency(const std::string& s){ return log(LogSeverity::emergency, s); }

        /*!
            \brief Log a message with warning severity
        
            \param s message to log
        */
        void warn(const std::string& s){ return log(LogSeverity::warn, s); }
        /*!
            \brief Log a message with error severity
        
            \param s message to log
        */
        void err(const std::string& s){ return log(LogSeverity::err, s); }
        /*!
            \brief Log a message with critical severity
        
            \param s message to log
        */
        void crit(const std::string& s){ return log(LogSeverity::crit, s); }
        /*!
            \brief Log a message with emergency severity
        
            \param s message to log
        */
        void emerg(const std::string& s){ return log(LogSeverity::emerg, s); }

        /*!
            \brief Check number of log facilities
        
            \return number of log facilities
        */
        size_t num_loggers() { return targets_.size(); }
        /*!
            \brief Return the default severity (only used for stream operators)
        
            \return default severity
        */
        LogSeverity severity() { return severity_; }

        /*!
            \brief set the default severity for stream operators

            \param me this logger
            \param severity new, default log severity
        
            \return this logger, for chaining
        */
        friend Log& operator<<(Log &me, LogSeverity severity){
            me.severity_ = severity;
            return me;
        }
        /*!
            \brief log a message via stream operator, using default log severity

            \param me this logger
            \param t message to log
        
            \return this logger, for chaining
        */
        friend Log& operator<<(Log& me, const std::string& t){
            me.log(me.severity_, t);
            return me;
        }
        /*!
            \brief log a non-string object via stream operator, using default log severity

            This function will use a standard stringbuf and ostream to convert the input to a string.

            \param me this logger
            \param t non-string object to log 
        
            \return this logger, for chaining
        */
        template<typename T>
        friend Log& operator<<(Log& me, const T& t){
            std::stringbuf buf;
            std::ostream out{&buf};
            out << t;
            me.log(me.severity_, buf.str());
            return me;
        }

    private:
        std::vector<LogTarget> targets_{};
        LogSeverity severity_{LogSeverity::info};

};
} // namespace oc

#endif // OC_LOG
