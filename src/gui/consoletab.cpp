#include "consoletab.h"
#include "common/console.h"
#include "gui.h"
#include "interface/ievent.h"
#include "../consolecommands.h"

#include <ui_consoletab.h>
#include <QKeyEvent>
#include <QStringListModel>
#include <QAbstractItemView>

#include <string>

CConsoleTab::CConsoleTab(QWidget* parent) : QWidget(parent)
{
	m_pUI = new Ui::ConsoleTab();
	m_pUI->setupUi(this);

	m_pCommandHistory = new QCompleter();

	m_pCommandList = new QCompleter(m_CmdList);
	m_pUI->Entry->setCompleter(m_pCommandList);

	m_pUI->Entry->installEventFilter(this);

	connect(m_pUI->SubmitBtn, &QPushButton::clicked, this, &CConsoleTab::SubmitClicked);
	connect(m_pUI->Entry, SIGNAL(textEdited(const QString&)), this, SLOT(TextChanged(const QString&)));

	// get cmd list
	g_pEvent->AddEventFunction([]()
		{
			GUI()->OnCommandListUpdated(CmdList()->GetCommandList());
		});
}

CConsoleTab::~CConsoleTab()
{
	delete m_pUI;
	delete m_pCommandHistory;
	delete m_pCommandList;
}

// prints log message to console
void CConsoleTab::Log(int level, const std::string& msg)
{
	// set color for message
	switch (level)
	{
	case CON_ERROR:
	case CON_FATAL_ERROR:
		m_pUI->History->setTextColor(Qt::red);
		break;
	case CON_WARNING:
		m_pUI->History->setTextColor(Qt::yellow);
		break;
	default:
		m_pUI->History->setTextColor(Qt::black);
	};

	// move to the end and insert text
	m_pUI->History->moveCursor(QTextCursor::End);
	m_pUI->History->insertPlainText(QString::fromLocal8Bit(msg.c_str()));

	// restore color
	m_pUI->History->setTextColor(Qt::black);
}

void CConsoleTab::SubmitClicked()
{
	QString text = m_pUI->Entry->text();

	g_pEvent->AddEventConsoleCommand(text.toStdString());

	// update command history list
	QStringListModel* model = (QStringListModel*)(m_pCommandHistory->model());
	if (!model)
		model = new QStringListModel();

	QStringList list = model->stringList();
	if (!list.contains(text))
		list.append(text);

	model->setStringList(list);
	m_pCommandHistory->setModel(model);

	m_pUI->Entry->clear();
}

void CConsoleTab::TextChanged(const QString& text)
{
	// change completer back to m_pCommandList
	if (m_pUI->Entry->completer() != m_pCommandList)
		m_pUI->Entry->setCompleter(m_pCommandList);
}

bool CConsoleTab::eventFilter(QObject* obj, QEvent* event)
{
	if (event->type() == QEvent::KeyPress)
	{
		if (obj == m_pUI->Entry)
		{
			// catch enter/return key pressed
			QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
			if (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return)
			{
				SubmitClicked();
			}
			else if (keyEvent->key() == Qt::Key_Down || keyEvent->key() == Qt::Key_Up)
			{
				// show input history
				QAbstractItemView* a = m_pUI->Entry->completer()->popup();

				// if no command list, show last commands
				if (!m_pCommandList->popup()->isVisible()) 
				{
					if (m_pUI->Entry->completer() != m_pCommandHistory)
						m_pUI->Entry->setCompleter(m_pCommandHistory);

					m_pCommandHistory->complete();
				}
			}
		}
	}
	return QObject::eventFilter(obj, event);
}

void CConsoleTab::OnCommandListUpdated(const std::vector<std::string>& cmdList)
{
	m_CmdList.clear();
	for (auto& str : cmdList)
	{
		m_CmdList.push_back(QString::fromStdString(str));
	}

	if (m_pCommandList)
		delete m_pCommandList;

	m_pCommandList = new QCompleter(m_CmdList);
	m_pUI->Entry->setCompleter(m_pCommandList);
}