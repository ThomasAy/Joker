/**
 * @file
 * @copyright (C) 2012-2014 Phonations
 * @license http://www.gnu.org/licenses/gpl.html GPL version 2 or higher
 */

#include "PhVideoEngine.h"

PhVideoEngine::PhVideoEngine(PhVideoSettings *settings) :
	_settings(settings),
	_fileName(""),
	_tcType(PhTimeCodeType25),
	_frameIn(0),
	_oldFrame(0),
	_isProcessing(false),
	_bufferSize(settings->videoBufferSize()),
	_bufferFreeSpace(settings->videoBufferSize()),
	_nextDecodingFrame(0),
	_lastDecodedFrame(0),
	_direction(0),
	_pFormatContext(NULL),
	_videoStream(NULL),
	_videoFrame(NULL),
	_deinterlace(false),
	_bilinearFiltering(true),
	_audioStream(NULL),
	_audioFrame(NULL)
{
	PHDEBUG << "Using FFMpeg widget for video playback.";
	av_register_all();
	avcodec_register_all();

	// Setup the thread worker according to
	// http://mayaposch.wordpress.com/2011/11/01/how-to-really-truly-use-qthreads-the-full-explanation/
	this->moveToThread(&_processingThread);
	connect(&_processingThread, &QThread::started, this, &PhVideoEngine::process);

	connect(&_clock, &PhClock::timeChanged, this, &PhVideoEngine::onTimeChanged);
	connect(&_clock, &PhClock::rateChanged, this, &PhVideoEngine::onRateChanged);
}

bool PhVideoEngine::open(QString fileName)
{
	PHDEBUG << fileName;

	this->close();

	_clock.setTime(0);
	_clock.setRate(0);
	_oldFrame = _oldFrame = PHFRAMEMIN;

	if(avformat_open_input(&_pFormatContext, fileName.toStdString().c_str(), NULL, NULL) < 0)
		return false;

	// Retrieve stream information
	if (avformat_find_stream_info(_pFormatContext, NULL) < 0)
		return false; // Couldn't find stream information

	av_dump_format(_pFormatContext, 0, fileName.toStdString().c_str(), 0);

	// Find video stream
	for(int i = 0; i < (int)_pFormatContext->nb_streams; i++) {
		AVMediaType streamType = _pFormatContext->streams[i]->codec->codec_type;
		PHDEBUG << i << ":" << streamType;
		switch(streamType) {
		case AVMEDIA_TYPE_VIDEO:
			_videoStream = _pFormatContext->streams[i];
			PHDEBUG << "\t=> video";
			break;
		case AVMEDIA_TYPE_AUDIO:
			if(_audioStream == NULL)
				_audioStream = _pFormatContext->streams[i];
			PHDEBUG << "\t=> audio";
			break;
		default:
			PHDEBUG << "\t=> unknown";
			break;
		}
	}

	if(_videoStream == NULL)
		return false;

	// Reading timecode type
	_tcType = PhTimeCode::computeTimeCodeType(this->framePerSecond());

	emit timeCodeTypeChanged(_tcType);

	// Reading timestamp :
	AVDictionaryEntry *tag = av_dict_get(_pFormatContext->metadata, "timecode", NULL, AV_DICT_IGNORE_SUFFIX);
	if(tag == NULL)
		tag = av_dict_get(_videoStream->metadata, "timecode", NULL, AV_DICT_IGNORE_SUFFIX);

	if(tag) {
		PHDEBUG << "Found timestamp:" << tag->value;
		_frameIn = PhTimeCode::frameFromString(tag->value, _tcType);
	}

	PHDEBUG << "length:" << this->length();

	PHDEBUG << "size : " << _videoStream->codec->width << "x" << _videoStream->codec->height;
	AVCodec * videoCodec = avcodec_find_decoder(_videoStream->codec->codec_id);
	if(videoCodec == NULL) {
		PHDEBUG << "Unable to find the codec:" << _videoStream->codec->codec_id;
		return false;
	}


	if (avcodec_open2(_videoStream->codec, videoCodec, NULL) < 0) {
		PHDEBUG << "Unable to open the codec:" << _videoStream->codec;
		return false;
	}

	_nextDecodingFrame = _frameIn;

	if(_audioStream) {
		AVCodec* audioCodec = avcodec_find_decoder(_audioStream->codec->codec_id);
		if(audioCodec) {
			if(avcodec_open2(_audioStream->codec, audioCodec, NULL) < 0) {
				PHDEBUG << "Unable to open audio codec.";
				_audioStream = NULL;
			}
			else {
				_audioFrame = av_frame_alloc();
				PHDEBUG << "Audio OK.";
			}
		}
		else {
			PHDEBUG << "Unable to find codec for audio.";
			_audioStream = NULL;
		}
	}
	_fileName = fileName;

	_isProcessing = true;
	_processingThread.start(QThread::LowestPriority);

	return true;
}

void PhVideoEngine::close()
{
	PHDEBUG << _fileName;
	if(_isProcessing) {
		PHDEBUG << "Stop the processing thread.";
		clearBuffer();
		_isProcessing = false;
		_processingThread.wait();
	}

	if(_pFormatContext) {
		PHDEBUG << "Close the media context.";
		if(_videoStream)
			avcodec_close(_videoStream->codec);
		if(_audioStream)
			avcodec_close(_audioStream->codec);
		avformat_close_input(&_pFormatContext);

		PHDEBUG << _fileName << "closed";
	}
	_frameIn = _nextDecodingFrame = 0;
	_pFormatContext = NULL;
	_videoStream = NULL;
	_audioStream = NULL;
	PHDEBUG << _fileName << "closed";

	_fileName = "";
}

void PhVideoEngine::setDeinterlace(bool deinterlace)
{
	PHDEBUG << deinterlace;
	if(deinterlace != _deinterlace) {
		_deinterlace = deinterlace;
		_bufferMutex.lock();
		clearBuffer();
		_nextDecodingFrame = _oldFrame;
		_bufferMutex.unlock();
	}
	_oldFrame = PHFRAMEMIN;
}

bool PhVideoEngine::bilinearFiltering()
{
	return _videoRect.bilinearFiltering();
}

void PhVideoEngine::setBilinearFiltering(bool bilinear)
{
	_videoRect.setBilinearFiltering(bilinear);
}

void PhVideoEngine::drawVideo(int x, int y, int w, int h)
{
	if(_videoStream) {
		PhFrame frame = _clock.frame(_tcType);
		if(_settings)
			frame += _settings->screenDelay() * PhTimeCode::getFps(_tcType) * _clock.rate() / 1000;

//		PHDEBUG << frame << _oldFrame;
		if(frame != _oldFrame) {
			uint8_t *buffer = getBuffer(frame);
			if(buffer) {
				int height = this->height();
				if(_deinterlace)
					height = height / 2;
//				PHDEBUG << frame << _deinterlace;
				_videoRect.createTextureFromRGBBuffer(buffer, this->width(), height);
				_oldFrame = frame;
				_frameCounter.tick();
			}
			else
				PHDEBUG << "No buffer";
		}
		_videoRect.setRect(x, y, w, h);
		_videoRect.setZ(-10);
		_videoRect.draw();
	}
}

void PhVideoEngine::process()
{
	PHDEBUG;

	_videoFrame = av_frame_alloc();

	while(_isProcessing) {
		QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
		// If the decoder is blocked
		while (!_bufferFreeSpace.tryAcquire()) {
			// Process the events (max time 5ms)
			QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
			// Decrease CPU load if the player is in pause
			if(_direction == 0)
				QThread::msleep(5);
		}
//		QTime t;
//		t.start();
		decodeFrame(_nextDecodingFrame);
//		PHDEBUG << t.elapsed() << "ms to decode the frame" << _nextDecodingFrame;
		switch (_direction) {
		case 1:
		case 0:
			_nextDecodingFrame++;
			break;
		case -1:
			_nextDecodingFrame--;
		default:
			break;
		}
	}
	_processingThread.quit();
	PHDEBUG << "Bye bye";
}

void PhVideoEngine::onTimeChanged(PhTime time)
{
	PhFrame frame = time / PhTimeCode::timePerFrame(_tcType);
//	PHDEBUG << frame;
	_bufferMutex.lock();
//	PHDBG(25) << "Want" << frame << _direction << "Have ("<<  _bufferMap.count() <<":" << _bufferFreeSpace.available() << "):" << _bufferMap.keys();
	if(!_bufferMap.contains(frame))
		_nextDecodingFrame = frame;
	if(_direction >= 0) {
		// Remove old frames
		PhFrame min = PHFRAMEMAX;
		foreach(PhFrame key, _bufferMap.keys()) {
			if(key < frame - _bufferSize / 2) {
				delete _bufferMap[key];
				_bufferMap.remove(key);
				_bufferFreeSpace.release();
				//PHDEBUG << "Remove" << key;
			}
			if(key < min)
				min = key;
		}
		if(min > frame) {
			clearBuffer();
			_nextDecodingFrame = frame;
		}
	}
	else {
		// Remove future frames
		PhFrame max = PHFRAMEMIN;
		foreach(PhFrame key, _bufferMap.keys()) {
			if(key > frame + _bufferSize / 2) {
				delete _bufferMap[key];
				_bufferMap.remove(key);
				_bufferFreeSpace.release();
			}
			if(key > max)
				max = key;
		}
		if(max < frame) {
			clearBuffer();
			_nextDecodingFrame = frame;
		}
	}
	PHDEBUG << _oldFrame << frame;
//	PHDEBUG << "Should decode:" << _nextDecodingFrame << "Have ("<<  _bufferMap.count() <<":" << _bufferFreeSpace.available() << "):" << _bufferMap.keys();
	_bufferMutex.unlock();
}

void PhVideoEngine::onRateChanged(PhRate rate)
{
	if(rate < 0)
		_direction = -1;
	else if (rate > 0)
		_direction = 1;
	else
		_direction = 0;
	PHDEBUG << "Direction is now" <<_direction;
}

uint8_t *PhVideoEngine::getBuffer(PhFrame frame)
{
	//PHDBG(24) << frame << _framesFree.available();
	uint8_t *buffer = NULL;
	_bufferMutex.lock();
	if(_bufferMap.contains(frame)) {
		buffer = _bufferMap[frame];
	}
	_bufferMutex.unlock();

	return buffer;
}

int PhVideoEngine::bufferSize()
{
	return _settings->videoBufferSize();
}

int PhVideoEngine::bufferOccupation()
{
	return _bufferSize - _bufferFreeSpace.available();
}

int64_t PhVideoEngine::frame2time(PhFrame f)
{
	int64_t t = 0;
	if(_videoStream) {
		PhFrame fps = PhTimeCode::getFps(timeCodeType());
		t = f * _videoStream->time_base.den / _videoStream->time_base.num / fps;
	}
	return t;
}

PhFrame PhVideoEngine::time2frame(int64_t t)
{
	PhFrame f = 0;
	if(_videoStream) {
		PhFrame fps = PhTimeCode::getFps(timeCodeType());
		f = t * _videoStream->time_base.num * fps / _videoStream->time_base.den;
	}
	return f;
}

void PhVideoEngine::decodeFrame(PhFrame frame)
{
	// Exit if the frame is already in the buffer
	_bufferMutex.lock();
	if(_bufferMap.contains(frame)) {
		_bufferMutex.unlock();
		//Release the unused ressource
		_bufferFreeSpace.release();
		return;
	}
	_bufferMutex.unlock();

	if(frame - _lastDecodedFrame != 1) {
		int flags = AVSEEK_FLAG_ANY;
		int64_t timestamp = frame2time(frame - _frameIn);
		PHDEBUG << "seek:" << _direction << frame;
		av_seek_frame(_pFormatContext, _videoStream->index, timestamp, flags);
	}

	int frameFinished = 0;
	while(frameFinished == 0) {
		AVPacket packet;
		int error = av_read_frame(_pFormatContext, &packet);
		switch(error) {
		case 0:
			if(packet.stream_index == _videoStream->index) {
				avcodec_decode_video2(_videoStream->codec, _videoFrame, &frameFinished, &packet);
				if(frameFinished) {

					int frameHeight = _videoFrame->height;
					if(_deinterlace)
						frameHeight /= 2;
					// As the following formats are deprecated (see https://libav.org/doxygen/master/pixfmt_8h.html#a9a8e335cf3be472042bc9f0cf80cd4c5)
					// we replace its with the new ones recommended by LibAv
					// in order to get ride of the warnings
					AVPixelFormat pixFormat;
					switch (_videoStream->codec->pix_fmt) {
					case AV_PIX_FMT_YUVJ420P:
						pixFormat = AV_PIX_FMT_YUV420P;
						break;
					case AV_PIX_FMT_YUVJ422P:
						pixFormat = AV_PIX_FMT_YUV422P;
						break;
					case AV_PIX_FMT_YUVJ444P:
						pixFormat = AV_PIX_FMT_YUV444P;
						break;
					case AV_PIX_FMT_YUVJ440P:
						pixFormat = AV_PIX_FMT_YUV440P;
					default:
						pixFormat = _videoStream->codec->pix_fmt;
						break;
					}
					SwsContext * swsContext = sws_getContext(_videoFrame->width, _videoFrame->height, pixFormat,
					                                         _videoFrame->width, frameHeight, AV_PIX_FMT_RGB24,
					                                         SWS_POINT, NULL, NULL, NULL);

					uint8_t * rgb = new uint8_t[_videoFrame->width * frameHeight * 3];
					int linesize = _videoFrame->width * 3;
					if (0 <= sws_scale(swsContext, (const uint8_t * const *) _videoFrame->data,
					                   _videoFrame->linesize, 0, _videoFrame->height, &rgb,
					                   &linesize)) {
						_bufferMutex.lock();
						_bufferMap[frame] = rgb;
						//PHDBG(25) << "Decoding" << PhTimeCode::stringFromFrame(frame, PhTimeCodeType25) << packet.dts << _bufferFreeSpace.available();
						_bufferMutex.unlock();
						_lastDecodedFrame = frame;
						//PHDEBUG << "Add" << frame;
					}
				}     // if frame decode is not finished, let's read another packet.
			}
			else if(_audioStream && (packet.stream_index == _audioStream->index)) {
				int ok = 0;
				avcodec_decode_audio4(_audioStream->codec, _audioFrame, &ok, &packet);
				//				if(ok) {
				//					PHDEBUG << "audio:" << _audioFrame->nb_samples;
				//				}
			}
			break;
		case AVERROR_EOF:
			// As we reach the end of the file, it's useless to go full speed
			QThread::msleep(10);
		case AVERROR_INVALIDDATA:
		default:
			{
				char errorStr[256];
				av_strerror(error, errorStr, 256);
				PHDEBUG << "error on frame" << frame << ":" << errorStr;
				// In order to get out of the while in case of error
				frameFinished = -1;
				break;
			}
		}
		//Avoid memory leak
		av_free_packet(&packet);
	}
}

void PhVideoEngine::clearBuffer()
{
	PHDEBUG << "Clearing the buffer";
	qDeleteAll(_bufferMap);
	_bufferFreeSpace.release(_bufferMap.count());
	_bufferMap.clear();
}

void PhVideoEngine::setFrameIn(PhFrame frame)
{
	PHDEBUG << frame;
	_bufferMutex.lock();
	clearBuffer();
	_nextDecodingFrame = _frameIn = frame;
	_bufferMutex.unlock();
}

void PhVideoEngine::setTimeIn(PhTime timeIn)
{
	setFrameIn(timeIn / PhTimeCode::timePerFrame(_tcType));
}

PhVideoEngine::~PhVideoEngine()
{
	close();
}

PhFrame PhVideoEngine::frameIn()
{
	return _frameIn;
}

PhFrame PhVideoEngine::frameLength()
{
	if(_videoStream)
		return time2frame(_videoStream->duration);
	return 0;
}

PhTime PhVideoEngine::length()
{
	return frameLength() * PhTimeCode::timePerFrame(_tcType);
}

float PhVideoEngine::framePerSecond()
{
	float fps = 0;
	if(_videoStream) {
		fps = _videoStream->avg_frame_rate.num / _videoStream->avg_frame_rate.den;
		// See http://stackoverflow.com/a/570694/2307070
		// for NaN handling
		if(fps != fps) {
			fps = _videoStream->time_base.den;
			fps /= _videoStream->time_base.num;
		}
	}
	return fps;
}

int PhVideoEngine::width()
{
	if(_videoStream)
		return _videoStream->codec->width;
	return 0;
}

int PhVideoEngine::height()
{
	if(_videoStream)
		return _videoStream->codec->height;
	return 0;
}

QString PhVideoEngine::codecName()
{
	if(_videoStream)
		return _videoStream->codec->codec_name;
	return "";
}
