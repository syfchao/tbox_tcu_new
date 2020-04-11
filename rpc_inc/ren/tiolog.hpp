/***
      __   __  _  _  __     __    ___
     / _\ (  )( \/ )(  )   /  \  / __)
    /    \ )(  )  ( / (_/\(  O )( (_ \
    \_/\_/(__)(_/\_)\____/ \__/  \___/
    version 1.2.4
    tiot log by chenjianwen
***/

#ifndef TIO_LOG_HPP
#define TIO_LOG_HPP


#include <algorithm>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <vector>


#define tioLog_INTERNAL__FUNC __func__


#define tioLog_INTERNAL__LOG_SEVERITY(SEVERITY_) std::clog << static_cast<tioLog::Severity>(SEVERITY_)
#define tioLog_INTERNAL__LOG_SEVERITY_TAG(SEVERITY_, TAG_) std::clog << static_cast<tioLog::Severity>(SEVERITY_) << TAG(TAG_)

#define tioLog_INTERNAL__ONE_COLOR(FG_) tioLog::Color::FG_
#define tioLog_INTERNAL__TWO_COLOR(FG_, BG_) tioLog::TextColor(tioLog::Color::FG_, tioLog::Color::BG_)

// https://stackoverflow.com/questions/3046889/optional-parameters-with-c-macros
#define tioLog_INTERNAL__VAR_PARM(PARAM1_, PARAM2_, FUNC_, ...) FUNC_
#define tioLog_INTERNAL__LOG_MACRO_CHOOSER(...) tioLog_INTERNAL__VAR_PARM(__VA_ARGS__, tioLog_INTERNAL__LOG_SEVERITY_TAG, tioLog_INTERNAL__LOG_SEVERITY, )
#define tioLog_INTERNAL__COLOR_MACRO_CHOOSER(...) tioLog_INTERNAL__VAR_PARM(__VA_ARGS__, tioLog_INTERNAL__TWO_COLOR, tioLog_INTERNAL__ONE_COLOR, )


#define LOG(...) tioLog_INTERNAL__LOG_MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__) << TIMESTAMP << FUNC
#define SLOG(...) tioLog_INTERNAL__LOG_MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__) << TIMESTAMP << SPECIAL << FUNC

#define COLOR(...) tioLog_INTERNAL__COLOR_MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__)

#define FUNC tioLog::Function(tioLog_INTERNAL__FUNC, __FILE__, __LINE__)
#define TAG tioLog::Tag
#define COND tioLog::Conditional
#define SPECIAL tioLog::Type::special
#define TIMESTAMP tioLog::Timestamp(std::chrono::system_clock::now())


#define logTrace        LOG(TRACE)
#define logDebug        LOG(DEBUG)
#define logInfo         LOG(INFO)
#define logNotice       LOG(NOTICE)
#define logWarn         LOG(WARNING)
#define logError        LOG(ERROR)
#define logFatal        LOG(FATAL)

/*
#define logTrace()      LOG(TRACE)
#define logDebug()      LOG(DEBUG)
#define logInfo()       LOG(INFO)
#define logNotice()     LOG(NOTICE)
#define logWarn()       LOG(WARNING)
#define logError()      LOG(ERROR)
#define logFatal()      LOG(FATAL)
*/

/**
 * @brief
 * Severity of the log message
 */
enum SEVERITY
{
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    NOTICE = 3,
    WARNING = 4,
    ERROR = 5,
    FATAL = 6
};

namespace tioLog
{

/**
 * @brief
 * Severity of the log message
 *
 * Mandatory parameter for the LOG macro
 */
enum class Severity : std::int8_t
{
    trace = SEVERITY::TRACE,
    debug = SEVERITY::DEBUG,
    info = SEVERITY::INFO,
    notice = SEVERITY::NOTICE,
    warning = SEVERITY::WARNING,
    error = SEVERITY::ERROR,
    fatal = SEVERITY::FATAL
};

enum class Type
{
    normal,
    special,
    all
};

/**
 * @brief
 * Color used for console colors
 */
enum class Color
{
    none = 0,
    NONE = 0,
    black = 1,
    BLACK = 1,
    red = 2,
    RED = 2,
    green = 3,
    GREEN = 3,
    yellow = 4,
    YELLOW = 4,
    blue = 5,
    BLUE = 5,
    magenta = 6,
    MAGENTA = 6,
    cyan = 7,
    CYAN = 7,
    white = 8,
    WHITE = 8
};

struct TextColor
{
    TextColor(Color foreground = Color::none, Color background = Color::none) : foreground(foreground), background(background)
    {
    }

    Color foreground;
    Color background;
};

struct Conditional
{
    Conditional() : Conditional(true)
    {
    }

    Conditional(bool value) : is_true_(value)
    {
    }

    void set(bool value)
    {
        is_true_ = value;
    }

    bool is_true() const
    {
        return is_true_;
    }

private:
    bool is_true_;
};

struct Timestamp
{
    using time_point_sys_clock = std::chrono::time_point<std::chrono::system_clock>;

    Timestamp(std::nullptr_t) : is_null_(true)
    {
    }

    Timestamp() : Timestamp(nullptr)
    {
    }

    Timestamp(const time_point_sys_clock& time_point) : time_point(time_point), is_null_(false)
    {
    }

    Timestamp(time_point_sys_clock&& time_point) : time_point(std::move(time_point)), is_null_(false)
    {
    }

    virtual ~Timestamp() = default;

    explicit operator bool() const
    {
        return !is_null_;
    }

    /// strftime format + proprietary "#ms" for milliseconds
//    std::string to_string(const std::string& format = "%Y-%m-%d %H-%M-%S.#ms") const
    //fpp 20200411 mod
    std::string to_string(const std::string& format = "%Y%m%d %H:%M:%S.#ms") const
    {
        std::time_t now_c = std::chrono::system_clock::to_time_t(time_point);
        struct ::tm now_tm = localtime_xp(now_c);
        char buffer[256];
        strftime(buffer, sizeof buffer, format.c_str(), &now_tm);
        std::string result(buffer);
        size_t pos = result.find("#ms");
        if (pos != std::string::npos)
        {
            int ms_part = std::chrono::time_point_cast<std::chrono::milliseconds>(time_point).time_since_epoch().count() % 1000;
            char ms_str[4];
            if (snprintf(ms_str, 4, "%03d", ms_part) >= 0)
                result.replace(pos, 3, ms_str);
        }
        return result;
    }

    time_point_sys_clock time_point;

private:
    bool is_null_;

    inline std::tm localtime_xp(std::time_t timer) const
    {
        std::tm bt;
#if defined(__unix__)
        localtime_r(&timer, &bt);
#elif defined(_MSC_VER)
        localtime_s(&bt, &timer);
#else
        static std::mutex mtx;
        std::lock_guard<std::mutex> lock(mtx);
        bt = *std::localtime(&timer);
#endif
        return bt;
    }
};

/**
 * @brief
 * Tag (string) for log line
 */
struct Tag
{
    Tag(std::nullptr_t) : text(""), is_null_(true)
    {
    }

    Tag() : Tag(nullptr)
    {
    }

    Tag(const std::string& text) : text(text), is_null_(false)
    {
    }

    Tag(std::string&& text) : text(std::move(text)), is_null_(false)
    {
    }

    virtual ~Tag() = default;

    explicit operator bool() const
    {
        return !is_null_;
    }

    std::string text;

private:
    bool is_null_;
};

/**
 * @brief
 * Capture function, file and line number of the log line
 */
struct Function
{
    Function(const std::string& name, const std::string& file, size_t line) : name(name), file(file), line(line), is_null_(false)
    {
    }

    Function(std::string&& name, std::string&& file, size_t line) : name(std::move(name)), file(std::move(file)), line(line), is_null_(false)
    {
    }

    Function(std::nullptr_t) : name(""), file(""), line(0), is_null_(true)
    {
    }

    Function() : Function(nullptr)
    {
    }

    virtual ~Function() = default;

    explicit operator bool() const
    {
        return !is_null_;
    }

    std::string name;
    std::string file;
    size_t line;

private:
    bool is_null_;
};

struct Metadata
{
    Metadata() : severity(Severity::trace), tag(nullptr), type(Type::normal), function(nullptr), timestamp(nullptr)
    {
    }

    Severity severity;
    Tag tag;
    Type type;
    Function function;
    Timestamp timestamp;
};

/**
 * @brief
 * Abstract log sink
 *
 * All log sinks must inherit from this Sink
 */
struct Sink
{
    Sink(Severity severity, Type type) : severity(severity), sink_type_(type)
    {
    }

    virtual ~Sink() = default;

    virtual void log(const Metadata& metadata, const std::string& message) = 0;
    virtual Type get_type() const
    {
        return sink_type_;
    }

    virtual Sink& set_type(Type sink_type)
    {
        sink_type_ = sink_type;
        return *this;
    }

    Severity severity;

protected:
    Type sink_type_;
};

/// ostream operators << for the meta data structs
static std::ostream& operator<<(std::ostream& os, const Severity& log_severity);
static std::ostream& operator<<(std::ostream& os, const Type& log_type);
static std::ostream& operator<<(std::ostream& os, const Timestamp& timestamp);
static std::ostream& operator<<(std::ostream& os, const Tag& tag);
static std::ostream& operator<<(std::ostream& os, const Function& function);
static std::ostream& operator<<(std::ostream& os, const Conditional& conditional);
static std::ostream& operator<<(std::ostream& os, const Color& color);
static std::ostream& operator<<(std::ostream& os, const TextColor& text_color);

using log_sink_ptr = std::shared_ptr<Sink>;

class Log : public std::basic_streambuf<char, std::char_traits<char>>
{
public:
    static Log& instance()
    {
        static Log instance_;
        return instance_;
    }

    /// Without "init" every LOG(X) will simply go to clog
    static void init(const std::vector<log_sink_ptr> log_sinks = {})
    {
        Log::instance().log_sinks_.clear();

        for (const auto& sink : log_sinks)
            Log::instance().add_logsink(sink);
    }

    template <typename T, typename... Ts>
    static std::shared_ptr<T> init(Ts&&... params)
    {
        std::shared_ptr<T> sink = Log::instance().add_logsink<T>(std::forward<Ts>(params)...);
        init({sink});
        return sink;
    }

    template <typename T, typename... Ts>
    std::shared_ptr<T> add_logsink(Ts&&... params)
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        static_assert(std::is_base_of<Sink, typename std::decay<T>::type>::value, "type T must be a Sink");
        std::shared_ptr<T> sink = std::make_shared<T>(std::forward<Ts>(params)...);
        log_sinks_.push_back(sink);
        return sink;
    }

    void add_logsink(const log_sink_ptr& sink)
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        log_sinks_.push_back(sink);
    }

    void remove_logsink(const log_sink_ptr& sink)
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        log_sinks_.erase(std::remove(log_sinks_.begin(), log_sinks_.end(), sink), log_sinks_.end());
    }

    static std::string to_string(Severity logSeverity)
    {
        switch (logSeverity)
        {
            case Severity::trace:
                return "Trace";
            case Severity::debug:
                return "Debug";
            case Severity::info:
                return "Info";
            case Severity::notice:
                return "Notice";
            case Severity::warning:
                return "Warn";
            case Severity::error:
                return "Err";
            case Severity::fatal:
                return "Fatal";
            default:
                std::stringstream ss;
                ss << logSeverity;
                return ss.str();
        }
    }

protected:
    Log() noexcept
    {
        std::clog.rdbuf(this);
        std::clog << Severity() << Type::normal << Tag() << Function() << Conditional() << tioLog::Color::NONE << std::flush;
    }

    virtual ~Log()
    {
        sync();
    }

    int sync() override
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        if (!buffer_.str().empty())
        {
            if (conditional_.is_true())
            {
                for (const auto& sink : log_sinks_)
                {
                    if ((metadata_.type == Type::all) || (sink->get_type() == Type::all) || (metadata_.type == sink->get_type()))
                        if (metadata_.severity >= sink->severity)
                            sink->log(metadata_, buffer_.str());
                }
            }
            buffer_.str("");
            buffer_.clear();
        }

        return 0;
    }

    int overflow(int c) override
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        if (c != EOF)
        {
            if (c == '\n')
                sync();
            else
                buffer_ << static_cast<char>(c);
        }
        else
        {
            sync();
        }
        return c;
    }

private:
    friend std::ostream& operator<<(std::ostream& os, const Severity& log_severity);
    friend std::ostream& operator<<(std::ostream& os, const Type& log_type);
    friend std::ostream& operator<<(std::ostream& os, const Timestamp& timestamp);
    friend std::ostream& operator<<(std::ostream& os, const Tag& tag);
    friend std::ostream& operator<<(std::ostream& os, const Function& function);
    friend std::ostream& operator<<(std::ostream& os, const Conditional& conditional);

    std::stringstream buffer_;
    Metadata metadata_;
    Conditional conditional_;
    std::vector<log_sink_ptr> log_sinks_;
    std::recursive_mutex mutex_;
};

struct SinkFormat : public Sink
{
    SinkFormat(Severity severity, Type type, const std::string& format) : Sink(severity, type), format_(format)
    {
    }

    virtual void set_format(const std::string& format)
    {
        format_ = format;
    }

    void log(const Metadata& metadata, const std::string& message) override = 0;

protected:
    virtual void do_log(std::ostream& stream, const Metadata& metadata, const std::string& message) const
    {
        std::string result = format_;
        if (metadata.timestamp)
            result = metadata.timestamp.to_string(result);

        size_t pos = result.find("#severity");
        if (pos != std::string::npos)
            result.replace(pos, 9, Log::to_string(metadata.severity));

        pos = result.find("#tag_func");
        if (pos != std::string::npos)
            result.replace(pos, 9, metadata.tag ? metadata.tag.text : (metadata.function ? metadata.function.name : "log"));

        pos = result.find("#tag");
        if (pos != std::string::npos)
            result.replace(pos, 4, metadata.tag ? metadata.tag.text : "");

        pos = result.find("#function");
        if (pos != std::string::npos)
            result.replace(pos, 9, metadata.function ? metadata.function.name : "");

        pos = result.find("#message");
        if (pos != std::string::npos)
        {
            result.replace(pos, 8, message);
            stream << result << std::endl;
        }
        else
        {
            if (result.empty() || (result.back() == ' '))
                stream << result << message << std::endl;
            else
                stream << result << " " << message << std::endl;
        }
    }

    std::string format_;
};

/**
 * @brief
 * Formatted logging to cout
 */
struct SinkCout : public SinkFormat
{
    SinkCout(Severity severity, Type type, const std::string& format = "%Y-%m-%d %H-%M-%S.#ms [#severity] (#tag_func)") : SinkFormat(severity, type, format)
    {
    }

    void log(const Metadata& metadata, const std::string& message) override
    {
        do_log(std::cout, metadata, message);
    }
};

/**
 * @brief
 * Formatted logging to cerr
 */
struct SinkCerr : public SinkFormat
{
    SinkCerr(Severity severity, Type type, const std::string& format = "%Y-%m-%d %H-%M-%S.#ms [#severity] (#tag_func)") : SinkFormat(severity, type, format)
    {
    }

    void log(const Metadata& metadata, const std::string& message) override
    {
        do_log(std::cerr, metadata, message);
    }
};

/**
 * @brief
 * Formatted logging to file
 */
struct SinkFile : public SinkFormat
{
    SinkFile(Severity severity, Type type, const std::string& filename, const std::string& format = "%Y-%m-%d %H-%M-%S.#ms [#severity] (#tag_func)")
        : SinkFormat(severity, type, format)
    {
        ofs.open(filename.c_str(), std::ofstream::out | std::ofstream::trunc);
    }

    ~SinkFile() override
    {
        ofs.close();
    }

    void log(const Metadata& metadata, const std::string& message) override
    {
        do_log(ofs, metadata, message);
    }

protected:
    mutable std::ofstream ofs;
};


struct SinkNative : public Sink
{
    SinkNative(const std::string& ident, Severity severity, Type type = Type::all) : Sink(severity, type), log_sink_(nullptr), ident_(ident)
    {

        /// will not throw or something. Use "get_logger()" to check for success
        log_sink_ = nullptr;

    }

    virtual log_sink_ptr get_logger()
    {
        return log_sink_;
    }

    void log(const Metadata& metadata, const std::string& message) override
    {
        if (log_sink_ != nullptr)
            log_sink_->log(metadata, message);
    }

protected:
    log_sink_ptr log_sink_;
    std::string ident_;
};


struct SinkCallback : public Sink
{
    using callback_fun = std::function<void(const Metadata& metadata, const std::string& message)>;

    SinkCallback(Severity severity, Type type, callback_fun callback) : Sink(severity, type), callback_(callback)
    {
    }

    void log(const Metadata& metadata, const std::string& message) override
    {
        if (callback_)
            callback_(metadata, message);
    }

private:
    callback_fun callback_;
};

static std::ostream& operator<<(std::ostream& os, const Severity& log_severity)
{
    Log* log = dynamic_cast<Log*>(os.rdbuf());
    if (log != nullptr)
    {
        std::lock_guard<std::recursive_mutex> lock(log->mutex_);
        if (log->metadata_.severity != log_severity)
        {
            log->sync();
            log->metadata_.severity = log_severity;
            log->metadata_.type = Type::normal;
            log->metadata_.timestamp = nullptr;
            log->metadata_.tag = nullptr;
            log->metadata_.function = nullptr;
            log->conditional_.set(true);
        }
    }
    else
    {
        os << Log::to_string(log_severity);
    }
    return os;
}

static std::ostream& operator<<(std::ostream& os, const Type& log_type)
{
    Log* log = dynamic_cast<Log*>(os.rdbuf());
    if (log != nullptr)
    {
        std::lock_guard<std::recursive_mutex> lock(log->mutex_);
        log->metadata_.type = log_type;
    }
    return os;
}

static std::ostream& operator<<(std::ostream& os, const Timestamp& timestamp)
{
    Log* log = dynamic_cast<Log*>(os.rdbuf());
    if (log != nullptr)
    {
        std::lock_guard<std::recursive_mutex> lock(log->mutex_);
        log->metadata_.timestamp = timestamp;
    }
    else if (timestamp)
    {
        os << timestamp.to_string();
    }
    return os;
}

static std::ostream& operator<<(std::ostream& os, const Tag& tag)
{
    Log* log = dynamic_cast<Log*>(os.rdbuf());
    if (log != nullptr)
    {
        std::lock_guard<std::recursive_mutex> lock(log->mutex_);
        log->metadata_.tag = tag;
    }
    else if (tag)
    {
        os << tag.text;
    }
    return os;
}

static std::ostream& operator<<(std::ostream& os, const Function& function)
{
    Log* log = dynamic_cast<Log*>(os.rdbuf());
    if (log != nullptr)
    {
        std::lock_guard<std::recursive_mutex> lock(log->mutex_);
        log->metadata_.function = function;
    }
    else if (function)
    {
        os << function.name;
    }
    return os;
}

static std::ostream& operator<<(std::ostream& os, const Conditional& conditional)
{
    Log* log = dynamic_cast<Log*>(os.rdbuf());
    if (log != nullptr)
    {
        std::lock_guard<std::recursive_mutex> lock(log->mutex_);
        log->conditional_.set(conditional.is_true());
    }
    return os;
}

static std::ostream& operator<<(std::ostream& os, const TextColor& text_color)
{
    os << "\033[";
    if ((text_color.foreground == Color::none) && (text_color.background == Color::none))
        os << "0"; // reset colors if no params

    if (text_color.foreground != Color::none)
    {
        os << 29 + static_cast<int>(text_color.foreground);
        if (text_color.background != Color::none)
            os << ";";
    }
    if (text_color.background != Color::none)
        os << 39 + static_cast<int>(text_color.background);
    os << "m";

    return os;
}

static std::ostream& operator<<(std::ostream& os, const Color& color)
{
    os << TextColor(color);
    return os;
}

} // namespace tioLog




#endif // TIO_LOG_HPP
