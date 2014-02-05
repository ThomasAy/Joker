#ifndef PREFERENCESPANEL_H
#define PREFERENCESPANEL_H

#include <QDialog>

namespace Ui {
class PreferencesPanel;
}

class PreferencesPanel : public QDialog
{
	Q_OBJECT

public:
	explicit PreferencesPanel(QString audioOutput, QString audioInput, QWidget *parent = 0);
	~PreferencesPanel();
	QString selectedAudioOutput();
	QString selectedAudioInput();

private:
	Ui::PreferencesPanel *ui;
};

#endif // PREFERENCESPANEL_H