#pragma once

#include <QWidget>

namespace Ui
{
	class MainTab;
}

class CMainTab : public QWidget
{
	Q_OBJECT

public:
	CMainTab(QWidget* parent = nullptr);
	~CMainTab();

public slots:
	void UpdateInfo(int status, int totalConnections, int uptime, double memoryUsage);
	void SendNoticeBtnClicked();

private:
	Ui::MainTab* m_pUI;
};