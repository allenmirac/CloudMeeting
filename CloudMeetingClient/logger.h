#ifndef LOGGER_H
#define LOGGER_H

#include <QDebug>

#define LOG_DEBUG qDebug() << "[" << __FILE__ << __FUNCTION__ << __LINE__ << "] "
#define LOG_INFO qInfo() << "[" << __FILE__ << __FUNCTION__ << __LINE__ << "] "
#define LOG_WARN qWarning() << "[" << __FILE__ << __FUNCTION__ << __LINE__ << "] "
#define LOG_CRIT qCritical() << "[" << __FILE__ << __FUNCTION__ << __LINE__ << "] "

void initLog(const QString &logPath = QStringLiteral("Log"), int logMaxCount = 1024, bool async = true);



#endif // LOGGER_H
