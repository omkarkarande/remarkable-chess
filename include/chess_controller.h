#pragma once

#include "chess_session.h"

#include <QObject>
#include <QString>

class ChessController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString selectedSquare READ selectedSquare NOTIFY stateChanged)
    Q_PROPERTY(QString turnLabel READ turnLabel NOTIFY stateChanged)

public:
    explicit ChessController(QObject* parent = nullptr);

    [[nodiscard]] QString selectedSquare() const;
    [[nodiscard]] QString turnLabel() const;

    Q_INVOKABLE QString pieceAt(int file, int rank) const;
    Q_INVOKABLE void tap(int file, int rank);

signals:
    void stateChanged();

private:
    ChessSession session_;

    static QString notationFor(int file, int rank);
    static QString glyphFor(char piece);
};
