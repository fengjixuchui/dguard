
#include <QVBoxLayout>
#include "dgui.h"
#include "logic/DgiEngine.h"
#include <QWidget>
#include <QLabel>
#include <QObject.h>


WgtNotAvailableYet::WgtNotAvailableYet(QWidget *parent ) :QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    QLabel* lbl = new QLabel(QString("This functionality is not available in a free version..."), this);

    lbl->setFont(QFont("Calibri", 11, QFont::OldEnglish));

    this->setLayout(layout);

    layout->addWidget(lbl);
}

dgui::dgui(QWidget *parent)
    : QMainWindow(parent)
{

    this->resize(600, 400);

//     initTabs();
// 
//     setCentralWidget(m_menu);
// 
//     this->setWindowTitle("Data Guard for Windows. (Free version)");
// 
//     //m_menu->show();
//     //ui.setupUi(this);
// 
//     this->show();
}

void dgui::handlerStart()
{
    initTabs();

//     QVBoxLayout *lay = new QVBoxLayout(this);
//     QLabel* label = new QLabel("burluckij@gmail.com", m_menu);
// 
//     this->setLayout(lay);
//     lay->addWidget(label);
//     lay->addWidget(m_menu);
//     lay->addStretch();

    setCentralWidget(m_menu);

    this->setWindowTitle("Data Guard for Windows (Free version)");

    this->show();

    this->move(qApp->desktop()->screen()->rect().center() - this->rect().center());

    this->setFixedSize(this->geometry().width(), this->geometry().height());
}

void dgui::handlerPasswordSet(std::wstring _pass)
{
    bool mprSet, wrongPass;
    std::string sid = dguard::GetEngine().login(_pass, mprSet, wrongPass);

    if (sid.empty())
    {
        MessageBoxA(0, "Error - application can not be opened with a provided master-password.", "Data Guard", MB_ICONWARNING);
    }
    else
    {
        handlerStart();
    }
}

void dgui::initTabs()
{
    m_menu = new QTabWidget(this);

    m_menu->setMouseTracking(false);
    m_menu->setContextMenuPolicy(Qt::DefaultContextMenu);
    m_menu->setLayoutDirection(Qt::LeftToRight);
    m_menu->setAutoFillBackground(false);
    m_menu->setTabPosition(QTabWidget::North); // QTabWidget::West
    m_menu->setTabShape(QTabWidget::Rounded); // QTabWidget::Rounded
    m_menu->setIconSize(QSize(24, 24));
    m_menu->setElideMode(Qt::ElideNone);
    m_menu->setUsesScrollButtons(true);
    m_menu->setDocumentMode(true); // false
    m_menu->setTabsClosable(false);
    m_menu->setMovable(true);

    m_wgtFlock = new view::WgtFLock();
    m_menu->addTab(m_wgtFlock, QString());
    m_menu->setTabText(m_menu->indexOf(m_wgtFlock), QObject::tr("Folder Lock"));
    m_menu->setTabIcon(m_menu->indexOf(m_wgtFlock), QIcon(":dgui/ico/folderlock_32x32.ico"));

    //m_wgtShredder = new view::WgtShredder();
    auto *wgtShredder = new WgtNotAvailableYet();
    m_menu->addTab(wgtShredder, QString());
    m_menu->setTabText(m_menu->indexOf(wgtShredder), QObject::tr("Shredder"));
    m_menu->setTabIcon(m_menu->indexOf(wgtShredder), QIcon(":dgui/ico/shredder_32x32.ico"));

    auto *wgtFileEncryption = new WgtNotAvailableYet();
    m_menu->addTab(wgtFileEncryption, QString());
    m_menu->setTabText(m_menu->indexOf(wgtFileEncryption), QObject::tr("Encryption"));
    m_menu->setTabIcon(m_menu->indexOf(wgtFileEncryption), QIcon(":dgui/ico/encryption_32x32.ico"));

#ifndef DGI_FOLDER_LOCK_ONLY

    m_wgtEncryption = new view::WgtEncryption();
    m_menu->addTab(m_wgtEncryption, QString());
    m_menu->setTabText(m_menu->indexOf(m_wgtEncryption), QObject::tr("File encryption"));
    m_menu->setTabIcon(m_menu->indexOf(m_wgtEncryption), QIcon(":Data_Guard/ico/security.ico"));

    m_wgtBankcards = new view::WgtBankcards();
    m_menu->addTab(m_wgtBankcards, QString());
    m_menu->setTabText(m_menu->indexOf(m_wgtBankcards), QObject::tr("Bank cards"));
    m_menu->setTabIcon(m_menu->indexOf(m_wgtBankcards), QIcon(":Data_Guard/ico/card.ico"));


    auto *wgt = new WgtNotAvailableYet();
    m_menu->addTab(wgt, QString("Settings"));
    m_menu->setTabText(m_menu->indexOf(wgt), QObject::tr("Settings"));
    m_menu->setTabIcon(m_menu->indexOf(wgt), QIcon(":dgui/ico/settings.ico"));

#endif

}


