Проект Cosmic Turtle
====================

# Введение

Проект Cosmic turtle является низкоуровневым модулем работы с COM-портом, 
основанным на Qt-компоненте QSerialPort.

# Основной функционал

* Работа с COM-портом на уровне последовательностей QByteArray.
* Наличие очереди сообщений.
* Возможность поиска ответных корректных пакетов по заданным правилам.

## Подготовка к работе

Для того, чтобы начать работать с проектом необходимо выполнить следующие шаги:

1. Подключить к своему проекту модуль com/csmturtle.hpp
2. Подключить к своему проекту модуль com/csmturtle.cpp
3. Если вы используете стиль раздельного хранения cpp и hpp файлов, необходимо 
исправить #include "csmturtle.hpp" в файле com/csmturtle.cpp на путь, 
соответствующий вашей системе хранения.

Далее необходимо определиться, в каком режиме работает ваше устройство. 
Я разделяю три режима работы:

1. Режим "запрос-ответ".
2. Режим "только запись".
3. Режим "только чтение".

Чаще всего устройства работают в первом режиме (например устройства серии 
"Термодат"), однако иногда встречаются устройства, работающие во втором 
(set-only реле) или третьем (например измерители фирмы Leuze electronic) режимах.

Рассмотрим подробности настройки модуля для работы во всех трех режимах.

### Настройка для режима "Запрос-ответ"

Для работы в режиме "запрос-ответ", мы будем использовать следующий функционал:

* Слот bytesIn, подключившись к которому мы сможем отправлять данные в порт.
* Сигнал bytesOut, подключившись к которому мы сможем получить от устройства 
ответ.
* Сигнал timeout, подключившись к которому мы узнаем, что устройство находится 
в неактивном режиме (или был выставлен неправильный временн`ой режим).
* (опционально, отладка) Сигнал logWrite, рассказывающий о попытке записи в 
порт.
* (опционально, отладка) Сигнал logRead, рассказывающий об успешном нахождении 
пакета в потоке байт от устройства.
* (опционально, отладка) Сигнал logWarning, сообщающий о неудачах в ходе 
настройки порта
* (опционально, отладка) Сигнал logTimeout, сообщающий об отсутствии ответа от 
устройства за заданный период времени.
* (опционально) функцию setBeginSequence, устанавливающую правила поиска 
начала пакета.
* функцию setEndSequence, устанавливающую правила поиска конца пакета.

Сначала создадим экземпляр класса CSMCom и настроим подключение:

```C++
CSMCom csmcom;

csmcom.setPortName("COM3");
csmcom.setBaudRate(QSerialPort::Baud115200);
csmcom.setDataBits(QSerialPort::Data8);
csmcom.setStopBits(QSerialPort::OneStop);
csmcom.setFlowControl(QSerialPort::NoFlowControl);
```

Затем подключим необходимые сигналы и слоты между собой:

```C++
QObject::connect(&csmcom, SIGNAL(logRead(QByteArray)),
                 &csmlog, SLOT(log_ComCSM_read(QByteArray)));
QObject::connect(&csmcom, SIGNAL(logWrite(QByteArray)),
                 &csmlog, SLOT(log_ComCSM_write(QByteArray)));
QObject::connect(&csmcom, SIGNAL(logWarning(QString)),
                 &csmlog, SLOT(log_ComCSM_warning(QString)));
QObject::connect(&csmcom, SIGNAL(logTimeout()),
                 &csmlog, SLOT(log_ComCSM_timeout()));
```

*Примечание: в данном примере я не использовал сигнал bytesOut, однако крайне 
важно, чтобы вы подключились к нему*

Теперь откроем документацию на устройство и обратим внимание на формат входящих 
и исходящих пакетов. Представим, что в нашем случае пакеты имеют следующий 
формат (я опускаю приставку 0x__ для лучшей читаемости):

```
AA .. .. .. .. 55 FF
```
При этом заметим, что передача ведется в бинарном режиме и байты, 
задействованные в сигнатурах должны быть дублированы в середине пакета. Иными 
словами формат преобразуется к следующему:

```
[не AA], [AA], [не AA], .. .. .. .. , [не 55], [55], [FF], [не FF]
```

В нашем случае это означает, что сигнатура начальной последовательности будет 
следующей:

```
[не AA], [AA], [не AA]
```

А сигнатура окончания пакета следующей:
```
[не 55], [55], [FF], [не FF]
```

Зададим правила поиска пакетов:
```C++
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
correctend1.append(PreceptByte(false,  0xFF));
endseq.append(correctend1);

csmcom.setBeginSequence(beginseq);
csmcom.setEndSequence(endseq);
```

Рассмотрим случай, когда устройство работает не в бинарном режиме, а в режиме 
MODBUS:

```
48 .. .. .. 13
49 .. .. .. 13
50 .. .. .. 13
```

Как видно из примера, данное устройство различает три типа начальной 
последовательности и один тип завершающей.

Установим правила:
```C++
PreceptSet beginseq;
PreceptSet endseq;

PreceptArray correctbegin1;
correctbegin1.append(PreceptByte(true,  0x48));
beginseq.append(correctbegin1);

PreceptArray correctbegin2;
correctbegin1.append(PreceptByte(true,  0x49));
beginseq.append(correctbegin2);

PreceptArray correctbegin3;
correctbegin1.append(PreceptByte(true,  0x50));
beginseq.append(correctbegin3);

PreceptArray correctend1;
correctend1.append(PreceptByte(true,  0x13));
endseq.append(correctend1);

csmcom.setBeginSequence(beginseq);
csmcom.setEndSequence(endseq);
```

Теперь, когда правила установлены мы должны как-то инициировать наш разговор 
с устройством. Для этого используем слот bytesIn:

```C++

const QByteArray COM_ident("\xAA\x01\xFE\x00\x00\x00\x00\x00\x04\xFB\x02\x00\xFF\xFD\x55\xFF", 16);

csmcom.bytesIn(COM_ident);
```

*Обратите внимание, что проект является лишь прослойкой к COM-порту и не имеет 
функционала полноценного разборщика данных. Именно поэтому здесь использована 
константа*

Если бы наш логгер, к которому мы подключили слоты выводил данные в консоль, то
 мы увидели бы примерно следующую картину:\

```
[CSMCOM] Tx (15:11:46.079):
AA 01 FE 00 00 00 00 00 04 FB 02 00 FF FD 55 FF


[CSMCOM] Rx (15:11:46.267):
AA FE 01 00 00 00 00 00 41 BE 02 80 01 00 02 01
37 49 6D 70 65 64 61 6E 63 65 20 2D 67 61 75 67
65 20 56 32 2E 30 31 20 28 66 72 65 71 75 65 6E
63 79 20 72 61 6E 67 65 20 31 30 2E 2E 2E 37 35
30 30 30 20 48 7A 29 00 00 EE D6 55 FF
```

#### Настройка таймаута

В предыдущем примере мы опустили настройку времени ожидания, рассмотрим же ее 
теперь. Каждый раз после записи данных в устройство модуль сгенерирует один из 
сигналов bytesOut или timeout. Если до окончания времени ожидания не будет 
обнаружена корректная последовательность байт - входной  накопительный буфер 
будет очищен, а сигнал timeout испущен. Рассмотрим, отчего зависит время 
ожидания.

Есть два способа задани времени ожидания: глобальный и локальный. Локальное 
задание означает, что мы устанавливаем время ожидания для каждого пакета в 
отдельности; для этого у слота bytesIn есть необязательный параметр 
requestedTimeout, означающий время ожидания в миллисекундах. Это полезно 
например если на короткий запрос "как дела?" устройство ответит 
"хорошовчеравечеромвстретиласвоюсоседкумыснеймилобесе... 
...такяпровелапятьлетжизни". Однако в большинстве случаев размер запроса 
приблизительно равен размеру ответа. Для задания глобального таймаута 
используется функция setTimeoutPerByte, принимающая значение в миллисекундах. 
Данная функция установит коэффициент ожидания, необходимый для автоматического 
расчета времени ожидания. Например, устройство работает на скорости 115200 бод, 
время отклика 50 миллисекунд, а стандартный размер пакетов (равно входящих и 
исходящих) равен 2000 байт. Проведем расчеты:

115 200 бод - это 11 400 байт в секунду, 2000 байт передадутся за время, равное 
175 миллисекундам. Добавим время отклика, равное 50 и получим, что конец пакета 
в самом лучшем случае придет к нам через 225 миллисекунд. Добавим примерно 50% 
на задержки в линии, ограничение устройства и пр, и получим примерно 350 
миллисекунд. Теперь поделим это время на стандартный размер исходящего пакета - 
2000 байт - получим нужный коэффициент 0.175 мс/байт, который нам и следует 
установить. Если размеры входящих и исходящих пакетов различны, то делить 
следует на размер **исходящего** пакета. Например, если бы наш размер исходящего 
пакета был бы равен 8ми байтам, то коэффициент принял бы вид 350 / 8 = 43.75 
мс/байт.

#### Использование очереди сообщений

Предположим, что нам необходимо узнать у устройства три значения типа float32. 
Так же предположим, что все значения можно получить, использовав разные команды 
(ну или разные параметры в одной команде). Напишем такой код:

```C++
csmcom.bytesIn(GETTHATFLOAT1);
csmcom.bytesIn(GETTHATFLOAT2);
csmcom.bytesIn(GETTHATFLOAT3);
```

Можно заметить, что мы не стали устанавливать какие-либо задержки между двумя 
командами. Это не приведет к "перемешиванию" ответа, поскольку, отправив первое 
сообщение, модуль отправит остальные два в очередь на отправку и отправит их по 
тому же правилу после того, как получит один из двух возможных исходов для уже 
отправленного.

### Настройка для режима "только запись"

Главные отличительные особенности работы в таком режиме - это установка 
коэффициента ожидания в значение "0" и игнорирование сигналов bytesOut и 
timeout.

### Настройка для режима "только чтение"

В данном режиме "наблюдения" модуль будет сканировать COM-порт на предмет 
корректного пакета и отправлять сигнал bytesOut как только обнаружит пакет. 
Например, предположим, что устройство работает по протоколу MODBUS, непрерывно 
отсылая некоторое число в формате float32 (пусть, например, это будет 
расстояние). Тогда возможным режимом работы может стать следующий:

```
Rx (+00015):
0x52 0x51 0x49 0x54 0x56 0x48 0x48 0x48 0x13 // = 0x43168000 = (float32)150.5
Rx (+00025):
0x52 0x51 0x49 0x54 0x56 0x48 0x48 0x48 0x13 // = 0x43168000 = (float32)150.5
Rx (+00035):
0x52 0x51 0x49 0x54 0x56 0x48 0x48 0x48 0x13 // = 0x43168000 = (float32)150.5
Rx (+00045):
0x52 0x51 0x49 0x54 0x56 0x48 0x48 0x48 0x13 // = 0x43168000 = (float32)150.5
Rx (+00055):
0x52 0x51 0x49 0x54 0x56 0x48 0x48 0x48 0x13 // = 0x43168000 = (float32)150.5

В таком случае (если была задана завершающая последовательность 0x13) модуль 
будет посылать сигнал bytesOut, содержащий в себе полученную последовательность.

Никто не запрещает в данном режиме отправку данных через слот bytesIn, 
поскольку, что вполне логично, устройству может потребоваться "старт-стоп" 
сигнал.
