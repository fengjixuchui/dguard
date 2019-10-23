//
//	Author:
//			Burlutsky Stanislav
//			burluckij@gmail.com
//	
//  Copyright (C) Burlutsky Stanislav (burluckij@gmail.com). All Rights Reserved.
//

#include "WgtFLock.h"

#include <QObject.h>
#include <QtCore/QVariant>
#include <QAction>
#include <QApplication>
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
#include <QVariant>

#include "../../logic/DgiCommon.h"
#include "../../logic/DgiEngine.h"
#include "../../../../service/DgiService/helpers/internal/helpers.h"


#define TXT_FLOCK_HEADER		QObject::tr("Folder Lock")
#define TXT_FLOCK_DESCRIPTION	QObject::tr("Here you can lock an access and hide any files and folders which you want to protect against its leak and unauthorized access.\nAll modern file systems are supported - NTFS, ReFS and etc.")
#define TXT_FLOCK_ADD			QObject::tr("Add")
#define TXT_FLOCK_DELETE_ALL	QObject::tr("Delete all")
#define TXT_FLOCK_PATH			QObject::tr("File path")
#define TXT_FLOCK_STATUS		QObject::tr("Protection status")

#define TXT_FLOCK_DELETE_ALL_MSG            QObject::tr("Do you really want to delete protection for all files?")
#define TXT_FLOCK_MSG_CAPTION               QObject::tr("Folder Lock")
#define TXT_FLOCK_DELETE_ALL_ERROR          QObject::tr("Error. Could not clear protected list.")
#define TXT_FLOCK_CHOOSE_DIALOG             QObject::tr("Please choose directory which you want to protect")
#define TXT_FLOCK_CHOOSE_FILE_DIALOG        QObject::tr("Please choose file which you want to protect")
#define TXT_FLOCK_ADD_FILE_ERROR            QObject::tr("Error - could not add file.")
#define TXT_FLOCK_ADD_DIR_ERROR             QObject::tr("Error - could not add directory.")
#define TXT_FLOCK_ADD_FS_NOT_SUPPORTED      QObject::tr("Sorry, but the target file system is not supported.\nYou can protect files and directories only for NTFS and ReFS.\nOld file systems as FAT and FAT32 are not supported.")
#define TXT_FLOCK_ADD_SUCCESS_FILE          QObject::tr("File was successfully added into protected list.\nNow nobody can get an access to the file because it is locked!")
#define TXT_FLOCK_ADD_SUCCESS_DIR           QObject::tr("Directory was successfully added into protected list.\nNow nobody can open the directory because it is locked!")
#define TXT_FLOCK_REMOVE                    QObject::tr("Do you really want to remove protection for the object?")
#define TXT_FLOCK_REMOVE_SUCCESS            QObject::tr("Success! Protection was removed.")


#define DATA_FLOCK_ROLE           2
#define DATA_FLOCK_ID             3
#define DATA_FLOCK_STATE          4
#define DATA_FLOCK_TYPE           5

#define USE_FLAT_BUTTONS		true
#define BUTTONS_SIZE			QSize(24,24)

namespace view
{
    WgtFLock::WgtFLock(QWidget *parent /*= 0*/) :
        QWidget(parent),
        m_contextChangeState(new WgtFLockContextChange()),
        m_conextAdd(new WgtFLockContextAddNew())
    {
        QVBoxLayout* pLayoutMain = new QVBoxLayout(this);
        this->setLayout(pLayoutMain);

        QHBoxLayout* pLayoutButtons = new QHBoxLayout(this);

        this->m_header = new QLabel(TXT_FLOCK_HEADER, this);
        this->m_description = new QLabel(TXT_FLOCK_DESCRIPTION, this);
        this->m_add = new QPushButton(TXT_FLOCK_ADD, this);
        this->m_removeAll = new QPushButton(TXT_FLOCK_DELETE_ALL, this);
        this->m_tree = new dguard::ExtendedTree(this);
        this->m_tree->setContextMenuPolicy(Qt::CustomContextMenu);

        pLayoutMain->addWidget(m_header);
        pLayoutMain->addWidget(m_description);

        //pLayoutMain->addStretch(10);
        //pLayoutMain->setSpacing(10);
        //pLayoutMain->insertStretch(2, 10);

        pLayoutMain->addLayout(pLayoutButtons);
        pLayoutButtons->addWidget(m_add);
        pLayoutButtons->addStretch();
        pLayoutButtons->addWidget(m_removeAll);
        pLayoutMain->addWidget(m_tree);

        this->m_tree->headerItem()->setHidden(FALSE);
        this->m_tree->headerItem()->setIcon(0, QIcon(":dgui/ico/folder.ico"));
        this->m_tree->headerItem()->setIcon(1, QIcon(":dgui/ico/security_32x32.ico"));
        this->m_tree->headerItem()->setText(0, TXT_FLOCK_PATH);
        this->m_tree->headerItem()->setText(1, TXT_FLOCK_STATUS);

        this->m_tree->headerItem()->setFont(0, QFont("Calibri", 10, QFont::Medium));
        this->m_tree->headerItem()->setFont(1, QFont("Calibri", 10, QFont::Medium));


        this->m_removeAll->setIconSize(BUTTONS_SIZE);
        this->m_removeAll->setFlat(USE_FLAT_BUTTONS);
        this->m_removeAll->setIcon(QIcon(":dgui/ico/bin.ico"));
        this->m_add->setFlat(USE_FLAT_BUTTONS);
        this->m_add->setIconSize(BUTTONS_SIZE);
        this->m_add->setIcon(QIcon(":dgui/ico/add.ico"));
        this->m_tree->setAutoScroll(true);
        this->m_tree->setColumnWidth(0, 300);
        this->m_tree->setIconSize(QSize(24, 24));

        this->show();

        QFont f("Arial", 10, QFont::Bold);
        this->m_header->setFont(f);
        this->m_description->setFont(QFont("Calibri", 10, QFont::OldEnglish));

        connect(this->m_add, SIGNAL(clicked()), this, SLOT(addNewFlock()));
        connect(this->m_removeAll, SIGNAL(clicked()), this, SLOT(deleteProtectedFiles()));
        //connect(this->m_tree, SIGNAL(itemClicked(QTreeWidgetItem*, int)), this, SLOT(handlerFLockClicked(QTreeWidgetItem*, int)));

        connect(this->m_tree, &QTreeWidget::customContextMenuRequested, this, &WgtFLock::prepareMenu);

        //
        //  Slots for handling context menu commands.
        //
        connect(this->m_contextChangeState, SIGNAL(signalToLock(QTreeWidgetItem*)), this, SLOT(handlerToLock(QTreeWidgetItem*)), Qt::DirectConnection);
        connect(this->m_contextChangeState, SIGNAL(signalToHide(QTreeWidgetItem*)), this, SLOT(handlerToHide(QTreeWidgetItem*)), Qt::DirectConnection);
        connect(this->m_contextChangeState, SIGNAL(signalToLockAndHide(QTreeWidgetItem*)), this, SLOT(handlerToLockAndHide(QTreeWidgetItem*)), Qt::DirectConnection);
        connect(this->m_contextChangeState, SIGNAL(signalToUnlock(QTreeWidgetItem*)), this, SLOT(handlerToUnlock(QTreeWidgetItem*)), Qt::DirectConnection);
        connect(this->m_contextChangeState, SIGNAL(signalToRemove(QTreeWidgetItem*)), this, SLOT(handlerToRemove(QTreeWidgetItem*)), Qt::DirectConnection);

        connect(this->m_conextAdd, SIGNAL(signalAddFile()), this, SLOT(handlerAddFile()), Qt::DirectConnection);
        connect(this->m_conextAdd, SIGNAL(signalAddDir()), this, SLOT(handlerAddDir()), Qt::DirectConnection);

        QSize SizeForButtons(BUTTONS_SIZE);

        updateProtectedFiles();
    }

    void WgtFLock::updateProtectedFiles()
    {
        std::vector<::dguard::FLOCK_INFO> protectedFiles;
        auto& engine = dguard::GetEngine();

        bool success = engine.flockGetAll(protectedFiles);

        if (success)
        {
            m_tree->clear();

            for (auto one_file : protectedFiles)
            {
                QTreeWidgetItem* flockItem = new QTreeWidgetItem();

                flockItem->setData(DATA_FLOCK_ID, DATA_FLOCK_ROLE, QByteArray(one_file.id.c_str(), one_file.id.length()));
                flockItem->setData(DATA_FLOCK_TYPE, DATA_FLOCK_ROLE, QVariant(one_file.ftype) );
                //flockItem->setData(DATA_FLOCK_STATE, DATA_FLOCK_ROLE, QVariant(one_file.state.c_));

                if (one_file.ftype == dguard::FLockType_File)
                {
                    flockItem->setIcon(0, QIcon(":dgui/ico/file_24x24.ico"));
                }
                else
                {
                    //flockItem->setIcon(0, QIcon(":dgui/ico/dir_24x24.ico"));
                    flockItem->setIcon(0, QIcon(":dgui/ico/folder.ico"));
                }

                if (one_file.state != "Unlocked")
                {
                    flockItem->setIcon(1, QIcon(":dgui/ico/locked_32x32.ico"));
                }
                else
                {
                    //flockItem->setIcon(1, QIcon(":dgui/ico/security_32x32.ico"));
                    flockItem->setIcon(1, QIcon(":dgui/ico/unlocked_32x32.ico"));
                }

                QString filepath = QString::fromStdWString(one_file.filepath);
                flockItem->setText(0, filepath);
                flockItem->setText(1, QString(one_file.state.c_str()) );

                this->m_tree->addTopLevelItem(flockItem);
            }
        }
        else
        {
            engine.getLog().print(std::string(__FUNCTION__) + ": error - failed to receive list of protected files.");
        }
    }

    void WgtFLock::addNewFlock()
    {
        this->m_conextAdd->ShowMenu();
    }

    void WgtFLock::deleteProtectedFiles()
    {
        QMessageBox msgBox;
        msgBox.setText(TXT_FLOCK_DELETE_ALL_MSG);
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setWindowTitle(TXT_FLOCK_MSG_CAPTION);
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Cancel);

        int ret = msgBox.exec();

        if (ret == QMessageBox::Cancel)
        {
            return;
        }

        auto& engine = dguard::GetEngine();
        bool success = engine.flockDeleteAll();

        if (!success)
        {
            QMessageBox msgBox;
            msgBox.setWindowTitle(TXT_FLOCK_MSG_CAPTION);
            msgBox.setText(TXT_FLOCK_DELETE_ALL_ERROR);
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.exec();
        }
        else
        {
            updateProtectedFiles();
        }
    }

    void WgtFLock::handlerFLockClicked(QTreeWidgetItem* item, int column)
    {
        //
        //  Show context menu with options for flocks list modifications.
        //

        if (this->m_tree->rightButtonClicked())
        {
            this->m_contextChangeState->ShowMenu((QTreeWidgetItem*)item);
        }

        //
        //  Update flocks list each time when user clicks on tree.
        //
        //  Be careful with 'QTreeWidgetItem* item' pointer and refresh it only after object is used.
        //

        // refreshProtectedFiles();
    }

    void WgtFLock::prepareMenu(const QPoint& pos)
    {
        int x = rand();

        QTreeWidgetItem *item = this->m_tree->itemAt(pos);

        if (item)
        {
            this->m_contextChangeState->ShowMenu((QTreeWidgetItem*)item);
        }
    }

    void WgtFLock::handlerToLock(QTreeWidgetItem* _item)
    {
        QVariant flockId = _item->data(DATA_FLOCK_ID, DATA_FLOCK_ROLE);

        std::thread(
            &WgtFLock::changeFlockState,
            this,
            std::string((const char*)flockId.toByteArray().data(), flockId.toByteArray().length()),
            ::dguard::FLockProtectionState::FLock_Locked
        ).detach();
    }

    void WgtFLock::handlerToHide(QTreeWidgetItem* _item)
    {
        QVariant flockId = _item->data(DATA_FLOCK_ID, DATA_FLOCK_ROLE);

        std::thread(
            &WgtFLock::changeFlockState,
            this,
            std::string((const char*)flockId.toByteArray().data(), flockId.toByteArray().length()),
            ::dguard::FLockProtectionState::FLock_Hidden
        ).detach();
        
        //
        //  I do not know why but when I start changeFlockState() method in a different thread, I do not receive any exceptions from Qt in a debug mode.
        //  That's strange.
        //

        //this->changeFlockState(_item->text(0).toStdWString(), ::dguard::FLockProtectionState::FLock_Hidden);
    }

    void WgtFLock::handlerToLockAndHide(QTreeWidgetItem* _item)
    {
        QVariant flockId = _item->data(DATA_FLOCK_ID, DATA_FLOCK_ROLE);

        auto flockStrId = std::string((const char*)flockId.toByteArray().data(), flockId.toByteArray().length());

        dguard::GetEngine().getLog().print(std::string(__FUNCTION__) + ": qwerty flock id is - " + flockStrId);

        std::thread(
            &WgtFLock::changeFlockState,
            this,
            std::string((const char*)flockId.toByteArray().data(), flockId.toByteArray().length()),
            ::dguard::FLockProtectionState::FLock_HiddenAndLocked
        ).detach();
    }

    void WgtFLock::handlerToUnlock(QTreeWidgetItem* _item)
    {
        QVariant flockId = _item->data(DATA_FLOCK_ID, DATA_FLOCK_ROLE);

        std::thread(
            &WgtFLock::changeFlockState,
            this,
            std::string((const char*)flockId.toByteArray().data(), flockId.toByteArray().length()),
            ::dguard::FLockProtectionState::FLock_Unlocked
        ).detach();
    }

    void WgtFLock::handlerToRemove(QTreeWidgetItem* _item)
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle(TXT_FLOCK_MSG_CAPTION);
        msgBox.setText(TXT_FLOCK_REMOVE);
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Cancel);

        int ret = msgBox.exec();

        if (ret == QMessageBox::Cancel)
        {
            return;
        }

        bool removed = dguard::GetEngine().flockRemove(_item->text(0).toStdWString());

        if (removed)
        {
            dguard::GetEngine().getLog().print(std::string(__FUNCTION__) + ": success - flock was removed.");

            updateProtectedFiles();

            QMessageBox msgBox;
            msgBox.setWindowTitle(TXT_FLOCK_MSG_CAPTION);
            msgBox.setText(TXT_FLOCK_REMOVE_SUCCESS);
            msgBox.setIcon(QMessageBox::Information);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.exec();
        }
        else
        {
            dguard::GetEngine().getLog().print(std::string(__FUNCTION__) + ": error - failed to remove flock.");
        }
    }

    void WgtFLock::changeFlockState(std::string _flockId, ::dguard::FLockProtectionState _state)
    {
        bool success = dguard::GetEngine().flockSetStateById(_flockId, _state);

        if (success)
        {
            updateProtectedFiles();
        }
        else
        {
            dguard::GetEngine().getLog().print(std::string(__FUNCTION__) + ": error - failed to change flock state to " + std::to_string((int)_state));
        }
    }

    std::string generateFlockId()
    {
        char buffer[256] = { 0 };
        //DWORD ticks = GetTickCount();
        SYSTEMTIME ft = { 0 };
        GetSystemTime(&ft);
        memcpy(buffer, &ft, sizeof(ft));

        std::string s = std::string((const char*)buffer, sizeof(ft));

        for (int i = s.length(); i < 16; ++i)
        {
            s.push_back(rand());
        }

        return s;
    }

    void WgtFLock::handlerAddFile()
    {
        QString filepath = QFileDialog::getOpenFileName(this,
            TXT_FLOCK_CHOOSE_FILE_DIALOG,
            "",
            tr("Any files (*.*)"),
            NULL,
            QFileDialog::DontUseNativeDialog);

        if (!filepath.length()) {
            return;
        }

        for (int i = 0; i < filepath.size(); ++i){
            if (filepath[i] == '/') {
                filepath[i] = '\\';
            }
        }

        bool supportedFs = false;
        if (dguard::GetEngine().flockIsSupportedFs(filepath.toStdWString(), supportedFs))
        {
            if (!supportedFs)
            {
                QMessageBox msgBox;
                msgBox.setWindowTitle(TXT_FLOCK_MSG_CAPTION);
                msgBox.setText(TXT_FLOCK_ADD_FS_NOT_SUPPORTED);
                msgBox.setIcon(QMessageBox::Warning);
                msgBox.setStandardButtons(QMessageBox::Ok);
                msgBox.exec();
            }
        }
        else
        {
            //
            //  Something went wrong and we could not verify target file system where object resides.
            //
        }

        std::string id = generateFlockId();
        bool success = dguard::GetEngine().flockAdd(filepath.toStdWString(), id, dguard::FLockFileType::FLockType_File, dguard::FLock_Locked);

        if (success)
        {
            dguard::GetEngine().getLog().print(std::string(__FUNCTION__) + ": success - new flock was added.");

            QMessageBox msgBox;
            msgBox.setWindowTitle(TXT_FLOCK_MSG_CAPTION);
            msgBox.setText(TXT_FLOCK_ADD_SUCCESS_FILE);
            msgBox.setIcon(QMessageBox::Information);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.exec();
        }
        else
        {
            dguard::GetEngine().getLog().print(std::string(__FUNCTION__) + ": error - failed to add new flock.");

            QMessageBox msgBox;
            msgBox.setWindowTitle(TXT_FLOCK_MSG_CAPTION);
            msgBox.setText(TXT_FLOCK_ADD_FILE_ERROR);
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.exec();
        }

        updateProtectedFiles();
    }

    void WgtFLock::handlerAddDir()
    {
        QString dirpath = QFileDialog::getExistingDirectory(this, TXT_FLOCK_CHOOSE_DIALOG, "", QFileDialog::DontUseNativeDialog);

        if (!dirpath.length()) {
            return;
        }

        for (int i = 0; i < dirpath.size(); ++i){
            if (dirpath[i] == '/') {
                dirpath[i] = '\\';
            }
        }

        bool supportedFs = false;
        if (dguard::GetEngine().flockIsSupportedFs(dirpath.toStdWString(), supportedFs))
        {
            if (!supportedFs)
            {
                QMessageBox msgBox;
                msgBox.setWindowTitle(TXT_FLOCK_MSG_CAPTION);
                msgBox.setText(TXT_FLOCK_ADD_FS_NOT_SUPPORTED);
                msgBox.setIcon(QMessageBox::Warning);
                msgBox.setStandardButtons(QMessageBox::Ok);
                msgBox.exec();
            }
        }
        else
        {
            //
            //  Something went wrong and we could not verify target file system where object resides.
            //
        }

        //
        //  By default we add flock with unlocked status.
        //

        std::string id = generateFlockId();
        bool success = dguard::GetEngine().flockAdd(dirpath.toStdWString(), id, dguard::FLockFileType::FLockType_Dir, dguard::FLock_Locked);

        if (success)
        {
            dguard::GetEngine().getLog().print(std::string(__FUNCTION__) + ": success - new flock was added.");

            QMessageBox msgBox;
            msgBox.setWindowTitle(TXT_FLOCK_MSG_CAPTION);
            msgBox.setText(TXT_FLOCK_ADD_SUCCESS_DIR);
            msgBox.setIcon(QMessageBox::Information);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.exec();
        }
        else
        {
            dguard::GetEngine().getLog().print(std::string(__FUNCTION__) + ": error - failed to add new flock.");

            QMessageBox msgBox;
            msgBox.setWindowTitle(TXT_FLOCK_MSG_CAPTION);
            msgBox.setText(TXT_FLOCK_ADD_DIR_ERROR);
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.exec();
        }

        updateProtectedFiles();
    }
}
