#ifndef CSMTURTLE_HPP
#define CSMTURTLE_HPP

/*! \file csmturtle.hpp
 *  \brief Главный заголовочный файл проекта Cosmic Turtle
 *
 * Данный файл содержит сигнатуры классов CSMCom и CSMSpinner.
 *
 *  Класс CSMCom предназначен для использования в качестве средства доступа
 * к COM-порту с возможностью стакинга команд в очередь, вырезания из потока
 * пакетов по заданным параметрам.
 *
 *  Класс CSMSpinner является вспомогательным и не должен быть использован за
 * пределами данного проекта.
 *
 *  Работа с устройством одновременно ведется в двух режимах:
 * - Работа в режиме прослушивания потока.
 *   + В данном режиме постоянно опрашивается COM-порт на предмет поступивших
 *     данных. В случае обнаружения сигнатуры конца (обязательно) и начала
 *     (если задано) пакета класс CSMCom испустит сигнал bytesOut.
 * - Работа в режиме "запрос-ответ".
 *   + В данном режиме на каждое отправленное сообщение предполагается ответ. В
 *     случае, если ответа не поступает до истечения времени таймаута, будет
 *     испущен сигнал bytesTimeout.
 *
 *  Для начала работы с проектом необходимо выполнить следующие обязательные
 * действия:
 *
 * - Установить значение endSequence.
 *
 *  Также доступны следующие необязательные опции:
 *
 * - Установка значения beginSequence.
 * - Подключение к слоту bytesIn.
 * - Подключение к сигналу bytesTimeout.
 * - Подключение к сигналу bytesOut.
 * - Подключение к сигналу logOut.
 * - Подключение к сигналу logIn.
 * - Подключение к сигналу logTimeout.
 *
 *  \author Алексей Шишкин
 *  \date   26.04.2014
 */

#include <QSerialPort>
#include <QList>
#include <QVector>
#include <QObject>
#include <QThread>
#include <QTime>

class CSMSpinner;

/*!
 * \brief Структура, определяющая один байт правила
 */
struct PreceptByte
{
    /*!
     * \brief Флаг выбора байта. Значение TRUE - использовать байт byte,
     * значение FALSE - использовать любой байт кроме byte.
     */
    bool     exactly;
    /*!
     * \brief Значение байта, задающего правило.
     */
    quint8   byte;

    /*!
     * \brief Конструктор по умолчанию для обеспечения компиляции кода.
     */
    PreceptByte() {}
    /*!
     * \brief Основной инициализирующий конструктор
     * \param flag Флаг выбора байта
     * \param val Значение байта
     */
    PreceptByte(bool flag, quint8 val) : exactly(flag), byte(val) {}
};
/*!
 * \brief Shortcut для описания последовательности.
 */
typedef QVector<PreceptByte > PreceptArray;
/*!
 * \brief Short для задания набора последовательностей.
 */
typedef QList  <PreceptArray> PreceptSet;

/*!
 *  \brief Порт, открываемый по умолчанию.
 *
 *  Используется в конструкторе класса.
 */
#ifdef WIN32
#define CT_DEFAULT_PORTNAME "//./COM1"
#else
#define CT_DEFAULT_PORTNAME "/dev/tty1"
#endif
/*!
 *  \brief Скорость порта по умолчанию.
 *
 *  Используется в конструкторе класса.
 */
#define CT_DEFAULT_BAUDRATE QSerialPort::Baud115200
/*!
 *  \brief Значение завершающей пакет последовательности
 * по умолчанию
 *
 *  Используется в процессе инициализации класса.
 */
#define CT_DEFAULT_ENDSEQ PreceptByte(true,  0xFF)
/*!
 *  \brief Значение по умолчанию для коэффициента таймаута
 *
 *  Используется в процессе инициализации класса. В мс.
 */
#define CT_DEFAULT_TPB 500.0
/*!
 *  \brief Значение по умолчанию для частоты опроса кольца
 *
 *  Используется в while цикле класса CSMSpinner
 */
#define CT_DEFAULT_RINGPERIOD 100

/*!
 * \brief Класс работы с COM-портом
 *
 * Использует QSerialPort для обеспечения кроссплатформенного доступа к COM-
 * порту.
 */
class CSMCom : public QObject
{
    Q_OBJECT

public:
    /*!
     *  \brief Конструктор класса.
     *
     *  В случае опущенных входных параметров создается соединение с параметрами
     *  по умолчанию.
     *  \param portName Имя порта. Для Win-систем <b>//./COMXXX</b>,
     * для *nix-систем <b>/dev/ttyXX</b>
     *  \param baudRate Скорость порта. См. <b>QSerialPort::BaudRateXXXXX</b>
     *
     * \see CT_DEFAULT_PORTNAME
     * \see CT_DEFAULT_BAUDRATE
     */
    CSMCom(QString portName = CT_DEFAULT_PORTNAME,
           qint32  baudRate = CT_DEFAULT_BAUDRATE);
    /*!
     *  \brief Деструктор класса.
     *
     *  При вызове деструктора активное соединение будет разорвано,
     * текущий накопительный буфер будет очищен. Если в нем содержались
     * данные, то они будут потеряны.
     */
    ~CSMCom();

public slots:
     /*!
      *  \brief Слот записи потока байт.
      *
      *  Данный слот позволяет записать поток байт в COM-port. В случае, если
      * соединение не было установлено, данные будут потеряны. После успешной
      * отправки данных через некоторое время будет испущен либо сигнал
      * bytesЩге либо сигнал timeout.
      *  До момента, пока не будет испущен ответный сигнал вместо физической
      * отправки данных они будут помещены в очередь сообщений.
      *
      * \todo Обсудить возможность отложенной записи в случае отсутствия устройства,
      * но наличия данных для записи.
      *
      * \param bytes Байтовая последовательность для записи.
      * \param requestedTimeout Требуемый таймаут. По умолчанию вычисляется
      * по коэффициенту tpb.
      */
     void bytesIn(QByteArray bytes, qint32 requestedTimeout = -1);
signals:
     /*!
      *  \brief Сигнал полученных данных.
      *
      *  Данный сигнал будет испущен, когда в накопительном буфере COM-порта
      * появится корректный с точки зрения протокола пакет или при достижении
      * таймаута (в таком случае будет возвращена пустая последовательность).
      *
      *  \param bytes Байтовая последовательность из накопительного буфера.
      */
     void bytesOut(QByteArray bytes);
     /*!
      *  \brief Сигнал таймаута
      *
      *  Данный сигнал будет испущен, если после отправки команды устройству
      * ответ не был получен до истечения таймера таймаута. Перед вызовом
      * сигнала накопительный буфер будет очищен.
      *
      *  \see timeoutPerByte
      *  \see setTimeoutPerByte
      *  \see bytesIn
      */
     void timeout();
     /*!
      *  \brief Лог-сигнал записываемых данных
      *
      *  Данный сигнал будет испущен, когда будет произведена попытка записи
      * в порт.
      *
      *  \param bytes Байтовая последовательность
      */
     void logWrite(QByteArray bytes);
     /*!
      *  \brief Лог-сигнал прочитанных данных
      *
      *  Данный сигнал будет вызван, когда будет произведена попытка чтения
      * в порт.
      *
      *  \param bytes Байтовая последовательность
      */
     void logRead(QByteArray bytes);
     /*!
      *  \brief Лог-сигнал ошибки низкого приоритета
      *
      *  Данный сигнал будет вызван, если в процессе исполнения программы
      * произойдет ошибка, позволяющая продолжить работу программы.
      *
      *  \param string Лог-строка
      */
      void logWarning(QString string);
      /*!
       * \brief Лог-сигнал таймаута
       *
       *  Данный сигнал будет вызван, если выйдет время ожидания ответа на
       * запрос. Эквивалентен сигналу timeout, введен для семантики.
       *
       * \see timeout
       */
      void logTimeout();

public:
    /*!
     *  \brief Вернуть имя текущего порта.
     *
     *  Возвращает имя текущего порта, для Win-систем <b>COMXXX</b>,
     * для *nix <b>ttyXX</b>.
     *  \return Имя порта.
     *  \see setPortName
     */
    QString portName();
    /*!
     *  \brief Установить имя текущего порта.
     *
     *  Устанавливает новое соединение по указанному имени. Допускаются как
     * длинные так и короткие имена (COM1 vs //./COM1).
     *  \param portName Новое имя.
     *  \return Статус успешности установки нового имени.
     *  \see portName
     */
    bool setPortName(QString portName);
    /*!
     *  \brief Вернуть текущую скорость порта.
     *
     *  Возвращает текущую установленную скорость порта.
     *  \return Текущая скорость в бодах.
     *  \see setBaudRate
     */
    qint32 baudRate();
    /*!
     *  \brief Установить текущую скорость порта.
     *
     *  Устанавливает текущую скорость порта. Для допустимых значений см.
     * <b>QSerialPort::BaudRateXXXXX</b>.
     * \param baudRate Новая скорость.
     * \return Статус успешности установки новой скорости.
     * \see baudRate
     */
    bool setBaudRate(qint32 baudRate);
    /*!
     *  \brief Возврат текущей установленной последовательности, являющейся
     * признаком начала корректного пакета.
     *  \return Байтовая последовательность.
     *  \see setBeginSequence
     */
    PreceptSet beginSequence();
    /*!
     *  \brief Установка последовательности,
     * являющейся признаком конца корректного пакета.
     *
     *  Данные прочитанные из COM-порта и собираемые в накопительном буфере
     * проверяются на данную последовательность и если она будет обнаружена -
     * часть буфера от последовательности startSequence и до последовательности
     * endSequence включая их будет передана сигналом bytesOut().
     * В случае, если найдено более одного совпадения будет
     * испущено соответствующее количество сигналов.
     *  \param newseq Новая последовательность.
     *  \return Статус успешности установки новой последовательности.
     *  \see startSequence
     */
    bool setBeginSequence(PreceptSet newseq);
    /*!
     *  \brief Возврат текущей установленной последовательности, являющейся
     * признаком конца корректного пакета.
     *  \return Байтовая последовательность.
     *  \see setEndSequence
     */
    PreceptSet endSequence();
    /*!
     *  \brief Установка последовательности,
     * являющейся признаком конца корректного пакета.
     *
     *  Данные прочитанные из COM-порта и собираемые в накопительном буфере
     * проверяются на данную последовательность и если она будет обнаружена -
     * часть буфера от последовательности beginSequence и до последовательности
     * endSequence включая их будет передана сигналом bytesOut().
     * В случае, если найдено более одного совпадения будет
     * испущено соответствующее количество сигналов. Инициализируется значением
     * [CT_DEFAULT_ENDSEQ](@ref CT_DEFAULT_ENDSEQ).
     *  \param newseq Новая последовательность.
     *  \return Статус успешности установки новой последовательности.
     *  \see finalSequence
     */
    bool setEndSequence(PreceptSet newseq);
    /*!
     *  \brief Возвращает текущий коэффициент таймаута.
     *
     *  При отправке данных через слот bytesIn() данный коэффициент отвечает
     * за то, какое время дается устройству на обработку сигнала и отправку
     * ответа. Например значение 0.5 означает, что на каждый отправленный
     * функцией bytesIn() байт таймаут будет отодвинут на 500 мкс.
     *
     * \note Для систем с низкой частотой системного таймера суммарное
     * значение таймаута будет округлено по частоте таймера. Например,
     * если частота системного таймера 15 мс, а длина пакета 500 байт при
     * коэффициенте 0.25 суммарный таймаут будет не 125 мс, а 135.
     *
     * \return Значение коэффициента.
     * \see setTimeoutPerByte
     */
    qreal timeoutPerByte();
    /*!
     *  \brief Установка значения коэффициента таймаута.
     *
     *  \param timeout Значение коэффициента
     *  \return Статус успешности установки нового коэффициента.
     *  \see timeoutPerByte
     */
    bool setTimeoutPerByte(qreal timeout);
    /*!
     * \brief Установка четности порта
     * \param parity Значение четности
     * \return Статус успешности установки
     */
    bool setParity(QSerialPort::Parity parity);
    /*!
     * \brief Вернуть текущее значение четности порта
     * \return Текущее значение четности
     */
    QSerialPort::Parity parity();
    /*!
     * \brief Установка количества бит данных порта
     * \param dataBits Новое значение
     * \return Статус успешности установки
     */
    bool setDataBits(QSerialPort::DataBits dataBits);
    /*!
     * \brief Запрос текущего количества бит данных порта
     * \return Текущее значение
     */
    QSerialPort::DataBits dataBits();
    /*!
     * \brief Установка текущего значения стоповых бит
     * \param stopBits Новое значение стоповых бит
     * \return Статус успешности установки
     */
    bool setStopBits(QSerialPort::StopBits stopBits);
    /*!
     *  \brief Запрос текущего значения стоповых битов для порта
     *  \return Текущее значение
     */
    QSerialPort::StopBits stopBits();
    /*!
     *  \brief Установка контроля потока порта
     *  \param flow Новое значение
     *  \return Статус успешности установки
     */
    bool setFlowControl(QSerialPort::FlowControl flow);
    /*!
     *  \brief Запрос текущего значения контроля потоком
     *  \return Текущее значение
     */
    QSerialPort::FlowControl flowControl();
    /*!
     *  \brief Флаг подключенного порта
     *  \return Открыт порт или нет
     */
    bool isConnected();
    /*!
     *  \brief Отладочная функция для перевода PreceptSet в QString
     *  \param rules Правила для перевода
     *  \return Строка
     */
    static QString rulesToString(PreceptSet rules);
    /*!
     *  \brief Отладочная функция переноса потока байт в строку
     *  \param bytes Поток байт
     *  \return  Выходная строка
     */
    static QString bytesToString(QByteArray bytes);

private slots:
    /*!
     *  \brief Слот, реализующий вызовы сигналов родителя
     *  \param bytes Прочитанные байты
     */
    void bytesReady(QByteArray bytes);

private:
    /*!
     *  \brief Переменная QSerialPort, используемая для базовой реализации
     */
    QSerialPort port;
    /*!
     *  \brief Переменная, содержащая текущую начинающую последовательность
     * для корректного пакета
     */
    PreceptSet  beginseq;
    /*!
     *  \brief Переменная, содержащая текущую завершающую последовательность
     * для корректного пакета
     */
    PreceptSet  endseq;
    /*!
     *  \brief Переменная, содержащая текущее значение timeout-per-byte.
     *
     *  \see timeoutPerByte
     *  \see setTimeoutPerByte
     */
    qreal tpb;
    /*!
     *  \brief Поток, обеспечивающий чтение данных из потока
     *
     *  \see CSMSpinner
     */
    CSMSpinner * spinner;
};

/*!
 *  \brief  Класс-поток для накопления данных с порта.
 *
 *  Данный класс используется для сбора данных с порта. Класс использует данные
 * родителя (FoxCom) для обеспечения настроек выделения пакетов и доступа к
 * порту. Этот класс не должен быть использован за пределами проекта Cosmic
 * Turtle.
 *
 *  \see CSMCom
 */
class CSMSpinner : public QThread
{
    Q_OBJECT

public:
    /*!
     *  \brief Конструктор класса
     *  \param port Указатель на экземпляр класса QSerialPort
     *  \param beginseqptr Указатель на последовательность, содержащую правила
     * обнаружения начала корректного пакета.
     *  \param endseqptr Указатель на последовательность, содержащую правила
     * обнаружения конца корректного пакета.
     *  \param tpb Указатель на коэффициент таймаута
     *  \param parentptr Указатель на родителя - класс CSMCom
     */
    CSMSpinner(QSerialPort    * port,
               PreceptSet     * beginseqptr,
               PreceptSet     * endseqptr,
               qreal          * tpb,
               CSMCom         * parentptr);
    /*!
     *  \brief Деструктор класса
     */
    ~CSMSpinner();

public slots:
    /*!
     *  \brief Перегрузка исполняемого метода потока
     */
    void run() Q_DECL_OVERRIDE;

private:
    /*!
     *  \brief Флаг контроля потоком
     */
    bool terminated;
    /*!
    *  \brief Флаг "занятости" COM-порта.
    *
    *  Взводится при отправке пакета, во взведенном состоянии блокирует
    * возможность отправки других сообщений. Если при установленном флаге будет
    * произведена попытка записи в порт последовательности байт будут помещены в
    * очередь.
    *
    *  \see sendqueue
    */
    bool busy;
    /*!
     *  \brief Указатель на родительскую переменную порта
     */
    QSerialPort * portcopy;
    /*!
     *  \brief Накопительный буфер
     */
    QByteArray incoming;
    /*!
     *  \brief Переменная, содержащая указатель на текущую начинающую
     * последовательность для корректного пакета
     *
     * \see endseq
     */
    PreceptSet * beginseq;
    /*!
     *  \brief Переменная, содержащая указатель на текущую завершающую
     * последовательность для корректного пакета
     *
     * \see beginseq
     */
    PreceptSet * endseq;
    /*!
     *  \brief Очередь отправки байтовых последовательностей.
     *
     *  В случае, если флаг busy взведен здесь хранятся сообщения, которые
     * ждут своей очереди на отправку.
     *
     *  \see busy
     */
    QList<QByteArray> sendqueue;
    /*!
     *  \brief Обратный отсчет таймаута
     *
     *  Переменная, содержащее время в миллисекундах до срабатывания сигнала
     * timeout класса CSMCom и обнуления накопительного буфера.
     */
    qint32 timeleft;
    /*!
     *  \brief Указатель на коэффициент таймаута
     */
    qreal * tpbcopy;
    /*!
     *  \brief Счетчик отсчета таймаута
     */
    QTime timeout;
    /*!
     *  \brief Указатель на родителя для вызова сигналов класса CSMCom
     */
    CSMCom * parent;

private:
    /*!
     *  \brief Функция поиска подпоследовательности байт по заданным правилам
     *
     *  \param rules Правила поиска
     *  \param bytes Массив, в котором следует производить поиск
     *  \param pos (out) Позиция, на которой зафиксировано первое совпадение
     *  \param rule (out) Индекс последовательности из rules, которая была
     * найдена
     *  \return Возвращает значение pos
     */
    static qint32 ruleApplier(PreceptSet * rules, QByteArray bytes,
                              qint32 * pos, qint32 * rule);
    /*!
     *  \brief Поиск начала пакета
     *  \param pos (out) Позиция, на которой зафиксировано первое совпадение
     *  \param rule (out) Индекс последовательности из rules, которая была
     * найдена
     *  \return Возвращает значение pos
     *  \see ruleApplier
     */
    qint32 sequenceBeginSearch(qint32 * pos, qint32 * rule);
    /*!
     *  \brief Поиск конца пакета
     *  \param pos (out) Позиция, на которой зафиксировано первое совпадение
     *  \param rule (out) Индекс последовательности из rules, которая была
     * найдена
     *  \return Возвращает значение pos
     *  \see ruleApplier
     */
    qint32 sequenceEndSearch(qint32 * pos, qint32 * rule);

signals:
    /*!
     *  \brief Сигнал полученных данных
     *
     *  Предназначен для класса CSMCom
     *  \param bytes Прочитанные данные
     */
    void bytesOut(QByteArray bytes);

public slots:
    /*!
     *  \brief Слот записи данных
     *  \param bytes Данные для записи
     *  \param requestedTimeout Таймаут. Значение по умолчанию активирует
     * коэффициент tpbcopy.
     */
    void bytesToWrite(QByteArray bytes, qint32 requestedTimeout = -1);
};

#endif // CSMTURTLE_HPP
