#ifndef PHLTCREADER_H
#define PHLTCREADER_H

#include <QString>
#include <QtMultimedia/QAudioInput>
#include <QObject>



#include "ltc.h"

#include "PhTools/PhClock.h"
#include "PhTools/PhTimeCode.h"

class PhLtcReader : public QObject
{

    Q_OBJECT

public:
    explicit PhLtcReader(QObject *parent = 0);

    void init(QString _input="");
    QList<QString> inputList();
    PhClock * clock();

private slots:
    void onNotify();

private:
    PhClock _clock;

    QAudioInput *_input;

    qint64 position;
    QIODevice * buffer2;
    LTCDecoder * decoder;

};

#endif // PHLTCREADER_H
