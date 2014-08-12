/**
 * @file
 * @copyright (C) 2012-2014 Phonations
 * @license http://www.gnu.org/licenses/gpl.html GPL version 2 or higher
 */

#include "PhTools/PhDebug.h"
#include "Synchronizer.h"

Synchronizer::Synchronizer()
	: _syncType(NoSync),
	_stripClock(NULL),
	_videoClock(NULL),
	_syncClock(NULL),
	_settingStripFrame(false),
	_settingVideoFrame(false),
	_settingSonyFrame(false),
	_settingStripRate(false),
	_settingVideoRate(false),
	_settingSonyRate(false)
{
}

void Synchronizer::setStripClock(PhClock *clock)
{
	_stripClock = clock;
	connect(_stripClock, SIGNAL(frameChanged(PhTime)), this, SLOT(onStripFrameChanged(PhTime)));
	connect(_stripClock, SIGNAL(rateChanged(PhRate)), this, SLOT(onStripRateChanged(PhRate)));
}

void Synchronizer::setVideoClock(PhClock *clock)
{
	_videoClock = clock;
	connect(_videoClock, SIGNAL(frameChanged(PhTime)), this, SLOT(onVideoFrameChanged(PhTime)));
	connect(_videoClock, SIGNAL(rateChanged(PhRate)), this, SLOT(onVideoRateChanged(PhRate)));
	connect(_videoClock, SIGNAL(tcTypeChanged(PhTimeCodeType)), this, SLOT(onVideoTCTypeChanged(PhTimeCodeType)));
}

void Synchronizer::setSyncClock(PhClock *clock, SyncType type)
{
	_syncClock = clock;
	_syncType = type;
	if(_syncClock) {
		connect(_syncClock, SIGNAL(frameChanged(PhTime)), this, SLOT(onSyncFrameChanged(PhTime)));
		connect(_syncClock, SIGNAL(rateChanged(PhRate)), this, SLOT(onSyncRateChanged(PhRate)));
	}
}

void Synchronizer::onStripFrameChanged(PhTime time)
{
	if(!_settingStripFrame) {
		PHDBG(2) << time;
		if(_syncClock) {
			// Apply precise correction.
			// We don't change sony clock because this would desynchronize the sony master.
			PhFrame frame = time/PhTimeCode::timePerFrame(_syncClock->timeCodeType());
			if(qAbs(frame - _syncClock->frame()) > 1) {
				PHDEBUG << "correct :" << frame << _syncClock->frame();
				_settingStripFrame = true;
				_stripClock->setTime(_syncClock->time());
				_settingStripFrame = false;
			}
		}

		if(_syncType != Sony) {
			_settingVideoFrame = true;
			_videoClock->setTime(time);
			_settingVideoFrame = false;
		}
	}
}

void Synchronizer::onStripRateChanged(PhRate rate)
{
	if(!_settingStripRate) {
		PHDEBUG << rate;
		if(_syncClock) {
			_settingSonyRate = true;
			_syncClock->setRate(rate);
			_settingSonyRate = false;
		}
		_settingVideoRate = true;
		_videoClock->setRate(rate);
		_settingVideoRate = false;
	}
}

void Synchronizer::onVideoFrameChanged(PhTime)
{
//	if(!_settingVideoFrame)
//		PHDEBUG << time;
}

void Synchronizer::onVideoRateChanged(PhRate)
{
}

void Synchronizer::onVideoTCTypeChanged(PhTimeCodeType tcType)
{
	PHDEBUG << tcType;
	_stripClock->setTimeCodeType(tcType);
	if(_syncClock)
		_syncClock->setTimeCodeType(tcType);
}

void Synchronizer::onSyncFrameChanged(PhTime time)
{
	if(!_settingSonyFrame) {
		if(_syncType == Sony) {
			_settingVideoFrame = true;
			_videoClock->setTime(time);
			_settingVideoFrame = false;
		}
		// We apply correction here only when there is a significant change of sony frame.
		// Precise correction occurs in onStripFrameChanged() that is called after
		// on SonyFrameChanged (see VideoStripView::paint()).
		PhFrame frame = time/PhTimeCode::timePerFrame(_stripClock->timeCodeType());
		PHDBG(3) << frame;
		PhFrame error = qAbs(frame - _stripClock->frame());
		if((error > 10) || ((_stripClock->rate() == 0) && (error > 0))) {
			PHDEBUG << "correct error:" << frame << _stripClock->frame();
			_settingStripFrame = true;
			_stripClock->setTime(time);
			_settingStripFrame = false;
		}
	}
}

void Synchronizer::onSyncRateChanged(PhRate rate)
{
	if(!_settingSonyRate) {
		PHDEBUG << rate;
		_settingStripRate = true;
		_stripClock->setRate(rate);
		_settingStripRate = false;
		_settingVideoRate = true;
		_videoClock->setRate(rate);
		_settingVideoFrame = false;
	}
}
