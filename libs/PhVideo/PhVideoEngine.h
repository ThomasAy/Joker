/**
 * @file
 * @copyright (C) 2012-2014 Phonations
 * @license http://www.gnu.org/licenses/gpl.html GPL version 2 or higher
 */

#ifndef PHVIDEOENGINE_H
#define PHVIDEOENGINE_H

#include <QObject>
#include <QElapsedTimer>
#include <QSemaphore>
#include <QThread>

#include "PhSync/PhClock.h"
#include "PhTools/PhTickCounter.h"
#include "PhGraphic/PhGraphicTexturedRect.h"

#include "PhVideoSettings.h"

#include "PhAVDecoder.h"

/**
 * @brief The video engine
 *
 * It provide engine which compute the video from a file to an openGL texture.
 */
class PhVideoEngine : public QObject
{
	Q_OBJECT
public:
	/**
	 * @brief PhVideoEngine constructor
	 */
	explicit PhVideoEngine();
	~PhVideoEngine();

	// Properties
	/**
	 * @brief Get the video file name
	 * @return the video file name
	 */
	QString fileName() {
		return _fileName;
	}

	/**
	 * @brief The video timecode type
	 * @return A timecode type value
	 */
	PhTimeCodeType timeCodeType() {
		return _tcType;
	}

	/**
	 * @brief Get the clock
	 * @return the clock
	 */
	PhClock* clock() {
		return &_clock;
	}

	/**
	 * @brief Get the first frame of the video file
	 * @return A frame value
	 */
	PhFrame frameIn() {
		return _frameIn;
	}

	/**
	 * @brief Set first frame
	 * @param frameIn the new first frame
	 */
	void setFrameIn(PhFrame frameIn);

	/**
	 * @brief Get the starting time of the video file
	 * @return A time value
	 */
	PhTime timeIn() {
		return _frameIn * PhTimeCode::timePerFrame(_tcType);
	}

	/**
	 * @brief Set the video starting time
	 * @param timeIn A time value.
	 */
	void setTimeIn(PhTime timeIn);

	/**
	 * @brief Get last frame
	 * @return the last frame of the video file
	 */
	PhFrame frameOut() {
		return _frameIn + frameLength() - 1;
	}

	/**
	 * @brief Get the video ending time
	 * @return A time value.
	 */
	PhTime timeOut() {
		return frameOut() * PhTimeCode::timePerFrame(_tcType);
	}

	/**
	 * @brief Get the video length in frame
	 * @return A frame value
	 */
	PhFrame frameLength();

	/**
	 * @brief Get the video length
	 * @return A time value
	 */
	PhTime length();

	/**
	 * @brief Get the codec name
	 * @return the codec name
	 */
	float framePerSecond();
	/**
	 * @brief Get the width
	 * @return the PhVideoEngine width (not necessary the video width)
	 */
	int width();
	/**
	 * @brief Get the height
	 * @return the PhVideoEngine height (not necessary the video height)
	 */
	int height();
	/**
	 * @brief Get the codec name
	 * @return the codec name
	 */
	QString codecName();

	// Methods
	/**
	 * @brief Open a video file
	 * @param fileName A video file path
	 * @return True if the file was opened successfully, false otherwise
	 */
	bool open(QString fileName);
	/**
	 * @brief Close
	 *
	 * Close the PhVideoEngine, freeing all objects
	 */
	void close();
	/**
	 * @brief Check if video shall be deinterlace
	 * @return True if deinterlace false otherwise
	 */
	bool deinterlace();

	/**
	 * @brief Set the video deinterlace mode
	 * @param deinterlace True if deinterlace false otherwise
	 */
	void setDeinterlace(bool deinterlace);

	/**
	 * @brief Retrieve the video filtering
	 * @return True if bilinear filtering is enabled
	 */
	bool getBilinearFiltering() {
		return _bilinearFiltering;
	}

	/**
	 * @brief Enable or disable the video bilinear filtering
	 * Video bilinear filtering is enabled by default.
	 * @param bilinear True to enable bilinear filtering
	 */
	void setBilinearFiltering(bool bilinear);

	/**
	 * @brief draw the video depending on the parameters
	 * @param x coordinates of the upperleft corner
	 * @param y coordinates of the upperleft corner
	 * @param w width
	 * @param h height
	 */
	void drawVideo(int x, int y, int w, int h);

	/**
	 * @brief refreshRate
	 * @return the refreshRate
	 */
	int refreshRate() {
		return _frameCounter.frequency();
	}
signals:
	/**
	 * @brief Signal sent upon a different timecode type message
	 * @param tcType A timecode type value.
	 */
	void timeCodeTypeChanged(PhTimeCodeType tcType);

private:
	bool goToFrame(PhFrame frame);
	int64_t frame2time(PhFrame f);
	PhFrame time2frame(int64_t t);

	/**
	 * @brief The buffer size
	 * @return
	 */
	int bufferSize();

	/**
	 * @brief The buffer occupation
	 * @return
	 */
	int bufferOccupation();

	/**
	 * @brief Set the settings
	 * @param settings
	 */
	void setSettings(PhVideoSettings *settings);
public slots:
	/**
	 * @brief errorString
	 *
	 * For slot worker compatibility
	 */
	void errorString(QString);

private:
	PhVideoSettings *_settings;
	QString _fileName;
	PhTimeCodeType _tcType;
	PhClock _clock;
	PhFrame _oldFrame;
	PhFrame _frameIn;

	PhAVDecoder *_decoder;

	PhGraphicTexturedRect _videoRect;
	PhTickCounter _frameCounter;
};

#endif // PHVIDEOENGINE_H
