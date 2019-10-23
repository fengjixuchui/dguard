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
#include <QTreeWidget>
#include <QProgressBar>
#include <QStylePainter>
#include <QSystemTrayIcon>
#include <QTreeWidget.h>
#include <QTableWidgetItem>
#include <qmenu.h>
#include <qmenubar.h>

#include "../ExtendedTree.h"

#define TXT_ACT_LOCK                QObject::tr("Lock access")
#define TXT_ACT_HIDE                QObject::tr("Hide")
#define TXT_ACT_LOCK_AND_HIDE       QObject::tr("Lock and Hide")
#define TXT_ACT_UNLOCK              QObject::tr("Unlock")
#define TXT_ACT_REMOVE              QObject::tr("Remove")



namespace view
{
    class WgtFLockContextChange : public QWidget
    {
        Q_OBJECT

    private:
        QMenu* m_contextMenu;
        QTreeWidgetItem* m_item;
        QAction* m_actLock;
        QAction* m_actHide;
        QAction* m_actLockAndHide;
        QAction* m_actUnlock;
        QAction* m_actRemove;

    public:
        WgtFLockContextChange()
        {
            m_contextMenu = new QMenu(this);

            m_actLock = m_contextMenu->addAction(TXT_ACT_LOCK);
            m_actHide = m_contextMenu->addAction(TXT_ACT_HIDE);
            m_actLockAndHide = m_contextMenu->addAction(TXT_ACT_LOCK_AND_HIDE);
            m_contextMenu->addSeparator();
            m_actUnlock = m_contextMenu->addAction(TXT_ACT_UNLOCK);
            m_actRemove = m_contextMenu->addAction(TXT_ACT_REMOVE);

            m_actLock->setIcon(QIcon(":dgui/ico/locked_32x32.ico"));
            m_actHide->setIcon(QIcon(":dgui/ico/locked_32x32.ico"));
            m_actLockAndHide->setIcon(QIcon(":dgui/ico/locked_32x32.ico"));
            m_actUnlock->setIcon(QIcon(":dgui/ico/unlocked_32x32.ico"));
            m_actRemove->setIcon(QIcon(":dgui/ico/close.ico"));

            connect(m_contextMenu, SIGNAL(triggered(QAction*)), this, SLOT(slotActivated(QAction*)));
        }

        QMenu* Get()
        {
            return m_contextMenu;
        }

        void ShowMenu(QTreeWidgetItem* i)
        {
            m_item = i;
            m_contextMenu->exec(QCursor::pos());
        }

    signals:
        void signalToLock(QTreeWidgetItem*);
        void signalToHide(QTreeWidgetItem*);
        void signalToLockAndHide(QTreeWidgetItem*);
        void signalToUnlock(QTreeWidgetItem*);
        void signalToRemove(QTreeWidgetItem*);


    public slots:

        void slotActivated(QAction* pAction)
        {
            if (pAction == m_actLock)
            {
                emit signalToLock(m_item);
            }
            else if (pAction == m_actHide)
            {
                emit signalToHide(m_item);
            }
            else if (pAction == m_actLockAndHide)
            {
                emit signalToLockAndHide(m_item);
            } 
            else if (pAction == m_actUnlock)
            {
                emit signalToUnlock(m_item);
            }
            else if (pAction == m_actRemove)
            {
                emit signalToRemove(m_item);
            }
        }
    };

}
