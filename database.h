#ifndef DATABASE_H
#define DATABASE_H

#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QDebug>

/*
    This file defines a helper function to open a connection to an
    in-memory SQLITE database and to create a test table.

    If you want to use another database, simply modify the code
    below. All the examples in this directory use this function to
    connect to a database.
*/
//! [0]
bool createConnection(QSqlDatabase *db)
{

//    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db->setDatabaseName(":memory:");

    if (!db->open()) {
        QMessageBox::critical(nullptr, QObject::tr("Cannot open database"),
            QObject::tr("Unable to establish a database connection.\n"
                        "This example needs SQLite support. Please read "
                        "the Qt SQL driver documentation for information how "
                        "to build it.\n\n"
                        "Click Cancel to exit."), QMessageBox::Cancel);
        return false;
    }

    QSqlQuery query;
    query.exec("create table demanda (id int primary key, "
               "data varchar(10), hora varchar(10), posto varchar(10), "
               "ativa INTEGER, reativa INTEGER, fatorPotencia REAL, ufer varchar(3), "
               "intervaloReativo INTEGER, obs varchar(10))");

//    query.exec("insert into demanda values(101,
//    '01/01/2019', '10:50:45', 'PONTA', 10, 5, 0.7, "
//               "'CAP', 1, '')");


    return true;
}

#endif // DATABASE_H
