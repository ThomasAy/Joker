/**
 * @file
 * @copyright (C) 2012-2014 Phonations
 * @license http://www.gnu.org/licenses/gpl.html GPL version 2 or higher
 */

#ifndef PHLTCWRITER_H
#define PHLTCWRITER_H

#include <ltc.h>

#include "PhTools/PhClock.h"
#include "PhTools/PhTimeCode.h"

#include "PhAudio/PhAudioWriter.h"

#define FRAME_PER_BUFFER 1920
/** Audio sample rate */
#define SAMPLE_RATE 48000

/**
 * @brief Send master LTC generator
 */
class PhLtcWriter : public PhAudioWriter
{
	Q_OBJECT
public:
	/**
	 * @brief PhLtcWriter constructor
	 * @param tcType the timecode type
	 * @param parent the reader's parent
	 */
	explicit PhLtcWriter(PhTimeCodeType tcType, QObject *parent = 0);
	/**
	 * @brief Initialize the writer
	 *
	 * It initialize the writer on the given input device if it's found,
	 * or take the default output device if not.
	 * @param deviceName The desired output device
	 * @return True if succeed, false otherwise
	 */
	bool init(QString deviceName = "");
	/**
	 * @brief Get the output list
	 * @return Return all the output devices
	 */
	static QList<QString> outputList();
	/**
	 * @brief Get the writer clock
	 * @return The writer clock
	 */
	PhClock *clock();

private:

	int processAudio(void *outputBuffer,
	                 unsigned long framesPerBuffer);
	static int audioCallback( const void *inputBuffer, void *outputBuffer,
	                          unsigned long framesPerBuffer,
	                          const PaStreamCallbackTimeInfo* timeInfo,
	                          PaStreamCallbackFlags statusFlags,
	                          void *userData );
	float data;

	PhClock _clock;
	LTCEncoder *_encoder;
	SMPTETimecode _st;

};

#endif // PHLTCWRITER_H
