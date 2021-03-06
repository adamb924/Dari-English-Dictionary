#ifndef WINDOW_H
#define WINDOW_H

#include <QMainWindow>

class QSqlDatabase;
class QSqlQueryModel;

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

protected:
    void keyReleaseEvent(QKeyEvent * event);

private:
    Ui::MainWindow *ui;

    QSqlQueryModel * mQueryModel;

    Window::SearchKeys columnNameForSearching() const;

    void searchFromBeginning(QString searchString);
    void searchAnySubstring(QString searchString);
    void searchRegularExpression(QString searchString);

private slots:
    void searchByChanged();
    void updateDefinition(const QModelIndex & index);
    void updateSuggestions();
    void versionInformation();
};

#endif
