/**
 *
 **/

#include "SessionDispatcher.h"
#include "CartaLib/LinearMap.h"
#include "core/MyQApp.h"
#include "core/SimpleRemoteVGView.h"
#include <iostream>
#include <QImage>
#include <QPainter>
#include <QXmlInputSource>
#include <cmath>
#include <QTime>
#include <QTimer>
#include <QCoreApplication>
#include <functional>

#include <QStringList>

#include <thread>
// #include "websocketclientwrapper.h"
// #include "websockettransport.h"
// #include "qwebchannel.h"
#include <QBuffer>
#include <QThread>

#include "NewServerConnector.h"

void SessionDispatcher::startWebSocketChannel(){

    int port = 4317;

    // setup the QWebSocketServer
    m_pWebSocketServer = new QWebSocketServer(QStringLiteral("QWebChannel Standalone Example Server"), QWebSocketServer::NonSecureMode, this);
    if (!m_pWebSocketServer->listen(QHostAddress::Any, port)) {
        qFatal("Failed to open web socket server.");
        return;
    }

    qDebug() << "SessionDispatcher listening on port" << port;

    connect(m_pWebSocketServer, &QWebSocketServer::newConnection,
            this, &SessionDispatcher::onNewConnection);

    // // wrap WebSocket clients in QWebChannelAbstractTransport objects
    // m_clientWrapper = new WebSocketClientWrapper(m_pWebSocketServer);

    // // setup the channel
    // m_channel = new QWebChannel();
    // QObject::connect(m_clientWrapper, &WebSocketClientWrapper::clientConnected,
    //                  m_channel, &QWebChannel::connectTo);

    // m_channel->registerObject(QStringLiteral("QConnector"), this);
}

SessionDispatcher::SessionDispatcher()
{
    //Qt's built-in WebSocket + QWebChannel
    m_pWebSocketServer = nullptr;
    // m_clientWrapper = nullptr;
    // m_channel = nullptr;
    startWebSocketChannel();

}

SessionDispatcher::~SessionDispatcher()
{
    m_pWebSocketServer->close();

    if (m_pWebSocketServer != nullptr) {
        delete m_pWebSocketServer;
    }

    // if (m_clientWrapper != nullptr) {
    //     delete m_clientWrapper;
    // }

    // if (m_channel != nullptr) {
    //     delete m_channel;
    // }
}

void SessionDispatcher::onNewConnection()
{
    qDebug() << "new Client Session !!!!";

    QWebSocket* socket = m_pWebSocketServer->nextPendingConnection();
    QString sessionID = QString::number(std::rand());
    NewServerConnector *connector =  new NewServerConnector();

    sessionList[socket] = connector;
    setConnectorInMap(sessionID, connector);

    connect(connector, SIGNAL(startViewerSignal(const QString &)), connector, SLOT(startViewerSlot(const QString &)));
    connect(connector, SIGNAL(onTextMessageSignal(QString)), connector, SLOT(onTextMessage(QString)));
    connect(connector, SIGNAL(onBinaryMessageSignal(QByteArray)), connector, SLOT(onBinaryMessage(QByteArray)));

    connect(socket, &QWebSocket::textMessageReceived, this, &SessionDispatcher::onTextMessage);
    connect(socket, &QWebSocket::binaryMessageReceived, this, &SessionDispatcher::onBinaryMessage);

    connect(connector, SIGNAL(jsTextMessageResultSignal(QString)), this, SLOT(forwardTextMessageResult(QString)) );
    connect(connector, SIGNAL(jsBinaryMessageResultSignal(QByteArray)), this, SLOT(forwardBinaryMessageResult(QByteArray)) );

    // create a simple thread
    QThread* newThread = new QThread();

    // let the new thread handle its events
    connector->moveToThread(newThread);
    newThread->setObjectName(sessionID);

    // start new thread's event loop
    newThread->start();

    //trigger signal
    emit connector->startViewerSignal(sessionID);
}

void SessionDispatcher::onTextMessage(QString message){
    QWebSocket* socket = qobject_cast<QWebSocket*>(sender());
    // QString sessionID = sessionList[socket];
    // NewServerConnector *connector = static_cast<NewServerConnector*>(getConnectorInMap(sessionID));
    NewServerConnector* connector= sessionList[socket];
    if (connector != nullptr){
        emit connector->onTextMessageSignal(message);
    }
}

void SessionDispatcher::onBinaryMessage(QByteArray message){

}

void SessionDispatcher::forwardTextMessageResult(QString result){
    QWebSocket* socket = nullptr;
    NewServerConnector* connector = qobject_cast<NewServerConnector*>(sender());
    std::map<QWebSocket*, NewServerConnector*>::iterator iter;
    for (iter = sessionList.begin(); iter != sessionList.end(); ++iter){
        if (iter->second == connector){
            socket = iter->first;
            break;
        }
    }
    if (socket){
        socket->sendTextMessage(result);
    }
    else {
        qDebug() << "ERROR! Cannot find the corresponding websocket!";
    }
}

void SessionDispatcher::forwardBinaryMessageResult(QByteArray result){
    QWebSocket* socket = nullptr;
    NewServerConnector* connector = qobject_cast<NewServerConnector*>(sender());
    std::map<QWebSocket*, NewServerConnector*>::iterator iter;
    for (iter = sessionList.begin(); iter != sessionList.end(); ++iter){
        if (iter->second == connector){
            socket = iter->first;
            break;
        }
    }
    if (socket){
        socket->sendBinaryMessage(result);
    }
    else {
        qDebug() << "ERROR! Cannot find the corresponding websocket!";
    }
}

// void SessionDispatcher::jsSendCommandSlot(const QString & sessionID, const QString & senderSession, const QString &cmd, const QString & parameter)
// {
//     // forward commands
//     NewServerConnector *connector = static_cast<NewServerConnector*>(getConnectorInMap(sessionID));
//     if (connector != nullptr){
//         emit connector->jsSendCommandSignal(sessionID, senderSession, cmd, parameter);
//     }
//     return;
// }

IConnector* SessionDispatcher::getConnectorInMap(const QString & sessionID){

    mutex.lock();
    auto iter = clientList.find(sessionID);

    if(iter != clientList.end()) {
        auto connector = iter->second;
        mutex.unlock();
        return connector;
    }

    mutex.unlock();
    return nullptr;
}

void SessionDispatcher::setConnectorInMap(const QString & sessionID, IConnector *connector){
    mutex.lock();
    clientList[sessionID] = connector;
    mutex.unlock();
}

// void SessionDispatcher::jsViewUpdatedSignalForwardSlot(const QString & sessionID, const QString & viewName, const QString & img, qint64 id){
//     emit jsViewUpdatedSignal(sessionID, viewName, img, id);
// }


// void SessionDispatcher::jsCommandResultsSignalForwardSlot(const QString & sessionID, const QString & senderSession, const QString & cmd, const QString & results, const QString & subIdentifier){
//     emit jsCommandResultsSignal(sessionID, senderSession, cmd, results, subIdentifier);
// }

// //TODO implement later
// void SessionDispatcher::jsSendKeepAlive(){
// //    qDebug() << "get keepalive packet !!!!";
// }

// void SessionDispatcher::newSessionCreatedSlot(const QString & sessionID)
// {
//     qDebug() << "new Client Session !!!!";

//     if (getConnectorInMap(sessionID) != nullptr){
//         qDebug()<<"Find, the value is !!!!!";
//         return;
//     }

//     NewServerConnector *connector =  new NewServerConnector();

//     setConnectorInMap(sessionID, connector);

//     connect(connector, SIGNAL(startViewerSignal(const QString &)), connector, SLOT(startViewerSlot(const QString &)));
//     connect(connector,
//             SIGNAL(jsSendCommandSignal(const QString &, const QString &, const QString &, const QString &)),
//             connector,
//             SLOT(jsSendCommandSlot(const QString &, const QString &, const QString &, const QString &)));

//     // setup view size
//     connect(connector,
//             SIGNAL(jsUpdateViewSizeSignal(const QString &, const QString &, int, int) ),
//             connector,
//             SLOT( jsUpdateViewSizeSlot(const QString &, const QString &, int, int)));


//     connect(connector,
//             SIGNAL(jsCommandResultsSignal(const QString &, const QString &, const QString &, const QString &, const QString &)),
//             this,
//             SLOT(jsCommandResultsSignalForwardSlot(const QString &, const QString &, const QString &, const QString &, const QString &))
//             );

//     connect(connector,
//             SIGNAL(jsViewUpdatedSignal(const QString &,  const QString &, const QString &, qint64)),
//             this,
//             SLOT(jsViewUpdatedSignalForwardSlot(const QString &, const QString &, const QString &, qint64))
//             );

//     // create a simple thread
//     QThread* newThread = new QThread();
// //        connector->selfThread = newThread;

//     // let the new thread handle its events
//     connector->moveToThread(newThread);
//     newThread->setObjectName(sessionID);

//     // start new thread's event loop
//     newThread->start();

//     //trigger signal
//     emit connector->startViewerSignal(sessionID);
// }

// void SessionDispatcher::jsUpdateViewSizeSlot(const QString & sessionID, const QString & viewName, int width, int height)
// {
//     // forward commands
//     NewServerConnector *connector = static_cast<NewServerConnector*>(getConnectorInMap(sessionID));
//     if (connector != nullptr){
//         emit connector->jsUpdateViewSizeSignal(sessionID, viewName, width, height);
//     }

//     return;
// }

//********* will comment the below later

void SessionDispatcher::initialize(const InitializeCallback & cb)
{
}

void SessionDispatcher::setState(const QString& path, const QString & newValue)
{

}

QString SessionDispatcher::getState(const QString & path  )
{
    return "";
}

QString SessionDispatcher::getStateLocation( const QString& saveName ) const {
    return "";
}

IConnector::CallbackID SessionDispatcher::addCommandCallback(
        const QString & cmd,
        const IConnector::CommandCallback & cb)
{
    return 0;
}

IConnector::CallbackID SessionDispatcher::addMessageCallback(
        const QString & cmd,
        const IConnector::MessageCallback & cb)
{
    return 0;
}

IConnector::CallbackID SessionDispatcher::addStateCallback(
        IConnector::CSR path,
        const IConnector::StateChangedCallback & cb)
{
    return 0;
}

void SessionDispatcher::registerView(IView * view)
{

}

void SessionDispatcher::unregisterView( const QString& viewName ){
}

qint64 SessionDispatcher::refreshView(IView * view)
{
    return 0;
}

void SessionDispatcher::removeStateCallback(const IConnector::CallbackID & /*id*/)
{
    qFatal( "not implemented");
}

Carta::Lib::IRemoteVGView * SessionDispatcher::makeRemoteVGView(QString viewName)
{
    return nullptr;
}
//*********
