/**
* Copyright (C) 2012-2013 Phonations
* License: http://www.gnu.org/licenses/gpl.html GPL version 2 or higher
*/

#ifndef GraphicTestView_H
#define GraphicTestView_H

#include "PhGraphicView.h"
#include "PhGraphicImage.h"
#include "PhGraphicText.h"


class GraphicTestView : PhGraphicView
{
public :
	explicit GraphicTestView(QWidget *parent = 0, QString name = "");

protected:
	bool init();

	void paint();

private:
	PhGraphicImage _image;
};

#endif // GraphicTestView_H