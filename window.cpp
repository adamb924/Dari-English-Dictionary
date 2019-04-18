#include <QtWidgets>
#include <QtSql>
#include <QtDebug>

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
        QMessageBox::critical (0,"Fatal error", "The driver for the database is not available. It is unlikely that you will solve this on your own. Rather you had better contact the developer.");
        mUnrecoverableError=true;
        return;
    }

    mDb = QSqlDatabase::addDatabase("QSQLITE");

    QDir appFolder( QCoreApplication::applicationDirPath() );
    QString dbPath = appFolder.absoluteFilePath("dedic2.db");
    if( !appFolder.exists("dedic2.db") )
    {
        QMessageBox::critical (this,tr("Error Message"),tr("The database file does not exist (%1)").arg(dbPath) );
        mUnrecoverableError=true;
        return;
    }

    mDb.setConnectOptions("QSQLITE_ENABLE_REGEXP");

    mDb.setDatabaseName( dbPath );
    if(!mDb.open())
    {
        QMessageBox::critical (this, tr("Error Message"), tr("There was a problem in opening the database. The database said: %1. It is unlikely that you will solve this on your own. Rather you had better contact the developer.").arg(mDb.lastError().databaseText()) );
        mUnrecoverableError=true;
        return;
    }

    ui->numberEdit->setValidator(new QIntValidator(1, 100000, this));
    ui->numberEdit->setText( tr("%1").arg(250) );

    connect(ui->comboBox, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(searchByChanged()));
    connect(ui->listWidget, SIGNAL(currentTextChanged(const QString &)), this, SLOT(updateDefinition(QString)));
    connect(ui->searchEdit, SIGNAL(textEdited(const QString &)), this, SLOT(updateSuggestions()));
    connect(ui->numberEdit, SIGNAL(textEdited(const QString &)), this, SLOT(updateSuggestions()));
    connect(ui->versionButton, SIGNAL(clicked(bool)), this, SLOT(versionInformation()));
    connect(ui->searchMethodCombo, SIGNAL(currentTextChanged(QString)), this, SLOT(updateSuggestions()) );

    ui->searchEdit->setFocus();
}

Window::~Window()
{
    mDb.close();
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

void Window::updateDefinition(const QString &str)
{
    if(str.isEmpty())
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

    query.bindValue(0, str);
    if( !query.exec() )
    {
        qWarning() << query.lastError() << str << ui->comboBox->currentIndex() << query.executedQuery();
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
    ui->listWidget->clear();

    QString searchString = ui->searchEdit->text();

    if(searchString.isEmpty())
        return;

    if(!searchString.length()) { return; }

    if( ui->searchMethodCombo->currentText() == tr("Search from beginning") ) {
        searchFromBeginning(searchString);
    } else if ( ui->searchMethodCombo->currentText() == tr("Search for any substring") ) {
        searchFromBeginning(searchString);
        searchAnySubstring(searchString);
    } else if ( ui->searchMethodCombo->currentText() == tr("Search with regular expression") ) {
        searchRegularExpression(searchString);
    }

    ui->listWidget->verticalScrollBar()->setSliderPosition(0);
}

void Window::searchFromBeginning(QString searchString)
{
    QSqlQuery query;
    int maxRows = ui->numberEdit->text().toInt();

    switch( columnNameForSearching() )
    {
    case Window::Glassman:
        query.exec( QString("select glassman from dari where substr(replace(glassman,'g','ɡ'),1,%1)=replace('%2','g','ɡ') order by glassman limit %3;").arg(searchString.length()).arg(searchString).arg(maxRows));
        break;
    case Window::English:
        query.exec( QString("select english from english where substr(replace(english,'g','ɡ'),1,%1)=replace('%2','g','ɡ') order by english limit %3;").arg(searchString.length()).arg(searchString).arg(maxRows));
        break;
    case Window::IPA:
        query.exec( QString("select ipa from dari where substr(replace(ipa,'g','ɡ'),1,%1)=replace('%2','g','ɡ') order by ipa limit %3;").arg(searchString.length()).arg(searchString).arg(maxRows));
        break;
    case Window::Dari:
        query.exec( QString("select persian from dic where substr(replace(persian,'g','ɡ'),1,%1)=replace('%2','g','ɡ') order by persian limit %3;").arg(searchString.length()).arg(searchString).arg(maxRows));
        break;
    }
    if( ! query.exec() )
    {
        qWarning() << query.lastError();
        return;
    }

    while(query.next())
    {
        ui->listWidget->addItem(query.value(0).toString());
    }
}

void Window::searchAnySubstring(QString searchString)
{
    QSqlQuery query;
    int remainingRows = ui->numberEdit->text().toInt() - ui->listWidget->count();

//    searchString.replace("g","ɡ"); // 0x0067 v. 0x0261


    if( searchString.length()>1 && remainingRows > 0 )
    {
        switch( columnNameForSearching() )
        {
        case Window::Glassman:
            query.prepare( QString("select distinct glassman from dari where replace(glassman,'g','ɡ') like replace('%_%1%','g','ɡ') order by glassman limit %2;").arg(searchString).arg(remainingRows) );
            break;
        case Window::English:
            query.prepare( QString("select distinct english from english where replace(english,'g','ɡ') like replace('%_%1%','g','ɡ') order by english limit %2;").arg(searchString).arg(remainingRows) );
            break;
        case Window::IPA:
            query.prepare( QString("select distinct ipa from dari where replace(ipa,'g','ɡ') like replace('%_%1%','g','ɡ') order by ipa limit %2;").arg(searchString).arg(remainingRows) );
            break;
        case Window::Dari:
            query.prepare( QString("select distinct dari from dari where replace(dari,'g','ɡ') like replace('%_%1%','g','ɡ') order by dari limit %2;").arg(searchString).arg(remainingRows) );
            break;
        }
        if( ! query.exec() )
        {
            qWarning() << query.lastError();
        }
        while(query.next() )
        {
            ui->listWidget->addItem(query.value(0).toString());
        }
    }
}

void Window::searchRegularExpression(QString searchString)
{
    QSqlQuery query;
    int maxRows = ui->numberEdit->text().toInt();

    switch( columnNameForSearching() )
    {
    case Window::Glassman:
        query.prepare( QString("select distinct glassman from dari where replace(glassman,'g','ɡ') REGEXP '%1' order by glassman limit %2;").arg(searchString).arg(maxRows) );
        break;
    case Window::English:
        query.prepare( QString("select distinct english from english where replace(english,'g','ɡ') REGEXP '%1' order by english limit %2;").arg(searchString).arg(maxRows) );
        break;
    case Window::IPA:
        query.prepare( QString("select distinct ipa from dari where replace(ipa,'g','ɡ') REGEXP '%1' order by ipa limit %2;").arg(searchString).arg(maxRows) );
        break;
    case Window::Dari:
        query.prepare( QString("select distinct dari from dari where replace(dari,'g','ɡ') REGEXP '%1' order by dari limit %2;").arg(searchString).arg(maxRows) );
        break;
    }
    if( ! query.exec() )
    {
        qWarning() << query.lastError();
    }
    while(query.next()  )
    {
        ui->listWidget->addItem(query.value(0).toString());
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
