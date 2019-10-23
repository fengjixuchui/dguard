//
//	Author:
//			Burlutsky Stanislav
//			burluckij@gmail.com
//	
//  Copyright (C) Burlutsky Stanislav (burluckij@gmail.com). All Rights Reserved.
//

#include "WgtAuth.h"
#include <QtWidgets/QApplication>
#include <QObject.h>
#include <QtCore/QVariant>
#include <QAction>
#include <QApplication>
#include <QStyle>
#include <QDesktopWidget>
#include <QCoreApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QProgressBar>
#include <QPushButton>
#include <QTabWidget>
#include <QTreeWidget>
#include <QWidget>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QComboBox>
#include <QMessageBox>
#include <thread>
#include <memory>
#include <QFileDialog>
#include <QUrl>

#include "../../logic/DgiCommon.h"
#include "../../logic/DgiEngine.h"
#include "../../../../service/DgiService/helpers/internal/helpers.h"

#define TXT_CAPTION             ("Data Guard")
#define TXT_AUTH_CAPTION        QObject::tr("Data Guard")
#define TXT_AUTH_MESSAGE		QObject::tr("Please enter master-password before to start work with Data Guard.")
#define TXT_AUTH_LOGIN          QObject::tr("Login")
#define TXT_AUTH_SHOW_PASS      QObject::tr("Show password")
#define TXT_AUTH_CHANGE_PASS    QObject::tr("... or change password")


#define TXT_AUTH_CHANGE_PASS_DESCR  QObject::tr("Here you can change a master-password.")
#define TXT_AUTH_CHECKBOX           QObject::tr("Show passwords")
#define TXT_AUTH_CHANGE             QObject::tr("Change")
#define TXT_AUTH_CANCEL             QObject::tr("Cancel")
#define TXT_AUTH_CURRENT_PASS       QObject::tr("Current password")
#define TXT_AUTH_NEW_PASS           QObject::tr("New password")

#define TXT_AUTH_SET_PASS           QObject::tr("Please set master-password and remember it!\nIf you lose or just forget the master-password you can not work with Data Guard.")
#define TXT_AUTH_SET                QObject::tr("Set")
#define TXT_AUTH_SET_PASS_MSG       QObject::tr("Please confirm that you really want to set master-password?")


#define TXT_MB_SOME_FIELDS_EMPTY    "Error - not all fields are filled!"
#define TXT_MB_WRONG_PASS           "You entered wrong confirmation for master-password."
#define TXT_MB_MPR_ALREADY_SET      "Error - master-password is already set."
#define TXT_MB_MPR_SET              "Master-password successfully set!"
#define TXT_MB_MPR_WAS_NOT_SET_ERR  "Error - master-password was not set."

#define TXT_MB_WRONG_MPR_CONFIRMATION        "You entered wrong confirmation for master-password."
#define TXT_MB_WRONG_CURRENT_PASS            "Error - you entered wrong current password."
#define TXT_MB_YOU_TRIED_MPR_NO_SET          "Error - you tried to change master-password which was not set yet."
#define TXT_MB_MPR_SUCCESS_CHANGE            "Master password was successfully changed!"
#define TXT_MB_MPR_NOT_CHANGED               "Error - master password was not changed."

#define TXT_DGUARD_LINK                      "www.dguard.org"

#define USE_FLAT_BUTTONS		false
#define BUTTONS_SIZE			QSize(24,24)

namespace view
{
    void setDGuardLinkLabel(QLabel* lblDguardLink)
    {
        lblDguardLink->setText("<a href=\"https://www.dguard.org/\">www.dguard.org</a>");
        lblDguardLink->setTextFormat(Qt::RichText);
        lblDguardLink->setTextInteractionFlags(Qt::TextBrowserInteraction);
        lblDguardLink->setOpenExternalLinks(true);
    }

    WgtAuth::WgtAuth(QWidget *parent /*= 0*/): QWidget(parent), m_changePasswordWindow(nullptr)
    {
        QFont usedFont("Arial", 10, QFont::Bold);

        QVBoxLayout* pLayoutMain = new QVBoxLayout(this);
        QHBoxLayout* pLayoutButtons = new QHBoxLayout(this);
        QHBoxLayout* pLayoutEnterPass = new QHBoxLayout(this);

        this->setWindowTitle(TXT_AUTH_CAPTION);

        this->m_login = new QPushButton(TXT_AUTH_LOGIN, this);
        this->m_login->setIconSize(BUTTONS_SIZE);
        this->m_login->setFlat(USE_FLAT_BUTTONS);
        this->m_login->setDefault(true);
        this->m_login->setIcon(QIcon(":dgui/ico/ok.ico"));
        this->m_login->setFont(QFont("Calibri", 11, QFont::Medium));

        this->m_checkShowPassword = new QCheckBox(TXT_AUTH_SHOW_PASS, this);

        this->m_changePassword = new QCommandLinkButton(TXT_AUTH_CHANGE_PASS, this);
        this->m_password = new QLineEdit(this);
        this->m_password->setEchoMode(QLineEdit::Password);
        this->m_description = new QLabel(TXT_AUTH_MESSAGE, this);

        this->m_description->setFont(usedFont);
        this->m_description->setFont(QFont("Calibri", 10, QFont::OldEnglish));

        this->setLayout(pLayoutMain);

        pLayoutMain->addWidget(m_description);
        pLayoutMain->addLayout(pLayoutEnterPass);
        pLayoutEnterPass->addWidget(m_password);
        pLayoutEnterPass->addWidget(m_login);

        pLayoutMain->addWidget(this->m_checkShowPassword);

        pLayoutMain->addLayout(pLayoutButtons);

        QLabel* lblDguardLink = new QLabel(TXT_DGUARD_LINK, this);
        setDGuardLinkLabel(lblDguardLink);
        pLayoutMain->addWidget(lblDguardLink);

        pLayoutButtons->addWidget(m_changePassword);
        pLayoutButtons->addStretch();
        //pLayoutButtons->addWidget(m_login);

        QObject::connect(this->m_login, SIGNAL(clicked()), this, SLOT(handlerLogin()));
        QObject::connect(this->m_changePassword, SIGNAL(clicked()), this, SLOT(handlerChangePassword()));
        QObject::connect(this->m_checkShowPassword, SIGNAL(stateChanged(int)), this, SLOT(handlerShowPassword(int)));


        //this->show();
        //this->move(QApplication::desktop()->screen()->rect().center() - this->rect().center());
    }

    void WgtAuth::handlerLogin()
    {
        bool wrongPassword = false, passwordIsNotSet = false;
        std::wstring password = m_password->text().toStdWString();

        std::string sid = dguard::GetEngine().login(password, wrongPassword, passwordIsNotSet);

        if (!sid.empty())
        {
            dguard::GetEngine().getLog().print(std::string(__FUNCTION__) + ": success - user logged.");

            emit signalLogged();

            //
            //  Hide window after success logging.
            //

            this->hide();
        }
        else
        {
            //
            //  Handle error cases.
            //

            dguard::GetEngine().getLog().print(std::string(__FUNCTION__) + ": error - failed to login.");

            if (wrongPassword)
            {
                dguard::GetEngine().getLog().print(std::string(__FUNCTION__) + ": error - wrong password.");

                MessageBoxA(0, "Error - wrong password.", TXT_CAPTION, MB_ICONWARNING);
            }
            else if (passwordIsNotSet)
            {
                dguard::GetEngine().getLog().print(std::string(__FUNCTION__) + ": error - password was not set.");

                MessageBoxA(0, "Error - master password is not set.", TXT_CAPTION, MB_ICONWARNING);
            }
            else if (!wrongPassword && !passwordIsNotSet)
            {
                dguard::GetEngine().getLog().print(std::string(__FUNCTION__) + ": error - unknown error has occurred.");

                MessageBoxA(0, "Unknown error.", TXT_CAPTION, MB_ICONWARNING);
            }
        }
    }

    void WgtAuth::handlerChangePassword()
    {
        this->m_password->clear();
        this->hide();

        m_changePasswordWindow = new WgtAuthChangePass();

        QObject::connect(m_changePasswordWindow, SIGNAL(signalChanged(bool)), this, SLOT(handlerPasswordWasChanged(bool)) );

        m_changePasswordWindow->show();
        m_changePasswordWindow->move(qApp->desktop()->screen()->rect().center() - m_changePasswordWindow->rect().center());
        m_changePasswordWindow->setFixedSize(m_changePasswordWindow->geometry().width(), m_changePasswordWindow->geometry().height());
    }

    void WgtAuth::handlerPasswordWasChanged(bool)
    {
        if (m_changePasswordWindow)
        {
            delete m_changePasswordWindow;
            m_changePasswordWindow = nullptr;
        }
        
        this->show();
    }

    void WgtAuth::handlerShowPassword(int _checked)
    {
        if (_checked == Qt::Checked)
        {
            this->m_password->setEchoMode(QLineEdit::Normal);
        }
        else
        {
            this->m_password->setEchoMode(QLineEdit::Password);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////

    WgtAuthChangePass::WgtAuthChangePass(QWidget *parent /*= 0*/): QWidget(parent)
    {
        this->setWindowTitle(TXT_AUTH_CAPTION);

        QFont usedFont("Arial", 10, QFont::Bold);

        QVBoxLayout* layoutMain = new QVBoxLayout(this);

        this->setLayout(layoutMain);

        this->m_header = new QLabel(TXT_AUTH_CHANGE_PASS_DESCR, this);

        this->m_lblCurrentPassword = new QLabel(TXT_AUTH_CURRENT_PASS, this);
        this->m_lblNewPassword = new QLabel(TXT_AUTH_NEW_PASS, this);
        this->m_lblNewPasswordConfirm = new QLabel(TXT_AUTH_NEW_PASS, this);

        this->m_editCurrentPassword = new QLineEdit(this);
        this->m_editNewPassword = new QLineEdit(this);
        this->m_editNewPasswordConfirm = new QLineEdit(this);

        this->m_checkShowPassword = new QCheckBox(TXT_AUTH_CHECKBOX, this);

        this->m_changePassword = new QPushButton(TXT_AUTH_CHANGE, this);
        this->m_cancel = new QPushButton(TXT_AUTH_CANCEL, this);

        //
        //  Init.
        //

        this->m_header->setFont(usedFont);
        this->m_header->setFont(QFont("Calibri", 10, QFont::OldEnglish));

        layoutMain->addWidget(this->m_header);
        layoutMain->addSpacing(20);

        QHBoxLayout* layoutCurrentPass = new QHBoxLayout(this);
        layoutCurrentPass->addWidget(this->m_lblCurrentPassword);
        layoutCurrentPass->addWidget(this->m_editCurrentPassword);
        layoutMain->addLayout(layoutCurrentPass);
        layoutMain->addSpacing(10);

        QHBoxLayout* layoutNewPass = new QHBoxLayout(this);
        layoutNewPass->addWidget(this->m_lblNewPassword);
        layoutNewPass->addWidget(this->m_editNewPassword);
        layoutMain->addLayout(layoutNewPass);

        QHBoxLayout* layoutNewPassConfirm = new QHBoxLayout(this);
        layoutNewPassConfirm->addWidget(this->m_lblNewPasswordConfirm);
        layoutNewPassConfirm->addWidget(this->m_editNewPasswordConfirm);
        layoutMain->addLayout(layoutNewPassConfirm);

        layoutMain->addSpacing(10);

        layoutMain->addWidget(this->m_checkShowPassword);

        QHBoxLayout* layoutButtons = new QHBoxLayout(this);
        layoutButtons->addStretch();
        layoutButtons->addWidget(this->m_changePassword);
        layoutButtons->addWidget(this->m_cancel);
        layoutMain->addLayout(layoutButtons);

        QLabel* lblDguardLink = new QLabel(TXT_DGUARD_LINK, this);
        setDGuardLinkLabel(lblDguardLink);
        layoutMain->addWidget(lblDguardLink);

        this->m_changePassword->setDefault(true);
        this->m_editCurrentPassword->setEchoMode(QLineEdit::Password);
        this->m_editNewPassword->setEchoMode(QLineEdit::Password);
        this->m_editNewPasswordConfirm->setEchoMode(QLineEdit::Password);

        QObject::connect(m_checkShowPassword, SIGNAL(stateChanged(int)), this, SLOT(handlerShowPassword(int)));
        QObject::connect(m_changePassword, SIGNAL(clicked()), this, SLOT(handlerChange()));
        QObject::connect(m_cancel, SIGNAL(clicked()), this, SLOT(handlerCancel()));
    }

    void WgtAuthChangePass::handlerChange()
    {
        std::wstring current = this->m_editCurrentPassword->text().toStdWString();
        std::wstring newpass = this->m_editNewPassword->text().toStdWString();

        //
        //  Do some verification stuff.
        //

        if (this->m_editNewPassword->text().isEmpty() || this->m_editNewPasswordConfirm->text().isEmpty() || this->m_editCurrentPassword->text().isEmpty())
        {
            MessageBoxA(0, TXT_MB_SOME_FIELDS_EMPTY, TXT_CAPTION, MB_ICONWARNING);
            return;
        }

        if (this->m_editNewPassword->text() != this->m_editNewPasswordConfirm->text())
        {
            MessageBoxA(0, TXT_MB_WRONG_MPR_CONFIRMATION, TXT_CAPTION, MB_OK);
            return;
        }

        bool wrongPass = false, passIsNotSet = false;
        auto sid = dguard::GetEngine().login(current, passIsNotSet, wrongPass);

        if (sid.empty())
        {
            if (wrongPass)
            {
                MessageBoxA(0, TXT_MB_WRONG_CURRENT_PASS, TXT_CAPTION, MB_ICONWARNING);
                return;
            }

            if (passIsNotSet)
            {
                MessageBoxA(0, TXT_MB_YOU_TRIED_MPR_NO_SET, TXT_CAPTION, MB_ICONWARNING);
                return;
            }
        }

        bool changed = dguard::GetEngine().changeMasterPassword(current, newpass);

        if (changed)
        {
            MessageBoxA(0, TXT_MB_MPR_SUCCESS_CHANGE, TXT_CAPTION, MB_OK);
            
            this->hide();

            emit signalChanged(true);
            
            return;
        }
        else
        {
            MessageBoxA(0, TXT_MB_MPR_NOT_CHANGED, TXT_CAPTION, MB_ICONWARNING);

            this->hide();

            emit signalChanged(false);

            return;
        }
    }

    void WgtAuthChangePass::handlerCancel()
    {
        this->hide();

        emit signalChanged(false);
    }

    void WgtAuthChangePass::handlerShowPassword(int _checked)
    {
        if (_checked == Qt::Checked)
        {
            this->m_editCurrentPassword->setEchoMode(QLineEdit::Normal);
            this->m_editNewPassword->setEchoMode(QLineEdit::Normal);
            this->m_editNewPasswordConfirm->setEchoMode(QLineEdit::Normal);
        }
        else
        {
            this->m_editCurrentPassword->setEchoMode(QLineEdit::Password);
            this->m_editNewPassword->setEchoMode(QLineEdit::Password);
            this->m_editNewPasswordConfirm->setEchoMode(QLineEdit::Password);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////

    WgtAuthSetPass::WgtAuthSetPass(QWidget *parent /*= 0*/)
    {
        this->setWindowTitle(TXT_AUTH_CAPTION);

        QVBoxLayout* layoutMain = new QVBoxLayout(this);

        this->setLayout(layoutMain);

        this->m_header = new QLabel(TXT_AUTH_SET_PASS, this);

        this->m_lblNewPassword = new QLabel(TXT_AUTH_NEW_PASS, this);
        this->m_lblNewPasswordConfirm = new QLabel(TXT_AUTH_NEW_PASS, this);

        this->m_editNewPassword = new QLineEdit(this);
        this->m_editNewPasswordConfirm = new QLineEdit(this);

        this->m_checkShowPassword = new QCheckBox(TXT_AUTH_CHECKBOX, this);

        this->m_setPassword = new QPushButton(TXT_AUTH_SET, this);
        this->m_cancel = new QPushButton(TXT_AUTH_CANCEL, this);


        //
        //  Init.
        //

        this->m_header->setFont(QFont("Arial", 10, QFont::Bold));
        this->m_header->setFont(QFont("Calibri", 10, QFont::OldEnglish));

        layoutMain->addWidget(this->m_header);
        layoutMain->addSpacing(20);

        QHBoxLayout* layoutNewPass = new QHBoxLayout(this);
        layoutNewPass->addWidget(this->m_lblNewPassword);
        layoutNewPass->addWidget(this->m_editNewPassword);
        layoutMain->addLayout(layoutNewPass);

        QHBoxLayout* layoutNewPassConfirm = new QHBoxLayout(this);
        layoutNewPassConfirm->addWidget(this->m_lblNewPasswordConfirm);
        layoutNewPassConfirm->addWidget(this->m_editNewPasswordConfirm);
        layoutMain->addLayout(layoutNewPassConfirm);

        layoutMain->addSpacing(10);

        layoutMain->addWidget(this->m_checkShowPassword);

        QHBoxLayout* layoutButtons = new QHBoxLayout(this);
        layoutButtons->addStretch();
        layoutButtons->addWidget(this->m_setPassword);
        layoutButtons->addWidget(this->m_cancel);
        layoutMain->addLayout(layoutButtons);

        QLabel* lblDguardLink = new QLabel(TXT_DGUARD_LINK, this);
        setDGuardLinkLabel(lblDguardLink);
        layoutMain->addWidget(lblDguardLink);

        this->m_setPassword->setDefault(true);
        this->m_editNewPassword->setEchoMode(QLineEdit::Password);
        this->m_editNewPasswordConfirm->setEchoMode(QLineEdit::Password);

        QObject::connect(m_checkShowPassword, SIGNAL(stateChanged(int)), this, SLOT(handlerShowPassword(int)));
        QObject::connect(m_setPassword, SIGNAL(clicked()), this, SLOT(handlerSet()));
        QObject::connect(this->m_cancel, SIGNAL(clicked()), this, SLOT(handlerCancel()));
    }

    void WgtAuthSetPass::handlerSet()
    {
        std::wstring newpass = this->m_editNewPassword->text().toStdWString();
        std::wstring newpassConfirm = this->m_editNewPasswordConfirm->text().toStdWString();

        QMessageBox msgBox;
        msgBox.setWindowTitle(TXT_AUTH_CAPTION);
        msgBox.setText(TXT_AUTH_SET_PASS_MSG);
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Cancel);

        int ret = msgBox.exec();

        if (ret == QMessageBox::Cancel)
        {
            return;
        }

        if (this->m_editNewPassword->text().isEmpty() || this->m_editNewPasswordConfirm->text().isEmpty())
        {
            MessageBoxA(0, TXT_MB_SOME_FIELDS_EMPTY, TXT_CAPTION, MB_OK);
            return;
        }

        if (this->m_editNewPassword->text() != this->m_editNewPasswordConfirm->text())
        {
            MessageBoxA(0, TXT_MB_WRONG_PASS, TXT_CAPTION, MB_OK);
            return;
        }

        if (dguard::GetEngine().isPasswordSet())
        {
            MessageBoxA(0, TXT_MB_MPR_ALREADY_SET, TXT_CAPTION, MB_ICONWARNING);
            return;
        }

        bool passwordSet = dguard::GetEngine().setMasterPassword(newpass);

        if (passwordSet)
        {
            dguard::GetEngine().getLog().print(std::string(__FUNCTION__) + ": success - master-password is set.");

            MessageBoxA(0, TXT_MB_MPR_SET, TXT_CAPTION, MB_OK);

            emit signalPasswordSet(newpass);

            this->hide();
        }
        else
        {
            dguard::GetEngine().getLog().print(std::string(__FUNCTION__) + ": error - master-password was not set.");

            MessageBoxA(0, TXT_MB_MPR_WAS_NOT_SET_ERR, TXT_CAPTION, MB_ICONWARNING);
        }
    }

    void WgtAuthSetPass::handlerCancel()
    {
        this->hide();

        emit signalCancel();

        TerminateProcess(GetCurrentProcess(), 1);
    }

    void WgtAuthSetPass::handlerShowPassword(int _checked)
    {
        if (_checked == Qt::Checked)
        {
            this->m_editNewPassword->setEchoMode(QLineEdit::Normal);
            this->m_editNewPasswordConfirm->setEchoMode(QLineEdit::Normal);
        }
        else
        {
            this->m_editNewPassword->setEchoMode(QLineEdit::Password);
            this->m_editNewPasswordConfirm->setEchoMode(QLineEdit::Password);
        }
    }

}

