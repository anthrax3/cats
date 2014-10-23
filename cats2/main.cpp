#include "cats.hpp"
#include <QtWidgets/QApplication>
#include <QStyleFactory>

signed int
main(signed int ac, char** av) 
{
	QCoreApplication::setOrganizationName("NO!SOFT");
	QCoreApplication::setOrganizationDomain("https://www.idunno.com/nosoft/");
	QCoreApplication::setApplicationName("CATS");
	QApplication::setStyle(QStyleFactory::create("Fusion"));

	QApplication a(ac, av);
	cats_t w;
	w.show();
	return a.exec();
}
