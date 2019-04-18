#ifndef WINDOW_H
#define WINDOW_H

#include <QMainWindow>

class QSqlDatabase;

#include <QSqlDatabase>

namespace Ui {
    class MainWindow;
}

class Window : public QMainWindow
{
    Q_OBJECT

public:
    enum SearchKeys { Glassman, English, Dari, IPA };

    explicit Window(QWidget *parent = nullptr);
    ~Window();

    inline bool error() const { return mUnrecoverableError; }

protected:
    void keyReleaseEvent(QKeyEvent * event);

private:
    Ui::MainWindow *ui;
    QSqlDatabase mDb;
    bool mUnrecoverableError;

    Window::SearchKeys columnNameForSearching() const;

    void searchFromBeginning(QString searchString);
    void searchAnySubstring(QString searchString);
    void searchRegularExpression(QString searchString);

private slots:
    void searchByChanged();
    void updateDefinition(const QString &str);
    void updateSuggestions();
    void versionInformation();
};

#endif
