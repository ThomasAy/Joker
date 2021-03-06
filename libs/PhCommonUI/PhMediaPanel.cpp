/**
 * @file
 * @copyright (C) 2012-2014 Phonations
 * @license http://www.gnu.org/licenses/gpl.html GPL version 2 or higher
 */

#include "PhMediaPanel.h"
#include "ui_PhMediaPanel.h"

PhMediaPanel::PhMediaPanel(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::PhMediaPanel),
	_clock(NULL),
	_timeIn(0),
	_length(0)
{
	ui->setupUi(this);

	//Buttons Init

	ui->_playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
	connect(ui->_playButton, &QPushButton::clicked, this, &PhMediaPanel::onPlayPause);

	ui->_fastForwardButton->setIcon(style()->standardIcon(QStyle::SP_MediaSeekForward));
	connect(ui->_fastForwardButton, &QPushButton::clicked, this, &PhMediaPanel::onFastForward);

	ui->_fastRewindButton->setIcon(style()->standardIcon(QStyle::SP_MediaSeekBackward));
	connect(ui->_fastRewindButton, &QPushButton::clicked, this, &PhMediaPanel::onRewind);

	ui->_backButton->setIcon(style()->standardIcon(QStyle::SP_MediaSkipBackward));
	connect(ui->_backButton, &QPushButton::clicked, this, &PhMediaPanel::onBack);

	ui->_nextFrameButton->setIcon(style()->standardIcon(QStyle::SP_ArrowForward));
	connect(ui->_nextFrameButton, &QPushButton::clicked, this, &PhMediaPanel::onNextFrame);

	ui->_previousFrameButton->setIcon(style()->standardIcon(QStyle::SP_ArrowBack));
	connect(ui->_previousFrameButton, &QPushButton::clicked, this, &PhMediaPanel::onPreviousFrame);

	connect(ui->_slider, &QSlider::sliderMoved, this, &PhMediaPanel::onSliderChanged);

	connect(ui->timeCodeTypeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onTCTypeComboChanged()));

	ui->_playButton->setDefault(true);
}


PhMediaPanel::~PhMediaPanel()
{
	delete ui;
}

PhTimeCodeType PhMediaPanel::timeCodeType() const
{
	switch(ui->timeCodeTypeCombo->currentIndex()) {
	case 0:
		return PhTimeCodeType2398;
	case 1:
		return PhTimeCodeType24;
	case 3:
		return PhTimeCodeType2997;
	case 4:
		return PhTimeCodeType30;
	default:
		return PhTimeCodeType25;
	}
}

void PhMediaPanel::setSliderEnable(bool isEnabled)
{
	ui->_slider->setEnabled(isEnabled);
}

PhTime PhMediaPanel::timeIn() const
{
	return _timeIn;
}

void PhMediaPanel::setTimeIn(PhTime timeIn)
{
	_timeIn = timeIn;
	updateSlider();
}

PhTime PhMediaPanel::length()
{
	return _length;
}

void PhMediaPanel::setLength(PhTime length)
{
	_length = length;
	updateSlider();
}

void PhMediaPanel::setClock(PhTimeCodeType tcType, PhClock *clock)
{
	onTimeCodeTypeChanged(tcType);
	_clock = clock;
	if(_clock) {
		ui->_timecodeLabel->setText(PhTimeCode::stringFromTime(_clock->time(), tcType));
		connect(_clock, &PhClock::timeChanged, this, &PhMediaPanel::onTimeChanged);
		connect(_clock, &PhClock::rateChanged, this, &PhMediaPanel::onRateChanged);
	}
}

void PhMediaPanel::onRateChanged(PhRate rate)
{
	ui->_rateLabel->setText("x"+QString::number(rate));
	if(rate != 0)
		ui->_playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
	else
		ui->_playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
}

void PhMediaPanel::onTimeCodeTypeChanged(PhTimeCodeType tcType)
{
	switch(tcType) {
	case PhTimeCodeType2398:
		ui->timeCodeTypeCombo->setCurrentIndex(0);
		break;
	case PhTimeCodeType24:
		ui->timeCodeTypeCombo->setCurrentIndex(1);
		break;
	case PhTimeCodeType25:
		ui->timeCodeTypeCombo->setCurrentIndex(2);
		break;
	case PhTimeCodeType2997:
		ui->timeCodeTypeCombo->setCurrentIndex(3);
		break;
	case PhTimeCodeType30:
		ui->timeCodeTypeCombo->setCurrentIndex(4);
		break;
	}
}

void PhMediaPanel::onPlayPause()
{
	if(_clock) {
		if(_clock->rate())
			_clock->setRate(0);
		else
			_clock->setRate(1);
	}
	emit playPause();
}

void PhMediaPanel::onFastForward()
{
	if(_clock)
		_clock->setRate(3);
	emit fastForward();
}

void PhMediaPanel::onRewind()
{
	if(_clock)
		_clock->setRate(-3);
	emit rewind();
}

void PhMediaPanel::onBack()
{
	if(_clock)
		_clock->setTime(_timeIn);
	emit back();
}

void PhMediaPanel::onNextFrame()
{
	if(_clock) {
		_clock->setRate(0);
		_clock->setTime(_clock->time() + PhTimeCode::timePerFrame(this->timeCodeType()));
	}
	emit nextFrame();
}

void PhMediaPanel::onPreviousFrame()
{
	if(_clock) {
		_clock->setRate(0);
		_clock->setTime(_clock->time() - PhTimeCode::timePerFrame(this->timeCodeType()));
	}
	emit previousFrame();
}

void PhMediaPanel::onSliderChanged(int position)
{
	PhTime time = position * PhTimeCode::timePerFrame(this->timeCodeType());
	if(_clock)
		_clock->setTime(time);
	emit goToTime(time);
}

void PhMediaPanel::updateSlider()
{
	PhTimeCodeType tcType = this->timeCodeType();
	PhFrame frameIn = _timeIn / PhTimeCode::timePerFrame(tcType);
	PhFrame frameOut = (_timeIn + _length) / PhTimeCode::timePerFrame(tcType);
	ui->_slider->setMinimum(frameIn);
	ui->_slider->setMaximum(frameOut);
}

void PhMediaPanel::onTCTypeComboChanged()
{
	emit timeCodeTypeChanged(timeCodeType());
}

void PhMediaPanel::onTimeChanged(PhTime time)
{
	PhTimeCodeType tcType = this->timeCodeType();
	ui->_timecodeLabel->setText(PhTimeCode::stringFromTime(time, tcType));
	ui->_slider->setSliderPosition(time / PhTimeCode::timePerFrame(tcType));
}

