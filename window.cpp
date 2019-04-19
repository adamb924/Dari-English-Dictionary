#include <QtWidgets>
#include <QtSql>
#include <QtDebug>
#include <QListView>
#include <QSqlDatabase>

#include "window.h"
#include "ui_mainwindow.h"

Window::Window(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    mUnrecoverableError=false;

    if(!QSqlDatabase::isDriverAvailable("QSQLITE"))
    {
        QMessageBox::critical (nullptr,"Fatal error", "The driver for the database is not available. It is unlikely that you will solve this on your own. Rather you had better contact the developer.");
        mUnrecoverableError=true;
        return;
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");

    QDir appFolder( QCoreApplication::applicationDirPath() );
    QString dbPath = appFolder.absoluteFilePath("dedic2.db");
    if( !appFolder.exists("dedic2.db") )
    {
        QMessageBox::critical (this,tr("Error Message"),tr("The database file does not exist (%1)").arg(dbPath) );
        mUnrecoverableError=true;
        return;
    }

    db.setConnectOptions("QSQLITE_ENABLE_REGEXP");

    db.setDatabaseName( dbPath );
    if(!db.open())
    {
        QMessageBox::critical (this, tr("Error Message"), tr("There was a problem in opening the database. The database said: %1. It is unlikely that you will solve this on your own. Rather you had better contact the developer.").arg(db.lastError().databaseText()) );
        mUnrecoverableError=true;
        return;
    }

    db.exec("PRAGMA case_sensitive_like=ON;");

    mQueryModel = new QSqlQueryModel;

    connect(ui->comboBox, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(searchByChanged()));
    connect(ui->listView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(updateDefinition(const QModelIndex &)));
    connect(ui->searchEdit, SIGNAL(textEdited(const QString &)), this, SLOT(updateSuggestions()));
    connect(ui->versionButton, SIGNAL(clicked(bool)), this, SLOT(versionInformation()));
    connect(ui->searchMethodCombo, SIGNAL(currentTextChanged(QString)), this, SLOT(updateSuggestions()) );

    ui->searchEdit->setFocus();
}

Window::~Window()
{
    QSqlDatabase::database().close();
}

void Window::keyReleaseEvent(QKeyEvent *event)
{
    QMainWindow::keyReleaseEvent(event);
    int key = event->key();
    if( key >= Qt::Key_F1 && key <= Qt::Key_F4 )
    {
        key -= Qt::Key_F1; // now F1 is zero, F2 is one, etc.
        if( key < ui->comboBox->count() )
        {
            ui->comboBox->setCurrentIndex(key);
        }
    } else if ( key >= Qt::Key_F10 && key <= Qt::Key_F12 ) {
        key -= Qt::Key_F10;
        if( key < ui->searchMethodCombo->count() )
        {
            ui->searchMethodCombo->setCurrentIndex(key);
        }
    }
}

Window::SearchKeys Window::columnNameForSearching() const
{
    if( ui->comboBox->currentText() == tr("Dari") )
    {
        return Window::Dari;
    }
    else if( ui->comboBox->currentText() == tr("English") )
    {
        return Window::English;
    }
    else if( ui->comboBox->currentText() == tr("IPA") )
    {
        return Window::IPA;
    }
    else
    {
        return Window::Glassman;
    }
}

void Window::searchByChanged()
{
    if( ui->comboBox->currentText() == tr("Dari") )
    {
        ui->searchEdit->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
    }
    else
    {
        ui->searchEdit->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
    }

    updateSuggestions();
}

void Window::updateDefinition(const QModelIndex &index)
{
    QString string = mQueryModel->record( index.row() ).value(0).toString();

    if(string.isEmpty())
    {
        ui->textEdit->setHtml("<html></html>");
        return;
    }

    QString html = "";
    QString list;

    QSqlQuery query;
    switch( columnNameForSearching() )
    {
    case Window::Glassman:
        query.prepare("select distinct glassman,ipa,persian,did from dari where glassman=:string;");
        break;
    case Window::IPA:
        query.prepare("select distinct glassman,ipa,persian,did from dari where ipa=:string;");
        break;
    case Window::Dari:
        query.prepare("select distinct glassman,ipa,persian,did from dari where persian=:string;");
        break;
    case Window::English:
        query.prepare("select distinct glassman,ipa,persian,dari.did from dari,english,lr where english.english=? and english.eid=lr.eid and lr.did=dari.did;");
        break;
    }

    query.bindValue(0, string);
    if( !query.exec() )
    {
        qWarning() << query.lastError() << string << ui->comboBox->currentIndex() << query.executedQuery();
    }

    while(query.next())
    {
        html += "<table style=\"margin-bottom: 10px; font-size: 14pt; font-family: Charis SIL;\">";
        html += QString("<tr><td style=\"font-family: Tahoma,sans-serif; text-align: left; background-color: rgb(235,235,235); padding: 3px; padding-right: 6px; font-weight: bold;\">%1</td><td style=\"padding: 3px;\">%2</td></tr>").arg("IPA").arg(query.value(1).toString());
        html += QString("<tr><td style=\"font-family: Tahoma,sans-serif; text-align: left; background-color: rgb(235,235,235); padding: 3px; padding-right: 6px; font-weight: bold;\">%1</td><td style=\"padding: 3px;\">%2</td></tr>").arg("Glassman").arg(query.value(0).toString());

        list = "";
        QSqlQuery query2;
        query2.prepare("select english from english,lr where lr.eid=english.eid and lr.did=?;");
        query2.bindValue(0, query.value(3).toInt() );
        if( ! query2.exec() )
        {
            qWarning() << query.lastError();
        }
        while(query2.next())
        {
            list += query2.value(0).toString() + ", ";
        }
        list.chop(2);

        html += QString("<tr><td style=\"font-family: Tahoma,sans-serif; text-align: left; background-color: rgb(235,235,235); padding: 3px; padding-right: 6px; font-weight: bold;\">%1</td><td style=\"padding: 3px;\">%2</td></tr>").arg("English").arg(list);
        html += "</table>";
    }

    ui->textEdit->setHtml(html);
}

void Window::updateSuggestions()
{
    QString searchString = ui->searchEdit->text();

    if(searchString.isEmpty())
        return;

    if(!searchString.length()) { return; }

    if( ui->searchMethodCombo->currentText() == tr("Search from beginning") ) {
        searchFromBeginning(searchString);
    } else if ( ui->searchMethodCombo->currentText() == tr("Search for any substring") ) {
        searchAnySubstring(searchString);
    } else if ( ui->searchMethodCombo->currentText() == tr("Search with regular expression") ) {
        searchRegularExpression(searchString);
    }
}

void Window::searchFromBeginning(QString searchString)
{
    QSqlQuery query;

    switch( columnNameForSearching() )
    {
    case Window::Glassman:
        query.prepare( "select distinct glassman from dari where glassman LIKE ?||'%' order by glassman;" );
        break;
    case Window::English:
        query.prepare( "select distinct english from english where english LIKE ?||'%' order by english;" );
        break;
    case Window::IPA:
        query.prepare( "select distinct ipa from dari where replace(ipa,'ɡ','g') LIKE ?||'%' order by ipa;" );
        break;
    case Window::Dari:
        query.prepare( "select distinct dari from dari where dari LIKE ?||'%' order by dari;" );
        break;
    }
    query.bindValue(0, searchString);
    if( query.exec() )
    {
        mQueryModel->setQuery(query);
        ui->listView->setModel(mQueryModel);
    }
    else
    {
        qWarning() << query.lastError();
        return;
    }
}

void Window::searchAnySubstring(QString searchString)
{
    QSqlQuery query;
    QString queryString = "SELECT DISTINCT tablename,1 AS orderingTerm FROM tablename WHERE replace(tablename,'g','ɡ') LIKE ?||'%' "
                          "UNION "
                          "SELECT DISTINCT tablename,2 AS orderingTerm FROM tablename WHERE replace(tablename,'g','ɡ') LIKE '_'||?||'%' "
                          "ORDER BY orderingTerm ASC,tablename;";

    switch( columnNameForSearching() )
    {
    case Window::Glassman:
        query.prepare( "SELECT DISTINCT glassman,1 AS orderingTerm FROM dari WHERE glassman LIKE ?||'%' "
                       "UNION "
                       "SELECT DISTINCT glassman,2 AS orderingTerm FROM dari WHERE glassman LIKE '%_'||?||'%' "
                       "ORDER BY orderingTerm ASC,glassman;" );
        break;
    case Window::English:
        query.prepare( "SELECT DISTINCT english,1 AS orderingTerm FROM english WHERE english LIKE ?||'%' "
                       "UNION "
                       "SELECT DISTINCT english,2 AS orderingTerm FROM english WHERE english LIKE '%_'||?||'%' "
                       "ORDER BY orderingTerm ASC,english;" );
        break;
    case Window::IPA:
        query.prepare( "SELECT DISTINCT ipa,1 AS orderingTerm FROM dari WHERE replace(ipa,'ɡ','g') LIKE ?||'%' "
                       "UNION "
                       "SELECT DISTINCT ipa,2 AS orderingTerm FROM dari WHERE replace(ipa,'ɡ','g') LIKE '%_'||?||'%' "
                       "ORDER BY orderingTerm ASC,ipa;" );
        break;
    case Window::Dari:
        query.prepare( "SELECT DISTINCT dari,1 AS orderingTerm FROM dari WHERE dari LIKE ?||'%' "
                       "UNION "
                       "SELECT DISTINCT dari,2 AS orderingTerm FROM dari WHERE dari LIKE '%_'||?||'%' "
                       "ORDER BY orderingTerm ASC,dari;" );
        break;
    }
    query.bindValue(0, searchString);
    query.bindValue(1, searchString);
    if( query.exec() )
    {
        mQueryModel->setQuery(query);
        ui->listView->setModel(mQueryModel);
    }
    else
    {
        qWarning() << query.lastError();
        return;
    }
}

void Window::searchRegularExpression(QString searchString)
{
    QSqlQuery query;

    switch( columnNameForSearching() )
    {
    case Window::Glassman:
        query.prepare( "select distinct glassman from dari where glassman REGEXP ? order by glassman;" );
        break;
    case Window::English:
        query.prepare( "select distinct english from english where english REGEXP ? order by english;" );
        break;
    case Window::IPA:
        query.prepare( "select distinct ipa from dari where replace(ipa,'ɡ','g') REGEXP ? order by ipa;" );
        break;
    case Window::Dari:
        query.prepare( "select distinct dari from dari where dari REGEXP ? order by dari;" );
        break;
    }
    query.bindValue(0, searchString);
    if( query.exec() )
    {
        mQueryModel->setQuery(query);
        ui->listView->setModel(mQueryModel);
    }
    else
    {
        qWarning() << query.lastError();
        return;
    }
}

void Window::versionInformation()
{
    QSqlQuery query;
    QString htmlText = "";
    query.exec("select version,note from version;");
    while(query.next())
    {
        htmlText += "<b>Version " + query.value(0).toString() + "</b><br/>";
        htmlText += query.value(1).toString() + "<p>";
    }
    QMessageBox::information (this,"Version Information",htmlText);
}
