#pragma once

#include <QStyle>
#include <QDesktopWidget>
#include <QCoreApplication>
#include <QApplication>

#include "ui_dgui.h"

#include "view\flock\WgtFLock.h"
#include "view\encryption\WgtEncryption.h"
#include "view\shredder\WgtShredder.h"
#include "view\bankcards\WgtBankcards.h"

class dgui : public QMainWindow
{
    Q_OBJECT

public:
    dgui(QWidget *parent = Q_NULLPTR);

public slots:
    void handlerStart();
    void handlerPasswordSet(std::wstring _pass);

private:
    Ui::dguiClass ui;

    void initTabs();



    QTabWidget /*view::TestTabBar*/  *m_menu;

    view::WgtFLock* m_wgtFlock;
    view::WgtEncryption* m_wgtEncryption;
    view::WgtBankcards* m_wgtBankcards;
    view::WgtShredder* m_wgtShredder;
};

class WgtNotAvailableYet : public QWidget
{
    Q_OBJECT

public:
    WgtNotAvailableYet(QWidget *parent = 0);
};

