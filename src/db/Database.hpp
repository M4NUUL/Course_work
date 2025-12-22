#pragma once
#include <QString>
#include <QSqlDatabase>

class Database {
public:
    static Database& instance();
    bool open();
    QSqlDatabase get() const;
    void close();

private:
    Database() = default;
    QSqlDatabase db;
};
