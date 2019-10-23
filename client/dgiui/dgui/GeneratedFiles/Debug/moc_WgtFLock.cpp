/****************************************************************************
** Meta object code from reading C++ file 'WgtFLock.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.13.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../view/flock/WgtFLock.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'WgtFLock.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.13.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_view__WgtFLock_t {
    QByteArrayData data[17];
    char stringdata0[224];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_view__WgtFLock_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_view__WgtFLock_t qt_meta_stringdata_view__WgtFLock = {
    {
QT_MOC_LITERAL(0, 0, 14), // "view::WgtFLock"
QT_MOC_LITERAL(1, 15, 11), // "addNewFlock"
QT_MOC_LITERAL(2, 27, 0), // ""
QT_MOC_LITERAL(3, 28, 20), // "deleteProtectedFiles"
QT_MOC_LITERAL(4, 49, 19), // "handlerFLockClicked"
QT_MOC_LITERAL(5, 69, 16), // "QTreeWidgetItem*"
QT_MOC_LITERAL(6, 86, 4), // "item"
QT_MOC_LITERAL(7, 91, 6), // "column"
QT_MOC_LITERAL(8, 98, 11), // "prepareMenu"
QT_MOC_LITERAL(9, 110, 3), // "pos"
QT_MOC_LITERAL(10, 114, 13), // "handlerToLock"
QT_MOC_LITERAL(11, 128, 13), // "handlerToHide"
QT_MOC_LITERAL(12, 142, 20), // "handlerToLockAndHide"
QT_MOC_LITERAL(13, 163, 15), // "handlerToUnlock"
QT_MOC_LITERAL(14, 179, 15), // "handlerToRemove"
QT_MOC_LITERAL(15, 195, 14), // "handlerAddFile"
QT_MOC_LITERAL(16, 210, 13) // "handlerAddDir"

    },
    "view::WgtFLock\0addNewFlock\0\0"
    "deleteProtectedFiles\0handlerFLockClicked\0"
    "QTreeWidgetItem*\0item\0column\0prepareMenu\0"
    "pos\0handlerToLock\0handlerToHide\0"
    "handlerToLockAndHide\0handlerToUnlock\0"
    "handlerToRemove\0handlerAddFile\0"
    "handlerAddDir"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_view__WgtFLock[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      11,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   69,    2, 0x0a /* Public */,
       3,    0,   70,    2, 0x0a /* Public */,
       4,    2,   71,    2, 0x0a /* Public */,
       8,    1,   76,    2, 0x0a /* Public */,
      10,    1,   79,    2, 0x0a /* Public */,
      11,    1,   82,    2, 0x0a /* Public */,
      12,    1,   85,    2, 0x0a /* Public */,
      13,    1,   88,    2, 0x0a /* Public */,
      14,    1,   91,    2, 0x0a /* Public */,
      15,    0,   94,    2, 0x0a /* Public */,
      16,    0,   95,    2, 0x0a /* Public */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 5, QMetaType::Int,    6,    7,
    QMetaType::Void, QMetaType::QPoint,    9,
    QMetaType::Void, 0x80000000 | 5,    2,
    QMetaType::Void, 0x80000000 | 5,    2,
    QMetaType::Void, 0x80000000 | 5,    2,
    QMetaType::Void, 0x80000000 | 5,    2,
    QMetaType::Void, 0x80000000 | 5,    2,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void view::WgtFLock::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<WgtFLock *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->addNewFlock(); break;
        case 1: _t->deleteProtectedFiles(); break;
        case 2: _t->handlerFLockClicked((*reinterpret_cast< QTreeWidgetItem*(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 3: _t->prepareMenu((*reinterpret_cast< const QPoint(*)>(_a[1]))); break;
        case 4: _t->handlerToLock((*reinterpret_cast< QTreeWidgetItem*(*)>(_a[1]))); break;
        case 5: _t->handlerToHide((*reinterpret_cast< QTreeWidgetItem*(*)>(_a[1]))); break;
        case 6: _t->handlerToLockAndHide((*reinterpret_cast< QTreeWidgetItem*(*)>(_a[1]))); break;
        case 7: _t->handlerToUnlock((*reinterpret_cast< QTreeWidgetItem*(*)>(_a[1]))); break;
        case 8: _t->handlerToRemove((*reinterpret_cast< QTreeWidgetItem*(*)>(_a[1]))); break;
        case 9: _t->handlerAddFile(); break;
        case 10: _t->handlerAddDir(); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject view::WgtFLock::staticMetaObject = { {
    &QWidget::staticMetaObject,
    qt_meta_stringdata_view__WgtFLock.data,
    qt_meta_data_view__WgtFLock,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *view::WgtFLock::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *view::WgtFLock::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_view__WgtFLock.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int view::WgtFLock::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 11)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 11;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
