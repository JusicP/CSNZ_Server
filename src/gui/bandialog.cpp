#include "bandialog.h"
#include <ui_bandialog.h>
#include <QMessageBox.h>

#include "gui.h"
#include "interface/ievent.h"
#include "interface/iuserdatabase.h"
#include "interface/iserverinstance.h"

CBanDialog::CBanDialog(QWidget* parent, const Session& session) : QDialog(parent)
{
	m_pUI = new Ui::BanDialog();
	m_pUI->setupUi(this);

	m_Session = session;

	setWindowFlags(windowFlags() | Qt::MSWindowsFixedSizeDialogHint);

	m_pUI->ExpiryDateEntry->setMinimumDateTime(QDateTime::currentDateTime());

	m_pUI->BanClientID->setText(m_pUI->BanClientID->text().arg(m_Session.clientID));
	m_pUI->BanUserID->setText(m_pUI->BanUserID->text().arg(m_Session.userID));

	m_pUI->BanTypeBox->addItem("IP");

	connect(m_pUI->BanBtn, &QPushButton::clicked, this, &CBanDialog::Ban);
	connect(m_pUI->CancelBtn, &QPushButton::clicked, this, &CBanDialog::close);
	connect(m_pUI->BanTypeBox, &QComboBox::currentTextChanged, this, &CBanDialog::OnBanTypeChanged);

	CheckInfo();
}

CBanDialog::~CBanDialog()
{
	delete m_pUI;
}

void CBanDialog::CheckInfo()
{
	m_pUI->UserBanTypeBox->setEnabled(false);
	m_pUI->ExpiryDateEntry->setEnabled(false);
	m_pUI->BanReason->setEnabled(false);

	if (m_Session.userID > 0)
	{
		m_pUI->BanTypeBox->addItem("User");
	}

	if (!m_Session.hwid.empty())
	{
		m_pUI->BanTypeBox->addItem("HWID");
	}
}

void CBanDialog::Ban()
{
	QString banType = m_pUI->BanTypeBox->currentText();
	if (banType == "User")
	{
		UserBan ban;

		QString userBanType = m_pUI->UserBanTypeBox->currentText();
		if (userBanType == "With message")
			ban.banType = 1;
		else if (userBanType == "Silent (without showing reason on client)")
			ban.banType = 2;
		else
			ban.banType = 0;

		ban.reason = m_pUI->BanReason->toPlainText().toStdString();
		ban.term = m_pUI->ExpiryDateEntry->dateTime().currentSecsSinceEpoch() / 60;
	
		if (g_pUserDatabase->UpdateUserBan(m_Session.userID, ban) <= 0)
		{
			QMessageBox::critical(this, "Error", "An error occured while banning user");
			close();
		}
	}
	else if (banType == "HWID")
	{
		if (g_pUserDatabase->UpdateHWIDBanList(m_Session.hwid) <= 0)
		{
			QMessageBox::critical(this, "Error", "An error occured while banning user");
			close();
		}
	}
	else if (banType == "IP")
	{
		if (g_pUserDatabase->UpdateIPBanList(m_Session.ip) <= 0)
		{
			QMessageBox::critical(this, "Error", "An error occured while banning user");
			close();
		}
	}
	else
	{
		QMessageBox::warning(this, "Warning", "Unknown ban type");
	}

	int clientID = m_Session.clientID;
	g_pEvents->AddEventFunction([clientID]()
		{
			g_pServerInstance->DisconnectClient(g_pServerInstance->GetSocketByID(clientID));
		});

	close();
	setResult(1);
}

void CBanDialog::OnBanTypeChanged(const QString& text)
{
	if (text == "User")
	{
		m_pUI->UserBanTypeBox->setEnabled(true);
		m_pUI->ExpiryDateEntry->setEnabled(true);
		m_pUI->BanReason->setEnabled(true);
	}
	else
	{
		m_pUI->UserBanTypeBox->setEnabled(false);
		m_pUI->ExpiryDateEntry->setEnabled(false);
		m_pUI->BanReason->setEnabled(false);
	}
}