#include "chess_controller.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QMutex>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QStandardPaths>
#include <QTextStream>

namespace {
QMutex logMutex;

void logQtMessage(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    QMutexLocker lock(&logMutex);
    const QString directory = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(directory);
    QFile file(QDir(directory).filePath("launch.log"));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        return;
    }
    QTextStream stream(&file);
    stream << QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs) << " "
           << static_cast<int>(type) << " " << message;
    if (context.file != nullptr) {
        stream << " (" << context.file << ":" << context.line << ")";
    }
    stream << "\n";
}
}  // namespace

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    qInstallMessageHandler(logQtMessage);
    qInfo() << "Chess launch: application initialized"
            << "platform=" << QGuiApplication::platformName()
            << "quick-backend=" << qEnvironmentVariable("QT_QUICK_BACKEND")
            << "rhi-backend=" << qEnvironmentVariable("QSG_RHI_BACKEND");

    ChessController chess;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("chess", &chess);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { qCritical() << "Chess launch: QML root creation failed"; QCoreApplication::exit(1); },
                     Qt::QueuedConnection);
    engine.loadFromModule("Omi.RemarkableChess", "Main");
    if (engine.rootObjects().isEmpty()) {
        qCritical() << "Chess launch: no QML root object";
        return 1;
    }
    qInfo() << "Chess launch: QML root loaded";
    return app.exec();
}
