#include "loggerwindow.h"

using namespace Nova;

//TODO: add support and GUI elements for viewing honeyd logs

LoggerWindow::LoggerWindow(QWidget *parent)
    : QMainWindow(parent)
{
	ui.setupUi(this);
	isBasic = true;
	ui.settingsBoxAdv->hide();
	settingsBoxShowing = false;
	showNumberOfLogs = 0;
	initializeLoggingWindow();
	setLoadedPreferences();
	//create listening thread
	//reference NovaGUI CEListen, etc...
}

void LoggerWindow::initializeLoggingWindow()
{
	QFile file("/usr/share/nova/Logs/Nova.log");

	if(!file.open(QIODevice::ReadOnly))
	{
		ui.logDisplay->addTopLevelItem(logFileNotFound());
	}
	else
	{
		QTextStream in(&file);
		while(!in.atEnd())
		{
			QString pass = in.readLine();
			if(pass > 0)
				ui.logDisplay->addTopLevelItem(generateLogTabEntry(pass));
		}

		adjustColumnWidths();
	}
	//TODO: Add icons for checkboxes
	file.close();
}

void LoggerWindow::updateLogDisplay()
{
	QFile file("/usr/share/nova/Logs/Nova.log");
	uint16_t i = 0;

	if(!file.open(QIODevice::ReadOnly))
	{
		ui.logDisplay->addTopLevelItem(logFileNotFound());
	}
	else
	{
		ui.logDisplay->clear();
		QTextStream in(&file);
		if(showNumberOfLogs > 0)
		{
			ui.showLabel->setText("Last");
			while(!in.atEnd() && i < showNumberOfLogs)
			{
				QString pass = in.readLine();
				if(pass > 0)
				{
					ui.logDisplay->addTopLevelItem(generateLogTabEntry(pass));
					i++;
				}
			}
			while(!in.atEnd())
			{
				QString pass = in.readLine();
				if(pass > 0)
				{
					ui.logDisplay->takeTopLevelItem(0);
					ui.logDisplay->addTopLevelItem(generateLogTabEntry(pass));
				}
			}
		}
		else
		{
			ui.showLabel->setText("Show");
			while(!in.atEnd())
			{
				QString pass = in.readLine();
				if(pass > 0)
				{
					ui.logDisplay->addTopLevelItem(generateLogTabEntry(pass));
				}
			}
		}

		setLevelViews(viewLevels);
		adjustColumnWidths();
	}
	file.close();
}

void LoggerWindow::setLoadedPreferences()
{
	bool found = false;

	string home = GetHomePath();
	QString homePath = QString::fromStdString(home);
	homePath.append("/Config/NOVAConfig.txt");

	QFile file(homePath);

	if(!file.open(QIODevice::ReadOnly))
	{

	}
	else
	{
		QTextStream in(&file);
		while(!in.atEnd() && !found)
		{
			QString pass = in.readLine();
			QStringList parse = pass.split(" ");
			if(!parse[0].compare("LOG_VIEW_LEVELS"))
			{
				viewLevels = parse[1];
				setLevelViews(parse[1]);
				found = true;
			}
		}
	}
	file.close();
}

void LoggerWindow::setLevelViews(QString bitmask)
{
	if(bitmask[0] == '0')
	{
		ui.checkDebug->setCheckState(Qt::Checked);
		ui.checkDebug->setCheckState(Qt::Unchecked);
	}
	if(bitmask[1] == '0')
	{
		ui.checkInfo->setCheckState(Qt::Checked);
		ui.checkInfo->setCheckState(Qt::Unchecked);
	}
	if(bitmask[2] == '0')
	{
		ui.checkNotice->setCheckState(Qt::Checked);
		ui.checkNotice->setCheckState(Qt::Unchecked);
	}
	if(bitmask[3] == '0')
	{
		ui.checkWarning->setCheckState(Qt::Checked);
		ui.checkWarning->setCheckState(Qt::Unchecked);
	}
	if(bitmask[4] == '0')
	{
		ui.checkError->setCheckState(Qt::Checked);
		ui.checkError->setCheckState(Qt::Unchecked);
	}
	if(bitmask[5] == '0')
	{
		ui.checkCritical->setCheckState(Qt::Checked);
		ui.checkCritical->setCheckState(Qt::Unchecked);
	}
	if(bitmask[6] == '0')
	{
		ui.checkAlert->setCheckState(Qt::Checked);
		ui.checkAlert->setCheckState(Qt::Unchecked);
	}
	if(bitmask[7] == '0')
	{
		ui.checkEmergency->setCheckState(Qt::Checked);
		ui.checkEmergency->setCheckState(Qt::Unchecked);
	}
}

QTreeWidgetItem * LoggerWindow::generateLogTabEntry(QString line)
{
	QTreeWidgetItem * ret = new QTreeWidgetItem();
	QStringList parse = line.split(" ", QString::SkipEmptyParts);
	QString time(parse[0] + " " + parse[1]+ " " + parse[2]);
	ret->setText(0, time);
	QString process(parse[5].mid(0, (parse[5].size() - 1)));
	ret->setText(1, process);
	QString level(parse[6]);
	ret->setText(2, level);
	QString message(parse[7]);

	for(int i = 8; i < parse.size(); i++)
	{
		message.append(" ");
		message.append(parse[i]);
	}

	ret->setText(3, message);
	if(!level.compare("INFO"))
	{
		for(int i = 0; i < 4; i++)
		{
			ret->setBackgroundColor(i, "skyblue");
		}
	}
	else if(!level.compare("NOTICE"))
	{
		for(int i = 0; i < 4; i++)
		{
			ret->setBackgroundColor(i, "darkseagreen");
		}
	}
	else if(!level.compare("WARNING"))
	{
		for(int i = 0; i < 4; i++)
		{
			ret->setBackgroundColor(i, "yellow");
		}
	}
	else if(!level.compare("ERROR"))
	{
		for(int i = 0; i < 4; i++)
		{
			ret->setBackgroundColor(i, "tomato");
		}
	}
	else if(!level.compare("CRITICAL"))
	{
		for(int i = 0; i < 4; i++)
		{
			ret->setBackgroundColor(i, "orangered");
		}
	}
	else if(!level.compare("ALERT"))
	{
		for(int i = 0; i < 4; i++)
		{
			ret->setBackgroundColor(i, "red");
		}
	}
	else if(!level.compare("EMERGENCY"))
	{
		for(int i = 0; i < 4; i++)
		{
			ret->setBackgroundColor(i, "black");
			ret->setForeground(i, QBrush("white"));
		}
	}
	return ret;
}

QTreeWidgetItem * LoggerWindow::logFileNotFound()
{
	QTime time = QTime::currentTime();
	QTreeWidgetItem * ret = new QTreeWidgetItem();
	ret->setText(0, time.toString());
	ret->setText(1, "ERROR");
	ret->setText(2, "Log File not Found");
	return ret;
}

void LoggerWindow::on_settingsButton_clicked()
{
	if(!settingsBoxShowing)
	{
		ui.settingsBoxAdv->show();
		settingsBoxShowing = true;
	}
	else if(settingsBoxShowing)
	{
		ui.settingsBoxAdv->hide();
		settingsBoxShowing = false;
	}
}

/*void LoggerWindow::on_saveButton_clicked()
{
	if(ui.logTabContainer->currentIndex() == 0)
	{

	}
	else if(ui.logTabContainer->currentIndex() == 1)
	{

	}
}*/

void LoggerWindow::on_clearButton_clicked()
{
	ui.logDisplay->clear();
}
// TODO: make the filtering choices exclusive, OR establish and implement precedence of choices
void LoggerWindow::on_checkDebug_stateChanged(int state)
{
	if(state == Qt::Checked)
	{
		showSelected("DEBUG", false);
	}
	else if(state == Qt::Unchecked)
	{
		hideSelected("DEBUG", false);
	}
}

void LoggerWindow::on_checkInfo_stateChanged(int state)
{
	if(state == Qt::Checked)
	{
		showSelected("INFO", false);
	}
	else if(state == Qt::Unchecked)
	{
		hideSelected("INFO", false);
	}
}

void LoggerWindow::on_checkNotice_stateChanged(int state)
{
	if(state == Qt::Checked)
	{
		showSelected("NOTICE", false);
	}
	else if(state == Qt::Unchecked)
	{
		hideSelected("NOTICE", false);
	}
}

void LoggerWindow::on_checkWarning_stateChanged(int state)
{
	if(state == Qt::Checked)
	{
		showSelected("WARNING", false);
	}
	else if(state == Qt::Unchecked)
	{
		hideSelected("WARNING", false);
	}
}

void LoggerWindow::on_checkError_stateChanged(int state)
{
	if(state == Qt::Checked)
	{
		showSelected("ERROR", false);
	}
	else if(state == Qt::Unchecked)
	{
		hideSelected("ERROR", false);
	}
}

void LoggerWindow::on_checkCritical_stateChanged(int state)
{
	if(state == Qt::Checked)
	{
		showSelected("CRITICAL", false);
	}
	else if(state == Qt::Unchecked)
	{
		hideSelected("CRITICAL", false);
	}
}

void LoggerWindow::on_checkAlert_stateChanged(int state)
{
	if(state == Qt::Checked)
	{
		showSelected("ALERT", false);
	}
	else if(state == Qt::Unchecked)
	{
		hideSelected("ALERT", false);
	}
}

void LoggerWindow::on_checkEmergency_stateChanged(int state)
{
	if(state == Qt::Checked)
	{
		showSelected("EMERGENCY", false);
	}
	else if(state == Qt::Unchecked)
	{
		hideSelected("EMERGENCY", false);
	}
}

void LoggerWindow::on_checkClassification_stateChanged(int state)
{
	if(state == Qt::Checked)
	{
		showSelected("ClassificationEngine", true);
	}
	else if(state == Qt::Unchecked)
	{
		hideSelected("ClassificationEngine", true);
	}
}

void LoggerWindow::on_linesBox_currentIndexChanged(const QString & text)
{
	showNumberOfLogs = text.toInt();
	updateLogDisplay();
}

void LoggerWindow::hideSelected(QString level, bool isProcess)
{
	QTreeWidgetItemIterator it(ui.logDisplay);
	if(isProcess)
	{
		while (*it)
		{
			if((*it)->text(1) == level)
			(*it)->setHidden(true);
			++it;
		}
	}
	else
	{
		while (*it)
		{
			if((*it)->text(2) == level)
			(*it)->setHidden(true);
			++it;
		}
	}
}

void LoggerWindow::showSelected(QString level, bool isProcess)
{
	QTreeWidgetItemIterator it(ui.logDisplay);
	if(isProcess)
	{
		while (*it)
		{
			if((*it)->text(1) == level)
			(*it)->setHidden(false);
			++it;
		}
	}
	else
	{
		while (*it)
		{
			if((*it)->text(2) == level)
			(*it)->setHidden(false);
			++it;
		}
	}
}

void LoggerWindow::adjustColumnWidths()
{
	ui.logDisplay->resizeColumnToContents(0);
	ui.logDisplay->resizeColumnToContents(1);
	ui.logDisplay->resizeColumnToContents(2);
}

void LoggerWindow::on_closeButton_clicked()
{
	this->close();
}

LoggerWindow::~LoggerWindow()
{

}
