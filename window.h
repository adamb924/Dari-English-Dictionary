#ifndef WINDOW_H
#define WINDOW_H

#include <QMainWindow>

class QSqlDatabase;
class QLineEdit;
class QComboBox;
class QCheckBox;
class QListWidget;
class QTextEdit;

#include <QSqlDatabase>

#define GLASSMAN 0
#define ENGLISH 1
#define IPA 2
#define DARI 3

class Window : public QMainWindow
{
    Q_OBJECT

public:
    Window();
    ~Window();

    inline bool error() const { return bUnrecoverableError; }

private:
    QLineEdit *searchTextEntry;
    QComboBox *searchBy;
    QCheckBox *substringSearch;
    QListWidget *suggestions;
    QTextEdit *wordDisplay;
    QLineEdit *numberToReturn;

    QSqlDatabase db;

    bool bUnrecoverableError;

private slots:
    void searchByChanged(const QString &str);
    void updateDefinition(const QString &str);
    void updateSuggestions();
    void versionInformation();
};

#endif
