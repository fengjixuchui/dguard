/****************************************************************************
** Meta object code from reading C++ file 'WgtFLockContextAddNew.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.13.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../view/flock/WgtFLockContextAddNew.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'WgtFLockContextAddNew.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.13.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_view__WgtFLockContextAddNew_t {
    QByteArrayData data[7];
    char stringdata0[88];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_view__WgtFLockContextAddNew_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_view__WgtFLockContextAddNew_t qt_meta_stringdata_view__WgtFLockContextAddNew = {
    {
QT_MOC_LITERAL(0, 0, 27), // "view::WgtFLockContextAddNew"
QT_MOC_LITERAL(1, 28, 13), // "signalAddFile"
QT_MOC_LITERAL(2, 42, 0), // ""
QT_MOC_LITERAL(3, 43, 12), // "signalAddDir"
QT_MOC_LITERAL(4, 56, 13), // "slotActivated"
QT_MOC_LITERAL(5, 70, 8), // "QAction*"
QT_MOC_LITERAL(6, 79, 8) // "_pAction"

    },
    "view::WgtFLockContextAddNew\0signalAddFile\0"
    "\0signalAddDir\0slotActivated\0QAction*\0"
    "_pAction"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_view__WgtFLockContextAddNew[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   29,    2, 0x06 /* Public */,
       3,    0,   30,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       4,    1,   31,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 5,    6,

       0        // eod
};

void view::WgtFLockContextAddNew::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<WgtFLockContextAddNew *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->signalAddFile(); break;
        case 1: _t->signalAddDir(); break;
        case 2: _t->slotActivated((*reinterpret_cast< QAction*(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 2:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QAction* >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (WgtFLockContextAddNew::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&WgtFLockContextAddNew::signalAddFile)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (WgtFLockContextAddNew::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&WgtFLockContextAddNew::signalAddDir)) {
                *result = 1;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject view::WgtFLockContextAddNew::staticMetaObject = { {
    &QWidget::staticMetaObject,
    qt_meta_stringdata_view__WgtFLockContextAddNew.data,
    qt_meta_data_view__WgtFLockContextAddNew,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *view::WgtFLockContextAddNew::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *view::WgtFLockContextAddNew::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_view__WgtFLockContextAddNew.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int view::WgtFLockContextAddNew::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void view::WgtFLockContextAddNew::signalAddFile()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void view::WgtFLockContextAddNew::signalAddDir()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
