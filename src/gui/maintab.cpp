#include "maintab.h"
#include "common/utils.h"
#include "noticedialog.h"
#include "userbanlistdialog.h"
#include "hwidbanlistdialog.h"
#include "ipbanlistdialog.h"

#include <ui_maintab.h>
#include <QDateTime.h>
#include <QTimer.h>
#include <QMessageBox.h>

const int TIMEOUT_TIME = 120000; // 2 minutes in ms

CMainTab::CMainTab(QWidget* parent) : QWidget(parent)
{
	m_pUI = new Ui::MainTab();
	m_pUI->setupUi(this);
	
	m_nConnectedClients = 0;

	m_pServerHeartbeatTimer = new QTimer(this);
	m_pServerHeartbeatTimer->setInterval(TIMEOUT_TIME);

	connect(m_pUI->SendNoticeBtn, &QPushButton::clicked, this, &CMainTab::SendNoticeBtnClicked);
	connect(m_pUI->UserBanListBtn, &QPushButton::clicked, this, &CMainTab::OpenUserBanList);
	connect(m_pUI->IPBanListBtn, &QPushButton::clicked, this, &CMainTab::OpenIPBanList);
	connect(m_pUI->HWIDBanListBtn, &QPushButton::clicked, this, &CMainTab::OpenHWIDBanList);
	connect(m_pUI->ConfigEditorBtn, &QPushButton::clicked, this, [=]() { QMessageBox::warning(this, "Warning", "Unimplemented"); });
	connect(m_pUI->DelayedShutdownBtn, &QPushButton::clicked, this, [=]() { QMessageBox::warning(this, "Warning", "Unimplemented"); });

	connect(m_pServerHeartbeatTimer, &QTimer::timeout, this, &CMainTab::ServerTimeout);

	m_pServerHeartbeatTimer->start();
}

CMainTab::~CMainTab()
{
	delete m_pUI;
}

int CMainTab::GetConnectedClients()
{
	return m_nConnectedClients;
}

// update main tab info
void CMainTab::UpdateInfo(int status, int totalConnections, int uptime, double memoryUsage)
{
	m_nConnectedClients = totalConnections;

	m_pUI->ServerStatusLabel->setText(QString("Server status: %1").arg(status));
	m_pUI->ClientNumberLabel->setText(QString("Total connection: %1").arg(totalConnections));
	m_pUI->UptimeLabel->setText(QString("Uptime: %1").arg(FormatSeconds(uptime)));
	m_pUI->MemUsageLabel->setText(QString("Memory usage: %1mb").arg(memoryUsage, 0, 'f', 3));

	m_pServerHeartbeatTimer->start();
}

void CMainTab::SendNoticeBtnClicked()
{
	CNoticeDialog noticeDialog(this);
	noticeDialog.exec();
}

void CMainTab::OpenUserBanList()
{
	CUserBanListDialog dlg(this);
	dlg.exec();
}

void CMainTab::OpenIPBanList()
{
	CIPBanListDialog dlg(this);
	dlg.exec();
}

void CMainTab::OpenHWIDBanList()
{
	CHWIDBanListDialog dlg(this);
	dlg.exec();
}

void CMainTab::ServerTimeout()
{
	m_pServerHeartbeatTimer->stop();
	QMessageBox::warning(this, "Warning", QString("The server does not respond for more than %1 seconds.").arg(TIMEOUT_TIME / 1000));
}
