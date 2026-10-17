#include <stdarg.h>

#include <QDir>
#include <QDateTime>
#include <QStandardPaths>
#include <QMessageBox>
#include <QFile>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFontDatabase>
#include <QPlainTextEdit>
#include <QVBoxLayout>

#include "log.h"

Log * Log::mLog;

void Log::fatal( const QString& str )
{
    Log * self = g();

    // Add the log line
    self->add( str );

    // Write the whole log buffer into a file log.txt
    QFile file( QStandardPaths::writableLocation( QStandardPaths::TempLocation ) + QDir::separator() + "birdtray-lite-log.txt" );

    // Show the dialog
    QMessageBox::critical(nullptr, QCoreApplication::translate("Log", "Fatal"),
            QCoreApplication::translate(
                    "Log", "Fatal error: %1\n\nLog file is written into file %2")
                    .arg(str).arg(file.fileName()));

    self->mMutex.lock();

    if ( file.open(QFile::WriteOnly) )
    {
        QByteArray writedata = self->mEntries.join("\r\n").toUtf8();
        file.write( writedata );
        file.close();
    }

    // We're done, do not unlock self->mMutex
    exit( 1 );
}

void Log::debug(const char *fmt, ... )
{
    va_list vl;
    char buf[4096];

    va_start( vl, fmt );
    vsnprintf( buf, sizeof(buf) - 1, fmt, vl );
    va_end( vl );

    // Add the log line
    g()->add( buf );
}

void Log::showLogger()
{
    Log * self = g();

    QMutexLocker l( &self->mMutex );

    // Create the window if it does not exist. It is deleted when closed
    if ( self->mViewer.isNull() )
    {
        auto* dialog = new QDialog();
        dialog->setAttribute( Qt::WA_DeleteOnClose );
        dialog->setWindowTitle( QCoreApplication::translate( "Log", "Debug log viewer" ) );
        dialog->resize( 500, 380 );

        self->mViewer = new QPlainTextEdit( dialog );
        self->mViewer->setReadOnly( true );
        self->mViewer->setFont( QFontDatabase::systemFont( QFontDatabase::FixedFont ) );
        self->mViewer->appendPlainText( self->mEntries.join( "\r\n" ) );

        auto* buttons = new QDialogButtonBox( QDialogButtonBox::Close, dialog );
        QObject::connect( buttons, &QDialogButtonBox::rejected, dialog, &QDialog::reject );

        auto* layout = new QVBoxLayout( dialog );
        layout->addWidget( self->mViewer );
        layout->addWidget( buttons );
        dialog->show();
    }

    self->mViewer->ensureCursorVisible();
    self->mViewer->window()->activateWindow();
}

Log *Log::g()
{
    if ( mLog == nullptr )
        mLog = new Log();

    return mLog;
}

void Log::add(const QString &text)
{
    // Prepend a timestamp
    QDateTime now = QDateTime::currentDateTime();
    QString logline = QString("%1 %2 ") .arg( now.toString( "yyyy-MM-dd hh:mm:ss" )) .arg(text);

    QMutexLocker l( &mMutex );

    // Adding it to the log buffer
    if ( mEntries.count() >= MAX_LOG_LINES )
        mEntries.pop_front();

    mEntries.push_back( logline );

    // Add it to the open log window, if any
    if ( !mViewer.isNull() )
    {
        mViewer->appendPlainText( logline );
        mViewer->ensureCursorVisible();
    }
}
