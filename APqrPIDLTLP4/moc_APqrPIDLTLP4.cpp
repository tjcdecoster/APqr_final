/****************************************************************************
** Meta object code from reading C++ file 'APqrPIDLTLP4.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.5.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "APqrPIDLTLP4.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'APqrPIDLTLP4.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.5.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_APqrPIDLTLP4_t {
    QByteArrayData data[4];
    char stringdata0[35];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_APqrPIDLTLP4_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_APqrPIDLTLP4_t qt_meta_stringdata_APqrPIDLTLP4 = {
    {
QT_MOC_LITERAL(0, 0, 12), // "APqrPIDLTLP4"
QT_MOC_LITERAL(1, 13, 8), // "loadFile"
QT_MOC_LITERAL(2, 22, 0), // ""
QT_MOC_LITERAL(3, 23, 11) // "previewFile"

    },
    "APqrPIDLTLP4\0loadFile\0\0previewFile"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_APqrPIDLTLP4[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   29,    2, 0x08 /* Private */,
       1,    1,   30,    2, 0x08 /* Private */,
       3,    0,   33,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    2,
    QMetaType::Void,

       0        // eod
};

void APqrPIDLTLP4::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        APqrPIDLTLP4 *_t = static_cast<APqrPIDLTLP4 *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->loadFile(); break;
        case 1: _t->loadFile((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 2: _t->previewFile(); break;
        default: ;
        }
    }
}

const QMetaObject APqrPIDLTLP4::staticMetaObject = {
    { &DefaultGUIModel::staticMetaObject, qt_meta_stringdata_APqrPIDLTLP4.data,
      qt_meta_data_APqrPIDLTLP4,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *APqrPIDLTLP4::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *APqrPIDLTLP4::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_APqrPIDLTLP4.stringdata0))
        return static_cast<void*>(const_cast< APqrPIDLTLP4*>(this));
    return DefaultGUIModel::qt_metacast(_clname);
}

int APqrPIDLTLP4::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = DefaultGUIModel::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 3;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
