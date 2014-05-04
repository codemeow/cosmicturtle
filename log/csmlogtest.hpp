#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <QObject>
#include <QByteArray>
#include <QDateTime>

#include "com/csmturtle.hpp"

class CSMLogTest : public QObject
{
    Q_OBJECT

public:
    CSMLogTest() {}
    ~CSMLogTest() {}

public slots:
    void log_ComCSM_write(QByteArray bytes)
    {
        printf("[CSMCOM] Tx (%s):\n",
               QDateTime::currentDateTime().toString(
                "hh:mm:ss.zzz").toLatin1().data());
        printf("%s\n", CSMCom::bytesToString(bytes).toLatin1().data());
    }

    void log_ComCSM_read(QByteArray bytes)
    {
        printf("[CSMCOM] Rx (%s):\n",
               QDateTime::currentDateTime().toString(
                "hh:mm:ss.zzz").toLatin1().data());
        printf("%s\n", CSMCom::bytesToString(bytes).toLatin1().data());
    }

    void log_ComCSM_timeout()
    {
        printf("[CSMCOM] Rx (%s):\nTimeout\n\n",
               QDateTime::currentDateTime().toString(
                "hh:mm:ss.zzz").toLatin1().data());
    }

    void log_ComCSM_warning(QString message)
    {
        printf("[CSMCOM] Warning (%s):\n",
               QDateTime::currentDateTime().toString(
                "hh:mm:ss.zzz").toLatin1().data());
        printf("%s\n", message.toLatin1().data());
    }
};

#endif // LOGGER_HPP
