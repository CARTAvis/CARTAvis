// I suspect somewhere down the road we'll want to override notify() or some
// other functionality of QApplication, so we might as well provision for it by
// subclassing QApplication and using the subclass...
//
// The only code that should go into this class is Qt specific stuff that needs
// access to the internals of QApplication.

#pragma once

#include <QApplication>
#include <functional>
#include "IConnector.h"


// also, right now I use this to hold globally available functions/data

class MyQApp : public QApplication
{
    Q_OBJECT
public:
    explicit MyQApp(int & argc, char **argv);

//    virtual bool notify( QObject * obj, QEvent * event) Q_DECL_OVERRIDE ;

    /**
     * @brief set the platform
     * @param return the platform
     */

protected:

};


// convenience function for deferred function calls

///
/// \brief defer
/// \param function
///
void defer( const std::function< void()> & function );


///
/// \brief Internal class to implement defer()
///
class DeferHelper2 : public QObject
{
    Q_OBJECT

public:

    typedef std::function<void ()> VoidFunc;

//    DeferHelper2();

//    using QObject::QObject;

    void queue( const VoidFunc & func);

public slots:
    void execute( const VoidFunc & func);

};

