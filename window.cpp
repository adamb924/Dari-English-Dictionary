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

    mDb.setDatabaseName( dbPath );
    if(!mDb.open())
    {
        QMessageBox::critical (this, tr("Error Message"), tr("There was a problem in opening the database. The database said: %1. It is unlikely that you will solve this on your own. Rather you had better contact the developer.").arg(mDb.lastError().databaseText()) );
        mUnrecoverableError=true;
        return;
    }

    ui->numberEdit->setValidator(new QIntValidator(1, 100000, this));
    ui->numberEdit->setText( tr("%1").arg(25) );

    connect(ui->comboBox, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(searchByChanged()));
    connect(ui->listWidget, SIGNAL(currentTextChanged(const QString &)), this, SLOT(updateDefinition(QString)));
    connect(ui->searchEdit, SIGNAL(textEdited(const QString &)), this, SLOT(updateSuggestions()));
    connect(ui->substringCheckBox, SIGNAL(stateChanged(int)), this, SLOT(updateSuggestions()));
    connect(ui->numberEdit, SIGNAL(textEdited(const QString &)), this, SLOT(updateSuggestions()));
    connect(ui->versionButton, SIGNAL(clicked(bool)), this, SLOT(versionInformation()));
    connect(ui->substringCheckBox, SIGNAL(clicked(bool)), ui->numberEdit, SLOT(setEnabled(bool)));

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
    if( key >= Qt::Key_F1 && key <= Qt::Key_F35 )
    {
        key -= Qt::Key_F1; // now F1 is zero, F2 is one, etc.
        if( key < ui->comboBox->count() )
        {
            ui->comboBox->setCurrentIndex(key);
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

    QString str = ui->searchEdit->text();

    if(str.isEmpty())
        return;

    if(!str.length()) { return; }

    QSqlQuery query;
    int maxRows = ui->numberEdit->text().toInt();

    switch( columnNameForSearching() )
    {
    case Window::Glassman:
        query.exec( QString("select glassman from dari where substr(glassman,1,%1)='%2' order by glassman limit %3;").arg(str.length()).arg(str).arg(maxRows));
        break;
    case Window::English:
        query.exec( QString("select english from english where substr(english,1,%1)='%2' order by english limit %3;").arg(str.length()).arg(str).arg(maxRows));
        break;
    case Window::IPA:
        query.exec( QString("select ipa from dari where substr(ipa,1,%1)='%2' order by ipa limit %3;").arg(str.length()).arg(str).arg(maxRows));
        break;
    case Window::Dari:
        query.exec( QString("select persian from dic where substr(persian,1,%1)='%2' order by persian limit %3;").arg(str.length()).arg(str).arg(maxRows));
        break;
    }
    if( ! query.exec() )
    {
        qWarning() << query.lastError();
        return;
    }

    int remainingRows = maxRows;
    while(query.next())
    {
        ui->listWidget->addItem(query.value(0).toString());
        remainingRows--;
    }

    if(ui->substringCheckBox->isChecked() && str.length()>1 && remainingRows > 0 )
    {
        switch( columnNameForSearching() )
        {
        case Window::Glassman:
            query.prepare( QString("select distinct glassman from dari where glassman like '%_%1%' order by glassman limit %2;").arg(str).arg(remainingRows) );
            break;
        case Window::English:
            query.prepare( QString("select distinct english from english where english like '%_%1%' order by english limit %2;").arg(str).arg(remainingRows) );
            break;
        case Window::IPA:
            query.prepare( QString("select distinct ipa from dari where ipa like '%_%1%' order by ipa limit %2;").arg(str).arg(remainingRows) );
            break;
        case Window::Dari:
            query.prepare( QString("select distinct dari from dari where dari like '%_%1%' order by dari limit %2;").arg(str).arg(remainingRows) );
            break;
        }
        if( ! query.exec() )
        {
            qWarning() << query.lastError();
        }
        while(query.next())
        {
            ui->listWidget->addItem(query.value(0).toString());
        }
    }

    ui->listWidget->verticalScrollBar()->setSliderPosition(0);
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

