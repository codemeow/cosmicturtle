cosmicturtle
============

En
--

Low-level COM-port module based on QSerialPort


This project allows to write in and read out data from COM-port. Based on QSerialPort, built in Qt 5.

The main features:

* Message queue - new data will not be sent until timeout or read signal is emitted.
* Package finder - this module will look for the package signature in data stream and emit 'read' signal if a package is found.

Ru
--

Низкоуровневый модуль работы с COM-портом, основанный на QSerialPort


Данный проект позволяет писать и читать байтовый поток из COM-порта. Основывается на QSerialPort, Qt версии 5.

Основной функционал:

* Очередь сообщений - новые данный не будут записаны в COM-порт, пока не вернется сигнал timeout или read предыдущего сообщения.
* Поиск пакетов - данный модуль будет рассматривать байтовый поток на предмет заданных признаков пакетов и испустит сигнал 'read', если пакет будет найден.
