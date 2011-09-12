#include "todo_txt_akonadiresource.h"

#include "settings.h"
#include "settingsadaptor.h"

#include <kfiledialog.h>
#include <klocalizedstring.h>

#include <QtDBus/QDBusConnection>
#include <QtCore/QFileSystemWatcher>

#include <todo.h>
#include <event.h>

#include <akonadi/itemfetchscope.h>
#include <akonadi/changerecorder.h>

using namespace Akonadi;

typedef boost::shared_ptr<KCalCore::Incidence> IncidencePtr;
typedef boost::shared_ptr<KCalCore::Todo> TodoPtr;

todo_txt_akonadiResource::todo_txt_akonadiResource( const QString &id )
  : ResourceBase( id ),
  m_fsWatcher( new QFileSystemWatcher( this ) )
{
  new SettingsAdaptor( Settings::self() );
  QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
						Settings::self(),
						QDBusConnection::ExportAdaptors );
  changeRecorder()->itemFetchScope().fetchFullPayload();
}

todo_txt_akonadiResource::~todo_txt_akonadiResource()
{
}

void todo_txt_akonadiResource::retrieveCollections()
{
  // TODO: this method is called when Akonadi wants to have all the
  // collections your resource provides.
  // Be sure to set the remote ID and the content MIME types

  kWarning() << "retrieve collections";

  Collection c;
  c.setParent( Collection::root() );

  const QString todoFileName = QString(Settings::self()->todoTxtFile());

  kWarning() << "adding file to watch for changes";
  m_fsWatcher->addPath( todoFileName );
  connect( m_fsWatcher,
	   SIGNAL(fileChanged(QString)),
	   SLOT(slotFileChanged(QString)) );

  c.setRemoteId( todoFileName );
  c.setName( name() );

  QStringList mimeTypes;
  mimeTypes << QLatin1String("text/calendar");
  c.setContentMimeTypes( mimeTypes );

  Collection::List list;
  list << c;
  collectionsRetrieved( list );
}

void todo_txt_akonadiResource::slotFileChanged( const QString &file ) {
  Q_UNUSED( file );
  kWarning() << "change in todo.txt file detected.";
  synchronize();
}

void todo_txt_akonadiResource::retrieveItems( const Akonadi::Collection &collection )
{
  Q_UNUSED( collection );

  kWarning() << "retrieve items";

  // TODO: this method is called when Akonadi wants to know about all the
  // items in the given collection. You can but don't have to provide all the
  // data for each item, remote ID and MIME type are enough at this stage.
  // Depending on how your resource accesses the data, there are several
  // different ways to tell Akonadi when you are done.

  // our collections have the mapped path as their remote identifier
  const QString todoFileName = collection.remoteId();

  QFile todoFile(todoFileName);
  if (!todoFile.open(QIODevice::ReadOnly | QIODevice::Text))
    return;

  // read each line in todo.txt
  Item::List items;
  QTextStream in(&todoFile);
  int lineno = 0;
  while (!in.atEnd()) {
    QString line = in.readLine();

    QString remoteId(todoFileName + QLatin1Char( '/' ) + QString::number(lineno));

    KCalCore::Todo::Ptr todo(new KCalCore::Todo);
    todo->setSummary(line);

    Item item("application/x-vnd.akonadi.calendar.todo");
    item.setRemoteId(remoteId);
    item.setPayload<KCalCore::Todo::Ptr>(todo);

    items << item;

    lineno++;
  }

  todoFile.close();

  itemsRetrieved( items );
}

bool todo_txt_akonadiResource::retrieveItem( const Akonadi::Item &item,
					     const QSet<QByteArray> &parts )
{
  Q_UNUSED( parts );

  // TODO: this method is called when Akonadi wants more data for a
  // given item.  You can only provide the parts that have been
  // requested but you are allowed to provide all in one go

  // All data was read in in retrieveItems

  itemRetrieved( item );

  return true;
}

void todo_txt_akonadiResource::aboutToQuit()
{
  // TODO: any cleanup you need to do while there is still an active
  // event loop. The resource will terminate after this method returns
}

void todo_txt_akonadiResource::configure( WId windowId )
{
  Q_UNUSED( windowId );

  // TODO: this method is usually called when a new resource is being
  // added to the Akonadi setup. You can do any kind of user interaction here,
  // e.g. showing dialogs.
  // The given window ID is usually useful to get the correct
  // "on top of parent" behavior if the running window manager applies any kind
  // of focus stealing prevention technique
  //
  // If the configuration dialog has been accepted by the user by clicking Ok,
  // the signal configurationDialogAccepted() has to be emitted, otherwise, if
  // the user canceled the dialog, configurationDialogRejected() has to be emitted.

  const QString oldTodoTxtFile = Settings::self()->todoTxtFile();
  KUrl url;
  if ( !oldTodoTxtFile.isEmpty() )
    url = KUrl::fromPath( oldTodoTxtFile );
  else
    url = KUrl::fromPath( QDir::homePath() + "/todo.txt" );

  const QString title = i18nc( "@title:window", "Select TODO.txt file." );
  const QString filter = QString("");

  const QString newTodoTxtFile = KFileDialog::getOpenFileName( url, filter, 0, title );

  if ( newTodoTxtFile.isEmpty() )
    return;

  if ( oldTodoTxtFile == newTodoTxtFile )
    return;

  Settings::self()->setTodoTxtFile( newTodoTxtFile );

  Settings::self()->writeConfig();

  synchronize();

}

void todo_txt_akonadiResource::itemAdded( const Akonadi::Item &item,
					  const Akonadi::Collection &collection )
{
  Q_UNUSED( collection );

  kWarning() << "itemAdded() item id="  << item.id()
	     << ", remoteId=" << item.remoteId()
	     << ", mimeType=" << item.mimeType();

  // TODO: this method is called when somebody else, e.g. a client application,
  // has created an item in a collection managed by your resource.

  // NOTE: There is an equivalent method for collections, but it isn't part
  // of this template code to keep it simple

  KCalCore::Todo::Ptr todo;
  IncidencePtr ptrEvent;

  kWarning() << "hasPayload()" << item.hasPayload();

  if (item.hasPayload<KCalCore::Todo::Ptr>()) {
    kWarning() << "todo::Ptr ";// << todo->summary();
    todo = item.payload<KCalCore::Todo::Ptr>();

    if (todo) {
      kWarning() << "summary: " << todo->summary();

      const QString todoFileName = collection.remoteId();
      QFile todoFile(todoFileName);

      if (!todoFile.open(QIODevice::ReadWrite | QIODevice::Text)) {
	// TODO: Indicate fail.
	return;
      }

      QTextStream stream(&todoFile);
      int lineno = 0;
      while (!stream.atEnd()) {
	QString line = stream.readLine();
	Q_UNUSED( line );
	lineno++;
      }
      stream << todo->summary() << "\n";
      todoFile.close();

      QString remoteId(todoFileName + QLatin1Char( '/' ) + QString::number(lineno));

      Item newItem( item );
      newItem.setRemoteId( remoteId );
      newItem.setPayload<KCalCore::Todo::Ptr>( todo );

      changeCommitted( newItem );
    }

  } else {
    kError() << "Add without TODO payload!";
    const QString message = i18nc("@info:status",
				  "No TODO payload to add event.");
    emit error(message);
    emit status(Broken, message);
  }
  kWarning() << "itemAdded~";
}

void todo_txt_akonadiResource::itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  Q_UNUSED( item );
  Q_UNUSED( parts );

  kWarning() << "itemChanged() item id="  << item.id();

  // TODO: this method is called when somebody else, e.g. a client application,
  // has changed an item managed by your resource.

  // NOTE: There is an equivalent method for collections, but it isn't part
  // of this template code to keep it simple
}

void todo_txt_akonadiResource::itemRemoved( const Akonadi::Item &item )
{
  Q_UNUSED( item );

  kWarning() << "itemRemoved() item id="  << item.id()
	     << "remoteId=" << item.remoteId();

  int lastslash = item.remoteId().lastIndexOf("/");
  kWarning() << "slash is at: " << lastslash;
  if (lastslash < 0) {
    kError() << "remoteId malformed: " << item.remoteId();
    const QString message = i18nc("@info:status",
				  "RemoteId is malformed (Did not find /).");
    emit error(message);
    emit status(Broken, message);
    return;
  }

  const QString todoFileName = item.remoteId().left(lastslash);
  const QString linenotodelete = item.remoteId().right(item.remoteId().size() - lastslash - 1);

  kWarning() << "todoFileName: " << todoFileName;
  kWarning() << "lineNo: " << linenotodelete;

  bool conv_ok = false;
  const int linenotodelete_int = linenotodelete.toInt(&conv_ok);
  if (conv_ok == false) {
    kError() << "remoteId line number not an integer: " << linenotodelete;
    const QString message = i18nc("@info:status",
				  "RemoteId is malformed (Line number is not an integer).");
    emit error(message);
    emit status(Broken, message);
    return;
  }

  QFile todoFile(todoFileName);
  QFile todoTmpFile(todoFileName+".tmp");

  if (!todoFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    kError() << "Could not open TODO file.";
    const QString message = i18nc("@info:status",
				  "Could not open TODO file.");
    emit error(message);
    emit status(Broken, message);
    return;
  }

  QTextStream stream(&todoFile);
  QTextStream tmpstream(&todoTmpFile);

  kWarning() << "Trying to remove line " << linenotodelete_int;

  int lineno = 0;
  while (!stream.atEnd()) {
    QString line = stream.readLine();
    if (lineno == linenotodelete_int) {
      kWarning() << "Not copying line " << lineno
		 << " to tmp file: \n" << line;
      lineno++;
      continue;
    }
    kWarning() << "Copying line " << lineno
	       << " to tmp file: \n" << line;
    tmpstream << line;
    lineno++;
  }

  kWarning() << "Removing file " << todoFileName;
  todoFile.remove();
  kWarning() << "Renaming file " << todoFileName << ".tmp to "<< todoFileName;
  todoTmpFile.rename(todoFileName);

  kWarning() << "Done removing";
  changeCommitted( item );
  kWarning() << "itemRemoved~";
}

AKONADI_RESOURCE_MAIN( todo_txt_akonadiResource )

#include "todo_txt_akonadiresource.moc"
