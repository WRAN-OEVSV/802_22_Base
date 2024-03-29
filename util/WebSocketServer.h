/** --------------------------------------------------------------------------
 *  WebSocketServer.h
 *
 *  Base class that WebSocket implementations must inherit from.  Handles the
 *  client connections and calls the child class callbacks for connection
 *  events like onConnect, onMessage, and onDisconnect.
 *
 *  Author    : Jason Kruse <jason@jasonkruse.com> or @mnisjk
 *  Copyright : 2014
 *  License   : BSD (see LICENSE)
 *  --------------------------------------------------------------------------
 **/

//#pragma once

#ifndef _WEBSOCKETSERVER_H
#define _WEBSOCKETSERVER_H
#include <cstdint>
#include <map>
#include <queue>
#include <string>
#include <list>
#include <cstdio>
#include <ctime>
#include <sys/time.h>
#include <iostream>
#include <sstream>
#include <vector>
#include "libwebsockets.h"
#include "User.h"

using namespace std;



// Represents a client connection
class Connection
{
    queue<string>       buffer;     // Ordered list of pending messages to flush out when socket is writable
    map<string,string> keyValueMap;
    time_t             createTime;
    string user{"anonymous"};

public:

    time_t getCreateTime() const;

    void setCreateTime(time_t createTime);

    const string &getUser() const;

    queue<string> * getBuffer();

    void setUser(const string &user);
    void push_to_buffer(const string & buffer);
    string& operator[](const string & key);
};

/// WebSocketServer
/// ---------
class WebSocketServer
{
public:

    // Manages connections. Unfortunately this is public because static callback for
    // libwebsockets is defined outside the instance and needs access to it.
    map<int,Connection*> connections;

    // Constructor / Destructor
    WebSocketServer( int port, const string & certPath = "", const string& keyPath = "" );
    ~WebSocketServer( );

    void run(       uint64_t timeout = 50     );
    void wait(      uint64_t timeout = 50     );
    void send(      int socketID, const string& data );
    void broadcast( const string& data               );
    void broadcast_log(const string& data);

    // Key => value storage for each connection
    string getValue( int socketID, const string& name );
    void   setValue( int socketID, const string& name, const string& value );
    int    getNumberOfConnections( );

    // Overridden by children
    virtual void onConnect(    int socketID                        ) = 0;
    virtual void onMessage(    int socketID, const string& data    ) = 0;
    virtual void onDisconnect( int socketID                        ) = 0;
    virtual void   onError(    int socketID, const string& message ) = 0;


    // Wrappers, so we can take care of some maintenance
    void onConnectWrapper(    int socketID );
    void onDisconnectWrapper( int socketID );
    void onErrorWrapper( int socketID, const string& message );

    /**
     * Try to authenticate the user against the user database.
     * @param socketId socket id (if it is a positive integer, we will notify the user)
     * @param user user name
     * @param pass user password
     * @return true on success
     */
    bool authenticate(int socketId, const std::string & user, const std::string & pass);

protected:
    // Nothing, yet.

    map<string, User> users;
private:
    int                  _port;
    string               _keyPath;
    string               _certPath;
    struct lws_context  *_context;

    void _removeConnection( int socketID );
};

extern WebSocketServer *webSocketServer;
// WebSocketServer.h
#endif
