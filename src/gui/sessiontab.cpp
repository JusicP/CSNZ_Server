#include "sessiontab.h"
#include <ui_sessiontab.h>
#include <QKeyEvent>
#include <QMenu>

#include "gui.h"
#include "common/utils.h"

#include "interface/net/iextendedsocket.h"
#include "interface/iserverinstance.h"
#include "interface/iusermanager.h"
#include "interface/ievent.h"
#include "interface/iroom.h"

#include "usercharacterdialog.h"
#include "bandialog.h"

CSessionTab::CSessionTab(QWidget* parent) : QWidget(parent)
{
	m_pUI = new Ui::SessionTab();
	m_pUI->setupUi(this);

	connect(m_pUI->RefreshBtn, SIGNAL(clicked()), this, SLOT(Refresh()));
	connect(m_pUI->KickBtn, SIGNAL(clicked()), this, SLOT(Kick()));
	connect(m_pUI->BanBtn, SIGNAL(clicked()), this, SLOT(Ban()));
	connect(m_pUI->ShowOnlyLoggedIn, SIGNAL(toggled(bool)), this, SLOT(ShowOnlyLoggedInToggled(bool)));

	m_pUI->SessionList->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(m_pUI->SessionList, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(HandleContextMenu(const QPoint&)));

	m_bRefresing = false;

	//Session session = {};
	//session.clientID = 1;
	//session.userID = 1;
	//session.ip = "127.0.0.1";
	//m_Sessions.push_back(session);

	//OnSessionListUpdated(m_Sessions);
}

CSessionTab::~CSessionTab()
{
	delete m_pUI;
}

void CSessionTab::Refresh()
{
	if (m_bRefresing)
		return;

	m_pUI->RefreshBtn->setEnabled(false);

	g_pEvent->AddEventFunction([]()
		{
			std::vector<Session> sessions;
			std::vector<IExtendedSocket*> sockets = g_pServerInstance->GetClients();
			for (auto s : sockets)
			{
				Session session;
				session.clientID = s->GetID();
				session.ip = s->GetIP();
				session.hwid = s->GetHWID();

				IUser* user = g_pUserManager->GetUserBySocket(s);
				if (user)
				{
					session.userID = user->GetID();
					IRoom* room = user->GetCurrentRoom();
					session.roomID = room ? room->GetID() : 0;
					session.uptime = user->GetUptime();
					session.userName = user->GetUsername();
					session.status = user->GetStatus();
				}
				else
				{
					session.userID = -1;
					session.roomID = 0;
					session.uptime = 0;
					session.userName = "";
					session.status = 0;
				}

				sessions.push_back(session);
			}

			GUI()->OnSessionListUpdated(sessions);
		});

	m_bRefresing = true;
}

void CSessionTab::Kick()
{
	std::vector<int> clientsToKick;
	QItemSelectionModel* selections = m_pUI->SessionList->selectionModel();
	QModelIndexList selected = selections->selectedRows();
	while (!selected.isEmpty())
	{
		const QModelIndex& idx = selected.at(0);

		int clientID = m_pUI->SessionList->item(idx.row(), 0)->text().toInt();
		clientsToKick.push_back(clientID);

		m_pUI->SessionList->removeRow(idx.row());
		selected = selections->selectedRows();
	}

	if (!clientsToKick.empty())
	{
		g_pEvent->AddEventFunction([clientsToKick]()
			{
				for (auto id : clientsToKick)
				{
					IExtendedSocket* s = g_pServerInstance->GetSocketByID(id);
					if (s)
						g_pServerInstance->DisconnectClient(s);
				}

				// TODO: callback???
				//GUI()->OnClientKicked();
			});
	}
}

void CSessionTab::Ban()
{
	// show ban dlg
	QItemSelectionModel* selections = m_pUI->SessionList->selectionModel();
	QModelIndexList selected = selections->selectedRows();
	if (selected.size() > 1)
	{
		return;
	}

	if (!selected.isEmpty())
	{
		const QModelIndex& idx = selected.at(0);

		int clientID = m_pUI->SessionList->item(idx.row(), 0)->text().toInt();

		Session toBan;
		for (auto& session : m_Sessions)
		{
			if (session.clientID = clientID)
			{
				toBan = session;
				break;
			}
		}

		CBanDialog dlg(this, toBan);
		int res = dlg.exec();
		if (res)
		{
			m_pUI->SessionList->model()->removeRow(idx.row());
		}
	}
}

void CSessionTab::ShowOnlyLoggedInToggled(bool checked)
{
	for (int i = 0; i < m_pUI->SessionList->rowCount(); i++)
	{
		if (m_pUI->SessionList->item(i, 1)->text() == "-1")
		{
			checked ? m_pUI->SessionList->hideRow(i) : m_pUI->SessionList->showRow(i);
		}
	}
}

void CSessionTab::keyPressEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_F5)
	{
		// refresh session list if F5 pressed
		Refresh();
	}
}

QString UserStatusToString(int status)
{
	switch (status)
	{
	case STATUS_MENU:
		return "Menu";
	case STATUS_INROOM:
		return "In room";
	case STATUS_PLAYING:
		return "Playing";
	}

	return "Unknown";
}

void CSessionTab::OnSessionListUpdated(const std::vector<Session>& sessions)
{
	m_bRefresing = false;
	m_pUI->RefreshBtn->setEnabled(true);

	m_Sessions = sessions;

	m_pUI->SessionList->clearContents();
	m_pUI->SessionList->model()->removeRows(0, m_pUI->SessionList->rowCount());

	int row = 0;
	for (auto& session : sessions)
	{
		m_pUI->SessionList->insertRow(row);
		m_pUI->SessionList->setItem(row, 0, new QTableWidgetItem(QString::number(session.clientID)));
		m_pUI->SessionList->setItem(row, 1, new QTableWidgetItem(QString::number(session.userID)));
		m_pUI->SessionList->setItem(row, 2, new QTableWidgetItem(session.ip.c_str()));
		m_pUI->SessionList->setItem(row, 3, new QTableWidgetItem(session.userName.c_str()));
		m_pUI->SessionList->setItem(row, 4, new QTableWidgetItem(FormatSeconds(session.uptime)));
		m_pUI->SessionList->setItem(row, 5, new QTableWidgetItem(UserStatusToString(session.status)));
		row++;
	}
}

void CSessionTab::HandleContextMenu(const QPoint& pos)
{
	QItemSelectionModel* selections = m_pUI->SessionList->selectionModel();
	QModelIndexList selected = selections->selectedRows();
	if (selected.size() > 1)
		printf("> 1 selected\n");

	QTableWidgetItem* item = m_pUI->SessionList->itemAt(pos);
	if (item)
	{
		QMenu menu;
		int userID = m_pUI->SessionList->item(item->row(), 1)->text().toInt();
		if (userID > 0)
		{
			menu.addAction("View character", this, [=]() { OnOpenUserCharacterDialog(userID); });
			menu.addSeparator();
		}

		menu.addAction("Kick", this, SLOT(Kick()));
		menu.addAction("Ban", this, SLOT(Ban()));

		menu.exec(QCursor::pos());
	}
}

void CSessionTab::OnOpenUserCharacterDialog(int userID)
{
	CUserCharacterDialog dlg(this, userID);
	dlg.exec();
}