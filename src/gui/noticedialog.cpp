#include "noticedialog.h"
#include "selectuserdialog.h"

#include "gui.h"
#include "interface/iusermanager.h"
#include "interface/ipacketmanager.h"
#include "interface/ievent.h"

#include <ui_noticedialog.h>
#include <QMessageBox>

#define MAX_TEXT_LEN 128

CNoticeDialog::CNoticeDialog(QWidget* parent) : QDialog(parent)
{
	m_pUI = new Ui::NoticeDialog();
	m_pUI->setupUi(this);

	setWindowFlags(windowFlags() | Qt::MSWindowsFixedSizeDialogHint);

	connect(m_pUI->CancelBtn, SIGNAL(clicked()), this, SLOT(close()));
	connect(m_pUI->SendBtn, SIGNAL(clicked()), this, SLOT(SendClicked()));
	connect(m_pUI->SelectUsersBtn, SIGNAL(clicked()), this, SLOT(SelectUsersClicked()));
	connect(m_pUI->NotificationTextEdit, SIGNAL(textChanged()), this, SLOT(TextChanged()));
}

CNoticeDialog::~CNoticeDialog()
{
	delete m_pUI;
}

void CNoticeDialog::SendClicked()
{
	QString text = m_pUI->NotificationTextEdit->toPlainText();
	if (text.isEmpty())
	{
		QMessageBox::warning(this, "Warning", "You have not entered anything in the text field.");
		return;
	}

	if (m_SelectedUsers.empty())
	{
		QMessageBox::warning(this, "Warning", "You have not selected any users.");
		return;
	}

	std::string str = text.toStdString();
	std::vector<int> users = m_SelectedUsers;
	g_pEvents->AddEventFunction([str, users]()
		{
			for (auto userID : users)
			{
				IUser* user = g_pUserManager->GetUserById(userID);
				if (user)
				{
					g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(user->GetExtendedSocket(), str);
				}
			}
		});

	close();
}

void CNoticeDialog::SelectUsersClicked()
{
	CSelectUserDialog selectUserDialog(this, m_SelectedUsers);
	if (selectUserDialog.exec() == 1)
		m_SelectedUsers = selectUserDialog.GetSelectedUsers();
}

void CNoticeDialog::TextChanged()
{
	// TODO: don't put characters when limit is exceeded if cursor is not at end
	m_pUI->NotificationTextEdit->blockSignals(true);

	// check for characters limit
	QString text = m_pUI->NotificationTextEdit->toPlainText();
	if (text.length() > MAX_TEXT_LEN)
	{
		text.chop(text.length() - MAX_TEXT_LEN); // cut text after 128 character
		m_pUI->NotificationTextEdit->setPlainText(text);

		QTextCursor cursor(m_pUI->NotificationTextEdit->textCursor());
		cursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
		m_pUI->NotificationTextEdit->setTextCursor(cursor);
	}

	m_pUI->NotificationTextEdit->blockSignals(false);
}