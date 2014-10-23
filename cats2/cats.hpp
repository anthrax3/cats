#ifndef CATS2_HPP
#define CATS2_HPP

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <QtWidgets/QMenu>
#include <QtWidgets/QAction>
#include <QtWidgets/QTabWidget>

#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>
#include <QFile>
#include <QtConcurrent/QtConcurrentRun>
#include <QFutureWatcher>
#include <QFuture>
#include <QFileInfo>
#include <QMap>
#include <QVector>
#include <QPair>

#include <limits>
#include <algorithm>
#include <exception>
#include <array>
#include <chrono>

#include "plot.hpp"
#include "spectrogram.hpp"
#include "fsearch.hpp"
#include "signal.hpp"
#include "mp3.hpp"
#include "mel_filter.hpp"
#include "fft_factory.hpp"
#include "dct.hpp"
#include "mfcc.hpp"
#include "db.hpp"
#include "perceptual.hpp"
#include "feature_compare.hpp"

#include "filter_noise.hpp"

class cats_t : public QMainWindow
{
	public slots:
		void append(QString);
		void setText(QString);
		void updateStatus(QString, signed int to = 0);
		void hasError(QString);
		void fftFinished(void);
		void dbFinished(void);

	private slots:
		void openFile(void);
		void openDir(void);
		void exitApplication(void);

	private:
		QMutex							m_mutex;
		QWidget*						m_central;
		QVBoxLayout*					m_layout;
		QTabWidget*						m_tabs;
		QTextEdit*						m_text;
		QMenuBar*						m_menu;
		QStatusBar*						m_status;
		QMenu*							m_file;
		QAction*						m_openFile;
		QAction*						m_openDir;
		QAction*						m_exit;
		feature_compare_t				m_compare;
		std::chrono::time_point<std::chrono::system_clock> m_start;

		//signal_samples_vector_t*		m_signals;
		//feature_map_t*					m_features;

		QFutureWatcher< void >*			m_watcher;
		db_t*							m_db;

	protected:
		signal_t< double > decodeFile(QString);
		void encodeFile(signal_t< double >&, QString);

	public:
		cats_t(void);
		~cats_t(void);	

};


#endif
