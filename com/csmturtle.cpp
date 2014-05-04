#include <QDateTime>
#include "csmturtle.hpp"

const QString CT_BAUDRATE_ERROR = QString(QObject::tr("Baud Rate hasn't been set"));
const QString CT_PARITY_ERROR   = QString(QObject::tr("Parity hasn't been set."));
const QString CT_DATABITS_ERROR = QString(QObject::tr("Data bits hasn't been set."));
const QString CT_STOPBITS_ERROR = QString(QObject::tr("Stop bits hasn't been set."));
const QString CT_FLOWSET_ERROR  = QString(QObject::tr("Flow control hasn't been set."));
const QString CT_CANTOPEN_ERROR = QString(QObject::tr("Port hasn't been opened"));

/* CSMCom */

CSMCom::CSMCom(QString portName, qint32 baudRate)
{
    beginseq.clear();

    PreceptSet endset;
    PreceptArray endarr;
    endarr.append(CT_DEFAULT_ENDSEQ);
    endset.append(endarr);
    endseq.append(endset);
    tpb = CT_DEFAULT_TPB;
    port.setPortName(portName);
    if (port.open(QIODevice::ReadWrite))
    {
        if (!port.setBaudRate(baudRate))
            emit logWarning(CT_BAUDRATE_ERROR);
        if (!port.setParity(QSerialPort::NoParity))
            emit logWarning(CT_PARITY_ERROR);
        if (!port.setDataBits(QSerialPort::Data8))
            emit logWarning(CT_DATABITS_ERROR);
        if (!port.setStopBits(QSerialPort::OneStop))
            emit logWarning(CT_STOPBITS_ERROR);
        if (!port.setFlowControl(QSerialPort::NoFlowControl))
            emit logWarning(CT_FLOWSET_ERROR);
    }
    else
    {
        emit logWarning(CT_CANTOPEN_ERROR);
    }

    spinner = new CSMSpinner(&port, &beginseq, &endseq, &tpb, this);
    connect(spinner, SIGNAL(finished()),
            spinner, SLOT(deleteLater()));
    connect(spinner, SIGNAL(bytesOut(QByteArray)),
            this,    SLOT(bytesReady(QByteArray)));
    spinner->start();
}

CSMCom::~CSMCom()
{
    spinner->quit();
    spinner->wait();
    port.close();
}

void CSMCom::bytesIn(QByteArray bytes, qint32 requestedTimeout)
{
    QMetaObject::invokeMethod(spinner,
                              "bytesToWrite",
                              Qt::QueuedConnection,
                              Q_ARG(QByteArray, bytes),
                              Q_ARG(qint32, requestedTimeout));
}

QString CSMCom::portName()
{
       return port.portName();
}

bool CSMCom::setPortName(QString portName)
{
       port.close();
       port.setPortName(portName);
       if (!port.open(QIODevice::ReadWrite))
       {
           emit logWarning(CT_CANTOPEN_ERROR);
           return false;
       }
       else
       {
           return true;
       }
}

qint32 CSMCom::baudRate()
{
       return port.baudRate();
}

bool CSMCom::setBaudRate(qint32 baudRate)
{
    if (port.isOpen())
    {
        if (port.setBaudRate(baudRate))
        {
            return true;
        }
        else
        {
            emit logWarning(CT_BAUDRATE_ERROR);
            return false;
        }
    }
    else
    {
        emit logWarning(CT_CANTOPEN_ERROR);
        return false;
    }
}

PreceptSet CSMCom::beginSequence()
{
    return beginseq;
}

bool CSMCom::setBeginSequence(PreceptSet newseq)
{
    if (newseq.length() > 0)
    {
        beginseq.clear();
        beginseq.append(newseq);

        return true;
    }
    else
    {
        return false;
    }
}

PreceptSet CSMCom::endSequence()
{
    return endseq;
}

bool CSMCom::setEndSequence(PreceptSet newseq)
{
    if (newseq.length() > 0)
    {
        endseq.clear();
        endseq.append(newseq);

        return true;
    }
    else
    {
        return false;
    }
}

qreal CSMCom::timeoutPerByte()
{
    return tpb;
}

bool CSMCom::setTimeoutPerByte(qreal timeout)
{
    if (timeout > 0)
    {
        tpb = timeout;
        return true;
    }
    else
    {
        return false;
    }
}

bool CSMCom::setParity(QSerialPort::Parity parity)
{
    if (!port.setParity(parity))
    {
        emit logWarning(CT_PARITY_ERROR);
        return false;
    }
    else
    {
        return true;
    }
}

QSerialPort::Parity CSMCom::parity()
{
    return port.parity();
}

bool CSMCom::setDataBits(QSerialPort::DataBits dataBits)
{
    if (!port.setDataBits(dataBits))
    {
        emit logWarning(CT_DATABITS_ERROR);
        return false;
    }
    else
    {
        return true;
    }
}

QSerialPort::DataBits CSMCom::dataBits()
{
    return port.dataBits();
}

bool CSMCom::setStopBits(QSerialPort::StopBits stopBits)
{
    if (!port.setStopBits(stopBits))
    {
        emit logWarning(CT_STOPBITS_ERROR);
        return false;
    }
    else
    {
        return true;
    }
}

QSerialPort::StopBits CSMCom::stopBits()
{
    return port.stopBits();
}

bool CSMCom::setFlowControl(QSerialPort::FlowControl flow)
{
    if (!port.setFlowControl(flow))
    {
        emit logWarning(CT_FLOWSET_ERROR);
        return false;
    }
    else
    {
        return true;
    }
}

QSerialPort::FlowControl CSMCom::flowControl()
{
    return port.flowControl();
}

bool CSMCom::isConnected()
{
    return port.isOpen();
}

QString CSMCom::rulesToString(PreceptSet rules)
{
    QString result;

    for (int i = 0; i < rules.length(); i++)
    {
        result.append(QString("Sequence #%1\n").arg(i));
        result.append(QString("["));
        for (int j = 0; j < rules.at(i).length(); j++)
        {
            result.append(QString("(%1, %2),")
                .arg(rules.at(i).at(j).exactly)
                .arg(rules.at(i).at(j).byte, 2, 16));
        }
        result.chop(1);
        result.append(QString("]\n"));
    }

    return result;
}

QString CSMCom::bytesToString(QByteArray bytes)
{
    QString result;

    for (qint32 i = 0; i < bytes.length(); i++)
    {
        result.append(
            QString("%1 ").arg(
                bytes.at(i) & 0xFF, 2, 0x10, QChar('0')).toUpper());
        if (i % 16 == 15) result.append('\n');
    }
    result.append('\n');

    return result;
}

void CSMCom::bytesReady(QByteArray bytes)
{
    emit logRead(bytes);
    emit bytesOut(bytes);
}

/* CSMSpinner */

CSMSpinner::CSMSpinner(QSerialPort * port,
                       PreceptSet  * beginseqptr,
                       PreceptSet  * endseqptr,
                       qreal       * tpb,
                       CSMCom      * parentptr)
{
    terminated = false;
    portcopy   = port;
    beginseq   = beginseqptr;
    endseq     = endseqptr;
    busy       = false;
    tpbcopy    = tpb;
    timeleft   = 0;
    parent     = parentptr;
    incoming.clear();
    sendqueue.clear();
}

CSMSpinner::~CSMSpinner()
{

}

void CSMSpinner::run()
{
    qint32 beginpos;
    qint32 endpos;
    qint32 rulebeg;
    qint32 ruleend;

    while (!terminated)
    {
        /* Timeout event */
        if ((busy) && (timeout.elapsed() > timeleft))
        {
            busy = false;
            emit parent->timeout();
            emit parent->logTimeout();
            incoming.clear();
        }

        /* Process queue */
        if ((!(busy)) && (!sendqueue.isEmpty()))
        {
            emit parent->bytesIn(sendqueue.first());
            sendqueue.removeFirst();
        }

        /* Read available bytes */
        if (portcopy->bytesAvailable())
        {
            incoming.append(portcopy->readAll());
        }

        /* Unload the processor */
        msleep(CT_DEFAULT_RINGPERIOD);

        /* Send ready signal when package signature is found */
        if (((beginpos = sequenceBeginSearch(&beginpos, &rulebeg)) > -1) &&
            ((endpos   = sequenceEndSearch  (&endpos,   &ruleend)) > -1) &&
            ((beginpos < endpos)))
        {
            busy = false;
            emit bytesOut(incoming.mid(beginpos, endpos + endseq->at(ruleend).length() + 1));
            incoming.remove(0, endpos - beginpos + endseq->at(ruleend).length() + 1);
        }
    }
}

qint32 CSMSpinner::ruleApplier(PreceptSet * rules, QByteArray bytes,
                               qint32     * pos,   qint32 * rule)
{
    if (rules->length() == 0)
    {
        *pos  = 0;
        *rule = -1;
        return *pos;
    }

    int  counter;
    for (qint32 i = 0; i < rules->length(); i++)
    {
        for (qint32 j = -rules->at(i).length() + 1;
             j < bytes.length() - rules->at(i).length() + 1;
             j++)
        {
            counter = 0;

            for (qint32 k = 0; k < rules->at(i).length(); k++)
            {
                if ((j + k < 0) || (j + k >= bytes.length()))
                {
                    if (!rules->at(i)[k].exactly)
                    {
                        counter++;
                    }
                    else
                    {
                        break;
                    }
                }
                else
                {
                    if (((!rules->at(i)[k].exactly) && (rules->at(i)[k].byte != (bytes[j + k] & 0xFF))) ||
                        (( rules->at(i)[k].exactly) && (rules->at(i)[k].byte == (bytes[j + k] & 0xFF))))
                    {
                        counter++;
                    }
                    else
                    {
                        break;
                    }
                }
            }

            if (counter == rules->at(i).length())
            {
                if (j < 0) j = 0;
                *pos  = j;
                *rule = i;

                return *pos;
            }
        }
    }

    *pos  = -1;
    *rule = -1;
    return *pos;
}

qint32 CSMSpinner::sequenceBeginSearch(qint32 * pos, qint32 * rule)
{
    return ruleApplier(beginseq, incoming, pos, rule);
}

qint32 CSMSpinner::sequenceEndSearch(qint32 * pos, qint32 * rule)
{
    return ruleApplier(endseq, incoming, pos, rule);
}

void CSMSpinner::bytesToWrite(QByteArray bytes, qint32 requestedTimeout)
{
    if (!busy)
    {
        busy = true;
        emit parent->logWrite(bytes);
        portcopy->write(bytes);
        if (requestedTimeout == -1)
        {
            timeleft = ((float)bytes.length() * (float)*tpbcopy);
        }
        else
        {
            timeleft = requestedTimeout;
        }
        timeout.restart();
    }
    else
    {
        sendqueue.append(bytes);
    }
}
