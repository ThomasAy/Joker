#
# Copyright (C) 2012-2014 Phonations
# License: http://www.gnu.org/licenses/gpl.html GPL version 2 or higher
#

TARGET = VideoSyncTest

VERSION = 1.0.0

TOP_ROOT = $${_PRO_FILE_PWD_}/../..

include($$TOP_ROOT/common/common.pri)

include($$TOP_ROOT/libs/PhTools/PhTools.pri)
include($$TOP_ROOT/libs/PhSync/PhSync.pri)
include($$TOP_ROOT/libs/PhAudio/PhAudio.pri)
include($$TOP_ROOT/libs/PhGraphic/PhGraphic.pri)
include($$TOP_ROOT/libs/PhVideo/PhVideo.pri)
include($$TOP_ROOT/libs/PhCommonUI/PhCommonUI.pri)

HEADERS += VideoSyncTestWindow.h \
	VideoSyncTestSettings.h \
	../VideoTest/VideoTestSettings.h \
	../VideoTest/VideoTestWindow.h

SOURCES += \
	main.cpp \
	VideoSyncTestWindow.cpp \
	../VideoTest/VideoTestWindow.cpp

FORMS += \
	../VideoTest/VideoTestWindow.ui

QMAKE_POST_LINK += $${QMAKE_COPY} $$shell_path($${TOP_ROOT}/data/fonts/HelveticaCYPlain.ttf) $${RESOURCES_PATH} $${CS}

PH_DEPLOY_LOCATION = $$(TESTS_RELEASE_PATH)
include($$TOP_ROOT/common/deploy.pri)
