#ifndef TODO_TXT_AKONADIRESOURCE_H
#define TODO_TXT_AKONADIRESOURCE_H

#include <akonadi/resourcebase.h>

#include <kdirwatch.h>

class QFileSystemWatcher;

class todo_txt_akonadiResource : public Akonadi::ResourceBase,
  public Akonadi::AgentBase::Observer
{
  Q_OBJECT

  public:
    todo_txt_akonadiResource( const QString &id );
    ~todo_txt_akonadiResource();

    //KDirWatch dirwatch;

    //public slots:
    //void todoFileChanged( const QString &path );

  public Q_SLOTS:
    virtual void configure( WId windowId );

  protected Q_SLOTS:
    void retrieveCollections();
    void retrieveItems( const Akonadi::Collection &col );
    bool retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts );
    void slotFileChanged( const QString &file );

  protected:
    virtual void aboutToQuit();

    virtual void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    virtual void itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts );
    virtual void itemRemoved( const Akonadi::Item &item );

    QFileSystemWatcher *m_fsWatcher;
};

#endif
