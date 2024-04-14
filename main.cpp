#include <QApplication>
#include <QWidget>
#include <QCalendarWidget>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QPushButton>
#include <QVBoxLayout>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QTextEdit>
#include <QMessageBox>
#include <QDebug>

class DiaryApp : public QWidget {
    Q_OBJECT // Add Q_OBJECT macro to enable signals and slots
public:
    DiaryApp(QWidget *parent = nullptr) : QWidget(parent) {
        // Initialize database connection
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
        db.setDatabaseName("diary.db");

        if (!db.open()) {
            qDebug() << "Error: Unable to open database";
            return;
        }

        // Create diary table if it doesn't exist
        QSqlQuery query;
        query.exec("CREATE TABLE IF NOT EXISTS diary ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                   "date DATE,"
                   "entry TEXT"
                   ")");

        // Initialize calendar widget
        calendarWidget = new QCalendarWidget(this);

        // Initialize system tray icon
        systemTrayIcon = new QSystemTrayIcon(this);
        systemTrayIcon->setIcon(QIcon(":/icons/icon.png"));
        systemTrayIcon->setVisible(true);

        // Tray menu
        trayMenu = new QMenu(this);
        QAction *showAction = trayMenu->addAction("Show");
        QAction *quitAction = trayMenu->addAction("Quit");
        connect(showAction, &QAction::triggered, this, &DiaryApp::show);
        connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);
        systemTrayIcon->setContextMenu(trayMenu);

        // Input box for diary entry
        entryTextEdit = new QTextEdit(this);

        // Buttons
        addButton = new QPushButton("Add Entry", this);
        updateButton = new QPushButton("Update Entry", this);
        removeButton = new QPushButton("Remove Entry", this);

        // Connect signals and slots
        connect(calendarWidget, &QCalendarWidget::activated, this, &DiaryApp::showEntry);
        connect(systemTrayIcon, &QSystemTrayIcon::activated, this, &DiaryApp::toggleWindow);
        connect(addButton, &QPushButton::clicked, this, &DiaryApp::addEntry);
        connect(updateButton, &QPushButton::clicked, this, &DiaryApp::updateEntry);
        connect(removeButton, &QPushButton::clicked, this, &DiaryApp::confirmRemoveEntry);

        // Layout
        layout = new QVBoxLayout(this);
        layout->addWidget(calendarWidget);
        layout->addWidget(entryTextEdit);
        layout->addWidget(addButton);
        layout->addWidget(updateButton);
        layout->addWidget(removeButton);

        setLayout(layout);

        // Show days with entries as green on the calendar
        highlightDaysWithEntries();
    }

private slots:
    void showEntry(const QDate &date) {
        QString dateString = date.toString("yyyy-MM-dd");
        QSqlQuery query;
        query.prepare("SELECT entry FROM diary WHERE date = :date");
        query.bindValue(":date", dateString);

        if (query.exec() && query.next()) {
            QString entry = query.value(0).toString();
            entryTextEdit->setPlainText(entry); // Display entry in the text edit box
        } else {
            entryTextEdit->clear();
        }
    }

    void toggleWindow(QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::DoubleClick) {
            if (isVisible())
                hide();
            else
                show();
        }
    }

    void addEntry() {
        // Get current selected date
        QDate currentDate = calendarWidget->selectedDate();
        QString dateString = currentDate.toString("yyyy-MM-dd");

        // Get entry text from the text edit box
        QString entryText = entryTextEdit->toPlainText();

        // Add entry to database
        QSqlQuery query;
        query.prepare("INSERT INTO diary (date, entry) VALUES (:date, :entry)");
        query.bindValue(":date", dateString);
        query.bindValue(":entry", entryText);

        if (!query.exec()) {
            qDebug() << "Error inserting diary entry:" << query.lastError().text();
            return;
        }

        qDebug() << "Diary entry added successfully for" << dateString;

        // Update calendar
        highlightDaysWithEntries();
    }

    void updateEntry() {
        // Get current selected date
        QDate currentDate = calendarWidget->selectedDate();
        QString dateString = currentDate.toString("yyyy-MM-dd");

        // Get updated entry text from the text edit box
        QString updatedEntryText = entryTextEdit->toPlainText();

        // Update entry in database
        QSqlQuery query;
        query.prepare("UPDATE diary SET entry = :entry WHERE date = :date");
        query.bindValue(":entry", updatedEntryText);
        query.bindValue(":date", dateString);

        if (!query.exec()) {
            qDebug() << "Error updating diary entry:" << query.lastError().text();
            return;
        }

        qDebug() << "Diary entry updated successfully for" << dateString;

        // Update calendar
        highlightDaysWithEntries();
    }

    void confirmRemoveEntry() {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Confirm", "Are you sure you want to remove this entry?",
                                      QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            removeEntry();
        }
    }

    void removeEntry() {
        // Get current selected date
        QDate currentDate = calendarWidget->selectedDate();
        QString dateString = currentDate.toString("yyyy-MM-dd");

        // Remove entry from database
        QSqlQuery query;
        query.prepare("DELETE FROM diary WHERE date = :date");
        query.bindValue(":date", dateString);

        if (!query.exec()) {
            qDebug() << "Error removing diary entry:" << query.lastError().text();
            return;
        }

        qDebug() << "Diary entry removed successfully for" << dateString;

        // Update calendar
        highlightDaysWithEntries();
    }

private:
    QVBoxLayout *layout;
    QCalendarWidget *calendarWidget;
    QSystemTrayIcon *systemTrayIcon;
    QMenu *trayMenu;
    QTextEdit *entryTextEdit;
    QPushButton *addButton;
    QPushButton *updateButton;
    QPushButton *removeButton;

    void highlightDaysWithEntries() {
        // Reset all date text formats
        QTextCharFormat defaultFormat;
        calendarWidget->setDateTextFormat(QDate(), defaultFormat);

        // Highlight days with entries as green
        QSqlQuery query("SELECT date FROM diary");
        while (query.next()) {
            QString dateString = query.value(0).toString();
            QDate date = QDate::fromString(dateString, "yyyy-MM-dd");
            QTextCharFormat entryFormat;
            entryFormat.setBackground(Qt::green);
            calendarWidget->setDateTextFormat(date, entryFormat);
        }
    }
};

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    DiaryApp w;
    w.show();
    return a.exec();
}

#include "main.moc" // Include the moc file for QObject
