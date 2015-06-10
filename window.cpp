#include <QtWidgets>
#include <QtSql>

#include "window.h"

Window::Window()
{
    bUnrecoverableError=false;

    if(!QSqlDatabase::isDriverAvailable("QSQLITE"))
    {
        QMessageBox::critical (0,"Fatal error", "The driver for the database is not available. It is unlikely that you will solve this on your own. Rather you had better contact the developer.");
        bUnrecoverableError=true;
        return;
    }
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("dedic.db");
    if(!db.open())
    {
        QMessageBox::information (this,"Error Message","There was a problem in opening the database. The program said: " + db.lastError().databaseText() + " It is unlikely that you will solve this on your own. Rather you had better contact the developer." );
        bUnrecoverableError=true;
        return;
    }

    resize(750, 420);

    QWidget *widget = new QWidget;
    setCentralWidget(widget);

    QHBoxLayout *horizontalLayout = new QHBoxLayout;

    QVBoxLayout *SearchPane = new QVBoxLayout;

    SearchPane->setObjectName(QString::fromUtf8("SearchPane"));
    searchTextEntry = new QLineEdit();
    searchTextEntry->setObjectName(QString::fromUtf8("entry"));
    SearchPane->addWidget(searchTextEntry);

    SearchPane->addWidget(new QLabel(tr("Search by...")));

    searchBy = new QComboBox();
    searchBy->setObjectName(QString::fromUtf8("searchBy"));
    searchBy->addItem(tr("Glassman"));
    searchBy->addItem(tr("English"));
    //	searchBy->addItem(tr("IPA"));
    //	searchBy->addItem(tr("Dari"));
    SearchPane->addWidget(searchBy);

    suggestions = new QListWidget();
    suggestions->setObjectName(QString::fromUtf8("suggestions"));
    SearchPane->addWidget(suggestions);

    substringSearch = new QCheckBox(tr("Search for any substring"));
    SearchPane->addWidget(substringSearch);

    QValidator *validator = new QIntValidator(1, 100000, this);

    QHBoxLayout *numberLayout = new QHBoxLayout;
    numberToReturn = new QLineEdit("25");
    numberToReturn->setObjectName(QString::fromUtf8("numbertoreturn"));
    numberToReturn->setMaximumWidth(30);
    numberToReturn->setValidator(validator);
    numberLayout->addWidget(new QLabel(tr("Returning up to")));
    numberLayout->addWidget(numberToReturn);
    numberLayout->addWidget(new QLabel(tr("results")));
    numberLayout->addStretch(99999);
    SearchPane->addLayout(numberLayout);

    QPushButton *versionInfo = new QPushButton("Version Information", this);
    SearchPane->addWidget(versionInfo);
    wordDisplay = new QTextEdit(this);
    wordDisplay->setObjectName(QString::fromUtf8("wordDisplay"));
    wordDisplay->setReadOnly(true);
    wordDisplay->setFontPointSize(20);
    wordDisplay->setHtml("<style>font-color: red;</style><h1>Welcome</h1><p>To begin, start typing a word in the box in the upper left, and then select the desired dictionary entry from the one below it. Search either by the English word, or by the Glassman transcription.");

    horizontalLayout->addLayout(SearchPane,1);
    horizontalLayout->addWidget(wordDisplay,3);

    widget->setLayout(horizontalLayout);

    connect(searchBy, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(searchByChanged(const QString &)));
    connect(suggestions, SIGNAL(currentTextChanged(const QString &)), this, SLOT(updateDefinition(QString)));
    connect(searchTextEntry, SIGNAL(textEdited(const QString &)), this, SLOT(updateSuggestions()));
    connect(substringSearch, SIGNAL(stateChanged(int)), this, SLOT(updateSuggestions()));
    connect(numberToReturn, SIGNAL(textEdited(const QString &)), this, SLOT(updateSuggestions()));
    connect(versionInfo, SIGNAL(clicked(bool)), this, SLOT(versionInformation()));
    connect(substringSearch, SIGNAL(clicked(bool)), numberToReturn, SLOT(setEnabled(bool)));

    substringSearch->setChecked(false);
    setWindowTitle(tr("Dari-English Dictionary"));

    searchTextEntry->setFocus();
}

Window::~Window()
{
    db.close();
}

void Window::searchByChanged(const QString &str)
{
    switch(searchBy->currentIndex())
    {
    case GLASSMAN:
        searchTextEntry->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
        break;
    case ENGLISH:
        searchTextEntry->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
        break;
    case IPA:
        searchTextEntry->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
        break;
    case DARI:
        searchTextEntry->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
        break;
    }

    updateSuggestions();
}

void Window::updateDefinition(const QString &str)
{
    if(str.isEmpty())
    {
        wordDisplay->setHtml("<html></html>");
        return;
    }

    QSqlQuery query(db);
    QSqlQuery query2(db);

    QString html = "";
    QString list;

    QString searchString = str;
    searchString.replace(QString("'"),QString("''"));

    switch(searchBy->currentIndex())
    {
    case GLASSMAN:
        query.exec("select glassman,ipa,persian,persian_spoken,did from dari where glassman='" + searchString  + "';");
        break;
    case IPA:
        query.exec("select glassman,ipa,persian,persian_spoken,did from dari where ipa'" + searchString  + "';");
        break;
    case DARI:
        query.exec("select glassman,ipa,persian,persian_spoken,did from dari where persian'" + searchString  + "';");
        break;
    case ENGLISH:
        query.exec("select glassman,ipa,persian,persian_spoken,dari.did from dari,english,lr where english.english='" + searchString  + "' and english.eid=lr.eid and lr.did=dari.did;");
        break;
    }

    while(query.next())
    {
        html += "<table style=\"margin-bottom: 10px; font-size: 14pt; font-family: Charis SIL;\">";
        html += QString("<tr><td style=\"font-family: Tahoma,sans-serif; text-align: left; background-color: rgb(235,235,235); padding: 3px; padding-right: 6px; font-weight: bold;\">%1</td><td style=\"padding: 3px;\">%2</td></tr>").arg("Glassman").arg(query.value(0).toString());

        list = "";
        query2.exec("select english from english,lr where lr.eid=english.eid and lr.did="+query.value(4).toString()+";");
        while(query2.next())
        {
            list += query2.value(0).toString() + ", ";
        }
        list.chop(2);

        html += QString("<tr><td style=\"font-family: Tahoma,sans-serif; text-align: left; background-color: rgb(235,235,235); padding: 3px; padding-right: 6px; font-weight: bold;\">%1</td><td style=\"padding: 3px;\">%2</td></tr>").arg("English").arg(list);
        html += "</table>";
    }

    wordDisplay->setHtml(html);
}

void Window::updateSuggestions()
{
    suggestions->clear();

    QString str = searchTextEntry->text();

    if(str.isEmpty())
        return;

    QString maxcount;
    maxcount = numberToReturn->text();

    if(!str.length()) { return; }

    QString searchString = str;
    searchString.replace(QString("'"),QString("''"));

    QSqlQuery query(db);

    switch(searchBy->currentIndex())
    {
    case GLASSMAN:
        query.exec("select glassman from dari where substr(glassman,1," + QString::number(str.length()) + ")='" + searchString  + "' order by glassman;");
        while(query.next())
        {
            suggestions->addItem(query.value(0).toString());
        }
        break;
	case ENGLISH:
        query.exec("select english from english where substr(english,1," + QString::number(str.length()) + ")='" + searchString  + "' order by english;");
        while(query.next())
        {
            suggestions->addItem(query.value(0).toString());
        }
        break;
	case IPA:
        query.exec("select ipa from dari where substr(ipa,1," + QString::number(str.length()) + ")='" + searchString  + "' order by ipa;");
        while(query.next())
        {
            suggestions->addItem(query.value(0).toString());
        }
        break;
	case DARI:
        query.exec("select persian from dic where substr(persian,1," + QString::number(str.length()) + ")='" + searchString  + "' order by persian;");
        while(query.next())
        {
            suggestions->addItem(query.value(0).toString());
        }
        break;
    }

    if(substringSearch->isChecked() && str.length()>1)
    {
        switch(searchBy->currentIndex())
        {
        case GLASSMAN:
            query.exec("select distinct glassman from dari where glassman like '%_" + searchString  + "%' order by glassman limit " + maxcount + " ;");

            while(query.next())
            {
                suggestions->addItem(query.value(0).toString());
            }
            break;
		case ENGLISH:
            query.exec("select distinct english from english where english like '%_" + searchString  + "%' order by english;");
            while(query.next())
            {
                suggestions->addItem(query.value(0).toString());
            }
            break;
		case IPA:
            query.exec("select distinct ipa from dari where ipa like '%_" + searchString  + "%' order by ipa;");
            while(query.next())
            {
                suggestions->addItem(query.value(0).toString());
            }
            break;
		case DARI:
            query.exec("select distinct dari from dari where dari like '%_" + searchString  + "%' order by dari;");
            while(query.next())
            {
                suggestions->addItem(query.value(0).toString());
            }
            break;
        }
    }

    suggestions->verticalScrollBar()->setSliderPosition(0);
}

void Window::versionInformation()
{
    QSqlQuery query(db);
    QString htmlText = "";
    query.exec("select version,note from version;");
    while(query.next())
    {
        htmlText += "<b>Version " + query.value(0).toString() + "</b><br/>";
        htmlText += query.value(1).toString() + "<p>";
    }
    QMessageBox::information (this,"Version Information",htmlText);
}
