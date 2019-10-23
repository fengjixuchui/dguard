//
//	Author:
//			Burlutsky Stanislav
//			burluckij@gmail.com
//	
//  Copyright (C) Burlutsky Stanislav (burluckij@gmail.com). All Rights Reserved.
//

#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QCheckBox>
#include <QTreeWidget>
#include <QProgressBar>
#include <QStylePainter>
#include <QLineEdit>
#include <QCommandLinkButton>

#include "../ExtendedTree.h"
#include "../logic/DgiCommon.h"

namespace view
{
    class WgtAuthChangePass;

	class WgtAuth : public QWidget
    {
		Q_OBJECT

	public:

        WgtAuth(QWidget *parent = 0);

    signals:
        void signalLogged();

    public slots:
        void handlerLogin();
        void handlerChangePassword();
        void handlerPasswordWasChanged(bool);
        void handlerShowPassword(int);

	private:

		QLabel* m_header;
		QLabel* m_description;
		QPushButton* m_login;
		QPushButton* m_cancel;
        QLineEdit* m_password;
        QCheckBox* m_checkShowPassword;
        QCommandLinkButton* m_changePassword;

        WgtAuthChangePass* m_changePasswordWindow;
	};


    class WgtAuthChangePass : public QWidget
    {
        Q_OBJECT

    public:

        WgtAuthChangePass(QWidget *parent = 0);

    signals:
        void signalChanged(bool);
        void signalCancel();

    public slots:
        void handlerChange();
        void handlerCancel();
        void handlerShowPassword(int);

    private:

        QLabel* m_header;
        QLabel* m_lblCurrentPassword;
        QLabel* m_lblNewPassword;
        QLabel* m_lblNewPasswordConfirm;

        QLineEdit* m_editCurrentPassword;
        QLineEdit* m_editNewPassword;
        QLineEdit* m_editNewPasswordConfirm;

        QCheckBox* m_checkShowPassword;

        QPushButton* m_changePassword;
        QPushButton* m_cancel;
    };

    class WgtAuthSetPass : public QWidget
    {
        Q_OBJECT

    public:

        WgtAuthSetPass(QWidget *parent = 0);

    signals:
        void signalPasswordSet(std::wstring _pass);
        void signalCancel();

    public slots:
        void handlerSet();
        void handlerCancel();
        void handlerShowPassword(int);

    private:

        QLabel* m_header;
        QLabel* m_lblNewPassword;
        QLabel* m_lblNewPasswordConfirm;

        QLineEdit* m_editNewPassword;
        QLineEdit* m_editNewPasswordConfirm;

        QCheckBox* m_checkShowPassword;

        QPushButton* m_setPassword;
        QPushButton* m_cancel;
    };
}
