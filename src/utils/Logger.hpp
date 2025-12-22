#pragma once
#include <QString>

class Logger {
public:
    static void log(int userId, const QString &action, const QString &details);
};
