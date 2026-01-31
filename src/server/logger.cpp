#include "logger.h"
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QStandardPaths>
#include <QMutex>
#include <QTextStream>
#include <cstdarg>

namespace suyan {
namespace log {

namespace {

QFile* g_logFile = nullptr;
QMutex g_mutex;
Level g_level = Level::Info;
QString g_moduleName = QStringLiteral("server");
qint64 g_maxFileSize = 10 * 1024 * 1024;  // 10MB
int g_maxFileCount = 5;

const char* levelToString(Level level) {
    switch (level) {
        case Level::Debug:   return "DEBUG";
        case Level::Info:    return "INFO";
        case Level::Warning: return "WARN";
        case Level::Error:   return "ERROR";
        default:             return "UNKNOWN";
    }
}

QString getLogDir() {
    QString appData = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    return appData + QStringLiteral("/SuYan/logs");
}

QString getLogFilePath() {
    return getLogDir() + QStringLiteral("/") + g_moduleName + QStringLiteral(".log");
}

void rotateIfNeeded() {
    if (!g_logFile || g_logFile->size() < g_maxFileSize) {
        return;
    }

    g_logFile->close();
    delete g_logFile;
    g_logFile = nullptr;

    QString logDir = getLogDir();
    QString baseName = g_moduleName + QStringLiteral(".log");

    for (int i = g_maxFileCount - 1; i >= 1; --i) {
        QString oldName = logDir + QStringLiteral("/") + g_moduleName + 
                          QStringLiteral(".") + QString::number(i) + QStringLiteral(".log");
        QString newName = logDir + QStringLiteral("/") + g_moduleName + 
                          QStringLiteral(".") + QString::number(i + 1) + QStringLiteral(".log");
        QFile::remove(newName);
        QFile::rename(oldName, newName);
    }

    QString currentLog = getLogFilePath();
    QString rotatedLog = logDir + QStringLiteral("/") + g_moduleName + QStringLiteral(".1.log");
    QFile::rename(currentLog, rotatedLog);

    g_logFile = new QFile(currentLog);
    g_logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
}

void writeLog(Level level, const char* fmt, va_list args) {
    if (level < g_level || !g_logFile) {
        return;
    }

    QMutexLocker locker(&g_mutex);

    rotateIfNeeded();

    if (!g_logFile || !g_logFile->isOpen()) {
        return;
    }

    char msgBuffer[4096];
    vsnprintf(msgBuffer, sizeof(msgBuffer), fmt, args);

    QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz"));
    QString line = QStringLiteral("[%1] [%2] %3\n")
        .arg(timestamp)
        .arg(QString::fromLatin1(levelToString(level)))
        .arg(QString::fromUtf8(msgBuffer));

    QTextStream stream(g_logFile);
    stream << line;
    stream.flush();
}

}  // namespace

void initialize(const wchar_t* moduleName) {
    QMutexLocker locker(&g_mutex);

    if (g_logFile) {
        return;
    }

    if (moduleName) {
        g_moduleName = QString::fromWCharArray(moduleName);
    }

    QString logDir = getLogDir();
    QDir().mkpath(logDir);

    g_logFile = new QFile(getLogFilePath());
    g_logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
}

void shutdown() {
    QMutexLocker locker(&g_mutex);

    if (g_logFile) {
        g_logFile->close();
        delete g_logFile;
        g_logFile = nullptr;
    }
}

void setLevel(Level level) {
    g_level = level;
}

void setMaxFileSize(qint64 bytes) {
    g_maxFileSize = bytes;
}

void setMaxFileCount(int count) {
    g_maxFileCount = count;
}

void debug(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    writeLog(Level::Debug, fmt, args);
    va_end(args);
}

void info(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    writeLog(Level::Info, fmt, args);
    va_end(args);
}

void warning(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    writeLog(Level::Warning, fmt, args);
    va_end(args);
}

void error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    writeLog(Level::Error, fmt, args);
    va_end(args);
}

}  // namespace log
}  // namespace suyan
