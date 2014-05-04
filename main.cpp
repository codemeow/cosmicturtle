#include <QCoreApplication>
#include "com/csmturtle.hpp"
#include "log/csmlogtest.hpp"

const QByteArray COM_ident("\xAA\x01\xFE\x00\x00\x00\x00\x00\x04\xFB\x02\x00\xFF\xFD\x55\xFF", 16);

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    CSMCom     csmcom;
    CSMLogTest csmlog;

    csmcom.setPortName("COM3");
    csmcom.setBaudRate(QSerialPort::Baud115200);
    csmcom.setDataBits(QSerialPort::Data8);
    csmcom.setStopBits(QSerialPort::OneStop);
    csmcom.setFlowControl(QSerialPort::NoFlowControl);

    QObject::connect(&csmcom, SIGNAL(logRead(QByteArray)),
                     &csmlog, SLOT(log_ComCSM_read(QByteArray)));
    QObject::connect(&csmcom, SIGNAL(logWrite(QByteArray)),
                     &csmlog, SLOT(log_ComCSM_write(QByteArray)));
    QObject::connect(&csmcom, SIGNAL(logWarning(QString)),
                     &csmlog, SLOT(log_ComCSM_warning(QString)));
    QObject::connect(&csmcom, SIGNAL(logTimeout()),
                     &csmlog, SLOT(log_ComCSM_timeout()));

    PreceptSet beginseq;
    PreceptSet endseq;

    PreceptArray correctbegin1;
    correctbegin1.append(PreceptByte(false, 0xAA));
    correctbegin1.append(PreceptByte(true,  0xAA));
    correctbegin1.append(PreceptByte(false, 0xAA));
    beginseq.append(correctbegin1);

    PreceptArray correctend1;
    correctend1.append(PreceptByte(false, 0x55));
    correctend1.append(PreceptByte(true,  0x55));
    correctend1.append(PreceptByte(true,  0xFF));
    PreceptArray correctend2;
    correctend2.append(PreceptByte(false, 0x55));
    correctend2.append(PreceptByte(true,  0x55));
    correctend2.append(PreceptByte(false, 0xFF));
    endseq.append(correctend1);
    endseq.append(correctend2);

    csmcom.setBeginSequence(beginseq);
    csmcom.setEndSequence(endseq);

    csmcom.bytesIn(COM_ident);

    return a.exec();
}
